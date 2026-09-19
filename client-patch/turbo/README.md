# PN Turbo

An independent held-input implementation for the PN Windows client. The initial
profile follows the user's local MuhRO `YXExt.ini`: F1–F3 smart-cast, F5–F7 key
repeat, a 10 ms repeat gap, Alt+P toggle, and disabled on launch. No MuhRO binary
or source code is bundled.

## Player controls

Open **Turbo Setup.cmd**, or **Turbo Setup** in the PN launcher. Assign each key
to one mode, then save. Settings reload within about half a second; release any
held keys before using the changed profile.

* **Smart-cast** presses the hotbar key and clicks the current mouse target.
* **Key repeat** presses only the key, for self skills and items.
* **Alt+P** toggles the focused game instance. The toggle key is configurable.
* **Enter / Esc** pauses turbo while logged into a character. Enable it again
  after chat or menus are closed. Login-screen Enter preserves the startup setting.
* Optional **Alt+right-click** repeats right-click while both remain held.

Delays are the gap *after* a complete input cycle, between 10 and 5000 ms.
Windows timer scheduling and the input pulses add time; 10 ms does not imply
100 casts per second. Server cooldowns, cast time and animation restrictions
still apply. Smart-cast uses a 32 ms key press, a 50 ms targeting wait after
release, and a 32 ms mouse press. Regular key repeat retains its 16 ms pulse.

Turbo requires a focused, visible PN game and the authenticated bank companion
session. It suspends for the bank panel, native edit fields, modifier shortcuts,
focus loss and logout. Held keys must be released after suspension. Custom game
text boxes opened only with the mouse cannot be reliably identified through
Windows controls: pause turbo before typing into them.

## Integration and separation

`FontScale.dll` forwards the existing font exports and loads `BankUI.dll`, then
`PNTurbo.dll`. The approved `FontScaleOriginal.dll`, game executable and archives
are untouched. BankUI exports only a readiness boolean and game window handle;
the turbo module receives no account identifiers, credentials or bank balances.

The DLL uses a dedicated message loop, low-level input hooks and tagged
`SendInput` events. The scheduler tracks only configured physical holds. It
suppresses captured originals, ignores unrelated injected input and cancels its
own outstanding input when a hold becomes invalid. It never runs an unattended
macro or changes the server protocol.

`PN-Turbo.status.ini` contains transient state and counters for troubleshooting,
including the build, process ID, holds, clicks and accepted/rejected synthesized
mouse events. It does not record typed text, key names or player information.
It is not a release file. With several clients open, the latest instance to
write wins; this file is diagnostic, not shared runtime state.

The client receives only the DLL, setup executable, default INI and player help.
Source, compilers, tests and rollback copies stay under Server-Development.
`PN-Turbo.ini` is preserved by signed LAN updates.

## Build and checks

Run `build.ps1 -Output <absolute-development-output-directory>` with the existing
32-bit MinGW and .NET Framework compiler. The script builds the extension and
its bank/font loader integration, then checks:

* held-only scheduling, delays, fairness, focus/session interruption and rearming;
* the shipping Windows INI parser, hot reload, duplicate rejection and scan codes;
* the actual input phase code with a recording sender: targeting cannot share
  the key-release tick, release during the wait produces no click, and cancel
  releases an outstanding click;
* bank transport, UI and refresh regressions;
* setup defaults, atomic Unicode settings round trip and validation.

The native adapter tests install no desktop hooks and generate no desktop input.
The deployment wrapper also checks the real font-forwarding chain and approved
font metrics. `TurboConfigTest.exe --render-preview <absolute-png-path>` renders
the setup form for visual review without activating it.

Actual skill targeting still requires a gameplay check. Record that separately
from automated checks; a passing input test does not prove a skill was cast.

## References

* [MuhRO patcher / turbo behavior](https://wiki.muhro.eu/Patcher)
* [MuhRO configurable delays and toggle update](https://dis.muhro.eu/t/patch-notes-194-12-december-2025/3974)
* [Windows SendInput ordering and key state](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
* [Windows timer scheduling](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-settimer)

GPL-3.0-or-later, consistent with the surrounding PN/rAthena development tree.
