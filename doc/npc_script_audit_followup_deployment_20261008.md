# NPC script audit follow-up deployment — October 8, 2026

The remaining NPC audit repairs are live. Production verification completed at
13:21 Bangkok time on October 8, 2026. The deployed implementation is
`957f42ea2dcb42a0a4564aef415ec2026a5d6a11` on `improvements-20261002`. It was
pushed to origin before rollout.

It follows the [first audit deployment](npc_script_audit_deployment_20261008.md)
and contains the "Follow-up release" items in the
[audit record](npc_script_audit_20261008.md):

- encounter-start rosters for Ghost Ship reports, the Final Battle story reward
  and the Sticky Sea story branch;
- The Undying boss and vision GIDs;
- a Twilight Garden ambush that no longer depends on the leader staying online;
- Dark Whisper and Geffen Event 2 continuing when a member logs out;
- smaller dialogue and selection repairs.

## Validation and cutover

The payload is the 16 files changed since the deployed commit. It was staged
over the morning's deployed candidate and checked against that commit's file
hashes. It passed the map-server build and all 105 full native release checks,
including private whole-NPC startup on a disposable database. The updated
`ghost_ship_report_claim_test.py` also passed natively in the same candidate:
25 cases, 102 assertions, including a refused late joiner.
`episode21_encounter_flow_test.py` needs Git history for its baseline fixtures,
so it was not run in the history-free candidate.

Read-only preflight found no drift and zero online characters. The guarded
map-only cutover then:

1. fenced ingress;
2. took a fresh quiescent census;
3. gracefully saved and stopped only the map server;
4. backed up every replaced file;
5. installed the 16 files, the rebuilt map binary and the runtime manifest;
6. restarted the map server, then reopened ingress after the fresh heartbeat and
   health checks passed.

Postflight verified every installed hash, the backups, the running executable
and runtime manifest, and seven healthy services. The map startup log has no
errors. The signed client feed is unchanged and verified, and the game is
online. No SQL schema or client assets changed.

## Pending and recovery

Rendered client playtesting of the changed encounters has not been performed.
Runs in progress at the restart ended with the map server. New Ghost Ship,
Final Battle and Sticky Sea runs record their rosters at the start.

Backups and the journal are at
`/app/pn-npc-audit-20261008b/production-cutover`. Its `release_deploy.py
--rollback` restores this release's preimages, which are the first audit
deployment. See the [receipt](evidence/npc_script_audit_followup_deployment_20261008.json).
Release scripts and local evidence are in `OPS/npc-audit-20261008b`.
