# Chapter 2 client compatibility patch

Chapter 2's post-2025 map files are not fully extractable from current encrypted
Gravity archives. The server therefore ships a synchronized map-cache alias set.
Clients must append `resnametable_chapter2.txt` to the active
`data/resnametable.txt` and load `SystemEN/itemInfo_Chapter2.lua`, or run
`install_client_patch.ps1` to install both automatically.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File install_client_patch.ps1 `
  -ClientDataDirectory "C:\path\to\Ragnarok\data" `
  -BaseResnametable "C:\path\to\extracted\data\resnametable.txt"
```

When native Chapter 2 GAT/GND/RSW resources become legitimately available,
remove these Chapter 2 alias lines and rebuild the server cache from the same
native GAT files.
