"""Compile and exercise the production attack-move configuration helpers."""
from pathlib import Path
import subprocess


def run(tree: Path, out: Path, compiler: str, environment: dict) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    source = out / "config.cpp"
    source.write_text(r'''
#include "attackmove_config.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <climits>
#include <cstring>
#include <new>
#include <string>
#include <type_traits>
using namespace AttackMoveConfig;

struct Ini {
    bool present=false;
    std::string value;
    bool Is_Present(const char*, const char*) const { return present; }
    int Get_String(const char*, const char*, const char*, char* out, int size) const {
        const int count=std::min(static_cast<int>(value.size()),size-1);
        std::memcpy(out,value.data(),count);out[count]=0;
        return count;
    }
};

int main() {
    static_assert(std::is_trivially_default_constructible_v<Settings>);
    static_assert(std::is_trivially_copyable_v<Settings>);
    static_assert(std::is_trivially_default_constructible_v<TypeOverrides>);
    static_assert(std::is_trivially_copyable_v<TypeOverrides>);
    Settings defaults = Defaults();
    assert(!defaults.PersistentTargeting && defaults.RightClickDeselects);
    assert(defaults.AcquireRangePercent == 150 && defaults.RetainRangePercent == 300);
    assert(defaults.TargetArmedBuildings && defaults.TargetArmedWildlife && defaults.SupportEnabled);

    assert(!Valid_Percent(99) && Valid_Percent(100) && Valid_Percent(1000) && !Valid_Percent(1001));
    assert(!Valid_Percent(-1));
    assert(Resolve_Percent(-1, 150) == 150 && Resolve_Percent(125, 150) == 125);
    assert(!Resolve_Support(0, true) && Resolve_Support(1, false));
    assert(Resolve_Support(-1, true) && !Resolve_Support(-1, false));
    TypeOverrides inherit = Inherited();
    assert(Resolve_Percent(inherit.AcquireRangePercent, defaults.AcquireRangePercent) == 150);
    assert(Resolve_Percent(inherit.RetainRangePercent, defaults.RetainRangePercent) == 300);
    assert(Resolve_Support(inherit.SupportEnabled, defaults.SupportEnabled));

    // A map overlay changes inherited global values, but explicit type values stay fixed.
    Settings map = defaults;
    map.AcquireRangePercent = 100;
    map.RetainRangePercent = 100;
    map.SupportEnabled = false;
    assert(Resolve_Percent(inherit.AcquireRangePercent, map.AcquireRangePercent) == 100);
    assert(!Resolve_Support(inherit.SupportEnabled, map.SupportEnabled));
    TypeOverrides explicit_type = Inherited();
    explicit_type.AcquireRangePercent = 200;
    explicit_type.RetainRangePercent = 400;
    explicit_type.SupportEnabled = 1;
    assert(Resolve_Percent(explicit_type.AcquireRangePercent, map.AcquireRangePercent) == 200);
    assert(Resolve_Percent(explicit_type.RetainRangePercent, map.RetainRangePercent) == 400);
    assert(Resolve_Support(explicit_type.SupportEnabled, map.SupportEnabled));
    assert(Effective_Retain(200, 100) == 200 && Effective_Retain(100, 300) == 300);

    int inherited_global = 150;
    assert(Apply_Percent(inherited_global, false, 200, 150) && inherited_global == 150);
    assert(Apply_Percent(inherited_global, true, 200, 150) && inherited_global == 200);
    assert(!Apply_Percent(inherited_global, true, 99, 150) && inherited_global == 150);
    int inherited_type = -1;
    assert(Apply_Percent(inherited_type, false, 400, -1) && inherited_type == -1);
    assert(Apply_Percent(inherited_type, true, 400, -1) && inherited_type == 400);
    assert(!Apply_Percent(inherited_type, true, 1001, -1) && inherited_type == -1);
    for (int bad : {INT_MIN, -1, 0, 99, 1001, INT_MAX}) {
        assert(!Apply_Percent(inherited_global, true, bad, defaults.AcquireRangePercent));
        assert(inherited_global == defaults.AcquireRangePercent);
        assert(!Apply_Percent(inherited_type, true, bad, -1) && inherited_type == -1);
    }
    for (int good : {100, 101, 999, 1000}) {
        assert(Apply_Percent(inherited_global, true, good, defaults.AcquireRangePercent));
        assert(inherited_global == good);
    }
    assert(!Apply_Percent_Text(inherited_type, true, "abc", -1) && inherited_type == -1);
    assert(!Apply_Percent_Text(inherited_type, true, "", -1) && inherited_type == -1);
    assert(!Apply_Percent_Text(inherited_type, true, "99", -1) && inherited_type == -1);
    assert(!Apply_Percent_Text(inherited_type, true, "1001", -1) && inherited_type == -1);
    assert(!Apply_Percent_Text(inherited_type, true, "2147483648", -1) && inherited_type == -1);
    assert(Apply_Percent_Text(inherited_type, true, "+100", -1) && inherited_type == 100);
    assert(Apply_Percent_Text(inherited_type, true, "1000", -1) && inherited_type == 1000);
    assert(Apply_Percent_Text(inherited_type, false, nullptr, -1) && inherited_type == 1000);
    Ini ini;
    inherited_global=150;
    assert(Read_Percent(ini,"General","AttackMove.AcquireRangePercent",inherited_global,150));
    assert(inherited_global==150);
    ini.present=true;ini.value=" 100 ";
    assert(Read_Percent(ini,"General","AttackMove.AcquireRangePercent",inherited_global,150));
    assert(inherited_global==100);
    ini.value="not-an-integer";
    assert(!Read_Percent(ini,"General","AttackMove.AcquireRangePercent",inherited_global,150));
    assert(inherited_global==150);
    ini.value=std::string(70,'1');
    assert(!Read_Percent(ini,"General","AttackMove.AcquireRangePercent",inherited_global,150));
    assert(inherited_global==150);
    inherited_type=-1;ini.value="300";
    assert(Read_Percent(ini,"TANK","AttackMove.RetainRangePercent",inherited_type,-1));
    assert(inherited_type==300);
    ini.value="bad";
    assert(!Read_Percent(ini,"TANK","AttackMove.RetainRangePercent",inherited_type,-1));
    assert(inherited_type==-1);

    // NoInit placement reconstruction must leave persisted aggregate bytes intact.
    alignas(Settings) unsigned char settings_bytes[sizeof(Settings)];
    std::memcpy(settings_bytes, &defaults, sizeof(defaults));
    auto* restored = new (settings_bytes) Settings;
    assert(restored->AcquireRangePercent == 150 && restored->SupportEnabled);
    alignas(TypeOverrides) unsigned char type_bytes[sizeof(TypeOverrides)];
    std::memcpy(type_bytes, &explicit_type, sizeof(explicit_type));
    auto* restored_type = new (type_bytes) TypeOverrides;
    assert(restored_type->AcquireRangePercent == 200 && restored_type->SupportEnabled == 1);

    assert(Within_Radius(1536, 0, 1024, 150));
    assert(!Within_Radius(1537, 0, 1024, 150));
    assert(Within_Radius(0, -1024, 1024, 100));
    assert(!Within_Radius(1024, 1, 1024, 100));
    assert(Within_Radius(3000, 4000, 5000, 100));
    assert(!Within_Radius(3000, 4001, 5000, 100));
    assert(Within_Radius(1000000000LL, 0, 100000000, 1000));
    assert(!Within_Radius(1000000001LL, 0, 100000000, 1000));
    assert(!Within_Radius(1, 0, 0, 100));
    assert(!Within_Radius(INT64_MAX, 0, 1024, 100));
    assert(!Within_Radius(INT64_MIN, 0, INT_MAX, 1000));
    assert(Within_Radius(4000000000LL, 0, INT_MAX, 1000));
    assert(!Within_Radius(4000000000LL, 0, 1024, 100));
    assert(Within_Radius(21474836470LL, 0, INT_MAX, 1000));
    assert(!Within_Radius(21474836471LL, 0, INT_MAX, 1000));
    assert(Within_Radius(3000000000LL, 4000000000LL, 1000000000, 500));
    assert(!Within_Radius(3000000000LL, 4000000001LL, 1000000000, 500));

    const unsigned base = Config_CRC(0x12345678u, defaults);
    Settings changed = defaults;
    changed.PersistentTargeting = true;
    assert(Config_CRC(0x12345678u, changed) != base);
    changed = defaults; changed.RightClickDeselects = false;
    assert(Config_CRC(0x12345678u, changed) != base);
    changed = defaults; changed.AcquireRangePercent = 100;
    assert(Config_CRC(0x12345678u, changed) != base);
    changed = defaults; changed.RetainRangePercent = 400;
    assert(Config_CRC(0x12345678u, changed) != base);
    changed = defaults; changed.TargetArmedBuildings = false;
    assert(Config_CRC(0x12345678u, changed) != base);
    changed = defaults; changed.TargetArmedWildlife = false;
    assert(Config_CRC(0x12345678u, changed) != base);
    changed = defaults; changed.SupportEnabled = false;
    assert(Config_CRC(0x12345678u, changed) != base);
    std::puts("PASS configuration helpers");
}
''', encoding="utf-8")
    binary = out / "config.exe"
    built = subprocess.run([compiler, "/nologo", "/EHsc", "/std:c++17", "/UNDEBUG",
                            "/I" + str(tree / "src/extensions/techno"), str(source),
                            "/Fe:" + str(binary), "/Fo:" + str(out / "config.obj")],
                           capture_output=True, text=True, env=environment)
    if built.returncode:
        return {"name": "production configuration helpers", "status": "FAIL",
                "output": built.stdout + built.stderr}
    result = subprocess.run([str(binary)], capture_output=True, text=True, env=environment)
    return {"name": "production configuration helpers",
            "status": "PASS" if result.returncode == 0 else "FAIL",
            "exit_code": result.returncode, "output": result.stdout + result.stderr}
