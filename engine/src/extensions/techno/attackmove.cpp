// SPDX-License-Identifier: GPL-3.0-or-later
#include "always.h"
#include "attackmove.h"
#include "attackmove_logic.h"
#include "attackmove_config.h"
#include "technoext.h"
#include "technotypeext.h"
#include "extension.h"
#include "eventext.h"
#include "foot.h"
#include "infantry.h"
#include "infantrytype.h"
#include "infantrytypeext.h"
#include "unit.h"
#include "unittype.h"
#include "unittypeext.h"
#include "house.h"
#include "housetype.h"
#include "session.h"
#include "scenario.h"
#include "tacticalext.h"
#include "tibsun_globals.h"
#include "tibsun_inline.h"
#include "mouse.h"
#include "map.h"
#include "cell.h"
#include "target.h"
#include "weapontype.h"
#include "bullettype.h"
#include "verses.h"
#include "queue.h"
#include "anim.h"
#include "animtype.h"
#include "rules.h"
#include "rulesext.h"
#include "debughandler.h"
#include <cstddef>
#include <algorithm>
#include <vector>
#include <limits>

namespace AttackMove {
bool Armed() { return TacticalMapExtension && TacticalMapExtension->IsAttackMoveMode; }
bool Targeting() {
    return Armed() && !Map.PendingObject && !Map.IsSellMode && !Map.IsRepairMode &&
        !Map.IsPowerMode && !Map.IsWaypointMode && Map.TargettingType==SUPER_NONE &&
        !TacticalMapExtension->IsBeaconPlacementMode && !Scen->InputLock;
}
void Cancel_Input() {
    if (Armed()) {
        TacticalMapExtension->IsAttackMoveMode = false;
        Map.Set_Default_Mouse(MOUSE_NORMAL, false);
    }
}
static bool Movable(TechnoClass* unit) {
    if (!unit || !unit->IsActive || unit->IsInLimbo || unit->Strength <= 0 || !unit->House || !unit->TClass)
        return false;
    if (unit->RTTI != RTTI_UNIT && unit->RTTI != RTTI_INFANTRY && unit->RTTI != RTTI_AIRCRAFT) return false;
    // Noncombatants receive native Move; buildings and autonomous spawned units do not.
    return unit->TClass->MaxSpeed > 0 && !Extension::Fetch(unit->TClass)->IsSpawned && !unit->TClass->IsHunterSeeker;
}
static bool Deals_Damage(const WeaponTypeClass* weapon) {
    if (!weapon) return false;
    if (weapon->Attack > 0) return true;
    if (weapon->Attack < 0) return false;
    // Railgun and fire particle weapons use AmbientDamage while the launching
    // weapon has Damage=0. Split and airburst projectiles deal child-weapon damage.
    if (weapon->AmbientDamage > 0 && weapon->AttachedParticleSystem &&
        (weapon->IsRailgun || weapon->IsUseFireParticles || weapon->IsUseSparkParticles)) return true;
    const auto bullet = weapon->Bullet;
    const auto child = bullet ? bullet->AirburstWeapon : nullptr;
    return bullet && (bullet->IsAirburst || bullet->IsSplits) && child &&
        child->Attack > 0 && child->Bullet && child->WarheadPtr;
}
static bool Has_Attack_Weapon(TechnoClass* unit) {
    // Husk uses a zero-damage primary and a damaging secondary.
    for (auto slot : {WEAPON_SLOT_PRIMARY, WEAPON_SLOT_SECONDARY}) {
        const auto info = unit->Get_Weapon(slot);
        if (info && Deals_Damage(info->Weapon)) return true;
    }
    return false;
}
static bool Support_Eligible(TechnoClass* unit) {
    if (!RuleExtension) return false;
    if (!Movable(unit) || (unit->RTTI!=RTTI_UNIT && unit->RTTI!=RTTI_INFANTRY)) return false;
    if (!AttackMoveConfig::Resolve_Support(Extension::Fetch(unit->TClass)->AttackMoveSupportEnabled,
            RuleExtension->AttackMoveSettings.SupportEnabled)) return false;
    if (unit->RTTI==RTTI_UNIT && static_cast<UnitClass*>(unit)->Class->IsToHarvest) return false;
    if (unit->RTTI==RTTI_INFANTRY) {
        const auto type=static_cast<InfantryClass*>(unit)->Class;
        if (type->IsEngineer || type->IsJumpJet) return false;
    }
    // Healing weapons identify support units without enabling unarmed transports.
    for (auto slot : {WEAPON_SLOT_PRIMARY, WEAPON_SLOT_SECONDARY}) {
        const auto info=unit->Get_Weapon(slot);
        if (info && info->Weapon && info->Weapon->Attack<0) return true;
    }
    return false;
}
bool Eligible(TechnoClass* unit) {
    if (Support_Eligible(unit)) return true;
    if (!unit || !unit->IsActive || unit->IsInLimbo || unit->Strength <= 0 || !unit->House || !unit->TClass)
        return false;
    if (unit->RTTI != RTTI_UNIT && unit->RTTI != RTTI_INFANTRY) return false;
    const auto type = unit->TClass;
    // Explicit type opt-in also permits armed passenger carriers such as campaign Kazuar.
    // Default transport types remain excluded by the rules enablement pass.
    if (!Extension::Fetch(type)->CanAttackMove || Extension::Fetch(type)->IsSpawned || type->MaxSpeed <= 0 ||
        type->IsHunterSeeker || unit->Is_Renovator() || !unit->Is_Weapon_Equipped()) return false;
    if (unit->RTTI == RTTI_UNIT && static_cast<UnitClass*>(unit)->Class->IsToHarvest) return false;
    if (unit->RTTI == RTTI_INFANTRY) {
        const auto infantry = static_cast<InfantryClass*>(unit)->Class;
        if (infantry->IsJumpJet || infantry->IsEngineer) return false;
        const auto extension = Extension::Fetch(infantry);
        if (extension->IsMechanic || extension->IsOmniHealer) return false;
    }
    return Has_Attack_Weapon(unit);
}
static bool Controlled(TechnoClass* unit, int issuer) {
    if (!unit || !unit->House || issuer < 0 || issuer >= Houses.Count() || !Houses[issuer]) return false;
    return Session.Type == GAME_NORMAL ? unit->House->IsPlayerControl : unit->House->HeapID == issuer;
}
static void Notice(const char* text) {
    TacticalMapExtension->Set_Info_Text(text);
    TacticalMapExtension->IsInfoTextSet = true;
    TacticalMapExtension->InfoTextTimer = 180;
}
bool Arm() {
    if (!PlayerPtr || PlayerPtr->IsDefeated || Scen->InputLock || Map.PendingObject || Session.Messages.Is_Edit()) return false;
    bool any = false;
    for (int i=0;i<CurrentObjects.Count();++i) {
        auto obj = CurrentObjects[i];
        if (obj && obj->Is_Techno()) {
            auto unit = static_cast<TechnoClass*>(obj);
            any |= Movable(unit) && Controlled(unit, PlayerPtr->HeapID);
        }
    }
    if (!any) {
        // Log only explicit rejected activation attempts, never the per-frame scan.
        // Keep the game screen quiet while diagnosing unexpected type exclusions.
        for (int i=0;i<CurrentObjects.Count();++i) {
            auto obj=CurrentObjects[i];
            if (!obj || !obj->Is_Techno()) continue;
            auto unit=static_cast<TechnoClass*>(obj);
            if (!unit->TClass || !unit->House) continue;
            auto type=unit->TClass;
            DEBUG_WARNING("AttackMove rejected [{}]: eligible={} controlled={} active={} limbo={} strength={} enabled={} spawned={} speed={} renovator={} armed={} primary_damage={}.\n",
                type->IniName.c_str(), Eligible(unit), Controlled(unit,PlayerPtr->HeapID),
                unit->IsActive,unit->IsInLimbo,unit->Strength,Extension::Fetch(type)->CanAttackMove,
                Extension::Fetch(type)->IsSpawned,static_cast<int>(type->MaxSpeed),unit->Is_Renovator(),unit->Is_Weapon_Equipped(),
                unit->Get_Weapon(WEAPON_SLOT_PRIMARY) && unit->Get_Weapon(WEAPON_SLOT_PRIMARY)->Weapon
                    ? unit->Get_Weapon(WEAPON_SLOT_PRIMARY)->Weapon->Attack : 0);
        }
        return false;
    }
    Map.IsSellMode = Map.IsRepairMode = Map.IsPowerMode = Map.IsWaypointMode = false;
    Map.TargettingType = SUPER_NONE;
    TacticalMapExtension->IsBeaconPlacementMode = false;
    TacticalMapExtension->IsAttackMoveMode = true;
    Map.Set_Default_Mouse(MOUSE_CAN_ATTACK, false);
    return true;
}
static void Show_Destination_Feedback(const Cell& cell) {
    if (!Rule || !Rule->MoveFlash) return;
    // Match FootClass::Active_Click_With (004A36CE-004A3870). Multiplayer
    // move flashes use ID -2 and a separate list, preserving simulation IDs.
    static_assert(offsetof(ScenarioClass, UniqueID) == 0x214, "Native move-flash ID field");
    static_assert(offsetof(AnimTypeClass, YSortAdjust) == 0x1C8, "Native move-flash sort field");
    auto coord = Map[cell].Center_Coord();
    coord.Z = Map.Get_Height_GL(coord);
    if (Map[cell].IsUnderBridge) coord.Z += BRIDGE_LEPTON_HEIGHT;
    const bool multiplayer = !Session.Singleplayer_Game();
    const int saved_id = Scen->UniqueID;
    if (multiplayer) {
        Scen->UniqueID = -3;
        Rule->MoveFlash->YSortAdjust = -5000;
    }
    auto flash = new AnimClass(Rule->MoveFlash, coord);
    if (multiplayer) {
        Scen->UniqueID = saved_id;
        // The retained constructor hook also excludes ID -2 from AnimExtensions.
        if (flash) {
            Anims.Delete(flash);
            MoveFlashes.Add(flash);
        }
    }
}
void Click(const Cell& cell) {
    if (!Armed()) return;
    if (!PlayerPtr || PlayerPtr->IsDefeated || Scen->InputLock) { Cancel_Input(); return; }
    if (!Map.In_Radar(cell)) { Notice("Choose a destination inside the map."); return; }
    std::vector<TechnoClass*> selected;
    for (int i=0;i<CurrentObjects.Count();++i) {
        auto obj = CurrentObjects[i];
        if (obj && obj->Is_Techno()) {
            auto unit = static_cast<TechnoClass*>(obj);
            if (Movable(unit) && Controlled(unit,PlayerPtr->HeapID)) selected.push_back(unit);
        }
    }
    std::sort(selected.begin(),selected.end(),[](auto a,auto b) {
        return static_cast<unsigned>(TargetClass(a).Encode()) < static_cast<unsigned>(TargetClass(b).Encode());
    });
    if (selected.empty()) return;
    if (selected.size() > MAX_EVENTS) {
        Notice("Attack-Move accepts up to 64 units per order. Select a smaller group."); return;
    }
    if (!AttackMoveLogic::Queue_Fits(OutList.Count,static_cast<int>(selected.size()),MAX_EVENTS)) {
        Notice("Command queue full. Click again to retry Attack-Move."); return;
    }
    // Single-threaded input: the capacity check reserves the entire group.
    for (auto unit : selected) {
        const EventClassExt event(PlayerPtr->HeapID,TargetClass(unit),cell);
        OutList.Add(*reinterpret_cast<const EventClass*>(&event));
    }
    // Our consumed click bypasses the native reset at 00478BD8. Restore its
    // 25-frame display window so the existing renderer shows movement lines.
    TechnoClass::Reset_Action_Line_Timer();
    Show_Destination_Feedback(cell);
    // Finish local targeting; queued orders and the current selection remain intact.
    if (!RuleExtension || !RuleExtension->AttackMoveSettings.PersistentTargeting) Cancel_Input();
}
void Clear(TechnoClassExtension* ext) {
    ext->IsAttackMoving = false;
    ext->AttackMoveDestination = Coord(0,0,0);
    ext->ThreatScanCadenceTimer = 0;
    ext->AttackMoveCombatPending = 0;
    ext->AttackMoveRetries = 0;
    ext->AttackMoveRetryTimer = 0;
}
unsigned Simulation_CRC(unsigned crc) {
    crc=AttackMoveLogic::Mix(crc,AttackMoveLogic::Protocol);
    const auto cfg=RuleExtension ? RuleExtension->AttackMoveSettings : AttackMoveConfig::Defaults();
    crc=AttackMoveConfig::Config_CRC(crc,cfg);
    // Include configuration even when no unit of a type exists yet.
    for (int i=0;i<UnitTypes.Count();++i) {
        const auto ext=Extension::Fetch(UnitTypes[i]);
        crc=AttackMoveLogic::Mix(crc,ext->CanAttackMove);
        crc=AttackMoveLogic::Mix(crc,ext->AttackMoveScanInterval);
        const int acquire=AttackMoveConfig::Resolve_Percent(ext->AttackMoveAcquireRangePercent,cfg.AcquireRangePercent);
        const int retain=AttackMoveConfig::Effective_Retain(acquire,
            AttackMoveConfig::Resolve_Percent(ext->AttackMoveRetainRangePercent,cfg.RetainRangePercent));
        crc=AttackMoveLogic::Mix(crc,acquire);
        crc=AttackMoveLogic::Mix(crc,retain);
        crc=AttackMoveLogic::Mix(crc,AttackMoveConfig::Resolve_Support(ext->AttackMoveSupportEnabled,cfg.SupportEnabled));
    }
    for (int i=0;i<InfantryTypes.Count();++i) {
        const auto ext=Extension::Fetch(InfantryTypes[i]);
        crc=AttackMoveLogic::Mix(crc,ext->CanAttackMove);
        crc=AttackMoveLogic::Mix(crc,ext->AttackMoveScanInterval);
        const int acquire=AttackMoveConfig::Resolve_Percent(ext->AttackMoveAcquireRangePercent,cfg.AcquireRangePercent);
        const int retain=AttackMoveConfig::Effective_Retain(acquire,
            AttackMoveConfig::Resolve_Percent(ext->AttackMoveRetainRangePercent,cfg.RetainRangePercent));
        crc=AttackMoveLogic::Mix(crc,acquire);
        crc=AttackMoveLogic::Mix(crc,retain);
        crc=AttackMoveLogic::Mix(crc,AttackMoveConfig::Resolve_Support(ext->AttackMoveSupportEnabled,cfg.SupportEnabled));
    }
    std::vector<TechnoClass*> units;
    for (int i=0;i<Technos.Count();++i) {
        auto unit=Technos[i];
        if (unit && (unit->RTTI==RTTI_UNIT || unit->RTTI==RTTI_INFANTRY)) units.push_back(unit);
    }
    std::sort(units.begin(),units.end(),[](auto a,auto b) {
        return static_cast<unsigned>(TargetClass(a).Encode()) < static_cast<unsigned>(TargetClass(b).Encode());
    });
    for (auto unit:units) {
        const auto ext=Extension::Fetch(unit);
        crc=AttackMoveLogic::Mix(crc,TargetClass(unit).Encode());
        crc=AttackMoveLogic::State_CRC(crc,ext->IsAttackMoving,ext->AttackMoveDestination.X,
            ext->AttackMoveDestination.Y,ext->AttackMoveDestination.Z,ext->ThreatScanCadenceTimer,
            ext->AttackMoveCombatPending,ext->AttackMoveRetries,ext->AttackMoveRetryTimer);
    }
    return crc;
}
static void Move(FootClass* unit, const Coord& destination) {
    unit->Assign_Target(nullptr);
    unit->Assign_Destination(&Map[destination]);
    unit->Assign_Mission(MISSION_MOVE);
    unit->Commence();
    unit->Shorten_Mission_Timer();
}
void Execute(const EventClassExt& event) {
    const auto ref = event.Data.AttackMove.Whom;
    const Cell cell(event.Data.AttackMove.Where.X,event.Data.AttackMove.Where.Y);
    // Resolve through live entries, never dereference an unchecked heap index.
    TechnoClass* unit = nullptr;
    for (int i=0;i<Technos.Count();++i) {
        if (Technos[i] && AttackMoveLogic::Same_Target<xTargetClass>(TargetClass(Technos[i]),ref)) { unit=Technos[i]; break; }
    }
    if (!Movable(unit) || !Controlled(unit,event.ID) || !Map.In_Radar(cell)) return;
    const auto ext = Extension::Fetch(unit);
    Clear(ext);
    // Unarmed movers still receive ordinary movement; healing weapons use support targeting.
    if (!Eligible(unit)) {
        static_cast<FootClass*>(unit)->Clear_Navigation_List();
        Move(static_cast<FootClass*>(unit), Map[cell].Center_Coord());
        return;
    }
    ext->IsAttackMoving = true;
    ext->AttackMoveRetries = 2;
    ext->AttackMoveRetryTimer = static_cast<int>(Frame);
    ext->AttackMoveDestination = Map[cell].Center_Coord();
    ext->ThreatScanCadenceTimer = static_cast<int>(Frame);
    ext->HasOpportunityFireTarget = false;
    static_cast<FootClass*>(unit)->Clear_Navigation_List();
    Move(static_cast<FootClass*>(unit),ext->AttackMoveDestination);
    Tick(ext);
}
void Cancel_Replaced_Order(const EventClassExt& event) {
    switch (event.Type) {
    case EVENT_MEGAMISSION: case EVENT_MEGAMISSION_F: case EVENT_IDLE:
    case EVENT_SCATTER: case EVENT_DEPLOY: case EVENT_ARCHIVE: break;
    default: return;
    }
    const auto native = reinterpret_cast<const EventClass*>(&event);
    for (int i=0;i<Technos.Count();++i) {
        auto unit = Technos[i];
        if (unit && AttackMoveLogic::Same_Target<xTargetClass>(TargetClass(unit),native->Data.Target.Whom) && Controlled(unit,event.ID)) {
            Clear(Extension::Fetch(unit)); return;
        }
    }
}
static bool Valid_Target(TechnoClass* unit, TechnoClass* target, bool retaining=false) {
    if (!target || target==unit || !target->IsActive || target->IsInLimbo || target->Strength<=0 ||
        !target->House || !target->TClass || !target->TClass->IsLegalTarget || target->TClass->IsInvisible)
        return false;
    const bool support=Support_Eligible(unit);
    const bool allied=unit->House->Is_Ally(target->House);
    if (support) {
        if (!allied || target->Get_Health_Ratio()>=1.0 || target->In_Air()) return false;
        bool infantry=false,vehicle=true;
        if (unit->RTTI==RTTI_INFANTRY) {
            const auto type=Extension::Fetch(static_cast<InfantryClass*>(unit)->Class);
            infantry=!type->IsMechanic || type->IsOmniHealer;
            vehicle=type->IsMechanic || type->IsOmniHealer;
        }
        if (!((infantry && target->RTTI==RTTI_INFANTRY) ||
              (vehicle && (target->RTTI==RTTI_UNIT || target->RTTI==RTTI_AIRCRAFT)))) return false;
    } else {
        const bool mobile=target->RTTI==RTTI_UNIT || target->RTTI==RTTI_INFANTRY || target->RTTI==RTTI_AIRCRAFT;
        const bool defense=RuleExtension->AttackMoveSettings.TargetArmedBuildings &&
            target->RTTI==RTTI_BUILDING && Has_Attack_Weapon(target);
        const bool civilian=target->RTTI==RTTI_INFANTRY && static_cast<InfantryClass*>(target)->Class->IsCivilian;
        // Passive houses also own hostile wildlife. Keep harmless neutrals and
        // civilian infantry protected, but allow armed non-allied creatures.
        const bool passive=target->House->Class && target->House->Class->IsMultiplayPassive;
        if (!target->House->Class || !AttackMoveLogic::Enemy_Unit(mobile || defense,
            allied,passive && (!RuleExtension->AttackMoveSettings.TargetArmedWildlife ||
                !Has_Attack_Weapon(target)),civilian)) return false;
    }
    if (target->Cloak != UNCLOAKED && !Map[target->Center_Coord()].Sensed_By(unit->House->HeapID)) return false;
    const auto slot=unit->What_Weapon_Should_I_Use(target);
    const auto info=unit->Get_Weapon(slot);
    if (!info || !info->Weapon) return false;
    const auto weapon=info->Weapon;
    if (!weapon->Bullet || !weapon->WarheadPtr || (support ? weapon->Attack>=0 : !Deals_Damage(weapon)) ||
        (target->In_Air() ? !weapon->Bullet->IsAntiAircraft : !weapon->Bullet->IsAntiGround) ||
        !Verses::Get_PassiveAcquire(target->TClass->Armor,weapon->WarheadPtr) ||
        !Verses::Get_ForceFire(target->TClass->Armor,weapon->WarheadPtr) ||
        Verses::Get_Modifier(target->TClass->Armor,weapon->WarheadPtr)<=0 || unit->Ammo==0) return false;
    // Percentages scale the selected native weapon range. Keep native in-range
    // targets eligible, including engine bonuses; missions still govern firing.
    if (!unit->In_Range_Of(target,slot)) {
        const auto origin=unit->Center_Coord(), coord=target->Center_Coord();
        const std::int64_t dx=std::int64_t(coord.X)-origin.X,dy=std::int64_t(coord.Y)-origin.Y;
        const auto type=Extension::Fetch(unit->TClass);
        const auto& cfg=RuleExtension->AttackMoveSettings;
        const int acquire=AttackMoveConfig::Resolve_Percent(type->AttackMoveAcquireRangePercent,cfg.AcquireRangePercent);
        const int retain=AttackMoveConfig::Effective_Retain(acquire,
            AttackMoveConfig::Resolve_Percent(type->AttackMoveRetainRangePercent,cfg.RetainRangePercent));
        if (!AttackMoveConfig::Within_Radius(dx,dy,unit->Weapon_Range(slot),retaining ? retain : acquire)) return false;
    }
    switch (unit->Can_Fire(target,slot)) {
    case FIRE_OK: case FIRE_REARM: case FIRE_FACING: case FIRE_ROTATING:
    case FIRE_MOVING: case FIRE_CLOAKED: case FIRE_BUSY: case FIRE_RANGE: return true;
    default: return false;
    }
}
static TechnoClass* Find_Target(TechnoClass* unit) {
    TechnoClass* best = nullptr;
    std::int64_t best_distance = std::numeric_limits<std::int64_t>::max();
    unsigned best_id = 0;
    const auto origin = unit->Center_Coord();
    for (int i=0;i<Technos.Count();++i) {
        auto target = Technos[i];
        if (!Valid_Target(unit,target)) continue;
        const auto coord = target->Center_Coord();
        const std::int64_t dx = std::int64_t(coord.X)-origin.X, dy = std::int64_t(coord.Y)-origin.Y;
        const auto distance = dx*dx+dy*dy;
        const unsigned id = TargetClass(target).Encode();
        if (AttackMoveLogic::Better_Target(distance,id,best_distance,best_id)) {
            best=target; best_distance=distance; best_id=id;
        }
    }
    return best;
}
void Tick(TechnoClassExtension* ext) {
    if (!ext->IsAttackMoving) return;
    auto unit = static_cast<FootClass*>(ext->This());
    const bool eligible = Eligible(unit);
    const bool suspended = eligible && (unit->EMPFramesRemaining>0 || unit->Is_Immobilized());
    const bool allowed = unit->CurrentMission==MISSION_MOVE || unit->CurrentMission==MISSION_ATTACK || unit->CurrentMission==MISSION_GUARD;
    if (!eligible || !allowed) { Clear(ext); return; }
    if (suspended) return;
    TechnoClass* target = nullptr;
    // TarCom is swizzled/detached by the engine. Do not retain an additional pointer.
    if (Is_Target_Techno(unit->TarCom)) target=static_cast<TechnoClass*>(unit->TarCom);
    if (!Valid_Target(unit,target,true)) target=nullptr;
    // Native detachment can clear TarCom before this tick. Attack still identifies loss.
    if (!target && (unit->CurrentMission==MISSION_ATTACK || ext->AttackMoveCombatPending))
        ext->ThreatScanCadenceTimer=static_cast<int>(Frame);
    if (!target && AttackMoveLogic::Scan_Due(ext->ThreatScanCadenceTimer,
            Extension::Fetch(unit->TClass)->AttackMoveScanInterval,static_cast<unsigned>(Frame)))
        target=Find_Target(unit);
    const auto here = unit->Position.As_Cell();
    const auto dest = ext->AttackMoveDestination.As_Cell();
    // Combat can leave the native unit in Guard with no target. Continue the route
    // without consuming a retry reserved for failed pathfinding.
    if (!target && ext->AttackMoveCombatPending && !(here==dest)) {
        ext->AttackMoveCombatPending = 0;
        Move(unit,ext->AttackMoveDestination);
        return;
    }
    // Retry an abandoned route at most twice, separated by simulation time.
    // Do not reset the budget on movement or combat; unreachable routes must terminate.
    if (!target && !(here==dest) && unit->CurrentMission==MISSION_GUARD && unit->NavCom==nullptr && ext->AttackMoveRetries>0) {
        if (AttackMoveLogic::Scan_Due(ext->AttackMoveRetryTimer,30,static_cast<unsigned>(Frame))) {
            --ext->AttackMoveRetries;
            Move(unit,ext->AttackMoveDestination);
        }
        return;
    }
    const auto step = AttackMoveLogic::Decide({true,eligible,suspended,allowed,target!=nullptr,
        unit->CurrentMission==MISSION_ATTACK,here==dest,unit->CurrentMission==MISSION_GUARD && unit->NavCom==nullptr});
    switch (step) {
    case AttackMoveLogic::Step::Engage:
        ext->AttackMoveCombatPending = 1;
        if (unit->TarCom!=target || unit->CurrentMission!=MISSION_ATTACK) {
            unit->Assign_Destination(nullptr);
            unit->Assign_Target(target);
            unit->Assign_Mission(MISSION_ATTACK);
            unit->Commence();
            unit->Shorten_Mission_Timer();
        }
        break;
    case AttackMoveLogic::Step::Resume:
        Move(unit,ext->AttackMoveDestination); break;
    case AttackMoveLogic::Step::Finish:
        Clear(ext);
        unit->Assign_Target(nullptr);
        unit->Assign_Destination(nullptr);
        unit->Assign_Mission(MISSION_GUARD);
        unit->Commence();
        break;
    case AttackMoveLogic::Step::Cancel: Clear(ext); break;
    default: break;
    }
}
}
