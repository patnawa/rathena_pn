# Biosphere client quest patch

The server uses MuhRO-compatible quest behavior for Varmundt's Biosphere,
Depth 1, and Depth 2. The current translated kRO quest table still describes
the older combined Depth 1 objectives and has no records for the custom Soul,
Venom, Temple, or 3,000-kill quests.

Run this once from PowerShell, pointing `DataRoot` at the Ragnarok client
folder that contains `SystemEN`:

```powershell
.\install_client_patch.ps1 -DataRoot 'C:\path\to\Ragnarok\Data'
```

If Windows blocks local PowerShell scripts, invoke it with
`powershell.exe -ExecutionPolicy Bypass -File .\install_client_patch.ps1 ...`.

The installer accepts only a plaintext `SystemEN/OngoingQuests.lub`, keeps the
first original as `OngoingQuests.lub.bak-before-biosphere`, and replaces its
own marked block on later runs. It does not modify the compiled
`OngoingQuestInfoList.lub`.

The added `900100-900107` IDs are intentionally in the server's reserved
custom quest range and must remain synchronized with `db/import/quest_db.yml`.
