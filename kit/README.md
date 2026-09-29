# Attack-Move for Vinifera ports

This kit contains the attack-move source and a focused patch for Vinifera
`71a692a5f4a7d61cd1654f2544ee4d17fe473d70`. The separate Tiberian Sun
Client demo is a reference implementation. Apply the patch only to that pinned
baseline; compare each integration point before adapting it to another port.

Select units, press A, then click a destination. Eligible units engage nearby
targets and continue toward the destination after combat. The default is one
order per activation. `AttackMove.PersistentTargeting=yes` keeps A-click mode
armed for more destinations; right-click cancels it. Acquisition defaults to
150% of weapon range and target retention to 300%. Weapon firing range does
not change. Medics and compatible repair units support allies along the route.

## Where to start

- `src/extensions/techno/attackmove.cpp` implements the command, input,
  target selection, synchronized execution and route behavior.
- `src/extensions/techno/attackmove.h` declares the engine entry points.
- `src/extensions/techno/attackmove_logic.h` holds pure decisions and protocol
  helpers. `attackmove_config.h` defines defaults, inheritance, ranges and CRC
  mixing. All four files are copied from the canonical engine tree.
- `patches/attack-move.patch` connects the core to Vinifera's existing command,
  event, AI, extension, INI, save/load and checksum code.
- `docs/integration.md` and `validation/engine-hooks.json` identify the reference
  executable, hook bytes, registers and stack expectations.

`manifest.json` lists the SHA-256 hash of every other kit file and records
whether each patched engine file was added or modified, along with its final
hash and size. The export tool verifies the patch against the pinned baseline
before making the ZIP.

## Adapting it to another port

1. Compare the focused patch with your engine. Match its command and mouse
   handlers, event serialization, AI hook, extension fields, save/load and CRC
   path to your own source.
2. Verify the hook bytes and calling convention against your executable. Run
   `py -3 tools/verify_hooks.py --game "path/to/game.exe"` for the pinned TSC
   reference. Its addresses apply only to the listed `game.exe` SHA-256. For
   another executable, inspect and document each changed hook yourself.
3. Allocate an unused synchronized event ID and set a distinct protocol and
   save marker for your build. Event 38 and markers in this kit are reference
   TSC values. Every multiplayer peer must run a matching build.
4. Add `CanAttackMove=yes` to eligible ground combat types, then choose the
   global and per-type settings in [configuration](configuration.md). Keep
   special unit roles excluded unless you have tested their orders.
5. Register the command in your client's keyboard editor. Bind A only when
   free; report conflicts and preserve existing user bindings.
6. Build in an isolated game copy. Test targeting, acquisition boundaries,
   combat recovery, cancellation, support, Move/Stop, fresh save/load and
   matching-build multiplayer. The isolated tests cannot validate native
   hooks or network play.

The patch contains attack-move integration only. It does not contain the old
Chilong tiberium change or the separate TI campaign projectile repair.
Stock TSC roster choices and map edits are part of the full reference project,
not mandatory for another mod.

## Testing and dependencies

The complete project includes the pinned Vinifera and dependency source in its
full-source ZIP. The pins are in `pins.json`; the focused kit omits that large
tree. Build and isolated tests require Windows, Python 3.12 or newer, Git and
Visual Studio 2022 Build Tools with the x86 C++ toolchain and Windows SDK.

Against an extracted and built full-source tree, run:

```powershell
py -3 tools/test.py --engine "path/to/TSC-AttackMove-source/engine" --compile-commands "path/to/TSC-AttackMove-source/build/compile_commands.json"
```

The kit includes the test runner and its relevant isolated tests so you can
repeat them against a ported engine. The runner writes generated fixtures and
a receipt to `build/tests` under the kit directory. See the full project's
`docs/testing.md` for native test instructions.

Native acceptance is pending for the configurable revision. If a playable
build is distributed with this kit, consult its separate package and native
receipts for the tested DLL hash, settings and pending cases. The earlier
native test used a different DLL and settings. Run the native checks for your
own build and record its DLL hash before calling it accepted.

## Licensing

Vinifera and these attack-move changes are distributed under
GPL-3.0-or-later. Read `LICENSE.md` and `CREDITS.md`. The `licenses/` folder
contains notices for the dependencies identified in `pins.json`; the full-source
package carries those dependencies and their licenses. Game and client assets retain
their own rights; this source kit does not relicense them.
