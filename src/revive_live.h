#pragma once
#include "revive_auxiliary.h"
#include <deque>
#include <cstdint>
#include <future>
#include <memory>

namespace revive_live {
constexpr double WindowMs = 8000;
constexpr double RefreshMs = 500;
constexpr std::size_t MaxPresses = 4096;

struct Sample {
    chartcalc::Chart chart;
    double durationMs = 0;
    std::wstring status;
};

// UI-thread input history. Only press times become note heads. Release timing
// is tracked for input/session state, never used to invent chart LN tails.
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
}
