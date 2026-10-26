#include "analysis.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main() {
    int checks=0,failed=0;
    auto check=[&](const char* name,bool ok){ ++checks; if(!ok) ++failed; std::cout<<name<<": "<<(ok?"ok":"FAIL")<<'\n'; };
    auto make=[](int keys){ chartcalc::Chart c; c.keys=keys; c.title=L"<unsafe> & title"; c.artist=L"A \"quoted\" artist"; for(int i=0;i<240;++i)c.notes.push_back({i%keys,i*100.0,i*100.0+20}); return c; };
    const auto ten=chartcalc::Analyze(make(10));
    check("whole_chart_summary_uses_strongest_three",ten.strongestWindows==3 && ten.summary.ready && ten.points.size()==9);
    std::vector<double> loads; for(const auto& p:ten.points)loads.push_back(chartcalc::DifficultyValue(p.snapshot));
    std::sort(loads.rbegin(),loads.rend());
    check("summary_matches_three_window_average",std::abs(ten.summary.rating-(loads[0]+loads[1]+loads[2])/3)<1e-10);
    check("exact_one_second_nps",ten.maximumNps==10 && std::abs(ten.durationMs-23920)<1e-10);
    auto ending=make(10); ending.notes.resize(221);
    for(int i=0;i<=180;++i) ending.notes.push_back({i%10,22100.0+i*10,22120.0+i*10});
    const auto denseEnd=chartcalc::Analyze(ending);
    double beforeEnding=0;
    for(std::size_t i=0;i+1<denseEnd.points.size();++i) beforeEnding=std::max(beforeEnding,chartcalc::DifficultyValue(denseEnd.points[i].snapshot));
    check("off_grid_dense_ending_is_analyzed",denseEnd.points.back().timeMs==23920 && denseEnd.points.back().snapshot.currentNps>90 &&
        chartcalc::DifficultyValue(denseEnd.points.back().snapshot)>beforeEnding && chartcalc::DifficultyValue(denseEnd.summary)>beforeEnding);
    auto exact=make(10); exact.notes.back().endMs=24000;
    const auto aligned=chartcalc::Analyze(exact);
    check("grid_aligned_final_sample_is_not_duplicated",aligned.points.size()==9 && aligned.points.back().timeMs==24000 && aligned.points[7].timeMs==22000);
    check("revive_basis_and_dan_label",chartcalc::Basis(10)==L"10K Revive 2026" && chartcalc::DifficultyLabel(ten.summary).find(L"단")!=std::wstring::npos);
    const auto aery=chartcalc::Analyze(make(5));
    check("aery_uses_adjusted_rating",chartcalc::DifficultyLabel(aery.summary).find(L"⑤")!=std::wstring::npos && aery.summary.rating<10);
    const auto html=chartcalc::RenderReport(ten);
    check("safe_standalone_report",html.find("&lt;unsafe&gt; &amp; title")!=std::string::npos && html.find("<script")==std::string::npos && html.find("https://")==std::string::npos && html.find("default-src 'none'")!=std::string::npos);
    check("report_explains_reference_and_summary",html.find("10K Revive 2026")!=std::string::npos && html.find("가장 강한 3개 구간")!=std::string::npos && html.find("단위 합격 예측이 아닙니다")!=std::string::npos);
    auto brief=make(4); brief.notes.resize(30);
    const auto shortResult=chartcalc::Analyze(brief);
    check("short_chart_is_honest_about_window_count",shortResult.strongestWindows==1 && shortResult.points.size()==1);
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
