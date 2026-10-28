#pragma once
#include "analysis_key_mode.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace overlay {
struct LaneDisplay {
    int registered = 0, visible = 0, width = 0, slotWidth = 0;
    double pitch = 0;
    bool compact = false;
    std::array<int, 256> index{};
    bool Visible(int lane) const { return lane >= 0 && lane < registered && index[lane] >= 0; }
    int Center(int lane, int canvasWidth) const {
        if (!Visible(lane)) return -1;
        return static_cast<int>(std::round(canvasWidth / 2.0 + pitch * (index[lane] - (visible - 1) / 2.0)));
    }
};

// Keep pitch/slot width from the full configured canvas. Folding only removes
// unused slots; it never rescales notes, colors, labels or physical key indices.
inline LaneDisplay MakeLaneDisplay(int registered, int fullWidth, int spacing,
    const pattern::AnalysisKeySelector& selector, bool allowCompact) {
    LaneDisplay result;
    result.registered = std::clamp(registered, 0, 256);
    result.index.fill(-1);
    const bool selected = allowCompact && selector.CompactReady() && selector.Keys() < result.registered;
    for (int lane = 0; lane < result.registered; ++lane)
        if (!selected || selector.ObservedLane(lane)) result.index[lane] = result.visible++;
    result.compact = selected && result.visible > 0 && result.visible < result.registered;
    const int margin = std::max(24, fullWidth / 24);
    const int area = std::max(1, fullWidth - 2 * margin);
    result.slotWidth = result.registered ? std::max(1, area / result.registered) : area;
    result.pitch = result.registered > 1 ? std::max(1, area - result.slotWidth) *
        (std::clamp(spacing, 50, 160) / 100.0) / (result.registered - 1) : 0;
    result.width = fullWidth;
    if (result.compact) result.width = std::min(fullWidth, std::max(240,
        static_cast<int>(std::round(fullWidth - result.pitch * (result.registered - result.visible)))));
    return result;
}
}
