# Attack-Move configuration

Put global settings in `[General]` in Rules.ini. Map Rules.ini overlays can
change them for a scenario. Type keys go in the unit or infantry type section.
Missing type overrides inherit the effective global value, including a later
map override. A type can opt out of support without disabling combat attack-move.

| `[General]` key | Default | Meaning |
|---|---:|---|
| `AttackMove.PersistentTargeting` | `no` | Keep A-click armed after a successful order. |
| `AttackMove.RightClickDeselects` | `yes` | Deselect when right-click cancels armed targeting. |
| `AttackMove.AcquireRangePercent` | `150` | Radius to find a new target, as a percentage of selected weapon range. |
| `AttackMove.RetainRangePercent` | `300` | Radius to keep a valid current target. |
| `AttackMove.TargetArmedBuildings` | `yes` | Allow hostile armed defenses. |
| `AttackMove.TargetArmedWildlife` | `yes` | Allow armed hostile units owned by passive houses. |
| `AttackMove.SupportEnabled` | `yes` | Allow automatic healing and repair on the route. |

| Type key | Default | Meaning |
|---|---:|---|
| `CanAttackMove` | `no` | Opt a suitable ground combat type into the command. |
| `AttackMoveScanInterval` | `15` | Frames between target scans; nonpositive values use 15. |
| `AttackMove.AcquireRangePercent` | inherited | Override acquisition percentage for this type. |
| `AttackMove.RetainRangePercent` | inherited | Override retention percentage for this type. |
| `AttackMove.SupportEnabled` | inherited | Override healing/repair for this type. |

Range percentages are integers from 100 through 1000. An invalid explicit
global acquisition value produces a diagnostic and resets to 150; invalid
global retention resets to 300. An invalid type value produces a diagnostic
and resets that override to inheritance, even if the type had a valid value
from an earlier rules layer. Missing keys leave the current value in place,
so a map overlay that omits a type key retains an earlier explicit override.
The effective retention percentage is at least the effective acquisition
percentage. The calculation uses wide integer intermediates for deterministic
boundaries. These radii affect target search and pursuit; native `In_Range`
and weapon legality still decide whether a unit can fire.

The three commented presets are examples, not extra game modes:

- `presets/default.ini`: reference single-order behavior, 150% acquisition and
  300% retention.
- `presets/persistent.ini`: repeat destination clicks, otherwise current ranges.
- `presets/weapon-range.ini`: repeat destination clicks with 100% acquisition
  and 100% retention.

Copy only the desired keys into the relevant Rules.ini `[General]` and type
sections. Keep a separate test copy of the game while tuning. Multiplayer peers
must use identical rules and binaries; fresh saves are required after a
protocol/save marker change.
