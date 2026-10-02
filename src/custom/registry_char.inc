static void mapif_parse_registry_save(int32 fd) {
	const size_t length = RFIFOW(fd, 2);
	std::vector<pn_registry::Entry> entries;
	if (!pn_registry::decode(RFIFOP(fd, 0), length, entries)) return;
	const auto id = pn_registry::identity(RFIFOP(fd, 0));
	auto online = char_get_onlinedb().find(id.account);
	const bool owner = online != char_get_onlinedb().end() && online->second && online->second->char_id == id.character &&
		online->second->server >= 0 && online->second->server < MAX_MAP_SERVERS && map_server[online->second->server].fd == fd;
	if (id.scope) {
		if (!owner || !session_isActive(login_fd)) return;
		WFIFOHEAD(login_fd, length); std::memcpy(WFIFOP(login_fd, 0), RFIFOP(fd, 0), length);
		WFIFOW(login_fd, 0) = pn_registry::login_request; WFIFOSET(login_fd, length);
		return;
	}
	const pn_registry::Tables tables{schema_config.char_reg_num_table, schema_config.char_reg_str_table,
		schema_config.acc_reg_num_table, schema_config.acc_reg_str_table};
	const bool success = pn_registry::save(sql_handle, RFIFOP(fd, 0), length, tables, owner);
	const auto ack = pn_registry::acknowledgement(RFIFOP(fd, 0), success);
	WFIFOHEAD(fd, ack.size()); std::memcpy(WFIFOP(fd, 0), ack.data(), ack.size()); WFIFOSET(fd, ack.size());
}
