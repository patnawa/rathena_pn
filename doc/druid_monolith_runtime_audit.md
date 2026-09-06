# Druid Monolith runtime audit

Baseline: `4ab4f34a3`; reference implementation originally derived from GPL
[rAthena PR #9765](https://github.com/rathena/rathena/pull/9765).

## Confirmed defect and correction

Glacial Nova and Glacial Stomp previously used only the cached coordinates in
`SC_GLACIER_SHEILD`. The unit timer refreshes that status for `interval + 100`
milliseconds (currently 1100 ms). Unit deletion does not immediately remove
this status, and leaving the unit area also leaves a brief cached status.

Consequently, after Monolith deletion or walking outside its range, the old
functions could still damage the old area or teleport back to it. Replacing a
Monolith could also leave the old coordinates usable until the next refresh.

The active client's English skill description explicitly requires an existing
Monolith and forbids Stomp outside its range. Source inspected:
`english.grf:data/luafiles514/lua files/skillinfoz/skilldescript.lub`, SHA-256
`816457fbe7b99a4b650ebf9ae961f41c3989920161659042e8fcbf765e423292`,
entries `AT_GLACIER_MONOLITH` and `AT_GLACIER_STOMP`. This is client contract
evidence, not a replacement claim that all translated balance values are current.

The new resolver requires a currently live unit belonging to this caster, on
the same map, with the caster inside the unit's actual configured range. Nova
and Stomp retain their required-status check but use the live unit coordinates.
The database range remains seven cells (a 15-by-15 square, including its edge).
No damage formula, radius, duration, charge count or equipment data is changed.

On refusal, the resolver does not remove statuses, units or unrelated state.
Stomp sends failure rather than a success effect. Its existing path/cell check
is retained. The engine's existing resource/cooldown charging order is unchanged:
this patch does not promise a refund after a cast-end movement failure.

## Lifetime and cleanup review

`skill_delunit` first marks the unit dead, then clears its group reference and
removes its map/id entries; group deletion removes it from the owner's unit
list. The resolver reads the owner's existing shared-group list synchronously
and rejects dead/unplaced units. It does not cache a pointer across timers.

Callers copy the returned coordinates before any movement or other event can
run. No returned unit pointer is dereferenced after `unit_movepos`, which can
invoke NPC touch scripts. Existing unit deletion/sprite cleanup is unchanged.

## Native behavioral regression

```sh
python3 tools/ci/druid_monolith_runtime_test.py | g++ -std=c++17 -O2 -Wall -Wextra -fsanitize=address,undefined -x c++ - -o /tmp/druid_monolith_runtime_test
/tmp/druid_monolith_runtime_test
```

Result: 43 checks passed under AddressSanitizer and UndefinedBehaviorSanitizer.
The emitter extracts the actual resolver, Nova and Stomp function bodies from
the working source. The map/status/packet functions are controlled test seams;
this is not an in-game client test or a duplicated implementation of the logic.

Cases cover a valid unit, deleted unit with lingering status, range seven/eight,
diagonal boundary, replaced-unit coordinates, dead/unplaced/wrong-map units,
wrong owner, another skill's group, missing unit/group/status data, disabled
range, dead/unplaced caster and failed path checking. Refused casts do not emit
damage, movement or a success effect.

The same expectations against the deployed baseline reproduce the failure:

```sh
python3 tools/ci/druid_monolith_runtime_test.py --source-ref 4ab4f34a3 | g++ -std=c++17 -O2 -fsanitize=address,undefined -x c++ - -o /tmp/druid_monolith_before_test
/tmp/druid_monolith_before_test
```

Expected regression result: exit 1 at the deleted-Monolith case. The current
Druid factory also passes full syntax checking at `PACKETVER=20260219`.

Remaining: actual client movement/animations, live skill charging observations,
and the upstream uncertainty over Stomp's exact one-cell-offset destination.
