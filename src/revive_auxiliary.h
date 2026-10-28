#pragma once
#include "chart_reader.h"
#include "revive_native.h"
#include <array>
#include <atomic>
#include <chrono>
#include <string>

namespace revive_aux {
struct Summary {
    bool ready = false;
    // STREAM, SPEED, JACK, CHORD, TECH, HOLD: model-normalized chart traits.
    std::array<double, 6> skills{};
    std::wstring status;
    // Availability under the caller's model: file LN tails or the explicit
    // live hold-duration rule. Unavailable HOLD is shown as a dash.
    bool holdKnown = true;
};
// Pure 1K-18K calculation. observationMs < 0 uses the whole chart's duration;
// live callers supply a bounded observation window and shorter time budget.
// Never accesses a process or changes the main difficulty estimator.
Summary Analyze(const chartcalc::Chart& chart, const std::atomic<bool>* cancelled = nullptr,
    double observationMs = -1, std::chrono::milliseconds timeout = std::chrono::milliseconds(1500));
std::wstring SkillText(const Summary& summary, std::size_t index);
std::wstring SkillName(std::size_t index);
bool SkillAvailable(const Summary& summary, std::size_t index);
std::wstring SkillValue(const Summary& summary, std::size_t index);
std::wstring Label(const Summary& summary, bool compact = false);
std::wstring Details(const Summary& summary);
}
