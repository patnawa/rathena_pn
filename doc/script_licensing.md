# PN source signature

Our maintained scripts carry a PN banner naming **PN Development Team**, the contribution license and a direct link to their source. The banner is a source comment; it does not add dialogue, announcements or loading screens to the game.

Project-written contributions are offered under **GNU GPL version 3 or later**. See the repository's [LICENSE](../LICENSE) for the license text. The contribution notice does not claim authorship of an entire imported script. Existing rAthena and other contributor credits remain intact, and any separately identified third-party terms continue to apply. Game data, artwork and copied client tables are not relicensed by these notices.

Source: [patnawa/rathena_pn](https://github.com/patnawa/rathena_pn). The per-file banners link to the current source; Git history records individual revisions and contributors.

## Coverage

[The script notice manifest](script_notices.json) lists project-maintained NPC scripts, development and deployment tools, tests, and client patch scripts. The initial inventory uses script changes after the imported rAthena baseline `64fd20c6b`, rather than claiming unchanged upstream scripts as ours. Native engine files retain their existing upstream notices; this inventory covers scripts and project tools.

Client Lua payloads and tables can be byte-pinned, bundled or copied from external game data. Their companion notices are in [client source notices](../client-patch/SOURCE-NOTICES.md) and the manifest. Their bytes are deliberately unchanged. The license applies to our patch logic and contributions, not to external material contained in those files.

## Maintaining the signature

Run `python tools/script_notices.py` to check the inventory. For a new project script, add its path to `doc/script_notices.json` with `notice: inline`, the appropriate comment prefix and a category, then run `python tools/script_notices.py --apply`. Preserve existing author and license notices. Use a companion entry for a byte-sensitive or externally sourced client payload, and list its direct source link in `client-patch/SOURCE-NOTICES.md`.

The updater preserves BOMs, shebangs, leading encoding declarations, line endings and the original script body. It refuses to silently replace an existing PN banner with different details. Keep backup and deployment evidence outside the source tree.
