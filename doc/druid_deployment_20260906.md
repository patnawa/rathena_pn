# Druid deployment receipt - 2026-09-06

Deployment completed at approximately 14:34 ICT (07:34 UTC) on the owner's
Docker host. The 167-file source/data archive passed exact-byte candidate and
installed-file checks. Reviewed archive SHA256:
`690679784d7a91759ca8a4766e3e1d019977ebd00281fd89c3bd368615315340`.

The production-image `make -j4 server` used PACKETVER 20260219. Final isolated
`map-server --run-once` completed with no script/DB errors or memory leaks;
the existing root-user warning remains. The 31 extra item definitions were
included in that final parser load. Tests and unresolved limits are recorded in
[core](druid_integration.md), [client/progression](druid_client_progression.md),
[items](druid_item_compatibility.md), [episodes](episode_party_progression_audit.md)
and [combat](combat_bindings_audit.md).

## Installed binaries

| Binary | SHA256 |
| --- | --- |
| login-server | `63c219953b38f6b3f81fbe5b3aa70e41ab4f7c4fe0db09f7f795b37ba76ab35a` |
| char-server | `e4347f1e3e0d251513ae77bf29aba01a6202d5bcf8f9779cf17ccbfc12f35ee2` |
| map-server | `4e7521b9b5b5bdfe3dc3d948c161a27e6d64801be01779c83ad8380ccc742c92` |
| web-server | `9057196b4bc70f8ffd4835c2c9373c577c1c836286435ab62855e8a2c0d08232` |

All four exact inspected containers share `/app/rathena`, are running with zero
restart counts, and map authenticated to char. Web listens on 8888. No player
was online at either deployment preflight. Existing account values were not
changed by this deployment.

An initial attempt stopped before source/binary writes because `web` was not a
service in the production Compose file. Login was restored, then deployment
was retried using the four actual container names. The retry preserves each
container's configuration; it does not recreate containers or touch DB/FluxCP/npm.

Live-only custom `@go` help entries and `MF_MD_SELFDESTRUCTION` were inspected
and preserved before the final rebuild. No unrelated checkout edits were reset.

## Recoverability

On the server, under `/app/rathena-deploy-backups/`:

- `pre-druid-integration-retry-20260906.tar.gz` (45 MiB): prior four binaries
  and the existing scoped files.
- `pre-druid-integration-retry-20260906.sql` (22 MiB): pre-deployment Ragnarok
  database dump taken after core services stopped.
- The earlier preflight backup and all build/deployment logs are retained.

Restoring binaries requires stopping and restoring the coordinated core group,
not mixing old char with new map. New inert source files may remain after a
scoped-file rollback; restored import/config files keep them unloaded.

## Local client metadata

The owner client's `SystemEN/itemInfo.lua` now imports the clean-room Druid item
fragment once, preserving its previous imports. The Fashion fragment no longer
labels the eight enabled class outcomes unavailable. Installed fragments match
their repository copies exactly; the generic existing `EpisodClear20` icon is
used. No executable, GRF priority or protected reference assets were replaced.

Backups of the two existing client files are in
`server-work/client-before-druid-20260906/`. The Druid item fragment was a new
file. Restarting the client is required to read the updated metadata.

At this deployment, new initial-enchant recipes and material acquisition for
the 31 extra items were still pending. Correction from the subsequent read-only
identity audit: all 31 native names already exist in the original active client
table; the earlier unresolved results were missing server records, not missing
client aliases. No name-table patch is required. Rendering, player-attached combat, job
changes, fashion effects and relog persistence remain unverified; deployment
does not complete the broad episode/gameplay audit.
