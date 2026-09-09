# Client usability update

Install after the enchantment repair client patch. Close the game, back up `SystemEN/itemInfo.lua`, and copy `Start Game.cmd`, `Check Client.cmd`, `tools/` and `SystemEN/` into the matching client directory.

- **Start Game.cmd** checks the client files and starts Ragexe with the game directory as its working directory. It leaves a readable error on screen if a required file is missing.
- **Check Client.cmd** runs the same checks without starting the game.
- Item names no longer carry source-server suffixes. Official source labels appear at the bottom of tooltips; the ExampleRO custom-name placeholder is removed. Item IDs, database links, descriptions, art and slots are preserved.

The launcher checks DATA.INI archive presence, repeated priorities, gaps, duplicate archives, classic and Event Horizon GRF headers, this loader's literal Lua imports, and the FontScale configuration. It reads only archive headers, so it does not scan the multi-gigabyte base archive on every launch. It does not download files, alter game settings, require administrator privileges, or contact a server. PowerShell's execution-policy override applies only to the launcher process.

The file check does not validate all compressed contents or prove that gameplay works. The launcher opens the existing Ragexe.exe without modifying it. Existing graphics, sound, controls and font-size preferences remain as configured.

Validation on the supplied client: all nine active archives passed; seven launcher fixtures covered valid input and failure cases. Native Lua 5.1 registered 26,906 items, including 4,947 cleaned names. Item tables, resource names and slots were unchanged; source footers were verified. No rendered game session was performed.

For a different client loader, merge only the presentation settings (`DisplayServer = 3`, `DisplayCustomServer = 0`, `CServerName = 'Custom'`) into its own itemInfo.lua rather than replacing the file. Keep the existing import and override order.

Rollback: restore the previous SystemEN/itemInfo.lua and remove the three added launcher files. The active installation backup and validation evidence are under `server-work/client-usability-20260909` in the owner's game directory.
