"""Offline, atomic conversion of legacy Zeny tokens into account bank balances.

Dry run is the default. The caller supplies a PyMySQL-compatible connection.
Applying requires all application writers stopped, a matching reviewed plan hash,
and InnoDB for every changed table. No wallet/native protocol change is made.
"""
import hashlib
import json

LIMIT = (1 << 63) - 1
PRICES = {6024: 499000000, 12781: 998000}
RECEIPT = "pn_zeny_token_retirement"
STORAGE = ["storage"] + [f"pn_storage_{i:02}" for i in range(2, 8)]
SOURCES = {
    "inventory": ("id", "LEFT JOIN `char` c ON c.char_id=t.char_id", "c.account_id"),
    "cart_inventory": ("id", "LEFT JOIN `char` c ON c.char_id=t.char_id", "c.account_id"),
    **{name: ("id", "", "t.account_id") for name in STORAGE},
    "guild_storage": ("id", "LEFT JOIN guild g ON g.guild_id=t.guild_id LEFT JOIN `char` c ON c.char_id=g.char_id", "c.account_id"),
    "mail_attachments": ("id,index", "LEFT JOIN mail m ON m.id=t.id LEFT JOIN `char` c ON c.char_id=m.dest_id", "c.account_id"),
}


def query(connection, sql, params=()):
    with connection.cursor() as cursor:
        cursor.execute(sql, params)
        return cursor.fetchall()


def execute(connection, sql, params=()):
    with connection.cursor() as cursor:
        cursor.execute(sql, params)
        return cursor.rowcount


def inspect(connection):
    """Return a complete deterministic plan, refusing ambiguous or omitted stores."""
    tables = dict(query(connection, "SELECT TABLE_NAME,ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE()"))
    required = set(SOURCES) | {"char", "login", "guild", "mail", "auction", "acc_reg_num", "vending_items", "vendings", "buyingstore_items", "buyingstores"}
    missing = sorted(required - tables.keys())
    if missing:
        raise ValueError("Missing ownership tables: " + ", ".join(missing))
    columns = query(connection, "SELECT TABLE_NAME,COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()")
    item_tables = {table for table, column in columns if column == "nameid"}
    # These tables are logs, catalogues, or purchase orders, not owned inventory.
    excluded = {"guild_storage_log", "picklog", "auction", "item_db", "item_db2", "item_db_re", "item_db2_re", "mob_drop", "mob_item_ratio", "npc_market_data", "buyingstore_items"}
    for table in sorted(item_tables - set(SOURCES) - excluded):
        if not table.replace("_", "").isalnum():
            raise ValueError("Unexpected SQL table identifier")
        if query(connection, f"SELECT 1 FROM `{table}` WHERE nameid IN (6024,12781) LIMIT 1"):
            raise ValueError("Unmapped token-bearing table: " + table)
    if query(connection, "SELECT 1 FROM auction WHERE nameid IN (6024,12781) LIMIT 1"):
        raise ValueError("Settle token auctions before migration; auction ownership is ambiguous")
    rows, totals, seen = [], {}, set()
    for table, (keys, joins, owner) in SOURCES.items():
        if tables[table] != "InnoDB":
            raise ValueError("Nontransactional inventory table: " + table)
        key_names = keys.split(",")
        selected = ",".join(f"t.`{key}`" for key in key_names)
        records = query(connection, f"SELECT {selected},t.nameid,t.amount,{owner} FROM `{table}` t {joins} WHERE t.nameid IN (6024,12781) ORDER BY {selected} FOR UPDATE")
        for record in records:
            key = list(record[:len(key_names)])
            identity = (table, tuple(key))
            if identity in seen:
                raise ValueError(f"Ambiguous ownership join: {table} {key}")
            seen.add(identity)
            item, amount, account = record[len(key_names):]
            if not account or amount <= 0:
                raise ValueError(f"Orphan owner or invalid amount: {table} {key}")
            if not query(connection, "SELECT 1 FROM login WHERE account_id=%s", (account,)):
                raise ValueError(f"Missing login account: {account}")
            credit = int(amount) * PRICES[item]
            totals[account] = totals.get(account, 0) + credit
            rows.append({"table": table, "key": key, "item": item, "quantity": amount, "account": account, "credit": credit})
    # Listings are references to owned cart rows or purchase orders, never a
    # second asset to credit. Remove their exact keys in the same transaction.
    listings = []
    for table in ("vending_items", "vendings", "buyingstore_items", "buyingstores"):
        if tables[table] != "InnoDB":
            raise ValueError("Nontransactional shop table: " + table)
    for shop, index in query(connection, "SELECT v.vending_id,v.`index` FROM vending_items v JOIN cart_inventory c ON c.id=v.cartinventory_id WHERE c.nameid IN (6024,12781) ORDER BY v.vending_id,v.`index` FOR UPDATE"):
        listings.append({"table": "vending_items", "shop": shop, "index": index})
    for shop, index in query(connection, "SELECT buyingstore_id,`index` FROM buyingstore_items WHERE item_id IN (6024,12781) ORDER BY buyingstore_id,`index` FOR UPDATE"):
        listings.append({"table": "buyingstore_items", "shop": shop, "index": index})
    if tables["acc_reg_num"] != "InnoDB":
        raise ValueError("Account registry must use InnoDB")
    accounts = []
    for account, credit in sorted(totals.items()):
        balance = query(connection, "SELECT value FROM acc_reg_num WHERE account_id=%s AND `key`='#BANKVAULT' AND `index`=0 FOR UPDATE", (account,))
        before = int(balance[0][0]) if balance else 0
        if before < 0 or credit > LIMIT - before:
            raise ValueError(f"Account bank overflow: {account}")
        accounts.append({"account": account, "before": before, "credit": credit, "after": before + credit})
    plan = {"version": 2, "listings": listings, "prices": PRICES, "rows": rows, "accounts": accounts,
            "guild_storage_owner": "guild leader account", "mail_owner": "current recipient account"}
    plan["sha256"] = hashlib.sha256(json.dumps(plan, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return plan


def migrate(connection, *, apply=False, expected_hash=None, assert_offline=None):
    """Credit bank + delete exact item rows + journal in ONE transaction.

    assert_offline must independently verify every game/web writer is stopped.
    Caller must retain maintenance isolation until the process exits.
    """
    if apply:
        if not expected_hash or assert_offline is None:
            raise ValueError("Apply requires reviewed plan hash and offline writer verification")
        assert_offline()
        # DDL precedes transaction: never implicitly commit a partial migration.
        execute(connection, f"CREATE TABLE IF NOT EXISTS `{RECEIPT}` (plan_hash CHAR(64) PRIMARY KEY, plan_json LONGTEXT NOT NULL, committed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB")
        if query(connection, "SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME=%s", (RECEIPT,)) != (("InnoDB",),):
            raise ValueError("Migration journal must use InnoDB")
    connection.begin()
    try:
        if apply and query(connection, f"SELECT 1 FROM `{RECEIPT}` WHERE plan_hash=%s FOR UPDATE", (expected_hash,)):
            connection.rollback()
            return {"status": "already_committed", "sha256": expected_hash}
        plan = inspect(connection)
        if not apply:
            connection.rollback()
            return plan
        if plan["sha256"] != expected_hash:
            raise ValueError("Inventory/ownership/bank changed since dry run; review a new plan")
        assert_offline()
        for account in plan["accounts"]:
            execute(connection, "INSERT INTO acc_reg_num(account_id,`key`,`index`,value) VALUES (%s,'#BANKVAULT',0,%s) ON DUPLICATE KEY UPDATE value=VALUES(value)", (account["account"], account["after"]))
        for row in plan["listings"]:
            column = "vending_id" if row["table"] == "vending_items" else "buyingstore_id"
            if execute(connection, f"DELETE FROM `{row['table']}` WHERE `{column}`=%s AND `index`=%s", (row["shop"], row["index"])) != 1:
                raise ValueError("Token listing changed during migration")
        for table, parent, column in (("vending_items", "vendings", "vending_id"), ("buyingstore_items", "buyingstores", "buyingstore_id")):
            for shop in sorted({row["shop"] for row in plan["listings"] if row["table"] == table}):
                execute(connection, f"DELETE FROM `{parent}` WHERE id=%s AND NOT EXISTS (SELECT 1 FROM `{table}` WHERE `{column}`=%s)", (shop, shop))
        for row in plan["rows"]:
            keys = SOURCES[row["table"]][0].split(",")
            where = " AND ".join(f"`{key}`=%s" for key in keys)
            changed = execute(connection, f"DELETE FROM `{row['table']}` WHERE {where} AND nameid=%s AND amount=%s", tuple(row["key"]) + (row["item"], row["quantity"]))
            if changed != 1:
                raise ValueError("Token row changed during migration")
        execute(connection, f"INSERT INTO `{RECEIPT}`(plan_hash,plan_json) VALUES (%s,%s)", (expected_hash, json.dumps(plan, sort_keys=True)))
        connection.commit()
        return {"status": "committed", "sha256": expected_hash, "accounts": len(plan["accounts"]), "stacks": len(plan["rows"])}
    except BaseException:
        connection.rollback()
        raise


def main():
    import argparse
    import subprocess
    import pymysql
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--database', required=True)
    parser.add_argument('--db-container', default='rathena-db')
    parser.add_argument('--apply-reviewed-hash')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    info = json.loads(subprocess.check_output(['docker','inspect',args.db_container]))[0]
    environment = dict(entry.split('=',1) for entry in info['Config']['Env'] if '=' in entry)
    password = environment.get('MARIADB_ROOT_PASSWORD',environment.get('MYSQL_ROOT_PASSWORD'))
    if not password:
        raise ValueError('DB container has no supported root password environment entry')
    addresses = [network['IPAddress'] for network in info['NetworkSettings']['Networks'].values() if network['IPAddress']]
    if len(addresses) != 1:
        raise ValueError('Ambiguous DB container network; use migrate() with an explicit connection')
    def offline():
        writers = ['rathena-map','rathena-char','rathena-login','rathena-web','rathena-fluxcp']
        status = json.loads(subprocess.check_output(['docker','inspect',*writers]))
        running = [entry['Name'] for entry in status if entry['State']['Running']]
        if running:
            raise ValueError('Stop all application writers before apply: '+', '.join(running))
    connection = pymysql.connect(host=addresses[0], user='root', password=password, database=args.database, autocommit=True)
    try:
        # Reserve evidence before any apply writes. An existing/unwritable output
        # must not turn a successful commit into an apparently failed command.
        from pathlib import Path
        with Path(args.output).open('x',encoding='utf-8') as stream:
            try:
                result = migrate(connection, apply=bool(args.apply_reviewed_hash), expected_hash=args.apply_reviewed_hash, assert_offline=offline)
            except BaseException as error:
                json.dump({'status':'failed','error':type(error).__name__,'message':str(error)},stream,indent=2)
                stream.write('\n')
                raise
            json.dump(result,stream,indent=2)
            stream.write('\n')
        print(json.dumps({'output':args.output,'sha256':result['sha256'],'status':result.get('status','dry_run')}))
    finally:
        connection.close()


if __name__ == '__main__':
    main()
