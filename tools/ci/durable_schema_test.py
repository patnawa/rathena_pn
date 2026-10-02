#!/usr/bin/env python3
"""Keep fresh-install durable asset schema aligned with upgrade migrations."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
MAIN = (ROOT / "sql-files/main.sql").read_text(encoding="utf-8")


def create_statements(text: str, table: str) -> list[str]:
    pattern = re.compile(
        rf"CREATE\s+TABLE\s+IF\s+NOT\s+EXISTS\s+`?{re.escape(table)}`?\s*"
        rf"\(.*?\)\s*ENGINE\s*=\s*\w+(?:\s+AUTO_INCREMENT\s*=\s*\d+)?\s*;",
        re.IGNORECASE | re.DOTALL,
    )
    return pattern.findall(text)


def create_statement(text: str, table: str) -> str:
    matches = create_statements(text, table)
    assert len(matches) == 1, f"expected one CREATE TABLE for {table}, found {len(matches)}"
    return matches[0]


def canonical(statement: str) -> str:
    return re.sub(r"\s+", " ", statement).strip().lower()


def require_innodb(table: str) -> None:
    statement = create_statement(MAIN, table)
    assert re.search(r"\)\s*ENGINE\s*=\s*InnoDB\b", statement, re.IGNORECASE), (
        f"fresh schema table {table} must be InnoDB"
    )


migrations = {
    "pn_purchase_history": "upgrade_20261002_purchase_history.sql",
    "pn_registry_saves": "upgrade_20261002_registry_saves.sql",
    "pn_shop_commits": "upgrade_20260929_shop_purchase.sql",
    "pn_global_point_barriers": "upgrade_20260929_point_assets.sql",
    "pn_point_registry_keys": "upgrade_20260929_point_assets.sql",
    "pn_pet_entitlements": "upgrade_20260929_pet_entitlements.sql",
}

for table, migration_name in migrations.items():
    migration = (ROOT / "sql-files/upgrades" / migration_name).read_text(encoding="utf-8")
    assert canonical(create_statement(MAIN, table)) == canonical(
        create_statement(migration, table)
    ), f"fresh schema definition for {table} drifted from {migration_name}"

# Every table that can participate in the same durable asset transaction must
# be transactional on a main.sql-only install.  Upgrade scripts enforce the
# same property for an existing installation.
for table in (
    "pn_purchase_history",
    "acc_reg_num",
    "acc_reg_str",
    "char_reg_str",
    "global_acc_reg_str",
    "pn_registry_saves",
    "achievement",
    "barter",
    "char",
    "char_reg_num",
    "global_acc_reg_num",
    "inventory",
    "mail",
    "mail_attachments",
    "market",
    "pet",
    "pn_global_point_barriers",
    "pn_pet_entitlements",
    "pn_point_registry_keys",
    "pn_shop_commits",
    "sales",
):
    require_innodb(table)

inventory = create_statement(MAIN, "inventory")
for table in ("memo", "skill", "friends", "hotkey", "mercenary_owner"):
    require_innodb(table)

character_migration = (ROOT / "sql-files/upgrades/upgrade_20261002_character_save_atomicity.sql").read_text(encoding="utf-8")
for table in ("char", "memo", "skill", "friends", "hotkey", "mercenary_owner"):
    assert f"ALTER TABLE `{table}` ENGINE=InnoDB;" in character_migration

registry_migration = (ROOT / "sql-files/upgrades/upgrade_20261002_registry_saves.sql").read_text(encoding="utf-8")
for table in ("char_reg_str", "acc_reg_str", "global_acc_reg_str"):
    assert f"ALTER TABLE `{table}` ENGINE=InnoDB;" in registry_migration, f"Missing registry engine migration: {table}"

pet_lookup = re.search(
    r"(?:KEY|INDEX)\s+`pn_pet_identity_lookup`\s*"
    r"\(\s*`card0`\s*,\s*`card1`\s*,\s*`card2`\s*\)",
    inventory,
    re.IGNORECASE,
)
assert pet_lookup, "fresh inventory schema lacks the pet identity lookup index"

print(
    "PASS durable schema: 4 migration-aligned tables, 16 InnoDB participants, "
    "5 character-status companion tables, and pet identity index"
)
