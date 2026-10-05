# Time Dimensions tooltip corrections

`corrections.json` contains 57 distinct exact replacements for 58 description
lines on 27 weapons. Every record links Gravity's item description. The Lua
module runs after the existing item merges and changes only those lines.
Names, resource references, cards, slots and unrelated descriptions stay intact.

The patch states that the autocast skill must already be learned, adds the high
elemental requirement to the Elemental Master books, corrects the Soul Ascetic
and Night Watch trigger descriptions, and corrects the Meister set's Axe Stomp
label and Night Watch launcher's weapon category. It preserves the two documented
50% procs and the Hyper Novice's fixed Level 5 proc.

Build a separate candidate from an unpatched client with its actual Lua 5.1
interpreter:

```powershell
python client-patch/time_dimensions_tooltips/build.py --client C:/path/PN-Client --output C:/path/new-tooltip-candidate --lua C:/path/lua5.1.exe
```

The builder does not install or publish. It validates exact baseline lines,
executes the full client item loader, compares the entire resulting item table,
and checks idempotence. Only the two paths in `tooltip-candidate.json` belong in
the release payload. The other copied files are a test fixture.

The reviewed 2026-09-22 release workspace is
`Server-Development/time-dimensions-20260922` outside the repository. Its
`release.py prepare` builds the cumulative clean candidate from the verified
signed feed, prepares a four-file ZIP and signed feed manifest, and runs the
normal client checks. `release.py publish` is a distinct explicit operation with
feed-baseline checks. `install-final.py` uses `PNLauncher.exe --update`, backs up
nested paths, preserves unrelated files, and verifies the new module's
`existed=false` rollback entry.

See [the audit](../../doc/dimension_other_jobs_audit_20260922.md) for scope,
server findings, official sources, and the limits of the verification.
