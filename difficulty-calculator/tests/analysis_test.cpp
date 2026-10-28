#include "analysis.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
// Independent millisecond callback simulation: releases, keydown callbacks,
// then the UI timer. Fixtures use integer milliseconds and no tap overlaps.
chartcalc::Analysis SimulateLive(const chartcalc::Chart& chart) {
    struct Event { int lane; bool down; };
    std::map<int, std::vector<Event>> events;
    const double origin=chart.notes.front().startMs;
    int end=0;
    for(const auto& n:chart.notes) {
        const int start=static_cast<int>(n.startMs-origin);
        const int release=static_cast<int>((n.endMs>n.startMs?n.endMs:n.startMs+10)-origin);
        events[start].push_back({n.lane,true}); events[release].push_back({n.lane,false});
        end=std::max(end,release);
    }
    pattern::DifficultyEstimator estimator(chart.keys);
    pattern::LiveContinuity continuity;
    pattern::PatternSummary peak;
    chartcalc::Analysis result;
    auto observe=[&](int t) {
        auto snapshot=continuity.Update(estimator.Evaluate(t,true),t);
        if(!snapshot.idle)peak.Observe(snapshot);
        return snapshot;
    };
    for(int t=0;t<=end;++t) {
        const auto found=events.find(t);
        if(found!=events.end()) {
            auto actions=found->second;
            std::sort(actions.begin(),actions.end(),[](auto a,auto b){return a.down!=b.down?!a.down:a.lane<b.lane;});
            for(auto e:actions) {
                if(e.down){observe(t);estimator.KeyDown(e.lane,t);}
                else estimator.KeyUp(e.lane,t);
                continuity.RecordActivity(t);
            }
        }
        if(t%100==0 || t==end)result.points.push_back({double(t),observe(t)});
    }
    peak.Observe(pattern::Snapshot{}); result.summary=peak.Completed();
    return result;
}
bool Same(const pattern::Snapshot& a,const pattern::Snapshot& b) {
    return a.ready==b.ready && a.idle==b.idle && a.retained==b.retained && a.limited==b.limited &&
        a.profile==b.profile && a.samples==b.samples && a.heldKeys==b.heldKeys &&
        a.outsideCalibration==b.outsideCalibration && std::abs(a.rating-b.rating)<1e-10 &&
        std::abs(a.stars-b.stars)<1e-10 && std::abs(a.score-b.score)<1e-10 &&
        std::abs(a.nps-b.nps)<1e-10 && std::abs(a.currentNps-b.currentNps)<1e-10;
}
}

int main() {
    int checks=0,failed=0;
    auto check=[&](const char* name,bool ok){ ++checks; if(!ok) ++failed; std::cout<<name<<": "<<(ok?"ok":"FAIL")<<'\n'; };
    auto make=[](int keys){ chartcalc::Chart c; c.keys=keys; c.title=L"<unsafe> & title"; c.artist=L"A \"quoted\" artist"; for(int i=0;i<240;++i)c.notes.push_back({i%keys,i*100.0,i*100.0+20}); return c; };
    const auto ten=chartcalc::Analyze(make(10));
    check("whole_chart_uses_live_cadence_and_peak",ten.summary.ready && ten.points.size()==241 && ten.points.front().timeMs==0 && ten.points[1].timeMs==100);
    check("summary_matches_live_callbacks",Same(ten.summary,SimulateLive(make(10)).summary));
    for(int keys=4;keys<=10;++keys) {
        auto chart=make(keys);
        for(auto& n:chart.notes) { n.startMs+=1750; n.endMs+=1750; }
        // Long holds, an off-grid early burst, then a long rest and a restart.
        for(std::size_t i=0;i<chart.notes.size();++i) {
            auto& n=chart.notes[i];
            n.startMs=1750+i*73+(i>=100?12000:0);
            n.endMs=n.startMs+(i%5==0?170:10);
        }
        const auto file=chartcalc::Analyze(chart), live=SimulateLive(chart);
        bool equal=Same(file.summary,live.summary) && file.points.size()==live.points.size();
        for(std::size_t i=0;equal && i<file.points.size();++i)
            equal=file.points[i].timeMs==live.points[i].timeMs && Same(file.points[i].snapshot,live.points[i].snapshot);
        check(("live_replay_matches_"+std::to_string(keys)+"k").c_str(),equal);
    }
    check("exact_one_second_nps",ten.maximumNps==10 && std::abs(ten.durationMs-23920)<1e-10);
    auto ending=make(10); ending.notes.resize(221);
    for(int i=0;i<=180;++i) ending.notes.push_back({i%10,22100.0+i*10,22120.0+i*10});
    const auto denseEnd=chartcalc::Analyze(ending);
    double beforeEnding=0;
    for(const auto& p:denseEnd.points) if(p.timeMs<=22000) beforeEnding=std::max(beforeEnding,chartcalc::DifficultyValue(p.snapshot));
    check("off_grid_dense_ending_is_analyzed",denseEnd.points.back().timeMs==23920 && denseEnd.points.back().snapshot.currentNps>90 &&
        chartcalc::DifficultyValue(denseEnd.points.back().snapshot)>beforeEnding && chartcalc::DifficultyValue(denseEnd.summary)>beforeEnding);
    auto exact=make(10); exact.notes.back().endMs=24000;
    const auto aligned=chartcalc::Analyze(exact);
    check("grid_aligned_final_sample_is_not_duplicated",aligned.points.size()==241 && aligned.points.back().timeMs==24000 && aligned.points[239].timeMs==23900);
    check("revive_basis_and_dan_label",chartcalc::Basis(10)==L"10K Revive 2026" && chartcalc::DifficultyLabel(ten.summary).find(L"단")!=std::wstring::npos);
    const auto aery=chartcalc::Analyze(make(5));
    check("aery_uses_adjusted_rating",chartcalc::DifficultyLabel(aery.summary).find(L"⑤")!=std::wstring::npos && aery.summary.rating<10);
    const auto html=chartcalc::RenderReport(ten);
    check("safe_standalone_report",html.find("&lt;unsafe&gt; &amp; title")!=std::string::npos && html.find("<script")==std::string::npos && html.find("https://")==std::string::npos && html.find("default-src 'none'")!=std::string::npos);
    check("report_explains_reference_and_summary",html.find("10K Revive 2026")!=std::string::npos && html.find("실시간 방식의 최고 구간")!=std::string::npos && html.find("단위 합격 예측이 아닙니다")!=std::string::npos);
    auto brief=make(4); brief.notes.resize(30);
    const auto shortResult=chartcalc::Analyze(brief);
    check("four_key_ln_comparison_in_report",chartcalc::RenderReport(shortResult).find("4K LN Dan Courses v2 보조 비교: LN")!=std::string::npos);
    check("baseline_weight_does_not_use_live_ln_reference",chartcalc::RenderReport(chartcalc::Analyze(brief,1.0)).find("4K LN Dan Courses v2 보조 비교:")==std::string::npos);
    check("short_chart_matches_live_warmup",shortResult.points.size()==31 && Same(shortResult.summary,SimulateLive(brief).summary));
    auto warmup=brief; warmup.notes.resize(12);
    check("insufficient_live_warmup_is_not_rated",!chartcalc::Analyze(warmup).summary.ready);
    chartcalc::Chart early; early.keys=10;
    for(int i=0;i<150;++i)early.notes.push_back({i%10,i*20.0,i*20.0+10});
    for(int i=0;i<40;++i)early.notes.push_back({i%10,20000+i*200.0,20010+i*200.0});
    const auto earlyResult=chartcalc::Analyze(early);
    double latePeak=0;
    for(const auto& p:earlyResult.points) if(p.timeMs>=8000 && p.snapshot.ready && !p.snapshot.retained)
        latePeak=std::max(latePeak,chartcalc::DifficultyValue(p.snapshot));
    check("early_peak_survives_long_rest_and_restart",earlyResult.summary.ready &&
        chartcalc::DifficultyValue(earlyResult.summary)>latePeak && Same(earlyResult.summary,SimulateLive(early).summary));
    chartcalc::Chart chords; chords.keys=4;
    for(int i=0;i<100;++i)for(int lane=0;lane<4;++lane)
        chords.notes.push_back({lane,i*83.0,(i+1)*83.0});
    check("simultaneous_release_and_repress_matches_live",Same(chartcalc::Analyze(chords).summary,SimulateLive(chords).summary));
    auto empty=make(4); empty.notes.clear(); bool rejected=false;
    try{chartcalc::Analyze(empty);}catch(const std::runtime_error&){rejected=true;}
    check("empty_chart_rejected",rejected);
    auto tooMany=make(4); tooMany.notes.resize(200001); rejected=false;
    try{chartcalc::Analyze(tooMany);}catch(const std::runtime_error&){rejected=true;}
    check("parser_and_analyzer_share_note_limit",rejected);
    auto tooLong=make(4); tooLong.notes.back().endMs=7200001; rejected=false;
    try{chartcalc::Analyze(tooLong);}catch(const std::runtime_error&){rejected=true;}
    check("parser_and_analyzer_share_duration_limit",rejected);
    auto invalid=make(4); invalid.notes[1].lane=99; rejected=false;
    try{chartcalc::Analyze(invalid);}catch(const std::runtime_error&){rejected=true;}
    check("invalid_lane_rejected",rejected);
    auto overlaps=make(4); overlaps.notes[0].endMs=1000; rejected=false;
    try{chartcalc::Analyze(overlaps);}catch(const std::runtime_error&){rejected=true;}
    check("same_lane_hold_overlap_rejected",rejected);
    std::atomic<bool> cancelled{true}; rejected=false;
    try{chartcalc::Analyze(make(4),pattern::DefaultHoldWeight,&cancelled);}catch(const std::runtime_error&){rejected=true;}
    check("cancellation_stops_analysis",rejected);
    auto holds=make(10); for(auto& n:holds.notes)n.endMs=n.startMs+600;
    const auto live=chartcalc::Analyze(holds), reference=chartcalc::Analyze(holds,1.0);
    check("ln_weight_mode_is_explicit",reference.holdWeight==1.0 && reference.summary.rating>=live.summary.rating && chartcalc::RenderReport(reference).find("기준 채보 측정")!=std::string::npos);
    const auto path=std::filesystem::current_path()/(L"calculator-report-test-"+std::to_wstring(GetCurrentProcessId())+L".html");
    if(std::filesystem::exists(path))return 3;
    chartcalc::SaveReport(ten,path);
    std::ifstream input(path,std::ios::binary); std::string saved((std::istreambuf_iterator<char>(input)),{}); input.close();
    check("manual_report_save_matches_html",saved==html);
    std::filesystem::remove(path);
    std::cout<<"checks="<<checks<<" failures="<<failed<<'\n';
    return failed?1:0;
}
