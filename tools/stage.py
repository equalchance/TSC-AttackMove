"""Build an isolated, playable Tiberian Sun Client attack-move candidate."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_GAME = PROJECT.parent / "Tiberian Sun Client"
DEFAULT_OUT = PROJECT.parent / "_build" / "TSC-AM"

# These directories are produced by play, downloads, or private test sessions.
SKIP_DIRS = {"client", "debug", "saved games", "screenshots", "custom", "__pycache__", ".git"}
SKIP_NAMES = {
    "mpstats.txt", "syringe.log", "game.pdb", "vinifera.pdb", "dscore.dat", "friend_list",
    "ignore_list", "recent_list", "tunnel_cache", "custom_map_cache_v5",
    "updaterlog.txt",
}
SKIP_SUFFIXES = {".pdb", ".log", ".sav", ".dmp", ".tmp", ".bak"}
# The engine filters these roles itself; avoid giving them misleading INI opt-ins.
EXCLUDED_TYPES = {
    "GHUNTER", "NHUNTER", "HARV", "HORV", "WEED", "ENGINEER", "MHIJACK",
    "JUMPJET", "CHAMSPY", "MCV", "MOBWARG", "MOBWARN", "LIMPET", "ICBM",
}
SPECIAL_FLAGS = {"harvester", "engineer", "jumpjet", "isspawned", "ishunterseeker", "ismechanic", "isomnihealer"}
SECTION = re.compile(r"^\s*\[([^\]]+)\]")
KEY = re.compile(r"^\s*([^;\s=]+)\s*=\s*([^;]*?)(?:\s*;.*)?$")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sections(text: str) -> dict[str, dict[str, str]]:
    result: dict[str, dict[str, str]] = {}
    current: str | None = None
    for line in text.splitlines():
        match = SECTION.match(line)
        if match:
            current = match.group(1).upper()
            result.setdefault(current, {})
        elif current is not None:
            match = KEY.match(line)
            if match:
                result[current][match.group(1).lower()] = match.group(2).strip()
    return result


def read_ini(path: Path) -> str:
    return path.read_bytes().decode("cp1252", errors="surrogateescape")


def write_ini(path: Path, text: str) -> None:
    path.write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("cp1252", errors="surrogateescape"))


def patch_section(text: str, name: str, values: dict[str, str]) -> str:
    """Replace keys in one section, preserving every other key and comment."""
    lines = text.splitlines()
    start = next((i for i, line in enumerate(lines) if (m := SECTION.match(line)) and m.group(1).upper() == name.upper()), None)
    if start is None:
        lines.extend(["", f"[{name}]"])
        start = len(lines) - 1
    end = next((i for i in range(start + 1, len(lines)) if SECTION.match(lines[i])), len(lines))
    for key, value in values.items():
        positions = [i for i in range(start + 1, end) if (m := KEY.match(lines[i])) and m.group(1).lower() == key.lower()]
        if positions:
            lines[positions[0]] = f"{key}={value}"
            for i in reversed(positions[1:]):
                del lines[i]
                end -= 1
        else:
            lines.insert(end, f"{key}={value}")
            end += 1
    return "\n".join(lines) + "\n"


def included(relative: Path) -> bool:
    parts = [part.lower() for part in relative.parts]
    if any(part in SKIP_DIRS for part in parts[:-1]):
        return False
    name = parts[-1]
    if len(parts) == 1 and name in {"sun.ini", "keyboard.ini", "spawn.ini", "spawnmap.ini"}:
        return False
    if name in SKIP_NAMES or name.endswith(tuple(SKIP_SUFFIXES)):
        return False
    if name.startswith("client_") and name.endswith(".log"):
        return False
    return True


def copy_game(game: Path, out: Path) -> None:
    if not (game / "game.exe").is_file() or not (game / "Game.edb").is_file():
        raise ValueError("Game source lacks game.exe or load-bearing Game.edb")
    if out.exists():
        raise FileExistsError(f"Output already exists: {out}")
    if out == game or game in out.parents or out in game.parents:
        raise ValueError("Output and game source must be separate directories")
    for source in game.rglob("*"):
        if not source.is_file():
            continue
        relative = source.relative_to(game)
        if not included(relative):
            continue
        target = out / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def weapon_is_combat(name: str, rules: dict[str, dict[str, str]], seen: frozenset[str] = frozenset()) -> bool:
    if not name or name.lower() == "none":
        return False
    if name.upper() in seen:
        return False
    weapon = rules.get(name.upper(), {})
    try:
        damage = int(weapon.get("damage", "0"))
    except ValueError:
        return False
    if damage > 0:
        return True
    if damage < 0:
        return False
    def truth(key: str, section: dict[str, str]) -> bool:
        return section.get(key, "no").lower() in {"yes", "true", "1"}
    try:
        ambient = int(weapon.get("ambientdamage", "0"))
    except ValueError:
        ambient = 0
    if ambient > 0 and weapon.get("attachedparticlesystem") and any(truth(flag, weapon) for flag in ("israilgun", "usefireparticles", "usesparkparticles")):
        return True
    projectile = rules.get(weapon.get("projectile", "").upper(), {})
    if projectile and (truth("airburst", projectile) or truth("splits", projectile)):
        child = projectile.get("airburstweapon", "")
        return weapon_is_combat(child, rules, seen | {name.upper()})
    return False


def eligible(name: str, data: dict[str, str], rules: dict[str, dict[str, str]]) -> bool:
    if name in EXCLUDED_TYPES:
        return False
    if any(data.get(flag, "no").lower() in {"yes", "true", "1"} for flag in SPECIAL_FLAGS):
        return False
    try:
        if int(data.get("speed", "0").split()[0]) <= 0:
            return False
    except (ValueError, IndexError):
        return False
    return any(weapon_is_combat(data.get(slot, ""), rules) for slot in ("primary", "secondary"))


def all_type_names(data: dict[str, dict[str, str]]) -> set[str]:
    names: set[str] = set()
    for section in ("VEHICLETYPES", "INFANTRYTYPES"):
        names.update(value.split(";", 1)[0].strip().upper() for value in data.get(section, {}).values())
    return {name for name in names if name}


def apply_rules(stage: Path) -> dict:
    rules_path = stage / "INI" / "Rules.ini"
    text = read_ini(rules_path)
    rules = sections(text)
    names = all_type_names(rules)
    enabled: set[str] = set()
    for name in sorted(names):
        data = rules.get(name, {})
        if eligible(name, data, rules):
            text = patch_section(text, name, {"CanAttackMove": "yes", "AttackMoveScanInterval": "15"})
            enabled.add(name)
    write_ini(rules_path, text)
    return {"enabled_rules_types": sorted(enabled), "rules_type_count": len(names)}


def audit_and_patch_maps(stage: Path) -> dict:
    base = sections(read_ini(stage / "INI" / "Rules.ini"))
    base_names = all_type_names(base)
    fs_path = stage / "INI" / "Firestrm.ini"
    fs_overlay = sections(read_ini(fs_path))
    fs_base = {section: dict(values) for section, values in base.items()}
    for section, values in fs_overlay.items():
        fs_base.setdefault(section, {}).update(values)
    scanned: list[str] = []
    changed: dict[str, list[str]] = {}
    unresolved: dict[str, list[str]] = {}
    candidates = [fs_path]
    candidates += list((stage / "Maps").rglob("*.map")) + list((stage / "Maps").rglob("*.mpr"))
    candidates += list(stage.glob("*.map")) + list(stage.glob("*.mpr"))
    candidates += list((stage / "INI" / "Game Options").glob("*.ini"))
    candidates += list((stage / "INI" / "Map Code").glob("*.ini"))
    for path in sorted(set(candidates)):
        relative = path.relative_to(stage).as_posix()
        text = read_ini(path)
        overlay = sections(text)
        scanned.append(relative)
        # A map may replace a numbered type list. Include both its entries and
        # section overrides, then examine only the definitions present in it.
        names = base_names | all_type_names(fs_base) | all_type_names(overlay)
        if not names:
            continue
        # Options and maps can run under TS or Firestorm. Audit both effective
        # configurations so a weapon-only override cannot escape the roster.
        contexts = []
        for inherited in ([base] if path == fs_path else [base, fs_base]):
            merged = {section: dict(values) for section, values in inherited.items()}
            for section, values in overlay.items():
                merged.setdefault(section, {}).update(values)
            contexts.append(merged)
        fixes: list[str] = []
        unknown: list[str] = []
        for name in sorted(names):
            armed_contexts = [merged for merged in contexts if eligible(name, merged.get(name, {}), merged)]
            if armed_contexts:
                if any(merged[name].get("canattackmove", "no").lower() != "yes" or merged[name].get("attackmovescaninterval", "15") != "15" for merged in armed_contexts):
                    text = patch_section(text, name, {"CanAttackMove": "yes", "AttackMoveScanInterval": "15"})
                    fixes.append(name)
            elif name in overlay and name not in base_names and any(context.get(name, {}).get(slot, "").lower() not in {"", "none"} for context in contexts for slot in ("primary", "secondary")):
                unknown.append(name)
        if fixes:
            write_ini(path, text)
            changed[relative] = fixes
        if unknown:
            unresolved[relative] = unknown
    return {"maps_scanned": len(scanned), "map_overrides_enabled": changed, "map_types_for_review": unresolved}


def install_hotkeys(stage: Path) -> None:
    template = stage / "INI" / "Keyboard.ini"
    text = read_ini(template)
    bindings = sections(text).get("HOTKEY", {})
    collisions = [name for name, key in bindings.items() if name != "attackmove" and key == "65"]
    if collisions or bindings.get("attackmove", "65") != "65":
        raise ValueError(f"Clean keyboard template conflicts with A=65: {collisions or ['AttackMove']}")
    text = patch_section(text, "Hotkey", {"AttackMove": "65"})
    write_ini(template, text)
    write_ini(stage / "Keyboard.ini", text)
    commands = stage / "Resources" / "KeyboardCommands.ini"
    command_text = read_ini(commands)
    if "[AttackMove]" not in command_text:
        command_text += "\n[AttackMove]\nUIName=Attack Move\nCategory=Control\nDescription=Move toward a destination and engage enemies along the way.\nDefaultKey=65\t;A\n"
    write_ini(commands, command_text)


def disable_updater(stage: Path) -> None:
    definitions = stage / "Resources" / "ClientDefinitions.ini"
    text = patch_section(read_ini(definitions), "Settings", {"ModMode": "true"})
    write_ini(definitions, text)


def clean_editor_settings(stage: Path) -> None:
    settings = stage / "Map Editor/FinalSun/FinalSun.ini"
    if settings.exists():
        # Installed editor preferences can contain an absolute game path.
        # FinalSun resides two directories below the portable game root.
        write_ini(settings, "[FinalSun]\nLanguage=English\nFileSearchLikeTS=yes\n\n[TS]\nExe=..\\..\\\n")


def manifest(stage: Path, audit: dict, baseline: dict[str, str]) -> None:
    files = {}
    changed = []
    for path in sorted(p for p in stage.rglob("*") if p.is_file()):
        relative = path.relative_to(stage).as_posix()
        if relative == "manifest.json":
            continue
        digest = sha256(path)
        files[relative] = {"sha256": digest, "bytes": path.stat().st_size}
        if baseline.get(relative) != digest:
            changed.append(relative)
    (stage / "manifest.json").write_text(json.dumps({
        "format": 1, "label": "TSC Attack Move experimental", "native_gameplay": "pending",
        "multiplayer": "unverified", "files": files, "changed_from_game": changed,
        "audit": audit,
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def stage(game: Path, dll: Path, out: Path) -> dict:
    game, dll, out = game.resolve(), dll.resolve(), out.resolve()
    if not dll.is_file():
        raise FileNotFoundError(dll)
    baseline = {p.relative_to(game).as_posix(): sha256(p) for p in game.rglob("*") if p.is_file() and included(p.relative_to(game))}
    copy_game(game, out)
    try:
        shutil.copy2(dll, out / "Vinifera.dll")
        # Ship portable defaults; the installed SUN.ini contains player identity
        # and display settings and is deliberately never copied.
        shutil.copy2(PROJECT / "content" / "SUN.ini", out / "SUN.ini")
        shutil.copy2(PROJECT / "content" / "README - Attack Move.txt", out / "README - Attack Move.txt")
        install_hotkeys(out)
        disable_updater(out)
        clean_editor_settings(out)
        audit = apply_rules(out)
        audit.update(audit_and_patch_maps(out))
        required = ("game.exe", "Game.edb", "TiberianSun.exe", "LaunchVinifera.dat", "Resources/ClientDefinitions.ini", "INI/Rules.ini", "INI/Firestrm.ini", "INI/Keyboard.ini", "Keyboard.ini", "SUN.ini", "README - Attack Move.txt", "Vinifera.dll")
        missing = [name for name in required if not (out / name).is_file()]
        if missing:
            raise ValueError(f"Missing required game files: {missing}")
        manifest(out, audit, baseline)
        return audit
    except Exception:
        shutil.rmtree(out)
        raise


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, default=DEFAULT_GAME)
    parser.add_argument("--dll", type=Path, required=True, help="Validated sanitized Vinifera.dll")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    print(json.dumps(stage(args.game, args.dll, args.out), indent=2))


if __name__ == "__main__":
    main()
