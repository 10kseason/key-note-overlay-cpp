#include "analysis.h"
#include "dan_rating.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <iomanip>
#include <locale>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace chartcalc {
std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string value(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), value.data(), size, nullptr, nullptr);
    return value;
}
std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring value(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), value.data(), size);
    return value;
}
std::wstring Number(double value, int precision) {
    std::wostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(precision) << (std::isfinite(value) ? value : 0.0);
    return out.str();
}
double DifficultyValue(const pattern::Snapshot& s) {
    switch (s.profile) {
    case pattern::RatingProfile::Osu4: case pattern::RatingProfile::Osu7: case pattern::RatingProfile::Osu8: return s.stars;
    case pattern::RatingProfile::Aery5: case pattern::RatingProfile::Osu10: return s.rating;
    default: return s.score;
    }
}
std::wstring DifficultyLabel(const pattern::Snapshot& s) {
    if (!s.ready || s.limited || s.idle) return L"측정 부족 / 보류";
    const auto dan = pattern::EstimateDan(s);
    if (dan.available) return pattern::FormatDan(s.profile, dan);
    if (s.profile == pattern::RatingProfile::Aery5)
        return std::wstring(s.outsideCalibration < 0 ? L"<" : s.outsideCalibration > 0 ? L">" : L"") + L"⑤" + Number(s.rating);
    return Number(s.score, 0) + L"/100";
}
std::wstring Basis(int keys) {
    const auto profile = pattern::ProfileForKeys(keys);
    if (profile == pattern::RatingProfile::Aery5) return L"5K AERY · 보조 기준 230채보 v2.13";
    const std::wstring basis = pattern::DanBasis(profile);
    return basis.empty() ? L"참고 입력 부담 지수 (공식 등급 아님)" : basis;
}
Analysis Analyze(Chart chart, double holdWeight, const std::atomic<bool>* cancelled) {
    if (chart.keys < 1 || chart.keys > 18 || chart.notes.empty() || chart.notes.size() > 200000)
        throw std::runtime_error("Unsupported lane count, empty chart, or more than 200,000 notes.");
    if (holdWeight != pattern::DefaultHoldWeight && holdWeight != 1.0)
        throw std::runtime_error("Unsupported hold-weight mode.");
    std::sort(chart.notes.begin(), chart.notes.end(), [](const Note& a, const Note& b) { return a.startMs != b.startMs ? a.startMs < b.startMs : a.lane < b.lane; });
    Analysis result; result.chart = std::move(chart); result.holdWeight = holdWeight;
    struct Event { double time; int lane; bool down; };
    std::vector<Event> events;
    events.reserve(result.chart.notes.size() * 2);
    const double origin = result.chart.notes.front().startMs;
    double last = origin;
    std::size_t left = 0;
    std::vector<double> releases(result.chart.keys, -1), previousStart(result.chart.keys, -1);
    for (std::size_t i = 0; i < result.chart.notes.size(); ++i) {
        const auto& n = result.chart.notes[i];
        if (!std::isfinite(n.startMs) || !std::isfinite(n.endMs) || n.startMs < 0 || n.endMs < n.startMs || n.lane < 0 || n.lane >= result.chart.keys)
            throw std::runtime_error("Invalid chart note timing or lane.");
        if (n.startMs < releases[n.lane] || n.startMs <= previousStart[n.lane]) throw std::runtime_error("Overlapping notes in one lane cannot be represented as physical input.");
        previousStart[n.lane] = n.startMs;
        // Taps use the same 10ms release as the reference pipeline. Extremely
        // close repeats must not create an impossible overlapping physical key.
        releases[n.lane] = n.endMs > n.startMs ? n.endMs : n.startMs;
        double release = n.endMs > n.startMs ? n.endMs : n.startMs + 10;
        last = std::max(last, release);
        while (result.chart.notes[left].startMs <= n.startMs - 1000.0) ++left;
        result.maximumNps = std::max(result.maximumNps, static_cast<double>(i - left + 1));
    }
    std::vector<double> nextPress(result.chart.keys, std::numeric_limits<double>::infinity());
    for (auto it=result.chart.notes.rbegin(); it!=result.chart.notes.rend(); ++it) {
        const auto& n=*it;
        const double end=n.endMs>n.startMs?n.endMs:std::min(n.startMs+10,nextPress[n.lane]);
        events.push_back({n.startMs-origin,n.lane,true});
        events.push_back({end-origin,n.lane,false});
        nextPress[n.lane]=n.startMs;
    }
    result.durationMs = last - origin;
    if (result.durationMs > 2 * 60 * 60 * 1000) throw std::runtime_error("Charts longer than 2 hours are not supported.");
    result.averageNps = result.chart.notes.size() / std::max(1.0, result.durationMs / 1000.0);
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.time != b.time) return a.time < b.time;
        if (a.down != b.down) return !a.down;
        return a.lane < b.lane;
    });
    pattern::DifficultyEstimator estimator(result.chart.keys, holdWeight);
    std::size_t next = 0;
    // Include the final partial step: otherwise all notes in the last 0-2 seconds
    // of a file can be omitted from its summary and both graphs.
    for (double time = std::min(8000.0, result.durationMs);;) {
        if (cancelled && cancelled->load()) throw std::runtime_error("Analysis cancelled.");
        while (next < events.size() && events[next].time <= time) {
            const auto& event = events[next++];
            if (event.down) estimator.KeyDown(event.lane, event.time);
            else estimator.KeyUp(event.lane, event.time);
        }
        auto snapshot = estimator.Evaluate(time, true);
        result.limited |= snapshot.limited;
        result.points.push_back({time, snapshot});
        if (time >= result.durationMs) break;
        time = std::min(time + 2000.0, result.durationMs);
    }
    std::vector<pattern::Snapshot> strongest;
    for (const auto& point : result.points)
        if (point.snapshot.ready && !point.snapshot.idle && !point.snapshot.limited) strongest.push_back(point.snapshot);
    std::sort(strongest.begin(), strongest.end(), [](const auto& a, const auto& b) { return DifficultyValue(a) > DifficultyValue(b); });
    result.strongestWindows = static_cast<int>(std::min<std::size_t>(3, strongest.size()));
    if (result.strongestWindows && !result.limited) {
        result.summary = strongest.front();
        result.summary.stars = result.summary.rating = result.summary.score = result.summary.nps = 0;
        for (int i = 0; i < result.strongestWindows; ++i) {
            result.summary.stars += strongest[i].stars / result.strongestWindows;
            result.summary.rating += strongest[i].rating / result.strongestWindows;
            result.summary.score += strongest[i].score / result.strongestWindows;
            result.summary.nps += strongest[i].nps / result.strongestWindows;
            if (strongest[i].outsideCalibration != strongest.front().outsideCalibration) result.summary.outsideCalibration=0;
        }
    }
    return result;
}
namespace {
std::string Escape(const std::wstring& text) {
    std::string out;
    for (char c : Utf8(text)) {
        switch (c) { case '&': out += "&amp;"; break; case '<': out += "&lt;"; break; case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break; case '\'': out += "&#39;"; break; default: out += c; }
    }
    return out;
}
std::string N(double x, int digits = 1) { return Utf8(Number(x, digits)); }
}
std::string RenderReport(const Analysis& a) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "<!doctype html><html lang='ko'><meta charset='utf-8'><meta name='viewport' content='width=device-width'>"
        << "<meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; style-src 'unsafe-inline'\">"
        << "<title>채보 난이도 분석</title><style>body{background:#101826;color:#e8eff9;font:16px system-ui;margin:32px auto;max-width:1050px;padding:0 20px}"
        << "h1{color:#6fe0ec}section{background:#182437;padding:22px;margin:18px 0;border-radius:14px}.muted{color:#a7b8cc}svg{width:100%;height:auto}table{border-collapse:collapse;width:100%}td,th{padding:8px;text-align:left;border-bottom:1px solid #34465e}</style>"
        << "<h1>채보 난이도 분석</h1><section><h2>" << Escape(a.chart.title) << "</h2><p>" << Escape(a.chart.artist) << " · " << Escape(a.chart.variant) << "</p>"
        << "<p>" << a.chart.keys << "K · " << Escape(Basis(a.chart.keys)) << "</p><h2>" << Escape(DifficultyLabel(a.summary)) << "</h2>"
        << "<p>가장 강한 " << a.strongestWindows << "개 구간의 평균 · " << a.chart.notes.size() << " 노트 · " << N(a.durationMs / 1000) << "초 · 평균 NPS " << N(a.averageNps) << " · 최대 1초 NPS " << N(a.maximumNps,0) << "</p>"
        << "<p class='muted'>최근 8초 창을 2초 간격으로 분석하고 파일 끝 시점도 포함합니다. 전체 곡 공식 등급이나 단위 합격 예측이 아닙니다. LN 추가 부담 " << (a.holdWeight == 1 ? "1 (기준 채보 측정)" : "2/3 (오버레이와 동일)") << ".</p></section>";
    if (a.limited) out << "<p>일부 구간이 분석 버퍼를 초과해 요약 난이도를 보류했습니다.</p>";
    for (const auto& warning : a.chart.warnings) out << "<p>" << Escape(warning) << "</p>";
    for (bool difficulty : {false, true}) {
        double maximum = 1;
        for (const auto& p : a.points) maximum = std::max(maximum, difficulty ? DifficultyValue(p.snapshot) : p.snapshot.currentNps);
        out << "<section><h2>" << (difficulty ? "시간별 난이도 비교값" : "시간별 1초 NPS") << "</h2><svg viewBox='0 0 980 240' role='img' aria-label='시간별 분석 그래프'>";
        for (int j=0;j<=4;++j) out << "<line x1='45' x2='960' y1='" << 20+j*45 << "' y2='" << 20+j*45 << "' stroke='#34465e'/><text x='0' y='" << 25+j*45 << "' fill='#a7b8cc' font-size='12'>" << N(maximum*(1-j/4.0)) << "</text>";
        bool priorValid=false; double priorX=0, priorY=0;
        for (const auto& p : a.points) {
            bool valid = !difficulty || (p.snapshot.ready && !p.snapshot.idle && !p.snapshot.limited);
            double value = difficulty ? DifficultyValue(p.snapshot) : p.snapshot.currentNps;
            double x=45+915*p.timeMs/std::max(1.0,a.durationMs), y=200-180*value/maximum;
            if (valid && priorValid) out << "<line x1='" << N(priorX) << "' y1='" << N(priorY) << "' x2='" << N(x) << "' y2='" << N(y) << "' stroke='" << (difficulty?"#c2a4ff":"#6fe0ec") << "' stroke-width='2'/>";
            if (valid) out << "<circle cx='" << N(x) << "' cy='" << N(y) << "' r='3' fill='" << (difficulty?"#c2a4ff":"#6fe0ec") << "'><title>" << N(p.timeMs/1000) << "초 · " << (difficulty ? Escape(DifficultyLabel(p.snapshot)) : N(value)+" NPS") << "</title></circle>";
            priorValid=valid; priorX=x; priorY=y;
        }
        out << "<text x='45' y='230' fill='#a7b8cc'>0초</text><text x='885' y='230' fill='#a7b8cc'>" << N(a.durationMs/1000) << "초</text></svg></section>";
    }
    out << "<section><h2>구간별 값</h2><table><tr><th>끝 시각</th><th>직전 최대 8초 난이도</th><th>1초 NPS</th></tr>";
    for (const auto& p:a.points) out << "<tr><td>" << N(p.timeMs/1000) << "초</td><td>" << Escape(DifficultyLabel(p.snapshot)) << "</td><td>" << N(p.snapshot.currentNps,0) << "</td></tr>";
    out << "</table></section><p class='muted'>Chart Difficulty Calculator v0.1.0 · 공유 모델: 오버레이 v2.13 (5K AERY 보조 기준 230채보). 곡명·제작자 메타데이터를 포함하며 원본 절대 경로는 저장하지 않습니다. 외부 통신·스크립트 없이 열리는 HTML입니다.</p></html>";
    return out.str();
}
void SaveReport(const Analysis& analysis, const std::filesystem::path& path) {
    auto extension=path.extension().wstring();
    std::transform(extension.begin(),extension.end(),extension.begin(),[](wchar_t c){return std::towlower(c);});
    if (extension != L".html" && extension != L".htm") throw std::runtime_error("Choose a .html report filename.");
    const auto html = RenderReport(analysis);
    auto temporary = path; temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create the report temporary file.");
    bool ok = true;
    for (std::size_t offset=0; ok && offset<html.size();) {
        DWORD written=0;
        ok=WriteFile(file,html.data()+offset,static_cast<DWORD>(std::min<std::size_t>(html.size()-offset,1<<20)),&written,nullptr) && written>0;
        offset+=written;
    }
    ok = ok && FlushFileBuffers(file);
    CloseHandle(file);
    if (ok) ok = MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if (!ok) { DeleteFileW(temporary.c_str()); throw std::runtime_error("Report save failed; the previous destination was preserved where possible."); }
}
}
