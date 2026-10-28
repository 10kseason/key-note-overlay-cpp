#include "osu_link.h"
#include "osu_link_layout.h"
#include "analysis.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

int main(int argc,char** argv) {
    // Live reads are excluded from CTest. A human must explicitly opt in.
    if(argc>=2 && std::string(argv[1])=="--i-accept-memory-read-risk") {
        osu_link::Session session;session.Start(true);
        const bool watch=argc==3 && std::string(argv[2])=="--watch";
        std::string last;int changes=0;
        std::ofstream out("osu-link-live-result.txt");
        std::cout<<std::unitbuf;
        for(int i=0;i<(watch?100:40);++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            const auto v=session.Current();
            if(v.ready) {
                // Metadata only; no process bytes, addresses or paths are saved.
                const auto text=chartcalc::Utf8(v.title+L" ["+v.variant+L"] · "+v.difficulty);
                if(text!=last) {++changes;std::cout<<"LIVE_READY keys="<<v.keys<<" "<<text<<"\n";out<<text<<"\n";out.flush();last=text;}
                if(!watch) {session.Stop();std::cout<<"STOP_OFF="<<!session.Current().enabled<<"\n";return 0;}
            }
        }
        const auto v=session.Current();
        if(changes) {session.Stop();std::cout<<"CHANGES="<<changes<<" STOP_OFF="<<!session.Current().enabled<<"\n";return 0;}
        out<<chartcalc::Utf8(v.status+L"\n"+v.details);
        std::cout<<"LIVE_NOT_READY\n";return 1;
    }
    int failed=0,count=0;
    auto check=[&](bool pass,const char* name){++count;if(!pass){++failed;std::cerr<<"FAIL "<<name<<"\n";}};
    osu_link::Session session;
    check(!session.Current().enabled && !session.Current().ready && session.Current().reviveAuxiliary.empty(),"default off");
    check(!session.Start(false) && !session.Current().enabled,"consent required");
    session.Stop();check(!session.Current().enabled && session.Current().reviveAuxiliary.empty(),"idempotent stop clears auxiliary");
    for(const auto* s:{L"..",L"../escape",L"..\\escape",L"C:secret",L"a/b",L"a\\b",L"file.",L"file ",L"\nfoo"})
        check(!osu_link::SafeComponent(s),"path component rejection");
    check(osu_link::SafeComponent(L"곡명 [5K].osu"),"unicode component");
    const auto root=std::filesystem::temp_directory_path()/(L"keynote-osu-link-test-"+
        std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root/L"Songs"/L"set");
    {std::ofstream f(root/L"Songs"/L"set"/L"map.osu");f<<"test";}
    check(!osu_link::ResolveChart(root/L"Songs",L"set",L"map.osu").empty(),"contained chart");
    check(osu_link::ResolveChart(root/L"Songs",L"set",L"map.txt").empty(),"osu extension only");
    check(osu_link::ResolveChart(root/L"Songs",L"..",L"map.osu").empty(),"traversal denied");
    for(int keys=1;keys<=revive::MaxKeys;++keys) for(bool hasLn : {false,true}) {
        const auto file=root/L"Songs"/L"set"/L"map.osu";
        {
            std::ofstream out(file);
            out << "osu file format v14\n[General]\nMode:3\n[Metadata]\nTitle:Synthetic\nArtist:Test\nVersion:Normal\n"
                << "[Difficulty]\nCircleSize:" << keys << "\nOverallDifficulty:8\n[HitObjects]\n";
            for(int i=0;i<80;++i) {
                out << ((2*(i%keys)+1)*512/(2*keys)) << ",192," << i*100;
                // Even a short real LN has an explicit tail in the file.
                if(hasLn) out << ",128,0," << i*100+20 << ":0:0:0:0:\n";
                else out << ",1,0,0:0:0:0:\n";
            }
        }
        const auto analysis=chartcalc::Analyze(chartcalc::LoadChart(file));
        const auto skills=revive_aux::Analyze(analysis.chart);
        bool allAxes=skills.ready;
        for(double value:skills.skills)allAxes &= std::isfinite(value) && value>=0 && value<=100;
        check(analysis.chart.keys==keys && analysis.chart.notes.size()==80 && allAxes && skills.holdKnown &&
            (hasLn ? skills.skills[5]>0 : skills.skills[5]==0),
            "1K through 18K files distinguish real LN and LN-free charts");
    }
    std::filesystem::remove(root/L"Songs"/L"set"/L"map.osu");
    std::filesystem::remove(root/L"Songs"/L"set");std::filesystem::remove(root/L"Songs");std::filesystem::remove(root);
    std::vector<unsigned char> memory(0x40000);
    auto put32=[&](unsigned at,unsigned value){std::memcpy(memory.data()+at,&value,4);};
    auto putString=[&](unsigned at,const std::wstring& value){put32(at+4,static_cast<unsigned>(value.size()));std::memcpy(memory.data()+at+8,value.data(),value.size()*sizeof(wchar_t));};
    auto read=[&](unsigned at,void* out,std::size_t size){if(at>memory.size() || size>memory.size()-at)return false;std::memcpy(out,memory.data()+at,size);return true;};
    put32(0x10000,0x11000);put32(0x11078,0x20000);put32(0x11090,0x21000);put32(0x1106c,0x22000);
    putString(0x20000,L"set");putString(0x21000,L"map.osu");putString(0x22000,L"0123456789abcdef0123456789ABCDEF");
    osu_link::layout::Identity identity;
    check(osu_link::layout::Decode(read,0x10000,identity) && identity.file==L"map.osu" && identity.md5==L"0123456789abcdef0123456789abcdef","validated identity");
    put32(0x22004,33);check(!osu_link::layout::Decode(read,0x10000,identity),"bounded hash");
    putString(0x22000,L"0123456789abcdef0123456789abcdeg");check(!osu_link::layout::Decode(read,0x10000,identity),"invalid hash");
    putString(0x22000,L"0123456789abcdef0123456789abcdef");
    int slotReads=0;
    auto changing=[&](unsigned at,void* out,std::size_t size){if(at==0x10000 && ++slotReads==2)return false;return read(at,out,size);};
    check(!osu_link::layout::Decode(changing,0x10000,identity),"torn selection rejected");
    put32(0x21004,1000000);check(!osu_link::layout::Decode(read,0x10000,identity),"bounded string");
    put32(0x10000,0xfffffff0);check(!osu_link::layout::Decode(read,0x10000,identity),"pointer overflow rejected");
    std::cout<<count<<" checks, "<<failed<<" failures\n";
    return failed?1:0;
}
