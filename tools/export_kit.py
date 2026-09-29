"""Generate and verify a focused Attack-Move kit from the canonical project."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT.parent / "_build" / "TSC-AM-configurable" / "AttackMove-Kit.zip"
ARCHIVE_ROOT = "AttackMove"
ZIP_TIME = (1980, 1, 1, 0, 0, 0)
REQUIRED_CORE = {"attackmove.cpp", "attackmove.h", "attackmove_logic.h", "attackmove_config.h"}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def put(archive: zipfile.ZipFile, name: str, data: bytes) -> None:
    info = zipfile.ZipInfo(f"{ARCHIVE_ROOT}/{name}", ZIP_TIME)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)


def source_entries(root: Path) -> dict[str, bytes]:
    core_dir = root / "engine" / "src" / "extensions" / "techno"
    core = sorted(path for path in core_dir.glob("attackmove*")
                  if path.is_file() and path.suffix in {".h", ".cpp"})
    if not REQUIRED_CORE.issubset({path.name for path in core}):
        raise ValueError("Canonical attack-move core is incomplete")
    files = {f"src/extensions/techno/{path.name}": path for path in core}
    files.update({
        "patches/attack-move.patch": root / "patches" / "attack-move.patch",
        "pins.json": root / "pins.json",
        "validation/engine-hooks.json": root / "validation" / "engine-hooks.json",
        "README.md": root / "kit" / "README.md",
        "INTRODUCTION.txt": root / "kit" / "INTRODUCTION.txt",
        "configuration.md": root / "kit" / "configuration.md",
        "docs/integration.md": root / "kit" / "integration.md",
        "presets/default.ini": root / "kit" / "presets" / "default.ini",
        "presets/persistent.ini": root / "kit" / "presets" / "persistent.ini",
        "presets/weapon-range.ini": root / "kit" / "presets" / "weapon-range.ini",
        "LICENSE.md": root / "engine" / "LICENSE.md",
        "CREDITS.md": root / "engine" / "CREDITS.md",
        "licenses/TSpp-LICENSE.md": root / "engine" / "TSpp" / "LICENSE.md",
        "licenses/SDL-LICENSE.txt": root / "engine" / "SDL" / "LICENSE.txt",
        "licenses/imgui-LICENSE.txt": root / "engine" / "imgui" / "LICENSE.txt",
        "licenses/miniaudio-LICENSE": root / "engine" / "miniaudio" / "LICENSE",
        "tools/test.py": root / "tools" / "test.py",
        "tools/verify_hooks.py": root / "tools" / "verify_hooks.py",
    })
    for name in ("attackmove_fakes.h", "test_behavior.py", "test_codec.py", "test_config.py", "test_feedback.py", "test_input.py"):
        files[f"tests/{name}"] = root / "tests" / name
    missing = [name for name, path in files.items() if not path.is_file()]
    if missing:
        raise ValueError(f"Kit inputs missing: {missing}")
    return {name: path.read_bytes() for name, path in sorted(files.items())}


def patch_changed_paths(patch: bytes) -> list[str]:
    paths = re.findall(rb"^diff --git a/(.*?) b/\1$", patch, flags=re.MULTILINE)
    if not paths:
        raise ValueError("Focused integration patch has no changed paths")
    result = [path.decode("utf-8") for path in paths]
    if len(result) != len(set(result)) or any(not (name.startswith("src/") or name == "CMakeLists.txt") for name in result):
        raise ValueError("Patch contains duplicate or unrelated paths")
    return result


def patch_file_kinds(patch: bytes) -> dict[str, str]:
    kinds = {}
    for section in re.split(rb"(?=^diff --git a/)", patch, flags=re.MULTILINE):
        match = re.match(rb"diff --git a/(.*?) b/\1\r?\n", section)
        if match:
            kinds[match.group(1).decode("utf-8")] = "added" if re.search(
                rb"^new file mode ", section, flags=re.MULTILINE) else "modified"
    return kinds


def validate_identity(entries: dict[str, bytes], root: Path) -> dict:
    pins = json.loads(entries["pins.json"])
    hooks = json.loads(entries["validation/engine-hooks.json"])
    expected = pins["vinifera"]["commit"]
    actual = subprocess.check_output(["git", "-C", str(root / "engine"), "rev-parse", "HEAD"], text=True).strip()
    if actual != expected:
        raise ValueError(f"Vinifera checkout is {actual}; expected {expected}")
    if hooks["sha256"].lower() != pins["baseline_game_sha256"].lower():
        raise ValueError("Hook evidence executable hash differs from pins")
    if hooks["event_ids"] != {"PLAYER_OPTIONS": 36, "RESPONSE_TIME2": 37, "ATTACK_MOVE": 38}:
        raise ValueError("Hook evidence event allocation differs from reference port")
    if hooks["protocol_marker"].lower() != pins["protocol"].lower() or hooks["save_marker"].lower() != pins["save_marker"].lower():
        raise ValueError("Hook evidence compatibility markers differ from pins")
    changed = patch_changed_paths(entries["patches/attack-move.patch"])
    kinds = patch_file_kinds(entries["patches/attack-move.patch"])
    modified = {}
    for name in changed:
        path = root / "engine" / name
        if not path.is_file():
            raise ValueError(f"Patch names a missing engine file: {name}")
        modified[name] = {"change": kinds[name], "sha256": digest(path.read_bytes()),
                          "bytes": path.stat().st_size}
    if not REQUIRED_CORE.issubset({Path(name).name for name in changed}):
        raise ValueError("Patch does not add the canonical core files")
    return {
        "format": 2,
        "label": "Attack-Move developer kit, experimental",
        "reference_baseline": pins["vinifera"],
        "dependency_commits": pins["dependencies"],
        "reference_game_sha256": pins["baseline_game_sha256"],
        "reference_vinifera_sha256": pins["baseline_vinifera_sha256"],
        "reference_edb_sha256": pins["baseline_edb_sha256"],
        "reference_event_ids": hooks["event_ids"],
        "reference_protocol": pins["protocol"],
        "reference_save_marker": pins["save_marker"],
        "changed_engine_files": modified,
        "patch_sha256": digest(entries["patches/attack-move.patch"]),
        "files": {name: {"sha256": digest(data), "bytes": len(data)} for name, data in entries.items()},
    }


def assert_no_private_markers(entries: dict[str, bytes], extra: list[str]) -> None:
    try:
        from .package import markers
    except ImportError:
        from package import markers
    found = []
    for name, data in entries.items():
        blob = data.lower()
        for marker in markers(extra):
            variants = (marker, marker.decode("utf-8").encode("utf-16le"), marker.decode("utf-8").encode("utf-16be"))
            if any(value.lower() in blob for value in variants):
                found.append(name)
                break
    if found:
        raise ValueError(f"Private identity markers found in kit: {found}")


def export(out: Path, root: Path = ROOT, deny: list[str] | None = None) -> dict:
    root, out = root.resolve(), out.resolve()
    if out.exists():
        raise FileExistsError(out)
    if root in out.parents or out == root:
        raise ValueError("Kit archive output must be outside the source project")
    # source.py verifies that the focused patch reproduces the entire modified
    # source tree. It does not export or replace the patch in this mode.
    subprocess.run([sys.executable, str(root / "tools" / "source.py")], check=True,
                   stdout=subprocess.DEVNULL)
    entries = source_entries(root)
    manifest = validate_identity(entries, root)
    entries["manifest.json"] = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")
    assert_no_private_markers(entries, deny or [])
    out.parent.mkdir(parents=True, exist_ok=True)
    try:
        with zipfile.ZipFile(out, "w", allowZip64=True) as archive:
            for name, data in sorted(entries.items()):
                put(archive, name, data)
        with zipfile.ZipFile(out) as archive:
            if archive.testzip() is not None:
                raise ValueError("Corrupt kit ZIP")
            if set(archive.namelist()) != {f"{ARCHIVE_ROOT}/{name}" for name in entries}:
                raise ValueError("Kit ZIP inventory differs from canonical inputs")
            for name, data in entries.items():
                if archive.read(f"{ARCHIVE_ROOT}/{name}") != data:
                    raise ValueError(f"Kit ZIP content differs: {name}")
        return {"name": out.name, "sha256": digest(out.read_bytes()),
                "bytes": out.stat().st_size, "files": len(entries),
                "patch_sha256": manifest["files"]["patches/attack-move.patch"]["sha256"]}
    except Exception:
        out.unlink(missing_ok=True)
        raise


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--deny", action="append", default=[], help="Additional private text marker to reject")
    args = parser.parse_args()
    print(json.dumps(export(args.out, deny=args.deny), indent=2))


if __name__ == "__main__":
    main()
