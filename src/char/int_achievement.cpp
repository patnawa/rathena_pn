// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "int_achievement.hpp"

#include <custom/achievement_protocol.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <unordered_set>
#include <vector>

#include <common/db.hpp>
#include <common/malloc.hpp>
#include <common/mmo.hpp>
#include <common/showmsg.hpp>
#include <common/socket.hpp>
#include <common/sql.hpp>
#include <common/strlib.hpp>

#include "char.hpp"
#include "inter.hpp"
#include "int_mail.hpp"

/**
 * Load achievements for a character.
 * @param char_id: Character ID
 * @param count: Pointer to return the number of found entries.
 * @return Array of found entries. It has *count entries, and it is care of the caller to aFree() it afterwards.
 */
struct achievement *mapif_achievements_fromsql(uint32 char_id, int32 *count, bool* success = nullptr, bool lock = false)
{
	if (success) *success = false;
	struct achievement *achievelog = nullptr;
	struct achievement tmp_achieve;
	SqlStmt stmt{ *sql_handle };
	StringBuf buf;
	int32 i;

	if (!count)
		return nullptr;

	memset(&tmp_achieve, 0, sizeof(tmp_achieve));

	StringBuf_Init(&buf);
	StringBuf_AppendStr(&buf, "SELECT `id`, COALESCE(UNIX_TIMESTAMP(`completed`),0), COALESCE(UNIX_TIMESTAMP(`rewarded`),0)");
	for (i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
		StringBuf_Printf(&buf, ", `count%d`", i + 1);
	StringBuf_Printf(&buf, " FROM `%s` WHERE `char_id` = '%u'", schema_config.achievement_table, char_id);
	// Snapshot replacement holds both row/gap locks and the table metadata lock
	// until its transaction ends. Ordinary loads keep their existing read path.
	if (lock) StringBuf_AppendStr(&buf, " FOR UPDATE");

	if( SQL_ERROR == stmt.PrepareStr(StringBuf_Value(&buf))
	||  SQL_ERROR == stmt.Execute() )
	{
		SqlStmt_ShowDebug(stmt);
		*count = 0;
		return nullptr;
	}

	stmt.BindColumn(0, SQLDT_INT32, &tmp_achieve.achievement_id);
	stmt.BindColumn(1, SQLDT_INT32, &tmp_achieve.completed);
	stmt.BindColumn(2, SQLDT_INT32, &tmp_achieve.rewarded);
	for (i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
		stmt.BindColumn(3 + i, SQLDT_INT32, &tmp_achieve.count[i]);

	*count = (int32)stmt.NumRows();
	if (*count > 0) {
		i = 0;

		achievelog = (struct achievement *)aCalloc(*count, sizeof(struct achievement));
		int32 row_status;
		while (SQL_SUCCESS == (row_status = stmt.NextRow())) {
			if (i >= *count) // Sanity check, should never happen
				break;
			memcpy(&achievelog[i++], &tmp_achieve, sizeof(tmp_achieve));
		}
		if (row_status != SQL_NO_DATA) {
			aFree(achievelog);
			*count = 0;
			return nullptr;
		}
		if (i < *count) {
			// Should never happen. Compact array
			*count = i;
			achievelog = (struct achievement *)aRealloc(achievelog, sizeof(struct achievement) * i);
		}
	}

	if (success) *success = true;
	ShowInfo("achievement load complete from DB - id: %d (total: %d)\n", char_id, *count);

	return achievelog;
}

/**
 * Deletes an achievement from a character's achievementlog.
 * @param char_id: Character ID
 * @param achievement_id: Achievement ID
 * @return false in case of errors, true otherwise
 */
bool mapif_achievement_delete(uint32 char_id, int32 achievement_id)
{
	if (SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `id` = '%d' AND `char_id` = '%u'", schema_config.achievement_table, achievement_id, char_id)) {
		Sql_ShowDebug(sql_handle);
		return false;
	}

	return true;
}

/**
 * Adds an achievement to a character's achievementlog.
 * @param char_id: Character ID
 * @param ad: Achievement data
 * @return false in case of errors, true otherwise
 */
bool mapif_achievement_add(uint32 char_id, const struct achievement* ad)
{
	StringBuf buf;
	int32 i;

	ARR_FIND( 0, MAX_ACHIEVEMENT_OBJECTIVES, i, ad->count[i] != 0 );

	if( i == MAX_ACHIEVEMENT_OBJECTIVES && ad->completed == 0 && ad->rewarded == 0 ){
		// Do not save
		return true;
	}

	StringBuf_Init(&buf);
	StringBuf_Printf(&buf, "INSERT INTO `%s` (`char_id`, `id`, `completed`, `rewarded`", schema_config.achievement_table);
	for (i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
		StringBuf_Printf(&buf, ", `count%d`", i + 1);
	StringBuf_AppendStr(&buf, ")");
	StringBuf_Printf(&buf, " VALUES ('%u', '%d',", char_id, ad->achievement_id, (uint32)ad->completed, (uint32)ad->rewarded);
	if( ad->completed ){
		StringBuf_Printf(&buf, "FROM_UNIXTIME('%u'),", (uint32)ad->completed);
	}else{
		StringBuf_AppendStr(&buf, "NULL,");
	}
	if( ad->rewarded ){
		StringBuf_Printf(&buf, "FROM_UNIXTIME('%u')", (uint32)ad->rewarded);
	}else{
		StringBuf_AppendStr(&buf, "NULL");
	}
	for (i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
		StringBuf_Printf(&buf, ", '%d'", ad->count[i]);
	StringBuf_AppendStr(&buf, ")");

	if (SQL_ERROR == Sql_QueryStr(sql_handle, StringBuf_Value(&buf))) {
		Sql_ShowDebug(sql_handle);
		return false;
	}

	return true;
}

/**
 * Updates an achievement in a character's achievementlog.
 * @param char_id: Character ID
 * @param ad: Achievement data
 * @return false in case of errors, true otherwise
 */
bool mapif_achievement_update(uint32 char_id, const struct achievement* ad)
{
	StringBuf buf;
	int32 i;

	StringBuf_Init(&buf);
	StringBuf_Printf(&buf, "UPDATE `%s` SET ", schema_config.achievement_table);
	if( ad->completed ){
		StringBuf_Printf(&buf, "`completed` = FROM_UNIXTIME('%u'),", (uint32)ad->completed);
	}else{
		StringBuf_AppendStr(&buf, "`completed` = NULL,");
	}
	if( ad->rewarded ){
		StringBuf_Printf(&buf, "`rewarded` = FROM_UNIXTIME('%u')", (uint32)ad->rewarded);
	}else{
		StringBuf_AppendStr(&buf, "`rewarded` = NULL");
	}
	for (i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
		StringBuf_Printf(&buf, ", `count%d` = '%d'", i + 1, ad->count[i]);
	StringBuf_Printf(&buf, " WHERE `id` = %d AND `char_id` = %u", ad->achievement_id, char_id);

	if (SQL_ERROR == Sql_QueryStr(sql_handle, StringBuf_Value(&buf))) {
		Sql_ShowDebug(sql_handle);
		return false;
	}

	return true;
}

/**
 * Notifies the map-server of the result of saving a character's achievementlog.
 */
void mapif_achievement_save( int32 fd, uint32 char_id, bool success ){
	WFIFOHEAD(fd, 7);
	WFIFOW(fd, 0) = 0x3863;
	WFIFOL(fd, 2) = char_id;
	WFIFOB(fd, 6) = success;
	WFIFOSET(fd, 7);
}

static void mapif_achievement_logout_save(int32 fd, uint32 account_id, uint32 char_id,
	uint64 generation, bool success)
{
	WFIFOHEAD(fd, pn_achievement_protocol::logout_response_size);
	WFIFOW(fd, 0) = pn_achievement_protocol::logout_save_response;
	WFIFOB(fd, 2) = pn_achievement_protocol::version;
	WFIFOB(fd, 3) = success;
	WFIFOL(fd, 4) = account_id;
	WFIFOL(fd, 8) = char_id;
	memset(WFIFOP(fd, 12), 0, 4);
	WFIFOQ(fd, 16) = generation;
	WFIFOSET(fd, pn_achievement_protocol::logout_response_size);
}

/**
 * Handles the save request from mapserver for a character's achievementlog.
 * Received achievements are saved, and an ack is sent back to the map server.
 * @see inter_parse_frommap
 */
static bool mapif_achievement_rows_valid(uint32 char_id, const struct achievement* rows, int32 count)
{
	if (!char_id || count < 0 || (count && !rows)) return false;
	std::unordered_set<int32> identities;
	for (int32 i = 0; i < count; ++i) {
		if (rows[i].achievement_id <= 0 || rows[i].completed < 0 || rows[i].rewarded < 0 ||
			(rows[i].rewarded && !rows[i].completed) || !identities.insert(rows[i].achievement_id).second)
			return false;
		for (int32 value : rows[i].count)
			if (value < 0) return false;
	}
	return true;
}

static bool mapif_transactional_table_locked(const char* table)
{
	char* engine = nullptr;
	const bool transactional = Sql_Query(sql_handle, "SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'", table) == SQL_SUCCESS &&
		Sql_NextRow(sql_handle) == SQL_SUCCESS &&
		Sql_GetData(sql_handle, 0, &engine, nullptr) == SQL_SUCCESS && engine && !strcmpi(engine, "InnoDB");
	Sql_FreeResult(sql_handle);
	return transactional;
}

static bool mapif_achievement_persisted(const achievement& row)
{
	if (row.completed || row.rewarded) return true;
	for (int32 value : row.count)
		if (value) return true;
	return false;
}

static bool mapif_achievement_descends(const achievement& newer, const achievement& older)
{
	if (newer.achievement_id != older.achievement_id ||
		(older.completed && newer.completed != older.completed) ||
		(older.rewarded && newer.rewarded != older.rewarded))
		return false;
	for (int32 i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
		if (newer.count[i] < older.count[i]) return false;
	return true;
}

static bool mapif_achievement_merge_monotonic(uint32 char_id, const achievement* durable, int32 durable_count,
	const achievement* submitted, int32 submitted_count, std::vector<achievement>& merged)
{
	if (!mapif_achievement_rows_valid(char_id, durable, durable_count) || submitted_count < 0)
		return false;
	merged.clear();
	if (submitted_count)
		merged.assign(submitted, submitted + submitted_count);
	for (int32 i = 0; i < durable_count; ++i) {
		if (!mapif_achievement_persisted(durable[i]))
			continue;
		auto found = std::find_if(merged.begin(), merged.end(), [&](const achievement& row) {
			return row.achievement_id == durable[i].achievement_id;
		});
		if (found == merged.end()) {
			if (merged.size() == static_cast<size_t>(std::numeric_limits<int32>::max()))
				return false;
			merged.push_back(durable[i]);
			continue;
		}
		// SQL may already contain a shop progression commit or a reward whose ACK
		// was lost. An ordinary full snapshot must never roll those facts back.
		for (int32 objective = 0; objective < MAX_ACHIEVEMENT_OBJECTIVES; ++objective)
			found->count[objective] = std::max(found->count[objective], durable[i].count[objective]);
		if (durable[i].completed)
			found->completed = durable[i].completed;
		if (durable[i].rewarded)
			found->rewarded = durable[i].rewarded;
	}
	if (merged.size() > pn_achievement_protocol::max_logout_rows(sizeof(struct achievement)))
		return false;
	return mapif_achievement_rows_valid(char_id, merged.data(), static_cast<int32>(merged.size()));
}

static bool mapif_achievement_replace_locked(uint32 char_id, struct achievement* old_ad, int32 old_n,
	const struct achievement* rows, int32 new_n)
{
	int32 i, j, k;
	bool success = true;
	// The wire buffer is immutable; existing row helpers do not mutate input.
	const auto* new_ad = rows;

	for (i = 0; i < new_n; i++) {
		ARR_FIND(0, old_n, j, new_ad[i].achievement_id == old_ad[j].achievement_id);
		if (j < old_n) { // Update existing achievements
			// Only counts, complete, and reward are changable.
			ARR_FIND(0, MAX_ACHIEVEMENT_OBJECTIVES, k, new_ad[i].count[k] != old_ad[j].count[k]);
			if (k != MAX_ACHIEVEMENT_OBJECTIVES || new_ad[i].completed != old_ad[j].completed || new_ad[i].rewarded != old_ad[j].rewarded) {
				if ((success = mapif_achievement_update(char_id, &new_ad[i])) == false)
					break;
			}

			if (j < (--old_n)) {
				// Compact array
				memmove(&old_ad[j], &old_ad[j + 1], sizeof(struct achievement) * (old_n - j));
				memset(&old_ad[old_n], 0, sizeof(struct achievement));
			}
		} else { // Add new achievements
			if (new_ad[i].achievement_id) {
				if ((success = mapif_achievement_add(char_id, &new_ad[i])) == false)
					break;
			}
		}
	}

	for (i = 0; success && i < old_n; i++) { // Only erase rows after all writes succeeded.
		if ((success = mapif_achievement_delete(char_id, old_ad[i].achievement_id)) == false)
			break;
	}

	return success;
}

bool mapif_achievement_save_rows(uint32 char_id, const struct achievement* rows, int32 new_n)
{
	if (!mapif_achievement_rows_valid(char_id, rows, new_n) || Sql_BeginTransaction(sql_handle) != SQL_SUCCESS) return false;
	int32 old_n = 0;
	bool loaded = false;
	struct achievement *old_ad = mapif_achievements_fromsql(char_id, &old_n, &loaded, true);
	// A full snapshot includes deletions. Check the engine while the locked read
	// is held so an ALTER cannot cross the validation/write boundary.
	std::vector<achievement> merged;
	bool success = loaded && mapif_transactional_table_locked(schema_config.achievement_table) &&
		mapif_achievement_merge_monotonic(char_id, old_ad, old_n, rows, new_n, merged) &&
		mapif_achievement_replace_locked(char_id, old_ad, old_n, merged.data(), static_cast<int32>(merged.size()));
	if (old_ad) aFree(old_ad);

	const bool ended = Sql_EndTransaction(sql_handle, success) == SQL_SUCCESS;
	return success && ended;
}

AchievementCompareReplaceResult mapif_achievement_compare_replace_locked(uint32 char_id,
	const struct achievement* before, int32 before_count,
	const struct achievement* after, int32 after_count)
{
	if (!Sql_InTransaction(sql_handle) ||
		!mapif_achievement_rows_valid(char_id, before, before_count) ||
		!mapif_achievement_rows_valid(char_id, after, after_count))
		return AchievementCompareReplaceResult::Error;
	for (int32 i = 0; i < before_count; ++i)
		if (!mapif_achievement_persisted(before[i])) return AchievementCompareReplaceResult::Error;
	for (int32 i = 0; i < after_count; ++i)
		if (!mapif_achievement_persisted(after[i])) return AchievementCompareReplaceResult::Error;
	int32 current_count = 0;
	bool loaded = false;
	struct achievement* current = mapif_achievements_fromsql(char_id, &current_count, &loaded, true);
	if (!loaded || !mapif_transactional_table_locked(schema_config.achievement_table)) {
		if (current) aFree(current);
		return AchievementCompareReplaceResult::Error;
	}
	bool compatible = true;
	for (int32 i = 0; compatible && i < current_count; ++i) {
		if (!mapif_achievement_persisted(current[i])) continue;
		int32 found;
		ARR_FIND(0, before_count, found,
			current[i].achievement_id == before[found].achievement_id);
		// The map intentionally omits durable IDs absent from its current
		// achievement database. They are opaque here and remain unchanged. For an
		// ID the map did capture, SQL must still be that baseline or its ancestor.
		compatible = found == before_count || mapif_achievement_descends(before[found], current[i]);
		if (!compatible)
			ShowWarning("Achievement shop baseline stale for character %u achievement %d: SQL count1=%d completed=%lld rewarded=%lld, map count1=%d completed=%lld rewarded=%lld.\n",
				char_id, current[i].achievement_id, current[i].count[0], static_cast<long long>(current[i].completed),
				static_cast<long long>(current[i].rewarded), before[found].count[0],
				static_cast<long long>(before[found].completed), static_cast<long long>(before[found].rewarded));
	}
	if (!compatible) {
		if (current) aFree(current);
		return AchievementCompareReplaceResult::Conflict;
	}
	std::vector<achievement> merged;
	const bool success = mapif_achievement_merge_monotonic(char_id, current, current_count,
		after, after_count, merged) && mapif_achievement_replace_locked(char_id, current, current_count,
		merged.data(), static_cast<int32>(merged.size()));
	if (current) aFree(current);
	return success ? AchievementCompareReplaceResult::Applied : AchievementCompareReplaceResult::Error;
}

int32 mapif_parse_achievement_save(int32 fd)
{
	const uint16 length = RFIFOW(fd, 2);
	const uint32 char_id = RFIFOL(fd, 4);
	bool current_owner = false;
	for (const auto& entry : char_get_onlinedb()) {
		const auto& online = entry.second;
		if (online && online->char_id == char_id && online->server >= 0 && online->server < MAX_MAP_SERVERS &&
			map_server[online->server].fd == fd) { current_owner = true; break; }
	}
	const bool valid_length = length >= 8 && (length - 8) % sizeof(struct achievement) == 0;
	const bool success = current_owner && valid_length && mapif_achievement_save_rows(char_id,
		reinterpret_cast<const achievement*>(RFIFOP(fd, 8)), (length - 8) / sizeof(struct achievement));
	mapif_achievement_save(fd, char_id, success);

	return 0;
}

int32 mapif_parse_achievement_logout_save(int32 fd)
{
	const uint16 length = RFIFOW(fd, 2);
	if (length < pn_achievement_protocol::logout_request_size) {
		ShowError("mapif_parse_achievement_logout_save: Invalid packet length %u.\n", length);
		set_eof(fd);
		return 0;
	}
	if (RFIFOB(fd, 12) != pn_achievement_protocol::version) {
		ShowError("mapif_parse_achievement_logout_save: Unsupported protocol version %u.\n", RFIFOB(fd, 12));
		set_eof(fd);
		return 0;
	}
	const uint32 account_id = RFIFOL(fd, 4);
	const uint32 char_id = RFIFOL(fd, 8);
	const uint64 generation = RFIFOQ(fd, 16);
	bool current_owner = false;
	for (const auto& entry : char_get_onlinedb()) {
		const auto& online = entry.second;
		if (entry.first == account_id && online && online->char_id == char_id &&
			online->server >= 0 && online->server < MAX_MAP_SERVERS &&
			map_server[online->server].fd == fd) {
			current_owner = true;
			break;
		}
	}
	const bool valid_length = generation &&
		(length - pn_achievement_protocol::logout_request_size) % sizeof(struct achievement) == 0;
	const bool success = current_owner && valid_length && mapif_achievement_save_rows(char_id,
		reinterpret_cast<const achievement*>(RFIFOP(fd, pn_achievement_protocol::logout_request_size)),
		(length - pn_achievement_protocol::logout_request_size) / sizeof(struct achievement));
	mapif_achievement_logout_save(fd, account_id, char_id, generation, success);
	return 0;
}

/**
 * Sends the achievementlog of a character to the map-server.
 */
void mapif_achievement_load( int32 fd, uint32 char_id ){
	struct achievement *tmp_achievementlog = nullptr;
	int32 num_achievements = 0;
	bool loaded = false;

	tmp_achievementlog = mapif_achievements_fromsql(char_id, &num_achievements, &loaded);
	const size_t max_rows = pn_achievement_protocol::max_logout_rows(sizeof(struct achievement));
	const bool response_success = loaded && num_achievements >= 0 &&
		static_cast<size_t>(num_achievements) <= max_rows;
	if (loaded && !response_success)
		ShowWarning("mapif_achievement_load: Character %u has %d achievement rows; the versioned response supports at most %zu.\n",
			char_id, num_achievements, max_rows);
	if (!response_success)
		num_achievements = 0;
	const uint16 packet_length = static_cast<uint16>(num_achievements * sizeof(struct achievement) +
		pn_achievement_protocol::response_size);

	WFIFOHEAD(fd, packet_length);
	WFIFOW(fd, 0) = pn_achievement_protocol::load_response;
	WFIFOW(fd, 2) = static_cast<uint16>(packet_length);
	WFIFOL(fd, 4) = char_id;
	WFIFOB(fd, 8) = pn_achievement_protocol::version;
	WFIFOB(fd, 9) = response_success;

	if (num_achievements > 0)
		memcpy(WFIFOP(fd, pn_achievement_protocol::response_size), tmp_achievementlog,
			sizeof(struct achievement) * num_achievements);

	WFIFOSET(fd, packet_length);

	if (tmp_achievementlog)
		aFree(tmp_achievementlog);
}

/**
 * Sends achievementlog to the map server
 * NOTE: Achievements sent to the player are only completed ones
 * @see inter_parse_frommap
 */
int32 mapif_parse_achievement_load(int32 fd)
{
	if (RFIFOB(fd, 6) != pn_achievement_protocol::version) {
		ShowError("mapif_parse_achievement_load: Unsupported achievement load protocol version %u.\n", RFIFOB(fd, 6));
		set_eof(fd);
		return 0;
	}
	mapif_achievement_load( fd, RFIFOL(fd, 2) );

	return 0;
}

/**
 * Notify the map-server if claiming the reward has succeeded.
 */
void mapif_achievement_reward( int32 fd, uint32 char_id, int32 achievement_id, time_t rewarded ){
	WFIFOHEAD(fd, 14);
	WFIFOW(fd, 0) = 0x3864;
	WFIFOL(fd, 2) = char_id;
	WFIFOL(fd, 6) = achievement_id;
	WFIFOL(fd, 10) = (uint32)rewarded;
	WFIFOSET(fd, 14);
}

/**
 * Request of the map-server that a player claimed his achievement rewards.
 * @see inter_parse_frommap
 */
int32 mapif_parse_achievement_reward(int32 fd){
	time_t current = 0;
	uint32 char_id = RFIFOL(fd, 2);
	int32 achievement_id = RFIFOL(fd, 6);
	bool current_owner = false;
	for (const auto& entry : char_get_onlinedb()) {
		const auto& online = entry.second;
		if (online && online->char_id == char_id && online->server >= 0 && online->server < MAX_MAP_SERVERS &&
			map_server[online->server].fd == fd) { current_owner = true; break; }
	}

	mail_message message{};
	bool notify_mail = false;
	if (current_owner && Sql_BeginTransaction(sql_handle) == SQL_SUCCESS) {
		bool success = false;
		do {
			const bool sends_mail = RFIFOL(fd, 10) > 0;
			if (!mapif_transactional_table_locked(schema_config.achievement_table) ||
				(sends_mail && (!mapif_transactional_table_locked(schema_config.mail_db) ||
				 !mapif_transactional_table_locked(schema_config.mail_attachment_db))))
				break;
			if (Sql_Query(sql_handle,
				"SELECT COALESCE(UNIX_TIMESTAMP(`completed`),0),COALESCE(UNIX_TIMESTAMP(`rewarded`),0) FROM `%s` WHERE `char_id`=%u AND `id`=%d FOR UPDATE",
				schema_config.achievement_table, char_id, achievement_id) != SQL_SUCCESS)
				break;
			char *completed_value = nullptr, *rewarded_value = nullptr;
			const bool found = Sql_NextRow(sql_handle) == SQL_SUCCESS &&
				Sql_GetData(sql_handle, 0, &completed_value, nullptr) == SQL_SUCCESS && completed_value &&
				Sql_GetData(sql_handle, 1, &rewarded_value, nullptr) == SQL_SUCCESS && rewarded_value;
			const time_t completed = found ? static_cast<time_t>(strtoull(completed_value, nullptr, 10)) : 0;
			const time_t rewarded = found ? static_cast<time_t>(strtoull(rewarded_value, nullptr, 10)) : 0;
			Sql_FreeResult(sql_handle);
			if (!found || !completed) break;
			if (rewarded) {
				// Idempotently resolve a lost response/commit acknowledgement.
				current = rewarded;
				success = true;
				break;
			}

			current = time(nullptr);
			if (!current) break;
			if (sends_mail) { // Do not send a mail if no item reward.
				message.send_id = 0;
				safesnprintf(message.send_name, NAME_LENGTH, char_msg_txt(227)); // 227: GM
				message.dest_id = char_id;
				safestrncpy(message.dest_name, RFIFOCP(fd, 16), NAME_LENGTH);
				safesnprintf(message.title, MAIL_TITLE_LENGTH, char_msg_txt(228));
				safesnprintf(message.body, MAIL_BODY_LENGTH, char_msg_txt(229), RFIFOCP(fd, 16 + NAME_LENGTH));
				message.timestamp = current;
				message.status = MAIL_NEW;
				message.type = MAIL_INBOX_NORMAL;
				message.item[0].nameid = RFIFOL(fd, 10);
				message.item[0].amount = RFIFOW(fd, 14);
				message.item[0].identify = 1;
				if (!message.item[0].amount || !mail_savemessage_locked(&message)) break;
				notify_mail = true;
			}
			success = Sql_Query(sql_handle,
				"UPDATE `%s` SET `rewarded`=FROM_UNIXTIME('%u') WHERE `char_id`=%u AND `id`=%d AND `completed` IS NOT NULL AND `rewarded` IS NULL",
				schema_config.achievement_table, static_cast<uint32>(current), char_id, achievement_id) == SQL_SUCCESS &&
				Sql_NumRowsAffected(sql_handle) == 1;
		} while (false);
		if (Sql_EndTransaction(sql_handle, success) != SQL_SUCCESS) {
			current = 0;
			notify_mail = false;
		} else if (!success) {
			current = 0;
			notify_mail = false;
		}
	}
	if (current && notify_mail)
		mapif_Mail_new(&message);

	mapif_achievement_reward(fd, char_id, achievement_id, current);

	return 0;
}

/**
 * Parses achievementlog related packets from the map server.
 * @see inter_parse_frommap
 */
int32 inter_achievement_parse_frommap(int32 fd)
{
	switch (RFIFOW(fd, 0)) {
		case pn_achievement_protocol::load_request: mapif_parse_achievement_load(fd); break;
		case 0x3063: mapif_parse_achievement_save(fd); break;
		case 0x3064: mapif_parse_achievement_reward(fd); break;
		case pn_achievement_protocol::logout_save_request: mapif_parse_achievement_logout_save(fd); break;
		default:
			return 0;
	}
	return 1;
}
