# Native allocator alignment repair - 2026-09-06

The real script-VM test exposed a defect in the default `USE_MEMMGR` allocator,
not a stale map-object ABI. A standalone fixture directly includes the current
`src/common/malloc.cpp` and links no existing server objects or archives.

On the deployed Linux LP64 layout, the original backing block's data starts at
byte 36. A pooled `unit_head` requires eight-byte alignment but is placed four
bytes off alignment. The returned payload adds another 24 bytes and remains
misaligned. The original standalone probe found 3044 misaligned allocations out
of 3058; a fresh UBSan build stopped inside the allocator's own metadata write.

A separate 65537-byte allocation reproduces an unaligned `long` store in its
tail guard. Fixing only the backing-block address would not fix arbitrary odd
allocation lengths. The large-allocation pointer verifier also rejected the
last payload bytes because its outer range omitted the header offset.

## Repair and scope

- Align backing-block data and the payload-start member to `std::max_align_t`.
- Derive pooled and large payload positions with `offsetof`, not
  `sizeof(header) - sizeof(long)`, which is invalid after alignment adds padding.
- Update forward and reverse lookup, realloc, free, verification, debug poison
  and final cleanup offsets together. Large leak reporting already uses the
  address of the payload-start member.
- Use `memcpy` for tail-guard reads/writes. Guard values and overflow diagnostics
  are unchanged, without requiring requested sizes to be aligned.
- Include the full payload in the large allocation's verification interval.
- Compile-time checks require aligned block and unit strides and sufficient
  reserved space for the tail guard.

The new LP64 layout has block data offset 48, block stride 41008, pooled payload
offset 32/header size 48, and large payload offset 64/header size 80. All payload
and stride positions satisfy the platform's 16-byte fundamental alignment.
The additional per-unit padding is necessary overhead; this is not a memory
usage optimization. No support for explicitly over-aligned types is claimed.

Only private allocator structures change. Public function signatures, packet
structures, item/character data and SQL schemas are unchanged. Heap pointers
and allocator metadata are process-local; deploy rebuilt binaries through a
normal restart, never mix old and new allocator code in a running process.

## Reproducible tests

From the repository root on Linux/WSL:

```sh
python3 tools/ci/run_native_allocator_test.py --sanitizer address,undefined
python3 tools/ci/run_native_allocator_test.py --sanitizer address,undefined --debug
python3 tools/ci/run_native_allocator_test.py --sanitizer address,undefined --case shutdown
python3 tools/ci/run_native_allocator_test.py --sanitizer address,undefined --case shutdown --debug
python3 tools/ci/run_native_script_vm_test.py
```

Each normal/debug matrix passes 4662 allocations and 18774 assertions with zero
misaligned allocations or unexpected diagnostics. Coverage includes every tested
size class, exact 40896/40897 pooled cutoff, adjacent units/backing blocks,
free-unit reuse, non-FIFO frees, first/last/header/one-past verification,
calloc zeroing, pooled-to-large and large-to-large realloc preservation, existing
shrink-in-place behavior, null handling and string duplication.

Two intentional tail-guard corruptions produce two expected overflow diagnostics;
the test restores the allocator-owned guards and frees those allocations. These
messages are not sanitizer failures. Normal accounting returns to zero and the
allocator reports no leaks. Test logs are confined to the temporary build folder.

Each shutdown case passes 105 assertions after intentionally leaving one pooled
and one large allocation outstanding. Its expected leak-report warning exercises
actual shutdown cleanup; it is not a claim that this deliberate fixture had no
outstanding allocations. The allocator's existing process-exit global/accounting
teardown behavior is otherwise unchanged.

The isolated real script VM now freshly compiles both `script.cpp` and
`malloc.cpp`, plus its driver, with ASan and UBSan enabled and recovery disabled.
All 13 builtin cases and 21 native assertions pass; the old allocator stops that
same VM test during setup. Existing support objects still supply unrelated link
dependencies, so this remains a narrow VM test, not a fully sanitized server.

Baseline reproduction without changing the worktree:

```sh
python3 tools/ci/run_native_allocator_test.py --source-ref 78de18356 --case pool
python3 tools/ci/run_native_allocator_test.py --source-ref 78de18356 --case large-odd
```

Both must fail under UBSan. The original Git blob's LF SHA256 is
`2a94fc9cd3cec71584220c6d7b0cca2a2f0ae224460e0935db1d158003e62a9e`;
the equivalent original CRLF working file was
`7d87710528dac1b033d6367519d462adfc8cd396953d2f7c3f552996674ababa`.

Independent source review and reruns passed. `NO_MEMMGR` and `MINICORE` syntax
checks also passed. LLP64/32-bit runtime, broader allocator API edge cases,
full-server sanitizer coverage, sustained load and actual gameplay remain
outside the demonstrated test coverage. Candidate build/startup and production
readiness are recorded separately in the deployment receipt.
