#include "chart_reader.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

// Independent implementation from format descriptions, not a decoder source port:
// https://bm98.yaneu.com/bm98/bmsformat.html
// https://nvyu.net/rdm/rby_ex.php
// https://osu.ppy.sh/wiki/en/Client/File_formats/osu_(file_format)
namespace chartcalc { namespace {
constexpr std::size_t MaxBytes = 32 * 1024 * 1024, MaxNotes = 200000;
constexpr double MaxTime = 7200000.0;
[[noreturn]] void Fail(const char* text) { throw std::runtime_error(text); }
std::string Trim(std::string text) {
    const auto a = text.find_first_not_of(" \t\r\n"), b = text.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : text.substr(a, b-a+1);
}
std::string Upper(std::string s) { for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); return s; }
double Number(const std::string& raw) {
    const auto s = Trim(raw); std::size_t consumed = 0; double value;
    try { value = std::stod(s, &consumed); } catch (...) { Fail("Invalid numeric chart field"); }
    if (consumed != s.size() || !std::isfinite(value)) Fail("Invalid numeric chart field");
    return value;
}
int Integer(const std::string& text) {
    const double n = Number(text);
    if (n != std::floor(n) || n < -2147483647.0 || n > 2147483647.0) Fail("Expected an integer chart field");
    return static_cast<int>(n);
}
int Code(const std::string& s, int base = 36) {
    if (s.size()!=2) Fail("Invalid BMS object code");
    int value=0;
    for (unsigned char c : s) {
        c = static_cast<unsigned char>(std::toupper(c));
        int n = c>='0' && c<='9' ? c-'0' : c>='A' && c<='Z' ? c-'A'+10 : -1;
        if (n < 0 || n >= base) Fail("Invalid BMS object code");
        value=value*base+n;
    }
    return value;
}
std::wstring DecodeText(const std::string& s, unsigned codePage) {
    if (s.empty()) return {};
    const int n=MultiByteToWideChar(codePage, codePage==CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (!n) Fail("Chart text encoding is invalid");
    std::wstring result(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(codePage, codePage==CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, s.data(), static_cast<int>(s.size()), result.data(), n);
    return result;
}
std::vector<std::string> Split(const std::string& text, char separator) {
    std::vector<std::string> out; std::size_t start=0;
    for (;;) { auto end=text.find(separator,start); out.push_back(text.substr(start,end-start)); if(end==std::string::npos) break; start=end+1; }
    return out;
}
void Validate(Chart& chart) {
    if(chart.notes.empty()) Fail("No playable notes in chart");
    if(chart.notes.size()>MaxNotes) Fail("Chart exceeds 200,000 playable notes");
    std::sort(chart.notes.begin(),chart.notes.end(),[](const Note& a,const Note& b){return a.startMs!=b.startMs ? a.startMs<b.startMs : a.lane<b.lane;});
    std::array<double,18> lastStart{}, lastEnd{}; lastStart.fill(-1); lastEnd.fill(-1);
    for(const auto& n:chart.notes) {
        if(n.lane<0 || n.lane>=chart.keys || n.startMs<0 || n.endMs<n.startMs || n.endMs>MaxTime || !std::isfinite(n.endMs)) Fail("Invalid or over-two-hour note timing");
        if(n.startMs==lastStart[n.lane] || n.startMs<lastEnd[n.lane]) Fail("Overlapping notes in one lane are unsupported");
        lastStart[n.lane]=n.startMs; lastEnd[n.lane]=n.endMs;
    }
}
Chart ReadOsu(const std::string& raw) {
    Chart chart; chart.format="osu!mania";
    std::string section, title, titleUnicode, artist, artistUnicode; int mode=-1;
    std::vector<std::string> objects; std::istringstream stream(raw); std::string line;
    while(std::getline(stream,line)) {
        line=Trim(line); if(line.size()>262144) Fail("Chart line is too long");
        if(line.empty() || line.rfind("//",0)==0) continue;
        if(line.front()=='[') { section=line; continue; }
        if(section=="[HitObjects]") { if(objects.size()>=MaxNotes) Fail("Chart exceeds 200,000 playable notes"); objects.push_back(line); continue; }
        auto colon=line.find(':'); if(colon==std::string::npos) continue;
        const auto key=Trim(line.substr(0,colon)), value=Trim(line.substr(colon+1));
        if(section=="[General]" && key=="Mode") mode=Integer(value);
        if(section=="[Difficulty]" && key=="CircleSize") chart.keys=Integer(value);
        if(section=="[Metadata]") {
            if(key=="Title") title=value;
            if(key=="TitleUnicode") titleUnicode=value;
            if(key=="Artist") artist=value;
            if(key=="ArtistUnicode") artistUnicode=value;
            if(key=="Version") chart.variant=DecodeText(value,CP_UTF8);
        }
    }
    if(mode!=3) Fail("Only osu!mania Mode 3 charts are supported");
    if(chart.keys<1 || chart.keys>18) Fail("osu!mania chart must have 1 to 18 keys");
    chart.title=DecodeText(titleUnicode.empty()?title:titleUnicode,CP_UTF8);
    chart.artist=DecodeText(artistUnicode.empty()?artist:artistUnicode,CP_UTF8);
    for(const auto& object:objects) {
        const auto f=Split(object,','); if(f.size()<5) Fail("Malformed osu! hit object");
        const int x=Integer(f[0]), type=Integer(f[3]); const double start=Number(f[2]);
        if(x<0 || x>512 || type<0 || (type&10)) Fail("Unsupported osu!mania object");
        double end=start;
        if(type&128) { if(f.size()<6 || (type&1)) Fail("Malformed osu! hold"); end=Number(f[5].substr(0,f[5].find(':'))); if(end<=start) Fail("Invalid osu! hold duration"); }
        else if(!(type&1)) Fail("Unsupported osu!mania object");
        chart.notes.push_back({std::min(chart.keys-1,x*chart.keys/512),start,end});
    }
    return chart;
}

struct Position {
    int measure=0, numerator=0, denominator=1;
    bool operator<(const Position& other) const {
        if(measure!=other.measure) return measure<other.measure;
        return static_cast<std::int64_t>(numerator)*other.denominator < static_cast<std::int64_t>(other.numerator)*denominator;
    }
};
struct Object { std::string channel; int value; };
bool PlayChannel(const std::string& ch) { return (ch[0]=='1'||ch[0]=='2'||ch[0]=='5'||ch[0]=='6'); }
Chart ReadBms(const std::string& raw, unsigned encoding) {
    Chart chart; chart.format="BMS";
    if(encoding!=CP_UTF8) chart.warnings.push_back(L"BMS metadata decoded as Shift-JIS (CP932); save as UTF-8 if the title is garbled.");
    std::map<Position,std::vector<Object>> events;
    std::array<double,1000> scale; scale.fill(1);
    std::map<int,double> bpmDefs, stopDefs;
    double initialBpm=130; int lnobj=-1, lnmode=1, lntype=1, player=1;
    std::istringstream stream(raw); std::string line; bool second=false; std::size_t eventCount=0;
    while(std::getline(stream,line)) {
        line=Trim(line); if(line.size()>262144) Fail("Chart line is too long");
        if(line.empty() || line[0]!='#') continue;
        if(line.size()>=7 && std::isdigit(static_cast<unsigned char>(line[1])) && std::isdigit(static_cast<unsigned char>(line[2])) && std::isdigit(static_cast<unsigned char>(line[3])) && line[6]==':') {
            const int measure=Integer(line.substr(1,3)); const auto ch=Upper(line.substr(4,2)); const auto data=Trim(line.substr(7));
            if(ch=="02") { const double value=Number(data); if(value<0 || value>64) Fail("Invalid BMS measure length"); scale[measure]=value; continue; }
            const bool mine=ch[0]=='D'||ch[0]=='E';
            const bool relevant=PlayChannel(ch)||mine||ch=="03"||ch=="08"||ch=="09";
            if(!relevant) {
                // Audio, images and invisible keys do not contribute physical presses.
                if(ch=="01"||ch=="04"||ch=="06"||ch=="07"||ch[0]=='3'||ch[0]=='4') continue;
                if(data.find_first_not_of('0')!=std::string::npos) Fail("Unsupported BMS channel");
                continue;
            }
            if(data.empty() || data.size()%2) Fail("Malformed BMS channel data");
            const int count=static_cast<int>(data.size()/2);
            for(int i=0;i<count;++i) {
                const int value=Code(data.substr(static_cast<std::size_t>(i)*2,2),ch=="03"?16:36); if(!value) continue;
                if(mine) Fail("BMS mines are unsupported");
                if(PlayChannel(ch)) {
                    if(ch[1]<'1'||ch[1]>'5') Fail("Only scratch-free 5K/10K BMS lanes are supported");
                    second |= ch[0]=='2'||ch[0]=='6';
                }
                if(++eventCount>MaxNotes*3) Fail("Too many BMS events");
                const int divisor=std::gcd(i,count);
                auto& at=events[{measure,i/divisor,count/divisor}];
                if(std::any_of(at.begin(),at.end(),[&](const Object& e){return e.channel==ch;})) Fail("Overlapping BMS channel definitions are unsupported");
                at.push_back({ch,value});
            }
            continue;
        }
        auto end=line.find_first_of(" \t",1); const auto key=Upper(line.substr(1,end==std::string::npos?end:end-1));
        const auto value=end==std::string::npos ? "" : Trim(line.substr(end+1));
        if(key=="RANDOM"||key=="SETRANDOM"||key=="IF"||key=="ELSE"||key=="ELSEIF"||key=="ENDIF"||key=="ENDRANDOM"||key=="SWITCH"||key=="SETSWITCH"||key=="CASE"||key=="SKIP"||key=="DEF"||key=="ENDSW") Fail("Branch-dependent BMS charts are unsupported");
        if(key=="TITLE") chart.title=DecodeText(value,encoding);
        else if(key=="ARTIST") chart.artist=DecodeText(value,encoding);
        else if(key=="SUBTITLE") chart.variant=DecodeText(value,encoding);
        else if(key=="BPM") initialBpm=Number(value);
        else if(key.size()==5 && key.rfind("BPM",0)==0) bpmDefs[Code(key.substr(3))]=Number(value);
        else if(key.size()==6 && key.rfind("STOP",0)==0) stopDefs[Code(key.substr(4))]=Number(value);
        else if(key=="LNOBJ") lnobj=Code(value);
        else if(key=="LNMODE") lnmode=Integer(value);
        else if(key=="LNTYPE") lntype=Integer(value);
        else if(key=="PLAYER") player=Integer(value);
        else if(key=="BASE" && Integer(value)!=36) Fail("Non-base36 BMS objects are unsupported");
        else if(key=="STP"||key=="EXBPM"||key=="OCT/FP"||key=="SCROLL"||key=="WARP") Fail("Unsupported BMS timing extension");
    }
    // PLAYER describes game presentation, not the physical key count. Authentic
    // 10K packs use PLAYER 1, 2 and 3; actual playable channels define both hands.
    if(player<1 || player>3) Fail("Unsupported BMS player mode");
    if(lnmode<1 || lnmode>2 || lntype!=1) Fail("Only normal LN and CN (LNMODE 1/2, LNTYPE 1) are supported");
    // High positive BPM values are used for genuine timing gimmicks. Duration
    // and event-count bounds protect analysis without clipping those values.
    if(initialBpm<=0 || initialBpm>100000000) Fail("Invalid BMS BPM");
    for(const auto& p:bpmDefs) if(p.second<=0||p.second>100000000) Fail("Invalid BMS BPM definition");
    for(const auto& p:stopDefs) if(p.second<0||p.second>1000000000000.0) Fail("Invalid BMS STOP definition");
    if(lnobj==0) Fail("LNOBJ 00 is invalid");
    // A declared double-play chart remains 10K even when a sparse chart never
    // uses its second hand. Misdeclared PLAYER 1 files with 2P objects are 10K too.
    chart.keys=second||player==3?10:5;
    if(lnmode==2) chart.warnings.push_back(L"Charge notes are measured as physical holds; release judgement and gauges are not simulated.");
    std::array<double,1001> base{}; for(int i=0;i<1000;++i) base[i+1]=base[i]+4*scale[i];
    std::array<int,10> previous{}, open{}; previous.fill(-1); open.fill(-1);
    double beatBefore=0,time=0,bpm=initialBpm;
    for(const auto& entry:events) {
        const auto& pos=entry.first;
        const double beat=base[pos.measure]+4*scale[pos.measure]*pos.numerator/pos.denominator;
        time+=(beat-beatBefore)*60000/bpm; beatBefore=beat;
        if(time<0||time>MaxTime||!std::isfinite(time)) Fail("BMS timing exceeds two-hour limit");
        // A STOP begins after objects at this position; a new BPM governs that stop.
        double stop=0;
        for(const auto& object:entry.second) if(object.channel=="03") bpm=object.value;
        for(const auto& object:entry.second) {
            // Extended BPM wins over the legacy integer channel when both occur
            // at one position, as in the original Revive NiNi charts.
            if(object.channel=="08") {
                auto found=bpmDefs.find(object.value); if(found==bpmDefs.end()) Fail("Undefined extended BPM"); bpm=found->second;
            } else if(object.channel=="09") { auto found=stopDefs.find(object.value); if(found==stopDefs.end()) Fail("Undefined STOP value"); stop=found->second/48; }
        }
        for(const auto& object:entry.second) {
            const auto& ch=object.channel; if(!PlayChannel(ch)) continue;
            const int lane=(ch[0]=='2'||ch[0]=='6'?5:0)+ch[1]-'1';
            if(ch[0]=='5'||ch[0]=='6') {
                if(open[lane]>=0) {
                    auto& note=chart.notes[static_cast<std::size_t>(open[lane])];
                    if(time<=note.startMs) Fail("Invalid BMS long note duration");
                    note.endMs=time; open[lane]=-1; previous[lane]=-1;
                } else { open[lane]=static_cast<int>(chart.notes.size()); chart.notes.push_back({lane,time,time}); previous[lane]=-1; }
            } else {
                if(open[lane]>=0) Fail("Playable note inside BMS long note");
                if(object.value==lnobj) {
                    if(previous[lane]<0) Fail("BMS LNOBJ tail has no preceding head");
                    auto& note=chart.notes[static_cast<std::size_t>(previous[lane])];
                    if(time<=note.startMs) Fail("Invalid BMS LNOBJ duration");
                    note.endMs=time; previous[lane]=-1;
                } else { previous[lane]=static_cast<int>(chart.notes.size()); chart.notes.push_back({lane,time,time}); }
            }
            if(chart.notes.size()>MaxNotes) Fail("Chart exceeds 200,000 playable notes");
        }
        time+=stop*60000/bpm;
    }
    if(std::any_of(open.begin(),open.end(),[](int i){return i>=0;})) Fail("BMS long note is missing its end");
    return chart;
}
}

Chart LoadChart(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate); if(!in) Fail("Cannot open chart file");
    const auto length=in.tellg(); if(length<=0||length>static_cast<std::streamoff>(MaxBytes)) Fail("Chart must be nonempty and at most 32 MiB");
    std::string raw(static_cast<std::size_t>(length),'\0'); in.seekg(0); if(!in.read(raw.data(),length)) Fail("Cannot read complete chart file");
    if(raw.find('\0')!=std::string::npos) Fail("Binary/UTF-16 chart text is unsupported; convert to UTF-8");
    if(raw.rfind("\xEF\xBB\xBF",0)==0) raw.erase(0,3);
    const auto ext=Upper(path.extension().string());
    const bool utf8=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,raw.data(),static_cast<int>(raw.size()),nullptr,0)!=0;
    Chart chart;
    if(ext==".OSU") { if(!utf8) Fail("osu! charts must use UTF-8 encoding"); chart=ReadOsu(raw); }
    else if(ext==".BMS"||ext==".BME"||ext==".BML") chart=ReadBms(raw,utf8?CP_UTF8:932);
    else Fail("Choose an .osu, .bms, .bme or .bml chart");
    Validate(chart);
    if(chart.title.empty()) chart.title=path.stem().wstring();
    return chart;
}
}
