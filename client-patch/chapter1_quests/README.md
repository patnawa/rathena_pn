# Chapter 1 quest journal repair

Advancing the Lapine Messenger conversation completes quest 18368 and adds
18369. The shipped quest table lacks 18369, so the original client helper throws
nil-field errors from its title, description and reward callbacks. This overlay
adds complete records for the eleven missing World Tree quests 18369–18379
and 111 missing records in the Chapter 1 guide's later investigations.
It preserves every existing record and does not replace the client helper.

Descriptions follow `npc/custom/chapter1/CH1.c`, the quest database and the
curated Chapter 1 guide. The Land of Fire link uses the public survey entrance;
18377 is a completion record; 18379 describes Rubiel's Hazy Gate follow-up.
No quest progress, rewards, combat behavior or server files are changed.

This is a separate client repair layered over the frozen server source
`f8926fc0d251080697260d213fabb3c619c78d883ac022c31417686ebd1d38d8`.
Its new Git files and client artifact have their own identity. Existing native,
SQL and release-controller receipts retain their original identities; they are
not rebound to this repair or a new commit. Lua validation does not establish
complete rendered gameplay acceptance.

Copy both `SystemEN/Chapter1QuestInfo.lua` and `SystemEN/Chapter1GuideRecords.lua` into a closed client's matching directory
and append `dofile("SystemEN/Chapter1QuestInfo.lua")` after the existing imports
in `SystemEN/OngoingQuests.lub`. Back up the original loader first. All three
legacy loaders already delegate to that canonical file. Keep the current GRF
order and endpoint configuration; this repair does not require a GRF change.

Run the original compiled helper with a matching 32-bit Lua 5.1 runtime:

```powershell
python tools/ci/chapter1_quest_client_test.py --lua <lua5.1.exe> --client <client-root> --helper <original-questinfo_f.lub> --patch client-patch/chapter1_quests/SystemEN/Chapter1QuestInfo.lua
```

The regression reproduces the installed-data failure when `--patch` is omitted.
With the overlay, it exercises all three callbacks for all 180 guide IDs plus
the completion and Hazy Gate records (182 IDs total) through all
four loader paths, checks existing-record preservation, repeat loading and custom
overrides, and disables `table.insert` as in the client quest environment.
The 122 additions preserve all 60 existing guide records and all unrelated data.
Post-story daily quests outside that guide remain a separately tracked scope.
The generated companion is reproducible with `python client-patch/chapter1_quests/generate.py --check`.
