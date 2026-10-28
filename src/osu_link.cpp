#include "osu_link.h"
#include "osu_link_layout.h"
#include "analysis.h"
#include "revive_auxiliary.h"
#include "dan_rating.h"
#include <windows.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <vector>

namespace osu_link {
bool SafeComponent(const std::wstring& s) {
    return !s.empty() && s.size()<=255 && s!=L"." && s!=L".." && s.back()!=L'.' && s.back()!=L' ' &&
        std::none_of(s.begin(),s.end(),[](wchar_t c){return c<32 || c==127 || std::wstring(L"\\/:*?\"<>|").find(c)!=std::wstring::npos;});
}
std::filesystem::path ResolveChart(const std::filesystem::path& songs,const std::wstring& folder,const std::wstring& file) {
    if (!SafeComponent(folder) || !SafeComponent(file)) return {};
    auto ext=std::filesystem::path(file).extension().wstring();
    std::transform(ext.begin(),ext.end(),ext.begin(),::towlower);
    if (ext!=L".osu") return {};
    std::error_code ec;
    const auto root=std::filesystem::canonical(songs,ec); if(ec) return {};
    if(root.wstring().rfind(L"\\\\",0)==0 || GetDriveTypeW(root.root_path().c_str())==DRIVE_REMOTE) return {};
    const auto path=std::filesystem::canonical(root/folder/file,ec); if(ec) return {};
    // canonical resolves junctions/symlinks too. Reject a redirect out of Songs.
    const auto relative=path.lexically_relative(root);
    if(relative.empty() || relative.is_absolute() || *relative.begin()==L".." || !std::filesystem::is_regular_file(path,ec) || ec) return {};
    return path;
}
namespace {
struct Handle { HANDLE h=nullptr; ~Handle(){if(h && h!=INVALID_HANDLE_VALUE) CloseHandle(h);} };
class Reader {
    Handle process_;
    std::uint32_t slot_=0;
public:
    std::filesystem::path directory;
    bool Alive() const { DWORD code=0; return process_.h && GetExitCodeProcess(process_.h,&code) && code==STILL_ACTIVE; }
    bool Read(std::uint32_t at,void* out,std::size_t count) {
        SIZE_T copied=0;
        return ReadProcessMemory(process_.h,reinterpret_cast<void*>(static_cast<uintptr_t>(at)),out,count,&copied) && copied==count;
    }
    std::wstring Connect() {
        Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)};
        if(snapshot.h==INVALID_HANDLE_VALUE) return L"프로세스 목록을 확인하지 못했습니다.";
        PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry); DWORD pid=0;
        if(Process32FirstW(snapshot.h,&entry)) do {
            if(_wcsicmp(entry.szExeFile,L"osu!.exe")==0) {
                if(pid) return L"osu!가 여러 개 실행 중입니다. 한 개만 실행해 주세요.";
                pid=entry.th32ProcessID;
            }
        } while(Process32NextW(snapshot.h,&entry));
        if(!pid) return L"osu! stable 대기 · lazer는 지원하지 않습니다.";
        // Never request write/operation/debug rights, elevate, inject, or attach a debugger.
        process_.h=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid);
        if(!process_.h) return L"osu! 읽기 권한이 없습니다. 연동을 중지할 수 있습니다.";
        BOOL wow=FALSE;
        if(!IsWow64Process(process_.h,&wow) || !wow) return L"32비트 osu! stable만 지원합니다.";
        wchar_t path[32768]{}; DWORD size=ARRAYSIZE(path);
        if(!QueryFullProcessImageNameW(process_.h,0,path,&size)) return L"osu! 실행 경로를 확인하지 못했습니다.";
        directory=std::filesystem::path(std::wstring(path,size)).parent_path();
        return {};
    }
    bool Selection(layout::Identity& identity) {
        auto read=[&](std::uint32_t at,void* out,std::size_t size){return Read(at,out,size);};
        return slot_ && layout::Decode(read,slot_,identity) && SafeComponent(identity.folder) && SafeComponent(identity.file);
    }
    bool Locate(const std::atomic<bool>& cancelled) {
        slot_=0;
        const auto deadline=GetTickCount64()+2500;
        std::vector<unsigned char> buffer(1024*1024);
        std::vector<std::uint32_t> candidates;
        std::uint64_t address=0x10000, total=0;
        while(address<0x100000000ULL && !cancelled && GetTickCount64()<deadline) {
            MEMORY_BASIC_INFORMATION info{};
            if(!VirtualQueryEx(process_.h,reinterpret_cast<void*>(static_cast<uintptr_t>(address)),&info,sizeof(info))) return false;
            const auto start=reinterpret_cast<uintptr_t>(info.BaseAddress), end=std::min<std::uint64_t>(0x100000000ULL,start+info.RegionSize);
            if(end<=address) return false;
            const DWORD protection=info.Protect&0xff;
            // The anchor is JIT code. Avoid scanning ordinary data heaps, chat,
            // credentials, or input buffers; never retain or log scanned bytes.
            if(info.State==MEM_COMMIT && info.Type==MEM_PRIVATE && !(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) &&
                (protection==PAGE_EXECUTE_READ || protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY)) {
                for(std::uint64_t at=start;at<end && !cancelled; ) {
                    if(GetTickCount64()>=deadline || total>=256ULL*1024*1024) return false;
                    const auto size=static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(),end-at)); total+=size;
                    if(Read(static_cast<std::uint32_t>(at),buffer.data(),size)) {
                        auto found=buffer.begin(); const auto stop=buffer.begin()+size;
                        while((found=std::search(found,stop,layout::Anchor.begin(),layout::Anchor.end()))!=stop) {
                            const auto hit=at+static_cast<std::size_t>(found-buffer.begin()); std::uint32_t slot=0;
                            if(hit>=12 && Read(static_cast<std::uint32_t>(hit-12),&slot,4)) {
                                layout::Identity identity;
                                auto read=[&](std::uint32_t address,void* out,std::size_t count){return Read(address,out,count);};
                                if(layout::Decode(read,slot,identity) && SafeComponent(identity.folder) && SafeComponent(identity.file)) candidates.push_back(slot);
                            }
                            ++found;
                        }
                    }
                    if(size<=layout::Anchor.size()) break;
                    at+=size-(layout::Anchor.size()-1);
                }
            }
            address=end;
        }
        if(address<0x100000000ULL || cancelled) return false; // never accept a partial scan
        std::sort(candidates.begin(),candidates.end()); candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
        slot_=candidates.size()==1?candidates.front():0;
        return slot_!=0 && !cancelled;
    }
};
std::wstring Md5(const std::filesystem::path& path) {
    HCRYPTPROV provider=0; HCRYPTHASH hash=0;
    if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) return {};
    if(!CryptCreateHash(provider,CALG_MD5,0,0,&hash)) {CryptReleaseContext(provider,0);return {};}
    std::ifstream file(path,std::ios::binary); char bytes[65536]; std::uint64_t total=0; bool ok=!!file;
    while(ok && file) {
        file.read(bytes,sizeof(bytes)); const auto count=file.gcount(); total+=count;
        if(total>32ULL*1024*1024 || (count && !CryptHashData(hash,reinterpret_cast<BYTE*>(bytes),static_cast<DWORD>(count),0))) ok=false;
    }
    BYTE digest[16]{}; DWORD length=16; std::wstring result;
    if(ok && file.eof() && CryptGetHashParam(hash,HP_HASHVAL,digest,&length,0)) {
        for(BYTE b:digest) {result+=L"0123456789abcdef"[b>>4];result+=L"0123456789abcdef"[b&15];}
    }
    CryptDestroyHash(hash);CryptReleaseContext(provider,0);return result;
}
}
Session::~Session(){Stop();}
View Session::Current() const {std::lock_guard<std::mutex> lock(mutex_);return view_;}
void Session::Publish(View value) {std::lock_guard<std::mutex> lock(mutex_);if(!cancelled_) view_=std::move(value);}
bool Session::Start(bool riskAccepted,std::filesystem::path songsOverride) {
    if(!riskAccepted) return false;
    Stop();cancelled_=false;
    View start;start.enabled=true;start.status=L"osu! stable 연결 확인 중…";Publish(start);
    try {worker_=std::thread(&Session::Run,this,std::move(songsOverride));}
    catch(...) {Stop();return false;}
    return true;
}
void Session::Stop() {
    cancelled_=true;wake_.notify_all();if(worker_.joinable())worker_.join();
    std::lock_guard<std::mutex> lock(mutex_);view_=View{};
}
void Session::Run(std::filesystem::path songsOverride) {
    std::unique_ptr<Reader> reader;
    layout::Identity previous; View cached; std::filesystem::path previousPath;
    std::filesystem::file_time_type previousTime{}; ULONGLONG nextLocate=0;
    while(!cancelled_) {
        View next;next.enabled=true;
        try {
            if(!reader || !reader->Alive()) {
                reader=std::make_unique<Reader>();const auto error=reader->Connect();
                previous={};previousPath.clear();nextLocate=0;
                if(!error.empty()) {next.status=error;reader.reset();}
            }
            if(reader) {
                layout::Identity identity;
                bool selected=reader->Selection(identity);
                if(!selected && GetTickCount64()>=nextLocate) {
                    nextLocate=GetTickCount64()+10000;
                    selected=reader->Locate(cancelled_) && reader->Selection(identity);
                }
                if(!selected) {next.status=L"선택 채보를 확인하지 못했습니다. 선곡 화면에서 기다려 주세요. 업데이트 후에는 호환되지 않을 수 있습니다.";previous={};}
                else {
                    const auto songs=songsOverride.empty()?reader->directory/L"Songs":songsOverride;
                    const auto path=ResolveChart(songs,identity.folder,identity.file);
                    std::error_code ec;const auto time=path.empty()?std::filesystem::file_time_type{}:std::filesystem::last_write_time(path,ec);
                    if(path.empty() || ec) {next.status=L"선택 채보 파일을 찾지 못했습니다. Songs 폴더 위치를 지정해 주세요.";previous={};}
                    else if(identity==previous && path==previousPath && time==previousTime && cached.ready) next=cached;
                    else {
                        next.status=L"선택한 채보 분석 중…";Publish(next); // remove the old song before slow work
                        if(Md5(path)!=identity.md5) {next.status=L"게임의 채보와 파일 내용이 다릅니다. osu!에서 채보를 새로고침해 주세요.";previous={};}
                        else {
                            auto analysis=chartcalc::Analyze(chartcalc::LoadChart(path),pattern::DefaultHoldWeight,&cancelled_);
                            const auto auxiliary=revive_aux::Analyze(analysis.chart,&cancelled_);
                            layout::Identity after;
                            if(!reader->Alive() || !reader->Selection(after) || after!=identity || Md5(path)!=identity.md5) {
                                next.status=L"선택이 바뀌었습니다. 새 채보 확인 중…";previous={};
                            } else {
                                next.ready=true;next.keys=analysis.chart.keys;next.title=analysis.chart.title;
                                next.variant=analysis.chart.variant;next.difficulty=chartcalc::DifficultyLabel(analysis.summary);
                                next.reviveAuxiliary=revive_aux::Label(auxiliary);
                                next.reviveCompact=revive_aux::Label(auxiliary,true);
                                next.reviveSkills=revive_aux::Details(auxiliary);
                                next.revive=auxiliary;
                                next.lnDifficulty=pattern::FormatLnDan(analysis.summary);
                                next.status=L"연동 ON · 원본 채보 1.00× / 게임 모드 변경 미반영";
                                next.details=chartcalc::Basis(next.keys)+(next.lnDifficulty.empty()?L"":L" · "+next.lnDifficulty)+L"\n"+std::to_wstring(analysis.chart.notes.size())+L" 노트 · 최대 NPS "+chartcalc::Number(analysis.maximumNps,0)+
                                    L" · 실시간 방식 최고 구간";
                                if(analysis.limited) next.details+=L"\n분석 한도 초과: 난이도 보류";
                                previous=identity;previousPath=path;previousTime=time;cached=next;
                            }
                        }
                    }
                }
            }
        } catch(const std::exception& error) {
            previous={};next=View{};next.enabled=true;
            next.status=std::string(error.what())=="Only osu!mania Mode 3 charts are supported"?
                L"선택한 원본은 osu!mania 채보가 아닙니다. 변환 채보는 지원하지 않습니다.":
                L"채보를 분석하지 못했습니다. 지원하지 않는 채보이거나 읽기 오류입니다.";
        } catch(...) {previous={};next=View{};next.enabled=true;next.status=L"채보 분석 오류 · 연동을 껐다 켜서 다시 시도해 주세요.";}
        Publish(std::move(next));
        std::unique_lock<std::mutex> lock(mutex_);wake_.wait_for(lock,std::chrono::milliseconds(750),[&]{return cancelled_.load();});
    }
}
}
