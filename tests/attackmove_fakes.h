#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "attackmove_config.h"
#define DEBUG_WARNING(...) ((void)0)
#define DEBUG_INFO(...) ((void)0)
enum { RTTI_UNIT, RTTI_INFANTRY, RTTI_AIRCRAFT, RTTI_BUILDING };
enum WeaponSlotType { WEAPON_SLOT_PRIMARY, WEAPON_SLOT_SECONDARY };
enum { MISSION_MOVE, MISSION_ATTACK, MISSION_GUARD, MISSION_UNLOAD };
enum { FIRE_OK, FIRE_REARM, FIRE_FACING, FIRE_ROTATING, FIRE_MOVING, FIRE_CLOAKED, FIRE_BUSY, FIRE_RANGE, FIRE_AMMO, FIRE_ILLEGAL, FIRE_CANT };
enum { UNCLOAKED, CLOAKED, GAME_NORMAL, SUPER_NONE, MOUSE_NORMAL, MOUSE_CAN_ATTACK, MOUSE_PATROL_WAYPOINT, MAX_EVENTS=64 };
struct MouseTypeClass {static int From_Name(const char*){return MOUSE_PATROL_WAYPOINT;}};
enum { EVENT_MEGAMISSION, EVENT_MEGAMISSION_F, EVENT_IDLE, EVENT_SCATTER, EVENT_DEPLOY, EVENT_ARCHIVE };
struct Cell { int X=0,Y=0; Cell()=default;Cell(int x,int y):X(x),Y(y){} bool operator==(Cell b) const {return X==b.X && Y==b.Y;} };
struct Coord { int X=0,Y=0,Z=0; Coord()=default; Coord(int x,int y,int z):X(x),Y(y),Z(z){} Cell As_Cell() const {return {X/256,Y/256};} };
struct Weapon;
struct Bullet { bool IsAntiAircraft=true, IsAntiGround=true,IsAirburst=false,IsSplits=false; Weapon* AirburstWeapon=nullptr; };
struct Warhead { bool passive=true,force=true; double modifier=1; };
struct Weapon { int Attack=30; struct Bullet* Bullet=nullptr; Warhead* WarheadPtr=nullptr;
    int AmbientDamage=0; void* AttachedParticleSystem=nullptr;
    bool IsRailgun=false,IsUseFireParticles=false,IsUseSparkParticles=false; };
using WeaponTypeClass=Weapon;
struct WeaponInfo { struct Weapon* Weapon=nullptr; };
struct Type {
    bool CanAttackMove=true,IsSpawned=false,IsHunterSeeker=false,IsToHarvest=false;
    bool IsJumpJet=false,IsEngineer=false,IsMechanic=false,IsOmniHealer=false,IsCivilian=false;
    bool IsLegalTarget=true,IsInvisible=false;
    int MaxSpeed=10,AttackMoveScanInterval=15,Armor=0,MaxPassengers=0;
    int AttackMoveAcquireRangePercent=-1,AttackMoveRetainRangePercent=-1,AttackMoveSupportEnabled=-1;
    std::string IniName="TEST";
};
struct HouseType {bool IsMultiplayPassive=false;};
struct House {HouseType houseType; HouseType* Class=&houseType; int HeapID=0; bool IsPlayerControl=true,IsDefeated=false; bool Is_Ally(House* h){return h==this;}};
struct TechnoClassExtension;
struct MapCell {Coord center; Coord Center_Coord(){return center;} bool Sensed_By(int){return true;}};
struct TechnoClass {
    bool IsActive=true,IsInLimbo=false,renovator=false,immobilized=false,movable=true,airborne=false;
    int Strength=100,RTTI=RTTI_UNIT,id=0,Cloak=UNCLOAKED,EMPFramesRemaining=0,CurrentMission=MISSION_GUARD;
    struct House* House=nullptr;
    Type type;Type* TClass=&type;
    Bullet bullet;Warhead warhead;Weapon primary{30,&bullet,&warhead},secondary{0,&bullet,&warhead};
    WeaponInfo weapons[2]{{&primary},{&secondary}};
    TechnoClassExtension* ext=nullptr;Coord Position;TechnoClass* TarCom=nullptr;MapCell* NavCom=nullptr;
    bool range[2]{true,true};int fire=FIRE_OK,chosen=0,moves=0;bool cleared_queue=false;
    int Ammo=-1,weapon_range=1024;double health=1;
    int Weapon_Range(WeaponSlotType){return weapon_range;}
    double Get_Health_Ratio(){return health;}
    bool Is_Techno(){return true;} bool Is_Renovator(){return renovator;}
    bool Is_Weapon_Equipped(){return weapons[0].Weapon||weapons[1].Weapon;}
    bool Can_Player_Move(){return movable;} bool Is_Immobilized(){return immobilized;}
    bool In_Air(){return airborne;} Coord Center_Coord(){return Position;}
    WeaponInfo* Get_Weapon(int slot){return &weapons[slot];}
    WeaponSlotType What_Weapon_Should_I_Use(TechnoClass*){return static_cast<WeaponSlotType>(chosen);}
    bool In_Range_Of(TechnoClass*,int slot){return range[slot];} int Can_Fire(TechnoClass*,int){return fire;}
    void Assign_Target(TechnoClass* t){TarCom=t;} void Assign_Destination(MapCell* c){NavCom=c;}
    void Assign_Mission(int m){CurrentMission=m;if(m==MISSION_MOVE)++moves;}
    void Commence(){} void Shorten_Mission_Timer(){} void Clear_Navigation_List(){cleared_queue=true;}
    static void Reset_Action_Line_Timer(){}
};
using FootClass=TechnoClass;
struct UnitClass:TechnoClass {Type* Class=&type;};
struct InfantryClass:TechnoClass {Type* Class=&type;InfantryClass(){RTTI=RTTI_INFANTRY;}};
struct TechnoClassExtension {
    TechnoClass* unit;bool IsAttackMoving=false,HasOpportunityFireTarget=false;
    Coord AttackMoveDestination;int ThreatScanCadenceTimer=0,AttackMoveCombatPending=0;
    int AttackMoveRetries=0,AttackMoveRetryTimer=0;
    TechnoClass* This(){return unit;}
};
namespace Extension {Type* Fetch(Type* p){return p;} TechnoClassExtension* Fetch(TechnoClass* p){return p->ext;}}
namespace Verses {double Get_Modifier(int,Warhead* w){return w->modifier;} bool Get_PassiveAcquire(int,Warhead* w){return w->passive;} bool Get_ForceFire(int,Warhead* w){return w->force;}}
template<class T>struct List {std::vector<T*> items;int Count(){return int(items.size());} T* operator[](int i){return items[i];}};
List<TechnoClass> CurrentObjects,Technos;List<House> Houses;
List<Type> UnitTypes,InfantryTypes;
struct TargetClass {int id=-1;TargetClass()=default;TargetClass(TechnoClass* p):id(p->id){} int Encode()const{return id;} int Kind()const{return 1;} int Value()const{return id;}};
using xTargetClass=TargetClass;
struct EventClassExt {int ID=0,Type=EVENT_MEGAMISSION;struct DataType {struct AM {TargetClass Whom;Cell Where;} AttackMove;struct TG {TargetClass Whom;} Target;} Data;
    EventClassExt(int id,TargetClass t,Cell c):ID(id){Data.AttackMove={t,c};Data.Target.Whom=t;}};
using EventClass=EventClassExt;
struct Queue {int Count=0;std::vector<EventClass> items;void Add(const EventClass& e){items.push_back(e);++Count;}} OutList;
struct MapType {MapCell cell;bool IsSellMode=false,IsRepairMode=false,IsPowerMode=false,IsWaypointMode=false;void* PendingObject=nullptr;int TargettingType=SUPER_NONE;
    bool In_Radar(Cell c){return c.X>=0&&c.Y>=0&&c.X<100&&c.Y<100;}
    MapCell& operator[](const Cell& c){cell.center={c.X*256,c.Y*256,0};return cell;}
    MapCell& operator[](const Coord& c){return operator[](c.As_Cell());}
    void Set_Default_Mouse(int,bool){}
} Map;
struct Tactical {bool IsAttackMoveMode=false,IsBeaconPlacementMode=false,IsInfoTextSet=false;int InfoTextTimer=0;void Set_Info_Text(const char*){}} tactical,*TacticalMapExtension=&tactical;
struct Scenario {bool InputLock=false;} scenario,*Scen=&scenario;
struct SessionType {int Type=GAME_NORMAL;struct MessagesType {bool Is_Edit(){return false;}} Messages;} Session;
House player,enemy;House* PlayerPtr=&player;unsigned Frame=0;
struct RulesExtensionType {AttackMoveConfig::Settings AttackMoveSettings=AttackMoveConfig::Defaults();};
RulesExtensionType rules_extension,*RuleExtension=&rules_extension;
bool Is_Target_Techno(TechnoClass* p){return p!=nullptr;}
inline void check(bool ok,const char* message){if(!ok){std::printf("FAIL: %s\n",message);std::exit(2);}}
