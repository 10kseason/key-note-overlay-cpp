#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace pattern {
// 0 preserves the registered layout, -1 infers once per session, 4..10 fixes
// the analysis basis. Physical input/visual lanes are never changed here.
inline int NormalizeAnalysisKeyMode(int mode) {
    return mode == -1 || (mode >= 4 && mode <= 10) ? mode : 0;
}
class AnalysisKeySelector {
public:
    void Reset(int registered, int mode) {
        registered_ = std::clamp(registered, 1, 256);
        mode_ = NormalizeAnalysisKeyMode(mode);
        ClearSession();
    }
    void Key(int lane, bool down, double timeMs) {
        if (lane < 0 || lane >= registered_ || !std::isfinite(timeMs) || timeMs < 0 || timeMs < last_) return;
        if (down) {
            if (held_[lane]) return;
            if (heldCount_ == 0 && last_ >= 0 && timeMs - last_ >= 10000) ClearSession();
            if (first_ < 0) first_ = timeMs;
            if (!counts_[lane]) { ++observed_; lastNew_ = timeMs; }
            counts_[lane] = std::min(counts_[lane] + 1, 2);
            presses_ = std::min(presses_ + 1, 32);
            held_[lane] = true;
            ++heldCount_;
        } else {
            if (!held_[lane]) return;
            held_[lane] = false;
            --heldCount_;
        }
        last_ = timeMs;
        Update(timeMs);
    }
    void ReleaseAll(double timeMs) {
        if (heldCount_ && std::isfinite(timeMs)) last_ = std::max(last_, timeMs);
        held_.fill(false); heldCount_ = 0;
    }
    void Update(double nowMs) {
        if (mode_ != -1 || locked_ || first_ < 0 || !std::isfinite(nowMs) || nowMs < last_) return;
        if (observed_ < 4 || observed_ > 10 || presses_ < 32 || nowMs - first_ < 8000 || nowMs - lastNew_ < 3000) return;
        for (int count : counts_) if (count == 1) return;
        locked_ = observed_;
    }
    int Keys() const { return mode_ == -1 ? (locked_ ? locked_ : registered_) : mode_ > 0 ? mode_ : registered_; }
    int Observed() const { return observed_; }
    bool Pending() const { return mode_ == -1 && !locked_; }
    // A late extra lane invalidates the fixed inference; do not silently ignore
    // that key or switch comparison scales in the middle of a reported session.
    bool Conflict() const { return mode_ != 0 && (observed_ > (Pending() ? 10 : Keys())); }
    bool ObservedLane(int lane) const { return lane >= 0 && lane < registered_ && counts_[lane] > 0; }
    bool CompactReady() const {
        if (mode_ == 0 || Pending() || Conflict() || observed_ != Keys()) return false;
        // Manual mode knows the count, but not the physical subset. Wait until
        // every selected lane has been observed twice before hiding anything.
        for (int count : counts_) if (count == 1) return false;
        return true;
    }
private:
    void ClearSession() {
        counts_.fill(0); held_.fill(false); heldCount_ = observed_ = presses_ = locked_ = 0;
        first_ = last_ = lastNew_ = -1;
    }
    int registered_ = 8, mode_ = 0, locked_ = 0, observed_ = 0, presses_ = 0, heldCount_ = 0;
    double first_ = -1, last_ = -1, lastNew_ = -1;
    std::array<int, 256> counts_{};
    std::array<bool, 256> held_{};
};
}
