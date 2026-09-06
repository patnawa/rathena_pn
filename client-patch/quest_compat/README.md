# Quest compatibility client patch

Episode 19 uses current kRO quest IDs `18119-18121`. Older rAthena Moscovia
content historically used those same IDs, so this server migrates Moscovia to
reserved IDs `900200-900202`. Chapter 2 also uses local IDs `27101-27104` and
`27110-27119` because its late-kRO quest table is not present in the supported
client bundle. This patch supplies readable client quest text for both sets.

Run from PowerShell against the Ragnarok client data folder:

```powershell
.\install_client_patch.ps1 -DataRoot 'C:\path\to\Ragnarok\Data'
```

The installer is idempotent and keeps the first original file as
`SystemEN/OngoingQuests.lub.bak-before-quest-compat`.
