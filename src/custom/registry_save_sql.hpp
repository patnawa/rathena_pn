#ifndef PN_REGISTRY_SAVE_SQL_HPP
#define PN_REGISTRY_SAVE_SQL_HPP
#include <common/sql.hpp>
#include <custom/global_point.hpp>
#include <custom/registry_save.hpp>
#include <cstdlib>
#include <algorithm>

namespace pn_registry {
struct Tables { const char* character_num; const char* character_str; const char* account_num; const char* account_str; };
inline std::string session_where(const Identity& id) {
	return "account_id=" + std::to_string(id.account) + " AND char_id=" + std::to_string(id.character) +
		" AND nonce_hi=" + std::to_string(id.high) + " AND nonce_lo=" + std::to_string(id.low) + " AND scope=" + std::to_string(id.scope);
}
// Receipt and values share a transaction. A failed or ambiguous COMMIT is retried
// with the identical bytes; a matching receipt never writes the values again.
inline bool save(Sql* db, const void* data, size_t length, const Tables& tables, bool owner) {
	std::vector<Entry> entries;
	if (!decode(data, length, entries) || Sql_BeginTransaction(db) != SQL_SUCCESS) return false;
	const auto id = identity(data);
	const auto session = session_where(id);
	const auto where = session + " AND sequence=" + std::to_string(id.sequence);
	bool ok = false;
	do {
		if (Sql_Query(db, "SELECT payload FROM pn_registry_saves WHERE %s FOR UPDATE", where.c_str()) != SQL_SUCCESS) break;
		int row = Sql_NextRow(db); char* stored = nullptr; size_t size = 0;
		const bool duplicate = row == SQL_SUCCESS && Sql_GetData(db, 0, &stored, &size) == SQL_SUCCESS && stored && size == length && !std::memcmp(stored, data, length);
		Sql_FreeResult(db);
		// Hold a metadata lock before checking the engine, including duplicate ACKs.
		if (!pn_global_point::transactional(db, "pn_registry_saves")) break;
		if (row == SQL_SUCCESS) { ok = duplicate; break; }
		if (row != SQL_NO_DATA || !owner) break;
		if (Sql_Query(db, "SELECT sequence FROM pn_registry_saves WHERE %s ORDER BY sequence DESC LIMIT 1 FOR UPDATE", session.c_str()) != SQL_SUCCESS) break;
		row = Sql_NextRow(db); uint64_t previous = 0;
		if (row == SQL_SUCCESS && Sql_GetData(db, 0, &stored, nullptr) == SQL_SUCCESS && stored) previous = std::strtoull(stored, nullptr, 10);
		Sql_FreeResult(db);
		if ((row != SQL_SUCCESS && row != SQL_NO_DATA) || previous == UINT64_MAX || previous + 1 != id.sequence) break;
		ok = true;
		std::vector<std::string> checked_tables;
		for (const auto& entry : entries) {
			const bool account = entry.key[0] == '#';
			const char* table = account ? (entry.kind >= 2 ? tables.account_str : tables.account_num) : (entry.kind >= 2 ? tables.character_str : tables.character_num);
			const char* column = account ? "account_id" : "char_id";
			const uint32_t target = account ? id.account : id.character;
			char key[65]{}, value[509]{};
			Sql_EscapeStringLen(db, key, entry.key.data(), entry.key.size());
			Sql_EscapeStringLen(db, value, entry.text.data(), entry.text.size());
			if (!table || !pn_global_point::identifier(table) ||
				Sql_Query(db, "SELECT `value` FROM `%s` WHERE `%s`=%u AND `key`='%s' AND `index`=%u FOR UPDATE", table, column, target, key, entry.index) != SQL_SUCCESS) { ok = false; break; }
			Sql_FreeResult(db);
			if (std::find(checked_tables.begin(), checked_tables.end(), table) == checked_tables.end()) {
				if (!pn_global_point::transactional(db, table)) { ok = false; break; }
				checked_tables.emplace_back(table);
			}
			if (id.scope && entry.index == 0 && entry.kind < 2 && pn_global_point::pending(db, id.account, entry.key.c_str())) { ok = false; break; }
			if (entry.kind == 1 || entry.kind == 3) {
				ok = Sql_Query(db, "DELETE FROM `%s` WHERE `%s`=%u AND `key`='%s' AND `index`=%u", table, column, target, key, entry.index) == SQL_SUCCESS;
			} else if (entry.kind == 0) {
				ok = Sql_Query(db, "REPLACE INTO `%s` (`%s`,`key`,`index`,`value`) VALUES(%u,'%s',%u,%lld)", table, column, target, key, entry.index, static_cast<long long>(entry.number)) == SQL_SUCCESS;
			} else {
				ok = Sql_Query(db, "REPLACE INTO `%s` (`%s`,`key`,`index`,`value`) VALUES(%u,'%s',%u,'%s')", table, column, target, key, entry.index, value) == SQL_SUCCESS;
			}
			if (!ok) break;
		}
		if (!ok) break;
		SqlStmt statement{*db};
		ok = statement.Prepare("INSERT INTO pn_registry_saves(account_id,char_id,nonce_hi,nonce_lo,scope,sequence,payload) VALUES(%u,%u,%llu,%llu,%u,%llu,?)", id.account, id.character,
			static_cast<unsigned long long>(id.high), static_cast<unsigned long long>(id.low), id.scope, static_cast<unsigned long long>(id.sequence)) == SQL_SUCCESS &&
			statement.BindParam(0, SQLDT_BLOB, const_cast<void*>(data), length) == SQL_SUCCESS && statement.Execute() == SQL_SUCCESS;
	} while (false);
	return Sql_EndTransaction(db, ok) == SQL_SUCCESS && ok;
}
}
#endif
