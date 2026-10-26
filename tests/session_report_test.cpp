#include "session_report.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <cmath>

int main(int argc, char** argv) {
    int checks = 0, failed = 0;
    auto check = [&](const char* name, bool ok) { ++checks; if (!ok) ++failed; std::cout << name << ": " << (ok ? "ok" : "FAIL") << '\n'; };
    report::Recorder recorder;
    check("empty_never_completes", !recorder.Active() && !recorder.Finished(10000) && recorder.Take("empty", 10000).presses == 0);
    recorder.Begin(4, 1000, "synthetic <test> & \"fixture\"");
    recorder.Key(-1, true, 1000); recorder.Key(4, true, 1000);
    recorder.Key(0, true, std::numeric_limits<double>::quiet_NaN());
    recorder.Key(0, true, 1000); recorder.Key(0, true, 1001);
    check("held_prevents_finish", !recorder.Finished(100000));
    recorder.Key(0, false, 1200);
    check("release_boundary", !recorder.Finished(11199) && recorder.Finished(11200));
    pattern::Snapshot point;
    point.profile = pattern::RatingProfile::Osu4; point.idle = false; point.ready = true;
    point.stars = 3; point.currentNps = 4;
    recorder.Observe(point, 1200);
    auto retained = point; retained.retained = true; retained.stars = 50;
    recorder.Observe(retained, 1800);
    auto shortSession = recorder.Take("synthetic", 11200);
    check("duration_excludes_idle_tail_and_repeats", shortSession.durationMs == 200 && shortSession.presses == 1 && shortSession.notes.size() == 1);
    check("peak_does_not_use_retained_rating", shortSession.peak.snapshot.stars == 3 && shortSession.points.size() == 1);
    check("completed_once", !recorder.Active() && recorder.Take("again", 20000).presses == 0);
    recorder.Begin(10, 0, "synthetic demo — not a real performance");
    pattern::DifficultyEstimator estimator(10);
    for (int i = 0; i < 360; ++i) {
        const double t = i * 100.0;
        const int lane = (i * 7) % 10;
        recorder.Key(lane, true, t); estimator.KeyDown(lane, t);
        // Put holds after the tap stream so fixture timestamps stay monotonic.
        recorder.Key(lane, false, t + 30); estimator.KeyUp(lane, t + 30);
        if (i % 5 == 0) recorder.Observe(estimator.Evaluate(t + 30, true), t + 30);
    }
    recorder.Key(2, true, 36100); estimator.KeyDown(2, 36100);
    recorder.Observe(estimator.Evaluate(36500, true), 36500);
    recorder.Key(2, false, 36900); estimator.KeyUp(2, 36900);
    recorder.Key(7, true, 37000);
    auto session = recorder.Take("synthetic demo / interrupted hold", 37500);
    check("forced_completion_closes_hold", session.notes.back().interrupted && session.notes.back().endMs == 37500 && session.durationMs == 37500);
    check("10k_uses_pattern_load", report::DifficultyValue(session.peak.snapshot) == session.peak.snapshot.rating);
    auto html = report::RenderHtml(session);
    check("10k_report_identifies_revive_reference", html.find("10K Revive 2026 기준 v2.12") != std::string::npos &&
        html.find("10K BMS Dans (Tryr)") == std::string::npos);
    check("offline_document", html.find("<script") == std::string::npos && html.find("https://") == std::string::npos && html.find("default-src 'none'") != std::string::npos);
    check("three_graphs_and_summary", html.find("시간별 NPS") != std::string::npos && html.find("시간별 추정 난이도") != std::string::npos && html.find("레인별 타건 흐름") != std::string::npos && html.find("단위 합격") != std::string::npos);
    auto escaped = report::RenderHtml(shortSession);
    check("metadata_is_html_escaped", escaped.find("&lt;test&gt; &amp; &quot;fixture&quot;") != std::string::npos && escaped.find("<test>") == std::string::npos);
    report::Recorder aeryRecorder;
    pattern::DifficultyEstimator aeryEstimator(5);
    aeryRecorder.Begin(5, 0, "synthetic AERY feedback fixture");
    for (int i = 0; i < 160; ++i) {
        aeryRecorder.Key(i % 5, true, i * 100.0); aeryEstimator.KeyDown(i % 5, i * 100.0);
        aeryRecorder.Key(i % 5, false, i * 100.0 + 10); aeryEstimator.KeyUp(i % 5, i * 100.0 + 10);
    }
    aeryRecorder.Observe(aeryEstimator.Evaluate(15910), 15910);
    const auto aerySession = aeryRecorder.Take("synthetic", 25910);
    const auto aeryHtml = report::RenderHtml(aerySession);
    check("aery_report_uses_corrected_peak_and_graph_value", aerySession.peak.snapshot.ready && aerySession.points.size() == 1 &&
        std::abs(report::DifficultyValue(aerySession.peak.snapshot) - 8.6093545867583146) < 1e-10 &&
        report::DifficultyValue(aerySession.points.front().snapshot) == report::DifficultyValue(aerySession.peak.snapshot) &&
        aeryHtml.find("⑤8.6") != std::string::npos);
    check("aery_report_identifies_feedback_model", aeryHtml.find("AERY 저레벨 보정 v2.12") != std::string::npos &&
        aeryHtml.find("230개 보조 기준 v2.13") != std::string::npos &&
        html.find("AERY 저레벨 보정 v2.12") == std::string::npos);
    recorder.Begin(4, 0, "bounded");
    for (std::size_t i = 0; i < report::Recorder::MaxNotes + 3; ++i) {
        recorder.Key(0, true, i * 1000.0);
        recorder.Key(0, false, i * 1000.0 + 1);
        recorder.Observe(point, i * 1000.0 + 1);
    }
    auto bounded = recorder.Take("bounded", 60000000);
    check("bounded_memory", bounded.truncated && bounded.notes.size() == report::Recorder::MaxNotes && bounded.points.size() == report::Recorder::MaxPoints && bounded.presses == report::Recorder::MaxNotes + 3);
    recorder.Begin(4, 0, "limited"); recorder.Key(0, true, 0); recorder.Key(0, false, 30);
    point.limited = true; recorder.Observe(point, 30);
    auto limited = recorder.Take("limited", 10030);
    check("analysis_overflow_is_not_a_valid_peak", limited.analysisLimited && report::RenderHtml(limited).find("세션 최고 난이도를 보류") != std::string::npos);
    const auto root = std::filesystem::current_path() / ("report-test-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
    if (!std::filesystem::create_directory(root)) return 2;
    auto first = report::Save(session, root / L"한글 리포트");
    auto second = report::Save(session, root / L"한글 리포트");
    check("unicode_path_and_no_overwrite", first.ok && second.ok && first.path != second.path && std::filesystem::exists(first.path) && std::filesystem::exists(second.path));
    std::ifstream saved(first.path, std::ios::binary);
    std::string readback((std::istreambuf_iterator<char>(saved)), {}); saved.close();
    check("saved_content_matches", readback == html);
    { std::ofstream blocker(root / "blocked"); blocker << "preserve me"; }
    auto failure = report::Save(session, root / "blocked" / "reports");
    check("failure_is_reported", !failure.ok && failure.error != 0 && std::filesystem::file_size(root / "blocked") == 11);
    bool temporary = false;
    for (const auto& file : std::filesystem::recursive_directory_iterator(root)) temporary |= file.path().extension() == ".tmp";
    check("no_partial_files", !temporary);
    if (argc == 3 && std::string(argv[1]) == "--demo") {
        std::ofstream demo(std::filesystem::u8path(argv[2]), std::ios::binary);
        demo << html;
        check("demo_written", demo.good());
    }
    // Delete only this invocation's freshly created test directory.
    if (root.parent_path() != std::filesystem::current_path() || root.filename().string().find("report-test-") != 0) return 3;
    std::filesystem::remove_all(root);
    std::cout << "checks=" << checks << " failures=" << failed << '\n';
    return failed ? 1 : 0;
}
