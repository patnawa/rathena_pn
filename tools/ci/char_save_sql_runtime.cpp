// Links the production character writer against disposable MariaDB only.
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <char/char.hpp>
#include <char/inter.hpp>
#include <common/malloc.hpp>
#include <common/sql.hpp>
#include <common/timer.hpp>

static unsigned checks = 0;
static constexpr uint32 cid = 91001, aid = 92001;
static void require(bool ok, const char* label) {
    ++checks; if (!ok) { std::cerr << "FAIL " << label << std::endl; std::exit(1); }
}
static void sql(const std::string& query) {
    require(Sql_QueryStr(sql_handle, query.c_str()) == SQL_SUCCESS, query.c_str());
}
static std::string result(const std::string& query) {
    sql(query); std::string value; int32 row;
    while ((row = Sql_NextRow(sql_handle)) == SQL_SUCCESS) {
        for (uint32 i = 0; i < Sql_NumColumns(sql_handle); ++i) {
            char* data = nullptr; size_t length = 0;
            require(Sql_GetData(sql_handle, i, &data, &length) == SQL_SUCCESS, "SQL cell");
            value += data ? std::string(data, length) : "NULL"; value += '|';
        }
        value += '\n';
    }
    require(row == SQL_NO_DATA, "complete SQL result"); Sql_FreeResult(sql_handle); return value;
}
static std::string snapshot() {
    std::string value;
    for (const auto* query : {
        "SELECT * FROM `char` ORDER BY char_id", "SELECT * FROM memo ORDER BY memo_id",
        "SELECT * FROM skill ORDER BY char_id,id", "SELECT * FROM friends ORDER BY char_id,friend_id",
        "SELECT * FROM mercenary_owner ORDER BY char_id", "SELECT * FROM hotkey ORDER BY char_id,hotkey"})
        value += result(query);
    return value;
}
static mmo_charstatus base() {
    mmo_charstatus p{}; p.char_id = cid; p.account_id = aid; p.base_level = 5;
    std::strcpy(p.name, "Status fixture");
    std::strcpy(p.memo_point[0].map, "prontera"); p.memo_point[0].x = 10; p.memo_point[0].y = 20;
    p.skill[0].id = 1; p.skill[0].lv = 1; p.skill[0].flag = SKILL_FLAG_PERMANENT;
    p.friends[0].account_id = aid + 1; p.friends[0].char_id = cid + 1;
    p.mer_id = 1;
#ifdef HOTKEY_SAVING
    p.hotkeys[0].id = 1; p.hotkeys[0].lv = 1;
#endif
    return p;
}
static mmo_charstatus request() {
    auto p = base();
    char_get_chardb().clear(); char_get_chardb()[cid] = std::make_shared<mmo_charstatus>(p);
    p.base_level = 12; p.hair = 3; p.mer_id = 2; p.memo_point[0].x = 30;
    p.skill[0].lv = 10; p.friends[0].char_id = cid + 2;
#ifdef HOTKEY_SAVING
    p.hotkeys[0].lv = 10;
#endif
    return p;
}
static void seed() {
    sql("DROP TRIGGER IF EXISTS character_fault");
    for (const auto* table : {"char", "memo", "skill", "friends", "mercenary_owner", "hotkey"})
        sql(std::string("DELETE FROM `") + table + "`");
    sql("INSERT INTO `char`(char_id,account_id,name,base_level) VALUES(91001,92001,'Status fixture',5),(91002,92002,'Other fixture',7)");
    sql("INSERT INTO memo(memo_id,char_id,map,x,y) VALUES(1,91001,'prontera',10,20),(2,91002,'izlude',40,50)");
    sql("INSERT INTO skill(char_id,id,lv) VALUES(91001,1,1),(91002,2,7)");
    sql("INSERT INTO friends(char_id,friend_id) VALUES(91001,91002),(91002,91001)");
    sql("INSERT INTO mercenary_owner(char_id,merc_id) VALUES(91001,1),(91002,7)");
    sql("INSERT INTO hotkey(char_id,hotkey,itemskill_id,skill_lvl) VALUES(91001,0,1,1),(91002,0,2,7)");
}
static void saved() {
    require(result("SELECT base_level,hair FROM `char` WHERE char_id=91001") == "12|3|\n", "scalar fields saved");
    require(result("SELECT x,y FROM memo WHERE char_id=91001") == "30|20|\n", "memo saved");
    require(result("SELECT lv FROM skill WHERE char_id=91001") == "10|\n", "skills saved");
    require(result("SELECT friend_id FROM friends WHERE char_id=91001") == "91003|\n", "friends saved");
    require(result("SELECT merc_id FROM mercenary_owner WHERE char_id=91001") == "2|\n", "mercenary saved");
#ifdef HOTKEY_SAVING
    require(result("SELECT skill_lvl FROM hotkey WHERE char_id=91001") == "10|\n", "hotkeys saved");
#endif
    require(result("SELECT base_level FROM `char` WHERE char_id=91002") == "7|\n", "other character preserved");
}
extern "C" int __wrap_main(int argc, char** argv) {
    malloc_init(); timer_init(); sql_handle = Sql_Malloc();
    require(Sql_Connect(sql_handle, "root", "character-fixture-only", "character-save-db", 3306,
                        "character_save_probe") == SQL_SUCCESS, "connect isolated database");
    require(result("SELECT DATABASE()") == "character_save_probe|\n", "fixture database guard");
    std::strcpy(schema_config.char_db, "char"); std::strcpy(schema_config.memo_db, "memo");
    std::strcpy(schema_config.skill_db, "skill"); std::strcpy(schema_config.friend_db, "friends");
    std::strcpy(schema_config.hotkey_db, "hotkey"); std::strcpy(schema_config.mercenary_owner_db, "mercenary_owner");
    const std::string mode = argc > 1 ? argv[1] : "normal";
    if (mode == "crash-setup") {
        seed();
        sql("CREATE TRIGGER character_fault BEFORE INSERT ON hotkey FOR EACH ROW DO SLEEP(60)");
    } else if (mode == "crash") {
        auto p = request();
        require(char_mmo_char_tosql(cid, &p) != 0, "database crash must report failure");
        require(char_get_chardb()[cid]->base_level == 5, "database crash must retain old cache");
    } else if (mode == "crash-verify") {
        sql("DROP TRIGGER character_fault");
        require(result("SELECT base_level,hair FROM `char` WHERE char_id=91001") == "5|0|\n", "scalar rollback after crash");
        require(result("SELECT x,y FROM memo WHERE char_id=91001") == "10|20|\n", "memo rollback after crash");
        require(result("SELECT lv FROM skill WHERE char_id=91001") == "1|\n", "skill rollback after crash");
        require(result("SELECT friend_id FROM friends WHERE char_id=91001") == "91002|\n", "friend rollback after crash");
        require(result("SELECT merc_id FROM mercenary_owner WHERE char_id=91001") == "1|\n", "mercenary rollback after crash");
        auto p = request(); require(char_mmo_char_tosql(cid, &p) == 0, "restart retry succeeds"); saved();
    } else if (mode == "normal") {
        const std::vector<std::string> faults = {
            "BEFORE UPDATE ON `char`", "BEFORE DELETE ON memo", "BEFORE INSERT ON memo",
            "BEFORE DELETE ON skill", "BEFORE INSERT ON skill", "BEFORE DELETE ON friends",
            "BEFORE INSERT ON friends", "BEFORE INSERT ON mercenary_owner", "BEFORE INSERT ON hotkey"};
        for (const auto& fault : faults) {
            seed(); const auto before = snapshot(); auto p = request();
            sql("CREATE TRIGGER character_fault " + fault + " FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='character save fault'");
            require(char_mmo_char_tosql(cid, &p) != 0, "statement failure must be reported");
            require(snapshot() == before, "statement failure rolls back all six tables");
            require(char_get_chardb()[cid]->base_level == 5, "failed save cannot update cache");
            sql("DROP TRIGGER character_fault");
            require(char_mmo_char_tosql(cid, &p) == 0, "retry after statement failure"); saved();
            const auto after = snapshot();
            require(char_mmo_char_tosql(cid, &p) == 0, "duplicate save accepted");
            require(snapshot() == after, "duplicate save changes no rows");
        }
        for (const auto* table : {"char", "memo", "skill", "friends", "mercenary_owner", "hotkey"}) {
            seed(); auto p = request();
            sql(std::string("ALTER TABLE `") + table + "` ENGINE=MyISAM");
            const auto before = snapshot();
            require(char_mmo_char_tosql(cid, &p) != 0, "nontransactional participant refused");
            require(snapshot() == before, "engine refusal changes no rows");
            sql(std::string("ALTER TABLE `") + table + "` ENGINE=InnoDB");
        }
        seed(); auto p = request(); const auto before = snapshot(); p.account_id++;
        require(char_mmo_char_tosql(cid, &p) != 0, "database ownership mismatch refused");
        require(snapshot() == before, "ownership rejection preserves all rows");
    } else require(false, "unknown SQL test mode");
    char_get_chardb().clear(); Sql_Free(sql_handle); sql_handle = nullptr; timer_final(); malloc_final();
    std::cout << "CHARACTER_SAVE_SQL_OK mode=" << mode << " checks=" << checks << std::endl;
    return 0;
}
