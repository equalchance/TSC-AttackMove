# Tiberian Sun Client Attack-Move

A configurable port of Chilong Public Experimental Build 5's attack-move to
Tiberian Sun Client's Vinifera 71a692a engine. This project contains the source,
dependencies, focused patch, presets, tests, and scripts used to build a demo.

Press **A**, then click a destination. Units engage eligible targets along the
route and continue afterward. A successful order exits targeting while keeping
the units selected; invalid destinations remain active for retry. Right-click
cancels targeting. Move and Stop replace the unit's route. Medics can heal wounded infantry along
the route; compatible repair units can do the same for vehicles. Ordinary
unarmed ground units move with the group.

By default, combat acquisition uses 1.5 times the selected weapon's range and
pursuit retains a valid target up to 3 times that range. These percentages and
input behavior are configurable. Weapon firing range is unchanged.
Armed enemy defenses and armed hostile wildlife are valid targets; unarmed
buildings and civilian infantry are excluded from automatic acquisition.

The configurable revision has no native gameplay acceptance receipt. Multiplayer
is unverified. Use fresh scenarios and saves made by this build.
Do not mix its DLL with another attack-move release or engine baseline.
Native test results are recorded separately from automated test results.

## Downloads and integration kit

[Download the experimental release](https://github.com/equalchance/TSC-AttackMove/releases/tag/experimental-config-v1).

- **AttackMove-Kit.zip**: the small integration kit for TI developers and other Vinifera mod authors.
- **TSC-AttackMove-SOURCE.zip**: the complete reference source with pinned dependencies.
- **TSC-AttackMove-playable.zip**: a separate Tiberian Sun Client demo.

Start with the [developer guide](kit/README.md), [configuration settings](kit/configuration.md), and [three presets](kit/presets).
The canonical implementation is under `engine/src/extensions/techno/attackmove*`.
This repository includes the dependency source so it can build without the original workspace.

**Experimental:** 50 focused checks and six staging checks passed. The source archive also built after extraction into a directory containing spaces. The user confirmed a visible native game window; gameplay, preset, support, Move/Stop and save/load acceptance remain pending. Multiplayer is unverified. See the [native acceptance record](release-validation/native-acceptance.json).

## Build

Install Python 3.12 or newer, Git, and Visual Studio 2022 Build Tools with the
Desktop development with C++ workload, Windows SDK, and CMake tools.
The full-source archive includes the pinned dependencies. From this directory:

```powershell
py -3 tools/build.py
py -3 tools/test.py
py -3 tools/sanitize.py
```

The raw DLL is written to `build/Vinifera.dll`; the distributable copy is
`build/public/Vinifera.dll`. Sanitization changes only diagnostic path prefixes
in data sections and verifies that executable sections remain unchanged.
The build script discovers Visual Studio and the Windows SDK locally.

For configuration keys, inheritance and sample settings, see
[kit/configuration.md](kit/configuration.md). The three presets cover default
single-order controls, repeated targeting at 150%/300%, and repeated targeting
at 100%/100%. Copy the chosen keys into the game rules; the presets do not
create separate game modes.

## Stage and package

```powershell
py -3 tools/stage.py --game "path/to/Tiberian Sun Client" --dll build/public/Vinifera.dll --out "path/to/TSC-AM"
py -3 tools/package.py --stage "path/to/TSC-AM" --out "path/to/archives"
py -3 tools/export_kit.py --out "path/to/AttackMove-Kit.zip"
```

Staging creates a separate directory and preserves the input installation.
Packaging verifies file hashes and checks the source and playable archives.
The kit export verifies the focused patch and hashes every included file.
Keep test sessions in a separate copy so logs and saves cannot enter a release.
See [integration instructions](docs/integration.md) and
[test instructions](docs/testing.md).

## Source provenance

`pins.json` identifies the target executable, upstream commit and dependencies.
`patches/attack-move.patch` is the complete engine change against that commit.
For a checkout with Git metadata, `py -3 tools/source.py --export` regenerates
the patch and verifies that applying it to the baseline reproduces the source.
The source ZIP also contains the already-patched engine tree for direct builds.

The port preserves the baseline Vinifera spawner and existing event IDs 36/37.
Attack-move uses event 38, protocol marker `0x54534103`, and save marker
`0x54535303`. It adds the per-type keys `CanAttackMove` and
`AttackMoveScanInterval`. Defaults are `no` and 15 frames; invalid scan intervals
fall back to 15. Combat types opt in through game data. Other ports must verify
their own event allocation, protocol marker, save marker, hook bytes and
calling conventions against their executable.

This port does not add Chilong zoom, selection filters, sidebar changes, or
custom cursor assets. Features already present in the target baseline remain.

## Licenses and credits

Vinifera and the attack-move changes are distributed under GPL-3.0-or-later.
Read [the license](engine/LICENSE.md) and [upstream credits](engine/CREDITS.md).
Dependency license files are included with each dependency under `engine/`.
Chilong Edition supplied the attack-move implementation. Tiberian Sun Client
supplies the demo's game/client content and its existing credits.
The source license does not relicense the original game assets.
