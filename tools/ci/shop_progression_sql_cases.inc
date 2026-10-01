// Durable achievement progression must be part of the same transaction as
// the purchase assets and immutable receipt. Included by
// shop_recovery_sql_runtime.cpp after its fixture helpers.

static achievement progression_row(int32 id, int32 count, time_t completed = 0) {
    achievement row{};
    row.achievement_id = id;
    row.count[0] = count;
    row.completed = completed;
    return row;
}

static pn_shop_progression::Bundle progression_bundle(const pn_shop::Commit& request,
    const std::vector<achievement>& before, const std::vector<achievement>& after) {
    pn_shop_progression::Bundle bundle;
    auto bytes = pn_shop_progression::encode(request.account_id, request.char_id,
        request.nonce_hi, request.nonce_lo, request.sequence, before, after);
    require(!bytes.empty() && pn_shop_progression::decode(bytes.data(), bytes.size(), bundle),
        "fixture progression bundle encodes and decodes");
    return bundle;
}

static void progression_seed() {
    seed();
    sql("DROP TRIGGER IF EXISTS shop_progression_fault");
    sql("INSERT INTO achievement(char_id,id,count1) VALUES"
        "(99001313,220023,1),(99001313,220024,5),(99001314,770001,77)");
}

static std::string progression_rows() {
    return result("SELECT char_id,id,count1,COALESCE(UNIX_TIMESTAMP(completed),0),"
        "COALESCE(UNIX_TIMESTAMP(rewarded),0) FROM achievement ORDER BY char_id,id");
}

static int progression_sql_cases() {
    auto make_case = []() {
        auto request = ::request(pn_shop::Market);
        std::vector<achievement> before = {
            progression_row(220023, 1), progression_row(220024, 5)
        };
        std::vector<achievement> after = {
            progression_row(220023, 2, 1700000100),
            progression_row(220024, 5),
            progression_row(240001, 1, 1700000101)
        };
        auto bundle = progression_bundle(request, before, after);
        request.progression_size = static_cast<uint32>(bundle.bytes.size());
        return std::make_tuple(request, before, after, bundle);
    };

    // A database snapshot newer than, or divergent from, the immutable before
    // image is terminally stale. Roll back every tentative output to the outer
    // savepoint, then retain only the exact outcome-3 receipt.
    for (int conflict = 0; conflict < 2; ++conflict) {
        progression_seed();
        auto [request, before, after, bundle] = make_case();
        if (!conflict)
            sql("UPDATE achievement SET count1=9 WHERE char_id=99001313 AND id=220023");
        else
			sql("UPDATE achievement SET completed=FROM_UNIXTIME(1600000000) WHERE char_id=99001313 AND id=220023");
        const auto original_assets = state(false);
        const auto original_progression = progression_rows();
        require(pn_shop_tosql(request, true, &bundle) == pn_shop::ProgressionStale,
            "newer or divergent achievement baseline terminalizes as progression stale");
        require(state(false) == original_assets && progression_rows() == original_progression,
            "progression stale savepoint restores every asset and achievement row");
        require(result("SELECT outcome,COUNT(*) FROM pn_shop_commits GROUP BY outcome") == "3|1|\n",
            "progression stale records one durable terminal receipt");
        require(char_get_chardb().at(request.char_id)->zeny == request.wallet_before,
            "progression stale cannot advance character cache");
        const auto terminal = state();
        require(pn_shop_tosql(request, false, &bundle) == pn_shop::ProgressionStale,
            "exact progression-stale receipt replays after owner departure");
        require(state() == terminal,
            "progression-stale receipt replay changes no asset progression or receipt");
        auto altered_after = after;
        altered_after[0].count[0]++;
        auto altered_bundle = progression_bundle(request, before, altered_after);
        require(pn_shop_tosql(request, true, &altered_bundle) == pn_shop::Retry,
            "different companion cannot borrow a progression-stale receipt");
        require(state() == terminal,
            "tampered progression-stale replay changes no durable state");
    }

	// A row unchanged by the planned callbacks is intentionally absent from the
	// compact companion. Concurrent durable progress on that row is unrelated,
	// remains opaque to this transaction, and must survive a successful commit.
	progression_seed();
	auto [unrelated_request, unrelated_before, unrelated_after, unrelated_bundle] = make_case();
	sql("UPDATE achievement SET completed=FROM_UNIXTIME(1600000000) WHERE char_id=99001313 AND id=220024");
	require(pn_shop_tosql(unrelated_request, true, &unrelated_bundle) == pn_shop::Committed,
		"concurrent progress on an unchanged compacted row does not stale the purchase");
	committed(unrelated_request);
	require(result("SELECT count1,UNIX_TIMESTAMP(completed) FROM achievement WHERE char_id=99001313 AND id=220024") ==
		"5|1600000000|\n", "unchanged compacted row preserves newer durable state");

    // UPDATE, INSERT, DELETE, and final receipt failures each occur after at
    // least one earlier purchase write. Every case must restore the full SQL
    // snapshot, including the original achievement rows.
    const std::vector<std::string> faults = {
        "BEFORE UPDATE ON achievement FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='progression update fault'",
        "BEFORE INSERT ON achievement FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='progression insert fault'",
        "BEFORE DELETE ON achievement FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='progression delete fault'",
        "BEFORE INSERT ON pn_shop_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='progression receipt fault'"
    };
    for (size_t fault_index = 0; fault_index < faults.size(); ++fault_index) {
        progression_seed();
        auto [request, before, after, bundle] = make_case();
        if (fault_index == 2)
            sql("INSERT INTO achievement(char_id,id) VALUES(99001313,219999)");
        const auto original = state();
        sql("CREATE TRIGGER shop_progression_fault " + faults[fault_index]);
        require(pn_shop_tosql(request, true, &bundle) == pn_shop::Retry,
            "progression or receipt write fault remains retryable");
        require(state() == original,
            "progression or receipt write fault rolls back every purchase and achievement row");
        require(char_get_chardb().at(request.char_id)->zeny == request.wallet_before,
            "progression transaction failure cannot advance character cache");
        sql("DROP TRIGGER shop_progression_fault");
    }

    progression_seed();
    auto [load_request, load_before, load_after, load_bundle] = make_case();
    const auto load_original = state();
    sql("ALTER TABLE achievement CHANGE count1 damaged_count INT UNSIGNED NOT NULL DEFAULT 0");
    require(pn_shop_tosql(load_request, true, &load_bundle) == pn_shop::Retry,
        "achievement snapshot SQL read failure remains retryable");
    require(state() == load_original && result("SELECT COUNT(*) FROM pn_shop_commits") == "0|\n",
        "achievement read error rolls back assets and creates no terminal receipt");
    sql("ALTER TABLE achievement CHANGE damaged_count count1 INT UNSIGNED NOT NULL DEFAULT 0");

    // The achievement table itself is a participant and therefore must pass
    // the same transactional-engine gate as all purchase tables.
    progression_seed();
    auto [engine_request, engine_before, engine_after, engine_bundle] = make_case();
    const auto engine_state = state();
    sql("ALTER TABLE achievement ENGINE=MyISAM");
    require(pn_shop_tosql(engine_request, true, &engine_bundle) == pn_shop::Retry,
        "nontransactional achievement table is refused");
    require(state() == engine_state,
        "achievement engine refusal changes no purchase or progression state");
    sql("ALTER TABLE achievement ENGINE=InnoDB");

	// A current map may not know a retired or custom achievement ID that still
	// has durable progress. It is absent from both bundle images, remains opaque,
	// and must survive an otherwise compatible purchase unchanged.
	progression_seed();
	sql("INSERT INTO achievement(char_id,id,count1,completed,rewarded) VALUES"
		"(99001313,880001,9,FROM_UNIXTIME(1500000000),FROM_UNIXTIME(1500000100))");
	auto [opaque_request, opaque_before, opaque_after, opaque_bundle] = make_case();
	const auto opaque_outcome = pn_shop_tosql(opaque_request, true, &opaque_bundle);
	require(opaque_outcome == pn_shop::Committed,
		"opaque durable achievement absent from map metadata does not stale the purchase");
	committed(opaque_request);
	require(result("SELECT count1,UNIX_TIMESTAMP(completed),UNIX_TIMESTAMP(rewarded) FROM achievement "
		"WHERE char_id=99001313 AND id=880001") == "9|1500000000|1500000100|\n",
		"opaque durable achievement row is preserved byte-for-byte semantically");
	require(result("SELECT count1,UNIX_TIMESTAMP(completed) FROM achievement WHERE char_id=99001313 AND id=220023") ==
		"2|1700000100|\n", "known progression still advances beside an opaque durable row");

    // A failed ordinary pre-save can leave SQL at a strict monotonic ancestor
    // of a touched row's immutable before image. Fold that missing progress into
    // the purchase while preserving unchanged rows compacted out of the frame.
    // An all-zero legacy row is not persisted and is normalized away.
    progression_seed();
    sql("INSERT INTO achievement(char_id,id) VALUES(99001313,219999)");
    auto [ancestor_request, ancestor_before, ancestor_after, ancestor_bundle] = make_case();
    ancestor_before[0].count[0] = 3;
    ancestor_after[0].count[0] = 4;
    ancestor_bundle = progression_bundle(ancestor_request, ancestor_before, ancestor_after);
    ancestor_request.progression_size = static_cast<uint32>(ancestor_bundle.bytes.size());
    require(pn_shop_tosql(ancestor_request, true, &ancestor_bundle) == pn_shop::Committed,
        "strict monotonic database ancestor folds into atomic progression commit");
    committed(ancestor_request);
    require(progression_rows() ==
        "99001313|220023|4|1700000100|0|\n"
        "99001313|220024|5|0|0|\n"
        "99001313|240001|1|1700000101|0|\n"
        "99001314|770001|77|0|0|\n",
        "ancestor commit installs changed rows, preserves compacted rows, and deletes all-zero legacy row");
    require(result("SELECT COUNT(*) FROM achievement WHERE char_id=99001313 AND id=219999") == "0|\n",
        "all-zero legacy achievement row is normalized away");

    // A valid frame atomically installs the complete after image alongside the
    // purchase and receipt. A later exact retry must resolve only from that
    // receipt, without restoring any stale asset or progression snapshot.
    progression_seed();
    auto [request, before, after, bundle] = make_case();
    require(pn_shop_tosql(request, true, &bundle) == pn_shop::Committed,
        "purchase and prepared achievement snapshot commit together");
    committed(request);
    require(progression_rows() ==
        "99001313|220023|2|1700000100|0|\n"
        "99001313|220024|5|0|0|\n"
        "99001313|240001|1|1700000101|0|\n"
        "99001314|770001|77|0|0|\n",
        "progression commit performs exact update and insert while preserving existing and other-character rows");
    require(result("SELECT COUNT(*) FROM pn_shop_commits WHERE outcome=1") == "1|\n",
        "progression purchase has one committed receipt");

    newer();
    sql("UPDATE achievement SET count1=88 WHERE char_id=99001313 AND id=220023");
    sql("INSERT INTO achievement(char_id,id,count1) VALUES(99001313,250001,3)");
    const auto newer_state = state();
    require(pn_shop_tosql(request, false, &bundle) == pn_shop::Committed,
        "exact progression receipt replays after owner departure");
    require(state() == newer_state,
        "exact progression receipt replay never restores stale assets or achievement rows");

    // The companion frame is part of the immutable receipt identity. A second,
    // individually valid frame under the same nonce cannot borrow the receipt.
    auto altered_after = after;
    altered_after[0].count[0]++;
    auto altered_bundle = progression_bundle(request, before, altered_after);
    require(altered_bundle.bytes.size() == bundle.bytes.size(),
        "tampered fixture retains the declared companion length");
    require(pn_shop_tosql(request, true, &altered_bundle) == pn_shop::Retry,
        "different valid progression frame cannot reuse committed identity");
    require(state() == newer_state,
        "tampered progression replay changes no assets achievements or receipt");

    require(pn_shop_tosql(request, true) == pn_shop::Retry,
        "commit declaring progression cannot run without its companion frame");
    require(state() == newer_state,
        "missing companion frame changes no SQL state");

    auto wrong_header = bundle;
    wrong_header.header.char_id++;
    require(pn_shop_tosql(request, true, &wrong_header) == pn_shop::Retry,
        "companion identity mismatch is rejected before transaction");
    require(state() == newer_state,
        "companion identity mismatch changes no SQL state");

    std::cout << "SHOP_PROGRESSION_SQL_PASS " << checks
        << " checks; ancestor folding, terminal stale, rollback, replay and tamper rejection" << std::endl;
    Sql_Free(sql_handle); sql_handle = nullptr; return 0;
}
