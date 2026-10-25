#pragma once
#include <windows.h>
#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace window_placement {
struct Placement { RECT rect{}; bool separated = false; };

inline long long OverlapArea(const RECT& a, const RECT& b) {
    return static_cast<long long>(std::max(0L, std::min(a.right, b.right) - std::max(a.left, b.left))) *
        std::max(0L, std::min(a.bottom, b.bottom) - std::max(a.top, b.top));
}

// Work areas are ordered with the overlay's monitor first. Keep the saved
// overlay fixed; move only the settings window, including on negative monitors.
inline Placement BesideOverlay(const RECT& overlay, SIZE size, const std::vector<RECT>& workAreas, LONG gap = 16) {
    Placement fallback{};
    long long leastOverlap = std::numeric_limits<long long>::max();
    const RECT clearance{overlay.left - gap, overlay.top - gap, overlay.right + gap, overlay.bottom + gap};
    for (const RECT& work : workAreas) {
        auto clampX = [&](LONG x) { return std::clamp(x, work.left, std::max(work.left, work.right - size.cx)); };
        auto clampY = [&](LONG y) { return std::clamp(y, work.top, std::max(work.top, work.bottom - size.cy)); };
        const std::array<POINT, 8> candidates{{
            {clampX(overlay.left), overlay.bottom + gap}, {overlay.right + gap, clampY(overlay.top)},
            {overlay.left - size.cx - gap, clampY(overlay.top)}, {clampX(overlay.left), overlay.top - size.cy - gap},
            {work.left, work.top}, {clampX(work.right), work.top},
            {work.left, clampY(work.bottom)}, {clampX(work.right), clampY(work.bottom)}
        }};
        for (const POINT& point : candidates) {
            RECT rect{clampX(point.x), clampY(point.y), clampX(point.x) + size.cx, clampY(point.y) + size.cy};
            const bool fits = rect.right <= work.right && rect.bottom <= work.bottom;
            const auto overlap = OverlapArea(rect, overlay);
            if (fits && OverlapArea(rect, clearance) == 0) return {rect, true};
            if (overlap < leastOverlap) { leastOverlap = overlap; fallback = {rect, false}; }
        }
    }
    return fallback;
}
}
