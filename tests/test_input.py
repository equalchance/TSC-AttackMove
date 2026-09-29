"""Exercise the production click hook using simulated registers and stack."""
from pathlib import Path
import subprocess
from test_behavior import function


def run(tree: Path, out: Path, compiler: str, environment: dict) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    hook = function((tree / "src/extensions/display/displayext_hooks.cpp").read_text(),
                    "DEFINE_HOOK(0x00478BC7,")
    source = out / "input.cpp"
    source.write_text(r'''
#include <cassert>
enum ActionType { ACTION_MOVE=1, ACTION_TOGGLE_SELECT=7 };
struct Cell { short X,Y; } destination{20,30};
ActionType inputAction=ACTION_MOVE;
int offsetSeen=0, clicks=0;
namespace AttackMove {
bool armed=false;
bool Armed(){return armed;}
bool Targeting(){return armed;}
void Click(const Cell& cell){assert(&cell==&destination);++clicks;armed=false;}
}
#define DEFINE_HOOK(address,name,size) unsigned Dispatch()
#define GET(type,name,reg) type name=inputAction
#define GET_STACK(type,name,offset) offsetSeen=offset; type name=&destination
''' + hook + r'''
int main(){
    assert(Dispatch()==0 && clicks==0);
    inputAction=ACTION_TOGGLE_SELECT;assert(Dispatch()==0x00478BD8 && clicks==0);
    inputAction=ACTION_MOVE;AttackMove::armed=true;
    assert(Dispatch()==0x004790D1 && clicks==1 && offsetSeen==0x54 && !AttackMove::armed);
    assert(Dispatch()==0 && clicks==1);
}
''', encoding="utf-8")
    built = subprocess.run([compiler, "/nologo", "/EHsc", "/std:c++17", "/UNDEBUG", str(source),
                            "/Fe:" + str(out / "input.exe"), "/Fo:" + str(out / "input.obj")],
                           capture_output=True, text=True, env=environment)
    if built.returncode:
        return {"name": "production click hook", "status": "FAIL", "output": built.stdout + built.stderr}
    result = subprocess.run([str(out / "input.exe")], capture_output=True, text=True, env=environment)
    return {"name": "production click hook", "status": "PASS" if result.returncode == 0 else "FAIL",
            "exit_code": result.returncode, "output": result.stdout + result.stderr}
