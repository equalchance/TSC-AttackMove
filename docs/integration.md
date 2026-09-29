# Portable client integration

`tools/stage.py` assembles a new playable folder from an installed Tiberian Sun Client and a validated, sanitized `Vinifera.dll`. It never writes to the installed client. Set `--game`, `--dll`, and `--out` explicitly so the source works after extraction anywhere, including a path with spaces.

```powershell
py -3 tools/stage.py --game "path/to/Tiberian Sun Client" --dll "build/public/Vinifera.dll" --out "path/to/TSC-AM"
py -3 tools/package.py --stage "path/to/TSC-AM" --out "path/to/archives"
py -3 tools/export_kit.py --out "path/to/AttackMove-Kit.zip"
```

The stage retains the installed executable, launcher, assets, maps, and `Game.edb`. It excludes player settings, saves, logs, debug symbols, client caches, custom maps, and private tests. `content/SUN.ini` supplies clean portable settings. Both root and `INI/Keyboard.ini` come from the installed clean `INI/Keyboard.ini` template; `AttackMove=65` adds A without changing other bindings. The client hotkey catalog gets an Attack Move entry. The staged client uses `ModMode=true` and disables automatic update checks so a stock update cannot replace the experimental DLL. A player readme ships in the game folder.

The rules pass enables `CanAttackMove=yes` with `AttackMoveScanInterval=15` on eligible infantry and vehicles. It checks `Rules.ini`, Firestorm rules, shipped map type sections, and map numbered type lists. A map that arms an otherwise unarmed transport gets a local opt-in. Engineers, hunter-seekers, harvesters, jumpjets, and other special roles remain outside the combat opt-in. Medics and repair vehicles use the engine's automatic support path, so they do not require a combat flag. The generated `manifest.json` lists enabled types, map changes, and cases for review.

The 2026-09-26 installed baseline audit enabled 33 rules types: `4TNK`, `ART2`, `BGGY`, `BIKE`, `CTECH`, `CYBORG`, `CYC2`, `DEFENDER`, `DOGGIE`, `E1`, `E2`, `E3`, `ELCAD`, `FLMTNK`, `GHOST`, `HMEC`, `HUEY`, `HVR`, `JFISH`, `JUGG`, `MMCH`, `MUTANT`, `MUTANT3`, `MWMN`, `OXANNA`, `REAPER`, `SMECH`, `SONIC`, `STNK`, `SUBTANK`, `TTNK`, `UMAGON`, and `VISC_LRG`. The pass scanned 117 shipped maps plus 16 option and Firestorm overlays. Eleven maps needed local flags for armed type replacements. The manifest gives their paths and types. `INI/Game Options/No Silos.ini` defines `STORAGEU`, an unselectable pseudo unit with a zero-damage suicide weapon; it remains flagged for review and is not attack-move enabled.

`tools/package.py` verifies every staged file against the manifest, scans all playable and source bytes for local identity markers in ASCII and UTF-16, then writes deterministic playable and SOURCE ZIPs. It verifies each archived file after writing. The source ZIP contains the full project and vendored dependency source and notices. Git internals, build output, private runtime files, handovers, backups, and old native receipts stay out. The package receipt records archive hashes and changed baseline paths. Use `--deny TEXT` for another private marker.

`tools/export_kit.py` makes the smaller `AttackMove-Kit.zip`. Its manifest records every kit file hash and the final hash, size, and added/modified status of each engine file named by the focused patch. It checks the patch against the pinned upstream source before export. The kit carries the canonical attack-move source and configuration helper, sample presets, isolated test runner, hook evidence, GPL license and upstream credits. The full-source ZIP is the buildable package with dependencies.

An optional native receipt may be supplied with `--receipt PATH`. It must contain `dll_sha256` matching the staged DLL, `native_gameplay` (`pass`, `partial`, or `pending`), a `checks` object with explicit statuses, and `multiplayer` (`pass`, `partial`, or `unverified`). Without that receipt, the package remains labeled native gameplay pending and multiplayer unverified. A private smoke folder is never part of the playable ZIP.

Run `py -3 -m unittest discover -s tests -p test_staging.py -v` from the project directory for the staging and archive checks. Native gameplay, multiplayer, and non-16:9 behavior require separate play receipts; script tests do not establish them. The old native-v2 receipt applies to an earlier DLL and does not establish acceptance for the configurable revision.
