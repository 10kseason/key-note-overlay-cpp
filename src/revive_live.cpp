#include "revive_live.h"
#include <algorithm>
#include <cmath>

namespace revive_live {
void Window::Reset(int lanes) {
    ++session_;
    lanes_ = std::clamp(lanes, 0, 256); presses_.clear(); held_.fill(-1); observed_.fill(false);
    started_ = last_ = limitedUntil_ = -1;
}
void Window::Prune(double nowMs) {
    const double left = nowMs - WindowMs;
    presses_.erase(std::remove_if(presses_.begin(), presses_.end(),
        [left](const Press& p) { return p.end >= 0 && p.end < left; }), presses_.end());
}
void Window::Key(int lane, bool down, double nowMs) {
    if (lane < 0 || lane >= lanes_ || !std::isfinite(nowMs) || nowMs < 0 || nowMs < last_) return;
    if (down == (held_[lane] >= 0)) return;
    const bool anyHeld = std::any_of(held_.begin(), held_.end(), [](double t) { return t >= 0; });
    if (down && !anyHeld && last_ >= 0 && nowMs - last_ >= 10000) Reset(lanes_);
    last_ = nowMs;
    Prune(nowMs);
    if (down) {
        if (presses_.size() >= MaxPresses) {
            presses_.clear(); held_.fill(-1); limitedUntil_ = nowMs + WindowMs;
        }
        if (started_ < 0) started_ = nowMs;
        observed_[lane] = true;
        held_[lane] = nowMs; presses_.push_back({lane, nowMs, -1});
    } else {
        for (auto it = presses_.rbegin(); it != presses_.rend(); ++it) {
            if (it->lane == lane && it->end < 0) { it->end = nowMs; break; }
        }
        held_[lane] = -1;
    }
}
void Window::ReleaseAll(double nowMs) {
    for (int lane = 0; lane < lanes_; ++lane) if (held_[lane] >= 0) Key(lane, false, nowMs);
}
Sample Window::Snapshot(double nowMs, int analysisKeys) {
    const int keys = analysisKeys ? analysisKeys : lanes_;
    Sample sample; sample.chart.keys = keys;
    if (!revive::SupportsKeys(keys)) { sample.status = L"1~18K 지원"; return sample; }
    std::array<int, 256> mapping{};
    if (keys == lanes_) {
        for (int lane = 0; lane < lanes_; ++lane) mapping[lane] = lane;
    } else if (lanes_ > keys) {
        // All selected lanes must be known before using their hand matrix.
        // Keep their registered order for the entire session, even after a
        // lane leaves the rolling window. Never pack only the latest notes.
        int count = 0;
        for (int lane = 0; lane < lanes_; ++lane) if (observed_[lane]) mapping[lane] = count++;
        if (count != keys) { sample.status = count > keys ? L"키 수 확인" : std::to_wstring(keys) + L"키 배치 확인 중"; return sample; }
    } else { sample.status = L"배치/분석 키 확인"; return sample; }
    if (!std::isfinite(nowMs) || nowMs < 0 || nowMs < last_) { sample.status = L"시간 확인"; return sample; }
    Prune(nowMs);
    if (nowMs < limitedUntil_) { sample.status = L"입력 한도 · 보류"; return sample; }
    const double left = std::max(started_, nowMs - WindowMs);
    sample.durationMs = started_ < 0 ? 0 : nowMs - left;
    for (const auto& press : presses_) {
        const double end = press.end < 0 ? nowMs : press.end;
        // Classify before clipping: an already-qualified hold may have less
        // than 170ms left inside the window. Open keys use only observed time.
        const bool hold = end - press.start >= HoldThresholdMs;
        if (press.start > nowMs || (hold ? end <= left : press.start < left)) continue;
        const double start = std::max(press.start, left) - left;
        sample.chart.notes.push_back({mapping[press.lane], start, hold ? end - left : start});
    }
    if (sample.chart.notes.empty()) sample.status = L"입력 대기";
    else if (sample.chart.notes.size() < 8 || sample.durationMs < 1500) sample.status = L"입력 수집 중";
    return sample;
}

Meter::~Meter() { Stop(); }
void Meter::Invalidate(const std::wstring& status) {
    if (cancelled_) cancelled_->store(true);
    ++generation_; summary_ = {}; summary_.status = status; next_ = 0;
}
void Meter::Reset(int lanes) {
    analysisKeys_ = 0; window_.Reset(lanes); gate_.clear(); Invalidate(L"입력 대기");
}
void Meter::Key(int lane, bool down, double nowMs) {
    const auto session = window_.Session();
    window_.Key(lane, down, nowMs);
    if (window_.Session() != session) { gate_.clear(); Invalidate(L"입력 수집 중"); }
}
void Meter::Update(double nowMs, bool enabled, int analysisKeys, bool pending, bool conflict, bool limited) {
    if (analysisKeys != analysisKeys_) {
        analysisKeys_ = analysisKeys; gate_.clear(); Invalidate(L"입력 수집 중");
    }
    Sample sample;
    std::wstring gate = !enabled ? L"OFF" : conflict ? L"키 수 확인" : pending ? L"키 추정 중" :
        limited ? L"입력 한도 · 보류" : L"";
    if (gate.empty()) { sample = window_.Snapshot(nowMs, analysisKeys); gate = sample.status; }
    if (gate != gate_) { Invalidate(gate); gate_ = gate; }
    if (job_.valid() && job_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        revive_aux::Summary completed;
        try { completed = job_.get(); }
        catch (const std::exception&) { completed.status = L"계산 보류"; }
        if (jobGeneration_ == generation_ && gate.empty()) summary_ = std::move(completed);
        cancelled_.reset();
    }
    if (!gate.empty()) { summary_.ready = false; summary_.status = gate; return; }
    if (job_.valid() || nowMs < next_) return;
    next_ = nowMs + RefreshMs;
    cancelled_ = std::make_shared<std::atomic<bool>>(false);
    jobGeneration_ = generation_;
    try {
        job_ = std::async(std::launch::async, [sample = std::move(sample), cancel = cancelled_] {
            return revive_aux::Analyze(sample.chart, cancel.get(), sample.durationMs, std::chrono::milliseconds(100));
        });
        if (!summary_.ready) summary_.status = L"계산 중";
    } catch (const std::exception&) { summary_ = {}; summary_.status = L"계산 보류"; }
}
void Meter::Stop() {
    if (cancelled_) cancelled_->store(true);
    if (job_.valid()) job_.wait();
}

void CompletedSkills::Observe(const revive_aux::Summary& live, bool idle, bool counted, bool blocked, double strength) {
    if (idle) {
        // A newly completed rating must not inherit axes from an older play
        // if every auxiliary calculation failed. Short unqualified input,
        // however, leaves both previous results intact.
        if (counted_ && !blocked_) completed_ = peak_;
        peak_ = {}; best_ = 0; blocked_ = counted_ = false;
        return;
    }
    if (blocked) blocked_ = true;
    counted_ = counted_ || counted;
    if (!counted_ || blocked_ || !live.ready) return;
    // An asynchronous result may arrive after the live rating is retained.
    // Accept that first result, but only fresh ratings can replace the peak.
    if (!peak_.ready || (counted && strength >= best_)) { peak_ = live; best_ = strength; }
}
}
