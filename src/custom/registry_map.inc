// Included by intif.cpp; the journal belongs to the live or retained session.
void intif_registry_replay(const map_session_data* sd) {
	if (!sd || !chrif_isconnected()) return;
	for (const auto& packet : sd->registry_saves.packets)
		chrif_save_packet(packet.data(), packet.size());
}

int32 intif_saveregistry(map_session_data* sd) {
	if (!sd) return -1;
	// A pending purchase waits for these exact earlier deltas, even though it
	// fences serialization of newly changed registry values.
	intif_registry_replay(sd);
	if (pn_item_use_save_defer(sd, CSAVE_NORMAL) || pc_transaction_pending(sd) || !sd->regs.vars) return -1;
	std::vector<pn_registry::Packet> batch;
	std::array<pn_registry::Packet, 2> current{{pn_registry::Packet(pn_registry::header_size), pn_registry::Packet(pn_registry::header_size)}};
	current[1][6] = 1;
	std::vector<std::pair<int64, script_reg_state*>> captured;
	size_t bytes = 0;
	bool valid = true;
	DBKey key; DBData* data; DBIterator* iter = db_iterator(sd->regs.vars);
	for (data = iter->first(iter, &key); iter->exists(iter); data = iter->next(iter, &key)) {
		if (data->type != DB_DATA_PTR) continue;
		const char* name = get_str(script_getvarid(key.i64));
		if (name[0] == '@') continue;
		auto* source = static_cast<script_reg_state*>(db_data2ptr(data));
		if (!source->update) continue;
		pn_registry::Entry entry; entry.key = name; entry.index = script_getvaridx(key.i64);
		if (source->type) {
			const char* text = reinterpret_cast<script_reg_str*>(source)->value;
			entry.kind = text ? 2 : 3; if (text) entry.text = text;
		} else {
			entry.number = reinterpret_cast<script_reg_num*>(source)->value;
			entry.kind = entry.number ? 0 : 1;
		}
		if (entry.key.empty() || entry.key.size() > 31 || entry.text.size() > 253) { valid = false; break; }
		const unsigned scope = entry.key.size() > 1 && entry.key[0] == '#' && entry.key[1] == '#';
		auto& packet = current[scope];
		if (!pn_registry::append_entry(packet, entry)) {
			bytes += packet.size();
			if (bytes > pn_registry::max_bytes) { valid = false; break; }
			batch.push_back(std::move(packet)); packet.assign(pn_registry::header_size, 0); packet[6] = scope;
			if (!pn_registry::append_entry(packet, entry)) { valid = false; break; }
		}
		captured.emplace_back(key.i64, source);
	}
	dbi_destroy(iter);
	if (!valid) return -1;
	for (auto& packet : current) if (pn_registry::read<uint16_t>(packet.data(), 40)) batch.push_back(std::move(packet));
	if (!batch.empty() && !sd->registry_saves.retain(sd->status.account_id, sd->status.char_id, batch)) return -1;
	// Clear only the values now owned by the immutable journal. ACK handling
	// never touches current values or flags, so a newer edit cannot be cleared.
	for (const auto& value : captured) {
		auto* source = value.second; source->update = false;
		const bool deleted = source->type ? !reinterpret_cast<script_reg_str*>(source)->value : !reinterpret_cast<script_reg_num*>(source)->value;
		if (deleted) script_reg_destroy_single(sd, value.first, source);
	}
	sd->vars_dirty = false;
	intif_registry_replay(sd);
	return 0;
}

static void intif_parse_registry_saved(int32 fd) {
	const auto id = pn_registry::identity(RFIFOP(fd, 0));
	auto* sd = map_id2sd(id.account);
	if (!sd || sd->status.char_id != id.character) {
		auto* node = chrif_search(id.account);
		sd = node && node->char_id == id.character ? node->sd : nullptr;
	}
	if (sd && sd->registry_saves.acknowledge(RFIFOP(fd, 0), pn_registry::header_size))
		chrif_registry_saved(sd);
}
