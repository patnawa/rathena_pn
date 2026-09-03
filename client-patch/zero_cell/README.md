# Zero Cell client compatibility patch

Zero Cell adds two reward containers and the missing one-handed Encroached Axe.
The server definitions are in `db/import/zero_cell_item_db.yml`; clients also
need the matching item names, descriptions and resource names.

Install the metadata into a client root (the directory containing `SystemEN`):

```powershell
powershell -ExecutionPolicy Bypass -File install_client_patch.ps1 `
  -ClientRoot "C:\path\to\Ragnarok"
```

The installer is idempotent. It copies `SystemEN/itemInfo_ZeroCell.lua` and adds
the `zerocell` import to `SystemEN/itemInfo.lua` once.
