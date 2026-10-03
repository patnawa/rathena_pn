# Persistent damage lab history

The private lab automatically keeps the last three **completed** 60-second runs
in permanent character registry arrays (`PNLab*`, without `@`). Its console and
Build Notes expose the history. Logout during a test never calls `PN_LabSave`;
the existing instance lock expires and the next visit cleans the old target.
Normal logout/relog uses the server's existing character registry persistence.
This feature does not provide a separate crash-proof result commit protocol.

Each record contains label, completion epoch, total HP loss, elapsed milliseconds,
DPS, the full target configuration, and three diagnostic fingerprints:

- Equipment/stat identity covers equipment slots, item IDs, refine, grade, cards,
  enchant/random-option values, base stats, all six traits, class/levels and
  learned skills. Item serials and inventory order are excluded.
- Buff identity covers every active status type and all four status values plus
  option flags. Timer IDs and remaining durations are excluded.
- Runtime identity is the SHA256 of the verified manifest covering the loaded map
  executable and the complete database, NPC and effective configuration trees.
  The manifest also records source identity. Verification occurs before readiness;
  reloads invalidate comparison identity until a verified restart. Missing or
  invalid identity disables saved comparisons. Different fixture/production
  configurations intentionally produce different identities.

The builtin `pnlabidentity(0|1|2)` returns these fingerprints. Equipment and buff
identities use MD5 only as an inexpensive diagnostic comparison key, never for
authentication or trust; runtime manifest/file verification uses SHA256.

The lab samples equipment and buff identities every 100 ms. Equipment/stat/skill
changes invalidate the run. Buff changes mark a completed run as variable and
exclude it from automatic comparisons (including triggered/expiring buffs).
Changes entirely between samples cannot be detected. Rotation, remaining buff
duration, player positioning, companions and actual skill sequence are not
controlled; matching fingerprints are a screening tool, not proof of identical
combat conditions. The UI retains the normal-class, immobile, immune target
limitations and explicitly warns about player-controlled rotation.

Automatic A/B candidates require identical target, initial buffs and build, and
stable sampled buffs. Equipment identities may differ for A/B. Repeat medians
require matching equipment identity too. With two samples the displayed median
is the integer midpoint; with three it is the middle value. Spread is max minus
min. An incompatible target/build/buff run never appears as an A/B candidate.
The comparison lists each excluded saved run and its first blocking reason:
variable buffs, different target settings, different initial buffs, or a missing
or different verified server build. It then counts identical setups, gear A/B
candidates and excluded saved runs. Empty slots are not counted; the selected
run counts as an identical setup only when it is eligible. These explanations
do not change eligibility, the repeat median/spread or stored records.

Validation: `python3 tools/ci/lab_history_test.py --root <built Linux checkout>`
executes the production script VM and identity builtin with network access denied.
The transport and character registry are explicit doubles; player recreation
retains persistent registry values to test the relog boundary. Actual SQL relog,
timed world combat and client rendering must also pass release acceptance.

For the focused comparison UI regression:

```sh
python3 tools/ci/lab_history_test.py --comparison-only --build-dir /tmp/lab-comparison
/tmp/lab-comparison/test npc/custom/quality_services.txt /tmp/lab-comparison/items.yml comparison
```

Run the second command from the built checkout to test source-only wording
changes without rebuilding the harness. The same cases also run in the full lab
suite.
