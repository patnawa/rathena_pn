# Reference font correction

The screenshot `Screenshot 2026-09-19 202943.png` contains Arial regular at a
12-pixel character height with non-antialiased GDI rendering. The 43-character
item-description sample matches pixel for pixel (239 x 11 ink bounds, 241 x 15
GDI text extent). Tahoma and Microsoft Sans Serif do not match.

The old multiplier changed every eligible GDI and GDI+ font in the process,
including fonts owned by Windows and the bank extension. A positive GDI height
specifies the whole character cell, not the character's height. Increasing that
number slightly does not reliably reproduce the reference's character size.
See [Microsoft's CreateFont documentation](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta).

This module chooses Arial, natural width and non-antialiased rendering for
ordinary fonts created directly by the game executable. Positive heights 9..16
become negative character heights of the same number, with two measured exceptions:
the 14-cell Basic Information font becomes character height 11. Runtime probes
identified that slot in HP, SP, Base Lv. and Job Lv. labels. Their reference rasters
match Arial 11 exactly, including `Base Lv. 68` at 57 x 8 ink pixels. The separate
13-cell HP/SP/AP value fonts become character height 10; the reference sample
`5031  /  5031` matches exactly at 61 x 7 ink pixels. The user's
`Screenshot 2026-09-19 213444.png` shows clipping with the former 14-character size.
Explicit negative heights and other font sizes are preserved. Bold, italic and underline remain
available. Large text, symbol fonts, rotated text and calls originating in other
DLLs are untouched. No scaling, drawing or text-measurement hooks are installed.

The installed filename `FontScaleOriginal.dll` and DirectDraw export are retained
for compatibility with the existing `FontScale.dll` bank loader. The old scaler
is replaced; keeping these filenames does not enable scaling. The packed client
has no visible CreateFont import suitable for the available WARP font patch, so
the executable is kept byte-identical.

Build with `build.ps1 -Output ABSOLUTE_DIRECTORY` using the MSYS2 i686 GCC
toolchain. Copy the shipping `FontScale.dll` and `BankUI.dll` into that test
directory and run `NativeFontTest.exe`. Tests cover the four GDI font-creation
entry points, text metrics, original structure preservation, external DLL and
symbol/rotation/large-font exclusions, DirectDraw forwarding and bank loading.
`native-font-render.bmp` and `basic-font-render.bmp` are synthetic renderings
for pixel comparison, along with `resource-font-render.bmp`. The user confirmed
the item/chat match and then that the corrected HP/SP/AP values fit in-game.

`FontFix.log` is overwritten at startup and records at most 16 font corrections.
It contains only font/API diagnostics, no account or game text. A `Ready` line
proves hook installation; a subsequent `CreateFont...` line proves that a game
font actually passed through the correction. In-game appearance and clipping
still require checking the login screen, chat, item description, NPC dialogue,
equipment window and bank panel. Desktop inspection was unavailable at build
time; the reference comparison is a headless GDI test, not a gameplay screenshot.

Project code: GPL-3.0-or-later. MinHook is the existing vendored BSD-2-Clause
dependency in `../account_bank/vendor/MinHook`; its license is retained in the
distributed `BankUI-LICENSE.txt`. Windows supplies Arial; no font file is shipped.
