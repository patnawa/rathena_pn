# Native map-cache parser repair

Local verification: 2026-09-06. The runtime change is confined to
`src/map/map.cpp`: cache input loading, record parsing, and the existing startup
call sites. No cache bytes, GRF/map_readgat behavior, NPC, map flags, database
import, gameplay balance, or live server/client state was changed by this task.

## Proven defects

The previous reader cast variable-offset bytes to `map_cache_map_info*`, which
requires 4-byte alignment. Ordinary compressed lengths do not preserve that
alignment. Fresh ASan+UBSan compilation of the actual original `map.cpp` stopped
at record-name access (original line 3682) on `db/re/map_cache.dat`'s second record,
`izlude`, offset 2615 (modulo 4 = 3). The preceding `alberta` record is at offset
8 with compressed length 2587. The base cache also has an unaligned second record,
`alb2trea`, at offset 551 (modulo 4 = 3).

The same reader had no input-buffer length, bounded name check, or safe record
traversal bounds. Negative/oversized lengths could move the cursor outside the
allocation. It assigned map dimensions before validating the decoded payload and
ignored `decode_zip`'s return value and actual output length. A short decoded
allocation could therefore disagree with the dimensions used by later cell access.
The file loader also ignored seek/ftell failures and leaked its allocated buffer
after a short read.

## Design and compatibility

`map_readfromcache` now accepts both the actual cache byte size and scratch-buffer
capacity. Headers are copied with `memcpy` into aligned local structs, so even an
unaligned initial buffer is safe. Every declared record header is validated before
success: complete bounded header/payload, terminated nonempty name, positive
compressed length and dimensions, and at most `MAX_MAP_SIZE` cells. A complete
record walk must consume the actual supplied buffer; undeclared trailing data or
truncated later records cannot be hidden behind an earlier matching record.

The first matching name remains authoritative. Only that selected record is
decompressed. It must return `Z_OK`, produce exactly `xs*ys` bytes within the
supplied scratch capacity, and contain supported GAT values 0–6. Dimensions and a
new cell allocation are published only after all validation succeeds. Failed
reads leave the map unchanged; scratch bytes may have been used by zlib. A map
with an existing cell allocation is rejected without replacement. The only
production caller is the startup priority loop, which starts with unloaded maps
and breaks after first success, so it does not rely on replacement behavior.

The historical header `file_size` is deliberately **not authoritative**. The real
base cache declares 3056818 bytes but contains 3110866 bytes and 1305 complete
records. The parser uses the actual supplied size, preserving this valid existing
data. Separate synthetic tests also use declared sizes 0, 1, 8, and UINT32_MAX.
Native format/byte-order conventions are unchanged.

`map_init_mapcache` checks seek, signed ftell, minimum header size, rewind, complete
fread, and ferror. It reads into a temporary vector and swaps output only after
success. Short/failed reads release temporary storage and preserve prior output.
The caller closes FILE before either continuing or taking its existing fatal
initialization-failure path. Owned cache vectors replace manual raw-buffer cleanup.

## Native verification

`tools/ci/map_cache_native_test.py` snapshots and freshly compiles the actual
`map.cpp`, `malloc.cpp`, and `grfio.cpp`, plus the new C++ fixture, using ASan+UBSan
with no recovery or suppression. The fixture includes the real map source to reach
its private file loader; normal map-server main is renamed and never invoked.
The actual zlib wrapper and cell conversion run, not copied implementations.

The complete run passed **1,408 cases / 5,492 assertions / zero failures /
zero unexpected native errors**, including **all 1,313 actual cache records**.
It ended with `Memory manager: No memory leaks found.`. The runner requires that
message and the result marker, and rejects native/sanitizer/allocator diagnostics
even if the process exit status is zero.

Python independently parses each original file and decompresses its records with
Python zlib. The native test loads each full original cache through the actual
file loader, finds each map with the actual parser, and compares every decoded GAT
byte plus every cell's walkable, shootable, and water fields against that oracle.
This is a whole-cache test, not the earlier aligned single-record Gimli fixture.

| Cache | Records | Actual bytes | SHA-256 |
| --- | ---: | ---: | --- |
| `db/import/map_cache.dat` | 0 | 8 | `6cc16abd70eefb90dc0ba0d14fb088630873b2c6ad943f7442356735984c35a3` |
| `db/re/map_cache.dat` | 8 | 28232 | `e871cb73bae86d4be6e32d35ed34cc0456920203f3ffa4687bdba8eedaf627fd` |
| `db/map_cache.dat` | 1305 | 3110866 | `3d523caa567fdb6330c062824085bb19e1268940bec82fe360c1f14060e5b508` |

Malformed-input cases cover every byte truncation of a complete single-record
cache; INT32_MIN/negative/zero/INT32_MAX payload lengths; invalid dimensions;
oversized cell counts; bad record counts; missing name terminators/empty names;
damaged zlib headers/checksums/streams; shorter or longer decoded output; zero or
insufficient scratch capacity; unknown GAT values; undeclared trailing data;
invalid records before and after the target; all four input-alignment offsets;
populated output maps; absent names; null pointers; and the exact 512×512 limit.

The file-loader tests use real libc `FILE` streams (`fopencookie`) with explicit
I/O fault callbacks. Eight expected error cases cover empty/truncated files,
end-seek failure, ftell failure, rewind failure, short EOF, read error, and null
FILE. Each checks unchanged output and exactly one expected diagnostic; the
fault-specific cases also prove the intended callback failed. A valid minimal
empty cache succeeds. The caller closes every created stream exactly once.
Only these explicitly expected file-loader diagnostics are recorded rather than
printed; all other errors/warnings fail the test.

The original source reproducer fails under UBSan on the ordinary unaligned
`izlude` record before reaching its final postconditions. It is not described as
an ordinary successful regression run.

## Gimli compatibility and reproduction

Only the forward declaration and invocation in the existing
`tools/ci/gimli_checkpoint_reentry_test.py` were adapted to provide buffer sizes.
No case, expected route, source selector, or NPC source changed. Its fresh
ASan+UBSan run still passes **66 cases / 904 assertions**, all eight exact arrival
cells, zero errors, and leak-free shutdown. The same new executable with the
hash-exact previous helper still reports exactly **72 expected failures / 832
assertions / 66 cases / zero native errors**, also leak-free.

From the repository in Linux/WSL, after building local native support objects:

```sh
python3 tools/ci/map_cache_native_test.py --build-dir ../map-cache-fixed-20260906
python3 tools/ci/gimli_checkpoint_reentry_test.py --build-dir ../gimli-mapcache-fixed-20260906
```

For the preserved pre-repair snapshot, the optional read-only `--map-source` and
`--legacy` flags reproduce the unbounded API; the command intentionally fails:

```sh
python3 tools/ci/map_cache_native_test.py --legacy --map-source ../map-cache-before-20260906/map_source.cpp --build-dir ../map-cache-legacy-recheck-20260906
```

| Native source | SHA-256 used for reported result |
| --- | --- |
| Original `map.cpp` | `265b60e0e1a88e3d3b0f3fbdde48e8bb72e585dd6d0c1b1d33a4a37ab5513f00` |
| Repaired `map.cpp` | `20aa23c1031a929cbf30cf6e34d2879223d7e8f762746a2b86cfb719bdc8808b` |
| `malloc.cpp` | `064496e9722eeb1486178a6663aaa375ed7f312717986b9f41df3b69fd3a4a70` |
| `grfio.cpp` | `41227b3f024b663d76bc65e6cbddf45e0a4363c8b263a778a7749285f16d3c98` |

## Explicit limits

Kernel seccomp denies socket/connect/bind/listen before the test starts. There is
no real server startup, SQL, player movement, or native-client UI. Other map/common
support objects are linked from the local build and are not all freshly
sanitizer-instrumented; the target parser, allocator, and zlib wrapper are.
The linked system zlib binary itself is not rebuilt under sanitizers.

Only the selected record's compression stream is validated in each parser call;
unselected payloads are structurally bounded but not all decompressed during that
call. The tests do select every real record. `decode_zip` retains zlib uncompress
semantics, including tolerance for extra bytes within a declared compressed span
after a successful stream; this task does not change the shared decompressor.
Synthetic I/O callbacks do not simulate operating-system allocation exhaustion or
concurrent external replacement of a cache file. The actual production fatal
startup exit is not invoked by the isolated harness; caller ordering is reviewed
in source. `map_readgat` and unrelated engine features remain outside scope.
