# NPC name font correction — 21 September 2026

The reference-font correction shrank every positive 13-cell font request to
Arial character height 10 so that HP/SP/AP numbers fitted the Basic Information
panel. NPC names use a bold font at the same requested height and were also
shrunk. Font profile 4 limits that exception to non-bold requests, allowing the
bold NPC font to retain its full 13-character height.

Bounded observational diagnostics on the installed client identified these
separate font handles:

| Text | Requested height | Weight | Previous character height | Corrected height |
| --- | ---: | ---: | ---: | ---: |
| Kafra Employee, Healer | 13 | 700 (bold) | 10 | 13 |
| HP/SP values | 13 | 400 (regular) | 10 | 10 |
| Basic Information labels | 14 | 400 | 11 | 11 |
| Inventory count fonts | 11 | 400 or 700 | 11 | 11 |

Kafra Employee measured 77 × 12 pixels in the running client. The corrected
native font measures 101 × 16. Other text sharing the bold 13-cell font receives
the same full height. Explicit negative-height requests, body text, larger
display fonts, system fonts and fonts owned by the bank DLL retain their rules.

The native regression fails against the original font DLL and passes after the
fix. It covers all four font-creation APIs, input structure preservation, bold
name rendering, regular resource values, both inventory weights and bank-loader
compatibility. Body, Basic Information and resource-value bitmap outputs remain
byte-identical to the accepted reference profile. The shipping DLL contains only
the existing four font-creation hooks; temporary drawing observations are not
included. In-game acceptance of the corrected rendering remains a separate step.

Reproduce with `client-patch/native_font/build.ps1 -Output ABSOLUTE_DIRECTORY`.
Copy the shipping `FontScale.dll` and `BankUI.dll` into that directory and run
`NativeFontTest.exe`. The new name rendering is `npc-name-render.bmp`.
Runtime diagnosis, the failing/passing test runs, installation backups and
release evidence are retained in `Server-Development/npc-name-font-20260921`.
