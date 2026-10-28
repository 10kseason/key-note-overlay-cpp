#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include "analysis.h"
#include "dan_rating.h"
#include <algorithm>
#include <atomic>
#include <fstream>
#include <memory>
#include <thread>

namespace {
constexpr UINT Ready = WM_APP + 1;
constexpr int OpenId=101, SaveId=102, ModeId=103;
constexpr COLORREF Background=RGB(14,22,35), Card=RGB(23,35,53), Foreground=RGB(232,240,250), Muted=RGB(159,178,201), Cyan=RGB(103,221,235), Purple=RGB(194,164,255);
struct Outcome { std::unique_ptr<chartcalc::Analysis> analysis; std::wstring error; };
HWND window=nullptr, openButton=nullptr, saveButton=nullptr, modeBox=nullptr;
HFONT normalFont=nullptr, smallFont=nullptr, titleFont=nullptr, metricFont=nullptr;
std::thread worker;
std::atomic<bool> closing{false};
std::unique_ptr<chartcalc::Analysis> analysis;
std::wstring status=L"채보 파일을 열어 구간별 난이도와 NPS를 확인하세요.";
std::filesystem::path lastPath;
bool busy=false;
bool selfTesting=false;
int hover=-1;
HBRUSH backgroundBrush=nullptr;

RECT GraphRect(int width, int height, bool difficulty) {
    const int top=282, graphHeight=std::max(110,(height-top-104)/2);
    return {28, top+(difficulty?graphHeight+28:0), width-28, top+(difficulty?graphHeight+28:0)+graphHeight};
}
void Text(HDC dc, const std::wstring& text, RECT rect, HFONT font, COLORREF color, UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS) {
    auto old=SelectObject(dc,font); SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,text.c_str(),static_cast<int>(text.size()),&rect,flags);
    SelectObject(dc,old);
}
void Panel(HDC dc, RECT rect) { HBRUSH brush=CreateSolidBrush(Card); FillRect(dc,&rect,brush); DeleteObject(brush); }
void DrawGraph(HDC dc, RECT rect, bool difficulty) {
    Panel(dc,rect);
    Text(dc,difficulty?L"난이도 비교값 · 마우스를 올리면 단위 표시":L"1초 NPS · 0.1초 간격 표본",{rect.left+16,rect.top+6,rect.right-12,rect.top+34},smallFont,Muted);
    const RECT plot{rect.left+48,rect.top+44,rect.right-18,rect.bottom-24};
    double maximum=1;
    if (analysis) for (const auto& point:analysis->points)
        maximum=std::max(maximum,difficulty?chartcalc::DifficultyValue(point.snapshot):point.snapshot.currentNps);
    HPEN grid=CreatePen(PS_SOLID,1,RGB(45,64,87)); auto oldPen=SelectObject(dc,grid);
    for (int i=0;i<=4;++i) {
        const int y=plot.top+(plot.bottom-plot.top)*i/4;
        MoveToEx(dc,plot.left,y,nullptr); LineTo(dc,plot.right,y);
        Text(dc,chartcalc::Number(maximum*(1-i/4.0),1),{rect.left+3,y-9,plot.left-7,y+10},smallFont,Muted,DT_RIGHT|DT_SINGLELINE);
    }
    SelectObject(dc,oldPen); DeleteObject(grid);
    if (!analysis || analysis->points.empty()) return;
    HPEN trace=CreatePen(PS_SOLID,2,difficulty?Purple:Cyan); oldPen=SelectObject(dc,trace);
    HBRUSH dot=CreateSolidBrush(difficulty?Purple:Cyan); auto oldBrush=SelectObject(dc,dot);
    bool connected=false;
    for (const auto& point:analysis->points) {
        const bool valid=!difficulty || (point.snapshot.ready && !point.snapshot.idle && !point.snapshot.limited);
        const double value=difficulty?chartcalc::DifficultyValue(point.snapshot):point.snapshot.currentNps;
        int x=plot.left+static_cast<int>((plot.right-plot.left)*point.timeMs/std::max(1.0,analysis->durationMs));
        int y=plot.bottom-static_cast<int>((plot.bottom-plot.top)*value/maximum);
        if (valid) { if (connected) LineTo(dc,x,y); else MoveToEx(dc,x,y,nullptr); Ellipse(dc,x-2,y-2,x+2,y+2); }
        connected=valid;
    }
    if (hover>=0 && static_cast<std::size_t>(hover)<analysis->points.size()) {
        const int x=plot.left+static_cast<int>((plot.right-plot.left)*analysis->points[hover].timeMs/std::max(1.0,analysis->durationMs));
        MoveToEx(dc,x,plot.top,nullptr); LineTo(dc,x,plot.bottom);
    }
    SelectObject(dc,oldPen); DeleteObject(trace); SelectObject(dc,oldBrush); DeleteObject(dot);
    Text(dc,L"0초",{plot.left,plot.bottom+4,plot.left+60,rect.bottom},smallFont,Muted);
    Text(dc,chartcalc::Number(analysis->durationMs/1000)+L"초",{plot.right-100,plot.bottom+4,plot.right,rect.bottom},smallFont,Muted,DT_RIGHT|DT_SINGLELINE);
}
void Paint(HDC dc, int width, int height) {
    RECT all{0,0,width,height}; FillRect(dc,&all,backgroundBrush);
    Text(dc,L"채보 난이도 계산기",{28,18,width-28,55},titleFont,Foreground);
    Text(dc,L"파일에서 읽은 패턴 · 외부 통신 없음",{30,56,width-28,80},smallFont,Muted);
    const std::wstring title=analysis?analysis->chart.title:L"아직 선택한 채보가 없습니다";
    Text(dc,title,{28,128,width-28,163},titleFont,Foreground);
    std::wstring subtitle=analysis?analysis->chart.artist+L"  ·  "+analysis->chart.variant+L"  ·  "+std::to_wstring(analysis->chart.keys)+L"K":L"osu!mania / 스크래치 없는 BMS 채보를 선택하세요.";
    Text(dc,subtitle,{29,164,width-28,189},smallFont,Muted);
    Panel(dc,{28,198,width-28,265});
    Text(dc,analysis?chartcalc::DifficultyLabel(analysis->summary):L"분석 대기",{44,205,width/2,244},metricFont,Cyan);
    std::wstring basis = analysis ? L"실시간 방식의 최고 구간 · " + chartcalc::Basis(analysis->chart.keys) : L"최근 최대 8초 창을 실시간과 같은 방식으로 비교합니다.";
    if (analysis && analysis->holdWeight == pattern::DefaultHoldWeight) {
        const auto ln = pattern::FormatLnDan(analysis->summary);
        if (!ln.empty()) basis += L" · 4K LN v2: " + ln;
    }
    Text(dc,basis,{44,240,width-40,263},smallFont,Muted);
    if (analysis) {
        const auto& a=*analysis;
        std::wstring metrics=std::to_wstring(a.chart.notes.size())+L" 노트   "+chartcalc::Number(a.durationMs/1000)+L"초   최대 NPS "+chartcalc::Number(a.maximumNps,0);
        Text(dc,metrics,{width/2,209,width-46,242},normalFont,Foreground,DT_RIGHT|DT_SINGLELINE|DT_END_ELLIPSIS);
    }
    DrawGraph(dc,GraphRect(width,height,false),false);
    DrawGraph(dc,GraphRect(width,height,true),true);
    std::wstring info=status;
    if (analysis && hover>=0 && static_cast<std::size_t>(hover)<analysis->points.size()) {
        const auto& p=analysis->points[hover];
        info=chartcalc::Number(std::max(0.0,p.timeMs-8000)/1000)+L"–"+chartcalc::Number(p.timeMs/1000)+L"초 · "+chartcalc::DifficultyLabel(p.snapshot)+L" · NPS "+chartcalc::Number(p.snapshot.currentNps,0);
    }
    Text(dc,info,{28,height-65,width-28,height-36},smallFont,busy?Cyan:Muted);
    Text(dc,L"공식 곡 등급·단위 합격 예측이 아닙니다. 원본 채보와 음악은 변경하거나 실행하지 않습니다.",{28,height-34,width-28,height-10},smallFont,Muted);
}
void Layout() {
    RECT r{}; GetClientRect(window,&r);
    MoveWindow(openButton,28,91,126,29,TRUE);
    MoveWindow(saveButton,164,91,152,29,TRUE);
    MoveWindow(modeBox,330,91,std::max(260L,std::min(430L,r.right-358)),250,TRUE);
    InvalidateRect(window,nullptr,FALSE);
}
bool FileDialog(bool save, std::filesystem::path& path) {
    wchar_t buffer[32768]{};
    if (save) wcscpy_s(buffer,ARRAYSIZE(buffer),L"chart-analysis.html");
    OPENFILENAMEW ofn{}; ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=window; ofn.lpstrFile=buffer; ofn.nMaxFile=ARRAYSIZE(buffer);
    ofn.lpstrTitle=save?L"분석 리포트 저장":L"채보 파일 선택";
    ofn.lpstrFilter=save?L"HTML 리포트\0*.html\0\0":L"지원 채보\0*.osu;*.bms;*.bme;*.bml\0osu!mania\0*.osu\0BMS\0*.bms;*.bme;*.bml\0\0";
    ofn.lpstrDefExt=save?L"html":nullptr;
    ofn.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    const bool ok=save?GetSaveFileNameW(&ofn):GetOpenFileNameW(&ofn);
    if (ok) path=buffer;
    else if (CommDlgExtendedError()) MessageBoxW(window,L"파일 선택 창을 열지 못했습니다.",L"파일 선택 오류",MB_ICONERROR);
    return ok;
}
void Start(const std::filesystem::path& path) {
    if (busy) return;
    if (worker.joinable()) worker.join();
    busy=true; hover=-1; lastPath=path;
    status=L"채보를 읽고 분석하는 중입니다…";
    EnableWindow(openButton,FALSE); EnableWindow(saveButton,FALSE); EnableWindow(modeBox,FALSE);
    InvalidateRect(window,nullptr,FALSE);
    const double weight=SendMessageW(modeBox,CB_GETCURSEL,0,0)==1?1.0:pattern::DefaultHoldWeight;
    worker=std::thread([path,weight] {
        auto outcome=std::make_unique<Outcome>();
        try { outcome->analysis=std::make_unique<chartcalc::Analysis>(chartcalc::Analyze(chartcalc::LoadChart(path),weight,&closing)); }
        catch(const std::exception& e) { outcome->error=chartcalc::Wide(e.what()); }
        catch(...) { outcome->error=L"예상하지 못한 분석 오류가 발생했습니다."; }
        if (!closing.load()) {
            auto pointer=outcome.release();
            if (!PostMessageW(window,Ready,0,reinterpret_cast<LPARAM>(pointer))) delete pointer;
        }
    });
}
LRESULT CALLBACK Procedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
    switch(message) {
    case WM_CREATE:
        window=hwnd;
        openButton=CreateWindowW(L"BUTTON",L"채보 열기…",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,0,0,hwnd,reinterpret_cast<HMENU>(OpenId),nullptr,nullptr);
        saveButton=CreateWindowW(L"BUTTON",L"HTML 리포트 저장…",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,0,0,hwnd,reinterpret_cast<HMENU>(SaveId),nullptr,nullptr);
        modeBox=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,0,0,0,0,hwnd,reinterpret_cast<HMENU>(ModeId),nullptr,nullptr);
        for(HWND control:{openButton,saveButton,modeBox}) SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(normalFont),TRUE);
        SendMessageW(modeBox,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"오버레이와 동일 · LN 추가 부담 2/3"));
        SendMessageW(modeBox,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"기준 채보 측정 · LN 추가 부담 1"));
        SendMessageW(modeBox,CB_SETCURSEL,0,0); EnableWindow(saveButton,FALSE); Layout(); return 0;
    case WM_COMMAND:
        if(LOWORD(wParam)==OpenId) { std::filesystem::path path; if(FileDialog(false,path)) Start(path); return 0; }
        if(LOWORD(wParam)==ModeId && HIWORD(wParam)==CBN_SELCHANGE && !lastPath.empty()) { Start(lastPath); return 0; }
        if(LOWORD(wParam)==SaveId && analysis) {
            std::filesystem::path path;
            if(FileDialog(true,path)) {
                try { chartcalc::SaveReport(*analysis,path); status=L"HTML 리포트를 저장했습니다. 곡명·제작자 메타데이터가 포함됩니다."; }
                catch(const std::exception& e) { MessageBoxW(hwnd,chartcalc::Wide(e.what()).c_str(),L"저장 실패",MB_ICONERROR); }
                InvalidateRect(hwnd,nullptr,FALSE);
            }
            return 0;
        }
        break;
    case Ready: {
        std::unique_ptr<Outcome> outcome(reinterpret_cast<Outcome*>(lParam));
        if(worker.joinable()) worker.join();
        busy=false;
        EnableWindow(openButton,TRUE); EnableWindow(modeBox,TRUE);
        if(outcome->analysis) {
            analysis=std::move(outcome->analysis);
            status=analysis->limited?L"분석 버퍼 초과 구간이 있어 요약 난이도를 보류했습니다.":L"분석 완료 · 그래프에 마우스를 올리면 구간별 등급을 볼 수 있습니다.";
            if(!analysis->chart.warnings.empty()) status=analysis->chart.warnings.front();
        } else {
            analysis.reset(); status=L"분석하지 못했습니다: "+outcome->error;
            if(!selfTesting) MessageBoxW(hwnd,outcome->error.c_str(),L"채보 분석 오류",MB_ICONWARNING);
        }
        EnableWindow(saveButton,analysis!=nullptr); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    }
    case WM_MOUSEMOVE:
        if(analysis && !busy) {
            RECT r{}; GetClientRect(hwnd,&r); int newHover=-1;
            for(bool difficulty:{false,true}) {
                const auto box=GraphRect(r.right,r.bottom,difficulty); POINT p{GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
                if(PtInRect(&box,p) && !analysis->points.empty()) {
                    const double t=std::clamp((p.x-box.left-48.0)/std::max(1L,box.right-box.left-66),0.0,1.0)*analysis->durationMs;
                    auto found=std::lower_bound(analysis->points.begin(),analysis->points.end(),t,[](const auto& point,double time){return point.timeMs<time;});
                    newHover=static_cast<int>(found-analysis->points.begin());
                    if(newHover==static_cast<int>(analysis->points.size())) --newHover;
                    if(newHover>0 && t-analysis->points[newHover-1].timeMs<analysis->points[newHover].timeMs-t) --newHover;
                }
            }
            if(newHover!=hover) { hover=newHover; InvalidateRect(hwnd,nullptr,FALSE); }
        }
        return 0;
    case WM_MEASUREITEM:
        if(wParam==ModeId) { reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight=22; return TRUE; }
        break;
    case WM_DRAWITEM:
        if(wParam==ModeId) {
            auto& item=*reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            HBRUSH brush=CreateSolidBrush(item.itemState&ODS_SELECTED?RGB(37,64,86):Card);
            FillRect(item.hDC,&item.rcItem,brush); DeleteObject(brush);
            int index=item.itemID==static_cast<UINT>(-1)?static_cast<int>(SendMessageW(modeBox,CB_GETCURSEL,0,0)):static_cast<int>(item.itemID);
            wchar_t text[128]{};
            if(index>=0 && SendMessageW(modeBox,CB_GETLBTEXTLEN,index,0)<static_cast<LRESULT>(ARRAYSIZE(text))) SendMessageW(modeBox,CB_GETLBTEXT,index,reinterpret_cast<LPARAM>(text));
            RECT label=item.rcItem; label.left+=7;
            Text(item.hDC,text,label,normalFont,Foreground);
            if(item.itemState&ODS_FOCUS) DrawFocusRect(item.hDC,&item.rcItem);
            return TRUE;
        }
        break;
    case WM_SIZE: Layout(); return 0;
    case WM_GETMINMAXINFO: reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize={820,660}; return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{}; HDC dc=BeginPaint(hwnd,&paint); RECT rect{}; GetClientRect(hwnd,&rect);
        HDC memory=CreateCompatibleDC(dc); HBITMAP bitmap=CreateCompatibleBitmap(dc,std::max(1L,rect.right),std::max(1L,rect.bottom));
        auto old=SelectObject(memory,bitmap); Paint(memory,rect.right,rect.bottom); BitBlt(dc,0,0,rect.right,rect.bottom,memory,0,0,SRCCOPY);
        SelectObject(memory,old); DeleteObject(bitmap); DeleteDC(memory); EndPaint(hwnd,&paint); return 0;
    }
    case WM_CLOSE:
        closing.store(true);
        if(worker.joinable()) worker.join();
        { MSG message{}; while(PeekMessageW(&message,hwnd,Ready,Ready,PM_REMOVE)) delete reinterpret_cast<Outcome*>(message.lParam); }
        DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,message,wParam,lParam);
}
bool DumpPreview(const std::filesystem::path& path) {
    const int width=1020,height=760;
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=width; info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr; HDC dc=CreateCompatibleDC(nullptr); HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if(!bitmap) { DeleteDC(dc); return false; }
    auto old=SelectObject(dc,bitmap); Paint(dc,width,height);
    for(HWND control:{openButton,saveButton,modeBox}) {
        RECT rect{}; GetWindowRect(control,&rect); MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&rect),2);
        const int state=SaveDC(dc); SetViewportOrgEx(dc,rect.left,rect.top,nullptr);
        SendMessageW(control,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
        RestoreDC(dc,state);
        if(control==modeBox) {
            // A hidden combo's WM_PRINT omits its owner-drawn selection. Exercise
            // the same drawing handler explicitly; this is not a screen capture.
            DRAWITEMSTRUCT item{}; item.CtlType=ODT_COMBOBOX; item.CtlID=ModeId;
            item.itemID=static_cast<UINT>(SendMessageW(modeBox,CB_GETCURSEL,0,0));
            item.itemAction=ODA_DRAWENTIRE; item.hwndItem=modeBox; item.hDC=dc;
            item.rcItem={rect.left+3,rect.top+3,rect.right-21,rect.bottom-3};
            SendMessageW(window,WM_DRAWITEM,ModeId,reinterpret_cast<LPARAM>(&item));
        }
    }
    GdiFlush();
    BITMAPFILEHEADER header{}; header.bfType=0x4d42; header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader); header.bfSize=header.bfOffBits+width*height*4;
    std::ofstream out(path,std::ios::binary); out.write(reinterpret_cast<const char*>(&header),sizeof(header)); out.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader)); out.write(static_cast<const char*>(pixels),width*height*4);
    const bool ok=out.good(); out.close(); SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); return ok;
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show) {
    SetProcessDPIAware();
    normalFont=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    smallFont=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    titleFont=CreateFontW(-25,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    metricFont=CreateFontW(-29,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    backgroundBrush=CreateSolidBrush(Background);
    WNDCLASSW klass{}; klass.lpfnWndProc=Procedure; klass.hInstance=instance; klass.lpszClassName=L"ChartDifficultyCalculatorWindow"; klass.hCursor=LoadCursor(nullptr,IDC_ARROW); klass.hIcon=LoadIcon(nullptr,IDI_APPLICATION); RegisterClassW(&klass);
    HWND hwnd=CreateWindowExW(0,klass.lpszClassName,L"채보 난이도 계산기 v0.1.0",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1040,810,nullptr,nullptr,instance,nullptr);
    if(!hwnd) return 2;
    int argc=0; auto argv=CommandLineToArgvW(GetCommandLineW(),&argc); int code=0;
    if(argc>1 && std::wstring(argv[1])==L"--self-test") {
        selfTesting=true;
        const auto fixture=std::filesystem::current_path()/(L"calculator-ui-fixture-"+std::to_wstring(GetCurrentProcessId())+L".osu");
        if(std::filesystem::exists(fixture)) return 3;
        std::ofstream source(fixture,std::ios::binary);
        source << "osu file format v14\n[General]\nMode:3\n[Metadata]\nTitle:" << chartcalc::Utf8(L"합성 채보 · 실제 플레이 아님") << "\nArtist:Synthetic fixture\nVersion:" << chartcalc::Utf8(L"단계적으로 증가하는 밀도") << "\n[Difficulty]\nCircleSize:10\n[HitObjects]\n";
        double time=0;
        for(int i=0;i<500;++i) { source << (i%10*512+256)/10 << ",192," << static_cast<int>(time) << ",128,0," << static_cast<int>(time+20) << ":0:0:0:0:\n"; time+=i<160?150:i<340?90:60; }
        source.close();
        auto waitForAnalysis=[&] {
            const auto deadline=GetTickCount64()+10000;
            while(busy && GetTickCount64()<deadline) {
                MSG event{}; while(PeekMessageW(&event,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&event); DispatchMessageW(&event); }
                if(busy) MsgWaitForMultipleObjects(0,nullptr,FALSE,20,QS_ALLINPUT);
            }
            return !busy;
        };
        Start(fixture);
        const bool controlsDisabled=busy && !IsWindowEnabled(openButton) && !IsWindowEnabled(saveButton) && !IsWindowEnabled(modeBox);
        bool ok=waitForAnalysis() && analysis && !analysis->points.empty() && analysis->summary.ready;
        ok=ok && analysis->holdWeight==pattern::DefaultHoldWeight && IsWindowEnabled(saveButton) && IsWindowEnabled(openButton);
        SendMessageW(modeBox,CB_SETCURSEL,1,0);
        SendMessageW(hwnd,WM_COMMAND,MAKEWPARAM(ModeId,CBN_SELCHANGE),reinterpret_cast<LPARAM>(modeBox));
        ok=ok && waitForAnalysis() && analysis && analysis->holdWeight==1.0 && controlsDisabled && !IsWindowVisible(hwnd);
        std::ofstream log("calculator-ui-selftest.log"); log << "hidden_window=" << (!IsWindowVisible(hwnd)) << "\nworker_controls_disabled=" << controlsDisabled << "\nresult=" << (ok?"ok":"fail") << '\n';
        if(argc>2 && !DumpPreview(argv[2])) code=1;
        if(!ok) code=1;
        SendMessageW(hwnd,WM_CLOSE,0,0);
        std::filesystem::remove(fixture);
    } else {
        ShowWindow(hwnd,show); UpdateWindow(hwnd);
        if(argc>1) Start(argv[1]);
        MSG message{}; while(GetMessageW(&message,nullptr,0,0)>0) { if(!IsDialogMessageW(hwnd,&message)) { TranslateMessage(&message); DispatchMessageW(&message); } }
    }
    LocalFree(argv);
    for(HFONT font:{normalFont,smallFont,titleFont,metricFont}) DeleteObject(font);
    DeleteObject(backgroundBrush);
    return code;
}
