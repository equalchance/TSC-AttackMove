# Integration points for another Vinifera port

The focused patch is against Vinifera commit
`71a692a5f4a7d61cd1654f2544ee4d17fe473d70`. Its executable hooks were
checked against the `game.exe` SHA-256 in `pins.json` and
`validation/engine-hooks.json`. Hook addresses, overwritten bytes, register
and stack expectations are reference data for that executable only.

For the pinned executable, run
`py -3 tools/verify_hooks.py --game "path/to/game.exe"`. The verifier checks
the file hash, PE layout and recorded byte windows for the reference hook sites.
For another executable, check the overwritten instructions, register values,
stack arguments and continuation addresses at each site. The reference JSON
records the observed bytes and the known input at each site; it is not a
calling-convention proof for a different binary.

The patch connects these paths:

| Path | Integration to compare |
|---|---|
| Command and input | Register AttackMove, arm A-click, classify cursor action, submit destination events, cancel on right-click. |
| Event queue | Allocate event 38 on this TSC baseline; encode/decode its payload and dispatch it. Preserve events 36 and 37. |
| AI and lifecycle | Tick route/target selection; cancel on replacement orders, capture, limbo and invalid state. |
| Extension data | Store route, target and combat recovery state; read per-type INI keys. |
| Save and checksums | Save extension fields, mix effective simulation configuration into checksums, reject incompatible save/protocol markers. |
| Client | Add the command to the keyboard editor; bind A only if free and preserve an existing user's bindings. |

Review the patch alongside your port's equivalent code. Check executable
opcodes before installing each hook and verify event length, payload alignment,
dispatcher range and any register preservation. A port with different existing
events must allocate a free ID. Give each incompatible implementation its own
protocol and save marker. This reference uses event 38, preserving existing
`PLAYER_OPTIONS=36` and `RESPONSE_TIME2=37`.
The current TSC reference uses protocol marker `0x54534103` and save marker
`0x54535303`; change both when the event or saved state is incompatible.

Global Rules.ini keys and optional per-type overrides are documented in
`configuration.md`. A map overlay can change globals while a type with no
explicit override continues to inherit. Save/load and checksum handling must
use the same effective settings as target selection. Reset local armed
targeting on scenario changes and loads.

The core retains native weapon legality, special unit exclusions, payload
weapons, support targeting and combat route recovery. An unarmed ground unit
in a selected group receives an ordinary move order. Test these cases in the
actual executable after adapting hooks; the isolated tests establish source
decisions and codec behavior only.

The patch contains attack-move only. It omits Chilong's separate tiberium
change, TI campaign projectile repair, zoom, selection filters, custom cursor
assets and roster content.
