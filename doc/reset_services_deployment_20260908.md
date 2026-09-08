# Reset and card removal activation — 2026-09-08

Enabled two existing imports in `npc/scripts_custom.conf`:

- `npc/custom/resetnpc.txt`: Reset Girl at `prontera,150,193`.
- `npc/custom/card_remover.txt`: Wise Old Woman at `prt_in,28,73`.

The live change removed the two comment prefixes. Existing scripts, prices, and
card-removal failure behavior were preserved. No engine, client, schema, or
player-record edits were part of activation.

## Verification

- Local strict episode integrity audit: 905 enabled scripts, no integrity errors,
  and zero content-completeness warnings.
- Live configuration and both NPC scripts backed up before activation.
- Login admission closed and zero online characters confirmed before stopping map.
- Fresh map startup reached `Map Server is now online.` without Error/Fatal or
  script-error markers; login admission reopened.
- Both enabled imports verified in the installed configuration.

Backup directory:
`/app/rathena-deploy-backups/enable-reset-npcs-20260908T112600Z/`.
It contains the original configuration, both scripts, startup log, and a receipt
with before/after configuration checksums.

## Player acceptance

Visit both NPCs and open their dialogues. Reset Girl offers skills, stats, or
both. Card removal retains destructive failure options and confirms them before
charging. No player reset or card-removal transaction was performed during
deployment verification.

## Rollback

During a controlled reload or maintenance window, restore
`scripts_custom.conf.before` after checking for subsequent edits, or comment only
these imports again. Restart or reload affected scripts and verify startup/login.
Disabling the NPCs does not undo transactions players have already completed.
