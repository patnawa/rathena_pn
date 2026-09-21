# NPC name font correction — 21 September 2026

The reference-font correction shrank every positive 13-cell font request to
Arial character height 10 so that HP/SP/AP numbers fitted the Basic Information
panel. NPC names use a bold font at the same requested height and were also
shrunk. Font profile 5 distinguishes the bold name font from regular resource
values and uses a moderate 12-character height. The user confirmed the shared
NPC/character font was enlarged but found the initial 13-pixel size too large.

Bounded observational diagnostics on the installed client identified these
separate font handles:

| Text | Requested height | Weight | Previous character height | Corrected height |
| --- | ---: | ---: | ---: | ---: |
| NPC/character names (Kafra Employee, Healer observed) | 13 | 700 (bold) | 10 | 12 |
| HP/SP values | 13 | 400 (regular) | 10 | 10 |
| Basic Information labels | 14 | 400 | 11 | 11 |
| Inventory count fonts | 11 | 400 or 700 | 11 | 11 |

Kafra Employee measured 77 × 12 pixels in the original running client. The
13-pixel trial measured 101 × 16; the selected 12-pixel profile measures 89 × 15.
Other text sharing the bold 13-cell font uses the same size. Explicit negative-height requests, body text, larger
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
The 12-pixel follow-up is retained in `Server-Development/npc-name-font-r5-20260921`.
