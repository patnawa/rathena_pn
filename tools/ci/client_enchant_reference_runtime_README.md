# Complete Lua enchant registration with an inspected reference fallback

`client_enchant_reference_runtime_test.py` executes the complete original and
patched enchant lists and the actual supplied helper in native Win32 Lua 5.1.
It records native-registration requests. It does **not** execute the game,
emulate its whole backend, or claim protected-2026-client behavior.

Run from the repository root under WSL:

```sh
python3 tools/ci/client_enchant_reference_runtime_test.py
```

## Evidence for the explicit slot callback

The previous strict test deliberately refuses missing item-info metadata.
Read-only inspection of the supplied **unprotected 2025** reference executable
now establishes its narrower native fallback behavior. SHA-256:
`33d4d9af476b8d24b5954d38d121b2bbe93044681b2945bb9b235fd9b25990cb`.
No executable bytes were changed, protections bypassed, or game process started.

| Inspected location | Meaning |
| --- | --- |
| String `0xfe67e0`, registration `0x644659`, function pointer `0x64468b` | Registers C_GetSlotCount at `0x648350` |
| Call `0x648436` to `0x6866c0` | Calls the actual ItemDB_To_ItemID Lua lookup wrapper |
| Branch `0x64843f` to `0x6484a9`, double `0xfe68a0` | A failed name lookup returns -1, not zero |
| Call `0x648465` to `0x696970` | Known ID queries its item-info slot count |
| Call `0x696988` to `0x692aa0`, load `0x696990` | Looks up the metadata row, then reads its integer field at +0x30 |
| `0x692b0f` / `0x692b14` | Missing metadata selects the static default row at `0x122ded8`; found metadata selects the existing row |
| Call `0x692b4b` to `0x6925e0`; store `0x692664` | Default-row constructor explicitly initializes the slot field to zero |

The test pins the executable hash and rechecks the relevant strings, instruction
bytes, relative call destinations, and -1 double through a PE32 section reader.
Its explicit test callback returns zero **only for an already-resolved positive
ID with an absent metadata row**. Malformed existing metadata and unknown names
still fail. Each fallback query and every missing identity remain in the report;
zero is not presented as the true slot count of an undefined equipment item.

Only that explicit callback double changes. Neither actual Lua helper nor list
is rewritten. The strict missing-metadata test remains available separately.
The other C callbacks are success-returning recorders, not the native engine.

## Observed full-list result

Before the separate twelve-crown metadata installation, both complete lists pass
actual CheckFile and LoadAllData using this reference callback:

- Original: 159 groups and 7,193 recorded callbacks.
- Patched: 164 groups and 7,298 recorded callbacks.
- All 7,193 original callbacks remain identical as multisets (Lua pairs order
  is not promised), not merely the former 931 complete-prefix callbacks.
- Exactly 105 callbacks are added: 22 targets, 58 perfect recipes, and five each
  for order, eligibility, random options, reset, and caution.
- The same 43 absent metadata records cause 44 queries in each complete list.
  These missing records still require implementation; none is hidden.

This strengthens full Lua registration coverage under observed **2025-reference
semantics**. It does not establish that the protected 2026 client behaves the
same, nor prove rendered menus, purchases, charging, slots for absent gear,
inventory persistence, or any remaining missing server item definition.

After the separately verified crown and Shadow-166 metadata installations, the
same active full-list test passes with unchanged group/callback counts. The
explicitly reported remaining metadata gaps are now 27 (28 fallback queries),
not zero. See `doc/druid_shadow166_deployment_20260906.md` for the two deep-merge
proofs, historical checkpoints and installed hashes.
