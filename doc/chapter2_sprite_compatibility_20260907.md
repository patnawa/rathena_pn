# Chapter 2 missing monster sprite repair

The client error in Screenshot 2026-09-07 185913 reports a missing monster
SPR filename. MSCESXi was saved in `rgs_dun1`, where four custom monsters use
IDs 22702-22705. The active `data.grf` JobNameTable has no entries for those
IDs, or for any of the 24 definitions in `chapter2_mob_db.yml`. None of the
higher-priority client GRFs overrides that job-name table.

## Appearance-only compatibility

`db/chapter2_mob_avail.yml` maps all 24 custom monsters to existing monster
appearances. `db/import-tmpl/mob_avail.yml` imports it for new installations;
the same import was added to the live `db/import/mob_avail.yml`. Existing
installations must merge that footer import, not overwrite their own aliases.

For Distorted Ragsroot the mappings are:

| Monster | Fallback appearance |
| --- | --- |
| Folnir Nilf | Dark Priest |
| Folnir Myrk | Rybio |
| Folnir Hre | Phendark |
| Feeos | Dark Lord |

These are deliberate visual substitutes, not the native Chapter 2 artwork.
The mob-availability handler changes the transmitted look only. Monster IDs,
names, combat data, skills, drops, and quest objectives are unchanged.

## Verification and deployment

- All 24 Chapter 2 identities are covered exactly once.
- Every replacement resolves to an existing Renewal monster and client
  JobNameTable entry. All 17 distinct replacements have monster SPR and ACT
  entries in the active base GRF.
- Both template and deployed import paths parse and reference the new file.
- Live import SHA-256: `a8a8f1a18c88097cd0a7b31e473ca47752e095c67067b26bc4805684ef480978`.
- Alias file SHA-256: `9de7e4cd53b420a1ef2d2be34b2582f7386a3a451a878068736cccae5afcceb4`.
- MSCESXi's last position was backed up to database table
  `gm_ch2_sprite_rescue_20260907`, then moved to the saved town while offline.
- The previous live import is backed up under
  `/app/rathena-deploy-backups/chapter2-sprite-repair-20260907/mob_avail.yml`.

In-game traversal and rendering still require player confirmation. No client
executable or original GRF is modified by this server-side sprite repair.
When native Chapter 2 sprite mappings and resources are installed and tested,
remove this fallback import during a coordinated deployment.
