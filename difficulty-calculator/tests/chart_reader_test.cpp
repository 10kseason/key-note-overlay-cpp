#include "chart_reader.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace fs=std::filesystem;
int main() {
    int argc=0; wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(argc==4 && std::wstring(argv[1])==L"--dump") {
        try {
            auto chart=chartcalc::LoadChart(fs::path(argv[2]));
            std::ofstream out(fs::path(argv[3]),std::ios::binary);
            out<<std::setprecision(15);
            for(auto n:chart.notes) out<<n.lane<<' '<<n.startMs<<' '<<n.endMs<<'\n';
            if(!out) throw std::runtime_error("Cannot write dump");
            std::cout<<"keys="<<chart.keys<<" notes="<<chart.notes.size()<<'\n'; LocalFree(argv); return 0;
        } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; LocalFree(argv); return 1; }
    }
    LocalFree(argv);
    int checks=0,failed=0;
    auto check=[&](const char* name,bool ok){ ++checks; if(!ok) ++failed; std::cout<<name<<": "<<(ok?"ok":"FAIL")<<'\n'; };
    const auto root=fs::current_path()/("parser-fixture-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    fs::create_directory(root);
    auto load=[&](const std::string& text,const std::wstring& name=L"fixture.bms") {
        auto path=root/name; {std::ofstream out(path,std::ios::binary); out<<text;}
        return chartcalc::LoadChart(path);
    };
    auto reject=[&](const std::string& text,const std::wstring& name=L"fixture.bms") {
        try { load(text,name); return false; } catch(const std::exception&) {return true;}
    };
    try {
        const std::string base="#PLAYER 3\n#TITLE synthetic\n#BPM 120\n#LNMODE 2\n#LNTYPE 1\n#LNOBJ ZZ\n";
        auto bms=load(base+"#BPM02 240\n#STOP01 192\n#00011:01\n#00021:01\n#00025:0001\n#00151:0101\n#00165:0101\n#00212:01ZZ\n#00308:02\n#00313:0001\n#00409:01\n#00414:0001\n",L"한글-日本語.bms");
        const std::vector<chartcalc::Note> expected={{0,0,0},{5,0,0},{9,1000,1000},{0,2000,3000},{9,2000,3000},{1,4000,5000},{2,6500,6500},{3,8500,8500}};
        bool same=bms.keys==10 && bms.notes.size()==expected.size();
        for(std::size_t i=0;same && i<expected.size();++i) same=bms.notes[i].lane==expected[i].lane && bms.notes[i].startMs==expected[i].startMs && bms.notes[i].endMs==expected[i].endMs;
        check("unicode_path_two_hands_bpm_stop_lnobj_and_ln_channels",same);
        auto scaled=load("#BPM 120\n#00002:0.5\n#00011:01\n#00103:F0\n#00115:0001\n");
        check("measure_length_and_hex_bpm",scaled.keys==5 && scaled.notes.size()==2 && scaled.notes[1].startMs==1500);
        check("player_one_with_two_hands",load("#PLAYER 1\n#BPM 120\n#00011:01\n#00025:01\n").keys==10);
        check("player_two_uses_physical_channels",load("#PLAYER 2\n#BPM 120\n#00011:01\n#00025:01\n").keys==10);
        check("declared_double_play_keeps_empty_second_hand",load("#PLAYER 3\n#BPM 120\n#00011:01\n").keys==10);
        check("zero_measure_and_high_bpm",load("#BPM 120\n#BPM01 1000158\n#00011:01\n#00102:0\n#00208:01\n#00315:01\n").notes.size()==2);
        auto doubleBpm=load("#BPM 120\n#BPM01 240\n#00008:01\n#00003:78\n#00011:01\n#00115:01\n");
        check("extended_bpm_wins_at_shared_position",doubleBpm.notes[1].startMs==1000);
        const std::string osu="osu file format v14\n[General]\nMode:3\n[Metadata]\nTitle:fixture\nTitleUnicode:合成\n[Difficulty]\nCircleSize:4\n[HitObjects]\n0,192,100,1,0\n512,192,200,128,0,600:0:0:0:0:\n";
        auto mania=load(osu,L"fixture.osu");
        check("osu_mania_metadata_last_lane_and_hold",mania.keys==4 && mania.title==L"合成" && mania.notes.size()==2 && mania.notes[1].lane==3 && mania.notes[1].endMs==600);
        check("reject_scratch",reject(base+"#00016:01\n"));
        check("reject_extra_keys",reject(base+"#00018:01\n"));
        check("reject_mines",reject(base+"#000D1:01\n"));
        check("reject_hcn",reject(base+"#LNMODE 3\n#00051:0101\n"));
        check("reject_random",reject(base+"#RANDOM 2\n#IF 1\n#00011:01\n#ENDIF\n"));
        check("reject_missing_hold_end",reject(base+"#00051:01\n"));
        check("reject_missing_lnobj_head",reject(base+"#00011:ZZ\n"));
        check("reject_undefined_extended_bpm",reject(base+"#00008:01\n#00011:01\n"));
        check("reject_undefined_stop",reject(base+"#00009:01\n#00011:01\n"));
        check("reject_invalid_object_data",reject(base+"#00011:012\n"));
        check("reject_overlapping_channel_rows",reject(base+"#00011:01\n#00011:02\n"));
        check("reject_tap_inside_hold",reject(base+"#00051:010001\n#00011:000100\n"));
        check("reject_non_mania",reject("[General]\nMode:0\n[Difficulty]\nCircleSize:4\n[HitObjects]\n0,0,0,1,0\n",L"bad.osu"));
        check("reject_invalid_osu_key_count",reject("[General]\nMode:3\n[Difficulty]\nCircleSize:4.5\n[HitObjects]\n0,0,0,1,0\n",L"bad.osu"));
        check("reject_nan_time",reject("[General]\nMode:3\n[Difficulty]\nCircleSize:4\n[HitObjects]\n0,0,nan,1,0\n",L"bad.osu"));
        check("reject_binary",reject(std::string("#BPM 120\0#00011:01",19)));
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; ++failed; }
    // Only this invocation's fresh owned fixture folder is removed.
    if(root.parent_path()!=fs::current_path() || root.filename().string().rfind("parser-fixture-",0)!=0) return 2;
    fs::remove_all(root);
    std::cout<<"checks="<<checks<<" failures="<<failed<<'\n';
    return failed?1:0;
}
