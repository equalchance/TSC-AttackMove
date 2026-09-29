"""Exercise extracted production attack-move functions with isolated engine doubles."""
from pathlib import Path
import subprocess


def function(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    mode = 'code'
    while depth:
        if end >= len(source):
            raise ValueError(f'Unclosed production body for {signature}')
        char = source[end]
        following = source[end:end+2]
        if mode in ('"', "'"):
            if char == '\\':
                end += 2
                continue
            if char == mode:
                mode = 'code'
        elif mode == 'line':
            if char == '\n':
                mode = 'code'
        elif mode == 'block':
            if following == '*/':
                mode = 'code'
                end += 2
                continue
        elif following == '//':
            mode = 'line'
            end += 2
            continue
        elif following == '/*':
            mode = 'block'
            end += 2
            continue
        elif char in ('"', "'"):
            mode = char
        else:
            depth += (char == '{') - (char == '}')
        end += 1
    return source[start:end]


def extraction_check() -> dict:
    body = 'void Selected(){const char* a="}\\\"{"; /* } */ if(true){call();} // {\n}'
    actual = function('prefix\n' + body + '\nvoid Other(){}', 'void Selected(')
    if actual != body:
        raise ValueError('Production body extraction crossed a function boundary')
    return {'name': 'production extraction boundaries', 'status': 'PASS'}


def run(tree: Path, out: Path, compiler: str, environment: dict) -> list[dict]:
    out.mkdir(parents=True, exist_ok=True)
    code=(tree/'src/extensions/techno/attackmove.cpp').read_text()
    signatures=['bool Armed(', 'bool Targeting(', 'void Cancel_Input(', 'bool Eligible(', 'static bool Controlled(',
                'static void Notice(', 'bool Arm(', 'void Click(', 'void Clear(', 'static void Move(', 'void Execute(',
                'void Cancel_Replaced_Order(', 'unsigned Simulation_CRC(', 'static bool Valid_Target(', 'static TechnoClass* Find_Target(', 'void Tick(']
    # Include new helpers only when present, preserving their production bodies.
    helpers=[]
    for signature in ['static bool Movable(', 'static bool Deals_Damage(', 'static bool Has_Attack_Weapon(', 'static bool Support_Eligible(']:
        if signature in code:helpers.append(function(code,signature))
    body='#include "attackmove_fakes.h"\n#include "attackmove_logic.h"\nnamespace AttackMove {\n'
    body+='void Tick(TechnoClassExtension*); void Clear(TechnoClassExtension*); static void Show_Destination_Feedback(const Cell&){}\n'
    if 'static void Probe(' in code:body+='static void Probe(TechnoClass*){}\n'
    body+='\n'.join(helpers)+'\n'+'\n'.join(function(code,s) for s in signatures)+'\n}\n'
    # Synthetic archetypes cover both weapon slots without tying tests to a mod roster.
    body += 'constexpr bool secondary_infantry_enabled=true,primary_infantry_enabled=true,secondary_vehicle_enabled=true,transport_enabled=true;\n'
    body += 'constexpr int secondary_infantry_primary=0,secondary_infantry_secondary=30,primary_infantry_primary=30,primary_infantry_secondary=0;\n'
    body += 'constexpr int secondary_vehicle_primary=0,secondary_vehicle_secondary=30,transport_passengers=4;\n'
    body+=r'''
void setup(TechnoClass& u,TechnoClassExtension& e,int id,House* house=&player){u.id=id;u.House=house;u.ext=&e;e.unit=&u;}
int main(int argc,char** argv){
 player.HeapID=0;enemy.HeapID=1;enemy.IsPlayerControl=false;Houses.items={&player,&enemy};
 UnitClass combat,support,defense;InfantryClass secondary_infantry;
 TechnoClassExtension ce{&combat},se{&support},de{&defense},he{&secondary_infantry};
 setup(combat,ce,1);setup(support,se,2);setup(defense,de,3,&enemy);setup(secondary_infantry,he,4);
 support.type.CanAttackMove=false;support.primary.Attack=0;support.secondary.Attack=0;
 secondary_infantry.type.CanAttackMove=secondary_infantry_enabled;secondary_infantry.primary.Attack=secondary_infantry_primary;secondary_infantry.secondary.Attack=secondary_infantry_secondary;secondary_infantry.chosen=1;
 const std::string test=argv[1];
 if(test=="primary_infantry"){secondary_infantry.type.CanAttackMove=primary_infantry_enabled;secondary_infantry.primary.Attack=primary_infantry_primary;secondary_infantry.secondary.Attack=primary_infantry_secondary;secondary_infantry.chosen=0;}
 if(test=="support"){
   CurrentObjects.items={&combat,&support};Technos.items=CurrentObjects.items;
   check(AttackMove::Arm(),"mixed group can arm");AttackMove::Click({20,20});
   check(OutList.Count==2,"attack-move must enqueue support movement as well as combat");
   for(auto& event:OutList.items)AttackMove::Execute(event);
   check(support.CurrentMission==MISSION_MOVE && support.NavCom && !se.IsAttackMoving,"support gets ordinary move");
   CurrentObjects.items={&support};OutList={};
   check(AttackMove::Arm(),"support-only selection can arm");AttackMove::Click({10,10});
   check(OutList.Count==1,"support-only click queues move");
 } else if(test=="secondary_infantry" || test=="primary_infantry"){
   check(AttackMove::Eligible(&secondary_infantry),"Secondary infantry with damaging secondary must be combat eligible");
   CurrentObjects.items={&secondary_infantry};Technos.items={&secondary_infantry,&defense};
   check(AttackMove::Arm(),"Secondary infantry can arm");AttackMove::Click({20,20});check(OutList.Count==1,"Secondary infantry order queued");
   AttackMove::Execute(OutList.items[0]);
   check(he.IsAttackMoving && secondary_infantry.CurrentMission==MISSION_ATTACK && secondary_infantry.TarCom==&defense,"Secondary infantry acquires through selected secondary");
 } else if(test=="defense"){
   defense.RTTI=RTTI_BUILDING;
   check(AttackMove::Valid_Target(&combat,&defense),"armed enemy defense must qualify");
   defense.primary.Attack=0;defense.secondary.Attack=0;
   check(!AttackMove::Valid_Target(&combat,&defense),"unarmed structures remain excluded");
   defense.secondary.Attack=20;check(AttackMove::Valid_Target(&combat,&defense),"secondary-only defense qualifies");
   defense.House=&player;check(!AttackMove::Valid_Target(&combat,&defense),"allied defense excluded");
 } else if(test=="reacquire"){
   Technos.items={&combat,&defense};ce.IsAttackMoving=true;ce.AttackMoveDestination={5120,5120,0};
   ce.ThreatScanCadenceTimer=15;Frame=1;combat.CurrentMission=MISSION_ATTACK;combat.TarCom=nullptr;
   AttackMove::Tick(&ce);check(combat.TarCom==&defense && combat.CurrentMission==MISSION_ATTACK,"replacement acquired before periodic deadline");
 } else if(test=="recovery"){
   Technos.items={&combat};ce.IsAttackMoving=true;ce.AttackMoveDestination={5120,5120,0};ce.AttackMoveRetries=2;
   for(Frame=1;Frame<200;++Frame){combat.CurrentMission=MISSION_GUARD;combat.NavCom=nullptr;AttackMove::Tick(&ce);}
   check(combat.moves==2 && !ce.IsAttackMoving,"interrupted movement retries twice then stops");
 } else if(test=="combat_resume"){
   Technos.items={&combat,&defense};EventClass event(0,TargetClass(&combat),{20,20});
   AttackMove::Execute(event);
   check(ce.IsAttackMoving && ce.AttackMoveCombatPending==1 && combat.TarCom==&defense,
         "engagement records a pending route resumption");
   const int retries=ce.AttackMoveRetries;
   defense.Strength=0;combat.TarCom=nullptr;combat.CurrentMission=MISSION_GUARD;combat.NavCom=nullptr;
   Frame=1;AttackMove::Tick(&ce);
   check(combat.CurrentMission==MISSION_MOVE && combat.moves==2 && ce.AttackMoveRetries==retries &&
         ce.AttackMoveCombatPending==0,"first kill resumes route without spending path retries");
   defense.Strength=100;combat.TarCom=&defense;combat.CurrentMission=MISSION_ATTACK;
   Frame=2;AttackMove::Tick(&ce);
   check(ce.AttackMoveCombatPending==1,"second engagement records pending route resumption");
   defense.Strength=0;combat.TarCom=nullptr;combat.CurrentMission=MISSION_GUARD;combat.NavCom=nullptr;
   Frame=3;AttackMove::Tick(&ce);
   check(combat.CurrentMission==MISSION_MOVE && combat.moves==3 && ce.AttackMoveRetries==retries &&
         ce.AttackMoveCombatPending==0,"consecutive kills do not exhaust route retries");
 } else if(test=="input_once"){
   CurrentObjects.items={&combat};check(AttackMove::Arm(),"A arms targeting");
   AttackMove::Click({-1,20});check(AttackMove::Armed() && OutList.Count==0,"invalid destination stays armed");
   OutList.Count=MAX_EVENTS;AttackMove::Click({20,20});
   check(AttackMove::Armed() && OutList.items.empty(),"full queue stays armed for retry");
   OutList.Count=0;AttackMove::Click({20,20});
   check(!AttackMove::Armed() && OutList.Count==1 && CurrentObjects.Count()==1,
         "valid A-click queues once and keeps selection");
   AttackMove::Click({21,21});check(OutList.Count==1,"second click cannot queue another attack-move");
   check(AttackMove::Arm(),"A can arm another order");AttackMove::Cancel_Input();
   check(!AttackMove::Armed() && CurrentObjects.Count()==1,"cancellation keeps selection");
 } else if(test=="persistent_input"){
   RuleExtension->AttackMoveSettings.PersistentTargeting=true;
   CurrentObjects.items={&combat};check(AttackMove::Arm(),"persistent mode arms");
   AttackMove::Click({20,20});check(AttackMove::Armed() && OutList.Count==1,"persistent mode retains targeting after valid click");
   AttackMove::Click({21,21});check(AttackMove::Armed() && OutList.Count==2,"persistent mode accepts next destination");
   AttackMove::Cancel_Input();check(!AttackMove::Armed() && CurrentObjects.Count()==1,"persistent cancellation keeps selection");
 } else if(test=="config_crc"){
   const auto base=AttackMove::Simulation_CRC(0x12345678u);
   RuleExtension->AttackMoveSettings.TargetArmedBuildings=false;
   check(AttackMove::Simulation_CRC(0x12345678u)!=base,"global switch changes simulation CRC");
   RuleExtension->AttackMoveSettings.TargetArmedBuildings=true;
   UnitTypes.items={&combat.type};
   const auto type_base=AttackMove::Simulation_CRC(0x12345678u);
   combat.type.AttackMoveAcquireRangePercent=200;
   check(AttackMove::Simulation_CRC(0x12345678u)!=type_base,"type range changes simulation CRC before any units spawn");
   combat.type.AttackMoveAcquireRangePercent=-1;
   combat.type.AttackMoveSupportEnabled=0;
   check(AttackMove::Simulation_CRC(0x12345678u)!=type_base,"type support switch changes simulation CRC");
 } else if(test=="safety"){
   defense.House=&player;check(!AttackMove::Valid_Target(&combat,&defense),"allies excluded");defense.House=&enemy;
   combat.range[0]=false;defense.Position={4096,0,0};check(!AttackMove::Valid_Target(&combat,&defense),"no acquisition outside acquisition range");combat.range[0]=true;defense.Position={0,0,0};
   combat.fire=FIRE_AMMO;check(!AttackMove::Valid_Target(&combat,&defense),"empty ammo excluded");combat.fire=FIRE_REARM;
   check(AttackMove::Valid_Target(&combat,&defense),"reloading target retained");combat.warhead.passive=false;
   check(!AttackMove::Valid_Target(&combat,&defense),"passive acquire restrictions preserved");combat.warhead.passive=true;
   Technos.items={&combat};ce.IsAttackMoving=true;combat.EMPFramesRemaining=60;ce.AttackMoveRetries=2;
   AttackMove::Tick(&ce);check(ce.IsAttackMoving && !combat.moves && ce.AttackMoveRetries==2,"EMP suspends");combat.EMPFramesRemaining=0;
   EventClass e(0,TargetClass(&combat),{20,20});e.Type=EVENT_IDLE;AttackMove::Cancel_Replaced_Order(e);
   check(!ce.IsAttackMoving,"replacement stop wins");
   CurrentObjects.items={&combat,&support};AttackMove::Arm();OutList.Count=63;AttackMove::Click({20,20});
   check(OutList.items.empty(),"mixed group queue must be atomic");
   check(AttackMoveLogic::Better_Target(5,1,5,2),"stable identifier tie break");
 } else if(test=="secondary_vehicle" || test=="armed_transport"){
   if(test=="secondary_vehicle"){combat.type.CanAttackMove=secondary_vehicle_enabled;combat.primary.Attack=secondary_vehicle_primary;combat.secondary.Attack=secondary_vehicle_secondary;combat.chosen=1;}
   else{combat.type.CanAttackMove=transport_enabled;combat.type.MaxPassengers=transport_passengers;}
   Technos.items={&combat,&defense};EventClass event(0,TargetClass(&combat),{20,20});AttackMove::Execute(event);
   check(ce.IsAttackMoving && combat.TarCom==&defense && combat.CurrentMission==MISSION_ATTACK,"secondary-only vehicle or armed transport engages");
 } else if(test=="wildlife"){
   enemy.houseType.IsMultiplayPassive=true;
   check(AttackMove::Valid_Target(&combat,&defense),"armed hostile neutral must qualify");
   defense.primary.Attack=0;check(!AttackMove::Valid_Target(&combat,&defense),"unarmed neutral remains protected");
   defense.secondary.Attack=30;check(AttackMove::Valid_Target(&combat,&defense),"secondary-only neutral qualifies");
   defense.House=&player;check(!AttackMove::Valid_Target(&combat,&defense),"allied wildlife excluded");
   InfantryClass civilian;TechnoClassExtension ve{&civilian};setup(civilian,ve,12,&enemy);civilian.type.IsCivilian=true;
   check(!AttackMove::Valid_Target(&combat,&civilian),"civilian remains protected even when armed");
 } else if(test=="payload"){
   combat.primary.Attack=0;combat.secondary.Attack=0;
   check(!AttackMove::Eligible(&combat),"zero-damage tool cannot attack-move");
   check(!AttackMove::Valid_Target(&combat,&defense),"zero-damage tool cannot acquire");
   combat.primary.AmbientDamage=2;combat.primary.AttachedParticleSystem=&combat;
   combat.primary.IsUseFireParticles=true;
   check(AttackMove::Eligible(&combat) && AttackMove::Valid_Target(&combat,&defense),"fire particle weapon acquires");
   combat.primary.IsUseFireParticles=false;combat.primary.IsRailgun=true;
   check(AttackMove::Eligible(&combat) && AttackMove::Valid_Target(&combat,&defense),"railgun acquires");
   combat.primary.IsRailgun=false;combat.primary.IsUseSparkParticles=true;
   check(AttackMove::Eligible(&combat) && AttackMove::Valid_Target(&combat,&defense),"spark particle weapon acquires");
   combat.primary.IsUseSparkParticles=false;
   check(!AttackMove::Eligible(&combat),"ambient damage without particle mode is not armed");
   combat.primary.AmbientDamage=0;
   Weapon child{30,&combat.bullet,&combat.warhead};
   combat.bullet.IsAirburst=true;combat.bullet.AirburstWeapon=&child;
   check(AttackMove::Eligible(&combat) && AttackMove::Valid_Target(&combat,&defense),"airburst child weapon acquires");
   combat.bullet.IsAirburst=false;combat.bullet.IsSplits=true;
   check(AttackMove::Eligible(&combat) && AttackMove::Valid_Target(&combat,&defense),"split child weapon acquires");
   child.Attack=0;
   check(!AttackMove::Eligible(&combat),"zero-damage child does not arm");
   child.Attack=30;child.WarheadPtr=nullptr;
   check(!AttackMove::Eligible(&combat),"child lacking warhead does not arm");
   defense.primary.Attack=0;defense.secondary.Attack=0;
   check(!AttackMove::Valid_Target(&combat,&defense),"unarmed defense is protected");
   defense.primary.IsRailgun=true;defense.primary.AmbientDamage=150;defense.primary.AttachedParticleSystem=&defense;
   combat.primary.Attack=30;combat.bullet.IsSplits=false;
   check(AttackMove::Valid_Target(&combat,&defense),"payload-only defense is a target");
   enemy.houseType.IsMultiplayPassive=true;defense.RTTI=RTTI_UNIT;
   check(AttackMove::Valid_Target(&combat,&defense),"payload-only hostile passive unit is a target");
 } else if(test=="order"){
   CurrentObjects.items={&support,&secondary_infantry,&combat};
   check(AttackMove::Arm(),"mixed order arms");AttackMove::Click({20,20});
   check(OutList.items.size()==3,"all mixed units queue");
   check(OutList.items[0].Data.AttackMove.Whom.Value()==1 &&
         OutList.items[1].Data.AttackMove.Whom.Value()==2 &&
         OutList.items[2].Data.AttackMove.Whom.Value()==4,"events sorted by encoded unit ID");
 } else if(test=="determinism"){
   UnitClass other;TechnoClassExtension oe{&other};setup(other,oe,5,&enemy);
   defense.Position={500,0,0};other.Position={500,0,0};
   Technos.items={&combat,&other,&defense};
   check(AttackMove::Find_Target(&combat)==&defense,"lower target ID wins reversed enumeration");
   Technos.items={&combat,&defense,&other};
   check(AttackMove::Find_Target(&combat)==&defense,"lower target ID wins forward enumeration");
 } else if(test=="support_categories"){
   support.primary.Attack=-25;combat.health=.5;combat.RTTI=RTTI_AIRCRAFT;
   check(AttackMove::Valid_Target(&support,&combat),"repair vehicle accepts grounded aircraft");
   combat.airborne=true;check(!AttackMove::Valid_Target(&support,&combat),"repair vehicle rejects airborne aircraft");
   combat.airborne=false;combat.RTTI=RTTI_UNIT;
   secondary_infantry.primary.Attack=-25;secondary_infantry.secondary.Attack=0;secondary_infantry.chosen=0;secondary_infantry.type.IsMechanic=true;
   check(AttackMove::Valid_Target(&secondary_infantry,&combat),"mechanic repairs vehicles");
   combat.RTTI=RTTI_INFANTRY;check(!AttackMove::Valid_Target(&secondary_infantry,&combat),"mechanic rejects infantry");
   secondary_infantry.type.IsOmniHealer=true;check(AttackMove::Valid_Target(&secondary_infantry,&combat),"omni-healer accepts infantry");
   combat.RTTI=RTTI_UNIT;check(AttackMove::Valid_Target(&secondary_infantry,&combat),"omni-healer accepts vehicles");
 } else if(test=="range15"){
   combat.range[0]=false;combat.fire=FIRE_RANGE;defense.Position={1536,0,0};
   check(AttackMove::Valid_Target(&combat,&defense),"acquire at 1.5x weapon range");
   defense.Position.X=1537;check(!AttackMove::Valid_Target(&combat,&defense),"reject just outside 1.5x");
   defense.Position.X=1200;combat.Ammo=0;check(!AttackMove::Valid_Target(&combat,&defense),"range error must not hide empty ammo");
 } else if(test=="config_range"){
   combat.range[0]=false;combat.fire=FIRE_RANGE;
   RuleExtension->AttackMoveSettings.AcquireRangePercent=100;
   defense.Position.X=1024;check(AttackMove::Valid_Target(&combat,&defense),"map acquire boundary inclusive");
   defense.Position.X=1025;check(!AttackMove::Valid_Target(&combat,&defense),"map acquire boundary exclusive");
   combat.type.AttackMoveAcquireRangePercent=200;
   defense.Position.X=2048;check(AttackMove::Valid_Target(&combat,&defense),"type acquire override wins");
   defense.Position.X=2049;check(!AttackMove::Valid_Target(&combat,&defense),"type acquire override has exact boundary");
   combat.type.AttackMoveAcquireRangePercent=-1;
   combat.range[0]=true;defense.Position.X=1200;
   check(AttackMove::Valid_Target(&combat,&defense),"native in-range bonus wins beyond configured radius");
 } else if(test=="support_switches"){
   support.primary.Attack=-25;combat.health=.5;
   check(AttackMove::Eligible(&support),"support defaults on");
   RuleExtension->AttackMoveSettings.SupportEnabled=false;
   check(!AttackMove::Eligible(&support),"global support off disables inherited type");
   support.type.AttackMoveSupportEnabled=1;
   check(AttackMove::Eligible(&support),"explicit type yes overrides global no");
   RuleExtension->AttackMoveSettings.SupportEnabled=true;
   support.type.AttackMoveSupportEnabled=0;
   check(!AttackMove::Eligible(&support),"explicit type no overrides global yes");
   check(!AttackMove::Valid_Target(&support,&combat),"disabled support does not heal");
 } else if(test=="melee"){
   Technos.items={&secondary_infantry,&defense};he.IsAttackMoving=true;he.AttackMoveDestination={5120,5120,0};
   secondary_infantry.CurrentMission=MISSION_ATTACK;secondary_infantry.TarCom=&defense;secondary_infantry.weapon_range=512;
   secondary_infantry.range[1]=false;secondary_infantry.fire=FIRE_RANGE;defense.Position={800,0,0};
   AttackMove::Tick(&he);check(secondary_infantry.TarCom==&defense && secondary_infantry.CurrentMission==MISSION_ATTACK,"retain melee target outside acquisition range");
   defense.Position.X=1537;AttackMove::Tick(&he);check(!secondary_infantry.TarCom && secondary_infantry.CurrentMission==MISSION_MOVE,"abandon target beyond 3x chase limit");
   defense.Position.X=800;secondary_infantry.TarCom=&defense;secondary_infantry.CurrentMission=MISSION_ATTACK;
   defense.Strength=0;AttackMove::Tick(&he);check(!secondary_infantry.TarCom && secondary_infantry.CurrentMission==MISSION_MOVE,"resume after target dies");
 } else if(test=="heal" || test=="repair"){
   InfantryClass medic,patient;TechnoClassExtension me{&medic},pe{&patient};setup(medic,me,10);setup(patient,pe,11);
   TechnoClass* healer=test=="heal"?static_cast<TechnoClass*>(&medic):&support;
   TechnoClass* ally=test=="heal"?static_cast<TechnoClass*>(&patient):&combat;
   auto ext=healer->ext;healer->type.CanAttackMove=false;healer->primary.Attack=-25;ally->health=.5;
   Technos.items={healer,ally,&defense};EventClass event(0,TargetClass(healer),{20,20});AttackMove::Execute(event);
   check(ext->IsAttackMoving && healer->TarCom==ally && healer->CurrentMission==MISSION_ATTACK,"support acquires compatible injured ally");
   check(!AttackMove::Valid_Target(healer,&defense),"never heal enemy");
   defense.House=&player;defense.health=.5;defense.RTTI=RTTI_BUILDING;
   check(!AttackMove::Valid_Target(healer,&defense),"support does not acquire buildings");
   defense.RTTI=test=="heal"?RTTI_UNIT:RTTI_INFANTRY;
   check(!AttackMove::Valid_Target(healer,&defense),"support respects patient category");
   healer->warhead.modifier=0;check(!AttackMove::Valid_Target(healer,ally),"never acquire immune ally");healer->warhead.modifier=1;
   ally->health=1;AttackMove::Tick(ext);check(!healer->TarCom && healer->CurrentMission==MISSION_MOVE,"resume when healed");
   EventClass stop(0,TargetClass(healer),{20,20});stop.Type=EVENT_IDLE;AttackMove::Cancel_Replaced_Order(stop);check(!ext->IsAttackMoving,"support Stop cancels route");
 }
 std::printf("PASS %s\n",test.c_str());
}
'''
    src=out/'attack-move.cpp';src.write_text(body)
    compile_run=subprocess.run([compiler,'/nologo','/EHsc','/std:c++17','/I'+str(Path(__file__).parent),
                        '/I'+str(tree/'src/extensions/techno'),str(src),'/Fe:'+str(out/'test.exe'),'/Fo:'+str(out/'test.obj')],capture_output=True,text=True,env=environment)
    if compile_run.returncode:
        raise RuntimeError(compile_run.stdout+compile_run.stderr)
    results=[]
    cases=['support','secondary_infantry','defense','reacquire','recovery','combat_resume','input_once','persistent_input','config_crc','safety','range15','config_range','support_switches','melee','heal','repair','wildlife','primary_infantry','secondary_vehicle','armed_transport','payload','order','determinism','support_categories']
    for case in cases:
        result=subprocess.run([str(out/'test.exe'),case],capture_output=True,text=True,env=environment)
        results.append(dict(case=case,exit_code=result.returncode,output=result.stdout+result.stderr))
    return results
