// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <initializer_list>

namespace AttackMoveLogic {
constexpr unsigned Protocol = 0x54534103u;
constexpr bool Enemy_Unit(bool mobile, bool allied, bool passive_house, bool civilian) {
    return mobile && !allied && !passive_house && !civilian;
}
enum class Step { Inactive, Cancel, Suspend, Engage, Resume, Finish, Transit };
struct Input {
    bool active, eligible, suspended, allowed_mission;
    bool target_valid, attack_mission, arrived, movement_ended;
};
constexpr Step Decide(Input s) {
    if (!s.active) return Step::Inactive;
    if (!s.eligible || !s.allowed_mission) return Step::Cancel;
    if (s.suspended) return Step::Suspend;
    if (s.target_valid) return Step::Engage;
    if (s.arrived) return Step::Finish;
    if (s.attack_mission) return Step::Resume;
    if (s.movement_ended) return Step::Finish;
    return Step::Transit;
}
inline bool Scan_Due(int& deadline, int interval, unsigned frame) {
    // The event and AI wrapper can run in the same frame. A deadline avoids double ticking.
    const unsigned remaining = static_cast<unsigned>(deadline)-frame;
    if (remaining != 0 && remaining <= 0x7fffffffu) return false;
    deadline = static_cast<int>(frame + static_cast<unsigned>(interval > 0 ? interval : 15));
    return true;
}
constexpr bool Better_Target(std::int64_t distance, unsigned id, std::int64_t best_distance, unsigned best_id) {
    return distance < best_distance || (distance == best_distance && id < best_id);
}
constexpr bool Queue_Fits(int count, int requested, int capacity) {
    return count >= 0 && requested > 0 && count <= capacity && requested <= capacity-count;
}
constexpr unsigned Handshake(unsigned scenario) { return scenario ^ Protocol; }
constexpr unsigned Mix(unsigned crc, unsigned value) {
    return ((crc << 1) | (crc >> 31)) + value;
}
inline unsigned State_CRC(unsigned crc, bool active, int x, int y, int z, int deadline,
                          int combat_pending, int retries=0, int retry_deadline=0) {
    for (unsigned value : {static_cast<unsigned>(active),static_cast<unsigned>(x),static_cast<unsigned>(y),
                          static_cast<unsigned>(z),static_cast<unsigned>(deadline),static_cast<unsigned>(combat_pending),
                          static_cast<unsigned>(retries),
                          static_cast<unsigned>(retry_deadline)})
        crc=Mix(crc,value);
    return crc;
}
template<class Target>
bool Same_Target(const Target& a, const Target& b) {
    // TS++ xTargetClass::operator== is not const-correct; implicit conversion loses the ID.
    return a.Kind()==b.Kind() && a.Value()==b.Value();
}
}
