#include "revive_auxiliary.h"
#include "revive_config.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace revive_aux {
Summary Analyze(const chartcalc::Chart& chart, const std::atomic<bool>* cancelled,
    double observationMs, std::chrono::milliseconds timeout) {
    Summary result;
    if (cancelled && cancelled->load()) { result.status = L"취소"; return result; }
    if (!revive::SupportsKeys(chart.keys)) { result.status = L"1~18K 지원"; return result; }
    if (chart.notes.empty()) { result.status = L"노트 없음"; return result; }
    if (chart.notes.size() > revive::MaxEvents) { result.status = L"노트 한도 · 보류"; return result; }
    try {
        for (const auto& n : chart.notes)
            if (n.lane < 0 || n.lane >= chart.keys || !std::isfinite(n.startMs) || !std::isfinite(n.endMs) ||
                n.startMs < 0 || n.endMs < n.startMs || n.endMs > 7200000)
                throw std::runtime_error("revive invalid input");
        auto ordered = chart.notes;
        std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
            return a.startMs != b.startMs ? a.startMs < b.startMs : a.lane < b.lane;
        });
        std::array<double, revive::MaxKeys> previousStart, previousEnd;
        previousStart.fill(-1); previousEnd.fill(-1);
        const double origin = ordered.front().startMs;
        double last = origin;
        std::vector<revive::Note> notes;
        notes.reserve(std::min(ordered.size() * 2, revive::MaxEvents));
        for (const auto& n : ordered) {
            if (cancelled && cancelled->load()) { result.status = L"취소"; return result; }
            if (n.startMs <= previousStart[n.lane] || n.startMs < previousEnd[n.lane])
                throw std::runtime_error("revive invalid input");
            previousStart[n.lane] = n.startMs; previousEnd[n.lane] = n.endMs;
            const bool hold = n.endMs > n.startMs;
            notes.push_back({(n.startMs - origin) / 1000.0, n.lane, hold ? 1 : 0});
            if (hold) notes.push_back({(n.endMs - origin) / 1000.0, n.lane, 2});
            if (notes.size() > revive::MaxEvents) { result.status = L"노트 한도 · 보류"; return result; }
            last = std::max(last, n.endMs);
        }
        // Live snapshots supply their observation duration, including silence.
        // File analysis continues to use the full chart's first-to-last span.
        const double durationMs = observationMs >= 0 ? observationMs : last - origin;
        if (!std::isfinite(durationMs) || durationMs < last - origin || durationMs > 7200000)
            throw std::runtime_error("revive invalid duration");
        const auto calculated = revive::calculate(std::move(notes), std::max(1.0, durationMs / 1000.0),
            revive::ConfigForKeys(chart.keys), cancelled, timeout);
        const char* axes[] = {"STREAM", "SPEED", "JACK", "CHORD", "TECH", "HOLD"};
        std::array<double, 6> skills{};
        for (std::size_t i = 0; i < skills.size(); ++i) {
            skills[i] = calculated.scalars.at(std::string("skill_radar.") + axes[i]);
            if (!std::isfinite(skills[i]) || skills[i] < 0 || skills[i] > 100)
                throw std::runtime_error("revive invalid result");
        }
        result.ready = true; result.skills = skills;
    } catch (const std::exception& error) {
        const std::string reason = error.what();
        result.status = reason == "revive cancelled" ? L"취소" :
            reason == "revive time limit" ? L"시간 한도 · 보류" :
            reason == "revive density limit" ? L"밀도 한도 · 보류" : L"계산 보류";
    }
    return result;
}

namespace {
const wchar_t* const SkillNames[] = {L"스트림", L"속도", L"연타", L"동시치기", L"테크닉", L"홀드"};
}

std::wstring SkillName(std::size_t index) { return index < 6 ? SkillNames[index] : L""; }

bool SkillAvailable(const Summary& summary, std::size_t index) {
    return index < summary.skills.size() && summary.ready && (index != 5 || summary.holdKnown);
}

std::wstring SkillValue(const Summary& summary, std::size_t index) {
    if (index >= summary.skills.size()) return {};
    return SkillAvailable(summary,index) ? std::to_wstring(static_cast<int>(std::round(summary.skills[index]))) : L"—";
}

std::wstring SkillText(const Summary& summary, std::size_t index) {
    return index < 6 ? SkillName(index) + L" " + SkillValue(summary,index) : L"";
}

std::wstring Label(const Summary& summary, bool compact) {
    const std::wstring prefix = compact ? L"CPP · " : L"Revive CPP · 요구 스킬: ";
    if (!summary.ready) return prefix + summary.status;
    std::array<std::size_t, 6> order{0,1,2,3,4,5};
    // Keep ties stable; ranking describes chart traits, never the player's skill.
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return summary.skills[a] > summary.skills[b]; });
    std::wstring names;
    int shown = 0;
    for (const auto index : order) {
        if (shown == 2) break;
        if (!SkillAvailable(summary,index) || summary.skills[index] <= 0) continue;
        if (!names.empty()) names += L" / ";
        names += SkillNames[index];
        ++shown;
    }
    return prefix + (names.empty() ? L"특징 부족" : names);
}

std::wstring Details(const Summary& summary) {
    if (!summary.ready) return L"보조/EZ · 1~18K · 고정 배치";
    std::wstring text;
    for (std::size_t i = 0; i < summary.skills.size(); ++i) {
        if (i) text += i == 3 ? L"\n" : L"  ·  ";
        text += SkillText(summary,i);
    }
    return text + (summary.holdKnown ? L"  (특징 강도 0~100 · 보조/EZ)" : L"  (입력 특징 0~100 · 홀드 미확인)");
}
}
