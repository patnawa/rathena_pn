# Depth 1 document exchange transaction repair

2026-09-06. Scope is only `Depth Research Administrator#bio_d1` in
`npc/custom/varmundt_biosphere_depth.txt`. All other NPC bodies, including every
material, crown, equipment, research reward and Abyss conversion service, remain
byte-identical to the baseline. The historical source-only findings are in
[the follow-up audit](biosphere_document_exchange_followup_audit.md).

## Runtime behavior

The exchange still takes two `1001289` documents per selected pair and awards
three `REPUTATION_BIOSPHERE_DEPTH1` points, capped at 5000. Ceiling division is
retained, so the final pair can still award only one or two points. No Zeny,
new prices, refund policy, acquisition changes or new reputation gate were added.

After the last numeric input suspension it now requires status zero, checks the
`ba_in01` service map and original BaseLevel >=250 / `ep17_2_main >=33` access,
and rereads reputation, the current ceiling limit and current document count.
A selected order that no longer fits is refused without silently reducing it.
The gain is calculated from the current reputation before the original
`delitem -> add_reputation_points` payment/credit order, followed by the original
confirmation. There is no further suspension in this segment.

Valid negative reputation is supported: the native definition's range is
-5000..5000, allowing at most 3334 pairs and 6668 documents. No 30000-output cap,
chunked debit loop or output inventory/weight preflight is required for this
small, single-input, reputation-only exchange.

`countitem` excludes rental instances; ordinary `delitem` may consume one while
searching matching IDs. This native selection policy is unchanged. The fix
does not promise to select particular bound, UID or rental document instances
and does not transfer metadata or introduce a new filtering policy.

## Native test design and boundaries

`tools/ci/biosphere_document_exchange_test.py` reconstructs the entire genuine
baseline by inverting only this reviewed change and checking its full hash. It
uses no Git invocation or untracked original snapshot. CRLF-to-LF normalization
is an in-memory test view; raw source bytes are checked unchanged during the run.

The fixture uses the actual extracted NPC/access helper, numeric input builtin,
native inventory deletion, reputation database parsing and get/add reputation
builtins. Persistent character variables use the actual `pc_setreg2`,
`pc_setregistry`, `pc_readreg2` and `pc_readregistry`, backed by initialized native
ERS records with `vars_ok=true`. It checks values, update flags and dirty state,
not just an externally simulated award amount. Setup uses native registry load
mode and asserts update flags/dirty state initially clear, so the actual award
must set them, including the -3 to zero transition. Reputation notification
arguments are recorded at `clif_reputation_type`, before packet construction;
the gate statically reviews its formatter/layout, but no on-wire packet is claimed.
Real `do_init_script()` loads the gate-pinned constant files (11 ordered rows),
and a native lookup asserts the unique reputation constant resolves to 6. There
is no fixture-only hardcoded constant registration.

The attached character is deliberately already loaded. This does not execute
network authentication, SQL persistence, reconnect, or an unloaded-registry
failure/refund policy. Out-of-range raw reputation values created by privileged
editing or corrupt storage are also outside the valid in-range premise: the
getter clamps its view whereas the additive builtin reads the raw value.

The required `biosphere_document_callback_audit.validate(ROOT)` gate includes
the broader source closure and the existing material/ba_in01 callback gate.
The fixture executes real `questinfo` registrations for 19 owners/33 conditions,
real display initialization, and real nested `achievement_check_condition` via
`pc_show_questinfo` during deletion. Source-derived true/false probes exercise
each complete condition; they do not exhaust every OR arm/subexpression.
Registry, inventory, Zeny, weight, quest log and caller/RID preservation are
checked around each nested condition. Each successful exchange must increase
the nested-callback count during its debit; separate direct condition probes
cannot satisfy that assertion. A wrong-size display control verifies native
early return, reinitialization without evaluation, and restored evaluation.

The native driver is appended to the tracked crown test's boundary helpers,
without editing that shared file. World ID lookups, packet delivery, logging,
transient `@` registers and the weight status-notification sink are explicit
doubles. Reputation arithmetic and persistent registry operations are not
wrapped. Direct VM state-change cases do not prove a client can bypass native
NPC proximity/identity checks. Forced close is tested separately from numeric
zero; no particular client Escape encoding is assumed.

Seven scoped production units (`pc.cpp`, `script.cpp`, `itemdb.cpp`, `clif.cpp`,
`achievement.cpp`, `quest.cpp`, `malloc.cpp`) are freshly compiled with
PACKETVER20260219 and ASan/UBSan for the final proof. Other support objects are
linked from the local build, so this is not whole-engine sanitizer coverage.
Kernel socket/connect/bind/listen denial prevents contacting any server.

Run from the repository root in WSL after native support objects are built:

```sh
python3 -B tools/ci/biosphere_document_exchange_test.py \
  --native-build-dir ../biosphere-document-native-final-20260906
```

`--prepare-only` explicitly collects unvalidated fixture evidence and compiles
without executing or claiming a passing test. `--reuse-build` requires matching
tracked sources/headers, exact executable bytes and every linked scoped/support
object/archive. Object caching also checks source, headers, compiler flags and
each object checksum. Three in-memory controls reject changed source, executable
or support bytes without overwriting retained artifacts. An accepted run requires the full mandatory gate before and
after, an identical manifest, and unchanged raw runtime/source dependencies.

Coverage includes negative-boundary and near-cap amounts, single/split valid
stacks and odd document totals, initial cap/shortage, out-of-range input,
reputation changes at both Next and numeric-input suspensions, stale documents,
fresh access/map checks and actual forced close at both suspensions. The native
-3 to zero case verifies the zero-value update path, not merely a positive value
being written. No reward/acquisition or ordinary concurrent-NPC gameplay is
fabricated to create these injected states.

The **20 original controls are not 20 broken scenarios**: 14 state-change
arithmetic scenarios include preserved behavior (4990, three selected pairs,
then 4980 still correctly awards nine); four demonstrate ignored invalid input;
two insufficient-document originals already abort without partial payment.
Those last two produce counted native errors, while the repaired NPC refuses
cleanly. The controls execute the exact reconstructed original dialogue, not a
hand-written model of its arithmetic.

## Verification status

PASS on 2026-09-06 in `../biosphere-document-native-final-20260906`:

- Candidate: **105 cases, 68057 assertions, 3327 checked nested conditions**.
- Genuine original controls: **20 cases, 10869 assertions, 594 checked nested
  conditions**, classified above rather than mislabeled as 20 broken scenarios.
- Seven zero-exit diagnostic controls and three in-memory artifact/source
  rejection controls pass. Candidate stderr is empty. Both native children
  explicitly report `Memory manager: No memory leaks found.` with no ASan/UBSan
  issue. Only the two original single-debit shortage errors/warnings are accepted.
- The mandatory document gate passes before and after with the identical complete
  manifest. Raw runtime, compiled sources/headers, executable bytes and all linked
  object/archive bytes remain unchanged through the accepted proof.

All seven scoped production units and the fixture were freshly compiled in this
final directory. No live-server/client action or deployment was performed by
this fixture; network availability does not make its local proof a live receipt.

| Retained artifact | SHA-256 |
| --- | --- |
| `receipt.json` | `bce8640ee1cd73b9fc91c9076f523762fd5223b99443059b0a0cec784514c9f1` |
| `biosphere_document_exchange_test` executable | `d10df2526315a78a640b0ff8fcc75f11f2cc90a4518ae8a41e31643762883a4e` |
| `build.json` source/executable/link binding | `077f5fd1c2ab24909c3d959b8f6292a9ec491290401d4a985b98938948dce0a8` |
| Complete callback manifest, `json.dumps(manifest, sort_keys=True)` with default separators | `9296e6fc3d00e7b96a1a636243e19af4431c27917df3dd18a03723cbb9419803` |

| Source | LF-normalized SHA-256 |
| --- | --- |
| Genuine full baseline depth file | `6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988` |
| Full candidate depth file | `3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326` |

The current runtime source is LF-only, so its raw and normalized hashes agree.
Deployment, source integration and Git operations belong to the parent task.
