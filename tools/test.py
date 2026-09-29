"""Build isolated attack-move checks and write a machine-readable receipt."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
import test_behavior
import test_feedback
import test_codec
import test_config
import test_input


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def toolchain() -> tuple[str, dict[str, str]]:
    """Find installed MSVC and its x86 environment without a fixed VS edition/path."""
    program_files = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
    vswhere = program_files / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        raise RuntimeError("Visual Studio vswhere.exe was not found")
    found = subprocess.run([str(vswhere), "-latest", "-products", "*", "-requires",
                            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property",
                            "installationPath"], capture_output=True, text=True, check=True).stdout.strip()
    if not found:
        raise RuntimeError("Visual Studio C++ x86 tools were not found")
    vcvars = Path(found) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    if not vcvars.is_file():
        raise RuntimeError(f"Missing {vcvars}")
    # vcvarsall is a batch file. Capture its environment in a child cmd process.
    with tempfile.TemporaryDirectory(prefix="tsc-am-msvc-") as temporary:
        batch = Path(temporary) / "environment.cmd"
        batch.write_text(f'@echo off\r\ncall "{vcvars}" x86 >nul\r\nset\r\n', encoding="ascii")
        initialized = subprocess.run(["cmd.exe", "/d", "/c", str(batch)],
                                     capture_output=True, text=True)
    if initialized.returncode:
        raise RuntimeError(initialized.stdout + initialized.stderr)
    environment = os.environ.copy()
    for line in initialized.stdout.splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            environment[key] = value
    tools_dir = next((value for key, value in environment.items()
                      if key.upper() == "VCTOOLSINSTALLDIR"), None)
    if tools_dir is None:
        raise RuntimeError("vcvarsall completed without VCTOOLSINSTALLDIR")
    compiler_dir = Path(tools_dir) / "bin" / "Hostx64" / "x86"
    compiler = compiler_dir / "cl.exe"
    if not compiler.is_file():
        raise RuntimeError(f"Missing {compiler}")
    sdk_dir = next((value for key, value in environment.items()
                    if key.upper() == "WINDOWSSDKDIR"), None)
    sdk_version = next((value for key, value in environment.items()
                        if key.upper() == "WINDOWSSDKVERSION"), None)
    prepend = [str(compiler_dir)]
    if sdk_dir and sdk_version:
        prepend.append(str(Path(sdk_dir) / "bin" / sdk_version.rstrip("\\/") / "x86"))
    environment["PATH"] = os.pathsep.join(prepend) + os.pathsep + environment.get("PATH", "")
    return str(compiler), environment


def source_contract(engine: Path) -> list[dict]:
    """Check integration surfaces which isolated decision fixtures cannot execute."""
    src = engine / "src"
    def read(relative: str) -> str:
        return (src / relative).read_text(encoding="utf-8", errors="replace")
    event_h = read("extensions/event/eventext.h")
    event_cpp = read("extensions/event/eventext.cpp")
    hooks = read("extensions/event/eventext_hooks.cpp")
    techno_h = read("extensions/techno/technoext.h")
    techno_cpp = read("extensions/techno/technoext.cpp")
    attack_cpp = read("extensions/techno/attackmove.cpp")
    logic = read("extensions/techno/attackmove_logic.h")
    checks = {
        "protocol marker": "0x54534103" in logic,
        "save marker": "0x54535303" in read("extensions/extension.cpp"),
        "attack-move payload": bool(re.search(r"struct\s*\{\s*xTargetClass\s+Whom;\s*xCell\s+Where;\s*\}\s*AttackMove;", event_h)),
        "event length uses payload size": "sizeof(EventClassExt::Data.AttackMove)" in event_cpp,
        "event dispatches attack-move": "AttackMove::Execute(*this)" in event_cpp,
        "replacement order hook": "AttackMove::Cancel_Replaced_Order(*event)" in hooks,
        "startup handshake hook": "AttackMoveLogic::Handshake(ScenarioCRC)" in hooks,
        "simulation CRC hook": "AttackMove::Simulation_CRC" in hooks,
        "save state fields": all(field in techno_h and field in techno_cpp for field in
                                 ["IsAttackMoving", "AttackMoveDestination", "ThreatScanCadenceTimer",
                                  "AttackMoveCombatPending", "AttackMoveRetries", "AttackMoveRetryTimer"]),
        "state CRC function": "AttackMoveLogic::State_CRC" in attack_cpp,
        "codec compressed add": "_Add_Compressed_Events" in hooks,
        "codec compressed extract": "_Extract_Compressed_Events" in hooks,
        "codec uncompressed extract": "Extract_Uncompressed_Events" in hooks,
    }
    defines = read("vinifera/vinifera_defines.h")
    ordered = re.search(r"EXT_EVENT_PAD\s*=\s*EVENT_LATENCYFUDGE.*?EXT_EVENT_PLAYER_OPTIONS\s*,\s*EXT_EVENT_RESPONSE_TIME2\s*,\s*EXT_EVENT_ATTACK_MOVE\s*,", defines, re.S)
    native_defines = (engine / "TSpp/src/tibsun/tibsun_defines.h").read_text(encoding="utf-8", errors="replace")
    native_events = re.search(r"typedef enum EventType.*?EVENT_EMPTY\s*,(.*?)EVENT_LATENCYFUDGE\s*,", native_defines, re.S)
    # EVENT_EMPTY is zero; count each following native event before LATENCYFUDGE.
    native_count = len(re.findall(r"^\s*EVENT_[A-Z_]+\s*,", native_events.group(1), re.M)) if native_events else -1
    checks["attack-move event ID 38"] = bool(ordered and native_count + 4 == 38)
    rules_h = read("extensions/rules/rulesext.h")
    rules_cpp = read("extensions/rules/rulesext.cpp")
    type_h = read("extensions/technotype/technotypeext.h")
    type_cpp = read("extensions/technotype/technotypeext.cpp")
    checks["global settings persisted in rules extension"] = (
        "AttackMoveConfig::Settings AttackMoveSettings" in rules_h and
        "AttackMoveConfig::Defaults()" in rules_cpp and
        all(f"crc(AttackMoveSettings.{name})" in rules_cpp for name in (
            "PersistentTargeting", "RightClickDeselects", "AcquireRangePercent",
            "RetainRangePercent", "TargetArmedBuildings", "TargetArmedWildlife",
            "SupportEnabled")))
    checks["per-type overrides persisted and checksummed"] = all(
        name in type_h and f"crc({name})" in type_cpp for name in (
            "AttackMoveAcquireRangePercent", "AttackMoveRetainRangePercent",
            "AttackMoveSupportEnabled"))
    checks["global and type INI overlays"] = all(
        f'"AttackMove.{name}"' in rules_cpp for name in (
            "PersistentTargeting", "RightClickDeselects", "AcquireRangePercent",
            "RetainRangePercent", "TargetArmedBuildings", "TargetArmedWildlife",
            "SupportEnabled")) and all(f'"AttackMove.{name}"' in type_cpp for name in (
                "AcquireRangePercent", "RetainRangePercent", "SupportEnabled"))
    checks["literal percent parsing and fresh scenario reset"] = (
        rules_cpp.count("AttackMoveConfig::Read_Percent(ini, GENERAL,") == 2 and
        type_cpp.count("AttackMoveConfig::Read_Percent(ini, ini_name,") == 2 and
        "AttackMoveSettings = AttackMoveConfig::Defaults()" in rules_cpp and
        "AttackMove::Cancel_Input()" in read("extensions/scenario/scenarioext.cpp"))
    checks["configuration connected to simulation"] = (
        "AttackMoveSettings" in attack_cpp and "AttackMoveAcquireRangePercent" in attack_cpp and
        "AttackMoveRetainRangePercent" in attack_cpp and "AttackMoveSupportEnabled" in attack_cpp)
    save_hooks = read("vinifera/vinifera_hooks.cpp")
    save_load = read("vinifera/vinifera_saveload.cpp")
    save_version = read("extensions/extension.cpp")
    checks["current save version rejects previous layouts"] = (
        bool(re.search(r"return\s+version\s*\^\s*0x54535303u", save_version)) and
        "ViniferaGameVersion = Extension::Get_Save_Version_Number()" in save_hooks and
        "saveversion.Get_Vinifera_Version()) != ViniferaGameVersion" in save_load and
        "vinifera_version != ViniferaGameVersion" in save_load)
    return [{"name": name, "status": "PASS" if valid else "FAIL"} for name, valid in checks.items()]


def run_logic(engine: Path, output: Path, compiler: str, environment: dict) -> dict:
    source = output / "logic.cpp"
    source.write_text(r'''
#include "attackmove_logic.h"
#include <cassert>
#include <algorithm>
#include <array>
#include <cstdio>
int main() {
    using namespace AttackMoveLogic;
    static_assert(Protocol == 0x54534103u, "protocol marker");
    for (int bits=0; bits<16; ++bits) {
        bool mobile=bits&1, allied=bits&2, passive=bits&4, civilian=bits&8;
        assert(Enemy_Unit(mobile,allied,passive,civilian)==(bits==1));
    }
    Input s{true,true,false,true,false,false,false,false};
    assert(Decide(s)==Step::Transit);
    s.active=false;assert(Decide(s)==Step::Inactive);s.active=true;
    s.eligible=false;assert(Decide(s)==Step::Cancel);s.eligible=true;
    s.allowed_mission=false;assert(Decide(s)==Step::Cancel);s.allowed_mission=true;
    s.suspended=true;assert(Decide(s)==Step::Suspend);s.suspended=false;
    s.target_valid=true;assert(Decide(s)==Step::Engage);s.target_valid=false;
    s.attack_mission=true;assert(Decide(s)==Step::Resume);
    s.arrived=true;assert(Decide(s)==Step::Finish);
    int deadline=0, scans=0;
    for (unsigned frame=0;frame<=150;++frame) {
        bool due=Scan_Due(deadline,15,frame);
        assert(due==(frame%15==0));scans+=due;
        assert(!Scan_Due(deadline,15,frame));
    }
    assert(scans==11);
    deadline=static_cast<int>(0xfffffff8u);
    assert(Scan_Due(deadline,15,0xfffffff8u));
    assert(!Scan_Due(deadline,15,0xffffffffu));
    assert(Scan_Due(deadline,15,7));
    std::array<unsigned,4> ids={9,4,7,2};
    std::sort(ids.begin(),ids.end());
    do {unsigned best=100;for(auto id:ids)if(Better_Target(10,id,10,best))best=id;assert(best==2);}
    while(std::next_permutation(ids.begin(),ids.end()));
    assert(Queue_Fits(63,1,64)&&!Queue_Fits(64,1,64));
    assert(!Queue_Fits(0,0,64)&&!Queue_Fits(-1,1,64)&&!Queue_Fits(0,65,64));
    for(unsigned crc : {0u,1u,0x12345678u,0xffffffffu})
        assert(Handshake(crc)==(crc^Protocol));
    auto baseline=State_CRC(123,true,10,20,30,45,0,2,90);
    assert(baseline!=State_CRC(123,false,10,20,30,45,0,2,90));
    assert(baseline!=State_CRC(123,true,11,20,30,45,0,2,90));
    assert(baseline!=State_CRC(123,true,10,21,30,45,0,2,90));
    assert(baseline!=State_CRC(123,true,10,20,31,45,0,2,90));
    assert(baseline!=State_CRC(123,true,10,20,30,46,0,2,90));
    assert(baseline!=State_CRC(123,true,10,20,30,45,1,2,90));
    assert(baseline!=State_CRC(123,true,10,20,30,45,0,3,90));
    assert(baseline!=State_CRC(123,true,10,20,30,45,0,2,91));
    std::puts("PASS logic");
}
''', encoding="utf-8")
    binary = output / "logic.exe"
    build = subprocess.run([compiler, "/nologo", "/EHsc", "/std:c++17", "/UNDEBUG",
                            "/I" + str(engine / "src/extensions/techno"), str(source),
                            "/Fe:" + str(binary), "/Fo:" + str(output / "logic.obj")],
                           capture_output=True, text=True, env=environment)
    if build.returncode:
        return {"name": "production logic", "status": "FAIL", "output": build.stdout + build.stderr}
    result = subprocess.run([str(binary)], capture_output=True, text=True, env=environment)
    return {"name": "production logic", "status": "PASS" if result.returncode == 0 else "FAIL",
            "exit_code": result.returncode, "output": result.stdout + result.stderr}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=ROOT / "engine")
    parser.add_argument("--out", type=Path, default=ROOT / "build/tests")
    parser.add_argument("--compile-commands", type=Path, default=ROOT / "build/compile_commands.json",
                        help="CMake compile database for the engine build")
    args = parser.parse_args()
    engine, output = args.engine.resolve(), args.out.resolve()
    output.mkdir(parents=True, exist_ok=True)
    receipt = {"schema": 1, "time_utc": datetime.now(timezone.utc).isoformat(),
               "engine": str(engine), "output": str(output), "checks": [],
               "scope": "Extracted production bodies and source integration checks with engine doubles; native game, save/load, and network play require manual acceptance."}
    try:
        compiler, environment = toolchain()
        receipt["compiler"] = compiler
        receipt["checks"].append(test_behavior.extraction_check())
        receipt["checks"].extend(source_contract(engine))
        receipt["checks"].append(run_logic(engine, output, compiler, environment))
        receipt["checks"].append(test_config.run(engine, output / "config", compiler, environment))
        receipt["checks"].append(test_feedback.run(engine, output / "feedback", compiler, environment))
        receipt["checks"].append(test_input.run(engine, output / "input", compiler, environment))
        receipt["checks"].append(test_codec.run(engine, output / "codec", compiler, environment,
                                                 args.compile_commands.resolve()))
        for result in test_behavior.run(engine, output / "behavior", compiler, environment):
            receipt["checks"].append({"name": "behavior: " + result["case"],
                                      "status": "PASS" if result["exit_code"] == 0 else "FAIL", **result})
        surfaces = ["extensions/techno/attackmove.cpp", "extensions/techno/attackmove_logic.h",
                    "extensions/techno/attackmove_config.h", "extensions/rules/rulesext.cpp",
                    "extensions/rules/rulesext.h", "extensions/technotype/technotypeext.cpp",
                    "extensions/technotype/technotypeext.h",
                    "extensions/techno/technoext.cpp", "extensions/techno/technoext.h",
                    "extensions/event/eventext.cpp", "extensions/event/eventext.h",
                    "extensions/event/eventext_hooks.cpp", "extensions/extension.cpp",
                    "extensions/display/displayext_hooks.cpp", "new/beacon/beacon_hooks.cpp",
                    "vinifera/vinifera_defines.h"]
        receipt["source_sha256"] = {name: digest(engine / "src" / name) for name in surfaces}
        receipt["fixture_sha256"] = {p.relative_to(output).as_posix(): digest(p)
                                     for p in sorted(output.rglob("*.cpp"))}
    except Exception as error:
        receipt["checks"].append({"name": "runner", "status": "FAIL", "error": str(error)})
    receipt["status"] = "PASS" if receipt["checks"] and all(
        c["status"] == "PASS" for c in receipt["checks"]) else "FAIL"
    (output / "test-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    for check in receipt["checks"]:
        print(f'{check["status"]}: {check["name"]}')
        if check["status"] == "FAIL":
            print(check.get("error", check.get("output", "")))
    print(f'Receipt: {output / "test-receipt.json"}')
    return 0 if receipt["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
