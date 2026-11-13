#pragma once
#include <cmath>
#include <sstream>
#include <string>

inline bool PerfHarnessParseZoom(const std::string& command, std::string& id, float& delta)
{
    std::istringstream input(command);
    std::string op, extra;
    if (!(input >> op >> id >> delta) || op != "ZOOM" || (input >> extra) ||
        id.empty() || id.size() > 32 || !std::isfinite(delta) || delta == 0.0f ||
        delta < -1.0f || delta > 1.0f) return false;
    for (char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    return true;
}

inline bool PerfHarnessZoomReached(bool sameArea, float actual, float target)
{
    return sameArea && std::isfinite(actual) && std::isfinite(target) &&
           std::fabs(actual - target) <= 0.0001f;
}
