// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
class TechnoClass;
class TechnoClassExtension;
class EventClassExt;
class Cell;
namespace AttackMove {
bool Eligible(TechnoClass* unit);
bool Arm();
bool Armed();
bool Targeting();
void Cancel_Input();
void Click(const Cell& cell);
void Execute(const EventClassExt& event);
void Cancel_Replaced_Order(const EventClassExt& event);
void Clear(TechnoClassExtension* extension);
void Tick(TechnoClassExtension* extension);
unsigned Simulation_CRC(unsigned native_crc);
}
