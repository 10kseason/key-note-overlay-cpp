#include "revive_live.h"
#include "revive_hud_layout.h"
#include <cmath>
#include <iostream>
#include <thread>

int main() {
    int checks = 0, failures = 0;
    auto check = [&](const char* name, bool ok) { ++checks; if (!ok) { ++failures; std::cerr << name << " FAILED\n"; } };
    revive_live::Window window; window.Reset(10);
    for (int i = 0; i < 80; ++i) { window.Key(i % 10, true, i * 100); window.Key(i % 10, false, i * 100 + 30); }
    const auto sample = window.Snapshot(8000);
    check("eight_second_taps", sample.status.empty() && sample.chart.notes.size() == 80 && sample.durationMs == 8000);
    bool taps = true; for (const auto& n : sample.chart.notes) taps &= n.startMs == n.endMs;
    check("short_presses_are_taps", taps);
    // The same tap chart played with a slower release must not invent LN tails
    // or change the five press-based axes. Keyboard duration is not chart data.
    for(int keys : {4,7,10,18}) for(int heldMs : {90,95,120,200,500}) {
        revive_live::Window quick,slow;quick.Reset(keys);slow.Reset(keys);
        for(int i=0;i<16;++i) {
            quick.Key(i%keys,true,i*600);quick.Key(i%keys,false,i*600+30);
            slow.Key(i%keys,true,i*600);slow.Key(i%keys,false,i*600+heldMs);
        }
        const auto a=quick.Snapshot(9600),b=slow.Snapshot(9600);
        const auto quickTraits=revive_aux::Analyze(a.chart,nullptr,a.durationMs);
        const auto slowTraits=revive_aux::Analyze(b.chart,nullptr,b.durationMs);
        bool noTails=true;for(const auto& note:b.chart.notes)noTails &= note.startMs==note.endMs;
        check("slow_tap_release_does_not_create_ln",noTails && slowTraits.skills[5]==0);
        check("slow_tap_release_preserves_press_traits",quickTraits.ready && slowTraits.ready && quickTraits.skills==slowTraits.skills);
    }
    const auto expected = revive_aux::Analyze(sample.chart, nullptr, sample.durationMs);
    check("all_six_reference_values_available", expected.ready);
    revive_live::Meter meter; meter.Reset(10);
    for (int i = 0; i < 80; ++i) { meter.Key(i % 10, true, i * 100); meter.Key(i % 10, false, i * 100 + 30); }
    const auto started = std::chrono::steady_clock::now();
    meter.Update(8000, true, 10, false, false, false);
    check("async_snapshot_queued", meter.Busy());
    for (int retry = 0; retry < 200 && meter.Busy(); ++retry) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); meter.Update(8000, true, 10, false, false, false);
    }
    check("async_result_ready", meter.Current().ready);
    check("live_hold_is_unknown_not_a_zero_or_detection",!meter.Current().holdKnown && revive_aux::SkillValue(meter.Current(),5)==L"—" &&
        revive_aux::Details(meter.Current()).find(L"홀드 미확인")!=std::wstring::npos);
    for (int i = 0; i < 6; ++i) check("async_matches_snapshot", std::abs(expected.skills[i] - meter.Current().skills[i]) < 1e-8);
    std::cout << "80-press window completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count() << " ms (includes polling)\n";
    meter.Update(8100, true, 10, false, false, false);
    check("half_second_throttle", !meter.Busy());
    // A new session can arrive before the UI has collected the previous job.
    // Publishing that old future must not revive the preceding session's axes.
    meter.Update(8500, true, 10, false, false, false);
    meter.Key(0, true, 20000); meter.Key(0, false, 20030);
    meter.Update(20100, true, 10, false, false, false);
    check("new_session_rejects_inflight_old_result", !meter.Current().ready);
    meter.Update(8500, true, 10, false, false, false);
    meter.Reset(10);
    meter.Update(8500, false, 10, false, false, false);
    check("reset_and_disable_clear_old_values", !meter.Current().ready && meter.Current().status == L"OFF");
    meter.Update(18000, true, 10, false, false, false);
    check("old_generation_discarded", !meter.Current().ready);
    meter.Update(18000, true, 4, false, false, false);
    check("four_key_subset_waits_for_layout", !meter.Current().ready && meter.Current().status == L"4키 배치 확인 중");
    meter.Update(18000, true, 10, true, false, false);
    check("pending_key_mode", meter.Current().status == L"키 추정 중");
    meter.Update(18000, true, 10, false, true, false);
    check("conflicting_key_mode", meter.Current().status == L"키 수 확인");
    window.Reset(10); window.Key(0, true, 0); window.Key(0, true, 10); window.Key(0, false, 89);
    window.Key(1, true, 100); window.Key(1, false, 190); window.Key(2, true, 200);
    auto holds = window.Snapshot(8200);
    check("held_key_keeps_only_original_press", holds.chart.notes.size() == 1 && holds.chart.notes[0].startMs == 0 && holds.chart.notes[0].endMs == 0);
    window.ReleaseAll(8300);
    holds = window.Snapshot(8400);
    check("focus_loss_does_not_invent_tail_in_new_window", holds.chart.notes.empty());
    window.Key(2,true,8500);window.Key(2,false,8510);
    check("focus_loss_allows_fresh_press",window.Snapshot(8600).chart.notes.size()==1);
    window.Reset(10); window.Key(0, true, 0); window.Key(0, true, 20); window.Key(0, false, 89);
    window.Key(1, true, 100); window.Key(1, false, 190);
    holds = window.Snapshot(200);
    check("90ms_is_not_a_chart_ln_boundary_and_no_repeats", holds.chart.notes.size() == 2 && holds.chart.notes[0].endMs == 0 && holds.chart.notes[1].endMs == 100);
    window.Key(3, true, 180); window.Key(11, true, 200);
    check("invalid_events_ignored", window.Size() == 2);
    window.Key(0, true, 11000); window.Key(0, false, 11020);
    check("fresh_session_discards_history", window.Size() == 1 && window.Snapshot(11030).durationMs == 30);
    window.Reset(10);
    for (int i = 0; i <= 4096; ++i) { window.Key(i % 10, true, i); window.Key(i % 10, false, i + 0.1); }
    check("bounded_input_and_overflow_gate", window.Size() <= revive_live::MaxPresses && window.Snapshot(4100).status == L"입력 한도 · 보류");
    window.Reset(19); window.Key(0, true, 0);
    check("unsupported_analysis_never_publishes_values", window.Snapshot(2000).status == L"1~18K 지원");
    revive_live::Window fourWindow, subsetWindow;
    fourWindow.Reset(4); subsetWindow.Reset(10); meter.Reset(4);
    const int physical[] = {1,3,7,9};
    for(int i=0;i<80;++i) {
        for(auto* input : {&fourWindow,&subsetWindow}) {
            const int lane=input==&fourWindow?i%4:physical[i%4];
            input->Key(lane,true,i*100); input->Key(lane,false,i*100+95);
        }
        meter.Key(i%4,true,i*100); meter.Key(i%4,false,i*100+95);
    }
    const auto fourSample=fourWindow.Snapshot(8000);
    const auto subsetSample=subsetWindow.Snapshot(8000,4);
    const auto fourExpected=revive_aux::Analyze(fourSample.chart,nullptr,fourSample.durationMs);
    const auto subsetExpected=revive_aux::Analyze(subsetSample.chart,nullptr,subsetSample.durationMs);
    check("native_four_lane_window",fourSample.status.empty() && fourSample.chart.keys==4 && fourExpected.ready);
    check("sparse_four_key_mapping_matches",subsetSample.status.empty() && subsetSample.chart.keys==4 && subsetExpected.ready && subsetExpected.skills==fourExpected.skills);
    meter.Update(8000,true,4,false,false,false);
    for(int retry=0;retry<200 && meter.Busy();++retry) {std::this_thread::sleep_for(std::chrono::milliseconds(2));meter.Update(8000,true,4,false,false,false);}
    check("live_four_key_press_axes_match_hold_unknown",meter.Current().ready && meter.Current().skills==fourExpected.skills &&
        meter.Current().skills[5]==0 && !meter.Current().holdKnown);
    // An old 4K result may finish after a mode change. It must never label 10K.
    meter.Update(8500,true,4,false,false,false);
    meter.Update(8500,true,10,false,false,false);
    check("basis_change_discards_old_axes",!meter.Current().ready);
    subsetWindow.Key(9,true,12000); subsetWindow.Key(9,false,12030);
    const auto rolled=subsetWindow.Snapshot(16000,4);
    check("mapping_survives_window_expiry",rolled.chart.notes.size()==1 && rolled.chart.notes[0].lane==3);
    subsetWindow.Key(5,true,16010);
    check("extra_physical_lane_rejects_four_key_mapping",subsetWindow.Snapshot(16020,4).status==L"키 수 확인");
    subsetWindow.Reset(10);
    for(int lane : {1,3,7}) {subsetWindow.Key(lane,true,lane*100);subsetWindow.Key(lane,false,lane*100+30);}
    check("incomplete_four_key_mapping_has_no_values",subsetWindow.Snapshot(2000,4).status==L"4키 배치 확인 중");
    for (int keys=1; keys<=revive::MaxKeys; ++keys) {
        window.Reset(keys); meter.Reset(keys); subsetWindow.Reset(18);
        for (int i=0; i<80; ++i) {
            const int lane=i%keys;
            window.Key(lane,true,i*100); window.Key(lane,false,i*100+95);
            meter.Key(lane,true,i*100); meter.Key(lane,false,i*100+95);
            // Right-aligned physical keys retain their order after remapping.
            subsetWindow.Key(18-keys+lane,true,i*100); subsetWindow.Key(18-keys+lane,false,i*100+95);
        }
        const auto direct=window.Snapshot(8000);
        const auto reference=revive_aux::Analyze(direct.chart,nullptr,direct.durationMs);
        const auto remapped=subsetWindow.Snapshot(8000,keys);
        const auto remappedResult=revive_aux::Analyze(remapped.chart,nullptr,remapped.durationMs);
        check("all_key_counts_press_only_no_false_ln",reference.ready && reference.skills[5]==0);
        check("all_key_counts_remap_sparse_physical_lanes",remapped.status.empty() && remappedResult.ready && remappedResult.skills==reference.skills);
        meter.Update(8000,true,keys,false,false,false);
        for(int retry=0;retry<200 && meter.Busy();++retry) {std::this_thread::sleep_for(std::chrono::milliseconds(2));meter.Update(8000,true,keys,false,false,false);}
        check("all_key_counts_publish_press_traits_hold_unknown",meter.Current().ready && meter.Current().skills==reference.skills &&
            !meter.Current().holdKnown && !revive_aux::SkillAvailable(meter.Current(),5));
    }
    chartcalc::Chart dense; dense.keys = 10;
    for (int i=0;i<640;++i) dense.notes.push_back({i%10,i*12.5,std::min(8000.0,i*12.5+100)});
    const auto denseStarted=std::chrono::steady_clock::now();
    const auto denseResult=revive_aux::Analyze(dense,nullptr,8000,std::chrono::milliseconds(100));
    const auto denseElapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-denseStarted).count();
    check("dense_window_completes_or_explicitly_times_out",denseResult.ready || denseResult.status==L"시간 한도 · 보류");
    std::cout << "80 NPS / 640 holds: " << denseElapsed << " ms, ready=" << denseResult.ready << '\n';
    for (int width : {240,329,330,599,600,900}) for (bool live : {false,true}) for (bool selected : {false,true}) {
        const auto layout = revive_hud::Make(width, live, selected);
        check("six_cells_fit_grid", layout.columns * (6 / layout.columns) == 6 && (width - 8) / layout.columns >= 97);
        check("notes_keep_space_below_all_axes", revive_hud::PresentationHeight(90,28,layout) >= layout.total + 44);
    }
    meter.Stop();
    std::cout << "checks=" << checks << " failures=" << failures << '\n';
    return failures ? 1 : 0;
}
