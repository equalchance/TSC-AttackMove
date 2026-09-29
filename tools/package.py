"""Verify a staged game and make reproducible playable and source archives."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import zipfile
from pathlib import Path

try:
    from .stage import PROJECT, DEFAULT_OUT, sha256
except ImportError:
    from stage import PROJECT, DEFAULT_OUT, sha256

DEFAULT_ARCHIVES = PROJECT.parent / "_build" / "TSC-AM-archives"
GAME_ROOT = "TSC-AttackMove"
SOURCE_ROOT = "TSC-AttackMove-source"
SOURCE_SKIP_DIRS = {".git", "__pycache__", ".vs", ".vscode"}
SOURCE_SKIP_ROOT_DIRS = {"build", "debug", "release", "out", "_build", "private", "backups"}
SOURCE_SKIP_SUFFIXES = {".obj", ".pdb", ".ilk", ".ipch", ".log", ".dmp", ".tmp", ".bak", ".pyc"}
SOURCE_SKIP_NAMES = {"cmakecache.txt", "compile_commands.json", "sun.ini", "keyboard.ini", "spawn.ini", "spawnmap.ini", "handover.txt"}
ZIP_TIME = (1980, 1, 1, 0, 0, 0)


def source_files(project: Path) -> list[Path]:
    files = []
    for path in project.rglob("*"):
        if not path.is_file():
            continue
        if path.is_symlink():
            raise ValueError(f"Source contains a symlink: {path}")
        parts = [p.lower() for p in path.relative_to(project).parts]
        if any(part in SOURCE_SKIP_DIRS for part in parts[:-1]):
            continue
        # engine/src/debug is required source, not a generated debug build.
        if len(parts) > 1 and parts[0] in SOURCE_SKIP_ROOT_DIRS:
            continue
        clean_template = parts == ["content", "sun.ini"]
        private_receipt = parts[0] == "validation" and (parts[-1].startswith("native") or parts[-1].startswith("private"))
        backup = ".bak-" in parts[-1] or parts[-1].startswith("backup-")
        if private_receipt or backup or (parts[-1] in SOURCE_SKIP_NAMES and not clean_template) or parts[-1].endswith(tuple(SOURCE_SKIP_SUFFIXES)):
            continue
        files.append(path)
    return sorted(files, key=lambda path: path.relative_to(project).as_posix().lower())


def markers(extra: list[str]) -> list[bytes]:
    # Scan personal identifiers in every byte stream. Generic path examples
    # appear in dependency documentation and do not identify this installation.
    values = ["Kle" + "xile", "Handle=" + "lex" + "ile"]
    user = os.environ.get("USERNAME", "")
    if user and user.lower() not in {"user", "default", "public"}:
        values.append(user)
    values.extend(extra)
    return [value.encode("utf-8") for value in values if value]


def privacy_hits(paths: list[Path], root: Path, extra: list[str]) -> list[str]:
    needles = markers(extra)
    hits = []
    for path in paths:
        variants = [(needle.decode("utf-8"), item.lower()) for needle in needles
                    for item in (needle, needle.decode("utf-8").encode("utf-16le"), needle.decode("utf-8").encode("utf-16be"))]
        overlap = max((len(item) for _, item in variants), default=1) - 1
        tail = b""
        found = set()
        with path.open("rb") as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                data = (tail + chunk).lower()
                found.update(label for label, value in variants if value in data)
                tail = data[-overlap:] if overlap else b""
        hits.extend(f"{path.relative_to(root).as_posix()}: {label}" for label in sorted(found))
    return hits


def verify_stage(stage: Path) -> tuple[list[Path], dict]:
    manifest_path = stage / "manifest.json"
    if not manifest_path.is_file():
        raise ValueError("Stage has no manifest.json; run stage.py first")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    expected = manifest["files"]
    actual = {p.relative_to(stage).as_posix(): p for p in stage.rglob("*") if p.is_file() and p != manifest_path}
    if set(actual) != set(expected):
        raise ValueError(f"Stage inventory changed: missing={sorted(set(expected)-set(actual))}, extra={sorted(set(actual)-set(expected))}")
    for name, path in actual.items():
        if sha256(path) != expected[name]["sha256"] or path.stat().st_size != expected[name]["bytes"]:
            raise ValueError(f"Stage file changed: {name}")
    required = ("game.exe", "Game.edb", "Vinifera.dll", "TiberianSun.exe", "Resources/ClientDefinitions.ini", "INI/Rules.ini", "INI/Firestrm.ini", "Keyboard.ini")
    if any(name not in actual for name in required):
        raise ValueError("Stage lacks a required runtime file")
    if manifest.get("multiplayer") != "unverified" or manifest.get("native_gameplay") != "pending":
        raise ValueError("Unexpected experimental status in stage manifest")
    return [actual[name] for name in sorted(actual)] + [manifest_path], manifest


def write_zip(path: Path, root_name: str, root: Path, files: list[Path],
              extra: dict[str, bytes] | None = None) -> dict:
    if path.exists():
        raise FileExistsError(path)
    extra = extra or {}
    if set(extra) & {file.relative_to(root).as_posix() for file in files}:
        raise ValueError("Generated archive member collides with a source file")
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as archive:
        for file in files:
            relative = file.relative_to(root).as_posix()
            info = zipfile.ZipInfo(f"{root_name}/{relative}", ZIP_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            with file.open("rb") as source, archive.open(info, "w", force_zip64=True) as target:
                shutil.copyfileobj(source, target, 1024 * 1024)
        for relative, data in sorted(extra.items()):
            info = zipfile.ZipInfo(f"{root_name}/{relative}", ZIP_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError(f"Corrupt ZIP: {path}")
        expected = {f"{root_name}/{file.relative_to(root).as_posix()}": sha256(file) for file in files}
        expected.update({f"{root_name}/{name}": hashlib.sha256(data).hexdigest()
                         for name, data in extra.items()})
        if set(archive.namelist()) != set(expected):
            raise ValueError(f"ZIP inventory differs from source: {path}")
        for name, digest in expected.items():
            calculated = hashlib.sha256()
            with archive.open(name) as source:
                for chunk in iter(lambda: source.read(1024 * 1024), b""):
                    calculated.update(chunk)
            if calculated.hexdigest() != digest:
                raise ValueError(f"ZIP content differs: {name}")
    return {"name": path.name, "sha256": sha256(path), "bytes": path.stat().st_size,
            "files": len(files) + len(extra)}


def receipt_status(path: Path | None, dll_hash: str) -> dict:
    if path is None:
        return {"native_gameplay": "pending", "multiplayer": "unverified"}
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("dll_sha256", "").lower() != dll_hash.lower():
        raise ValueError("Native receipt DLL hash does not match the staged Vinifera.dll")
    native = str(data.get("native_gameplay", "")).lower()
    multiplayer = str(data.get("multiplayer", "unverified")).lower()
    if native not in {"pass", "partial", "pending"} or multiplayer not in {"pass", "partial", "unverified"}:
        raise ValueError("Native receipt has an unknown status")
    checks = data.get("checks", {})
    if not isinstance(checks, dict) or any(str(value).lower() not in {"pass", "fail", "pending", "skipped"} for value in checks.values()):
        raise ValueError("Native receipt checks must have explicit statuses")
    return {"native_gameplay": native, "multiplayer": multiplayer, "checks": checks,
            "dll_sha256": dll_hash, "receipt_sha256": sha256(path)}


def package(stage: Path, project: Path, out: Path, deny: list[str] | None = None, receipt: Path | None = None) -> dict:
    stage, project, out = stage.resolve(), project.resolve(), out.resolve()
    if out == stage or stage in out.parents or out in stage.parents or out == project or project in out.parents:
        raise ValueError("Archive output must be separate from the stage and source project")
    game_files, manifest = verify_stage(stage)
    status = receipt_status(receipt.resolve() if receipt else None, sha256(stage / "Vinifera.dll"))
    src_files = source_files(project)
    expected_source = ["engine/LICENSE.md", "engine/CREDITS.md", "engine/CMakeLists.txt", "tools/stage.py", "tools/package.py", "tools/export_kit.py", "tools/verify_hooks.py", "kit/README.md", "engine/src/extensions/techno/attackmove_config.h", "content/SUN.ini"]
    included_source = {path.relative_to(project).as_posix() for path in src_files}
    missing_source = [name for name in expected_source if name not in included_source]
    if missing_source:
        raise ValueError(f"Source project is incomplete: {missing_source}")
    if len(src_files) < 100:
        raise ValueError("Source archive unexpectedly small")
    findings = privacy_hits(game_files, stage, deny or []) + privacy_hits(src_files, project, deny or [])
    if findings:
        raise ValueError("Private markers found:\n" + "\n".join(findings[:50]))
    if out.exists() and any(out.iterdir()):
        raise FileExistsError(f"Archive output is not empty: {out}")
    out.mkdir(parents=True, exist_ok=True)
    try:
        game_zip = write_zip(out / "TSC-AttackMove-playable.zip", GAME_ROOT, stage, game_files)
        source_manifest = {
            "format": 1,
            "files": {file.relative_to(project).as_posix(): {
                "sha256": sha256(file), "bytes": file.stat().st_size
            } for file in src_files},
        }
        source_manifest_bytes = (json.dumps(source_manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")
        source_zip = write_zip(out / "TSC-AttackMove-SOURCE.zip", SOURCE_ROOT, project,
                               src_files, {"source-manifest.json": source_manifest_bytes})
        receipt = {
            "format": 1, "label": "TSC Attack Move experimental",
            **status,
            "changed_from_installed_game": manifest["changed_from_game"],
            "audit": manifest["audit"], "archives": [game_zip, source_zip],
            "privacy_scan": "ASCII, UTF-16LE, UTF-16BE; all game and source files",
        }
        (out / "package-receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return receipt
    except Exception:
        for path in out.iterdir():
            if path.is_file():
                path.unlink()
        raise


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--project", type=Path, default=PROJECT)
    parser.add_argument("--out", type=Path, default=DEFAULT_ARCHIVES)
    parser.add_argument("--deny", action="append", default=[], help="Additional private text marker to reject")
    parser.add_argument("--receipt", type=Path, help="Optional native test receipt bound to staged DLL SHA256")
    args = parser.parse_args()
    print(json.dumps(package(args.stage, args.project, args.out, args.deny, args.receipt), indent=2))


if __name__ == "__main__":
    main()
