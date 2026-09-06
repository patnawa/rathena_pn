# Depth document exchange deployment record — 2026-09-06

Scope: the Administrator document-to-reputation block in
`npc/custom/varmundt_biosphere_depth.txt`, and its tests/audit evidence. No engine,
database, binary, price, acquisition, Druid, healer or Grademk changes are part
of this batch. The complete episode/gameplay audit remains unfinished.

## Local release proof

The one runtime file changes from raw/LF SHA-256
`6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988` to
`3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326`.
The exact document-region inverse reconstructs the genuine full baseline;
all other bytes remain unchanged. Independent review found and corrected an
initial native-fixture dirty-flag weakness before the accepted final proof.

| Accepted native receipt | Cases / assertions | SHA-256 |
| --- | --- | --- |
| Document exchange | 105 / 68057 | `bce8640ee1cd73b9fc91c9076f523762fd5223b99443059b0a0cec784514c9f1` |
| Crown regression | 4152 / 200773 | `38f82278ecccbd74efb4bd195aae25c0bf621c45add5e96d4742de5ec2e5580a` |
| Conversion regression | 225 / 49497 | `e4ad4dc9f5ceb50c949951e47d51a1d524ec92f8d6192c815d5116fb8dd1e327` |
| Material regression | 1516 / 788683 | `c3f92bf5a14d9f5a50f7e1c16220f4c6013698b996e6cae19e049348c4886129` |

The new document proof freshly compiled seven production units and the native
fixture with ASan/UBSan. The three regression builds reused exact-source,
checksum-verified scoped objects and relinked; their production objects were
not all freshly compiled in this batch. Each proof records its actual transport,
world and registry boundaries. The document proof exercises real loaded native
persistent registry reads/writes and dirty/update transitions, not SQL durability.

The document original-script run contains 20 controls / 10869 assertions,
including preserved behavior and two already-safe shortage aborts; these are
not 20 reproduced bugs. The repaired run checks 3327 nested QuestInfo conditions.
The independent document dependency gate passed 40 deliberate negative controls;
the broad project gate passed 17 and the material gate passed 29. The exact
regression-region utilities passed 4 general newline forms / 6 mutations and
2 document newline forms / 6 mutations. Strict content passed: 900 NPC files,
110 DB imports, 71 runtime fragments, 99 instances and 56 route assertions,
with zero errors or warnings. These are scoped evidence, not graphical gameplay.

Detailed contracts and limitations are in the
[native exchange audit](biosphere_document_exchange_audit.md),
[dependency gate](biosphere_document_callback_audit.md), and
[independent registry source review](biosphere_document_registry_source_audit.md).

## Connectivity recovery and live preflight

The previous batch's VPN subnet-router outage prevented live access after
13:44:44 UTC. The route returned during this batch; root reconnected by SSH
at approximately 14:13 UTC without changing network, router or host settings.
All four game services were still running, not restarting, with restart count
zero. The map binary remained
`63e554b5829bce76d50b0edb1dacd84ff6bc200393c3f43d821647d05ea7d98a`.

A read-only SQL preflight returned zero online characters, zero persisted
`bonus_script` rows, and one scalar `RepPoints6` row with minimum and maximum
5000 and zero out-of-range values. This is an aggregate persisted-state snapshot,
not a claim of arbitrary historical-registry or reconnect coverage. The deployment
script must repeat these checks with login blocked and all game services stopped.

The live Depth baseline, preceding Biosphere quest/Final Battle/Garden files,
and the three reviewed live-only source/database differences matched their
previously recorded bytes. The live-only `suicidebombing.cpp` source remains
preserved and unbuilt; no new binary/source equivalence is asserted for it.

## Candidate and deployment outcome

The isolated candidate stage passed with exit zero at approximately 14:23 UTC.
Both project data and preserved-live item/pet data passed `map-server --run-once`
using the unchanged reviewed map binary and production image. Both loaded
3610 OnInit NPCs and reported no allocator leaks or script/database errors;
only the pre-existing root-user warning remained. Candidate public broad,
document (including material prerequisite), and conversion gates passed.
The runtime and unchanged binary/NPC hashes were checked before and after.

| Candidate artifact | SHA-256 |
| --- | --- |
| One-NPC runtime archive | `8646adcf0cf91dd48a2ef9b017f34e5b0be863fb497a97f9bad28be5c9d2dd37` |
| Runtime manifest | `5d5ec9c8be3ccc89c915b1d800c5f037a01c7c9bbc0a541d4b1688da5d91a124` |
| Candidate stage script | `f306606fdf738d0209e7f405dbd1709a10a7eb81a33b451ff8f6e761bfcb5240` |
| Deployment stage script | `05fe68c34a4e84cb689e1ece00508201e1b183cde279588a641a08594d1fd8f5` |
| Project startup log | `3149ea5f84e594b5cf1c59daf61f2f42309ba9e996b11aef749d93d6c28bffb8` |
| Preserved-live startup log | `fe623bb24865fa3d2d3d9004e3556aaa80d5634a73fcee3aed9ba0ca36bd4d1d` |
| Candidate stage log | `c859a3afe7bfaabe23890380ee91eb394c67b8ad58c295718cae88cf5245d421` |

The three candidate logs were downloaded and matched the exact remote hashes.
Local expected-live broad negative controls also passed all 21 refusals; this
overlay proof is separate from later actual installed public validation.

A further read-only GM query confirmed the 15 previously configured reputation
variables remain at their assigned maxima: Chapter 2 at 300; Episode 21 and
its families at 1000; Depth 1/2 and Wolf at 5000; Goblin/Isgard/Orc at 3000.
No account or registry values were written in this batch.

Production installation completed successfully with script exit zero, ready at
**2026-09-06T14:30:17Z (21:30:17 Bangkok)**. The new Depth file matched its exact
candidate bytes. The public broad, conversion, material and document gates all
accepted the actual installed tree under the explicit reviewed live profile.
All four service startup logs passed, with 3610 OnInit NPCs and the map server
online. A subsequent independent inspect again returned `running=true`,
`restarting=false`, restart count zero for login, char, map and web.

At each of four deployment checkpoints—before stopping anything, after blocking
login, after full quiescence before backup, and immediately before installation—
SQL showed online=0, bonus_script=0, and RepPoints6 count=1/min=5000/max=5000/
out-of-range=0. No characters were online at either stop boundary. The four binaries,
three reviewed live-only files and preceding quest/Final Battle/Garden files
remained unchanged. The deployment writes only the one scoped NPC, not SQL.

## Retained release evidence and recovery

Local evidence is outside Git in `../document-exchange-artifacts-20260906/`.
The remote evidence and backups are under
`/app/rathena-deploy-backups/document-exchange-artifacts-20260906/`, mode 0700;
the deployment uses umask 077. No SQL dump, binary, GRF, executable or credential
is included in this commit.

| Retained deployment artifact | SHA-256 |
| --- | --- |
| Bound release checks | `484a8f5c39936d4b8b73fe90216d4252761f799afe6910f89315fbdc666b51e5` |
| Pre-change one-NPC backup | `1a30aefbad352e2c42fab79d5bb4bc2c58fe1ad9f2aec6e6cc7777001d4187ba` |
| Quiescent SQL backup | `e421338390c27f7a70292596d9b7d2dabb64667aa97eafa79566223f07fe1a9e` |
| Deployment log | `da5646bc21a69fd6726f9c1f833c61256bfbd2afeda086001a0a8ed725eeb678` |
| Login startup log | `9e24340217b54e6bc1741fee4900bf5476e937d606082e47891f8d05a135f009` |
| Char startup log | `f4a3e2ae3cce4b41bbef02843005cd2af73ab562b91f4f7d8ca1adaa86426c52` |
| Map startup log | `a893c37bad47c33920c1edbf52df42ed7bb241c594f9b0fd7fa3a884461833d8` |
| Web startup log | `eb9758aead71fff99a3982fd06f8eaef5691236376a40b8e2ae6861b824e0a39` |

All five deployment/post-startup logs were downloaded and matched their remote
hashes. The SQL backup remains on the server and was not downloaded into Git.
The release builder bound all four native receipts to current source and their
exact callback manifests; the new document proof also binds its executable and
every linked scoped/support object/archive. It checked the one-member archive
contents and both downloaded candidate logs. Independent review approved the
final scripts; SQL parser guards passed three valid and seven refusal controls.

The one-file rollback path requires all game services actually stopped, verifies
the backup's single regular member and exact old content, restores only that NPC,
and checks preserved files before restart. It does not automatically restore SQL.
Rollback was not exercised in production because deployment succeeded. Backups
remain available; nothing material was deleted.

Graphical Druid combat, NPC interaction and relog persistence still need attached
client verification. The separate dormant parser findings are recorded in the
[follow-up source audit](reputation_parser_followup_audit.md), not silently
bundled into this NPC release. The broader episode audit remains active.
