"""Copy a staged stock client into a private manual attack-move fixture."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def set_values(text: str, section: str, values: dict[str, str]) -> str:
    pattern = re.compile(r"(?ms)^\[" + re.escape(section) + r"\]\r?\n(.*?)(?=^\[|\Z)")
    match = pattern.search(text)
    body = match.group(1) if match else ""
    for key, value in values.items():
        body = re.sub(r"(?m)^" + re.escape(key) + r"=.*(?:\r?\n|$)", "", body)
        body = body.rstrip() + f"\n{key}={value}\n"
    replacement = f"[{section}]\n{body}\n"
    return text[:match.start()] + replacement + text[match.end():] if match else text.rstrip() + "\n\n" + replacement


def fixture_files(out: Path, stock_map: Path, candidate_hash: str) -> dict:
    map_text = stock_map.read_text(encoding="cp1252")
    map_text = set_values(map_text, "Basic", {"Name": "Attack Move Private Test",
        "Player": "GDI", "HomeCell": "98", "AltHomeCell": "98", "MultiplayerOnly": "0",
        "SkipScore": "yes", "EndOfGame": "no"})
    # Campaign ownership must exist before the engine reads placed objects.
    map_text = set_values(map_text, "Houses", {"0": "GDI", "1": "Nod", "2": "Neutral", "3": "Special"})
    map_text = set_values(map_text, "GDI", {"PlayerControl": "yes", "IQ": "0", "Allies": "GDI"})
    map_text = set_values(map_text, "Nod", {"PlayerControl": "no", "IQ": "0", "Allies": "Nod"})
    map_text = set_values(map_text, "Waypoints", {"98": "118035"})
    map_text = set_values(map_text, "Structures", {
        "1041625": "Nod,NALASR,256,45,131,0,None,0,0,1,0,0,None,None,None,0,0"})
    units = [
        "Multi1,REPAIR,256,118,38,0,Sleep,None,0,-1,0,-1,1,0",
        "Multi1,MCV,128,121,39,0,Sleep,None,0,-1,0,-1,1,0",
        "Multi1,MMCH,256,116,38,0,Sleep,None,0,-1,0,-1,1,0",
        "Multi1,FLMTNK,256,119,36,0,Sleep,None,0,-1,0,-1,1,0",
        "Multi1,REAPER,256,120,37,0,Sleep,None,0,-1,0,-1,1,0",
        "Multi1,SUBTANK,256,121,36,0,Sleep,None,0,-1,0,-1,1,0",
        "Multi2,TTNK,256,126,42,0,Guard,None,0,-1,0,-1,1,0"]
    infantry = [
        "Multi1,MEDIC,256,116,35,0,Sleep,0,None,0,-1,0,1,0",
        "Multi1,E1,128,119,35,0,Sleep,0,None,0,-1,0,1,0",
        "Multi1,GHOST,256,117,36,0,Sleep,0,None,0,-1,0,1,0",
        "Multi2,E1,256,124,40,0,Guard,0,None,0,-1,0,1,0",
        "Multi2,E1,256,125,41,0,Guard,0,None,0,-1,0,1,0"]
    def campaign_object(row):
        fields = row.split(',')
        fields[0] = {'Multi1': 'GDI', 'Multi2': 'Nod'}[fields[0]]
        fields[3], fields[4] = fields[4], fields[3]
        return ','.join(fields)
    map_text = set_values(map_text, "Units", {str(i): campaign_object(row) for i, row in enumerate(units)})
    map_text = set_values(map_text, "Infantry", {str(i): campaign_object(row) for i, row in enumerate(infantry)})
    (out / "spawnmap.ini").write_text(map_text, encoding="cp1252")
    spawn = {"Scenario": "spawnmap.ini", "IsSinglePlayer": "Yes", "CampaignID": "-1",
             "GameSpeed": "2", "Firestorm": "False", "SidebarHack": "True", "Side": "0",
             "DifficultyModeHuman": "1", "DifficultyModeComputer": "1"}
    spawn_text = set_values("", "Settings", spawn)
    (out / "spawn.ini").write_text(spawn_text, encoding="ascii")
    sun = out / "SUN.ini"
    sun.write_text(set_values(sun.read_text(encoding="cp1252"), "Video",
                              {"ScreenWidth": "640", "ScreenHeight": "480", "Windowed": "True",
                               "WindowWidth": "1280", "WindowHeight": "960"}), encoding="cp1252")
    shutil.copy2(out / "LaunchVinifera.dat", out / "LaunchVinifera-test.exe")
    launcher = "$ErrorActionPreference = 'Stop'\nSet-Location -LiteralPath $PSScriptRoot\n"
    launcher += f"if ((Get-FileHash -LiteralPath 'Vinifera.dll' -Algorithm SHA256).Hash -ne '{candidate_hash}') {{ throw 'Private fixture DLL differs from prepared candidate.' }}\n"
    launcher += "foreach ($file in @('game.exe','Game.edb','spawn.ini','spawnmap.ini','LaunchVinifera-test.exe')) { if (!(Test-Path -LiteralPath $file)) { throw \"Missing $file\" } }\n"
    launcher += "Start-Process -FilePath (Join-Path $PSScriptRoot 'LaunchVinifera-test.exe') -WorkingDirectory $PSScriptRoot -ArgumentList @('-SPAWN','-CD.','-NO_VERSION_STRING')\n"
    (out / "Launch test.ps1").write_text(launcher, encoding="ascii")
    (out / "Start attack-move test.cmd").write_text(
        '@echo off\n powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Launch test.ps1"\n', encoding="ascii")
    receipt = {"schema": 1, "status": "PREPARED_NATIVE_ACCEPTANCE_PENDING", "candidate_dll_sha256": candidate_hash,
               "stock_map": str(stock_map), "stock_map_sha256": sha(stock_map), "fixture_map_sha256": sha(out / "spawnmap.ini"),
               "launch": str(out / "Start attack-move test.cmd"), "screen": "640x480",
               "scope": "Private stock-map fixture. Preparing files does not establish native launch or gameplay acceptance."}
    (out / "native-fixture.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--preset", type=Path, help="Apply a developer preset to the private map's General section")
    args = parser.parse_args()
    candidate, out = args.candidate.resolve(), args.out.resolve()
    if out.exists():
        raise SystemExit("Output already exists; choose a new private fixture directory.")
    if candidate == out or candidate in out.parents or out in candidate.parents:
        raise SystemExit("Candidate and private fixture must be separate directories.")
    for name in ["Vinifera.dll", "game.exe", "Game.edb", "LaunchVinifera.dat", "SUN.ini", "Maps/Tiberian Sun/terrace.map"]:
        if not (candidate / name).is_file():
            raise SystemExit("Missing candidate file: " + name)
    candidate_hash = sha(candidate / "Vinifera.dll")
    shutil.copytree(candidate, out)
    receipt = fixture_files(out, candidate / "Maps/Tiberian Sun/terrace.map", candidate_hash)
    if args.preset:
        import configparser
        preset = configparser.ConfigParser(interpolation=None, inline_comment_prefixes=(';',))
        preset.optionxform = str
        preset.read_string(args.preset.read_text(encoding='utf-8-sig'))
        values = dict(preset['General'])
        if any(not key.startswith('AttackMove.') for key in values):
            raise ValueError('Native preset may only override AttackMove.* settings')
        map_path = out / 'spawnmap.ini'
        map_path.write_text(set_values(map_path.read_text(encoding='cp1252'), 'General', values), encoding='cp1252')
        receipt['preset_sha256'] = sha(args.preset)
        receipt['fixture_map_sha256'] = sha(map_path)
    receipt['rules_sha256'] = sha(out / 'INI/Rules.ini')
    receipt['firestorm_rules_sha256'] = sha(out / 'INI/Firestrm.ini')
    (out / 'native-fixture.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(receipt["launch"])


if __name__ == "__main__":
    main()
