# Repeatable bug hunts

Run from Linux/WSL with Python 3.11+, PyYAML, GCC/G++, make, and sanitizers:

```sh
python3 tools/ci/bug_hunt.py --output ../bughunt-results \
  --client-root /path/to/client --lua /path/to/lua5.1
```

The output directory must be new and outside the repository. The runner covers
transactions, party progression, client checks, recovery helpers, combat helpers,
and release validation. It continues after failures and writes individual logs,
`report.json`, `dashboard.md`, `identity.json`, and `identity-final.json`. Identity contains current
source/data/configuration hashes (including ignored imports), Git revision and
working-tree changes, available server binary hashes, active archive order/hashes,
and client loose-file hashes. It does not copy source/configuration contents.
Both identities include the supplied client. A changed input or an identity
capture failure prevents a pass and is retained in the report. Checks that time
out have their whole process group stopped, including compiler/native children,
before later checks run. Repeated selections of the same area run once.
The runner clears inherited `PYTHONOPTIMIZE` so Python assertion checks remain
active, and forwards `--lua` as `LUA51` to fixtures that read it from the environment.

Without a client and Lua, the installed quest-loader test is explicitly blocked.
To run only server checks, use repeated `--area` arguments. Add `--native` for
the real script VM party and Rune fixtures; these require a coherent Linux map
object build with packet version 20260219. Old support objects can fail to link
or mismatch current headers. Build an isolated current candidate first.

On Windows, also run `python tools/ci/client_launcher_test.py` to execute the
shipping launcher against disposable fixtures. It never launches the game.

Every automated result remains distinct from acceptance. A zero exit code means
the selected offline checks passed on stable server and supplied client inputs, not that rendered
gameplay, SQL crash durability, or the installed production build was verified.
Client hashes describe the initial and final inventory; keep the client files
stable during the run. Reports retain source hashes even when the working tree is dirty.

Use the repository's **Reproducible bug** issue template. A fix needs the same
case failing before and passing afterward, with nearby regressions checked.
Track deployment separately from local validation. Do not convert unexecuted
acceptance cases into passes or infer full combat correctness from bonus-expression
tests. Full release startup still uses `release_checks.py` in an isolated build.
