#include "pattern_difficulty.h"
#include "mania_strain.h"
#include "calibration_tables.h"
#include "aery_auxiliary_reference.h"
#include "dan_rating.h"
#include "revive10_reference.h"
#include "ln4_reference.h"
#include "analysis_key_mode.h"
#include "lane_display.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

using pattern::DifficultyEstimator;
using pattern::Snapshot;

int main() {
    int checks = 0, failed = 0;
    auto check = [&](const char* name, bool ok) {
        std::cout << name << ": " << (ok ? "ok" : "FAIL") << '\n';
        ++checks;
        if (!ok) ++failed;
    };
    auto stream = [](int notes, double interval, int usedLanes, int configuredLanes = 8) {
        DifficultyEstimator result(configuredLanes);
        for (int i = 0; i < notes; ++i) {
            result.KeyDown(i % usedLanes, i * interval);
            result.KeyUp(i % usedLanes, i * interval + 10.0);
        }
        return result;
    };

    DifficultyEstimator empty;
    check("empty_waits", empty.Evaluate(10000).idle && !empty.Evaluate(10000).ready && empty.Evaluate(10000).score == 0);
    empty.KeyDown(0, 0);
    empty.KeyDown(0, 100);
    empty.KeyDown(0, 200);
    check("held_repeat_not_counted", empty.Evaluate(2000).samples == 1 && empty.Evaluate(2000).heldKeys == 1 && !empty.Evaluate(2000).ready);
    empty.KeyUp(0, 2100);
    check("idle_clears_display", empty.Evaluate(4000).idle && empty.Evaluate(4000).nps == 0 && empty.Evaluate(4000).score == 0);
    empty.KeyDown(1, 5000);
    check("new_session_after_pause", empty.Evaluate(5010).samples == 1 && !empty.Evaluate(5010).ready);

    auto slow = stream(32, 250.0, 8);
    auto fast = stream(64, 125.0, 8);
    const auto s = slow.Evaluate(7999), f = fast.Evaluate(7999);
    check("sustained_input_ready", s.ready && f.ready);
    check("higher_density_is_harder", f.score > s.score && std::abs(f.nps - 8.001) < 0.01);
    check("two_second_peak", std::abs(f.burstNps - 8.0) < 0.001);
    auto jacks = stream(64, 125.0, 1);
    const auto j = jacks.Evaluate(7999);
    check("adjacent_repeat_adds_load", j.jackPercent > 90 && j.score > f.score && f.jackPercent == 0);
    auto alternating = stream(64, 125.0, 2);
    check("alternation_not_jacks", alternating.Evaluate(7999).jackPercent == 0);
    auto extraLanes = stream(64, 125.0, 2, 18);
    check("unused_lanes_do_not_dilute", std::abs(extraLanes.Evaluate(7999).score - alternating.Evaluate(7999).score) < 0.0001);

    DifficultyEstimator chords;
    for (int i = 0; i < 32; ++i) {
        const int a = (i % 4) * 2;
        chords.KeyDown(a, i * 250.0);
        chords.KeyDown(a + 1, i * 250.0 + 5);
        chords.KeyUp(a, i * 250.0 + 20);
        chords.KeyUp(a + 1, i * 250.0 + 25);
    }
    const auto c = chords.Evaluate(7999);
    check("chord_metrics", c.maxChord == 2 && c.chordPercent == 100 && c.jackPercent == 0);
    check("chord_load_at_equal_density", c.samples == f.samples && c.score > f.score);
    DifficultyEstimator anchored;
    anchored.KeyDown(0, 0); anchored.KeyDown(1, 14); anchored.KeyDown(2, 28);
    check("chord_anchor_does_not_chain", anchored.Evaluate(40).maxChord == 2);
    DifficultyEstimator oneChord;
    for (int lane = 0; lane < 8; ++lane) oneChord.KeyDown(lane, lane);
    check("single_chord_insufficient", !oneChord.Evaluate(2000).ready && oneChord.Evaluate(2000).score == 0);
    DifficultyEstimator rapid;
    rapid.KeyDown(0, 0); rapid.KeyUp(0, 1); rapid.KeyDown(0, 5);
    check("rapid_same_key_not_chord", rapid.Evaluate(10).maxChord == 1 && rapid.Evaluate(10).jackPercent == 50);

    DifficultyEstimator hold, noHold;
    hold.KeyDown(0, 0);
    for (int i = 0; i < 24; ++i) {
        const double time = 200 + i * 200.0;
        const int lane = 1 + i % 4;
        hold.KeyDown(lane, time); noHold.KeyDown(lane, time);
        hold.KeyUp(lane, time + 10); noHold.KeyUp(lane, time + 10);
    }
    const auto h = hold.Evaluate(5000), n = noHold.Evaluate(5000);
    check("hold_overlap_detected", h.holdOverlapPercent > 90 && n.holdOverlapPercent == 0 && h.heldKeys == 1);
    check("hold_interference_adds_load", h.score > n.score);
    hold.ReleaseAll();
    check("release_all_closes_holds", hold.Evaluate(5000).heldKeys == 0);
    hold.Reset(4);
    check("reset_discards_history", hold.Evaluate(5000).idle && hold.BufferedEvents() == 0);

    DifficultyEstimator burst;
    for (int i = 0; i < 64; ++i) {
        const double time = i < 32 ? i * 30.0 : 1000.0 + (i - 32) * 210.0;
        burst.KeyDown(i % 8, time); burst.KeyUp(i % 8, time + 10);
    }
    const auto b = burst.Evaluate(7999);
    check("burst_is_distinct_from_average", b.samples == f.samples && b.burstNps > f.burstNps && b.score > f.score);
    check("rolling_window_expires_old_notes", slow.Evaluate(9000).samples == 27);

    DifficultyEstimator invalid(4);
    invalid.KeyDown(-1, 0); invalid.KeyDown(4, 0); invalid.KeyDown(0, -1);
    invalid.KeyDown(0, std::numeric_limits<double>::quiet_NaN());
    invalid.KeyDown(0, std::numeric_limits<double>::infinity());
    check("invalid_events_ignored", invalid.BufferedEvents() == 0);
    invalid.KeyDown(0, 100); invalid.KeyUp(0, 110); invalid.KeyDown(1, 105);
    check("out_of_order_events_ignored", invalid.Evaluate(150).samples == 1);
    check("invalid_clock_safe", !invalid.Evaluate(std::numeric_limits<double>::quiet_NaN()).ready);

    DifficultyEstimator bounded;
    for (int i = 0; i < 5000; ++i) { bounded.KeyDown(0, i); bounded.KeyUp(0, i + 0.25); }
    const auto cap = bounded.Evaluate(5000);
    check("memory_is_bounded", bounded.BufferedEvents() == DifficultyEstimator::MaxEvents);
    check("overflow_withholds_rating", cap.limited && !cap.ready && cap.score == 0);
    bounded.KeyDown(1, 14000);
    check("overflow_recovers_after_pause", !bounded.Evaluate(14001).limited && bounded.BufferedEvents() == 1);
    bool finite = true;
    for (const Snapshot* sample : {&s, &f, &j, &c, &h, &n, &b, &cap})
        finite = finite && std::isfinite(sample->score) && sample->score >= 0 && sample->score <= 100;
    check("scores_finite_and_bounded", finite);

    using pattern::RatingProfile;
    check("requested_key_profiles", pattern::ProfileForKeys(4) == RatingProfile::Osu4 &&
        pattern::ProfileForKeys(7) == RatingProfile::Osu7 && pattern::ProfileForKeys(10) == RatingProfile::Osu10 &&
        pattern::ProfileForKeys(5) == RatingProfile::Aery5 && pattern::ProfileForKeys(8) == RatingProfile::Osu8 &&
        pattern::ProfileForKeys(6) == RatingProfile::Generic && pattern::ProfileForKeys(9) == RatingProfile::Generic);
    auto osu = stream(64, 125.0, 4, 4);
    auto o = osu.Evaluate(7999);
    check("osu_uses_strain_stars", o.profile == RatingProfile::Osu4 && o.ready && o.rating > 0 && o.rating == o.stars && o.rating != o.score);
    check("one_second_nps_visible_before_rating", stream(4, 100.0, 4, 4).Evaluate(500).currentNps == 4 &&
        !stream(4, 100.0, 4, 4).Evaluate(500).ready);
    check("nps_excludes_left_boundary", osu.Evaluate(7875).currentNps == 8 && osu.Evaluate(8875).currentNps == 0 &&
        osu.Evaluate(8875).nps > 0 && osu.Evaluate(10875).currentNps == 0);

    // Analytical reference cases stay within one 400 ms section. These assert the
    // upstream constants and hold/release rules independently of the live wrapper.
    const double decay = std::pow(0.30, 0.1), laneDecay = std::pow(0.125, 0.1);
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-10; };
    check("strain_two_note_reference", near(pattern::ManiaStars({{0,0,3},{100,100,0}},4), (3.0 + decay) * 0.018));
    const double overall2 = (decay + 1.0) * decay + 1.0;
    const double jackReference = (2.0 * laneDecay + 2.0 + overall2) * 0.018;
    check("strain_same_column_reference", near(pattern::ManiaStars({{0,0,3},{100,100,0},{200,200,0}},4), jackReference));
    check("strain_alternating_reference", near(pattern::ManiaStars({{0,0,3},{100,100,0},{200,200,1}},4), (2.0 + overall2) * 0.018));
    check("strain_hold_factor_reference", near(pattern::ManiaStars({{0,0,3},{100,500,0},{200,200,1}},4,0,1.0),
        (2.5 + (decay + 1.0) * decay + 1.25) * 0.018));
    const double release = 1.0 / (1.0 + std::exp(0.27 * (30.0 - 100.0)));
    check("strain_release_overlap_reference", near(pattern::ManiaStars({{0,0,3},{100,300,0},{200,400,1}},4,0,1.0),
        (2.0 + overall2 + release) * 0.018));
    check("strain_simultaneous_reference", near(pattern::ManiaStars({{0,0,3},{100,100,0},{100,100,1}},4), (4.0 + decay) * 0.018));
    const double lnWeight = 2.0 / 3.0;
    check("ln_interference_extra_is_two_thirds", near(pattern::ManiaStars({{0,0,3},{100,500,0},{200,200,1}},4),
        (2.0 + overall2 + 0.75 * lnWeight) * 0.018));
    check("ln_release_extra_is_two_thirds", near(pattern::ManiaStars({{0,0,3},{100,300,0},{200,400,1}},4),
        (2.0 + overall2 + release * lnWeight) * 0.018));
    const std::vector<pattern::TimedNote> combinedHolds{{0,0,3},{100,600,0},{200,350,1},{300,450,2}};
    const double combinedBase = 2.0 + overall2 * decay + 1.0;
    const double combinedExtra = 0.25 * decay + 0.75 + 1.25 * release;
    check("ln_combined_interaction_scaled_once", near(pattern::ManiaStars(combinedHolds,4),
        (combinedBase + combinedExtra * lnWeight) * 0.018));
    check("ln_zero_weight_preserves_tap_base", near(pattern::ManiaStars(combinedHolds,4,0,0), combinedBase * 0.018));
    bool liveLnReduced = true, tapUnchanged = true, statsUnchanged = true, resetKeepsWeight = true;
    for (int keys : {4,5,6,7,8,9,10}) {
        DifficultyEstimator adjusted(keys), reference(keys,1.0), taps(keys), originalTaps(keys,1.0);
        auto feed = [&](DifficultyEstimator& estimator, bool holdNote, double origin) {
            estimator.KeyDown(keys-1,origin); estimator.KeyUp(keys-1,origin+10);
            estimator.KeyDown(0,origin+100);
            if (!holdNote) estimator.KeyUp(0,origin+110);
            for (int i=0; i<60; ++i) {
                const int lane = 1+i%(keys-1); const double t = origin+200+i*125;
                estimator.KeyDown(lane,t); estimator.KeyUp(lane,t+10);
            }
        };
        feed(adjusted,true,0); feed(reference,true,0); feed(taps,false,0); feed(originalTaps,false,0);
        const auto a = adjusted.Evaluate(7800), b = reference.Evaluate(7800);
        const auto t = taps.Evaluate(7800), u = originalTaps.Evaluate(7800);
        liveLnReduced = liveLnReduced && a.ready && b.ready && a.stars < b.stars && a.rating <= b.rating && a.score < b.score;
        tapUnchanged = tapUnchanged && near(t.stars,u.stars) && near(t.rating,u.rating) && near(t.score,u.score);
        statsUnchanged = statsUnchanged && a.currentNps == b.currentNps && a.nps == b.nps && a.samples == b.samples &&
            a.holdOverlapPercent == b.holdOverlapPercent && a.heldKeys == b.heldKeys;
        adjusted.Reset(keys); feed(adjusted,true,10000);
        resetKeepsWeight = resetKeepsWeight && near(adjusted.Evaluate(17800).stars,a.stars);
    }
    check("ln_reduced_in_all_supported_key_modes", liveLnReduced);
    check("tap_patterns_keep_original_ratings", tapUnchanged);
    check("ln_adjustment_keeps_nps_and_input_statistics", statsUnchanged);
    check("reset_preserves_live_ln_weight", resetKeepsWeight);
    // Identical note heads must not gain LN strain merely because a physical
    // tap was released at 120ms instead of 10ms. Exercise current AND completed
    // ratings, since a transient open-key error could otherwise stick forever.
    auto mixedInput = [](int keys, int mode, double tapMs, input_timing::HoldTiming timing) {
        struct Event { double time; int lane; bool down; };
        std::vector<Event> events;
        auto note = [&](int lane, double start, double duration) {
            events.push_back({start,lane,true});events.push_back({start+duration,lane,false});
        };
        note(keys-1,0,10);
        if(mode==1)note(0,100,7900);
        for(int i=0;i<96;++i) {
            const int lane=mode==1?1+i%(keys-1):i%keys;
            note(lane,200+i*80.,mode==2 || (mode==3 && i%4==0)?250:tapMs);
        }
        std::sort(events.begin(),events.end(),[](const auto& a,const auto& b) {
            if(a.time!=b.time)return a.time<b.time;
            if(a.down!=b.down)return !a.down;
            return a.lane<b.lane;
        });
        DifficultyEstimator estimator(keys,pattern::DefaultHoldWeight,timing);
        // Reset must retain the caller's timing policy, including calibration.
        estimator.Reset(keys);
        pattern::PatternSummary summary;
        std::size_t next=0;
        for(double now=0;now<=8000;now+=10) {
            while(next<events.size() && events[next].time<=now) {
                const auto e=events[next++];
                if(e.down) {summary.Observe(estimator.Evaluate(e.time));estimator.KeyDown(e.lane,e.time);}
                else estimator.KeyUp(e.lane,e.time);
            }
            summary.Observe(estimator.Evaluate(now));
        }
        const auto current=estimator.Evaluate(8000);
        summary.Observe(Snapshot{});
        return std::pair<Snapshot,Snapshot>{current,summary.Completed()};
    };
    const auto liveTiming=input_timing::HoldTiming::LiveInput, referenceTiming=input_timing::HoldTiming::ReferenceReplay;
    for(int keys : {4,5,6,7,8,9,10}) for(int mode : {0,1,3}) {
        const auto tap=mixedInput(keys,mode,10,liveTiming);
        for(double dwell : {89.,90.,120.,150.,169.999}) {
            const auto slow=mixedInput(keys,mode,dwell,liveTiming);
            check("slow_taps_preserve_live_and_completed_ratings",slow.first.ready && slow.second.ready &&
                near(tap.first.stars,slow.first.stars) && near(tap.second.stars,slow.second.stars) &&
                near(tap.first.rating,slow.first.rating) && near(tap.second.rating,slow.second.rating));
            check("slow_taps_preserve_nps_and_hold_classification",tap.first.samples==slow.first.samples &&
                tap.first.nps==slow.first.nps && tap.first.currentNps==slow.first.currentNps &&
                tap.first.holdOverlapPercent==slow.first.holdOverlapPercent);
            check("slow_taps_do_not_raise_either_four_key_dan",keys!=4 ||
                (pattern::FormatDan(pattern::RatingProfile::Osu4,pattern::EstimateDan(tap.second))==
                 pattern::FormatDan(pattern::RatingProfile::Osu4,pattern::EstimateDan(slow.second)) &&
                 pattern::FormatLnDan(tap.second)==pattern::FormatLnDan(slow.second)));
        }
    }
    const auto fixedMixed=mixedInput(4,1,120,liveTiming), oldMixed=mixedInput(4,1,120,referenceTiming);
    check("reference_replay_preserves_published_90ms_scale",std::abs(oldMixed.first.stars-4.24619573117)<1e-9 &&
        std::abs(fixedMixed.first.stars-2.86926782096)<1e-9 && fixedMixed.first.stars<oldMixed.first.stars);
    const auto belowHold=mixedInput(4,0,169.999,liveTiming), atHold=mixedInput(4,0,170,liveTiming);
    check("primary_170ms_hold_boundary",atHold.first.stars>belowHold.first.stars && atHold.second.stars>belowHold.second.stars);
    for(int keys : {4,5,7,8,10}) {
        const auto liveLong=mixedInput(keys,2,250,liveTiming), originalLong=mixedInput(keys,2,250,referenceTiming);
        // Once every remaining key has passed both thresholds, the unchanged
        // strain formula gives the same result. A historical peak may differ
        // because open keys qualify later under physical-input inference.
        check("qualified_long_notes_keep_settled_strain_and_rating",near(liveLong.first.stars,originalLong.first.stars) &&
            near(liveLong.first.rating,originalLong.first.rating));
    }
    std::vector<pattern::TimedNote> sequence;
    for (int i = 0; i < 100; ++i) sequence.push_back({i * 125.0,i * 125.0,i % 4});
    const double original = pattern::ManiaStars(sequence, 4);
    check("recent_sections_exclude_older_peaks", pattern::ManiaStars(sequence,4,8000) < original && pattern::ManiaStars(sequence,4,13000) == 0);
    for (auto& note : sequence) note.column = 3 - note.column;
    check("strain_lane_permutation_invariant", near(pattern::ManiaStars(sequence,4),original));
    check("strain_invalid_input_safe", pattern::ManiaStars({{std::numeric_limits<double>::quiet_NaN(),0,0},{100,100,1}},4) == 0 &&
        pattern::ManiaStars({{0,0,4},{100,100,0}},4) == 0 && pattern::ManiaStars({{0,0,0},{100,50,1}},4) == 0 &&
        pattern::ManiaStars(sequence,4,std::numeric_limits<double>::infinity()) == 0);

    auto monotonic = [](const auto& curve) {
        for (std::size_t i = 1; i < curve.size(); ++i)
            if (curve[i].strain <= curve[i-1].strain || curve[i].level < curve[i-1].level) return false;
        return true;
    };
    check("calibration_is_monotonic", monotonic(pattern::AeryCalibration) && monotonic(pattern::InsaneCalibration));
    const auto aery = stream(160,50,5,5).Evaluate(7999);
    const auto osu8 = stream(400,20,8,8).Evaluate(7999);
    check("aery_keeps_bms_calibration", aery.ready && aery.rating != aery.stars && aery.ratingError == pattern::AeryError80);
    check("8k_uses_osu_strain_without_bms_calibration", osu8.ready && osu8.profile == RatingProfile::Osu8 &&
        osu8.rating == osu8.stars && osu8.ratingError == 0 && osu8.outsideCalibration == 0);
    check("bms_outside_range_is_flagged", stream(8,1000,5,5).Evaluate(7999).outsideCalibration == -1 &&
        stream(400,20,1,5).Evaluate(7999).outsideCalibration == 1);

    // Fixed tap fixtures. Expected v2.13 values were calculated independently
    // from the frozen v2.12 features and Python reference helper.
    auto steadyAery = [&](double interval) { return stream(160, interval, 5, 5).Evaluate(159 * interval + 10); };
    const auto aeryFloor = steadyAery(1000), aeryEasy = steadyAery(500), aeryLow = steadyAery(125);
    const auto aeryUpperLow = steadyAery(100), aeryTransition = steadyAery(85), aeryMid = steadyAery(75);
    const auto aeryHigh = steadyAery(60), aeryVeryHigh = steadyAery(40);
    check("aery_feedback_floor_is_one", aeryFloor.ready && near(aeryFloor.rating, 1.0) && aeryFloor.outsideCalibration == -1);
    check("aery_easy_keeps_manual_reduction_with_aux", aeryEasy.ready && near(aeryEasy.rating, 1.0376954717569444));
    check("aery_low_range_uses_aux_and_original_reduction", near(aeryLow.rating, 7.5919525298511505) && near(aeryUpperLow.rating, 8.6093545867583146));
    check("aery_transition_reduction_uses_unshifted_base", near(aeryTransition.rating, 10.415946655284724) && near(aeryMid.rating, 11.448820058451817));
    check("aery_high_range_uses_only_small_aux_shift", near(aeryHigh.rating, 13.460455425251322) && near(aeryVeryHigh.rating, 16.008517294950938));
    check("aery_keeps_34_curve_points_and_230_separate_anchors", pattern::AeryCalibration.size() == 34 && pattern::AeryAuxiliaryAnchors.size() == 230);
    check("aery_aux_invalid_and_outside_queries_are_zero",
        pattern::AeryAuxiliaryDelta(std::numeric_limits<double>::quiet_NaN(), 2) == 0 &&
        pattern::AeryAuxiliaryDelta(5, std::numeric_limits<double>::infinity()) == 0 &&
        pattern::AeryAuxiliaryDelta(5, -1) == 0 && pattern::AeryAuxiliaryDelta(0, 2) == 0 &&
        pattern::AeryAuxiliaryDelta(100, 2) == 0);
    bool boundedAux = true;
    for (double x = 0; x < 12; x += 0.05) for (double logNps = 0; logNps < 8; logNps += 0.2) {
        const double shift = pattern::AeryAuxiliaryDelta(x, logNps);
        boundedAux = boundedAux && std::isfinite(shift) && std::abs(shift) <= 0.5;
    }
    check("aery_aux_is_finite_and_bounded_across_feature_grid", boundedAux);
    check("aery_feedback_preserves_strain_and_nps", near(aeryLow.stars, 1.5877456950552937) && near(aeryLow.nps, 8.0) &&
        aeryLow.currentNps == 8 && aeryLow.chordPercent == 0 && aeryLow.jackPercent == 0 && aeryLow.holdOverlapPercent == 0);
    pattern::PatternSummary aerySummary;
    aerySummary.Observe(aeryLow); aerySummary.Observe(Snapshot{});
    check("aery_completed_result_uses_corrected_value", aerySummary.Completed().ready && near(aerySummary.Completed().rating, aeryLow.rating));
    pattern::LiveContinuity aeryContinuity;
    aeryContinuity.RecordActivity(19885);
    aeryContinuity.Update(aeryLow, 19885);
    Snapshot aeryGap; aeryGap.profile = RatingProfile::Aery5;
    const auto aeryRetained = aeryContinuity.Update(aeryGap, 20385);
    check("aery_retained_value_is_not_corrected_twice", aeryRetained.ready && aeryRetained.retained && near(aeryRetained.rating, aeryLow.rating));

    pattern::PatternSummary summary;
    summary.Observe(o);
    check("completed_result_waits_for_idle", !summary.Completed().ready);
    auto tail = o;
    tail.rating = o.rating / 2.0;
    summary.Observe(tail);
    summary.Observe(osu.Evaluate(11000));
    check("completion_keeps_strongest_window", summary.Completed().ready && summary.Completed().rating == o.rating);
    summary.Observe(osu.Evaluate(30000));
    summary.Observe(stream(1,100,4,4).Evaluate(200));
    summary.Observe(Snapshot{});
    check("completed_result_survives_wait_and_short_input", summary.Completed().rating == o.rating);
    summary.Observe(tail);
    check("previous_result_stays_during_next_measurement", summary.Completed().rating == o.rating);
    summary.Observe(Snapshot{});
    check("next_completion_can_replace_with_lower_rating", summary.Completed().rating == tail.rating);
    summary.Observe(o);
    summary.Observe(cap);
    summary.Observe(Snapshot{});
    check("overflow_does_not_publish_partial_result", summary.Completed().rating == tail.rating);
    summary.Reset();
    summary.Observe(Snapshot{});
    check("explicit_reset_clears_previous_pattern", !summary.Completed().ready);

    pattern::LiveContinuity continuity;
    auto segment = stream(24,125,4,4);
    continuity.RecordActivity(2885);
    const auto live = continuity.Update(segment.Evaluate(3000,true),3000);
    summary.Reset(); summary.Observe(live);
    const auto paused = continuity.Update(segment.Evaluate(6000,true),6000);
    summary.Observe(paused);
    check("live_gap_keeps_rating_not_nps", paused.retained && paused.ready && !paused.idle &&
        paused.rating == live.rating && paused.currentNps == 0 && paused.nps > 0 && !summary.Completed().ready);
    const auto longGap = continuity.Update(segment.Evaluate(11000,true),11000);
    check("live_gap_keeps_rating_after_window_expires", longGap.retained && longGap.rating == live.rating && longGap.samples == 0);
    check("live_end_uses_last_release_boundary", !continuity.Update(segment.Evaluate(12884,true),12884).idle &&
        continuity.Update(segment.Evaluate(12885,true),12885).idle);
    summary.Observe(continuity.Update(segment.Evaluate(12886,true),12886));
    check("live_completion_preserves_actual_peak", summary.Completed().ready && summary.Completed().rating == live.rating);
    segment.KeyDown(0,14000); continuity.RecordActivity(14000);
    check("next_session_does_not_borrow_old_rating", !continuity.Update(segment.Evaluate(14001,true),14001).ready);

    continuity.Reset(); segment = stream(24,125,4,4);
    continuity.RecordActivity(2885); continuity.Update(segment.Evaluate(3000,true),3000);
    continuity.Update(segment.Evaluate(7500,true),7500);
    segment.KeyDown(0,7500); segment.KeyUp(0,7510); continuity.RecordActivity(7510);
    check("resumed_input_keeps_previous_rating_during_warmup", continuity.Update(segment.Evaluate(7600,true),7600).retained &&
        continuity.Update(segment.Evaluate(7600,true),7600).currentNps == 1);
    for (int i=1; i<24; ++i) { segment.KeyDown(i%4,7500+i*125); segment.KeyUp(i%4,7510+i*125); continuity.RecordActivity(7510+i*125); }
    const auto resumed = continuity.Update(segment.Evaluate(10500,true),10500);
    check("resumed_valid_pattern_updates_rating", resumed.ready && !resumed.retained);
    auto limited = resumed; limited.limited = true; limited.ready = false;
    check("overflow_never_reuses_cached_rating", !continuity.Update(limited,10500).ready &&
        !continuity.Update(segment.Evaluate(14000,true),14000).ready);
    continuity.Reset();
    check("continuity_reset_clears_session", continuity.Update(Snapshot{},20000).idle);
    auto held = stream(24,125,4,4); held.KeyDown(0,3000);
    continuity.RecordActivity(3000); continuity.Update(held.Evaluate(3001,true),3001);
    check("long_hold_does_not_finish_session", !continuity.Update(held.Evaluate(20000,true),20000).idle);
    held.KeyUp(0,20000); continuity.RecordActivity(20000);
    check("long_hold_release_gets_full_grace", !continuity.Update(held.Evaluate(29999,true),29999).idle &&
        continuity.Update(held.Evaluate(30000,true),30000).idle);

    using pattern::RatingProfile;
    using pattern::DanHeight;
    pattern::Snapshot lnSample; lnSample.ready = true; lnSample.idle = false; lnSample.profile = RatingProfile::Osu4;
    for (const auto& band : pattern::Dan4LnReference) {
        lnSample.stars = band.strain;
        const auto estimate = pattern::EstimateLnDan(lnSample);
        check("ln4_reference_anchor", estimate.available && estimate.low == band.low && estimate.high == band.high &&
            estimate.height == DanHeight::Level && estimate.outside == 0);
    }
    lnSample.stars = pattern::Dan4LnReference[2].strain;
    check("ln4_reversed_references_are_explicit", pattern::FormatLnDan(lnSample) == L"LN 3~4단 수준");
    lnSample.stars = pattern::Dan4LnReference.back().strain;
    check("ln4_extra_ranks_are_numeric", pattern::FormatLnDan(lnSample) == L"LN 15단 수준");
    lnSample.stars = 0;
    check("ln4_outside_range", pattern::FormatLnDan(lnSample) == L"LN 1단↓");
    lnSample.limited = true;
    check("ln4_limited_is_not_published", pattern::FormatLnDan(lnSample).empty());
    lnSample.limited = false; lnSample.profile = RatingProfile::Osu7;
    check("ln4_never_applied_to_other_modes", !pattern::EstimateLnDan(lnSample).available);
    auto danText = [](RatingProfile profile, double strain) {
        return pattern::FormatDan(profile, pattern::EstimateDan(profile, strain));
    };
    // Midpoints select a course band; the course anchor itself is neutral 수준.
    // Exercise each height independently of the generated table's unequal gaps.
    const auto& reference = pattern::Dan4Reference;
    const double anchor = reference[6].strain; // 4th Dan
    const double left = (reference[5].strain + anchor) / 2;
    const double right = (reference[7].strain + anchor) / 2;
    check("dan_four_requested_heights", danText(RatingProfile::Osu4, left + (anchor-left)*0.25) == L"4단 low" &&
        danText(RatingProfile::Osu4, left + (anchor-left)*0.75) == L"4단 mid low" &&
        danText(RatingProfile::Osu4, anchor) == L"4단 수준" &&
        danText(RatingProfile::Osu4, anchor + (right-anchor)*0.75) == L"4단 high");
    check("dan_midpoint_advances_band", pattern::EstimateDan(RatingProfile::Osu4, right).low == 5 &&
        pattern::EstimateDan(RatingProfile::Osu4, right).height == DanHeight::Low);
    auto validDanCurve = [](RatingProfile profile, const auto& curve) {
        int previous = curve.front().low;
        double progress = -1;
        for (const auto& band : curve) {
            const auto estimate = pattern::EstimateDan(profile, band.strain);
            if (!estimate.available || estimate.low != band.low || estimate.high != band.high ||
                estimate.height != DanHeight::Level || estimate.outside != 0 || estimate.low < previous || estimate.progress < progress) return false;
            previous = estimate.low; progress = estimate.progress;
        }
        return true;
    };
    check("dan_all_reference_anchors", validDanCurve(RatingProfile::Osu4, pattern::Dan4Reference) &&
        validDanCurve(RatingProfile::Osu7, pattern::Dan7Reference) && validDanCurve(RatingProfile::Osu8, pattern::Dan8Reference) &&
        validDanCurve(RatingProfile::Osu10, pattern::Revive10Reference));
    check("dan_ambiguous_neighbors_visible", danText(RatingProfile::Osu4, 4.697745) == L"8~9단 수준" &&
        danText(RatingProfile::Osu7, 4.575865) == L"3~4단 수준");
    check("dan_key_modes_do_not_share_scale", pattern::EstimateDan(RatingProfile::Osu4, 3.7).low == 4 &&
        pattern::EstimateDan(RatingProfile::Osu7, 3.7).low == 1 && pattern::EstimateDan(RatingProfile::Osu10, 3.7).low == 1);
    check("dan_endpoints_and_bms_10k", danText(RatingProfile::Osu4, 0) == L"입문1단↓" &&
        danText(RatingProfile::Osu7, 0) == L"0단↓" && danText(RatingProfile::Osu10, 100) == L"오버조이↑" &&
        danText(RatingProfile::Osu10, 0) == L"1단↓" &&
        danText(RatingProfile::Osu10, pattern::Revive10Reference.back().strain) == L"오버조이 수준");
    check("10k_revive_individual_courses", std::wstring(pattern::DanBasis(RatingProfile::Osu10)) == L"10K Revive 2026" &&
        danText(RatingProfile::Osu10, 8.36643) == L"3단 수준" && danText(RatingProfile::Osu10, 9.45195) == L"4단 수준" &&
        danText(RatingProfile::Osu10, 11.9438) == L"5단 수준" && danText(RatingProfile::Osu10, 12.4637) == L"6단 수준" &&
        danText(RatingProfile::Osu10, 16.3659) == L"10단 수준" && danText(RatingProfile::Osu10, 22.7947) == L"개전 수준" &&
        pattern::Revive10Reference.size() == 16 &&
        pattern::Revive10Reference.front().low == 1 && pattern::Revive10Reference.back().high == 16);
    check("10k_revive_11_14_are_numeric", pattern::DanLabel(RatingProfile::Osu10, 11) == L"11" &&
        pattern::DanLabel(RatingProfile::Osu10, 12) == L"12" && pattern::DanLabel(RatingProfile::Osu10, 14) == L"14");
    const double tenLeft = (12.6503 + 14.3099) / 2.0;
    const double tenRight = (14.3099 + 16.0635) / 2.0;
    check("10k_revive_heights_and_midpoint", danText(RatingProfile::Osu10, tenLeft + (14.3099-tenLeft)*0.25) == L"8단 low" &&
        danText(RatingProfile::Osu10, tenLeft + (14.3099-tenLeft)*0.75) == L"8단 mid low" &&
        danText(RatingProfile::Osu10, 14.3099) == L"8단 수준" &&
        danText(RatingProfile::Osu10, 14.3099 + (tenRight-14.3099)*0.75) == L"8단 high" &&
        pattern::EstimateDan(RatingProfile::Osu10, tenRight).low == 9);
    Snapshot routed; routed.profile = RatingProfile::Osu10; routed.stars = 2.56646; routed.rating = 8.36643;
    check("10k_routes_pattern_load_not_raw_stars", pattern::EstimateDan(routed).low == 3 &&
        pattern::EstimateDan(RatingProfile::Osu10, routed.stars).outside == -1);

    // Physical-event fixtures: no chart labels enter the live measurement.
    auto technical10 = [](bool varied, bool uneven, bool jitter, bool mirror, double weight = pattern::DefaultHoldWeight) {
        DifficultyEstimator estimator(10, weight);
        double time = 0;
        for (int i = 0; i < 64; ++i) {
            const int a = varied ? i % 10 : 0, b = varied ? (i * 3 + 1) % 10 : 1;
            const double press = time + (jitter ? (i % 3 - 1) * 3.0 : 0.0) + 10;
            const int left = mirror ? 9 - a : a, right = mirror ? 9 - b : b;
            estimator.KeyDown(left, press);
            if (right != left) estimator.KeyDown(right, press);
            estimator.KeyUp(left, press + 20);
            if (right != left) estimator.KeyUp(right, press + 20);
            time += uneven ? (i % 2 ? 150 : 100) : 125;
        }
        return estimator.Evaluate(7999);
    };
    const auto technical = technical10(true, true, false, false);
    const auto mirrored = technical10(true, true, false, true);
    const auto noisy = technical10(true, true, true, false);
    const auto even = technical10(true, false, false, false);
    const auto evenNoisy = technical10(true, false, true, false);
    check("10k_pattern_variety_distinguishes_combinations", technical.patternVariety > technical10(false, true, false, false).patternVariety);
    check("10k_rhythm_tolerates_small_timing_jitter", even.rhythmComplexity == 0 && evenNoisy.rhythmComplexity == 0 &&
        technical.rhythmComplexity > 0.9 && std::abs(technical.rhythmComplexity - noisy.rhythmComplexity) < 0.01);
    check("10k_lane_mirror_preserves_pattern_load", std::abs(technical.rating - mirrored.rating) < 1e-9 &&
        technical.currentNps == mirrored.currentNps);
    const auto repeated = stream(80,100,2,10).Evaluate(7999);
    check("10k_repeats_across_intervening_rows_are_measured", repeated.columnRepeatLoad > 0 && repeated.adjacentRepeatLoad == 0);
    DifficultyEstimator early(10);
    for (int i = 0; i < 16; ++i) { early.KeyDown(i % 10, i*100); early.KeyUp(i % 10, i*100+10); }
    const auto earlyVariety = early.Evaluate(1600).patternVariety;
    for (int i = 16; i < 80; ++i) { early.KeyDown(i % 10, i*100); early.KeyUp(i % 10, i*100+10); }
    check("10k_warmup_does_not_inflate_variety", earlyVariety == early.Evaluate(7999).patternVariety);
    check("10k_tap_features_keep_hold_weight_independent", technical.rating == technical10(true,true,false,false,1.0).rating &&
        technical.rating > technical.stars);
    check("dan_bms_and_generic_unchanged", !pattern::EstimateDan(RatingProfile::Aery5, 4).available &&
        !pattern::EstimateDan(RatingProfile::Generic, 4).available);
    check("8k_named_extreme_courses_and_bounds", danText(RatingProfile::Osu8, 7.23991) == L"Flare단 수준" &&
        danText(RatingProfile::Osu8, 7.97186) == L"Aqua단 수준" &&
        danText(RatingProfile::Osu8, pattern::Dan8Reference.back().strain) == L"Demise단 수준" &&
        danText(RatingProfile::Osu8, 100) == L"Demise단↑" && danText(RatingProfile::Osu8, 0) == L"0~1단↓");
    check("8k_ambiguous_ranges_are_honest_and_compact", danText(RatingProfile::Osu8, pattern::Dan8Reference[5].strain) == L"6~8단 수준" &&
        danText(RatingProfile::Osu8, pattern::Dan8Reference[6].strain) == L"9~10단 수준" &&
        danText(RatingProfile::Osu8, pattern::Dan8Reference[9].strain) == L"13~14단 수준");
    const double eightLeft = (7.193875 + 7.23991) / 2.0;
    const double eightRight = (7.23991 + 7.97186) / 2.0;
    check("8k_four_heights_and_midpoint", danText(RatingProfile::Osu8, eightLeft + (7.23991-eightLeft)*0.25) == L"Flare단 low" &&
        danText(RatingProfile::Osu8, eightLeft + (7.23991-eightLeft)*0.75) == L"Flare단 mid low" &&
        danText(RatingProfile::Osu8, 7.23991 + (eightRight-7.23991)*0.75) == L"Flare단 high" &&
        pattern::EstimateDan(RatingProfile::Osu8, eightRight).low == 12);
    check("dan_invalid_strain_rejected", !pattern::EstimateDan(RatingProfile::Osu4, -1).available &&
        !pattern::EstimateDan(RatingProfile::Osu4, std::numeric_limits<double>::quiet_NaN()).available &&
        !pattern::EstimateDan(RatingProfile::Osu10, std::numeric_limits<double>::infinity()).available);
    pattern::AnalysisKeySelector selector;
    selector.Reset(10, -1);
    for (int i = 0; i < 64; ++i) { selector.Key((i % 4) * 2, true, i * 125.); selector.Key((i % 4) * 2, false, i * 125. + 10); }
    check("auto_keys_wait_for_full_observation", selector.Pending());
    selector.Update(8000);
    check("auto_keys_lock_sparse_registered_subset", !selector.Pending() && selector.Keys() == 4 && !selector.Conflict());
    auto compact = overlay::MakeLaneDisplay(10,900,100,selector,true);
    auto full = overlay::MakeLaneDisplay(10,900,100,selector,false);
    check("folded_layout_preserves_slot_size_and_pitch", compact.compact && compact.visible==4 && compact.width==404 &&
        compact.slotWidth==full.slotWidth && compact.pitch==full.pitch && compact.index[6]==3 && compact.index[1]==-1);
    bool geometry=true;
    for(int width : {240,600,900,1200}) for(int spacing : {50,100,160}) {
        auto folded=overlay::MakeLaneDisplay(10,width,spacing,selector,true);
        auto original=overlay::MakeLaneDisplay(10,width,spacing,selector,false);
        geometry &= folded.width>=240 && folded.width<=width && folded.pitch==original.pitch && folded.slotWidth==original.slotWidth;
    }
    check("folded_layout_has_safe_width_without_rescaling", geometry);
    for (int i = 0; i < 20; ++i) { selector.Key(0, true, 8100 + i*125.); selector.Key(0, false, 8110 + i*125.); }
    check("auto_keys_never_shrink_in_session", selector.Keys() == 4);
    selector.Key(9, true, 11000); selector.Key(9, false, 11010);
    check("auto_late_extra_key_rechecks_without_permanent_conflict", selector.Pending() && !selector.Conflict() && selector.Keys() == 4);
    check("folding_reverts_on_extra_key", !overlay::MakeLaneDisplay(10,900,100,selector,true).compact);
    auto lateKeyCorrection=selector;
    lateKeyCorrection.Key(9,true,12000);lateKeyCorrection.Key(9,false,12010);lateKeyCorrection.Update(14000);
    check("auto_recovers_after_late_key_is_confirmed",!lateKeyCorrection.Pending() && !lateKeyCorrection.Conflict() && lateKeyCorrection.Keys()==5);
    lateKeyCorrection.Key(1,true,14100);lateKeyCorrection.Key(1,false,14110);
    lateKeyCorrection.Key(3,true,14200);lateKeyCorrection.Key(3,false,14210);
    lateKeyCorrection.Update(17200);
    check("auto_correction_waits_for_each_new_lane_twice",lateKeyCorrection.Pending() && lateKeyCorrection.Keys()==5);
    lateKeyCorrection.Key(1,true,17300);lateKeyCorrection.Key(1,false,17310);
    lateKeyCorrection.Key(3,true,17400);lateKeyCorrection.Key(3,false,17410);
    check("auto_can_correct_again_to_seven_keys",!lateKeyCorrection.Pending() && lateKeyCorrection.Keys()==7);
    selector.Reset(18,-1);
    for(int i=0;i<66;++i) {selector.Key(i%11,true,i*125.);selector.Key(i%11,false,i*125.+10);}
    selector.Update(9000);
    check("auto_more_than_ten_keys_remains_explicitly_unsupported",selector.Pending() && selector.Conflict());
    selector=lateKeyCorrection;
    selector.Key(0, true, 27410);
    check("auto_new_session_relearns_after_ten_seconds", selector.Pending() && !selector.Conflict() && selector.Observed() == 1);
    check("new_session_unfolds_while_observing", overlay::MakeLaneDisplay(10,900,100,selector,true).visible==10);
    selector.Key(0, true, 31010); selector.Update(31010);
    check("auto_held_repeat_is_not_new_session_or_samples", selector.Pending() && selector.Observed() == 1);
    selector.Reset(10, -1);
    for (int i=0;i<64;++i) { selector.Key(i%3,true,i*125.); selector.Key(i%3,false,i*125.+10); }
    selector.Update(8000);
    check("auto_does_not_invent_fourth_key", selector.Pending() && selector.Observed()==3);
    selector.Reset(10, 7);
    selector.Key(9,true,0); selector.Key(9,false,10);
    check("manual_basis_preserves_physical_lane_selection", selector.Keys()==7 && !selector.Pending() && !selector.Conflict());
    for(int lane=0;lane<7;++lane){selector.Key(lane,true,20+lane*20);selector.Key(lane,false,30+lane*20);}
    check("manual_extra_keys_not_silently_discarded", selector.Keys()==7 && selector.Conflict());
    selector.Reset(10, 999);
    check("invalid_key_mode_uses_registered_default", selector.Keys()==10 && !selector.Pending());
    selector.Reset(10,4);
    for(int lane : {0,2,4,6}) { selector.Key(lane,true,lane*20.); selector.Key(lane,false,lane*20.+10); }
    check("manual_folding_waits_for_repeated_physical_subset", !selector.CompactReady());
    for(int lane : {0,2,4,6}) { selector.Key(lane,true,200+lane*20.); selector.Key(lane,false,210+lane*20.); }
    check("manual_folding_uses_observed_not_first_four_keys", selector.CompactReady() &&
        overlay::MakeLaneDisplay(10,900,100,selector,true).index[6]==3 && !overlay::MakeLaneDisplay(10,900,100,selector,true).Visible(1));
    auto subset=stream(64,125,4,10);subset.SetAnalysisKeys(4);
    auto native4=stream(64,125,4,4);
    check("analysis_basis_changes_without_changing_input", subset.Evaluate(7999).profile==pattern::RatingProfile::Osu4 &&
        subset.Evaluate(7999).samples==native4.Evaluate(7999).samples && std::abs(subset.Evaluate(7999).rating-native4.Evaluate(7999).rating)<1e-12);
    auto tooMany=stream(64,125,5,10);tooMany.SetAnalysisKeys(4);
    check("manual_conflict_suppresses_rating_but_keeps_nps", tooMany.Evaluate(7999).keyModeConflict && !tooMany.Evaluate(7999).ready && tooMany.Evaluate(7999).currentNps>0);
    DifficultyEstimator sparse10(64),compact10(10);sparse10.SetAnalysisKeys(10);
    for(int i=0;i<80;i++){sparse10.KeyDown(32+i%10,i*100.);sparse10.KeyUp(32+i%10,i*100.+10);compact10.KeyDown(i%10,i*100.);compact10.KeyUp(i%10,i*100.+10);}
    check("10k_basis_packs_sparse_physical_indices", sparse10.Evaluate(7999).ready && std::abs(sparse10.Evaluate(7999).rating-compact10.Evaluate(7999).rating)<1e-12);
    pattern::LiveContinuity keyContinuity;
    auto readyKeys=native4.Evaluate(7999);keyContinuity.RecordActivity(7990);keyContinuity.Update(readyKeys,7999);
    auto pendingKeys=readyKeys;pendingKeys.ready=false;pendingKeys.keyModePending=true;
    check("pending_key_mode_never_retains_stale_rating", !keyContinuity.Update(pendingKeys,8000).ready);
    pattern::PatternSummary conflictedKeys;conflictedKeys.Observe(readyKeys);
    auto conflictKeys=readyKeys;conflictKeys.ready=false;conflictKeys.keyModeConflict=true;conflictedKeys.Observe(conflictKeys);conflictedKeys.Observe(pattern::Snapshot{});
    check("key_conflict_blocks_completed_peak", !conflictedKeys.Completed().ready);
    std::cout << "checks=" << checks << " failures=" << failed << '\n';
    return failed ? 1 : 0;
}
