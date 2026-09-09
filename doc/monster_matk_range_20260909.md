# Monster magic-attack range repair

Strict SQL import failed on custom monsters 22177 and 22180: their `Attack2`
values are 67,733 and 68,299, while the exported column allowed only 65,535.
All four monster SQL schemas and their generator templates now use unsigned
32-bit columns. An upgrade script widens existing default-named monster tables;
raw data values are retained without disabling strict mode.

The same values silently wrapped during native YAML loading to 2,197 and 2,763.
The loader now reads a wide integer, reports an explicit warning and caps it at
the existing 65,535 runtime limit before narrowing. Shared non-player Renewal
base magic-attack formulas also saturate instead of wrapping or inverting the
minimum/maximum damage range. Player and homunculus formulas, field layouts and
the combat ABI remain unchanged. This intentionally increases damage where the
old overflow had reduced it; it does not add 32-bit combat support.

Validation:

- Real `yaml2sql` export and strict MariaDB import preserve all 2,675 base and
  227 custom monster rows, including both reported values.
- All four table migrations preserve existing data and accept boundary values,
  unsigned 32-bit maximum and NULL.
- The source schema test covers 7,364 actual value/schema combinations.
- Production-source parser and formula probes pass 786,484 Renewal checks and
  36 pre-Renewal checks under undefined-behavior sanitization.
- The former implementation reproduces both low-value wraps and fails 124,604
  Renewal and 12 pre-Renewal postconditions.

The release gate includes both regression tests. Runtime deployment additionally
requires a production-compatible Alpine build, isolated startup and live-file
precondition checks. SQL export schemas do not require replacing production
player tables or loading generated monster tables into a YAML-backed server.
