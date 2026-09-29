// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>

namespace AttackMoveConfig {
// Keep these aggregates trivial: Rules and type extensions restore their bytes
// before invoking NoInit constructors during save loading.
struct Settings {
    bool PersistentTargeting, RightClickDeselects;
    int AcquireRangePercent, RetainRangePercent;
    bool TargetArmedBuildings, TargetArmedWildlife, SupportEnabled;
};
struct TypeOverrides {
    int AcquireRangePercent, RetainRangePercent, SupportEnabled;
};
constexpr Settings Defaults() { return {false, true, 150, 300, true, true, true}; }
constexpr TypeOverrides Inherited() { return {-1, -1, -1}; }
constexpr bool Valid_Percent(int value) { return value >= 100 && value <= 1000; }
inline bool Apply_Percent(int& stored, bool present, int candidate, int fallback) {
    if (!present) return true;
    if (!Valid_Percent(candidate)) { stored = fallback; return false; }
    stored = candidate;
    return true;
}
inline bool Apply_Percent_Text(int& stored, bool present, const char* raw, int fallback) {
    if (!present) return true;
    if (!raw) { stored = fallback; return false; }
    while (std::isspace(static_cast<unsigned char>(*raw))) ++raw;
    char* end = nullptr;
    const long parsed = std::strtol(raw, &end, 10);
    while (end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end == raw || !end || *end || parsed < 100 || parsed > 1000) {
        stored = fallback;
        return false;
    }
    stored = static_cast<int>(parsed);
    return true;
}
template<typename Ini> inline bool Read_Percent(Ini& ini, const char* section,
        const char* key, int& stored, int fallback) {
    if (!ini.Is_Present(section, key)) return true;
    char raw[64] = {};
    const int length = ini.Get_String(section, key, "", raw, sizeof(raw));
    if (length >= static_cast<int>(sizeof(raw) - 1)) {
        stored = fallback;
        return false;
    }
    return Apply_Percent_Text(stored, true, raw, fallback);
}
constexpr int Resolve_Percent(int override_value, int inherited) {
    return override_value >= 0 ? override_value : inherited;
}
constexpr bool Resolve_Support(int override_value, bool inherited) {
    return override_value >= 0 ? override_value != 0 : inherited;
}
constexpr int Effective_Retain(int acquire, int retain) {
    return retain >= acquire ? retain : acquire;
}
inline bool Within_Radius(std::int64_t dx, std::int64_t dy, int native_range, int percent) {
    if (native_range <= 0 || !Valid_Percent(percent)) return false;
    const std::uint64_t radius = std::uint64_t(native_range) * percent / 100;
    // Coordinates normally fit easily, but use two 64-bit limbs so even the
    // largest integer inputs cannot overflow the squared-distance comparison.
    const auto magnitude = [](std::int64_t n) -> std::uint64_t {
        return n < 0 ? std::uint64_t(-(n + 1)) + 1 : std::uint64_t(n);
    };
    const std::uint64_t x = magnitude(dx), y = magnitude(dy);
    if (x > radius || y > radius) return false;
    struct Wide { std::uint64_t high, low; };
    const auto square = [](std::uint64_t n) -> Wide {
        const std::uint64_t low = n & 0xffffffffu, high = n >> 32;
        const std::uint64_t base = low * low;
        const std::uint64_t cross = high * low;
        const std::uint64_t lower = base + (cross << 33);
        return {high * high + (cross >> 31) + (lower < base), lower};
    };
    const auto x2 = square(x), y2 = square(y), r2 = square(radius);
    const std::uint64_t sum = x2.low + y2.low;
    const std::uint64_t carry = sum < x2.low;
    const std::uint64_t upper = x2.high + y2.high + carry;
    return upper < r2.high || (upper == r2.high && sum <= r2.low);
}
constexpr unsigned Mix(unsigned crc, unsigned value) {
    return ((crc << 1) | (crc >> 31)) + value;
}
inline unsigned Config_CRC(unsigned crc, const Settings& s) {
    crc = Mix(crc, s.PersistentTargeting);
    crc = Mix(crc, s.RightClickDeselects);
    crc = Mix(crc, static_cast<unsigned>(s.AcquireRangePercent));
    crc = Mix(crc, static_cast<unsigned>(s.RetainRangePercent));
    crc = Mix(crc, s.TargetArmedBuildings);
    crc = Mix(crc, s.TargetArmedWildlife);
    return Mix(crc, s.SupportEnabled);
}
}
