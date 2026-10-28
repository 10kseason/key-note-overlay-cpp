#pragma once
#include "revive_auxiliary.h"
#include <deque>
#include <cstdint>
#include <future>
#include <memory>

namespace revive_live {
constexpr double WindowMs = 8000;
constexpr double RefreshMs = 500;
constexpr double HoldThresholdMs = 170;
constexpr std::size_t MaxPresses = 4096;

struct Sample {
    chartcalc::Chart chart;
    double durationMs = 0;
    std::wstring status;
};

// UI-thread input history. Presses held at least HoldThresholdMs become LN
// estimates; this describes physical input, not the selected chart's notes.
class Window {
public:
    void Reset(int lanes);
    void Key(int lane, bool down, double nowMs);
    void ReleaseAll(double nowMs);
    Sample Snapshot(double nowMs, int analysisKeys = 0);
    std::size_t Size() const { return presses_.size(); }
    std::uint64_t Session() const { return session_; }
private:
    struct Press { int lane; double start, end; };
    void Prune(double nowMs);
    std::deque<Press> presses_;
    std::array<double, 256> held_{};
    std::array<bool, 256> observed_{};
    int lanes_ = 0;
    double started_ = -1, last_ = -1, limitedUntil_ = -1;
    std::uint64_t session_ = 0;
};

// At most one immutable snapshot is calculated at once. Reset/disable cancel
// it without waiting on the UI thread; generation checks reject stale results.
class Meter {
public:
    ~Meter();
    void Reset(int lanes);
    void Key(int lane, bool down, double nowMs);
    void ReleaseAll(double nowMs) { window_.ReleaseAll(nowMs); }
    void Update(double nowMs, bool enabled, int analysisKeys, bool pending, bool conflict, bool limited);
    const revive_aux::Summary& Current() const { return summary_; }
    bool Busy() const { return job_.valid(); }
    void Stop();
private:
    void Invalidate(const std::wstring& status);
    Window window_;
    revive_aux::Summary summary_;
    std::future<revive_aux::Summary> job_;
    std::shared_ptr<std::atomic<bool>> cancelled_;
    std::uint64_t generation_ = 0, jobGeneration_ = 0;
    int analysisKeys_ = 0;
    double next_ = 0;
    std::wstring gate_;
};

// Axes of a session's strongest stretch, kept after the 8-second window
// expires. Mirrors pattern::PatternSummary: only an idle transition publishes,
// and an input limit or key conflict withholds the whole session.
class CompletedSkills {
public:
    void Observe(const revive_aux::Summary& live, bool idle, bool counted, bool blocked, double strength);
    void Reset() { peak_ = completed_ = {}; best_ = 0; blocked_ = counted_ = false; }
    const revive_aux::Summary& Completed() const { return completed_; }
private:
    revive_aux::Summary peak_, completed_;
    double best_ = 0;
    bool blocked_ = false, counted_ = false;
};
}
