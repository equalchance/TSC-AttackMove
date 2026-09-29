"""Exercise production feedback and right-click branches with engine doubles."""
from pathlib import Path
import re
import subprocess
from test_behavior import function

def run(tree: Path, out: Path, compiler: str, environment: dict) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    source = (tree/'src/extensions/techno/attackmove.cpp').read_text()
    feedback = function(source, 'static void Show_Destination_Feedback(')
    # Real layout assertions are checked by the full production compilation.
    feedback = re.sub(r'^.*static_assert.*\n', '', feedback, flags=re.M)
    right = function((tree/'src/new/beacon/beacon_hooks.cpp').read_text(),
                     'void DisplayClassExt::_Mouse_Right_Release(')
    branch = function(right, 'if (AttackMove::Armed())')
    click = function(source, 'void Click(')
    assert click.count('Show_Destination_Feedback(cell);') == 1
    assert click.index('OutList.Add(') < click.index('Show_Destination_Feedback(cell);')
    src = out/'feedback.cpp'
    src.write_text(r'''
#include <cassert>
#include <vector>
#include <algorithm>
struct Cell {};
struct Coord { int X=128,Y=128,Z=0; };
struct AnimTypeClass { int YSortAdjust=0; } type;
struct Rules { AnimTypeClass* MoveFlash=&type; } rules, *Rule=&rules;
struct Scenario { int UniqueID=42; } scenario, *Scen=&scenario;
struct SessionType { bool single=true; bool Singleplayer_Game(){return single;} } Session;
struct Config {bool RightClickDeselects=true;};
struct RulesExtensionType {Config AttackMoveSettings;};
RulesExtensionType rules_extension,*RuleExtension=&rules_extension;
struct MapType {
    bool IsUnderBridge=false;
    MapType& operator[](const Cell&){return *this;}
    Coord Center_Coord(){return {};}
    int Get_Height_GL(const Coord&){return 208;}
} Map;
int BRIDGE_LEPTON_HEIGHT=416;
struct AnimClass;
struct List {
    std::vector<AnimClass*> items;
    void Add(AnimClass* p){items.push_back(p);}
    void Delete(AnimClass* p){items.erase(std::remove(items.begin(),items.end(),p),items.end());}
} Anims, MoveFlashes;
struct AnimClass {
    int id; Coord coord;
    AnimClass(AnimTypeClass*, const Coord& c):id(++Scen->UniqueID),coord(c){Anims.Add(this);}
};
namespace AttackMove { bool armed=true; bool Armed(){return armed;}
void Cancel_Input(){armed=false;} }
int selected=5;
void Unselect_All(){selected=0;}
int fallthrough=0;
void Right(){
'''+branch+r'''
    ++fallthrough;
}
'''+feedback+r'''
int main(){
    Right(); assert(!AttackMove::armed && selected==0 && fallthrough==0);
    Right(); assert(fallthrough==1);
    AttackMove::armed=true;selected=5;RuleExtension->AttackMoveSettings.RightClickDeselects=false;
    Right(); assert(!AttackMove::armed && selected==5 && fallthrough==1);
    RuleExtension->AttackMoveSettings.RightClickDeselects=true;
    Cell cell;
    Show_Destination_Feedback(cell);
    assert(Anims.items.size()==1 && MoveFlashes.items.empty() && Scen->UniqueID==43);
    assert(Anims.items[0]->coord.Z==208);
    Session.single=false; Map.IsUnderBridge=true;
    Show_Destination_Feedback(cell);
    assert(Anims.items.size()==1 && MoveFlashes.items.size()==1 && Scen->UniqueID==43);
    assert(MoveFlashes.items[0]->id==-2 && MoveFlashes.items[0]->coord.Z==624);
    assert(type.YSortAdjust==-5000);
    Show_Destination_Feedback(cell);
    assert(MoveFlashes.items.size()==2 && Scen->UniqueID==43);
    Rule->MoveFlash=nullptr; Show_Destination_Feedback(cell);
    assert(MoveFlashes.items.size()==2 && Scen->UniqueID==43);
    for(auto p:Anims.items) delete p;
    for(auto p:MoveFlashes.items) delete p;
}
''')
    built = subprocess.run([compiler,'/nologo','/EHsc','/std:c++17',str(src),
                            '/Fe:'+str(out/'feedback.exe'),'/Fo:'+str(out/'feedback.obj')],
                           capture_output=True,text=True,env=environment)
    if built.returncode:
        return {'name':'production feedback','status':'FAIL','output':built.stdout+built.stderr}
    result = subprocess.run([str(out/'feedback.exe')],capture_output=True,text=True,env=environment)
    return {'name':'production feedback','status':'PASS' if result.returncode==0 else 'FAIL',
            'exit_code':result.returncode,'output':result.stdout+result.stderr}
