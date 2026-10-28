#include "revive_auxiliary.h"
#include "revive_config.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>

int main() {
    int checks = 0, failures = 0, cases = 0;
    auto check = [&](bool ok, const char* name) {
        ++checks;
        if (!ok) { ++failures; std::cerr << "FAIL " << name << '\n'; }
    };
    auto close = [](double a, double b) { return std::abs(a-b) <= std::max(2e-8, 1e-10 * std::max(std::abs(a),std::abs(b))); };
    // Synthetic notes and Python reference values from the preserved parity
    // experiment. No installed calculator, personal charts or network needed.
    for (int keys = 1; keys <= revive::MaxKeys; ++keys) for (const auto& file : std::filesystem::directory_iterator(
        keys == 10 ? "." : keys == 4 ? "../revive4" : "../revive_keys/" + std::to_string(keys))) {
        if (file.path().extension() != ".tsv") continue;
        ++cases;
        std::ifstream in(file.path());
        double duration, difficulty, level, rating; std::size_t count;
        in >> duration >> count >> difficulty >> level >> rating;
        std::array<double,6> expectedSkills;
        for (auto& value : expectedSkills) in >> value;
        std::vector<revive::Note> notes(count);
        for (auto& note : notes) in >> note.time >> note.lane >> note.type;
        if (!in) { check(false, "reference fixture read"); continue; }
        const auto native = revive::calculate(notes,duration,revive::ConfigForKeys(keys));
        check(close(native.scalars.at("revive_difficulty"),difficulty), "Python reference difficulty");
        check(native.scalars.at("revive_lv") == level, "Python reference LV");
        check(close(native.scalars.at("circus_rating"),rating), "Python reference rating");
        const char* axes[] = {"STREAM","SPEED","JACK","CHORD","TECH","HOLD"};
        for (std::size_t i=0; i<expectedSkills.size(); ++i)
            check(close(native.scalars.at(std::string("skill_radar.")+axes[i]),expectedSkills[i]), "Python reference skill axis");
        chartcalc::Chart chart; chart.keys = keys;
        std::sort(notes.begin(), notes.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
        std::array<double,revive::MaxKeys> heads; heads.fill(-1);
        for (const auto& note : notes) {
            if (note.type == 1) heads[note.lane] = note.time;
            else if (note.type == 2) {
                chart.notes.push_back({note.lane,heads[note.lane]*1000,note.time*1000}); heads[note.lane] = -1;
            } else chart.notes.push_back({note.lane,note.time*1000,note.time*1000});
        }
        const auto adapted = revive_aux::Analyze(chart);
        if (chart.notes.empty()) check(!adapted.ready,"empty chart has no skill values");
        else {
            const double first = notes.front().time;
            const double actualDuration = std::max(1.0, notes.back().time-first);
            for (auto& note : notes) note.time -= first;
            const auto wholeChart = revive::calculate(notes,actualDuration,revive::ConfigForKeys(keys));
            for (std::size_t i=0; i<expectedSkills.size(); ++i)
                check(adapted.ready && close(adapted.skills[i],wholeChart.scalars.at(std::string("skill_radar.")+axes[i])),
                    "chart adapter preserves skill axes at actual duration");
        }
    }
    check(cases == 167, "all 167 Python reference cases across 1K through 18K loaded");
    chartcalc::Chart chart; chart.keys = 10;
    for (int i=0; i<160; ++i) chart.notes.push_back({i*3%10,i*50.0,i*50.0});
    const auto original = revive_aux::Analyze(chart);
    check(original.ready && close(original.skills[0],52.07), "known 10K stream value");
    check(revive_aux::Label(original) == L"Revive CPP · 요구 스킬: 테크닉 / 스트림", "top chart skill names");
    check(revive_aux::Label(original,true) == L"CPP · 테크닉 / 스트림", "compact top chart skill names");
    const auto details=revive_aux::Details(original);
    check(details.find(L"스트림 52")!=std::wstring::npos && details.find(L"속도 43")!=std::wstring::npos &&
        details.find(L"연타 14")!=std::wstring::npos && details.find(L"동시치기 0")!=std::wstring::npos &&
        details.find(L"테크닉 57")!=std::wstring::npos && details.find(L"홀드 0")!=std::wstring::npos &&
        details.find(L"LV")==std::wstring::npos, "all six skill axes shown without LV");
    auto shifted = chart;
    for (auto& note : shifted.notes) { note.startMs += 12345; note.endMs += 12345; }
    const auto shiftedSkills=revive_aux::Analyze(shifted);
    for (std::size_t i=0; i<6; ++i)
        check(shiftedSkills.ready && close(shiftedSkills.skills[i],original.skills[i]), "chart lead-in does not change skills");
    for (int keys : {-1,0,19,256}) {
        auto other = chart; other.keys = keys;
        const auto unsupported = revive_aux::Analyze(other);
        check(!unsupported.ready && unsupported.skills == std::array<double,6>{} && unsupported.status == L"1~18K 지원", "unsupported keys have no old value");
    }
    for (int keys = 1; keys <= revive::MaxKeys; ++keys) {
        std::vector<revive::Note> holds;
        for (int lane=0; lane<keys; ++lane) { holds.push_back({0,lane,1}); holds.push_back({8,lane,2}); }
        const auto result = revive::calculate(holds,8,revive::ConfigForKeys(keys));
        check(close(result.scalars.at("skill_radar_components.HOLD.hold_lane_coverage"),1), "full hold coverage uses actual key count");
        bool rejected = false;
        try { revive::calculate({{0,keys,0}},1,revive::ConfigForKeys(keys)); }
        catch (const std::exception&) { rejected = true; }
        check(rejected,"every key count enforces lane bounds");
    }
    auto four = chart; four.keys = 4;
    for (auto& note : four.notes) note.lane %= 4;
    check(revive_aux::Analyze(four).ready, "four-key chart supported");
    four.notes.front().lane = 4;
    check(!revive_aux::Analyze(four).ready, "four-key out-of-range lane rejected");
    std::vector<revive::Note> fullHolds;
    for (int lane=0;lane<4;++lane) {fullHolds.push_back({0,lane,1});fullHolds.push_back({8,lane,2});}
    const auto occupied = revive::calculate(fullHolds,8,revive::ConfigForKeys(4));
    check(close(occupied.scalars.at("skill_radar_components.HOLD.hold_lane_coverage"),1), "4K hold coverage uses four lanes");
    bool invalidFour = false;
    try { revive::calculate({{0,4,0}},1,revive::ConfigForKeys(4)); }
    catch (const std::exception&) { invalidFour = true; }
    check(invalidFour, "native 4K bounds enforced");
    revive_aux::Summary ranking; ranking.ready=true; ranking.skills={10,20,100,40,50,60};
    check(revive_aux::Label(ranking,true)==L"CPP · 연타 / 홀드", "jack and hold ranking");
    ranking.holdKnown=false;
    check(revive_aux::Label(ranking,true)==L"CPP · 연타 / 테크닉" && revive_aux::SkillValue(ranking,5)==L"—" &&
        revive_aux::Details(ranking).find(L"홀드 60")==std::wstring::npos,"unknown hold is neither ranked nor displayed as a measurement");
    ranking.holdKnown=true;
    ranking.skills={0,0,0,0,0,0};
    check(revive_aux::Label(ranking,true)==L"CPP · 특징 부족", "zero traits do not invent required skills");
    ranking.skills={50,50,50,50,50,50};
    check(revive_aux::Label(ranking,true)==L"CPP · 스트림 / 속도", "stable tie order");
    auto malformed = chart; malformed.notes.front().lane = 10;
    check(!revive_aux::Analyze(malformed).ready, "invalid lane");
    malformed = chart; malformed.notes.front().startMs = std::numeric_limits<double>::quiet_NaN();
    check(!revive_aux::Analyze(malformed).ready, "NaN rejected before sort");
    malformed = chart; malformed.notes.push_back(chart.notes.front());
    check(!revive_aux::Analyze(malformed).ready, "duplicate press");
    malformed = chart; malformed.notes.front().endMs = 10000;
    check(!revive_aux::Analyze(malformed).ready, "overlapping hold");
    malformed = chart; malformed.notes.front().endMs = -1;
    check(!revive_aux::Analyze(malformed).ready, "negative hold end");
    malformed = chart; malformed.notes.resize(revive::MaxEvents + 1);
    check(revive_aux::Analyze(malformed).status == L"노트 한도 · 보류", "event bound");
    malformed.notes.clear();
    for (int i=0; i<22000; ++i) malformed.notes.push_back({i%10,i*100.0,i*100.0+10});
    check(revive_aux::Analyze(malformed).status == L"노트 한도 · 보류", "hold tails count toward event bound");
    malformed.notes.clear();
    for (int i=0; i<3000; ++i) malformed.notes.push_back({i%10,i*.1,i*.1});
    check(revive_aux::Analyze(malformed).status == L"밀도 한도 · 보류", "dense input bounded");
    std::atomic<bool> cancelled{true};
    check(revive_aux::Analyze(chart,&cancelled).status == L"취소", "pre-cancel");
    bool timedOut = false;
    try { revive::calculate({{0,0,0}},1,revive::DefaultConfig(),nullptr,std::chrono::milliseconds(0)); }
    catch (const std::exception& e) { timedOut = std::string(e.what()) == "revive time limit"; }
    check(timedOut,"deadline boundary");
    std::vector<revive::Note> longInput;
    for (std::size_t i=0; i<revive::MaxEvents; ++i) longInput.push_back({i*.02,static_cast<int>(i%10),0});
    cancelled=false;
    bool stopped = false;
    const auto start = std::chrono::steady_clock::now();
    std::thread worker([&] {
        try { revive::calculate(std::move(longInput),800,revive::DefaultConfig(),&cancelled); }
        catch (const std::exception& e) { stopped = std::string(e.what()) == "revive cancelled"; }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); cancelled=true; worker.join();
    check(stopped && std::chrono::steady_clock::now()-start < std::chrono::seconds(2), "running calculation cancels promptly");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
