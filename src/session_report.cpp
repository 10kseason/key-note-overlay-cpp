#include "session_report.h"
#include "dan_rating.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace report {
namespace {
std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}
std::string Escape(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
        case '&': result += "&amp;"; break;
        case '<': result += "&lt;"; break;
        case '>': result += "&gt;"; break;
        case '"': result += "&quot;"; break;
        case '\'': result += "&#39;"; break;
        default: result += c;
        }
    }
    return result;
}
std::string Number(double value, int precision = 1) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(precision) << (std::isfinite(value) ? value : 0.0);
    return out.str();
}
bool Valid(const pattern::Snapshot& s) { return s.ready && !s.idle && !s.retained && !s.limited && !s.keyModePending && !s.keyModeConflict; }
}

double DifficultyValue(const pattern::Snapshot& s) {
    switch (s.profile) {
    case pattern::RatingProfile::Osu4: case pattern::RatingProfile::Osu7: case pattern::RatingProfile::Osu8: return s.stars;
    case pattern::RatingProfile::Osu10: case pattern::RatingProfile::Aery5: return s.rating;
    default: return s.score;
    }
}
std::string DifficultyLabel(const pattern::Snapshot& s) {
    if (!Valid(s)) return "측정 부족 / 보류";
    const auto dan = pattern::EstimateDan(s);
    if (dan.available) {
        const auto ln = pattern::FormatLnDan(s);
        return Utf8(pattern::FormatDan(s.profile, dan) + (ln.empty() ? L"" : L" · " + ln));
    }
    if (s.profile == pattern::RatingProfile::Aery5)
        return std::string(s.outsideCalibration < 0 ? "<" : s.outsideCalibration > 0 ? ">" : "") + "⑤" + Number(s.rating);
    return Number(s.score, 0) + "/100";
}

void Recorder::Begin(int lanes, double nowMs, const std::string& startedLocal) {
    if (Active() || !std::isfinite(nowMs) || nowMs < 0) return;
    session_ = {};
    session_.lanes = std::clamp(lanes, 1, 256);
    session_.startedLocal = startedLocal;
    // No vector growth in the keyboard callback after session start.
    session_.notes.reserve(MaxNotes);
    session_.points.reserve(MaxPoints);
    held_.fill(-1);
    startMs_ = lastInputMs_ = nowMs;
    lastPointMs_ = -1;
}
void Recorder::Key(int lane, bool down, double nowMs) {
    if (!Active() || lane < 0 || lane >= session_.lanes || !std::isfinite(nowMs) || nowMs < lastInputMs_) return;
    int& held = held_[lane];
    if (down) {
        if (held != -1) return;
        ++session_.presses;
        if (session_.notes.size() < MaxNotes) {
            held = static_cast<int>(session_.notes.size());
            session_.notes.push_back({lane, nowMs - startMs_, nowMs - startMs_});
        } else { held = -2; session_.truncated = true; }
    } else {
        if (held == -1) return;
        if (held >= 0) session_.notes[held].endMs = nowMs - startMs_;
        held = -1;
    }
    lastInputMs_ = nowMs;
}
void Recorder::Observe(const pattern::Snapshot& snapshot, double nowMs) {
    if (!Active() || !std::isfinite(nowMs) || nowMs < lastInputMs_) return;
    session_.analysisLimited |= snapshot.limited;
    session_.analysisKeyConflict |= snapshot.keyModeConflict;
    Point point{nowMs - startMs_, snapshot};
    if (Valid(snapshot) && (!Valid(session_.peak.snapshot) || DifficultyValue(snapshot) > DifficultyValue(session_.peak.snapshot)))
        session_.peak = point;
    if (lastPointMs_ >= 0 && nowMs - lastPointMs_ < 500.0) return;
    lastPointMs_ = nowMs;
    if (session_.points.size() < MaxPoints) session_.points.push_back(point);
    else session_.truncated = true;
}
void Recorder::ReleaseAll(double nowMs) {
    if (!Active()) return;
    nowMs = std::max(nowMs, lastInputMs_);
    for (int lane = 0; lane < session_.lanes; ++lane) if (held_[lane] != -1) {
        if (held_[lane] >= 0) session_.notes[held_[lane]].interrupted = true;
        Key(lane, false, nowMs);
    }
}
bool Recorder::Finished(double nowMs) const {
    return Active() && std::isfinite(nowMs) && nowMs - lastInputMs_ >= pattern::LiveContinuity::FinishDelayMs &&
        std::all_of(held_.begin(), held_.end(), [](int index) { return index == -1; });
}
Session Recorder::Take(const std::string& reason, double nowMs) {
    if (!Active()) return {};
    ReleaseAll(nowMs);
    session_.durationMs = std::max(0.0, lastInputMs_ - startMs_);
    session_.reason = reason;
    // Waiting for the ten-second boundary must not inflate duration or the graph.
    while (!session_.points.empty() && session_.points.back().timeMs > session_.durationMs) session_.points.pop_back();
    Session result = std::move(session_);
    session_ = {};
    startMs_ = lastInputMs_ = lastPointMs_ = -1;
    return result;
}

std::string RenderHtml(const Session& s) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(2);
    const double seconds = std::max(1.0, s.durationMs / 1000.0);
    // Exact peak rolling NPS comes from captured press times, not the UI timer.
    double maxNps = 0;
    for (std::size_t i = 0, left = 0; i < s.notes.size(); ++i) {
        while (left < i && s.notes[left].startMs <= s.notes[i].startMs - 1000.0) ++left;
        maxNps = std::max(maxNps, static_cast<double>(i - left + 1));
    }
    out << R"HTML(<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'">
<title>Key Note · 연주 리포트</title><style>
:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#0e1524;color:#e5edf8;font:15px/1.65 system-ui,'Malgun Gothic',sans-serif}main{max-width:1120px;margin:auto;padding:40px 24px}h1{font-size:34px;margin:8px 0}h2{font-size:20px;margin:28px 0 8px}.eyebrow{color:#7dd3fc;letter-spacing:.13em}p,.muted{color:#aabbd1}.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;margin:24px 0}.card,section{border:1px solid #29354b;border-radius:14px;background:#151f32;padding:18px}.value{display:block;font-size:26px;font-weight:700;color:#fff}.warning{color:#fcd88a;border-left:3px solid #fcd88a;padding-left:14px}.scroll{overflow-x:auto}svg{display:block;font:12px system-ui}svg text{fill:#aabbd1}.chart{width:100%;height:auto}.grid{stroke:#2a3950;stroke-width:1}table{width:100%;border-collapse:collapse}td,th{text-align:left;padding:10px;border-bottom:1px solid #2a3950}footer{margin-top:28px;color:#aabbd1;font-size:13px}@media print{body{background:#fff;color:#172033}.card,section{background:#fff;color:#172033}.value{color:#172033}main{padding:10px}.scroll{overflow:visible}}
</style><main><div class="eyebrow">KEY NOTE OVERLAY · LOCAL REPORT</div><h1>이번 연주는 어땠나요?</h1><p>)HTML";
    out << Escape(s.startedLocal) << " · " << s.lanes << "K · " << Escape(s.reason) << "</p>";
    const int analysisKeys = s.peak.snapshot.analysisKeys > 0 ? s.peak.snapshot.analysisKeys : s.lanes;
    if (analysisKeys != s.lanes) out << "<p>등록 배치 " << s.lanes << "키 · 난이도 분석 기준 " << analysisKeys << "K</p>";
    const bool peakValid = Valid(s.peak.snapshot) && !s.analysisLimited && !s.analysisKeyConflict;
    out << "<div class='cards'><div class='card'>연주 길이<span class='value'>" << Number(s.durationMs / 1000) << "초</span></div>"
        << "<div class='card'>총 누름<span class='value'>" << s.presses << "회</span></div>"
        << "<div class='card'>최대 1초 NPS<span class='value'>" << (s.truncated ? "≥ " : "") << Number(maxNps, 0) << "</span></div>"
        << "<div class='card'>가장 어려웠던 관측 구간<span class='value'>" << Escape(peakValid ? DifficultyLabel(s.peak.snapshot) : "측정 부족 / 보류") << "</span></div></div>";
    out << "<p>전체 평균 " << Number(s.presses / seconds) << " NPS · 시작부터 마지막 뗌까지의 구간 평균 (1초 미만은 1초 기준). "
        << "단위 합격·정확도·실력 인증이 아닌, 실제 누른 입력의 추정 부담입니다.</p>";
    if (s.truncated) out << "<p class='warning'>기록 한도에 도달했습니다. 타건 흐름은 첫 50,000개, 그래프는 첫 7,200개 관측점까지만 보존됩니다. 최대 NPS는 보존된 입력 기준이며 전체 최고치보다 낮을 수 있습니다.</p>";
    if (s.analysisLimited) out << "<p class='warning'>분석 입력 버퍼가 초과된 구간이 있어 세션 최고 난이도를 보류합니다. 해당 구간의 난이도 선은 끊어 표시합니다.</p>";
    if (s.analysisKeyConflict) out << "<p class='warning'>고정한 분석 키 수보다 많은 키가 감지되어 세션 최고 난이도를 보류합니다. 분석 초기화 또는 키 수 설정 변경이 필요합니다.</p>";
    auto chart = [&](bool difficulty) {
        const char* title = difficulty ? "시간별 추정 난이도" : "시간별 NPS";
        double maximum = 1.0;
        for (const auto& point : s.points) {
            if (difficulty && !Valid(point.snapshot)) continue;
            maximum = std::max(maximum, difficulty ? DifficultyValue(point.snapshot) : point.snapshot.currentNps);
        }
        maximum *= 1.1;
        out << "<h2>" << title << "</h2><section><svg class='chart' viewBox='0 0 1000 235' role='img' aria-label='" << title << "'>";
        for (int i = 0; i <= 4; ++i) {
            const double y = 185.0 - 40 * i;
            out << "<line class='grid' x1='60' x2='975' y1='" << y << "' y2='" << y << "'/><text x='8' y='" << y + 4 << "'>" << Number(maximum * i / 4) << "</text>";
            out << "<text x='" << 60 + 915.0 * i / 4 << "' y='216' text-anchor='middle'>" << Number(seconds * i / 4) << "s</text>";
        }
        bool connected = false;
        double previousTime = -10000;
        std::ostringstream path;
        path.imbue(std::locale::classic()); path << std::fixed << std::setprecision(2);
        for (const auto& point : s.points) {
            if (difficulty && !Valid(point.snapshot)) { connected = false; continue; }
            const double value = difficulty ? DifficultyValue(point.snapshot) : point.snapshot.currentNps;
            const double x = 60 + 915 * point.timeMs / (seconds * 1000);
            const double y = 185 - 160 * value / maximum;
            path << (connected && point.timeMs - previousTime <= 1000 ? " L " : " M ") << x << ' ' << y;
            connected = true; previousTime = point.timeMs;
            out << "<circle cx='" << x << "' cy='" << y << "' r='2' fill='" << (difficulty ? "#c4a5ff" : "#67d9ec") << "'><title>" << Number(point.timeMs / 1000) << "초 · "
                << Escape(difficulty ? DifficultyLabel(point.snapshot) : Number(value) + " NPS") << "</title></circle>";
        }
        out << "<path d='" << path.str() << "' fill='none' stroke='" << (difficulty ? "#c4a5ff" : "#67d9ec") << "' stroke-width='2'/></svg>";
        out << (difficulty ? "<p>세로축은 " : "<p>");
        if (difficulty) {
            const auto profile = pattern::ProfileForKeys(analysisKeys);
            out << (profile == pattern::RatingProfile::Osu10 ? "10K 패턴 비교 부담" : profile == pattern::RatingProfile::Aery5 ? "AERY ⑤ 추정값" : profile == pattern::RatingProfile::Generic ? "입력 지수 (0~100)" : "osu!mania 구간 strain 환산값 (★)");
            out << ". 점에 마우스를 올리면 단위·등급을 볼 수 있습니다. 준비 중·연결 대기·표본 손실은 선을 잇지 않습니다.";
        } else out << "최근 1초 누름 수를 약 0.5초마다 관측합니다. 타이머 지연은 빈 구간으로 남기며, 카드의 최대 NPS는 타건 시각에서 별도로 계산합니다.";
        out << "</p></section>";
    };
    chart(false); chart(true);
    out << "<h2>가장 어려웠던 관측 구간</h2><section>";
    if (peakValid) {
        out << "<table><tr><th>시간 (직전 최대 8초)</th><th>추정 난이도</th><th>동시치기</th><th>연속 같은 레인</th><th>홀드 겹침</th></tr><tr><td>"
            << Number(std::max(0.0, s.peak.timeMs - 8000) / 1000) << "–" << Number(std::min(s.durationMs, s.peak.timeMs) / 1000) << "초</td><td>"
            << Escape(DifficultyLabel(s.peak.snapshot)) << "</td><td>" << Number(s.peak.snapshot.chordPercent) << "%</td><td>" << Number(s.peak.snapshot.jackPercent)
            << "%</td><td>" << Number(s.peak.snapshot.holdOverlapPercent) << "%</td></tr></table>";
    } else out << "<p>유효한 난이도 표본이 부족하거나 분석 버퍼가 초과되었습니다. 짧은 타건의 NPS·레인 흐름은 아래에서 확인할 수 있습니다.</p>";
    out << "</section><h2>레인별 타건 흐름</h2><section><p>왼쪽 → 오른쪽이 시간입니다. 레인 번호는 오버레이의 왼쪽부터이며 실제 키 이름은 저장하지 않습니다. 짧은 누름은 점처럼, 90ms 이상은 길이로 표시합니다. 가로로 스크롤하고 막대에 마우스를 올려 시각을 확인하세요.</p><div class='scroll'>";
    const double width = std::clamp(seconds * 40, 950.0, 48000.0);
    const double scale = width / (seconds * 1000);
    out << "<svg width='" << width + 70 << "' height='" << s.lanes * 26 + 45 << "' role='img' aria-label='레인별 타건 흐름'>";
    for (int lane = 0; lane < s.lanes; ++lane) {
        out << "<text x='4' y='" << lane * 26 + 40 << "'>L" << lane + 1 << "</text><line class='grid' x1='55' x2='" << width + 55 << "' y1='" << lane * 26 + 35 << "' y2='" << lane * 26 + 35 << "'/>";
    }
    const double tick = std::max(1.0, std::ceil(seconds / std::max(1.0, width / 100.0)));
    for (double t = 0; t <= seconds; t += tick) out << "<text x='" << 55 + t * 1000 * scale << "' y='16'>" << Number(t, 0) << "s</text>";
    for (const auto& note : s.notes) {
        const bool hold = note.endMs - note.startMs >= 90;
        out << "<rect x='" << 55 + note.startMs * scale << "' y='" << note.lane * 26 + 28 << "' width='" << std::max(2.0, hold ? (note.endMs - note.startMs) * scale : 2.0)
            << "' height='14' rx='2' fill='" << (note.interrupted ? "#fcd88a" : hold ? "#c4a5ff" : "#67d9ec") << "'><title>L" << note.lane + 1 << " · " << Number(note.startMs / 1000, 3)
            << "–" << Number(note.endMs / 1000, 3) << "초" << (note.interrupted ? " · 설정 변경/종료로 홀드 닫힘" : "") << "</title></rect>";
    }
    out << "</svg></div></section><footer>Key Note Overlay · 리포트 v1 · 난이도 모델 v2.8";
    if (analysisKeys == 5) out << " / AERY 저레벨 보정 v2.12 + 230개 보조 기준 v2.13";
    if (analysisKeys == 10) out << " / 10K Revive 2026 기준 v2.12";
    out << " / LN 추가 부담 2/3. 곡명·게임 판정·놓친 노트·손 배치·클리어 여부는 알 수 없습니다. 다른 키 수의 수치를 직접 비교하지 마세요. 로컬 시각과 등록 레인의 타이밍이 포함되므로 공유 전 확인하세요. 외부 통신·스크립트·글꼴 다운로드가 없는 독립 HTML입니다.</footer></main></html>";
    return out.str();
}

SaveResult Save(const Session& session, const std::filesystem::path& directory) {
    static std::atomic<unsigned long long> sequence{0};
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) return {false, {}, static_cast<unsigned long>(ec.value())};
    SYSTEMTIME time{}; GetLocalTime(&time);
    wchar_t stamp[64];
    swprintf(stamp, 64, L"session-%04u%02u%02u-%02u%02u%02u-%03u-", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
    std::filesystem::path path, temporary;
    HANDLE file = INVALID_HANDLE_VALUE;
    for (int tries = 0; tries < 32; ++tries) {
        path = directory / (std::wstring(stamp) + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++sequence) + L".html");
        temporary = path; temporary += L".tmp";
        file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) break;
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) return {false, {}, error};
    }
    if (file == INVALID_HANDLE_VALUE) return {false, {}, GetLastError()};
    DWORD error = ERROR_SUCCESS;
    try {
        const std::string html = RenderHtml(session);
        for (std::size_t offset = 0; offset < html.size();) {
            DWORD written = 0;
            if (!WriteFile(file, html.data() + offset, static_cast<DWORD>(std::min<std::size_t>(html.size() - offset, 65536)), &written, nullptr) || !written) {
                error = GetLastError(); if (!error) error = ERROR_WRITE_FAULT; break;
            }
            offset += written;
        }
        if (!error && !FlushFileBuffers(file)) error = GetLastError();
    } catch (...) { error = ERROR_NOT_ENOUGH_MEMORY; }
    if (!CloseHandle(file) && !error) error = GetLastError();
    // Deliberately omit REPLACE_EXISTING: even a name collision cannot erase a report.
    if (!error && !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH)) error = GetLastError();
    if (error) { DeleteFileW(temporary.c_str()); return {false, {}, error}; }
    return {true, path, 0};
}
}
