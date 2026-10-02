// Executes the production receipt writer on disposable, separate char/login DBs.
#include <custom/registry_save_sql.hpp>
#include <common/malloc.hpp>
#include <common/timer.hpp>
#include <iostream>

static Sql* db;
static unsigned checks = 0;
static void require(bool ok, const char* label) {
    ++checks; if (!ok) { std::cerr << "FAIL " << label << std::endl; std::exit(1); }
}
static void sql(const std::string& query) { require(Sql_QueryStr(db, query.c_str()) == SQL_SUCCESS, query.c_str()); }
static std::string result(const std::string& query) {
    sql(query); std::string value; int row;
    while ((row = Sql_NextRow(db)) == SQL_SUCCESS) {
        for (uint32 i = 0; i < Sql_NumColumns(db); ++i) {
            char* data = nullptr; size_t length = 0;
            require(Sql_GetData(db, i, &data, &length) == SQL_SUCCESS, "SQL cell");
            value += data ? std::string(data, length) : "NULL"; value += '|';
        }
        value += '\n';
    }
    require(row == SQL_NO_DATA, "complete SQL result"); Sql_FreeResult(db); return value;
}
static const pn_registry::Tables local{"char_reg_num", "char_reg_str", "acc_reg_num", "acc_reg_str"};
static const pn_registry::Tables global{nullptr, nullptr, "global_acc_reg_num", "global_acc_reg_str"};
static bool save(const pn_registry::Packet& packet, bool owner = true) {
    return pn_registry::save(db, packet.data(), packet.size(), packet[6] ? global : local, owner);
}
static pn_registry::Packet packet(unsigned scope, uint64_t sequence = 1, bool deletion = false) {
    pn_registry::Packet p(pn_registry::header_size); p[6] = scope;
    pn_registry::write<uint16_t>(p.data(), 0, scope ? pn_registry::login_request : pn_registry::request_packet);
    pn_registry::write<uint16_t>(p.data(), 4, pn_registry::version);
    pn_registry::write<uint32_t>(p.data(), 8, 92001); pn_registry::write<uint32_t>(p.data(), 12, 91001);
    pn_registry::write<uint64_t>(p.data(), 16, 101); pn_registry::write<uint64_t>(p.data(), 24, 202);
    pn_registry::write<uint64_t>(p.data(), 32, sequence);
    require(pn_registry::append_entry(p, {scope ? "##Point" : "#CASHPOINTS", "", 0, 100 + int64_t(sequence), uint8_t(deletion ? 1 : 0)}), "encode numeric account entry");
    require(pn_registry::append_entry(p, {scope ? "##Preset$" : "#Preset$", "O'Brien's \\ preset", 2, 0, uint8_t(deletion ? 3 : 2)}), "encode string account entry");
    if (!scope) {
        require(pn_registry::append_entry(p, {"QuestStep", "", 3, 17 + int64_t(sequence), uint8_t(deletion ? 1 : 0)}), "encode character number");
        require(pn_registry::append_entry(p, {"LabSetup$", "Normal target", 0, 0, uint8_t(deletion ? 3 : 2)}), "encode character string");
    }
    pn_registry::write<uint16_t>(p.data(), 2, p.size()); return p;
}
static std::vector<std::string> tables(unsigned scope) {
    return scope ? std::vector<std::string>{"global_acc_reg_num", "global_acc_reg_str", "pn_registry_saves"} :
        std::vector<std::string>{"char_reg_num", "char_reg_str", "acc_reg_num", "acc_reg_str", "pn_registry_saves"};
}
static void connect(unsigned scope) {
    if (db) Sql_Free(db); db = Sql_Malloc();
    require(Sql_Connect(db, "root", "registry-fixture-only", "registry-save-db", 3306,
        scope ? "registry_login_probe" : "registry_save_probe") == SQL_SUCCESS, "connect isolated database");
    require(result("SELECT DATABASE()") == (scope ? "registry_login_probe|\n" : "registry_save_probe|\n"), "fixture guard");
}
static void reset(unsigned scope) {
    sql("DROP TRIGGER IF EXISTS registry_fault");
    for (const auto& table : tables(scope)) sql("DELETE FROM " + table);
}
static std::string snapshot(unsigned scope) {
    std::string value;
    for (const auto& table : tables(scope)) value += result("SELECT * FROM " + table + " ORDER BY 1,2");
    return value;
}
extern "C" int __wrap_main(int argc, char** argv) {
    malloc_init(); timer_init(); connect(0);
    const std::string mode = argc > 1 ? argv[1] : "normal";
    if (mode == "crash-setup") {
        reset(0); require(save(packet(0)), "seed crash fixture");
        sql("CREATE TRIGGER registry_fault BEFORE INSERT ON pn_registry_saves FOR EACH ROW DO SLEEP(60)");
    } else if (mode == "crash") {
        require(!save(packet(0, 2)), "database kill must refuse success");
    } else if (mode == "crash-verify") {
        sql("DROP TRIGGER registry_fault");
        require(result("SELECT value FROM acc_reg_num") == "101|\n", "currency rolled back after crash");
        require(result("SELECT value FROM char_reg_num") == "18|\n", "quest rolled back after crash");
        require(result("SELECT sequence FROM pn_registry_saves") == "1|\n", "no false receipt after crash");
        require(save(packet(0, 2)), "restart retry commits");
        sql("UPDATE acc_reg_num SET value=90"); require(save(packet(0, 2)), "lost ACK replay");
        require(result("SELECT value FROM acc_reg_num") == "90|\n", "restart replay cannot restore spent points");
    } else if (mode == "normal") {
        for (unsigned scope = 0; scope < 2; ++scope) {
            connect(scope); reset(scope);
            auto first = packet(scope), second = packet(scope, 2);
            const auto empty = snapshot(scope);
            require(!save(first, false), "new displaced writer rejected");
            require(!save(second), "sequence gap refused"); require(snapshot(scope) == empty, "refusals change no tables");
            require(save(first), "first save commits"); require(save(second), "ordered second save commits");
            const auto settled = snapshot(scope);
            require(save(first, false), "already committed displaced replay only acknowledges receipt");
            require(snapshot(scope) == settled, "old acknowledged request cannot overwrite newer edit");
            auto altered = first; altered.back() ^= 1;
            require(!save(altered), "same identity with changed payload rejected");
            const std::string numeric = scope ? "global_acc_reg_num" : "acc_reg_num";
            sql("UPDATE " + numeric + " SET value=90");
            require(save(second), "lost ACK retried after later debit");
            require(result("SELECT value FROM " + numeric) == "90|\n", "old save cannot restore spent currency");
            require(save(packet(scope, 3, true)), "numeric and string deletions commit");
            for (const auto& table : tables(scope)) if (table != "pn_registry_saves") require(result("SELECT COUNT(*) FROM " + table) == "0|\n", "deletion persisted");
            for (const auto& table : tables(scope)) {
                reset(scope); require(save(first), "fault fixture baseline"); const auto before = snapshot(scope);
                sql("CREATE TRIGGER registry_fault BEFORE INSERT ON " + table + " FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='registry save fault'");
                require(!save(second), "SQL failure cannot acknowledge"); require(snapshot(scope) == before, "all values and receipt rolled back");
                sql("DROP TRIGGER registry_fault"); require(save(second), "retry succeeds after SQL fault");
                reset(scope); sql("ALTER TABLE " + table + " ENGINE=MyISAM"); const auto original = snapshot(scope);
                require(!save(first), "nontransactional participant refused"); require(snapshot(scope) == original, "engine refusal changes no rows");
                sql("ALTER TABLE " + table + " ENGINE=InnoDB");
            }
        }
    } else require(false, "unknown mode");
    Sql_Free(db); db = nullptr; timer_final(); malloc_final();
    std::cout << "REGISTRY_SAVE_SQL_OK mode=" << mode << " checks=" << checks << std::endl; return 0;
}
