#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include "alpha_surface.h"
#include "pattern_difficulty.h"
#include "analysis_key_mode.h"
#include "lane_display.h"
#include "dan_rating.h"
#include "session_report.h"
#include "game_overlay.h"
#include "self_test_io.h"
#include "window_placement.h"
#include "revive_live.h"
#include "revive_hud_layout.h"
#if KEYNOTE_OSU_MEMORY_LINK
#include "osu_link.h"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cerrno>
#include <cwchar>
#include <cwctype>
#include <cstdio>
#include <fstream>
#include <future>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

constexpr UINT WM_APP_KEY_DOWN = WM_APP + 1;
constexpr UINT WM_APP_KEY_UP = WM_APP + 2;
constexpr UINT WM_APP_TRAY = WM_APP + 3;
constexpr UINT_PTR TIMER_RENDER = 1;
constexpr UINT_PTR TIMER_DIFFICULTY = 2;
constexpr UINT_PTR TIMER_TRAY_RETRY = 3;
constexpr UINT ID_TRAY_SETTINGS = 40001;
constexpr UINT ID_TRAY_EXIT = 40002;
constexpr UINT ID_TRAY_MOVE = 40003;
constexpr UINT TRAY_ICON_ID = 1;
constexpr COLORREF kTransparentKey = RGB(1, 2, 3);
constexpr int kMinOverlayWidth = 650;
constexpr int kMinOverlayHeight = 300;

constexpr int IDC_LANES = 1001;
constexpr int IDC_X = 1002;
constexpr int IDC_Y = 1003;
constexpr int IDC_WIDTH = 1004;
constexpr int IDC_HEIGHT = 1005;
constexpr int IDC_DURATION = 1006;
constexpr int IDC_TRAVEL = 1007;
constexpr int IDC_DEBUG_BG = 1008;
constexpr int IDC_CLICK_THROUGH = 1009;
constexpr int IDC_TOPMOST = 1010;
constexpr int IDC_APPLY = 1011;
constexpr int IDC_SAVE = 1012;
constexpr int IDC_TEST = 1013;
constexpr int IDC_STATUS = 1014;
constexpr int IDC_CIRCLE_NOTES = 1018;
constexpr int IDC_VIS_CLASSIC = 1019;
constexpr int IDC_VIS_NEON = 1020;
constexpr int IDC_VIS_MINIMAL = 1021;
constexpr int IDC_OPACITY = 1022;
constexpr int IDC_LANE_BG = 1023;
constexpr int IDC_ROUNDED_BOXES = 1024;
constexpr int IDC_THIN_OUTLINE = 1025;
constexpr int IDC_KEY_TEXT_BOTTOM = 1026;
constexpr int IDC_LANE_SPACING = 1027;
constexpr int IDC_NOTE_WIDTH = 1028;
constexpr int IDC_NOTE_HEIGHT = 1029;
constexpr int IDC_TABS = 1030;
constexpr int IDC_PRESET = 1031;
constexpr int IDC_LANE_LIST = 1032;
constexpr int IDC_RECORD = 1033;
constexpr int IDC_REPLACE = 1034;
constexpr int IDC_MOVE_LEFT = 1035;
constexpr int IDC_MOVE_RIGHT = 1036;
constexpr int IDC_REMOVE = 1037;
constexpr int IDC_REVERT = 1038;
constexpr int IDC_CENTER = 1039;
constexpr int IDC_DIRTY = 1040;
constexpr int IDC_LANE_SUMMARY = 1041;
constexpr int IDC_LONG_ALPHA = 1042;
constexpr int IDC_VISUAL_NAME = 1043;
constexpr int IDC_DIFFICULTY_ENABLED = 1044;
constexpr int IDC_DIFFICULTY_HUD = 1045;
constexpr int IDC_DIFFICULTY_SCORE = 1046;
constexpr int IDC_DIFFICULTY_METRICS = 1047;
constexpr int IDC_DIFFICULTY_BAR = 1048;
constexpr int IDC_DIFFICULTY_RESET = 1049;
constexpr int IDC_DIFFICULTY_NPS = 1050;
constexpr int IDC_DIFFICULTY_PREVIOUS = 1051;
constexpr int IDC_DIFFICULTY_PROFILE = 1052;
constexpr int IDC_MOVE_MODE = 1053;
constexpr int IDC_SAVE_POSITION = 1054;
constexpr int IDC_STYLE_KEYS = 1055;
constexpr int IDC_KEY_COLOR = 1056;
constexpr int IDC_KEY_OPACITY = 1057;
constexpr int IDC_PICK_COLOR = 1058;
constexpr int IDC_RESET_KEY_STYLE = 1059;
constexpr int IDC_STYLE_PREVIEW = 1060;
constexpr int IDC_STYLE_NAME = 1061;
constexpr int IDC_REPORT_AUTO = 1062;
constexpr int IDC_REPORT_OPEN = 1063;
constexpr int IDC_REPORT_RETRY = 1064;
constexpr int IDC_REPORT_STATUS = 1065;
constexpr int IDC_GAME_OVERLAY = 1066;
constexpr int IDC_GAME_STATUS = 1067;
constexpr int IDC_OSU_ENABLE = 1068;
constexpr int IDC_OSU_WARNING = 1069;
constexpr int IDC_OSU_STATUS = 1070;
constexpr int IDC_OSU_TITLE = 1071;
constexpr int IDC_OSU_RATING = 1072;
constexpr int IDC_OSU_DETAILS = 1073;
constexpr int IDC_OSU_FOLDER = 1074;
constexpr int IDC_ANALYSIS_KEYS = 1075;
constexpr int IDC_SIZE_SMALLER = 1076;
constexpr int IDC_SIZE_LARGER = 1077;
constexpr int IDC_SPACING_SLIDER = 1078;
constexpr int IDC_OSU_REVIVE = 1079;
constexpr int IDC_OSU_SKILLS = 1080;
constexpr int IDC_KEY_VIEWER_FOLDED = 1081;

constexpr double kHoldThresholdMs = 90.0;
constexpr double kChordAlignWindowMs = 15.0;
constexpr double kPulseMs = 95.0;
#ifndef WDA_EXCLUDEFROMCAPTURE
constexpr DWORD WDA_EXCLUDEFROMCAPTURE = 0x00000011;
#endif

struct Lane {
    UINT vk = 0;
    std::wstring token;
    std::wstring label;
};

struct KeyStyle {
    COLORREF color = CLR_INVALID; // Default palette follows lane order until customized.
    int opacity = 100;
    bool operator==(const KeyStyle& other) const { return color == other.color && opacity == other.opacity; }
};

struct Config {
    int x = 80;
    int y = 40;
    int width = 900;
    int height = kMinOverlayHeight;
    int durationMs = 850;
    int travelPx = 180;
    int noteWidth = 58;
    int noteHeight = 34;
    int opacityPercent = 100;
    int laneSpacingPercent = 100;
    int longNoteAlpha = 92;
    std::wstring mode = L"8K";
    std::wstring visualPreset = L"Classic";
    bool debugBackground = true;
    bool clickThrough = true;
    bool alwaysOnTop = true;
    bool circleNotes = false;
    bool laneBackground = true;
    bool roundedBoxes = true;
    bool thinOutline = true;
    bool keyTextBottom = true;
    bool difficultyEnabled = true;
    bool difficultyHud = true;
    bool keyViewerFolded = false; // Presentation only; the registered input stays active.
    int analysisKeyMode = 0; // 0=registered, -1=auto with upward correction, 4..10=manual
    bool reportAutoSave = false; // Local input timing persistence is explicitly opt-in.
    bool gameOverlay = false;
    std::wstring lanesText = L"A,S,D,F,J,K,L,SEMICOLON";
    std::array<std::wstring, 7> presetLanes{}; // empty entry uses the built-in layout
    std::array<KeyStyle, 256> keyStyles{}; // Physical key identity survives reorder/preset changes.
    std::vector<Lane> lanes;
};

struct Note {
    unsigned long long id = 0;
    int lane = 0;
    std::wstring label;
    double pressMs = 0.0;
    double releaseMs = -1.0;
    bool held = true;
    COLORREF color = RGB(88, 213, 255);
    int opacity = 100;
};

struct Preset {
    const wchar_t* mode;
    const wchar_t* lanes;
};

struct VisualTheme {
    COLORREF background = RGB(28, 28, 28);
    COLORREF laneFill = RGB(18, 24, 34);
    COLORREF laneLine = RGB(68, 78, 92);
    COLORREF outline = RGB(236, 244, 255);
    COLORREF text = RGB(8, 17, 31);
    COLORREF shadow = RGB(10, 12, 16);
    int laneAlpha = 76;
    int pulseAlpha = 112;
};

HINSTANCE gInstance = nullptr;
HWND gSettingsWnd = nullptr;
HWND gOverlayWnd = nullptr;
HFONT gGuiFont = nullptr;
HFONT gNoteFont = nullptr;
HFONT gKeyFont = nullptr;
HFONT gHudValueFont = nullptr;
HFONT gHudMediumFont = nullptr;
HFONT gHudCompactFont = nullptr;
HFONT gHudSmallFont = nullptr;
HFONT gHudLabelFont = nullptr;
HFONT gOsuWarningFont = nullptr;
Config gConfig;
Config gSavedConfig;
bool gFillingControls = false;
bool gUnapplied = false;
bool gUnsaved = false;
bool gUiSelfTest = false;
bool gMoveMode = false; // Temporary: never changes the saved click-through preference.
bool gOverlayMoving = false;
RECT gMoveStart{};
bool gAppRunning = false;
bool gShuttingDown = false;
bool gCloseInProgress = false;
bool gPickingColor = false;
bool gTrayAdded = false;
bool gTrayVersion4 = false;
bool gTrayMenuActive = false;
bool gSettingsForegroundBoost = false;
UINT gTaskbarCreatedMessage = 0;
NOTIFYICONDATAW gTrayIcon{};
// Native API boundaries also let lifecycle tests run without altering the tray
// or showing a modal dialog. Normal execution uses the Windows APIs directly.
auto gNotifyIcon = &Shell_NotifyIconW;
auto gExitMessageBox = &MessageBoxW;
auto gTrackTrayMenu = &TrackPopupMenu;
int gBuildingPage = -1;
int gCurrentPage = 0;
#if KEYNOTE_OSU_MEMORY_LINK
std::array<std::vector<HWND>, 6> gPageControls;
osu_link::Session gOsuLink;
osu_link::View gOsuView;
std::filesystem::path gOsuSongsOverride;
auto gOsuRiskMessageBox = &MessageBoxW;
#else
std::array<std::vector<HWND>, 5> gPageControls;
#endif
pattern::DifficultyEstimator gDifficulty;
pattern::AnalysisKeySelector gAnalysisKeys;
int gConfirmedAutoKeys = 0;
pattern::Snapshot gDifficultySnapshot;
pattern::PatternSummary gPatternSummary;
pattern::LiveContinuity gLiveContinuity;
revive_live::Meter gReviveLive;
revive_live::CompletedSkills gCompletedSkills;
report::Recorder gReportRecorder;
std::deque<report::Session> gPendingReports;
std::future<report::SaveResult> gReportWrite;
bool gReportSaveFailed = false;
bool gReportWaitForFreshSession = false;
std::wstring gReportStatus;
constexpr std::size_t kMaxPendingReports = 4;
game_overlay::ForegroundClassifier gForegroundClassifier;
game_overlay::Game gForegroundGame = game_overlay::Game::None;
double gNextGameRaiseMs = 0;
enum class CaptureMode { None, Sequence, Replace };
CaptureMode gCaptureMode = CaptureMode::None;
std::wstring gCaptureOriginal;
std::vector<std::wstring> gCapturedKeys;
int gCaptureIndex = -1;
bool gCaptureWasUnapplied = false;
std::wstring gDraftVisualPreset;
std::array<std::wstring, 7> gDraftPresetLanes{};
struct KeyStyleDraft { std::wstring color; std::wstring opacity = L"100"; };
std::array<KeyStyleDraft, 256> gDraftKeyStyles{};
UINT gStyleVk = 0;
std::array<int, 256> gVkToLaneIndex{};
std::array<bool, 256> gKeyDown{};
std::array<double, 256> gPendingLaneDownMs{};
std::array<double, 256> gPendingLaneUpMs{};
HHOOK gKeyboardHook = nullptr;
std::vector<Note> gNotes;
std::wstring gConfigPath;
std::wstring gIniConfigPath;
std::wstring gJsonConfigPath;
bool gIniSaveFailed = false;
DWORD gConfigSaveError = ERROR_SUCCESS;
unsigned long long gNextNoteId = 1;
double gChordAnchorMs = -1000000.0;

void ShutdownApplication();
void RequestApplicationClose();
void ShowSettingsWindow();
void FinishOverlayMove(HWND hwnd);
void FinishSessionReport(const std::string& reason, double nowMs);
void PollReportWrites(bool wait = false);
void UpdateGameOverlay(double nowMs);
int OverlayPresentationHeight(int width);

bool AddTrayIcon() {
    if (gShuttingDown || !IsWindow(gSettingsWnd)) return false;
    if (gTrayAdded) return true;
    gTrayIcon = {};
    gTrayIcon.cbSize = sizeof(gTrayIcon);
    gTrayIcon.hWnd = gSettingsWnd;
    gTrayIcon.uID = TRAY_ICON_ID;
    gTrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    gTrayIcon.uCallbackMessage = WM_APP_TRAY;
    gTrayIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION); // shared system icon
    lstrcpynW(gTrayIcon.szTip, L"Key Note Overlay · 설정 열기 / 우클릭 메뉴", ARRAYSIZE(gTrayIcon.szTip));
    gTrayAdded = gNotifyIcon(NIM_ADD, &gTrayIcon) != FALSE;
    gTrayVersion4 = false;
    if (gTrayAdded) {
        gTrayIcon.uVersion = NOTIFYICON_VERSION_4;
        gTrayVersion4 = gNotifyIcon(NIM_SETVERSION, &gTrayIcon) != FALSE;
        KillTimer(gSettingsWnd, TIMER_TRAY_RETRY);
    } else {
        // Explorer may be starting. Retry only while this app is running.
        SetTimer(gSettingsWnd, TIMER_TRAY_RETRY, 5000, nullptr);
    }
    return gTrayAdded;
}

void RemoveTrayIcon() {
    if (gTrayIcon.hWnd) KillTimer(gTrayIcon.hWnd, TIMER_TRAY_RETRY);
    if (gTrayAdded) gNotifyIcon(NIM_DELETE, &gTrayIcon);
    gTrayAdded = gTrayVersion4 = false;
    gTrayIcon = {};
}

UINT TrayEvent(WPARAM wParam, LPARAM lParam) {
    const UINT id = gTrayVersion4 ? HIWORD(lParam) : static_cast<UINT>(wParam);
    return gTrayAdded && id == TRAY_ICON_ID ? (gTrayVersion4 ? LOWORD(lParam) : static_cast<UINT>(lParam)) : 0;
}

bool IsSettingsInteraction(HWND window) {
    return window && gSettingsWnd && GetAncestor(window, GA_ROOTOWNER) == gSettingsWnd;
}

void SetSettingsForegroundBoost(bool enabled) {
    if (!IsWindow(gSettingsWnd) || (!enabled && !gSettingsForegroundBoost)) return;
    gSettingsForegroundBoost = enabled;
    // Settings need to clear our topmost overlay while being edited, but must
    // return to the ordinary window band as soon as the user switches away.
    SetWindowPos(gSettingsWnd, enabled ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

bool MayRaiseGameOverlay(HWND foreground) {
    // TrackPopupMenu runs a nested message loop: the normal overlay timer keeps
    // firing there and would otherwise cover the menu (or an owned dialog).
    return !gTrayMenuActive && !IsSettingsInteraction(foreground);
}

void ShowTrayMenu(HWND owner) {
    if (gTrayMenuActive || !IsWindow(owner)) return;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING, ID_TRAY_SETTINGS, L"설정 열기");
    AppendMenuW(menu, MF_STRING | (gMoveMode ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_MOVE,
        gMoveMode ? L"오버레이 이동 모드 끝내기" : L"오버레이 마우스로 이동");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"종료");
    SetMenuDefaultItem(menu, ID_TRAY_SETTINGS, FALSE);
    POINT position{};
    GetCursorPos(&position);
    // A minimized/hidden settings HWND cannot reliably foreground a tray menu.
    // Use an independent, zero-area tool window in the topmost band. It exists
    // only while the menu is open and never restores settings on right-click.
    HWND menuOwner = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"STATIC", L"",
        WS_POPUP, position.x, position.y, 0, 0, nullptr, nullptr, gInstance, nullptr);
    if (!menuOwner) { DestroyMenu(menu); return; }
    gTrayMenuActive = true;
    if (!gUiSelfTest) {
        ShowWindow(menuOwner, SW_SHOWNOACTIVATE);
        SetForegroundWindow(menuOwner);
    }
    const UINT command = gTrackTrayMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        position.x, position.y, 0, menuOwner, nullptr);
    PostMessageW(menuOwner, WM_NULL, 0, 0);
    DestroyWindow(menuOwner);
    DestroyMenu(menu);
    gTrayMenuActive = false;
    // Let the shell dismiss its menu tracking before processing a close request.
    PostMessageW(owner, WM_NULL, 0, 0);
    if (command && IsWindow(owner)) PostMessageW(owner, WM_COMMAND, command, 0);
    else if (gTrayAdded) gNotifyIcon(NIM_SETFOCUS, &gTrayIcon);
}

const std::array<COLORREF, 8> kPalette = {
    RGB(88, 213, 255),
    RGB(255, 209, 102),
    RGB(124, 242, 154),
    RGB(255, 122, 182),
    RGB(167, 139, 250),
    RGB(249, 115, 22),
    RGB(56, 189, 248),
    RGB(248, 250, 252),
};

const std::array<Preset, 7> kPresets = {{
    {L"5K", L"D,F,SPACE,J,K"},
    {L"8K", L"A,S,D,F,J,K,L,SEMICOLON"},
    {L"10K", L"A,S,D,F,V,N,J,K,L,SEMICOLON"},
    {L"4K", L"D,F,J,K"},
    {L"6K", L"S,D,F,J,K,L"},
    {L"7K", L"S,D,F,SPACE,J,K,L"},
    {L"9K", L"A,S,D,F,SPACE,J,K,L,SEMICOLON"},
}};

double NowMs() {
    static LARGE_INTEGER freq = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value;
    }();
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    return (static_cast<double>(now.QuadPart) * 1000.0) / static_cast<double>(freq.QuadPart);
}

void ResetDifficulty(int lanes) {
    FinishSessionReport("분석 초기화 / 설정 변경", NowMs());
    gReportWaitForFreshSession = false;
    gDifficulty.Reset(lanes);
    gAnalysisKeys.Reset(lanes, gConfig.analysisKeyMode);
    gConfirmedAutoKeys = 0;
    gPatternSummary.Reset();
    gCompletedSkills.Reset();
    gLiveContinuity.Reset();
    gReviveLive.Reset(lanes);
}

void ObservePatternSummary(const pattern::Snapshot& s) {
    gPatternSummary.Observe(s);
    gCompletedSkills.Observe(gReviveLive.Current(), gReviveLive.CurrentRating(),
        s.idle, s.ready && !s.retained, s.limited || s.keyModeConflict);
}

pattern::Snapshot EvaluateLiveDifficulty(double nowMs) {
    gAnalysisKeys.Update(nowMs);
    if (gConfig.analysisKeyMode == -1 && !gAnalysisKeys.Pending() && !gAnalysisKeys.Conflict() &&
        gConfirmedAutoKeys != gAnalysisKeys.Keys()) {
        // Do not mix peaks or async results from different inferred key counts.
        // Keep input history so the corrected mode can resume without relearning.
        if (gConfirmedAutoKeys) FinishSessionReport("자동 분석 키 수 보정", nowMs);
        gConfirmedAutoKeys = gAnalysisKeys.Keys();
        gPatternSummary.Reset(); gCompletedSkills.Reset();
        gReviveLive.SetAnalysisKeys(gConfirmedAutoKeys);
    }
    gDifficulty.SetAnalysisKeys(gConfig.analysisKeyMode == 0 ? 0 : gAnalysisKeys.Keys());
    auto raw = gDifficulty.Evaluate(nowMs, true);
    raw.keyModePending = gAnalysisKeys.Pending();
    raw.keyModeConflict = gAnalysisKeys.Conflict() || (raw.keyModeConflict && !raw.keyModePending);
    if (raw.keyModePending || raw.keyModeConflict) raw.ready = false;
    return gLiveContinuity.Update(raw, nowMs);
}

overlay::LaneDisplay CurrentLaneDisplay(int fullWidth) {
    auto display = overlay::MakeLaneDisplay(static_cast<int>(gConfig.lanes.size()), fullWidth,
        gConfig.laneSpacingPercent, gAnalysisKeys,
        gConfig.difficultyEnabled && gConfig.analysisKeyMode != 0 && gCaptureMode == CaptureMode::None);
    // Folding preserves note geometry but must not bypass the window minimum.
    display.width = std::max(kMinOverlayWidth, display.width);
    return display;
}

bool KeyViewerFolded() { return gConfig.keyViewerFolded && gConfig.difficultyEnabled && gConfig.difficultyHud; }
int FoldedHudPixels(int logical) { return (logical * 3 + 1) / 2; }
int FoldedHudWidth(int pixels) { return pixels * 2 / 3; }
int OverlayPresentationWidth() {
    const int width = CurrentLaneDisplay(gConfig.width).width;
    return KeyViewerFolded() ? FoldedHudPixels(width) : width;
}

void SyncOverlayDisplayWidth(HWND hwnd) {
    if (!hwnd) return;
    RECT client{};
    if (!GetClientRect(hwnd, &client)) return;
    const int width = OverlayPresentationWidth();
    const int height = OverlayPresentationHeight(width);
    if (client.right == width && client.bottom == height) return;
    // Temporary presentation dimensions never overwrite the saved size.
    if (SetWindowPos(hwnd, nullptr, 0, 0, width, height,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) InvalidateRect(hwnd, nullptr, FALSE);
}

int ClampInt(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

COLORREF BlendColor(COLORREF fg, COLORREF bg, int alpha) {
    alpha = ClampInt(alpha, 0, 255);
    int r = GetRValue(fg) * alpha / 255 + GetRValue(bg) * (255 - alpha) / 255;
    int g = GetGValue(fg) * alpha / 255 + GetGValue(bg) * (255 - alpha) / 255;
    int b = GetBValue(fg) * alpha / 255 + GetBValue(bg) * (255 - alpha) / 255;
    return RGB(r, g, b);
}

double AlignChordPressMs(double eventMs) {
    if (eventMs - gChordAnchorMs > kChordAlignWindowMs) {
        gChordAnchorMs = eventMs;
    }
    return gChordAnchorMs;
}

double TakePendingLaneTime(std::array<double, 256>& pending, int lane) {
    if (lane < 0 || lane >= static_cast<int>(pending.size())) {
        return NowMs();
    }
    double eventMs = pending[static_cast<size_t>(lane)];
    pending[static_cast<size_t>(lane)] = 0.0;
    return eventMs > 0.0 ? eventMs : NowMs();
}

std::wstring ExeDirectory() {
    std::wstring path(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    path.resize(len);
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".\\";
    }
    return path.substr(0, slash + 1);
}

std::wstring CurrentDirectory() {
    std::wstring path(MAX_PATH, L'\0');
    DWORD len = GetCurrentDirectoryW(static_cast<DWORD>(path.size()), path.data());
    path.resize(len);
    if (path.empty() || path.back() == L'\\' || path.back() == L'/') {
        return path;
    }
    return path + L"\\";
}

std::wstring WidenAscii(const std::string& value) {
    std::wstring out;
    out.reserve(value.size());
    for (unsigned char c : value) {
        out.push_back(static_cast<wchar_t>(c));
    }
    return out;
}

bool ReadTextFile(const std::wstring& path, std::string& out) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 1024 * 1024) {
        CloseHandle(file);
        return false;
    }
    out.assign(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    BOOL ok = out.empty() || ReadFile(file, out.data(), static_cast<DWORD>(out.size()), &read, nullptr);
    CloseHandle(file);
    if (!ok) {
        out.clear();
        return false;
    }
    out.resize(read);
    return true;
}

bool WriteTextFile(const std::wstring& path, const std::string& text) {
    // Same-directory staging keeps the previous settings intact until the whole
    // payload is written, flushed and read back. Never reuse someone else's temp.
    static unsigned long long serial = 0;
    std::wstring staging;
    HANDLE file = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 16; ++attempt) {
        staging = path + L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(++serial);
        file = CreateFileW(staging.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) break;
        if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD error = ERROR_SUCCESS;
    size_t offset = 0;
    while (offset < text.size()) {
        DWORD written = 0;
        if (!WriteFile(file, text.data() + offset, static_cast<DWORD>(text.size() - offset), &written, nullptr) || written == 0) {
            error = GetLastError();
            if (error == ERROR_SUCCESS) error = ERROR_WRITE_FAULT;
            break;
        }
        offset += written;
    }
    if (error == ERROR_SUCCESS && !FlushFileBuffers(file)) error = GetLastError();
    if (!CloseHandle(file) && error == ERROR_SUCCESS) error = GetLastError();
    std::string verified;
    if (error == ERROR_SUCCESS && (!ReadTextFile(staging, verified) || verified != text)) error = ERROR_WRITE_FAULT;
    if (error == ERROR_SUCCESS && !MoveFileExW(staging.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        error = GetLastError();
    if (error != ERROR_SUCCESS) {
        DeleteFileW(staging.c_str());
        SetLastError(error);
        return false;
    }
    return true;
}

void JsonSkipWs(const std::string& text, size_t& pos) {
    while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\n' || text[pos] == '\r' || text[pos] == '\t')) {
        ++pos;
    }
}

size_t JsonValuePos(const std::string& text, const char* key) {
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) {
        return std::string::npos;
    }
    pos = text.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return std::string::npos;
    }
    ++pos;
    JsonSkipWs(text, pos);
    return pos;
}

bool JsonReadStringAt(const std::string& text, size_t& pos, std::string& out) {
    JsonSkipWs(text, pos);
    if (pos >= text.size() || text[pos] != '"') {
        return false;
    }
    ++pos;
    out.clear();
    while (pos < text.size()) {
        char c = text[pos++];
        if (c == '"') {
            return true;
        }
        if (c == '\\' && pos < text.size()) {
            char escaped = text[pos++];
            if (escaped == 'n') out.push_back('\n');
            else if (escaped == 'r') out.push_back('\r');
            else if (escaped == 't') out.push_back('\t');
            else out.push_back(escaped);
        } else {
            out.push_back(c);
        }
    }
    return false;
}

bool JsonReadString(const std::string& text, const char* key, std::string& out) {
    size_t pos = JsonValuePos(text, key);
    return pos != std::string::npos && JsonReadStringAt(text, pos, out);
}

bool JsonReadInt(const std::string& text, const char* key, int& out) {
    size_t pos = JsonValuePos(text, key);
    if (pos == std::string::npos) {
        return false;
    }
    bool negative = pos < text.size() && text[pos] == '-';
    if (negative) ++pos;
    if (pos >= text.size() || text[pos] < '0' || text[pos] > '9') {
        return false;
    }
    int value = 0;
    while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
        value = value * 10 + (text[pos++] - '0');
    }
    out = negative ? -value : value;
    return true;
}

bool JsonReadBool(const std::string& text, const char* key, bool& out) {
    size_t pos = JsonValuePos(text, key);
    if (pos == std::string::npos) {
        return false;
    }
    if (text.compare(pos, 4, "true") == 0) {
        out = true;
        return true;
    }
    if (text.compare(pos, 5, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}

bool JsonReadStringArray(const std::string& text, const char* key, std::vector<std::string>& out) {
    size_t pos = JsonValuePos(text, key);
    if (pos == std::string::npos || pos >= text.size() || text[pos] != '[') {
        return false;
    }
    ++pos;
    out.clear();
    while (pos < text.size()) {
        JsonSkipWs(text, pos);
        if (pos < text.size() && text[pos] == ']') {
            return true;
        }
        std::string value;
        if (!JsonReadStringAt(text, pos, value)) {
            return false;
        }
        out.push_back(value);
        JsonSkipWs(text, pos);
        if (pos < text.size() && text[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < text.size() && text[pos] == ']') {
            return true;
        }
        return false;
    }
    return false;
}

std::string JsonEscape(const std::wstring& value) {
    std::string out;
    for (wchar_t wc : value) {
        char c = wc <= 127 ? static_cast<char>(wc) : '?';
        if (c == '"' || c == '\\') {
            out.push_back('\\');
            out.push_back(c);
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out.push_back(c);
        }
    }
    return out;
}

std::wstring Trim(std::wstring value) {
    auto first = std::find_if_not(value.begin(), value.end(), [](wchar_t c) {
        return std::iswspace(c) != 0;
    });
    auto last = std::find_if_not(value.rbegin(), value.rend(), [](wchar_t c) {
        return std::iswspace(c) != 0;
    }).base();
    if (first >= last) {
        return L"";
    }
    return std::wstring(first, last);
}

std::wstring Upper(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towupper(c));
    });
    return value;
}

std::wstring NormalizeVisualPreset(const std::wstring& raw) {
    std::wstring preset = Upper(Trim(raw));
    if (preset == L"NEON") return L"Neon";
    if (preset == L"MINIMAL") return L"Minimal";
    return L"Classic";
}

VisualTheme CurrentTheme() {
    std::wstring preset = NormalizeVisualPreset(gConfig.visualPreset);
    if (preset == L"Neon") {
        return VisualTheme{
            RGB(8, 10, 18),
            RGB(4, 20, 36),
            RGB(34, 211, 238),
            RGB(165, 243, 252),
            RGB(3, 7, 18),
            RGB(2, 6, 23),
            88,
            150
        };
    }
    if (preset == L"Minimal") {
        return VisualTheme{
            RGB(22, 22, 22),
            RGB(24, 24, 24),
            RGB(82, 82, 82),
            RGB(214, 214, 214),
            RGB(18, 18, 18),
            RGB(8, 8, 8),
            48,
            72
        };
    }
    return VisualTheme{};
}

void ApplyVisualPreset(const std::wstring& rawPreset, Config& config = gConfig) {
    config.visualPreset = NormalizeVisualPreset(rawPreset);
    if (config.visualPreset == L"Neon") {
        config.opacityPercent = 94;
        config.laneSpacingPercent = 104;
        config.longNoteAlpha = 76;
        config.circleNotes = true;
        config.laneBackground = true;
        config.roundedBoxes = true;
        config.thinOutline = true;
        config.keyTextBottom = true;
    } else if (config.visualPreset == L"Minimal") {
        config.opacityPercent = 86;
        config.laneSpacingPercent = 92;
        config.longNoteAlpha = 58;
        config.circleNotes = false;
        config.laneBackground = false;
        config.roundedBoxes = true;
        config.thinOutline = true;
        config.keyTextBottom = false;
    } else {
        config.opacityPercent = 100;
        config.laneSpacingPercent = 100;
        config.longNoteAlpha = 92;
        config.circleNotes = false;
        config.laneBackground = true;
        config.roundedBoxes = true;
        config.thinOutline = true;
        config.keyTextBottom = true;
    }
}

std::wstring NormalizeToken(const std::wstring& raw) {
    std::wstring token = Upper(Trim(raw));
    if (token == L"SPC" || token == L"SPACEBAR") return L"SPACE";
    if (token == L"RETURN") return L"ENTER";
    if (token == L"ESCAPE") return L"ESC";
    if (token == L"CONTROL" || token == L"LCONTROL") return L"LCTRL";
    if (token == L"RCONTROL") return L"RCTRL";
    if (token == L"LMENU" || token == L"OPTION") return L"LALT";
    if (token == L"RMENU") return L"RALT";
    if (token == L"SEMICOLON" || token == L"COLON") return L";";
    if (token == L"COMMA") return L",";
    if (token == L"DOT" || token == L"PERIOD") return L".";
    if (token == L"SLASH") return L"/";
    if (token == L"BACKSLASH") return L"\\";
    if (token == L"LBRACKET") return L"[";
    if (token == L"RBRACKET") return L"]";
    if (token == L"QUOTE" || token == L"APOSTROPHE") return L"'";
    if (token == L"BACKTICK") return L"`";
    if (token == L"MINUS") return L"-";
    if (token == L"EQUALS") return L"=";
    return token;
}

std::wstring TokenForConfig(const std::wstring& raw) {
    std::wstring token = NormalizeToken(raw);
    if (token == L";") return L"SEMICOLON";
    if (token == L",") return L"COMMA";
    if (token == L".") return L"PERIOD";
    if (token == L"/") return L"SLASH";
    if (token == L"\\") return L"BACKSLASH";
    if (token == L"[") return L"LBRACKET";
    if (token == L"]") return L"RBRACKET";
    if (token == L"'") return L"QUOTE";
    if (token == L"`") return L"BACKTICK";
    if (token == L"-") return L"MINUS";
    if (token == L"=") return L"EQUALS";
    return token;
}

bool TokenToVk(const std::wstring& raw, UINT& outVk, std::wstring& outLabel) {
    std::wstring token = NormalizeToken(raw);
    if (token.empty()) {
        return false;
    }

    if (token.size() == 1) {
        wchar_t c = token[0];
        if ((c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9')) {
            outVk = static_cast<UINT>(c);
            outLabel = token;
            return true;
        }
    }

    static const std::unordered_map<std::wstring, UINT> named = {
        {L"SPACE", VK_SPACE}, {L"ENTER", VK_RETURN}, {L"TAB", VK_TAB},
        {L"ESC", VK_ESCAPE}, {L"BACK", VK_BACK}, {L"BACKSPACE", VK_BACK},
        {L"LEFT", VK_LEFT}, {L"RIGHT", VK_RIGHT}, {L"UP", VK_UP}, {L"DOWN", VK_DOWN},
        {L"SHIFT", VK_SHIFT}, {L"LSHIFT", VK_LSHIFT}, {L"RSHIFT", VK_RSHIFT},
        {L"CTRL", VK_CONTROL}, {L"LCTRL", VK_LCONTROL}, {L"RCTRL", VK_RCONTROL},
        {L"ALT", VK_MENU}, {L"LALT", VK_LMENU}, {L"RALT", VK_RMENU},
        {L"HOME", VK_HOME}, {L"END", VK_END}, {L"INSERT", VK_INSERT}, {L"DELETE", VK_DELETE},
        {L"PAGEUP", VK_PRIOR}, {L"PAGEDOWN", VK_NEXT}, {L"CAPSLOCK", VK_CAPITAL},
        {L"NUM0", VK_NUMPAD0}, {L"NUM1", VK_NUMPAD1}, {L"NUM2", VK_NUMPAD2},
        {L"NUM3", VK_NUMPAD3}, {L"NUM4", VK_NUMPAD4}, {L"NUM5", VK_NUMPAD5},
        {L"NUM6", VK_NUMPAD6}, {L"NUM7", VK_NUMPAD7}, {L"NUM8", VK_NUMPAD8}, {L"NUM9", VK_NUMPAD9},
        {L"NUMADD", VK_ADD}, {L"NUMSUBTRACT", VK_SUBTRACT}, {L"NUMMULTIPLY", VK_MULTIPLY},
        {L"NUMDIVIDE", VK_DIVIDE}, {L"NUMDECIMAL", VK_DECIMAL},
        {L";", VK_OEM_1}, {L"=", VK_OEM_PLUS}, {L",", VK_OEM_COMMA},
        {L"-", VK_OEM_MINUS}, {L".", VK_OEM_PERIOD}, {L"/", VK_OEM_2},
        {L"`", VK_OEM_3}, {L"[", VK_OEM_4}, {L"\\", VK_OEM_5},
        {L"]", VK_OEM_6}, {L"'", VK_OEM_7},
    };

    auto found = named.find(token);
    if (found != named.end()) {
        outVk = found->second;
        outLabel = token;
        return true;
    }

    if (token.size() >= 2 && token[0] == L'F') {
        wchar_t* end = nullptr;
        long number = std::wcstol(token.c_str() + 1, &end, 10);
        if (end != token.c_str() + 1 && *end == L'\0' && number >= 1 && number <= 24) {
            outVk = VK_F1 + static_cast<UINT>(number - 1);
            outLabel = token;
            return true;
        }
    }

    return false;
}

std::wstring TokenFromVk(UINT vk) {
    if ((vk >= L'A' && vk <= L'Z') || (vk >= L'0' && vk <= L'9')) return std::wstring(1, static_cast<wchar_t>(vk));
    if (vk >= VK_F1 && vk <= VK_F24) return L"F" + std::to_wstring(vk - VK_F1 + 1);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return L"NUM" + std::to_wstring(vk - VK_NUMPAD0);
    const wchar_t* names[] = {L"SPACE", L"ENTER", L"TAB", L"ESC", L"SHIFT", L"CTRL", L"ALT", L"BACKSPACE", L"LEFT", L"RIGHT", L"UP", L"DOWN",
        L"LSHIFT", L"RSHIFT", L"LCTRL", L"RCTRL", L"LALT", L"RALT", L"HOME", L"END", L"INSERT", L"DELETE",
        L"PAGEUP", L"PAGEDOWN", L"CAPSLOCK", L"NUMADD", L"NUMSUBTRACT", L"NUMMULTIPLY", L"NUMDIVIDE", L"NUMDECIMAL",
        L"SEMICOLON", L"COMMA", L"PERIOD", L"SLASH", L"BACKSLASH", L"LBRACKET", L"RBRACKET", L"QUOTE", L"BACKTICK", L"MINUS", L"EQUALS"};
    for (const wchar_t* name : names) {
        UINT candidate = 0;
        std::wstring label;
        if (TokenToVk(name, candidate, label) && candidate == vk) return name;
    }
    return L"";
}

bool ParseInteger(const std::wstring& raw, int minValue, int maxValue, int& result) {
    const std::wstring value = Trim(raw);
    if (value.empty()) return false;
    wchar_t* end = nullptr;
    errno = 0;
    long parsed = std::wcstol(value.c_str(), &end, 10);
    if (errno == ERANGE || end == value.c_str() || *end || parsed < minValue || parsed > maxValue) return false;
    result = static_cast<int>(parsed);
    return true;
}

std::wstring ColorHex(COLORREF color) {
    if (color == CLR_INVALID) return L"";
    std::wostringstream text;
    text << L'#' << std::uppercase << std::hex << std::setfill(L'0') << std::setw(2) << static_cast<int>(GetRValue(color))
        << std::setw(2) << static_cast<int>(GetGValue(color)) << std::setw(2) << static_cast<int>(GetBValue(color));
    return text.str();
}

bool ParseColor(const std::wstring& raw, COLORREF& color) {
    std::wstring text = Trim(raw);
    if (text.empty()) { color = CLR_INVALID; return true; }
    if (text.front() == L'#') text.erase(text.begin());
    if (text.size() != 6) return false;
    unsigned rgb = 0;
    for (wchar_t c : text) {
        c = static_cast<wchar_t>(towupper(c));
        int digit = c >= L'0' && c <= L'9' ? c - L'0' : c >= L'A' && c <= L'F' ? c - L'A' + 10 : -1;
        if (digit < 0) return false;
        rgb = rgb * 16 + digit;
    }
    color = RGB((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255);
    return true;
}

COLORREF LaneColor(int lane) {
    const auto& style = gConfig.keyStyles[gConfig.lanes[static_cast<size_t>(lane)].vk];
    return style.color == CLR_INVALID ? kPalette[static_cast<size_t>(lane) % kPalette.size()] : style.color;
}

// Editing is strict: never silently discard a lane or replace an empty layout with 8K.
// Loading legacy files retains the old tolerant path in RebuildLaneConfig.
bool ValidateLaneText(const std::wstring& text, std::vector<std::wstring>& tokens, std::wstring& error) {
    tokens.clear();
    std::array<bool, 256> used{};
    size_t start = 0;
    do {
        size_t end = text.find(L',', start);
        std::wstring token = Trim(text.substr(start, end == std::wstring::npos ? end : end - start));
        UINT vk = 0;
        std::wstring label;
        if (token.empty()) { error = L"빈 키가 있어요. 키를 입력하고 불필요한 쉼표를 지워 주세요."; return false; }
        if (!TokenToVk(token, vk, label) || vk >= used.size()) {
            error = L"인식할 수 없는 키: " + token;
            return false;
        }
        if (used[vk]) { error = L"같은 키가 두 번 있어요: " + token; return false; }
        used[vk] = true;
        tokens.push_back(TokenForConfig(token));
        if (end == std::wstring::npos) break;
        start = end + 1;
    } while (start <= text.size());
    error.clear();
    return true;
}

std::wstring JoinLaneTokens(const std::vector<std::wstring>& tokens) {
    std::wstring result;
    for (const auto& token : tokens) {
        if (!result.empty()) result += L",";
        result += token;
    }
    return result;
}

int PresetIndexForCount(std::size_t count) {
    for (std::size_t i = 0; i < kPresets.size(); ++i)
        if (count == static_cast<std::size_t>(_wtoi(kPresets[i].mode))) return static_cast<int>(i);
    return -1;
}

void RememberPreset(std::array<std::wstring, 7>& presets, const std::wstring& text) {
    std::vector<std::wstring> tokens;
    std::wstring error;
    if (!ValidateLaneText(text, tokens, error)) return;
    const int index = PresetIndexForCount(tokens.size());
    if (index < 0) return;
    const auto canonical = JoinLaneTokens(tokens);
    presets[index] = canonical == kPresets[index].lanes ? L"" : canonical;
}

void NormalizePresetLayouts() {
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
        std::vector<std::wstring> tokens;
        std::wstring error;
        auto& layout = gConfig.presetLanes[i];
        if (!ValidateLaneText(layout, tokens, error) || PresetIndexForCount(tokens.size()) != static_cast<int>(i)) layout.clear();
        else layout = JoinLaneTokens(tokens) == kPresets[i].lanes ? L"" : JoinLaneTokens(tokens);
    }
    // Older saves only had the current layout. Keep it as that key-count preset.
    RememberPreset(gConfig.presetLanes, gConfig.lanesText);
}

bool SameConfig(const Config& a, const Config& b) {
    auto values = [](const Config& c) {
        return std::tie(c.x, c.y, c.width, c.height, c.durationMs, c.travelPx, c.noteWidth, c.noteHeight,
            c.opacityPercent, c.laneSpacingPercent, c.longNoteAlpha, c.visualPreset, c.debugBackground,
            c.clickThrough, c.alwaysOnTop, c.circleNotes, c.laneBackground, c.roundedBoxes, c.thinOutline,
            c.keyTextBottom, c.difficultyEnabled, c.difficultyHud, c.keyViewerFolded, c.analysisKeyMode, c.reportAutoSave, c.gameOverlay, c.lanesText, c.presetLanes, c.keyStyles);
    };
    return values(a) == values(b);
}

std::vector<std::wstring> SplitLaneText(const std::wstring& text) {
    std::vector<std::wstring> parts;
    std::wstringstream stream(text);
    std::wstring part;
    while (std::getline(stream, part, L',')) {
        part = Trim(part);
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

void RebuildLaneConfig() {
    gConfig.lanes.clear();
    gVkToLaneIndex.fill(-1);

    for (const std::wstring& token : SplitLaneText(gConfig.lanesText)) {
        UINT vk = 0;
        std::wstring label;
        if (!TokenToVk(token, vk, label)) {
            continue;
        }
        if (vk >= gVkToLaneIndex.size() || gVkToLaneIndex[vk] >= 0) {
            continue;
        }
        int laneIndex = static_cast<int>(gConfig.lanes.size());
        gConfig.lanes.push_back(Lane{vk, NormalizeToken(token), label});
        gVkToLaneIndex[vk] = laneIndex;
    }

    if (gConfig.lanes.empty()) {
        gConfig.lanesText = L"A,S,D,F,J,K,L,SEMICOLON";
        gConfig.mode = L"8K";
        RebuildLaneConfig();
    }
}

std::wstring CanonicalLaneText() {
    std::wstring text;
    for (const Lane& lane : gConfig.lanes) {
        if (!text.empty()) {
            text += L",";
        }
        text += TokenForConfig(lane.token);
    }
    return text;
}

std::wstring ReadIniString(const wchar_t* section, const wchar_t* key, const wchar_t* fallback) {
    std::array<wchar_t, 512> buffer{};
    GetPrivateProfileStringW(section, key, fallback, buffer.data(), static_cast<DWORD>(buffer.size()), gIniConfigPath.c_str());
    return buffer.data();
}

int ReadIniInt(const wchar_t* section, const wchar_t* key, int fallback) {
    return static_cast<int>(GetPrivateProfileIntW(section, key, fallback, gIniConfigPath.c_str()));
}

bool ReadIniBool(const wchar_t* section, const wchar_t* key, bool fallback) {
    return ReadIniInt(section, key, fallback ? 1 : 0) != 0;
}

void ClampAndRebuildConfig() {
    gConfig.visualPreset = NormalizeVisualPreset(gConfig.visualPreset);
    gConfig.width = std::max(kMinOverlayWidth, gConfig.width);
    gConfig.height = std::max(kMinOverlayHeight, gConfig.height);
    gConfig.durationMs = std::max(120, gConfig.durationMs);
    gConfig.travelPx = std::max(40, gConfig.travelPx);
    gConfig.noteWidth = ClampInt(gConfig.noteWidth, 24, 220);
    gConfig.noteHeight = ClampInt(gConfig.noteHeight, 18, 140);
    gConfig.opacityPercent = ClampInt(gConfig.opacityPercent, 20, 100);
    gConfig.laneSpacingPercent = ClampInt(gConfig.laneSpacingPercent, 50, 160);
    gConfig.longNoteAlpha = ClampInt(gConfig.longNoteAlpha, 32, 180);
    for (auto& style : gConfig.keyStyles) style.opacity = ClampInt(style.opacity, 0, 100);
    RebuildLaneConfig();
}

void ResolveConfigPaths() {
    std::wstring exeDir = ExeDirectory();
    std::wstring curDir = CurrentDirectory();
    std::wstring exeJson = exeDir + L"key_note_cpp_config.json";
    std::wstring exeIni = exeDir + L"key_note_cpp_config.ini";
    std::wstring curJson = curDir + L"key_note_cpp_config.json";
    std::wstring curIni = curDir + L"key_note_cpp_config.ini";

    if (GetFileAttributesW(exeJson.c_str()) != INVALID_FILE_ATTRIBUTES || GetFileAttributesW(exeIni.c_str()) != INVALID_FILE_ATTRIBUTES) {
        gJsonConfigPath = exeJson;
        gIniConfigPath = exeIni;
    } else if (GetFileAttributesW(curJson.c_str()) != INVALID_FILE_ATTRIBUTES || GetFileAttributesW(curIni.c_str()) != INVALID_FILE_ATTRIBUTES) {
        gJsonConfigPath = curJson;
        gIniConfigPath = curIni;
    } else {
        gJsonConfigPath = exeJson;
        gIniConfigPath = exeIni;
    }
    gConfigPath = gJsonConfigPath;
}

void LoadIniConfig() {
    gConfig.x = ReadIniInt(L"overlay", L"x", gConfig.x);
    gConfig.y = ReadIniInt(L"overlay", L"y", gConfig.y);
    gConfig.width = ReadIniInt(L"overlay", L"width", gConfig.width);
    gConfig.height = ReadIniInt(L"overlay", L"height", gConfig.height);
    gConfig.durationMs = ReadIniInt(L"overlay", L"duration_ms", gConfig.durationMs);
    gConfig.travelPx = ReadIniInt(L"overlay", L"travel_px", gConfig.travelPx);
    gConfig.noteWidth = ReadIniInt(L"overlay", L"note_width", gConfig.noteWidth);
    gConfig.noteHeight = ReadIniInt(L"overlay", L"note_height", gConfig.noteHeight);
    gConfig.visualPreset = ReadIniString(L"overlay", L"visual_preset", gConfig.visualPreset.c_str());
    ApplyVisualPreset(gConfig.visualPreset);
    gConfig.opacityPercent = ReadIniInt(L"overlay", L"opacity_percent", gConfig.opacityPercent);
    gConfig.laneSpacingPercent = ReadIniInt(L"overlay", L"lane_spacing_percent", gConfig.laneSpacingPercent);
    gConfig.longNoteAlpha = ReadIniInt(L"overlay", L"long_note_alpha", gConfig.longNoteAlpha);
    gConfig.debugBackground = ReadIniBool(L"overlay", L"debug_background", gConfig.debugBackground);
    gConfig.clickThrough = ReadIniBool(L"overlay", L"click_through", gConfig.clickThrough);
    gConfig.alwaysOnTop = ReadIniBool(L"overlay", L"always_on_top", gConfig.alwaysOnTop);
    gConfig.circleNotes = ReadIniBool(L"overlay", L"circle_notes", gConfig.circleNotes);
    gConfig.laneBackground = ReadIniBool(L"overlay", L"lane_background", gConfig.laneBackground);
    gConfig.roundedBoxes = ReadIniBool(L"overlay", L"rounded_boxes", gConfig.roundedBoxes);
    gConfig.thinOutline = ReadIniBool(L"overlay", L"thin_outline", gConfig.thinOutline);
    gConfig.keyTextBottom = ReadIniBool(L"overlay", L"key_text_bottom", gConfig.keyTextBottom);
    gConfig.difficultyEnabled = ReadIniBool(L"overlay", L"difficulty_enabled", gConfig.difficultyEnabled);
    gConfig.difficultyHud = ReadIniBool(L"overlay", L"difficulty_hud", gConfig.difficultyHud);
    gConfig.keyViewerFolded = ReadIniBool(L"overlay", L"key_viewer_folded", gConfig.keyViewerFolded);
    gConfig.analysisKeyMode = pattern::NormalizeAnalysisKeyMode(ReadIniInt(L"overlay", L"analysis_key_mode", gConfig.analysisKeyMode));
    gConfig.reportAutoSave = ReadIniBool(L"overlay", L"report_auto_save", gConfig.reportAutoSave);
    gConfig.gameOverlay = ReadIniBool(L"overlay", L"game_overlay", gConfig.gameOverlay);
    gConfig.mode = ReadIniString(L"input", L"mode", gConfig.mode.c_str());
    gConfig.lanesText = ReadIniString(L"input", L"lanes", gConfig.lanesText.c_str());
    for (std::size_t i = 0; i < kPresets.size(); ++i)
        gConfig.presetLanes[i] = ReadIniString(L"input_presets", kPresets[i].mode, L"");
    NormalizePresetLayouts();
    gConfig.keyStyles = {};
    for (UINT vk = 0; vk < gConfig.keyStyles.size(); ++vk) {
        const auto token = TokenFromVk(vk);
        if (token.empty()) continue;
        auto& style = gConfig.keyStyles[vk];
        ParseColor(ReadIniString(L"key_styles", (L"color_" + token).c_str(), L""), style.color);
        style.opacity = ClampInt(ReadIniInt(L"key_styles", (L"opacity_" + token).c_str(), 100), 0, 100);
    }
}

bool HasCompleteJsonObject(const std::string& text) {
    // Detect empty/truncated saves before touching gConfig so the INI fallback
    // remains usable. Field parsing below intentionally keeps legacy tolerance.
    size_t pos = text.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
    JsonSkipWs(text, pos);
    if (pos >= text.size() || text[pos] != '{') return false;
    std::vector<char> stack;
    bool quoted = false, escaped = false;
    for (; pos < text.size(); ++pos) {
        const char c = text[pos];
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
            continue;
        }
        if (c == '"') quoted = true;
        else if (c == '{' || c == '[') stack.push_back(c);
        else if (c == '}' || c == ']') {
            if (stack.empty() || stack.back() != (c == '}' ? '{' : '[')) return false;
            stack.pop_back();
            if (stack.empty()) { ++pos; JsonSkipWs(text, pos); return pos == text.size(); }
        }
    }
    return false;
}

bool LoadJsonConfig() {
    std::string json;
    if (!ReadTextFile(gJsonConfigPath, json) || !HasCompleteJsonObject(json)) {
        return false;
    }
    int intValue = 0;
    bool boolValue = false;
    std::string stringValue;

    if (JsonReadInt(json, "x", intValue)) gConfig.x = intValue;
    if (JsonReadInt(json, "y", intValue)) gConfig.y = intValue;
    if (JsonReadInt(json, "width", intValue)) gConfig.width = intValue;
    if (JsonReadInt(json, "height", intValue)) gConfig.height = intValue;
    if (JsonReadInt(json, "duration_ms", intValue)) gConfig.durationMs = intValue;
    if (JsonReadInt(json, "travel_px", intValue)) gConfig.travelPx = intValue;
    if (JsonReadInt(json, "note_width", intValue)) gConfig.noteWidth = intValue;
    if (JsonReadInt(json, "note_height", intValue)) gConfig.noteHeight = intValue;
    if (JsonReadString(json, "visual_preset", stringValue)) {
        gConfig.visualPreset = WidenAscii(stringValue);
        ApplyVisualPreset(gConfig.visualPreset);
    }
    if (JsonReadInt(json, "opacity_percent", intValue)) gConfig.opacityPercent = intValue;
    if (JsonReadInt(json, "lane_spacing_percent", intValue)) gConfig.laneSpacingPercent = intValue;
    if (JsonReadInt(json, "long_note_alpha", intValue)) gConfig.longNoteAlpha = intValue;
    if (JsonReadBool(json, "debug_background", boolValue)) gConfig.debugBackground = boolValue;
    if (JsonReadBool(json, "click_through", boolValue)) gConfig.clickThrough = boolValue;
    if (JsonReadBool(json, "always_on_top", boolValue)) gConfig.alwaysOnTop = boolValue;
    if (JsonReadBool(json, "circle_notes", boolValue)) gConfig.circleNotes = boolValue;
    if (JsonReadBool(json, "lane_background", boolValue)) gConfig.laneBackground = boolValue;
    if (JsonReadBool(json, "rounded_boxes", boolValue)) gConfig.roundedBoxes = boolValue;
    if (JsonReadBool(json, "thin_outline", boolValue)) gConfig.thinOutline = boolValue;
    if (JsonReadBool(json, "key_text_bottom", boolValue)) gConfig.keyTextBottom = boolValue;
    if (JsonReadBool(json, "difficulty_enabled", boolValue)) gConfig.difficultyEnabled = boolValue;
    if (JsonReadBool(json, "difficulty_hud", boolValue)) gConfig.difficultyHud = boolValue;
    if (JsonReadBool(json, "key_viewer_folded", boolValue)) gConfig.keyViewerFolded = boolValue;
    if (JsonReadInt(json, "analysis_key_mode", intValue)) gConfig.analysisKeyMode = pattern::NormalizeAnalysisKeyMode(intValue);
    if (JsonReadBool(json, "report_auto_save", boolValue)) gConfig.reportAutoSave = boolValue;
    if (JsonReadBool(json, "game_overlay", boolValue)) gConfig.gameOverlay = boolValue;
    if (JsonReadString(json, "mode", stringValue)) gConfig.mode = WidenAscii(stringValue);

    std::vector<std::string> laneTokens;
    if (JsonReadStringArray(json, "lanes", laneTokens) && !laneTokens.empty()) {
        std::wstring lanes;
        for (const std::string& lane : laneTokens) {
            if (!lanes.empty()) lanes += L",";
            lanes += WidenAscii(lane);
        }
        gConfig.lanesText = lanes;
    } else if (JsonReadString(json, "lanes", stringValue)) {
        gConfig.lanesText = WidenAscii(stringValue);
    }
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
        gConfig.presetLanes[i].clear();
        const std::string key = "preset_" + std::to_string(_wtoi(kPresets[i].mode)) + "k";
        if (JsonReadString(json, key.c_str(), stringValue)) gConfig.presetLanes[i] = WidenAscii(stringValue);
    }
    NormalizePresetLayouts();
    gConfig.keyStyles = {};
    for (UINT vk = 0; vk < gConfig.keyStyles.size(); ++vk) {
        const auto wideToken = TokenFromVk(vk);
        if (wideToken.empty()) continue;
        const std::string token(wideToken.begin(), wideToken.end());
        auto& style = gConfig.keyStyles[vk];
        if (JsonReadString(json, ("key_color_" + token).c_str(), stringValue)) ParseColor(WidenAscii(stringValue), style.color);
        if (JsonReadInt(json, ("key_opacity_" + token).c_str(), intValue)) style.opacity = ClampInt(intValue, 0, 100);
    }
    return true;
}

void LoadConfig() {
    ResolveConfigPaths();
    if (!LoadJsonConfig()) {
        LoadIniConfig();
    }
    ClampAndRebuildConfig();
}

bool SaveIniConfig() {
    // Canonical mode, theme and key names are ASCII. Write a complete compatible
    // INI in one replacement instead of leaving a partially updated set of keys.
    auto ascii = [](const std::wstring& s) { return std::string(s.begin(), s.end()); };
    std::ostringstream out;
    out << "[overlay]\r\n";
    auto field = [&](const char* key, auto value) { out << key << '=' << value << "\r\n"; };
    field("x", gConfig.x); field("y", gConfig.y);
    field("width", gConfig.width); field("height", gConfig.height);
    field("duration_ms", gConfig.durationMs); field("travel_px", gConfig.travelPx);
    field("note_width", gConfig.noteWidth); field("note_height", gConfig.noteHeight);
    field("visual_preset", ascii(gConfig.visualPreset));
    field("opacity_percent", gConfig.opacityPercent); field("lane_spacing_percent", gConfig.laneSpacingPercent);
    field("long_note_alpha", gConfig.longNoteAlpha); field("debug_background", gConfig.debugBackground);
    field("click_through", gConfig.clickThrough); field("always_on_top", gConfig.alwaysOnTop);
    field("circle_notes", gConfig.circleNotes); field("lane_background", gConfig.laneBackground);
    field("rounded_boxes", gConfig.roundedBoxes); field("thin_outline", gConfig.thinOutline);
    field("key_text_bottom", gConfig.keyTextBottom);
    field("difficulty_enabled", gConfig.difficultyEnabled); field("difficulty_hud", gConfig.difficultyHud);
    field("key_viewer_folded", gConfig.keyViewerFolded);
    field("analysis_key_mode", gConfig.analysisKeyMode);
    field("report_auto_save", gConfig.reportAutoSave);
    field("game_overlay", gConfig.gameOverlay);
    out << "\r\n[input]\r\n";
    field("mode", ascii(gConfig.mode)); field("lanes", ascii(CanonicalLaneText()));
    out << "\r\n[input_presets]\r\n";
    for (std::size_t i = 0; i < kPresets.size(); ++i)
        if (!gConfig.presetLanes[i].empty()) out << ascii(kPresets[i].mode) << '=' << ascii(gConfig.presetLanes[i]) << "\r\n";
    out << "\r\n[key_styles]\r\n";
    for (UINT vk = 0; vk < gConfig.keyStyles.size(); ++vk) {
        const auto token = TokenFromVk(vk);
        if (token.empty()) continue;
        const auto& style = gConfig.keyStyles[vk];
        if (style.color != CLR_INVALID) field(("color_" + ascii(token)).c_str(), ascii(ColorHex(style.color)));
        if (style.opacity != 100) field(("opacity_" + ascii(token)).c_str(), style.opacity);
    }
    if (!WriteTextFile(gIniConfigPath, out.str())) return false;
    // Invalidate Win32's profile cache after replacing the file externally.
    // This all-null cache-flush call returns zero even on success.
    WritePrivateProfileStringW(nullptr, nullptr, nullptr, gIniConfigPath.c_str());
    return true;
}

std::string BoolText(bool value) {
    return value ? "true" : "false";
}

bool SaveJsonConfig() {
    std::ostringstream out;
    out << "{\n";
    out << "  \"overlay\": {\n";
    out << "    \"x\": " << gConfig.x << ",\n";
    out << "    \"y\": " << gConfig.y << ",\n";
    out << "    \"width\": " << gConfig.width << ",\n";
    out << "    \"height\": " << gConfig.height << ",\n";
    out << "    \"duration_ms\": " << gConfig.durationMs << ",\n";
    out << "    \"travel_px\": " << gConfig.travelPx << ",\n";
    out << "    \"note_width\": " << gConfig.noteWidth << ",\n";
    out << "    \"note_height\": " << gConfig.noteHeight << ",\n";
    out << "    \"visual_preset\": \"" << JsonEscape(gConfig.visualPreset) << "\",\n";
    out << "    \"opacity_percent\": " << gConfig.opacityPercent << ",\n";
    out << "    \"lane_spacing_percent\": " << gConfig.laneSpacingPercent << ",\n";
    out << "    \"long_note_alpha\": " << gConfig.longNoteAlpha << ",\n";
    out << "    \"debug_background\": " << BoolText(gConfig.debugBackground) << ",\n";
    out << "    \"click_through\": " << BoolText(gConfig.clickThrough) << ",\n";
    out << "    \"always_on_top\": " << BoolText(gConfig.alwaysOnTop) << ",\n";
    out << "    \"circle_notes\": " << BoolText(gConfig.circleNotes) << ",\n";
    out << "    \"lane_background\": " << BoolText(gConfig.laneBackground) << ",\n";
    out << "    \"rounded_boxes\": " << BoolText(gConfig.roundedBoxes) << ",\n";
    out << "    \"thin_outline\": " << BoolText(gConfig.thinOutline) << ",\n";
    out << "    \"key_text_bottom\": " << BoolText(gConfig.keyTextBottom) << ",\n";
    out << "    \"difficulty_enabled\": " << BoolText(gConfig.difficultyEnabled) << ",\n";
    out << "    \"difficulty_hud\": " << BoolText(gConfig.difficultyHud) << ",\n";
    out << "    \"key_viewer_folded\": " << BoolText(gConfig.keyViewerFolded) << ",\n";
    out << "    \"analysis_key_mode\": " << gConfig.analysisKeyMode << ",\n";
    out << "    \"report_auto_save\": " << BoolText(gConfig.reportAutoSave) << ",\n";
    out << "    \"game_overlay\": " << BoolText(gConfig.gameOverlay) << "\n";
    out << "  },\n";
    out << "  \"input\": {\n";
    out << "    \"mode\": \"" << JsonEscape(gConfig.mode) << "\",\n";
    out << "    \"lanes\": [";
    std::vector<std::wstring> lanes = SplitLaneText(CanonicalLaneText());
    for (size_t i = 0; i < lanes.size(); ++i) {
        if (i != 0) out << ", ";
        out << "\"" << JsonEscape(lanes[i]) << "\"";
    }
    out << "]";
    for (std::size_t i = 0; i < kPresets.size(); ++i)
        if (!gConfig.presetLanes[i].empty())
            out << ",\n    \"preset_" << _wtoi(kPresets[i].mode) << "k\": \"" << JsonEscape(gConfig.presetLanes[i]) << "\"";
    out << "\n";
    out << "  },\n  \"key_styles\": {";
    bool firstStyle = true;
    for (UINT vk = 0; vk < gConfig.keyStyles.size(); ++vk) {
        const auto wideToken = TokenFromVk(vk);
        if (wideToken.empty()) continue;
        const std::string token(wideToken.begin(), wideToken.end());
        const auto& style = gConfig.keyStyles[vk];
        auto separator = [&]() { out << (firstStyle ? "\n" : ",\n"); firstStyle = false; };
        if (style.color != CLR_INVALID) {
            separator(); out << "    \"key_color_" << token << "\": \"" << JsonEscape(ColorHex(style.color)) << "\"";
        }
        if (style.opacity != 100) { separator(); out << "    \"key_opacity_" << token << "\": " << style.opacity; }
    }
    out << "\n  }\n}\n";
    return WriteTextFile(gJsonConfigPath, out.str());
}

bool SaveConfig() {
    ClampAndRebuildConfig();
    gIniSaveFailed = false;
    gConfigSaveError = ERROR_SUCCESS;
    if (!SaveJsonConfig()) { gConfigSaveError = GetLastError(); return false; }
    // JSON is authoritative at startup. A compatibility-copy failure must not
    // pretend that successfully committed settings disappeared.
    gIniSaveFailed = !SaveIniConfig();
    if (gIniSaveFailed) gConfigSaveError = GetLastError();
    return true;
}

void SetStatus(const std::wstring& text) {
    HWND status = GetDlgItem(gSettingsWnd, IDC_STATUS);
    if (status) {
        SetWindowTextW(status, text.c_str());
    }
}

std::wstring WindowText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    std::wstring value(static_cast<size_t>(len + 1), L'\0');
    if (len > 0) {
        GetWindowTextW(hwnd, value.data(), len + 1);
    }
    value.resize(static_cast<size_t>(len));
    return value;
}

std::wstring DifficultyProfileName() {
    if (gConfig.analysisKeyMode == -1 && gAnalysisKeys.Pending()) return L"자동 키 수 추정";
    const int keys = gConfig.analysisKeyMode == 0 ? static_cast<int>(gConfig.lanes.size()) : gAnalysisKeys.Keys();
    switch (pattern::ProfileForKeys(keys)) {
    case pattern::RatingProfile::Osu4: return L"Reform 4K";
    case pattern::RatingProfile::Osu7: return L"Jinjin 7K";
    case pattern::RatingProfile::Osu8: return L"Regular 8K";
    case pattern::RatingProfile::Osu10: return L"Revive 10K";
    case pattern::RatingProfile::Aery5: return L"AERY 5K";
    default: return std::to_wstring(keys) + L"K 입력 지수";
    }
}

std::wstring FormatDifficultyRating(const pattern::Snapshot& s) {
    std::wostringstream out;
    out << std::fixed;
    if (s.outsideCalibration < 0) out << L"<";
    if (s.outsideCalibration > 0) out << L">";
    switch (s.profile) {
    case pattern::RatingProfile::Osu4: case pattern::RatingProfile::Osu7: case pattern::RatingProfile::Osu8: case pattern::RatingProfile::Osu10:
        return pattern::FormatDan(s.profile, pattern::EstimateDan(s));
    case pattern::RatingProfile::Aery5: out << L"⑤" << std::setprecision(1) << s.rating; break;
    default: out << std::setprecision(0) << s.score << L"/100"; break;
    }
    return out.str();
}

std::wstring DifficultyValue() {
    if (!gConfig.difficultyEnabled || gCaptureMode != CaptureMode::None) return L"정지";
    if (gDifficultySnapshot.limited) return L"보류";
    if (gDifficultySnapshot.idle) return L"대기";
    if (gDifficultySnapshot.keyModeConflict) return L"키 수 확인";
    if (gDifficultySnapshot.keyModePending) return gAnalysisKeys.Rechecking() ? L"키 보정 중" : L"키 추정 중";
    if (!gDifficultySnapshot.ready) return L"분석 중";
    return FormatDifficultyRating(gDifficultySnapshot);
}

std::wstring PreviousPatternValue() {
    const auto& completed = gPatternSummary.Completed();
    return completed.ready ? FormatDifficultyRating(completed) : L"측정 전";
}

void EnsureHudFonts() {
    auto make = [](int size, int weight) {
        return CreateFontW(size, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
    };
    if (!gHudValueFont) gHudValueFont = make(48, FW_BOLD);
    if (!gHudMediumFont) gHudMediumFont = make(32, FW_BOLD);
    if (!gHudCompactFont) gHudCompactFont = make(26, FW_BOLD);
    if (!gHudSmallFont) gHudSmallFont = make(22, FW_BOLD);
    if (!gHudLabelFont) gHudLabelFont = make(13, FW_NORMAL);
    if (!gOsuWarningFont) gOsuWarningFont = make(28, FW_BOLD);
}

void DestroyHudFonts() {
    for (HFONT* font : {&gHudValueFont, &gHudMediumFont, &gHudCompactFont, &gHudSmallFont, &gHudLabelFont, &gOsuWarningFont}) {
        if (*font) DeleteObject(*font);
        *font = nullptr;
    }
}

HFONT FitDifficultyFont(HDC dc, const std::wstring& text, int width, HFONT preferred, int height = 1000) {
    HGDIOBJ old = SelectObject(dc, preferred);
    HFONT chosen = preferred;
    TEXTMETRICW preferredMetrics{};
    GetTextMetricsW(dc, &preferredMetrics);
    for (HFONT font : {preferred, gHudMediumFont, gHudCompactFont, gHudSmallFont, gGuiFont, gHudLabelFont}) {
        if (!font) continue;
        SelectObject(dc, font);
        SIZE extent{};
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &extent);
        if (extent.cy > preferredMetrics.tmHeight) continue;
        chosen = font;
        if (extent.cx <= width && extent.cy <= height) break;
    }
    SelectObject(dc, old);
    return chosen;
}

void SetDifficultyText(HWND control, const std::wstring& text) {
    if (!control) return;
    RECT bounds{}; GetClientRect(control, &bounds);
    HDC dc = GetDC(control);
    HFONT font = FitDifficultyFont(dc, text, bounds.right - 2, gHudValueFont, bounds.bottom);
    ReleaseDC(control, dc);
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SetWindowTextW(control, text.c_str());
}

std::wstring DifficultyDetails(bool multiline) {
    const auto& s = gDifficultySnapshot;
    std::wostringstream out;
    out << std::fixed << std::setprecision(1);
    if (multiline) {
        out << L"8초 평균  " << s.nps << L" NPS     |     순간 밀도  " << s.burstNps << L" NPS (2초)\n"
            << L"동시치기  " << s.chordPercent << L" %     |     최대 " << s.maxChord << L"키 동시\n"
            << L"같은 키 연타  " << s.jackPercent << L" %";
        const auto ln = pattern::FormatLnDan(s);
        if (!ln.empty()) out << L"     |     4K LN v2 비교: " << ln;
        out << L"\n"
            << L"홀드 중 추가 입력  " << s.holdOverlapPercent << L" %\n"
            << L"최근 8초 입력  " << s.samples << L"회     |     누르고 있는 키  " << s.heldKeys << L"개";
        if (s.keyModeConflict) out << L"\n고정 수보다 많은 키 감지 · 분석 초기화 또는 키 수 설정 변경";
        else if (s.keyModePending) out << L"\n자동 추정: " << gAnalysisKeys.Observed() << L"키 관측 · 8초/32회/키별 2회 · 새 키 없이 3초";
        else if (gConfig.analysisKeyMode != 0) out << L"\n" << (gConfig.analysisKeyMode == -1 ? L"자동 고정 " : L"수동 고정 ")
            << gAnalysisKeys.Keys() << L"K · 등록 배치 " << gConfig.lanes.size() << L"키 유지";
        else if (s.retained) out << L"\n입력 연결 대기 · 마지막 추정 유지 (10초 무입력 시 결과 확정)";
        else if (s.ready && s.ratingError > 0) out << L"\n기존 오차 ±" << s.ratingError << L" · 보조·저레벨·LN 조정 전 검증값";
        const auto dan = pattern::EstimateDan(s);
        if (s.ready && dan.available && !s.retained && gConfig.analysisKeyMode == 0) {
            out << L"\n기준: " << pattern::DanBasis(s.profile) << L"  |  참고 ★" << std::setprecision(2) << s.stars;
            if (s.profile == pattern::RatingProfile::Osu10) out << L"  |  패턴 " << s.rating;
            if (dan.outside != 0) out << (dan.outside < 0 ? L"  |  기준 최저 미만 ↓" : L"  |  기준 최고 초과 ↑");
            else out << L"  |  구간 상당";
        }
    } else {
        out << s.nps << L" NPS  |  순간 " << s.burstNps << L"  |  동시 " << s.maxChord << L"K  |  연타 " << s.jackPercent << L"%";
    }
    return out.str();
}

std::filesystem::path ReportDirectory() {
    return std::filesystem::path(gJsonConfigPath).parent_path() / L"reports";
}

std::string ReportStartTime() {
    SYSTEMTIME time{};
    GetLocalTime(&time);
    char text[64];
    std::snprintf(text, sizeof(text), "%04u-%02u-%02u %02u:%02u:%02u (local)",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    return text;
}

void FinishSessionReport(const std::string& reason, double nowMs) {
    if (!gReportRecorder.Active()) return;
    auto session = gReportRecorder.Take(reason, nowMs);
    if (session.presses) gPendingReports.push_back(std::move(session));
    // This can run inside the hook: queue only, never render HTML or touch disk.
}

void ObserveSessionReport(const pattern::Snapshot& snapshot, double nowMs) {
    if (!gReportRecorder.Active()) return;
    if (gReportRecorder.Finished(nowMs)) FinishSessionReport("마지막 입력 후 10초 · 자동 완료", nowMs);
    else gReportRecorder.Observe(snapshot, nowMs);
}

void PollReportWrites(bool wait) {
    do {
        if (gReportWrite.valid()) {
            if (!wait && gReportWrite.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
            report::SaveResult result;
            try { result = gReportWrite.get(); }
            catch (...) { result.error = ERROR_NOT_ENOUGH_MEMORY; }
            if (result.ok) {
                gPendingReports.pop_front();
                gReportStatus = L"저장 완료: " + result.path.filename().wstring();
                gReportSaveFailed = false;
            } else {
                gReportSaveFailed = true;
                gReportStatus = L"저장 실패 (오류 " + std::to_wstring(result.error) + L"). 기록을 보관 중입니다. [저장 재시도]를 눌러 주세요.";
            }
        }
        if (gPendingReports.empty() || gReportSaveFailed) return;
        try {
            // The worker owns a copy; the queue retains the original until a
            // successful atomic save, including on disk-full/permission failures.
            gReportWrite = std::async(std::launch::async, report::Save, gPendingReports.front(), ReportDirectory());
        } catch (...) {
            gReportSaveFailed = true;
            gReportStatus = L"리포트 저장 작업을 시작하지 못했어요. [저장 재시도]를 눌러 주세요.";
        }
    } while (wait && !gReportSaveFailed);
}

void RefreshReportStatus() {
    std::wstring status = gReportStatus;
    if (gReportWrite.valid()) status = L"완료한 연주 리포트를 저장하고 있어요.";
    else if (gPendingReports.size() >= kMaxPendingReports)
        status = L"미저장 리포트 4개: 새 기록을 일시 중지했어요. 폴더 권한·공간 확인 후 저장 재시도.";
    else if (!gReportSaveFailed && gReportRecorder.Active()) status = L"기록 중 · 마지막 입력 후 10초가 지나면 자동 저장합니다.";
    else if (!gReportSaveFailed && gConfig.reportAutoSave && gReportWaitForFreshSession)
        status = L"저장 복구 완료 · 10초간 입력을 멈춘 뒤 시작하는 새 연주부터 기록합니다.";
    else if (status.empty()) status = gConfig.reportAutoSave ? L"저장 대기 · 설정 폴더의 reports에 HTML로 저장합니다." : L"자동 저장 꺼짐 · 켜고 [적용 + 저장]하면 레인·타이밍을 기록합니다.";
    SetDlgItemTextW(gSettingsWnd, IDC_REPORT_STATUS, status.c_str());
    EnableWindow(GetDlgItem(gSettingsWnd, IDC_REPORT_RETRY), gReportSaveFailed);
}

#if KEYNOTE_OSU_MEMORY_LINK
void ApplyOsuLinkView(const osu_link::View& next) {
    const bool changed=next.enabled!=gOsuView.enabled || next.ready!=gOsuView.ready || next.status!=gOsuView.status ||
        next.title!=gOsuView.title || next.variant!=gOsuView.variant || next.keys!=gOsuView.keys || next.difficulty!=gOsuView.difficulty ||
        next.details!=gOsuView.details || next.reviveAuxiliary!=gOsuView.reviveAuxiliary ||
        next.reviveCompact!=gOsuView.reviveCompact || next.reviveSkills!=gOsuView.reviveSkills || next.lnDifficulty!=gOsuView.lnDifficulty;
    gOsuView=next;
    if(!changed) return;
    SetDlgItemTextW(gSettingsWnd,IDC_OSU_STATUS,next.status.c_str());
    SetDlgItemTextW(gSettingsWnd,IDC_OSU_TITLE,next.ready?(next.title+L" ["+next.variant+L"]").c_str():L"선택 채보 대기");
    SetDlgItemTextW(gSettingsWnd,IDC_OSU_RATING,next.ready?(L"원본 "+std::to_wstring(next.keys)+L"K · 예상 "+next.difficulty).c_str():L"—");
    SetDlgItemTextW(gSettingsWnd,IDC_OSU_DETAILS,next.ready?next.details.c_str():L"");
    SetDlgItemTextW(gSettingsWnd,IDC_OSU_REVIVE,next.ready?next.reviveAuxiliary.c_str():L"");
    SetDlgItemTextW(gSettingsWnd,IDC_OSU_SKILLS,next.ready?next.reviveSkills.c_str():L"");
    if(gOverlayWnd) InvalidateRect(gOverlayWnd,nullptr,FALSE);
}

void RefreshOsuLinkDisplay() { ApplyOsuLinkView(gOsuLink.Current()); }

void ToggleOsuLink() {
    const bool enable=SendDlgItemMessageW(gSettingsWnd,IDC_OSU_ENABLE,BM_GETCHECK,0,0)==BST_CHECKED;
    if(!enable) gOsuLink.Stop();
    else {
        const int choice=gOsuRiskMessageBox(gSettingsWnd,
            L"이 기능의 위험부담은 전부 사용자 몫입니다!!!\n\n"
            L"osu! 프로세스 메모리를 읽는 비공식 실험 기능입니다. 안티치트 제재나 계정 불이익이 없다고 보장하지 않습니다.\n\n"
            L"메모리 수정·DLL 주입 없이 선택 채보 정보만 조회합니다. 이번 실행에서만 켜며, 재실행하면 OFF입니다.\n\n위험을 이해하고 켜겠습니까?",
            L"위험 경고 · osu! 읽기 전용 연동",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2);
        // UI self-tests must never attach to an unrelated running game.
        if(choice!=IDYES || gUiSelfTest || !gOsuLink.Start(true,gOsuSongsOverride))
            SendDlgItemMessageW(gSettingsWnd,IDC_OSU_ENABLE,BM_SETCHECK,BST_UNCHECKED,0);
    }
    RefreshOsuLinkDisplay();
}

void ChooseOsuSongs() {
    // Folder changes stop an active reader; selecting a directory is not consent
    // to resume process access. The checkbox requires the warning again.
    const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    BROWSEINFOW info{};info.hwndOwner=gSettingsWnd;info.lpszTitle=L"osu!의 Songs 폴더 선택 (이번 실행에만 적용)";
    info.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NONEWFOLDERBUTTON;
    PIDLIST_ABSOLUTE item=SHBrowseForFolderW(&info);
    wchar_t path[MAX_PATH]{};
    if(item && SHGetPathFromIDListW(item,path)) {
        gOsuLink.Stop();gOsuSongsOverride=path;
        SendDlgItemMessageW(gSettingsWnd,IDC_OSU_ENABLE,BM_SETCHECK,BST_UNCHECKED,0);
        RefreshOsuLinkDisplay();SetStatus(L"Songs 폴더를 지정했어요. 위험 안내를 확인하고 연동을 켜세요. 폴더 경로는 저장하지 않습니다.");
    }
    CoTaskMemFree(item);if(SUCCEEDED(initialized))CoUninitialize();
}

#endif

void RefreshDifficultyDisplay(double nowMs = NowMs()) {
#if KEYNOTE_OSU_MEMORY_LINK
    RefreshOsuLinkDisplay();
#endif
    UpdateGameOverlay(nowMs);
    HWND headline = GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_SCORE);
    HWND metrics = GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_METRICS);
    const std::wstring oldTitle = WindowText(headline);
    const std::wstring oldDetails = WindowText(metrics);
    HWND npsControl = GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_NPS);
    const std::wstring oldNps = WindowText(npsControl);
    HWND previousControl = GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_PREVIOUS);
    const std::wstring oldPrevious = WindowText(previousControl);
    gDifficultySnapshot = gConfig.difficultyEnabled && gCaptureMode == CaptureMode::None ?
        EvaluateLiveDifficulty(nowMs) : pattern::Snapshot{};
    auto skillsText = [] {
        return revive_aux::Details(gReviveLive.Current()) + gReviveLive.Current().status +
            revive_aux::Details(gCompletedSkills.Completed());
    };
    const auto oldSkills = skillsText();
    gReviveLive.Update(nowMs, gConfig.difficultyEnabled && gCaptureMode == CaptureMode::None,
        gAnalysisKeys.Keys(), gDifficultySnapshot.keyModePending, gDifficultySnapshot.keyModeConflict,
        gDifficultySnapshot.limited, {gDifficultySnapshot.ready && !gDifficultySnapshot.retained,
            gDifficultySnapshot.profile == pattern::RatingProfile::Generic ?
                gDifficultySnapshot.score : gDifficultySnapshot.rating});
    if (gConfig.difficultyEnabled && gCaptureMode == CaptureMode::None) ObservePatternSummary(gDifficultySnapshot);
    const bool skillsChanged = oldSkills != skillsText();
    ObserveSessionReport(gDifficultySnapshot, nowMs);
    PollReportWrites();
    RefreshReportStatus();
    const std::wstring title = DifficultyValue();
    const std::wstring details = DifficultyDetails(true);
    const std::wstring previous = PreviousPatternValue();
    if (oldPrevious != previous) SetDifficultyText(previousControl, previous);
    SetDlgItemTextW(gSettingsWnd, IDC_DIFFICULTY_PROFILE,
        (DifficultyProfileName() + (gDifficultySnapshot.retained ? L" · 연결 대기" : L" · 분석")).c_str());
    std::wostringstream nps;
    nps << std::fixed << std::setprecision(1) << L"NPS  " << gDifficultySnapshot.currentNps;
    if (npsControl && oldNps != nps.str()) SetWindowTextW(npsControl, nps.str().c_str());
    if (oldTitle != title) SetDifficultyText(headline, title);
    if (metrics && oldDetails != details) SetWindowTextW(metrics, details.c_str());
    double bar = gDifficultySnapshot.score;
    switch (gDifficultySnapshot.profile) {
    case pattern::RatingProfile::Osu4: case pattern::RatingProfile::Osu7: case pattern::RatingProfile::Osu8: case pattern::RatingProfile::Osu10:
        bar = pattern::EstimateDan(gDifficultySnapshot).progress; break;
    case pattern::RatingProfile::Aery5: bar = gDifficultySnapshot.rating / 20.5 * 100.0; break;
    default: break;
    }
    SendDlgItemMessageW(gSettingsWnd, IDC_DIFFICULTY_BAR, PBM_SETPOS,
        gDifficultySnapshot.ready ? static_cast<WPARAM>(ClampInt(static_cast<int>(std::round(bar)), 0, 100)) : 0, 0);
    if (gOverlayWnd && gConfig.difficultyEnabled && gConfig.difficultyHud &&
        (skillsChanged || oldTitle != title || oldDetails != details || oldNps != nps.str() || oldPrevious != previous)) InvalidateRect(gOverlayWnd, nullptr, FALSE);
}

revive_hud::Layout CurrentHudLayout(int width) {
#if KEYNOTE_OSU_MEMORY_LINK
    const bool selected = gOsuView.enabled;
#else
    constexpr bool selected = false;
#endif
    const bool liveLn = gDifficultySnapshot.profile == pattern::RatingProfile::Osu4;
#if KEYNOTE_OSU_MEMORY_LINK
    const bool selectedLn = selected && gOsuView.ready && gOsuView.keys == 4;
#else
    constexpr bool selectedLn = false;
#endif
    return revive_hud::Make(width, gConfig.difficultyEnabled && gConfig.difficultyHud, selected, liveLn, selectedLn);
}

int DifficultyHeaderHeight(int width, int /*height*/) { return CurrentHudLayout(width).total; }
int OverlayPresentationHeight(int width) {
    if (KeyViewerFolded()) return FoldedHudPixels(CurrentHudLayout(FoldedHudWidth(width)).total + (gMoveMode ? 28 : 0));
    return revive_hud::PresentationHeight(gConfig.height, gConfig.noteHeight, CurrentHudLayout(width));
}

void DrawSkillGrid(HDC dc, int width, int top, const std::wstring& source,
                   const revive_aux::Summary& skills, const revive_hud::Layout& layout) {
    RECT panel{4, top, width - 4, top + layout.skillHeight - 1};
    HBRUSH brush = CreateSolidBrush(RGB(10, 16, 26));
    FillRect(dc, &panel, brush); DeleteObject(brush);
    SetTextColor(dc, RGB(184, 207, 255));
    RECT caption{8, top, width - 8, top + 28};
    const auto text = source + (skills.ready ? L"" : L" · " + skills.status);
    SelectObject(dc, FitDifficultyFont(dc,text,width-16,gHudSmallFont,28));
    DrawTextW(dc, text.c_str(), -1, &caption, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    // Every axis keeps its position, including zero and unavailable values.
    // Width controls the row count; no ranking or ellipsis hides an axis.
    for (int i = 0; i < 6; ++i) {
        const int column = i % layout.columns, row = i / layout.columns;
        RECT cell{4 + (width - 8) * column / layout.columns, top + 28 + row * layout.rowHeight,
                  4 + (width - 8) * (column + 1) / layout.columns, top + 26 + (row+1) * layout.rowHeight};
        RECT name{cell.left+4,cell.top,cell.right-4,cell.bottom};
        RECT number=name;
        if (layout.stackedSkills) { name.bottom=name.top+24; number.top=name.bottom; }
        else { number.left=number.right-50; name.right=number.left; }
        const auto skillName=revive_aux::SkillName(i), value=revive_aux::SkillValue(skills,i);
        const bool available=revive_aux::SkillAvailable(skills,i);
        SetTextColor(dc, available ? RGB(169, 245, 180) : RGB(151, 161, 176));
        SelectObject(dc,gHudSmallFont);
        DrawTextW(dc,skillName.c_str(),-1,&name,DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
        SelectObject(dc,layout.stackedSkills?gHudMediumFont:gHudCompactFont);
        DrawTextW(dc,value.c_str(),-1,&number,DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
        if (available) {
            RECT bar{cell.left + 6, cell.bottom - 1,
                cell.left + 6 + static_cast<LONG>((cell.right - cell.left - 12) * skills.skills[i] / 100.0), cell.bottom + 1};
            brush = CreateSolidBrush(RGB(86, 169, 130)); FillRect(dc, &bar, brush); DeleteObject(brush);
        }
    }
}

void DrawLnComparison(HDC dc, int width, int top, const std::wstring& value) {
    const bool stacked=width<600;
    RECT row{4, top, width - 4, top + (stacked?60:40)};
    HBRUSH brush = CreateSolidBrush(RGB(10, 16, 26)); FillRect(dc, &row, brush); DeleteObject(brush);
    SetTextColor(dc, RGB(220, 195, 255));
    RECT caption{8,top,width-8,row.bottom}, number=caption;
    if(stacked) {caption.bottom=top+24;number.top=caption.bottom;}
    else {caption.right=96;number.left=100;}
    SelectObject(dc,gHudSmallFont);
    DrawTextW(dc,L"4K LN",-1,&caption,DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    const auto text=value.empty()?L"측정 대기":value.rfind(L"LN ",0)==0?value.substr(3):value;
    SelectObject(dc,FitDifficultyFont(dc,text,number.right-number.left,gHudMediumFont,number.bottom-number.top));
    DrawTextW(dc,text.c_str(),-1,&number,DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}

// Narrow HUDs have one LN cell: live input first, then the finished pattern.
std::wstring HudLnValue(int width) {
    const auto value = pattern::FormatLnDan(gDifficultySnapshot);
    const auto previous = pattern::FormatLnDan(gPatternSummary.Completed());
    if (previous.empty()) return value;
    if (value.empty()) return L"완료 " + previous.substr(3);
    return width >= 600 ? value + L" · 완료 " + previous : value;
}

// Keep the labelled previous result while new input is still collecting.
// The live meter remains independent and takes over once it has valid axes.
bool ShowsCompletedSkills() {
    return !gReviveLive.Current().ready && gCompletedSkills.Completed().ready;
}

void DrawDifficultyHud(HDC dc, int width, int headerHeight) {
    if (headerHeight == 0) return;
    EnsureHudFonts();
    const auto layout = CurrentHudLayout(width);
    const bool large = width >= 600;
    const COLORREF colors[] = {RGB(112, 229, 255), RGB(169, 245, 180), RGB(255, 222, 138)};
    std::wostringstream nps;
    nps << std::fixed << std::setprecision(1) << gDifficultySnapshot.currentNps;
    std::wstring label = DifficultyProfileName() + (gDifficultySnapshot.retained ? L" · 유지" : L" · 추정");
    if (large && gDifficultySnapshot.ready && gDifficultySnapshot.ratingError > 0) {
        std::wostringstream error;
        error << std::fixed << std::setprecision(1) << L" · 기존 ±" << gDifficultySnapshot.ratingError;
        label += error.str();
    }
    const std::wstring labels[] = {label, L"완료된 패턴 · 측정 레벨", L"NPS · 최근 1초"};
    const std::wstring values[] = {DifficultyValue(), PreviousPatternValue(), nps.str()};
    HGDIOBJ oldFont = SelectObject(dc, gHudLabelFont ? gHudLabelFont : gGuiFont);
    SetBkMode(dc, TRANSPARENT);
    const int npsWidth = std::min(180, width / 5);
    const int ratingWidth = (width - npsWidth) / 2;
    for (int i = 0; layout.mainHeight > 0 && i < 3; ++i) {
        const bool stacked = width < 600;
        RECT panel = stacked ? RECT{4, i == 0 ? 0 : i == 1 ? 30 : 60, width - 4, i == 0 ? 30 : i == 1 ? 60 : 76} :
            RECT{i == 0 ? 4 : i == 1 ? ratingWidth + 2 : width - npsWidth + 2, 2,
                i == 0 ? ratingWidth - 2 : i == 1 ? width - npsWidth - 2 : width - 4, layout.mainHeight - 2};
        HBRUSH brush = CreateSolidBrush(RGB(10, 16, 26));
        FillRect(dc, &panel, brush); DeleteObject(brush);
        RECT accent{panel.left, panel.bottom - (stacked ? 1 : 3), panel.right, panel.bottom};
        brush = CreateSolidBrush(colors[i]); FillRect(dc, &accent, brush); DeleteObject(brush);
        SelectObject(dc, gHudLabelFont ? gHudLabelFont : gGuiFont);
        SetTextColor(dc, RGB(209, 221, 235));
        RECT caption = stacked ? RECT{panel.left + 4, panel.top, 64, panel.bottom - 1} :
            RECT{panel.left + 4, 3, panel.right - 4, 22};
        const wchar_t* shortLabel = i == 0 ? L"실시간" : i == 1 ? L"완료" : L"NPS";
        DrawTextW(dc, stacked ? shortLabel : labels[i].c_str(), -1, &caption,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        SetTextColor(dc, colors[i]);
        RECT number = stacked ? RECT{68, panel.top, panel.right - 3, panel.bottom - 1} :
            RECT{panel.left + 3, 20, panel.right - 3, panel.bottom - 3};
        HFONT preferred = stacked ? (i == 2 ? gHudLabelFont : gHudCompactFont) : gHudValueFont;
        SelectObject(dc, FitDifficultyFont(dc, values[i], number.right - number.left,
            preferred, number.bottom - number.top));
        DrawTextW(dc, values[i].c_str(), -1, &number, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    }
    if (layout.liveLnHeight) DrawLnComparison(dc, width, layout.mainHeight, HudLnValue(width));
    if (layout.mainHeight) {
        const bool completed = ShowsCompletedSkills();
        DrawSkillGrid(dc, width, layout.mainHeight + layout.liveLnHeight, completed ? L"보조지표 · 완료" : L"보조지표",
            completed ? gCompletedSkills.Completed() : gReviveLive.Current(), layout);
    }
#if KEYNOTE_OSU_MEMORY_LINK
    if (gOsuView.enabled) {
        RECT row{4, layout.selectedTop, width - 4, layout.selectedTop + 18};
        HBRUSH brush = CreateSolidBrush(RGB(10, 16, 26)); FillRect(dc, &row, brush); DeleteObject(brush);
        SelectObject(dc, gHudLabelFont); SetTextColor(dc, RGB(184, 207, 255));
        const auto text = gOsuView.ready ? L"osu! 원본 " + std::to_wstring(gOsuView.keys) + L"K · 예상 " + gOsuView.difficulty +
            L" · 1.00× 모드 미반영 · " + gOsuView.title + L" [" + gOsuView.variant + L"]" : L"osu! · " + gOsuView.status;
        row.left += 4;
        DrawTextW(dc, text.c_str(), -1, &row, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        auto skills = gOsuView.revive;
        if (!gOsuView.ready) { skills = {}; skills.status = L"대기"; }
        if (layout.selectedLnHeight) DrawLnComparison(dc, width, layout.selectedTop + 18, gOsuView.lnDifficulty);
        DrawSkillGrid(dc, width, layout.selectedTop + 18 + layout.selectedLnHeight, L"채보 보조지표", skills, layout);
    }
#endif
    SelectObject(dc, oldFont);
}

bool GameInputAllowed() {
    if (!gConfig.gameOverlay) return true;
    HWND foreground = GetForegroundWindow();
    return game_overlay::AllowsInput(true, gForegroundClassifier.Identify(foreground),
        foreground == gSettingsWnd && gCurrentPage == 3);
}

void RecordDifficultyInput(int lane, bool down, double timeMs) {
    if (!gConfig.difficultyEnabled || gCaptureMode != CaptureMode::None) return;
    if (lane < 0 || lane >= static_cast<int>(gConfig.lanes.size()) || !std::isfinite(timeMs) || timeMs < 0) return;
    // Typing configuration values is not a pattern. Releases still close holds
    // started in the game, so switching to settings cannot leave phantom holds.
    if (down && (!GameInputAllowed() || gPickingColor || (GetForegroundWindow() == gSettingsWnd && gCurrentPage != 3))) return;
    const bool wasHeld = gDifficulty.IsHeld(lane);
    bool startsFreshSession = false;
    if (down) {
        // Publish a finished session even if the UI timer was delayed until the
        // first key of the next session. The estimator may reset on this keydown.
        const auto before = EvaluateLiveDifficulty(timeMs);
        startsFreshSession = before.idle;
        ObservePatternSummary(before);
        ObserveSessionReport(before, timeMs);
        gDifficulty.KeyDown(lane, timeMs);
    }
    else gDifficulty.KeyUp(lane, timeMs);
    if (wasHeld != gDifficulty.IsHeld(lane)) {
        gReviveLive.Key(lane, down, timeMs);
        gAnalysisKeys.Key(lane, down, timeMs);
        gLiveContinuity.RecordActivity(timeMs);
        if (gConfig.reportAutoSave) {
            if (down && !gReportRecorder.Active()) {
                if (gPendingReports.size() >= kMaxPendingReports) gReportWaitForFreshSession = true;
                else if (!gReportWaitForFreshSession || startsFreshSession) {
                    gReportWaitForFreshSession = false;
                    gReportRecorder.Begin(static_cast<int>(gConfig.lanes.size()), timeMs, ReportStartTime());
                }
            }
            gReportRecorder.Key(lane, down, timeMs);
        }
        // A newly observed lane makes the old inferred peak untrustworthy even
        // if playback ends before its second press or the next UI refresh.
        if (gAnalysisKeys.Conflict() || gAnalysisKeys.Rechecking()) {
            pattern::Snapshot invalid;
            invalid.idle = false;
            invalid.keyModeConflict = true;
            ObservePatternSummary(invalid);
            gReportRecorder.Observe(invalid, timeMs);
        }
    }
}

void ApplyOverlayWindowStyle() {
    if (!gOverlayWnd) return;

    const LONG_PTR ex = game_overlay::ExtendedStyle(GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE),
        gConfig.gameOverlay, gConfig.clickThrough, gMoveMode);
    SetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE, ex);
    // Presentation uses per-pixel alpha. Calling SetLayeredWindowAttributes here
    // would disable UpdateLayeredWindow until WS_EX_LAYERED is reset.

    SetWindowPos(
        gOverlayWnd,
        gConfig.gameOverlay || gConfig.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        gConfig.x,
        gConfig.y,
        OverlayPresentationWidth(),
        OverlayPresentationHeight(OverlayPresentationWidth()),
        SWP_NOACTIVATE | SWP_FRAMECHANGED | (gUiSelfTest ? 0 : SWP_SHOWWINDOW)
    );
    if (IsSettingsInteraction(GetForegroundWindow())) SetSettingsForegroundBoost(true);
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
}

void ToggleOverlayMoveMode() {
    gMoveMode = !gMoveMode;
    ApplyOverlayWindowStyle();
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_MOVE_MODE), gMoveMode ? L"이동 모드 끝내기" : L"마우스로 위치 이동");
    SetStatus(gMoveMode ? L"오버레이를 잡아 끌어 주세요. 놓은 위치는 자동 저장됩니다. 끝나면 이동 모드를 꺼 주세요." :
        L"이동 모드를 끝냈어요. 원래 클릭 통과 설정으로 돌아갑니다.");
}

void ResetInputRuntimeState() {
    gReportRecorder.ReleaseAll(NowMs());
    gDifficulty.ReleaseAll(NowMs());
    gReviveLive.ReleaseAll(NowMs());
    gAnalysisKeys.ReleaseAll(NowMs());
    // Events posted for the old lane order must not run after a layout edit.
    if (gOverlayWnd) {
        MSG pending{};
        while (PeekMessageW(&pending, gOverlayWnd, WM_APP_KEY_DOWN, WM_APP_KEY_UP, PM_REMOVE)) {}
    }
    gNotes.clear();
    gKeyDown.fill(false);
    gPendingLaneDownMs.fill(0.0);
    gPendingLaneUpMs.fill(0.0);
    gChordAnchorMs = -1000000.0;
}

void RefreshGameOverlay(game_overlay::Game game, double nowMs) {
    if (gConfig.gameOverlay && gForegroundGame != game_overlay::Game::None && game != gForegroundGame) {
        ResetInputRuntimeState(); // Alt-Tab cannot leave an indefinitely held note.
        gLiveContinuity.RecordActivity(nowMs);
    }
    const bool changed = game != gForegroundGame;
    gForegroundGame = game;
    const HWND foreground = GetForegroundWindow();
    const bool editingSettings = IsSettingsInteraction(foreground);
    // Alt-Tab back to an owned modal dialog need not reactivate the settings
    // HWND itself. Reconcile once on that transition without stealing focus.
    if (!gUiSelfTest && gSettingsForegroundBoost != editingSettings) SetSettingsForegroundBoost(editingSettings);
    if (gConfig.gameOverlay && gOverlayWnd && !gUiSelfTest && MayRaiseGameOverlay(foreground)) {
        // Switching to Discord/OBS must not hide or suspend their capture source.
        // Foreground detection still filters input and releases old held notes.
        if (changed || nowMs >= gNextGameRaiseMs || !IsWindowVisible(gOverlayWnd)) {
            // Games may raise their own topmost window on focus/display changes.
            // Raise our independent window without activating it or touching theirs.
            SetWindowPos(gOverlayWnd, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            gNextGameRaiseMs = nowMs + 500;
        }
    }
    const wchar_t* status = !gConfig.gameOverlay ? L"OBS / 바탕화면 모드" : gMoveMode ? L"위치 이동 미리보기 · 끝내도 표시 유지" :
        game == game_overlay::Game::Osu ? L"osu! 감지 · 입력 분석 중" :
        game == game_overlay::Game::Qwilight ? L"Qwilight 감지 · 입력 분석 중" : L"표시 유지 · osu! / Qwilight 입력 대기";
    if (WindowText(GetDlgItem(gSettingsWnd, IDC_GAME_STATUS)) != status)
        SetDlgItemTextW(gSettingsWnd, IDC_GAME_STATUS, status);
}

void UpdateGameOverlay(double nowMs) {
    RefreshGameOverlay(gConfig.gameOverlay ? gForegroundClassifier.Identify(GetForegroundWindow()) : game_overlay::Game::None, nowMs);
}

Note* FindActiveNote(int lane) {
    for (auto it = gNotes.rbegin(); it != gNotes.rend(); ++it) {
        if (it->lane == lane && it->held) {
            return &(*it);
        }
    }
    return nullptr;
}

void BeginLaneHold(int lane) {
    if (!gOverlayWnd || lane < 0 || lane >= static_cast<int>(gConfig.lanes.size())) {
        return;
    }
    if (FindActiveNote(lane)) {
        return;
    }
    Note note;
    double eventMs = TakePendingLaneTime(gPendingLaneDownMs, lane);
    note.id = gNextNoteId++;
    note.lane = lane;
    note.label = gConfig.lanes[static_cast<size_t>(lane)].label;
    note.pressMs = AlignChordPressMs(eventMs);
    note.releaseMs = -1.0;
    note.held = true;
    note.color = LaneColor(lane);
    note.opacity = gConfig.keyStyles[gConfig.lanes[static_cast<size_t>(lane)].vk].opacity;
    gNotes.push_back(std::move(note));
    if (gNotes.size() > 256) {
        gNotes.erase(gNotes.begin(), gNotes.begin() + static_cast<std::ptrdiff_t>(gNotes.size() - 256));
    }
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
    UpdateWindow(gOverlayWnd);
}

void EndLaneHold(int lane) {
    if (!gOverlayWnd || lane < 0 || lane >= static_cast<int>(gConfig.lanes.size())) {
        return;
    }
    Note* note = FindActiveNote(lane);
    if (!note) {
        return;
    }
    note->held = false;
    note->releaseMs = std::max(note->pressMs, TakePendingLaneTime(gPendingLaneUpMs, lane));
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
    UpdateWindow(gOverlayWnd);
}

void SpawnTapNote(int lane) {
    if (!gOverlayWnd || lane < 0 || lane >= static_cast<int>(gConfig.lanes.size())) {
        return;
    }
    Note note;
    double eventMs = AlignChordPressMs(NowMs());
    note.id = gNextNoteId++;
    note.lane = lane;
    note.label = gConfig.lanes[static_cast<size_t>(lane)].label;
    note.pressMs = eventMs;
    note.releaseMs = note.pressMs + 40.0;
    note.held = false;
    note.color = LaneColor(lane);
    note.opacity = gConfig.keyStyles[gConfig.lanes[static_cast<size_t>(lane)].vk].opacity;
    gNotes.push_back(std::move(note));
    if (gNotes.size() > 256) {
        gNotes.erase(gNotes.begin(), gNotes.begin() + static_cast<std::ptrdiff_t>(gNotes.size() - 256));
    }
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
    UpdateWindow(gOverlayWnd);
}

void DrawKeyLabel(HDC dc, const RECT& body, const std::wstring& text, COLORREF color) {
    RECT rect = body;
    if (gConfig.keyTextBottom) {
        rect.top = std::max(rect.top, rect.bottom - 17);
        rect.bottom -= 2;
    } else {
        rect.top += 2;
        rect.bottom = std::min(rect.bottom, rect.top + 17);
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HGDIOBJ oldFont = SelectObject(dc, gKeyFont ? gKeyFont : gNoteFont);
    DrawTextW(dc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
}

bool RenderOverlay(overlay::AlphaSurface& frame, double now) {
    if (!frame) return false;
    const int width = frame.width, height = frame.height;
    RECT client{0,0,width,height};
    HDC mem = frame.dc;
    frame.ClearGdi(client, kTransparentKey);
    VisualTheme theme = CurrentTheme();
    // A transparent pixel cannot receive mouse hits; move mode needs a solid canvas.
    const bool debugBackground = gConfig.debugBackground && !gConfig.gameOverlay;
    COLORREF canvasColor = (debugBackground || gMoveMode) ? theme.background : kTransparentKey;
    HBRUSH bg = CreateSolidBrush(canvasColor);
    if (debugBackground || gMoveMode) FillRect(mem, &client, bg);
    DeleteObject(bg);

    gNotes.erase(std::remove_if(gNotes.begin(), gNotes.end(), [&](const Note& note) {
        return !note.held && now - note.releaseMs >= gConfig.durationMs;
    }), gNotes.end());
    if (KeyViewerFolded()) {
        // Scale vector text and its layout together, including fallback fonts,
        // captions, selected-chart rows and NPS. No bitmap stretching is used.
        const int logicalWidth = FoldedHudWidth(width);
        const int header = CurrentHudLayout(logicalWidth).total;
        const int saved = SaveDC(mem);
        const XFORM scale{1.5f,0,0,1.5f,0,0};
        if (!saved || !SetGraphicsMode(mem, GM_ADVANCED) || !SetWorldTransform(mem, &scale)) {
            if (saved) RestoreDC(mem,saved);
            return false;
        }
        DrawDifficultyHud(mem,logicalWidth,header);
        if (gMoveMode) {
            RECT hint{4,header,logicalWidth-4,header+28};
            SelectObject(mem,gGuiFont); SetBkMode(mem,TRANSPARENT); SetTextColor(mem,RGB(112,229,255));
            DrawTextW(mem,L"이동 모드 · 놓으면 위치 저장",-1,&hint,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
        RestoreDC(mem,saved);
        frame.FinishGdi();
        return true; // Neither lanes, key labels nor note/hold animations are drawn.
    }

    const int contentTop = DifficultyHeaderHeight(width, height);
    const int savedDc = SaveDC(mem);
    IntersectClipRect(mem, 0, contentTop, width, height);

    auto lanes = CurrentLaneDisplay(gConfig.width);
    if (!lanes.compact) lanes = CurrentLaneDisplay(width); // Offscreen previews keep their own canvas size.
    const int laneCount = lanes.registered;
    const int laneSlotW = lanes.slotWidth;
    auto laneCenterX = [&](int lane) { return lanes.Center(lane, width); };

    if (gConfig.laneBackground && laneCount > 0) {
        COLORREF laneFill = BlendColor(theme.laneFill, canvasColor, theme.laneAlpha);
        HBRUSH laneBrush = CreateSolidBrush(laneFill);
        HPEN lanePen = CreatePen(PS_SOLID, 1, BlendColor(theme.laneLine, canvasColor, 72));
        HGDIOBJ oldBrush = SelectObject(mem, laneBrush);
        HGDIOBJ oldPen = SelectObject(mem, lanePen);
        for (int i = 0; i < laneCount; ++i) {
            if (!lanes.Visible(i)) continue;
            int x = laneCenterX(i);
            int laneW = std::max(24, std::min(96, laneSlotW - 8));
            RoundRect(mem, x - laneW / 2, contentTop + 5, x + laneW / 2, height - 5, 8, 8);
        }
        SelectObject(mem, oldPen);
        SelectObject(mem, oldBrush);
        DeleteObject(lanePen);
        DeleteObject(laneBrush);
    }

    if (debugBackground && laneCount > 0) {
        HPEN gridPen = CreatePen(PS_SOLID, 1, BlendColor(theme.laneLine, canvasColor, 118));
        HGDIOBJ oldPen = SelectObject(mem, gridPen);
        for (int i = 0; i < laneCount; ++i) {
            if (!lanes.Visible(i)) continue;
            int x = laneCenterX(i);
            MoveToEx(mem, x, 0, nullptr);
            LineTo(mem, x, height);
        }
        SelectObject(mem, oldPen);
        DeleteObject(gridPen);
    }

    RestoreDC(mem, savedDc);
    DrawDifficultyHud(mem, width, contentTop);
    if (gMoveMode) {
        HBRUSH border = CreateSolidBrush(RGB(112, 229, 255));
        FrameRect(mem, &client, border);
        DeleteObject(border);
        RECT hint{4, std::max(0, height - 25), width - 4, height - 2};
        HBRUSH hintBg = CreateSolidBrush(theme.background);
        FillRect(mem, &hint, hintBg);
        DeleteObject(hintBg);
        auto oldFont = SelectObject(mem, gGuiFont);
        SetBkMode(mem, TRANSPARENT);
        SetTextColor(mem, RGB(112, 229, 255));
        DrawTextW(mem, L"이동 모드 · 놓으면 위치 저장", -1, &hint, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(mem, oldFont);
    }
    frame.FinishGdi();
    if (gNotes.empty()) return true;
    overlay::AlphaSurface noteLayer(width,height);
    if (!noteLayer) return false;
    mem = noteLayer.dc;
    const int noteBottom = gMoveMode ? std::max(contentTop, height - 25) : height;
    IntersectClipRect(mem, gMoveMode ? 1 : 0, std::max(contentTop, gMoveMode ? 1 : 0), width - (gMoveMode ? 1 : 0), noteBottom);

    for (const Note& note : gNotes) {
        if (laneCount <= 0) break;
        if (!lanes.Visible(note.lane) || note.opacity <= 0) continue;
        double pressAge = std::max(0.0, now - note.pressMs);
        double releaseAge = note.held ? 0.0 : std::max(0.0, now - note.releaseMs);
        double holdMs = note.held ? pressAge : std::max(0.0, note.releaseMs - note.pressMs);
        // Reserve HUD space without covering note heads; turning the HUD off
        // restores the original travel distance and rendering geometry.
        const int effectiveHeight = gConfig.circleNotes ? std::min(gConfig.noteWidth, std::max(gConfig.noteHeight, 42)) : gConfig.noteHeight;
        const int travel = contentTop > 0 ? std::min(gConfig.travelPx, std::max(0, height - effectiveHeight - 16 - contentTop)) : gConfig.travelPx;
        double speed = static_cast<double>(travel) / static_cast<double>(std::max(1, gConfig.durationMs));
        double headTravel = std::min(static_cast<double>(travel), speed * pressAge);
        double tailTravel = note.held ? 0.0 : std::min(static_cast<double>(travel), speed * releaseAge);
        double fadeT = note.held ? 0.0 : std::max(0.0, std::min(1.0, releaseAge / gConfig.durationMs));
        int laneX = laneCenterX(note.lane);
        int noteW = ClampInt(gConfig.noteWidth, 24, 220);
        int noteH = ClampInt(gConfig.noteHeight, 18, 140);
        if (gConfig.circleNotes) {
            int diameter = std::min(noteW, std::max(noteH, 42));
            noteW = diameter;
            noteH = diameter;
        }
        int baseCenterY = height - noteH / 2 - 8;
        int centerY = baseCenterY - static_cast<int>(std::round(headTravel));
        int alphaFade = note.held || fadeT < 0.68 ? 255 : static_cast<int>(std::round(255.0 * (1.0 - fadeT) / 0.32));
        alphaFade = std::max(32, std::min(255, alphaFade));

        // Clear/composite only this note's horizontal strip, including the
        // largest pulse ring and shadow. The HUD stays outside the clip region.
        RECT noteRegion{laneX-noteW/2-28,contentTop,laneX+noteW/2+28,noteBottom};
        noteRegion = noteLayer.Bounds(noteRegion);
        noteLayer.ClearGdi(noteRegion, canvasColor);
        COLORREF bodyColor = note.color;
        if (alphaFade < 255) {
            bodyColor = BlendColor(bodyColor, canvasColor, alphaFade);
        }

        bool drawHoldBody = note.held || holdMs >= kHoldThresholdMs;
        if (drawHoldBody) {
            int tailTop = centerY + noteH / 2 - 2;
            int tailBottom = height - 8 - static_cast<int>(std::round(tailTravel));
            tailTop = std::max(0, std::min(height, tailTop));
            tailBottom = std::max(0, std::min(height, tailBottom));
            if (tailBottom > tailTop + 4) {
                int tailW = std::max(12, noteW / 2);
                RECT tailShadow{laneX - tailW / 2 + 3, tailTop + 3, laneX + tailW / 2 + 3, tailBottom + 3};
                HBRUSH tailShadowBrush = CreateSolidBrush(theme.shadow);
                FillRect(mem, &tailShadow, tailShadowBrush);
                DeleteObject(tailShadowBrush);

                RECT tail{laneX - tailW / 2, tailTop, laneX + tailW / 2, tailBottom};
                int tailAlpha = ClampInt(gConfig.longNoteAlpha * alphaFade / 255, 24, 180);
                COLORREF tailColor = BlendColor(note.color, canvasColor, tailAlpha);
                HBRUSH tailBrush = CreateSolidBrush(tailColor);
                HPEN tailPen = CreatePen(PS_SOLID, gConfig.thinOutline ? 1 : 2, BlendColor(theme.outline, canvasColor, 115));
                HGDIOBJ oldBrush = SelectObject(mem, tailBrush);
                HGDIOBJ oldPen = SelectObject(mem, tailPen);
                RoundRect(mem, tail.left, tail.top, tail.right, tail.bottom, 6, 6);
                SelectObject(mem, oldPen);
                SelectObject(mem, oldBrush);
                DeleteObject(tailPen);
                DeleteObject(tailBrush);

                int capH = std::max(10, noteH / 3);
                RECT tailCap{laneX - noteW / 2, tailBottom - capH / 2, laneX + noteW / 2, tailBottom + capH / 2};
                COLORREF capColor = BlendColor(note.color, canvasColor, ClampInt(tailAlpha + 46, 48, 210));
                HBRUSH capBrush = CreateSolidBrush(capColor);
                HPEN capPen = CreatePen(PS_SOLID, gConfig.thinOutline ? 1 : 2, BlendColor(theme.outline, canvasColor, 150));
                oldBrush = SelectObject(mem, capBrush);
                oldPen = SelectObject(mem, capPen);
                RoundRect(mem, tailCap.left, tailCap.top, tailCap.right, tailCap.bottom, 10, 10);
                SelectObject(mem, oldPen);
                SelectObject(mem, oldBrush);
                DeleteObject(capPen);
                DeleteObject(capBrush);
            }
        }

        double pulseT = 1.0 - std::max(0.0, std::min(1.0, pressAge / kPulseMs));
        if (pulseT > 0.0) {
            for (int ring = 0; ring < 3; ++ring) {
                int grow = 3 + ring * 5 + static_cast<int>(std::round(10.0 * pulseT));
                RECT pulse{laneX - noteW / 2 - grow, centerY - noteH / 2 - grow, laneX + noteW / 2 + grow, centerY + noteH / 2 + grow};
                int alpha = static_cast<int>(std::round((theme.pulseAlpha - ring * 28) * pulseT));
                COLORREF pulseColor = BlendColor(note.color, canvasColor, alpha);
                HPEN pulsePen = CreatePen(PS_SOLID, 1, pulseColor);
                HGDIOBJ oldPen = SelectObject(mem, pulsePen);
                HGDIOBJ oldBrush = SelectObject(mem, GetStockObject(NULL_BRUSH));
                if (gConfig.circleNotes) {
                    Ellipse(mem, pulse.left, pulse.top, pulse.right, pulse.bottom);
                } else {
                    RoundRect(mem, pulse.left, pulse.top, pulse.right, pulse.bottom, gConfig.roundedBoxes ? 16 : 3, gConfig.roundedBoxes ? 16 : 3);
                }
                SelectObject(mem, oldBrush);
                SelectObject(mem, oldPen);
                DeleteObject(pulsePen);
            }
        }

        RECT shadow{laneX - noteW / 2 + 3, centerY - noteH / 2 + 3, laneX + noteW / 2 + 3, centerY + noteH / 2 + 3};
        HBRUSH shadowBrush = CreateSolidBrush(theme.shadow);
        if (gConfig.circleNotes) {
            HGDIOBJ oldShadowBrush = SelectObject(mem, shadowBrush);
            HGDIOBJ oldShadowPen = SelectObject(mem, GetStockObject(NULL_PEN));
            Ellipse(mem, shadow.left, shadow.top, shadow.right, shadow.bottom);
            SelectObject(mem, oldShadowPen);
            SelectObject(mem, oldShadowBrush);
        } else {
            FillRect(mem, &shadow, shadowBrush);
        }
        DeleteObject(shadowBrush);

        RECT body{laneX - noteW / 2, centerY - noteH / 2, laneX + noteW / 2, centerY + noteH / 2};
        HBRUSH brush = CreateSolidBrush(bodyColor);
        HPEN pen = CreatePen(PS_SOLID, gConfig.thinOutline ? 1 : 2, theme.outline);
        HGDIOBJ oldBrush = SelectObject(mem, brush);
        HGDIOBJ oldPen = SelectObject(mem, pen);
        if (gConfig.circleNotes) {
            Ellipse(mem, body.left, body.top, body.right, body.bottom);
        } else if (gConfig.roundedBoxes) {
            RoundRect(mem, body.left, body.top, body.right, body.bottom, 12, 12);
        } else {
            Rectangle(mem, body.left, body.top, body.right, body.bottom);
        }
        SelectObject(mem, oldPen);
        SelectObject(mem, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);

        HPEN shinePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        oldPen = SelectObject(mem, shinePen);
        if (gConfig.circleNotes) {
            MoveToEx(mem, body.left + noteW / 3, body.top + noteH / 4, nullptr);
            LineTo(mem, body.right - noteW / 3, body.top + noteH / 4);
        } else {
            MoveToEx(mem, body.left + 7, body.top + 6, nullptr);
            LineTo(mem, body.right - 7, body.top + 6);
        }
        SelectObject(mem, oldPen);
        DeleteObject(shinePen);

        DrawKeyLabel(mem, body, note.label, theme.text);
        frame.CompositeGdi(noteLayer, noteRegion, note.opacity);
    }

    return true;
}

bool PresentOverlay(HWND hwnd, const overlay::AlphaSurface& frame) {
    SIZE size{frame.width,frame.height};
    POINT origin{};
    BLENDFUNCTION blend{AC_SRC_OVER,0,static_cast<BYTE>(ClampInt(gConfig.opacityPercent,20,100)*255/100),AC_SRC_ALPHA};
    return UpdateLayeredWindow(hwnd,nullptr,nullptr,&size,frame.dc,&origin,0,&blend,ULW_ALPHA) != FALSE;
}

void PaintOverlay(HWND hwnd) {
    SyncOverlayDisplayWidth(hwnd);
    PAINTSTRUCT ps{};
    BeginPaint(hwnd, &ps);
    RECT client{}; GetClientRect(hwnd, &client);
    overlay::AlphaSurface frame(client.right, client.bottom);
    if (!RenderOverlay(frame, NowMs())) {
        SetStatus(L"오버레이를 그릴 메모리가 부족해요. 크기를 줄여 다시 적용해 주세요.");
    } else if (!PresentOverlay(hwnd, frame)) {
        SetStatus(L"오버레이 표시 실패 (Windows 오류 " + std::to_wstring(GetLastError()) + L"). 설정에서 다시 적용해 주세요.");
    }
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        RequestApplicationClose();
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_RENDER);
        gOverlayMoving = false;
        if (gOverlayWnd == hwnd) gOverlayWnd = nullptr;
        if (gAppRunning && !gShuttingDown) ShutdownApplication();
        return 0;
    case WM_CREATE:
        SetTimer(hwnd, TIMER_RENDER, 8, nullptr);
        return 0;
    case WM_APP_KEY_DOWN:
        BeginLaneHold(static_cast<int>(wParam));
        return 0;
    case WM_APP_KEY_UP:
        EndLaneHold(static_cast<int>(wParam));
        return 0;
    case WM_TIMER:
        if (wParam == TIMER_RENDER) {
            SyncOverlayDisplayWidth(hwnd);
            if (!gNotes.empty()) InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PaintOverlay(hwnd);
        return 0;
    case WM_NCHITTEST:
        return gMoveMode || (!gConfig.gameOverlay && !gConfig.clickThrough) ? HTCAPTION : HTTRANSPARENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_ENTERSIZEMOVE:
        gOverlayMoving = (gMoveMode || (!gConfig.gameOverlay && !gConfig.clickThrough)) && GetWindowRect(hwnd, &gMoveStart);
        return 0;
    case WM_EXITSIZEMOVE:
        FinishOverlayMove(hwnd);
        return 0;
    case WM_NCLBUTTONDBLCLK:
        if (wParam == HTCAPTION) return 0; // Dragging must not maximize the OBS surface.
        break;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCAPTION) { SetCursor(LoadCursorW(nullptr, IDC_SIZEALL)); return TRUE; }
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND TrackControl(HWND hwnd) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(gGuiFont), TRUE);
    if (gBuildingPage >= 0) gPageControls[static_cast<size_t>(gBuildingPage)].push_back(hwnd);
    return hwnd;
}

HWND MakeLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id = 0) {
    return TrackControl(CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gInstance, nullptr));
}

HWND MakeEdit(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
    return TrackControl(CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gInstance, nullptr));
}

HWND MakeButton(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h, DWORD style = BS_PUSHBUTTON) {
    return TrackControl(CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gInstance, nullptr));
}

void SetEditInt(HWND parent, int id, int value) {
    SetWindowTextW(GetDlgItem(parent, id), std::to_wstring(value).c_str());
}

std::wstring DetectModeForLanes(const std::wstring& lanesText) {
    std::vector<std::wstring> tokens;
    std::wstring error;
    if (!ValidateLaneText(lanesText, tokens, error)) return L"CUSTOM";
    const int index = PresetIndexForCount(tokens.size());
    return index >= 0 ? kPresets[index].mode : L"CUSTOM";
}

void UpdateDirtyState() {
    const wchar_t* state = gUnapplied ? L"● 편집 중 · 아직 적용하지 않았어요" :
        (gUnsaved ? L"● 적용됨 · 저장하면 다음 실행에도 유지돼요" : L"저장 상태와 같아요");
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_DIRTY), state);
    SetWindowTextW(gSettingsWnd, (gUnapplied || gUnsaved) ?
        L"Key Note Overlay C++ Settings *" : L"Key Note Overlay C++ Settings");
}

void MarkEdited() {
    if (gFillingControls) return;
    gUnapplied = true;
    UpdateDirtyState();
}

void ShowSettingsPage(int page) {
    gCurrentPage = ClampInt(page, 0, static_cast<int>(gPageControls.size()) - 1);
    TabCtrl_SetCurSel(GetDlgItem(gSettingsWnd, IDC_TABS), gCurrentPage);
    for (size_t i = 0; i < gPageControls.size(); ++i) {
        for (HWND control : gPageControls[i]) ShowWindow(control, static_cast<int>(i) == gCurrentPage ? SW_SHOW : SW_HIDE);
    }
    InvalidateRect(gSettingsWnd, nullptr, TRUE);
}

void FillKeyStyleFields() {
    if (gStyleVk >= gDraftKeyStyles.size()) gStyleVk = 0;
    const bool filling = gFillingControls;
    gFillingControls = true;
    for (int id : {IDC_KEY_COLOR, IDC_KEY_OPACITY, IDC_PICK_COLOR, IDC_RESET_KEY_STYLE})
        EnableWindow(GetDlgItem(gSettingsWnd, id), gStyleVk != 0);
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_STYLE_NAME), (gStyleVk ? TokenFromVk(gStyleVk) + L" 키" : L"키 배치를 먼저 지정해 주세요.").c_str());
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_KEY_COLOR), gStyleVk ? gDraftKeyStyles[gStyleVk].color.c_str() : L"");
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_KEY_OPACITY), gStyleVk ? gDraftKeyStyles[gStyleVk].opacity.c_str() : L"");
    InvalidateRect(GetDlgItem(gSettingsWnd, IDC_STYLE_PREVIEW), nullptr, TRUE);
    gFillingControls = filling;
}

void RefreshKeyStyleList() {
    HWND list = GetDlgItem(gSettingsWnd, IDC_STYLE_KEYS);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    std::vector<std::wstring> tokens;
    std::wstring error;
    int selection = 0;
    if (ValidateLaneText(WindowText(GetDlgItem(gSettingsWnd, IDC_LANES)), tokens, error)) {
        for (size_t i = 0; i < tokens.size(); ++i) {
            UINT vk = 0; std::wstring label;
            TokenToVk(tokens[i], vk, label);
            const std::wstring text = std::to_wstring(i + 1) + L"    " + tokens[i];
            const LRESULT row = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
            SendMessageW(list, LB_SETITEMDATA, row, vk);
            if (vk == gStyleVk) selection = static_cast<int>(row);
        }
    }
    const int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
    SendMessageW(list, LB_SETCURSEL, count <= 0 ? -1 : selection, 0);
    gStyleVk = count <= 0 ? 0 : static_cast<UINT>(SendMessageW(list, LB_GETITEMDATA, selection, 0));
    FillKeyStyleFields();
}

void DrawStylePreview(HDC dc, RECT rect) {
    COLORREF color = CLR_INVALID;
    int opacity = 100;
    if (gStyleVk) {
        ParseColor(gDraftKeyStyles[gStyleVk].color, color);
        ParseInteger(gDraftKeyStyles[gStyleVk].opacity, 0, 100, opacity);
    }
    const int lane = static_cast<int>(SendDlgItemMessageW(gSettingsWnd, IDC_STYLE_KEYS, LB_GETCURSEL, 0, 0));
    if (color == CLR_INVALID) color = kPalette[static_cast<size_t>(std::max(0, lane)) % kPalette.size()];
    for (int y = rect.top; y < rect.bottom; y += 12) for (int x = rect.left; x < rect.right; x += 12) {
        COLORREF bg = ((x-rect.left)/12 + (y-rect.top)/12) % 2 ? RGB(218,218,218) : RGB(255,255,255);
        RECT cell{x,y,std::min(x+12,static_cast<int>(rect.right)),std::min(y+12,static_cast<int>(rect.bottom))};
        HBRUSH brush = CreateSolidBrush(bg); FillRect(dc, &cell, brush); DeleteObject(brush);
        RECT body{rect.left+20,rect.top+12,rect.right-20,rect.bottom-12}, covered{};
        if (IntersectRect(&covered, &cell, &body)) {
            brush = CreateSolidBrush(BlendColor(color, bg, opacity * 255 / 100));
            FillRect(dc, &covered, brush); DeleteObject(brush);
        }
    }
}

void PickKeyColor() {
    if (!gStyleVk) return;
    static COLORREF custom[16]{};
    COLORREF color = CLR_INVALID;
    ParseColor(gDraftKeyStyles[gStyleVk].color, color);
    const int lane = static_cast<int>(SendDlgItemMessageW(gSettingsWnd, IDC_STYLE_KEYS, LB_GETCURSEL, 0, 0));
    CHOOSECOLORW choice{};
    choice.lStructSize = sizeof(choice); choice.hwndOwner = gSettingsWnd;
    choice.rgbResult = color == CLR_INVALID ? kPalette[static_cast<size_t>(std::max(0,lane)) % kPalette.size()] : color;
    choice.lpCustColors = custom; choice.Flags = CC_FULLOPEN | CC_RGBINIT;
    gPickingColor = true;
    const BOOL accepted = ChooseColorW(&choice);
    gPickingColor = false;
    if (accepted) SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_KEY_COLOR), ColorHex(choice.rgbResult).c_str());
}

void RefreshLaneEditor() {
    HWND list = GetDlgItem(gSettingsWnd, IDC_LANE_LIST);
    int selected = static_cast<int>(SendMessageW(list, LB_GETCURSEL, 0, 0));
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    std::vector<std::wstring> tokens;
    std::wstring error;
    const bool valid = ValidateLaneText(WindowText(GetDlgItem(gSettingsWnd, IDC_LANES)), tokens, error);
    if (valid) {
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::wstring label = std::to_wstring(i + 1) + L"    " + tokens[i];
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
        SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(ClampInt(selected, 0, static_cast<int>(tokens.size()) - 1)), 0);
    }
    const std::wstring summary = valid ? std::to_wstring(tokens.size()) + L"개 키 · 목록 위에서 아래가 화면의 왼쪽에서 오른쪽 순서예요." : error;
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_LANE_SUMMARY), summary.c_str());
    HWND combo = GetDlgItem(gSettingsWnd, IDC_PRESET);
    int match = 0;
    const std::wstring mode = valid ? DetectModeForLanes(JoinLaneTokens(tokens)) : L"CUSTOM";
    for (int i = 1; i <= static_cast<int>(kPresets.size()); ++i) {
        int index = static_cast<int>(SendMessageW(combo, CB_GETITEMDATA, i, 0));
        if (index >= 0 && index < static_cast<int>(kPresets.size()) && mode == kPresets[static_cast<size_t>(index)].mode) match = i;
    }
    SendMessageW(combo, CB_SETCURSEL, match, 0);
    RefreshKeyStyleList();
}

void FillAppearance(const Config& config) {
    SetEditInt(gSettingsWnd, IDC_OPACITY, config.opacityPercent);
    SetEditInt(gSettingsWnd, IDC_LANE_SPACING, config.laneSpacingPercent);
    SendDlgItemMessageW(gSettingsWnd, IDC_SPACING_SLIDER, TBM_SETPOS, TRUE, config.laneSpacingPercent);
    SetEditInt(gSettingsWnd, IDC_LONG_ALPHA, config.longNoteAlpha);
    const std::pair<int, bool> checks[] = {{IDC_CIRCLE_NOTES, config.circleNotes}, {IDC_LANE_BG, config.laneBackground},
        {IDC_ROUNDED_BOXES, config.roundedBoxes}, {IDC_THIN_OUTLINE, config.thinOutline}, {IDC_KEY_TEXT_BOTTOM, config.keyTextBottom}};
    for (const auto& check : checks) SendMessageW(GetDlgItem(gSettingsWnd, check.first), BM_SETCHECK, check.second ? BST_CHECKED : BST_UNCHECKED, 0);
    gDraftVisualPreset = config.visualPreset;
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_VISUAL_NAME), (L"테마: " + gDraftVisualPreset).c_str());
}

void FillSettingsControls(HWND hwnd) {
    gFillingControls = true;
    gDraftPresetLanes = gConfig.presetLanes;
    for (UINT vk = 0; vk < gDraftKeyStyles.size(); ++vk)
        gDraftKeyStyles[vk] = {ColorHex(gConfig.keyStyles[vk].color), std::to_wstring(gConfig.keyStyles[vk].opacity)};
    SetWindowTextW(GetDlgItem(hwnd, IDC_LANES), gConfig.lanesText.c_str());
    SetEditInt(hwnd, IDC_X, gConfig.x);
    SetEditInt(hwnd, IDC_Y, gConfig.y);
    SetEditInt(hwnd, IDC_WIDTH, gConfig.width);
    SetEditInt(hwnd, IDC_HEIGHT, gConfig.height);
    SetEditInt(hwnd, IDC_DURATION, gConfig.durationMs);
    SetEditInt(hwnd, IDC_TRAVEL, gConfig.travelPx);
    SetEditInt(hwnd, IDC_NOTE_WIDTH, gConfig.noteWidth);
    SetEditInt(hwnd, IDC_NOTE_HEIGHT, gConfig.noteHeight);
    FillAppearance(gConfig);
    SendMessageW(GetDlgItem(hwnd, IDC_DEBUG_BG), BM_SETCHECK, gConfig.debugBackground ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_CLICK_THROUGH), BM_SETCHECK, gConfig.clickThrough ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_TOPMOST), BM_SETCHECK, gConfig.alwaysOnTop ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_DIFFICULTY_ENABLED), BM_SETCHECK, gConfig.difficultyEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_DIFFICULTY_HUD), BM_SETCHECK, gConfig.difficultyHud ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_KEY_VIEWER_FOLDED), BM_SETCHECK, gConfig.keyViewerFolded ? BST_CHECKED : BST_UNCHECKED, 0);
    SendDlgItemMessageW(hwnd, IDC_ANALYSIS_KEYS, CB_SETCURSEL,
        gConfig.analysisKeyMode == 0 ? 0 : gConfig.analysisKeyMode == -1 ? 1 : gConfig.analysisKeyMode - 2, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_REPORT_AUTO), BM_SETCHECK, gConfig.reportAutoSave ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_GAME_OVERLAY), BM_SETCHECK, gConfig.gameOverlay ? BST_CHECKED : BST_UNCHECKED, 0);
    RefreshLaneEditor();
    gFillingControls = false;
    UpdateDirtyState();
}

void SettingsError(int id, int page, const std::wstring& error) {
    ShowSettingsPage(page);
    SetStatus(error);
    SetFocus(GetDlgItem(gSettingsWnd, id));
    SendMessageW(GetDlgItem(gSettingsWnd, id), EM_SETSEL, 0, -1);
}

bool BuildSettingsCandidate(Config& candidate, bool reportErrors) {
    // Validate the entire draft before touching the active configuration or key map.
    candidate = gConfig;
    std::vector<std::wstring> tokens;
    std::wstring error;
    if (!ValidateLaneText(WindowText(GetDlgItem(gSettingsWnd, IDC_LANES)), tokens, error)) {
        if (reportErrors) SettingsError(IDC_LANES, 0, error);
        return false;
    }
    candidate.lanesText = JoinLaneTokens(tokens);
    candidate.mode = DetectModeForLanes(candidate.lanesText);
    candidate.presetLanes = gDraftPresetLanes;
    RememberPreset(candidate.presetLanes, candidate.lanesText);
    for (UINT vk = 0; vk < gDraftKeyStyles.size(); ++vk) {
        const auto& draft = gDraftKeyStyles[vk];
        auto& style = candidate.keyStyles[vk];
        const bool colorOk = ParseColor(draft.color, style.color);
        if (!colorOk || !ParseInteger(draft.opacity, 0, 100, style.opacity)) {
            if (reportErrors) {
                // Keep even an inactive key's invalid draft instead of losing it
                // when the user switches presets or moves to another key.
                gStyleVk = vk;
                HWND list = GetDlgItem(gSettingsWnd, IDC_STYLE_KEYS);
                int selected = -1;
                const int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
                for (int i=0; i<count; ++i) if (static_cast<UINT>(SendMessageW(list, LB_GETITEMDATA, i, 0)) == vk) selected = i;
                SendMessageW(list, LB_SETCURSEL, selected, 0);
                FillKeyStyleFields();
                SettingsError(colorOk ? IDC_KEY_OPACITY : IDC_KEY_COLOR, 4, TokenFromVk(vk) +
                    (colorOk ? L": 불투명도는 0~100 정수로 입력해 주세요." : L": 색은 #RRGGBB 형식 또는 빈칸(기본색)으로 입력해 주세요."));
            }
            return false;
        }
    }
    struct Field { int id; int page; const wchar_t* name; int min; int max; int* value; };
    const Field fields[] = {
        {IDC_X, 1, L"X 위치", -32000, 32000, &candidate.x}, {IDC_Y, 1, L"Y 위치", -32000, 32000, &candidate.y},
        {IDC_WIDTH, 1, L"가로 크기", kMinOverlayWidth, 10000, &candidate.width}, {IDC_HEIGHT, 1, L"세로 크기", kMinOverlayHeight, 4000, &candidate.height},
        {IDC_DURATION, 2, L"사라지는 시간", 120, 5000, &candidate.durationMs}, {IDC_TRAVEL, 2, L"이동 거리", 40, 4000, &candidate.travelPx},
        {IDC_NOTE_WIDTH, 2, L"노트 너비", 24, 220, &candidate.noteWidth}, {IDC_NOTE_HEIGHT, 2, L"노트 높이", 18, 140, &candidate.noteHeight},
        {IDC_LANE_SPACING, 2, L"레인 간격", 50, 160, &candidate.laneSpacingPercent},
        {IDC_OPACITY, 2, L"전체 불투명도", 20, 100, &candidate.opacityPercent},
        {IDC_LONG_ALPHA, 2, L"롱노트 진하기", 32, 180, &candidate.longNoteAlpha}};
    for (const auto& field : fields) {
        if (!ParseInteger(WindowText(GetDlgItem(gSettingsWnd, field.id)), field.min, field.max, *field.value)) {
            if (reportErrors) SettingsError(field.id, field.page, std::wstring(field.name) + L": " + std::to_wstring(field.min) + L"~" +
                std::to_wstring(field.max) + L" 범위의 정수를 입력해 주세요.");
            return false;
        }
    }
    const std::pair<int, bool*> checks[] = {{IDC_DEBUG_BG, &candidate.debugBackground}, {IDC_CLICK_THROUGH, &candidate.clickThrough},
        {IDC_TOPMOST, &candidate.alwaysOnTop}, {IDC_CIRCLE_NOTES, &candidate.circleNotes}, {IDC_LANE_BG, &candidate.laneBackground},
        {IDC_ROUNDED_BOXES, &candidate.roundedBoxes}, {IDC_THIN_OUTLINE, &candidate.thinOutline}, {IDC_KEY_TEXT_BOTTOM, &candidate.keyTextBottom},
        {IDC_DIFFICULTY_ENABLED, &candidate.difficultyEnabled}, {IDC_DIFFICULTY_HUD, &candidate.difficultyHud},
        {IDC_KEY_VIEWER_FOLDED, &candidate.keyViewerFolded},
        {IDC_REPORT_AUTO, &candidate.reportAutoSave}, {IDC_GAME_OVERLAY, &candidate.gameOverlay}};
    for (const auto& check : checks) *check.second = SendMessageW(GetDlgItem(gSettingsWnd, check.first), BM_GETCHECK, 0, 0) == BST_CHECKED;
    candidate.visualPreset = gDraftVisualPreset;
    const int keyChoice = static_cast<int>(SendDlgItemMessageW(gSettingsWnd, IDC_ANALYSIS_KEYS, CB_GETCURSEL, 0, 0));
    candidate.analysisKeyMode = keyChoice == 1 ? -1 : keyChoice >= 2 && keyChoice <= 8 ? keyChoice + 2 : 0;
    return true;
}

bool ReadSettingsControls() {
    Config candidate;
    if (!BuildSettingsCandidate(candidate, true)) return false;
    Config withoutFoldChange = candidate; withoutFoldChange.keyViewerFolded = gConfig.keyViewerFolded;
    const bool presentationOnly = SameConfig(withoutFoldChange, gConfig);
    const bool resetDifficulty = candidate.lanesText != gConfig.lanesText || candidate.difficultyEnabled != gConfig.difficultyEnabled ||
        candidate.reportAutoSave != gConfig.reportAutoSave || candidate.gameOverlay != gConfig.gameOverlay ||
        candidate.analysisKeyMode != gConfig.analysisKeyMode;
    if (resetDifficulty) FinishSessionReport("분석 / 저장 설정 변경", NowMs());
    gConfig = candidate;
    RebuildLaneConfig();
    if (!presentationOnly) ResetInputRuntimeState();
    if (resetDifficulty) ResetDifficulty(static_cast<int>(gConfig.lanes.size()));
    RefreshDifficultyDisplay();
    gUnapplied = false;
    gUnsaved = !SameConfig(gConfig, gSavedConfig);
    FillSettingsControls(gSettingsWnd);
    if (!gUiSelfTest) ApplyOverlayWindowStyle();
    SetStatus(L"오버레이에 적용했어요. 저장하면 다음 실행에도 유지됩니다.");
    return true;
}

bool SaveSettings() {
    if (!ReadSettingsControls()) return false;
    if (!SaveConfig()) {
        gUnsaved = true;
        UpdateDirtyState();
        SetStatus(L"저장 실패 (Windows 오류 " + std::to_wstring(gConfigSaveError) + L"). 폴더 권한과 파일 사용 여부를 확인해 주세요.\n" + gConfigPath);
        return false;
    }
    gSavedConfig = gConfig;
    gUnsaved = false;
    UpdateDirtyState();
    SetStatus(gIniSaveFailed ? L"설정 저장 완료. 호환 INI 갱신은 실패했지만 다음 실행에 JSON으로 복원됩니다.\n" + gConfigPath :
        L"저장 완료 · 다음 실행에도 유지됩니다.\n" + gConfigPath);
    return true;
}

bool SaveOverlayPosition(int x, int y) {
    gConfig.x = x;
    gConfig.y = y;
    // Save only coordinates over the last committed snapshot. Other applied or
    // invalid draft edits must not be silently committed by a mouse gesture.
    const Config active = gConfig;
    gConfig = gSavedConfig;
    gConfig.x = x;
    gConfig.y = y;
    const bool saved = SaveConfig();
    if (saved) { gSavedConfig.x = x; gSavedConfig.y = y; }
    gConfig = active;
    RebuildLaneConfig(); // SaveConfig rebuilds the global map for its temporary snapshot.
    gFillingControls = true;
    SetEditInt(gSettingsWnd, IDC_X, x);
    SetEditInt(gSettingsWnd, IDC_Y, y);
    gFillingControls = false;
    Config draft;
    gUnapplied = !BuildSettingsCandidate(draft, false) || !SameConfig(draft, gConfig);
    gUnsaved = !SameConfig(gConfig, gSavedConfig);
    UpdateDirtyState();
    if (!saved) SetStatus(L"위치 저장 실패 (Windows 오류 " + std::to_wstring(gConfigSaveError) +
        L"). 이전 저장 위치는 유지됩니다. [위치 저장]으로 다시 시도해 주세요.");
    else SetStatus(gIniSaveFailed ? L"위치 저장 완료. 호환 INI 갱신은 실패했지만 JSON으로 복원됩니다." :
        L"위치 저장 완료 · 다음 실행에도 복원됩니다. 다른 편집 내용은 그대로 유지했어요.");
    return saved;
}

void FinishOverlayMove(HWND hwnd) {
    if (!gOverlayMoving) return;
    gOverlayMoving = false;
    RECT position{};
    // Native move cancellation returns to the starting rectangle. Programmatic
    // movement never enters this path; neither should write a configuration.
    if (GetWindowRect(hwnd, &position) && (position.left != gMoveStart.left || position.top != gMoveStart.top))
        SaveOverlayPosition(position.left, position.top);
}

bool SavePositionDraft() {
    int x = 0, y = 0;
    if (!ParseInteger(WindowText(GetDlgItem(gSettingsWnd, IDC_X)), -32000, 32000, x)) {
        SettingsError(IDC_X, 1, L"X 위치: -32000~32000 범위의 정수를 입력해 주세요."); return false;
    }
    if (!ParseInteger(WindowText(GetDlgItem(gSettingsWnd, IDC_Y)), -32000, 32000, y)) {
        SettingsError(IDC_Y, 1, L"Y 위치: -32000~32000 범위의 정수를 입력해 주세요."); return false;
    }
    if (gOverlayWnd && !SetWindowPos(gOverlayWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        SetStatus(L"창 위치를 바꾸지 못했어요. 위치를 저장하지 않았습니다."); return false;
    }
    return SaveOverlayPosition(x, y);
}

void SetLaneTokens(const std::vector<std::wstring>& tokens, int selected) {
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_LANES), JoinLaneTokens(tokens).c_str());
    SendMessageW(GetDlgItem(gSettingsWnd, IDC_LANE_LIST), LB_SETCURSEL, selected, 0);
}

void EditSelectedLane(int command) {
    std::vector<std::wstring> tokens;
    std::wstring error;
    if (!ValidateLaneText(WindowText(GetDlgItem(gSettingsWnd, IDC_LANES)), tokens, error)) { SetStatus(error); return; }
    int selected = static_cast<int>(SendMessageW(GetDlgItem(gSettingsWnd, IDC_LANE_LIST), LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(tokens.size())) { SetStatus(L"목록에서 바꿀 키를 선택해 주세요."); return; }
    if (command == IDC_REMOVE) {
        if (tokens.size() == 1) { SetStatus(L"키는 최소 1개가 필요해요."); return; }
        tokens.erase(tokens.begin() + selected);
        selected = std::min(selected, static_cast<int>(tokens.size()) - 1);
    } else {
        int next = selected + (command == IDC_MOVE_LEFT ? -1 : 1);
        if (next < 0 || next >= static_cast<int>(tokens.size())) return;
        std::swap(tokens[static_cast<size_t>(selected)], tokens[static_cast<size_t>(next)]);
        selected = next;
    }
    SetLaneTokens(tokens, selected);
    SetStatus(L"키 배치를 편집했어요. 적용 또는 저장을 눌러 주세요.");
}

void SetCaptureControls(bool enabled) {
    for (const auto& page : gPageControls) for (HWND control : page) EnableWindow(control, enabled);
    for (int id : {IDC_APPLY, IDC_SAVE, IDC_TEST, IDC_REVERT, IDC_TABS}) EnableWindow(GetDlgItem(gSettingsWnd, id), enabled);
    EnableWindow(GetDlgItem(gSettingsWnd, IDC_RECORD), TRUE);
}

void EndKeyCapture(bool cancel) {
    if (gCaptureMode == CaptureMode::None) return;
    if (cancel) {
        SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_LANES), gCaptureOriginal.c_str());
        gUnapplied = gCaptureWasUnapplied;
    }
    gCaptureMode = CaptureMode::None;
    gCapturedKeys.clear();
    SetCaptureControls(true);
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_RECORD), L"순서대로 등록");
    ResetInputRuntimeState();
    RefreshDifficultyDisplay();
    UpdateDirtyState();
    SetStatus(cancel ? L"키 등록을 취소하고 이전 배치로 돌아왔어요." : L"키 등록 완료. 적용 또는 저장을 눌러 주세요.");
}

void BeginKeyCapture(CaptureMode mode) {
    if (gCaptureMode != CaptureMode::None) { EndKeyCapture(false); return; }
    gCaptureIndex = static_cast<int>(SendMessageW(GetDlgItem(gSettingsWnd, IDC_LANE_LIST), LB_GETCURSEL, 0, 0));
    gCaptureOriginal = WindowText(GetDlgItem(gSettingsWnd, IDC_LANES));
    gCapturedKeys.clear();
    std::wstring error;
    if (mode == CaptureMode::Replace && (!ValidateLaneText(gCaptureOriginal, gCapturedKeys, error) ||
        gCaptureIndex < 0 || gCaptureIndex >= static_cast<int>(gCapturedKeys.size()))) {
        SetStatus(L"유효한 키 배치에서 변경할 키를 선택해 주세요.");
        return;
    }
    gCaptureWasUnapplied = gUnapplied;
    ResetDifficulty(static_cast<int>(gConfig.lanes.size()));
    gCaptureMode = mode;
    RefreshDifficultyDisplay();
    SetCaptureControls(false);
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_RECORD), L"등록 완료");
    SetFocus(GetDlgItem(gSettingsWnd, IDC_RECORD));
    ResetInputRuntimeState();
    SetStatus(mode == CaptureMode::Sequence ? L"왼쪽부터 키를 하나씩 누르세요. [등록 완료]로 마침 · Esc로 취소" : L"새 키를 누르세요. Esc로 취소할 수 있어요.");
}

void CaptureKey(UINT vk) {
    if (gCaptureMode == CaptureMode::None) return;
    if (vk == VK_ESCAPE) { EndKeyCapture(true); return; }
    auto token = TokenFromVk(vk);
    if (token.empty()) { SetStatus(L"이 키는 등록할 수 없어요. 다른 키를 눌러 주세요."); return; }
    auto candidate = gCapturedKeys;
    if (gCaptureMode == CaptureMode::Replace) candidate[static_cast<size_t>(gCaptureIndex)] = token;
    else candidate.push_back(token);
    std::vector<std::wstring> validated;
    std::wstring error;
    if (!ValidateLaneText(JoinLaneTokens(candidate), validated, error)) { SetStatus(error); return; }
    gCapturedKeys = validated;
    SetLaneTokens(validated, gCaptureMode == CaptureMode::Replace ? gCaptureIndex : static_cast<int>(validated.size()) - 1);
    if (gCaptureMode == CaptureMode::Replace) EndKeyCapture(false);
    else SetStatus(std::to_wstring(validated.size()) + L"개 등록됨 · 다음 키를 누르거나 [등록 완료]를 누르세요. Esc로 취소");
}

UINT MessageVirtualKey(const MSG& msg) {
    UINT vk = static_cast<UINT>(msg.wParam);
    const bool extended = (msg.lParam & (1LL << 24)) != 0;
    if (vk == VK_SHIFT) vk = MapVirtualKeyW((msg.lParam >> 16) & 0xff, MAPVK_VSC_TO_VK_EX);
    if (vk == VK_CONTROL) vk = extended ? VK_RCONTROL : VK_LCONTROL;
    if (vk == VK_MENU) vk = extended ? VK_RMENU : VK_LMENU;
    return vk;
}

bool HandleCaptureMessage(const MSG& msg) {
    if (gCaptureMode == CaptureMode::None || GetForegroundWindow() != gSettingsWnd) return false;
    if (msg.message != WM_KEYDOWN && msg.message != WM_SYSKEYDOWN && msg.message != WM_KEYUP && msg.message != WM_SYSKEYUP) return false;
    if ((msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) && !(msg.lParam & (1LL << 30))) {
        CaptureKey(MessageVirtualKey(msg));
    }
    return true;
}

bool HandleDifficultyTestMessage(const MSG& msg) {
    if (!gConfig.difficultyEnabled || gCurrentPage != 3 || GetForegroundWindow() != gSettingsWnd) return false;
    if (msg.message != WM_KEYDOWN && msg.message != WM_KEYUP && msg.message != WM_SYSKEYDOWN && msg.message != WM_SYSKEYUP) return false;
    const UINT vk = MessageVirtualKey(msg);
    // The hook already recorded this press. In the analysis tab, playing a mapped
    // Space/Enter/arrow key must not activate focused settings buttons or checkboxes.
    return vk < gVkToLaneIndex.size() && gVkToLaneIndex[vk] >= 0;
}

void SpawnTestNotes() {
    for (int i = 0; i < static_cast<int>(gConfig.lanes.size()); ++i) SpawnTapNote(i);
}

void CenterOverlayDraft() {
    int width = 0, height = 0;
    if (!ParseInteger(WindowText(GetDlgItem(gSettingsWnd, IDC_WIDTH)), kMinOverlayWidth, 10000, width) ||
        !ParseInteger(WindowText(GetDlgItem(gSettingsWnd, IDC_HEIGHT)), kMinOverlayHeight, 4000, height)) {
        SetStatus(L"먼저 유효한 가로·세로 크기를 입력해 주세요."); return;
    }
    MONITORINFO info{sizeof(info), {}, {}, 0};
    if (!GetMonitorInfoW(MonitorFromWindow(gSettingsWnd, MONITOR_DEFAULTTONEAREST), &info)) return;
    SetEditInt(gSettingsWnd, IDC_X, info.rcWork.left + (info.rcWork.right - info.rcWork.left - width) / 2);
    SetEditInt(gSettingsWnd, IDC_Y, info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - height) / 2);
    SetStatus(L"이 설정 창이 있는 모니터의 중앙 위치를 입력했어요. 적용을 눌러 주세요.");
}

void ResizeOverlayDraft(double factor) {
    Config draft;
    if (!BuildSettingsCandidate(draft, true)) return;
    auto scaled = [factor](int value, int minimum, int maximum) {
        return ClampInt(static_cast<int>(std::round(value * factor)), minimum, maximum);
    };
    const int noteWidth = scaled(draft.noteWidth, 24, 220);
    const int noteHeight = scaled(draft.noteHeight, 18, 140);
    const int effectiveHeight = draft.circleNotes ? std::min(noteWidth, std::max(noteHeight, 42)) : noteHeight;
    // HUD typography stays independent of note scaling. Keep a complete note
    // below it, including the optional selected-chart row, at the small limit.
    int reserved = draft.difficultyEnabled && draft.difficultyHud ? 76 : 0;
#if KEYNOTE_OSU_MEMORY_LINK
    if (gOsuView.enabled) reserved += 28;
#endif
    int height = scaled(draft.height, std::max(kMinOverlayHeight, reserved + effectiveHeight + 16), 4000);
    if (factor < 1.0) height = std::min(height, draft.height);
    SetEditInt(gSettingsWnd, IDC_WIDTH, scaled(draft.width, kMinOverlayWidth, 10000));
    SetEditInt(gSettingsWnd, IDC_HEIGHT, height);
    SetEditInt(gSettingsWnd, IDC_NOTE_WIDTH, noteWidth);
    SetEditInt(gSettingsWnd, IDC_NOTE_HEIGHT, noteHeight);
    SetEditInt(gSettingsWnd, IDC_TRAVEL, scaled(draft.travelPx, 40, 4000));
    SetStatus(L"창·노트·이동 거리를 함께 조절했어요. 레벨 글자는 유지됩니다. [적용 + 테스트]로 확인하세요.");
}

HFONT CreateSettingsFont() {
    HFONT font = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
    return font ? font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

void ShowSettingsWindow() {
    if (gShuttingDown || !IsWindow(gSettingsWnd)) return;
    SetSettingsForegroundBoost(true);
    ShowWindow(gSettingsWnd, IsIconic(gSettingsWnd) ? SW_RESTORE : SW_SHOW);
    SetForegroundWindow(gSettingsWnd);
    // Windows can deny foreground activation. Do not leave a passive settings
    // window permanently topmost when that happens.
    if (!IsSettingsInteraction(GetForegroundWindow())) SetSettingsForegroundBoost(false);
}

void ShutdownApplication() {
    if (gShuttingDown) return;
    gShuttingDown = true;
    gAppRunning = false;
    gReviveLive.Stop();
#if KEYNOTE_OSU_MEMORY_LINK
    gOsuLink.Stop();gOsuView={};
#endif
    // Delete the shell icon while its callback HWND still exists. All normal
    // exits, partial startup failures and message-loop exits share this path.
    RemoveTrayIcon();
    if (gKeyboardHook) {
        UnhookWindowsHookEx(gKeyboardHook);
        gKeyboardHook = nullptr;
    }
    FinishSessionReport("앱 종료 · 진행 중 기록", NowMs());
    PollReportWrites(true);
    gCaptureMode = CaptureMode::None;
    if (IsWindow(gOverlayWnd)) DestroyWindow(gOverlayWnd);
    if (IsWindow(gSettingsWnd)) DestroyWindow(gSettingsWnd);
    gOverlayWnd = gSettingsWnd = nullptr;
    PostQuitMessage(0);
}

void RequestApplicationClose() {
    if (gShuttingDown || gCloseInProgress) return;
    if (IsWindow(gSettingsWnd)) SendMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
    else ShutdownApplication();
}

void CleanupApplication() {
    ShutdownApplication();
    for (HFONT* font : {&gNoteFont, &gKeyFont, &gGuiFont}) {
        if (*font) DeleteObject(*font);
        *font = nullptr;
    }
    DestroyHudFonts();
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (gTaskbarCreatedMessage && msg == gTaskbarCreatedMessage) {
        // Explorer lost all notification icons; register this live app again.
        gTrayAdded = gTrayVersion4 = false;
        if (gAppRunning && !gShuttingDown) AddTrayIcon();
        return 0;
    }
    switch (msg) {
    case WM_APP_TRAY: {
        if (gShuttingDown || gCloseInProgress) return 0;
        const UINT event = TrayEvent(wParam, lParam);
        if (event == NIN_SELECT || event == NIN_KEYSELECT || event == WM_LBUTTONUP || event == WM_LBUTTONDBLCLK)
            ShowSettingsWindow();
        else if (event == WM_CONTEXTMENU || event == WM_RBUTTONUP) ShowTrayMenu(hwnd);
        return 0;
    }
    case WM_CREATE: {
        gSettingsWnd = hwnd;
        for (auto& page : gPageControls) page.clear();
        gBuildingPage = -1;
        gFillingControls = true;
        MakeLabel(hwnd, L"Key Note Overlay  ·  설정", 18, 14, 680, 26);
        HWND tabs = TrackControl(CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
            18, 50, 684, 430, hwnd, reinterpret_cast<HMENU>(IDC_TABS), gInstance, nullptr));
        const wchar_t* titles[] = {L"키 배치", L"위치 · OBS", L"모양 · 움직임", L"난이도", L"키 색상"
#if KEYNOTE_OSU_MEMORY_LINK
            , L"osu! 연동"
#endif
        };
        for (int i = 0; i < static_cast<int>(gPageControls.size()); ++i) {
            TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = const_cast<wchar_t*>(titles[i]);
            TabCtrl_InsertItem(tabs, i, &item);
        }
        gBuildingPage = 0;
        MakeLabel(hwnd, L"키 프리셋", 38, 96, 105, 24);
        HWND combo = TrackControl(CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            150, 92, 190, 250, hwnd, reinterpret_cast<HMENU>(IDC_PRESET), gInstance, nullptr));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"사용자 지정"));
        SendMessageW(combo, CB_SETITEMDATA, 0, -1);
        for (int index : {3, 0, 4, 5, 1, 6, 2}) {
            LRESULT item = SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kPresets[static_cast<size_t>(index)].mode));
            SendMessageW(combo, CB_SETITEMDATA, item, index);
        }
        MakeLabel(hwnd, L"왼쪽 → 오른쪽 키 순서", 38, 136, 260, 24);
        TrackControl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            38, 164, 260, 214, hwnd, reinterpret_cast<HMENU>(IDC_LANE_LIST), gInstance, nullptr));
        MakeButton(hwnd, IDC_RECORD, L"순서대로 등록", 320, 164, 160, 32);
        MakeButton(hwnd, IDC_REPLACE, L"선택 키 변경", 492, 164, 170, 32);
        MakeButton(hwnd, IDC_MOVE_LEFT, L"← 앞으로", 320, 208, 105, 30);
        MakeButton(hwnd, IDC_MOVE_RIGHT, L"뒤로 →", 435, 208, 105, 30);
        MakeButton(hwnd, IDC_REMOVE, L"삭제", 550, 208, 112, 30);
        MakeLabel(hwnd, L"순서대로 등록: 원하는 키를 왼쪽부터 누르세요.\n선택 키 변경: 목록에서 키를 고른 뒤 누르세요.\n키 수별 배치는 [적용 + 저장]으로 기억해요.\nEsc로 등록 취소 · 다른 창으로 이동하면 취소\n숫자패드와 좌우 Shift / Ctrl / Alt도 구분해요.", 320, 256, 344, 116);
        MakeLabel(hwnd, L"직접 입력 (쉼표 구분, 예: D,F,SPACE,J,K)", 38, 386, 624, 22);
        MakeEdit(hwnd, IDC_LANES, L"", 38, 410, 624, 26);
        MakeLabel(hwnd, L"", 38, 444, 624, 28, IDC_LANE_SUMMARY);

        gBuildingPage = 1;
        MakeButton(hwnd, IDC_GAME_OVERLAY, L"게임용 모드 (osu! · Qwilight 입력 / 항상 표시)", 38, 92, 624, 26, BS_AUTOCHECKBOX);
        MakeLabel(hwnd, L"X", 38, 138, 24, 24); MakeEdit(hwnd, IDC_X, L"", 70, 134, 110, 26);
        MakeLabel(hwnd, L"Y", 210, 138, 24, 24); MakeEdit(hwnd, IDC_Y, L"", 242, 134, 110, 26);
        MakeButton(hwnd, IDC_CENTER, L"이 모니터 중앙으로", 390, 132, 238, 30);
        MakeButton(hwnd, IDC_MOVE_MODE, L"마우스로 위치 이동", 38, 174, 200, 30);
        MakeButton(hwnd, IDC_SAVE_POSITION, L"위치 저장", 248, 174, 160, 30);
        MakeLabel(hwnd, L"오버레이 크기 · 최소 650 × 300px", 38, 218, 370, 24);
        MakeButton(hwnd, IDC_SIZE_SMALLER, L"10% 축소", 430, 214, 110, 30);
        MakeButton(hwnd, IDC_SIZE_LARGER, L"10% 확대", 552, 214, 110, 30);
        MakeLabel(hwnd, L"창·노트 함께 조절\n레벨 글자 크기는 유지", 430, 252, 232, 40);
        MakeLabel(hwnd, L"가로", 38, 258, 50, 24); MakeEdit(hwnd, IDC_WIDTH, L"", 96, 254, 110, 26);
        MakeLabel(hwnd, L"세로", 242, 258, 50, 24); MakeEdit(hwnd, IDC_HEIGHT, L"", 300, 254, 110, 26);
        MakeButton(hwnd, IDC_CLICK_THROUGH, L"마우스 클릭을 뒤쪽 창으로 통과", 38, 300, 420, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_TOPMOST, L"항상 위에 표시", 38, 334, 280, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_KEY_VIEWER_FOLDED, L"키뷰어 접기 · 지표 글자 150%", 348, 334, 314, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_DEBUG_BG, L"위치 확인용 배경 표시", 38, 368, 350, 26, BS_AUTOCHECKBOX);
        MakeLabel(hwnd, L"클릭 통과·항상 위 · 디스코드/OBS로 전환해도 표시 유지 (게임 밖 입력 제외)\nosu!는 창 모드/테두리 없는 전체화면, Qwilight는 창/전체 화면을 사용하세요.", 38, 404, 624, 44);
        MakeLabel(hwnd, L"", 38, 452, 624, 24, IDC_GAME_STATUS);

        gBuildingPage = 2;
        MakeLabel(hwnd, L"테마", 38, 98, 70, 24);
        MakeButton(hwnd, IDC_VIS_CLASSIC, L"Classic", 120, 92, 110, 30);
        MakeButton(hwnd, IDC_VIS_NEON, L"Neon", 242, 92, 110, 30);
        MakeButton(hwnd, IDC_VIS_MINIMAL, L"Minimal", 364, 92, 110, 30);
        MakeLabel(hwnd, L"", 490, 98, 174, 24, IDC_VISUAL_NAME);
        MakeLabel(hwnd, L"노트 너비 (24~220)", 38, 150, 185, 24); MakeEdit(hwnd, IDC_NOTE_WIDTH, L"", 236, 146, 76, 26);
        MakeLabel(hwnd, L"노트 높이 (18~140)", 348, 150, 185, 24); MakeEdit(hwnd, IDC_NOTE_HEIGHT, L"", 554, 146, 90, 26);
        MakeLabel(hwnd, L"레인 간격 % (50~160)", 38, 192, 195, 24); MakeEdit(hwnd, IDC_LANE_SPACING, L"", 236, 188, 76, 26);
        HWND spacing = TrackControl(CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
            338, 184, 316, 34, hwnd, reinterpret_cast<HMENU>(IDC_SPACING_SLIDER), gInstance, nullptr));
        SendMessageW(spacing, TBM_SETRANGE, TRUE, MAKELPARAM(50, 160));
        SendMessageW(spacing, TBM_SETTICFREQ, 10, 0);
        SendMessageW(spacing, TBM_SETPAGESIZE, 0, 5);
        MakeLabel(hwnd, L"사라지는 시간 ms", 38, 234, 195, 24); MakeEdit(hwnd, IDC_DURATION, L"", 236, 230, 76, 26);
        MakeLabel(hwnd, L"이동 거리 px", 348, 234, 195, 24); MakeEdit(hwnd, IDC_TRAVEL, L"", 554, 230, 90, 26);
        MakeLabel(hwnd, L"롱노트 진하기 (32~180)", 38, 276, 195, 24); MakeEdit(hwnd, IDC_LONG_ALPHA, L"", 236, 272, 76, 26);
        MakeLabel(hwnd, L"불투명도 % (20~100)", 348, 276, 200, 24); MakeEdit(hwnd, IDC_OPACITY, L"", 554, 272, 90, 26);
        MakeButton(hwnd, IDC_CIRCLE_NOTES, L"원형 노트", 38, 320, 260, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_LANE_BG, L"레인 배경", 348, 320, 260, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_ROUNDED_BOXES, L"둥근 모서리", 38, 356, 260, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_THIN_OUTLINE, L"얇은 테두리", 348, 356, 260, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_KEY_TEXT_BOTTOM, L"키 이름을 아래쪽에 표시", 38, 392, 420, 26, BS_AUTOCHECKBOX);
        MakeLabel(hwnd, L"[적용 + 테스트]로 현재 편집한 모양과 움직임을 확인할 수 있어요.", 38, 438, 624, 28);
        gBuildingPage = 3;
        MakeButton(hwnd, IDC_DIFFICULTY_ENABLED, L"입력 난이도 분석 사용", 38, 96, 290, 26, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_DIFFICULTY_HUD, L"OBS 오버레이에도 표시", 348, 96, 310, 26, BS_AUTOCHECKBOX);
        EnsureHudFonts();
        HWND profileLabel = MakeLabel(hwnd, L"실시간 분석", 38, 130, 220, 20, IDC_DIFFICULTY_PROFILE);
        HWND previousLabel = MakeLabel(hwnd, L"완료된 패턴 · 측정 레벨", 270, 130, 224, 20);
        HWND npsCaption = MakeLabel(hwnd, L"최근 1초 입력", 506, 130, 156, 20);
        HWND scoreLabel = MakeLabel(hwnd, L"대기", 38, 152, 220, 48, IDC_DIFFICULTY_SCORE);
        HWND previousValue = MakeLabel(hwnd, L"측정 전", 270, 152, 224, 48, IDC_DIFFICULTY_PREVIOUS);
        HWND npsLabel = MakeLabel(hwnd, L"NPS  0.0", 506, 164, 156, 32, IDC_DIFFICULTY_NPS);
        for (HWND control : {profileLabel, previousLabel, npsCaption, scoreLabel, previousValue, npsLabel})
            SetWindowLongPtrW(control, GWL_STYLE, GetWindowLongPtrW(control, GWL_STYLE) | SS_CENTER);
        for (HWND control : {profileLabel, previousLabel, npsCaption})
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(gHudLabelFont), TRUE);
        SendMessageW(scoreLabel, WM_SETFONT, reinterpret_cast<WPARAM>(gHudValueFont), TRUE);
        SendMessageW(previousValue, WM_SETFONT, reinterpret_cast<WPARAM>(gHudValueFont), TRUE);
        SendMessageW(npsLabel, WM_SETFONT, reinterpret_cast<WPARAM>(gHudSmallFont), TRUE);
        HWND progress = TrackControl(CreateWindowW(PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            38, 204, 624, 10, hwnd, reinterpret_cast<HMENU>(IDC_DIFFICULTY_BAR), gInstance, nullptr));
        SendMessageW(progress, PBM_SETRANGE32, 0, 100);
        MakeLabel(hwnd, L"", 38, 222, 624, 118, IDC_DIFFICULTY_METRICS);
        MakeLabel(hwnd, L"분석 키 수", 38, 344, 100, 24);
        HWND keyMode = TrackControl(CreateWindowW(WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            142, 341, 250, 240, hwnd, reinterpret_cast<HMENU>(IDC_ANALYSIS_KEYS), gInstance, nullptr));
        SendMessageW(keyMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"등록 키 수 (기존)"));
        SendMessageW(keyMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"자동 추정 · 추가 키 보정"));
        for (int keys = 4; keys <= 10; ++keys) {
            const auto label = std::to_wstring(keys) + L"K 수동 고정";
            SendMessageW(keyMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
        MakeLabel(hwnd, L"배치 유지 · 확정 시 레인 접기", 406, 344, 256, 24);
        MakeButton(hwnd, IDC_REPORT_AUTO, L"연주 리포트 자동 저장 (레인·타이밍 포함, 로컬 HTML)", 38, 374, 624, 26, BS_AUTOCHECKBOX);
        MakeLabel(hwnd, L"", 38, 404, 624, 38, IDC_REPORT_STATUS);
        MakeButton(hwnd, IDC_DIFFICULTY_RESET, L"분석 초기화", 38, 450, 180, 30);
        MakeButton(hwnd, IDC_REPORT_OPEN, L"리포트 폴더 열기", 234, 450, 208, 30);
        MakeButton(hwnd, IDC_REPORT_RETRY, L"저장 재시도", 458, 450, 204, 30);
        gBuildingPage = 4;
        MakeLabel(hwnd, L"색과 투명도를 바꿀 키", 38, 98, 238, 24);
        TrackControl(CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            38, 132, 230, 264, hwnd, reinterpret_cast<HMENU>(IDC_STYLE_KEYS), gInstance, nullptr));
        MakeLabel(hwnd, L"", 298, 98, 360, 24, IDC_STYLE_NAME);
        MakeLabel(hwnd, L"색상 HEX (#RRGGBB)", 298, 134, 360, 24);
        MakeEdit(hwnd, IDC_KEY_COLOR, L"", 298, 162, 174, 28);
        MakeButton(hwnd, IDC_PICK_COLOR, L"색상 선택…", 488, 160, 174, 32);
        MakeLabel(hwnd, L"불투명도 % (0~100)", 298, 206, 250, 24);
        MakeEdit(hwnd, IDC_KEY_OPACITY, L"100", 552, 202, 110, 28);
        MakeLabel(hwnd, L"0% 숨김 · 100% 진하게 / 빈 HEX는 기본색", 298, 244, 364, 26);
        MakeButton(hwnd, IDC_STYLE_PREVIEW, L"", 298, 282, 364, 68, BS_OWNERDRAW);
        MakeButton(hwnd, IDC_RESET_KEY_STYLE, L"선택 키 기본값", 298, 366, 364, 30);
        MakeLabel(hwnd, L"노트·롱노트·잔상에 함께 적용됩니다. 전체 불투명도와 곱해집니다.\n키 순서와 프리셋을 바꿔도 키별 설정을 유지합니다. [적용 + 저장]으로 기억해요.", 38, 420, 624, 54);
#if KEYNOTE_OSU_MEMORY_LINK
        gBuildingPage = 5;
        HWND warning=MakeLabel(hwnd,osu_link::RiskWarning,38,94,624,64,IDC_OSU_WARNING);
        SendMessageW(warning,WM_SETFONT,reinterpret_cast<WPARAM>(gOsuWarningFont),TRUE);
        MakeLabel(hwnd,L"비공식 메모리 조회 기능 · 안티치트 제재 및 계정 불이익이 없다고 보장하지 않습니다.\n기본 OFF이며 앱을 다시 켜면 반드시 OFF로 돌아갑니다.",38,163,624,44);
        MakeButton(hwnd,IDC_OSU_ENABLE,L"위험 안내를 확인하고 이번 실행에서만 연동 켜기",38,215,624,28,BS_AUTOCHECKBOX);
        MakeLabel(hwnd,L"OFF · 앱을 시작할 때마다 꺼져 있습니다.",38,251,624,34,IDC_OSU_STATUS);
        MakeLabel(hwnd,L"선택 채보 대기",38,288,624,36,IDC_OSU_TITLE);
        HWND rating=MakeLabel(hwnd,L"—",38,328,624,26,IDC_OSU_RATING);
        SendMessageW(rating,WM_SETFONT,reinterpret_cast<WPARAM>(gHudSmallFont),TRUE);
        HWND selectedDetails=MakeLabel(hwnd,L"",38,356,624,28,IDC_OSU_DETAILS);
        SendMessageW(selectedDetails,WM_SETFONT,reinterpret_cast<WPARAM>(gHudLabelFont),TRUE);
        HWND revive=MakeLabel(hwnd,L"",38,386,624,26,IDC_OSU_REVIVE);
        SendMessageW(revive,WM_SETFONT,reinterpret_cast<WPARAM>(gHudSmallFont),TRUE);
        MakeLabel(hwnd,L"",38,414,624,32,IDC_OSU_SKILLS);
        MakeButton(hwnd,IDC_OSU_FOLDER,L"Songs 폴더 지정…",38,450,180,26);
        MakeLabel(hwnd,L"원본 1.00× · 보조: 1~18K / EZ · 모드 미반영",230,450,432,24);
#endif
        gBuildingPage = -1;
        MakeLabel(hwnd, L"", 18, 490, 684, 24, IDC_DIRTY);
        MakeButton(hwnd, IDC_APPLY, L"적용", 18, 520, 100, 32);
        MakeButton(hwnd, IDC_SAVE, L"적용 + 저장", 130, 520, 140, 32);
        MakeButton(hwnd, IDC_TEST, L"적용 + 테스트", 282, 520, 150, 32);
        MakeButton(hwnd, IDC_REVERT, L"저장 상태로 되돌리기", 444, 520, 258, 32);
        MakeLabel(hwnd, L"", 18, 562, 684, 42, IDC_STATUS);
        FillSettingsControls(hwnd);
        ShowSettingsPage(0);
        RefreshDifficultyDisplay();
        if (!gUiSelfTest) SetTimer(hwnd, TIMER_DIFFICULTY, 100, nullptr);
        SetStatus(L"키 배치를 고르고 [적용 + 테스트]로 확인해 보세요.");
        return 0;
    }
    case WM_TIMER:
        if (wParam == TIMER_DIFFICULTY) RefreshDifficultyDisplay();
        if (wParam == TIMER_TRAY_RETRY && gAppRunning) AddTrayIcon();
        return 0;
    case WM_CTLCOLORSTATIC:
        if(GetDlgCtrlID(reinterpret_cast<HWND>(lParam))==IDC_OSU_WARNING) {
            SetTextColor(reinterpret_cast<HDC>(wParam),RGB(200,20,20));
            SetBkColor(reinterpret_cast<HDC>(wParam),GetSysColor(COLOR_BTNFACE));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }
        break;
    case WM_DRAWITEM:
        if (wParam == IDC_STYLE_PREVIEW) {
            const auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            DrawStylePreview(draw->hDC, draw->rcItem); return TRUE;
        }
        break;
    case WM_HSCROLL:
        if (!gFillingControls && reinterpret_cast<HWND>(lParam) == GetDlgItem(hwnd, IDC_SPACING_SLIDER)) {
            SetEditInt(hwnd, IDC_LANE_SPACING, static_cast<int>(SendDlgItemMessageW(hwnd, IDC_SPACING_SLIDER, TBM_GETPOS, 0, 0)));
            SetStatus(L"레인 간격을 조절했어요. [적용 + 테스트]로 확인하고 [적용 + 저장]으로 보관하세요.");
            return 0;
        }
        break;
    case WM_NOTIFY:
        if (reinterpret_cast<NMHDR*>(lParam)->idFrom == IDC_TABS && reinterpret_cast<NMHDR*>(lParam)->code == TCN_SELCHANGE)
            ShowSettingsPage(TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_TABS)));
        return 0;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int event = HIWORD(wParam);
        if (id == ID_TRAY_SETTINGS) { ShowSettingsWindow(); return 0; }
        if (id == ID_TRAY_EXIT) { RequestApplicationClose(); return 0; }
        if (id == ID_TRAY_MOVE) { ToggleOverlayMoveMode(); return 0; }
        if (gFillingControls) return 0;
        if (event == EN_CHANGE) {
            if (id == IDC_LANE_SPACING) {
                int spacing = 0;
                if (ParseInteger(WindowText(GetDlgItem(hwnd, id)), 50, 160, spacing))
                    SendDlgItemMessageW(hwnd, IDC_SPACING_SLIDER, TBM_SETPOS, TRUE, spacing);
            }
            if (gStyleVk && (id == IDC_KEY_COLOR || id == IDC_KEY_OPACITY)) {
                auto& draft = gDraftKeyStyles[gStyleVk];
                (id == IDC_KEY_COLOR ? draft.color : draft.opacity) = WindowText(GetDlgItem(hwnd, id));
                InvalidateRect(GetDlgItem(hwnd, IDC_STYLE_PREVIEW), nullptr, TRUE);
            }
            MarkEdited();
            if (id == IDC_LANES) RefreshLaneEditor();
            return 0;
        }
        if (id == IDC_STYLE_KEYS && event == LBN_SELCHANGE) {
            const int selected = static_cast<int>(SendDlgItemMessageW(hwnd, id, LB_GETCURSEL, 0, 0));
            gStyleVk = selected < 0 ? 0 : static_cast<UINT>(SendDlgItemMessageW(hwnd, id, LB_GETITEMDATA, selected, 0));
            FillKeyStyleFields(); return 0;
        }
        if (id == IDC_PRESET && event == CBN_SELCHANGE) {
            HWND combo = GetDlgItem(hwnd, IDC_PRESET);
            int selected = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
            int index = static_cast<int>(SendMessageW(combo, CB_GETITEMDATA, selected, 0));
            if (index >= 0 && index < static_cast<int>(kPresets.size())) {
                // Retain the previous valid draft before changing key counts;
                // neither selection nor typing changes the active mapping yet.
                RememberPreset(gDraftPresetLanes, WindowText(GetDlgItem(hwnd, IDC_LANES)));
                const auto& custom = gDraftPresetLanes[static_cast<std::size_t>(index)];
                SetWindowTextW(GetDlgItem(hwnd, IDC_LANES), custom.empty() ? kPresets[index].lanes : custom.c_str());
                SetStatus(L"기억한 키 배치를 불러왔어요. [적용 + 저장]하면 키 수별 배치가 다음 실행에도 유지됩니다.");
            }
            return 0;
        }
        if (id == IDC_ANALYSIS_KEYS && event == CBN_SELCHANGE) { MarkEdited(); return 0; }
        if (event != BN_CLICKED) break;
        switch (id) {
#if KEYNOTE_OSU_MEMORY_LINK
        case IDC_OSU_ENABLE: ToggleOsuLink(); return 0;
        case IDC_OSU_FOLDER: ChooseOsuSongs(); return 0;
#endif
        case IDC_PICK_COLOR: PickKeyColor(); return 0;
        case IDC_RESET_KEY_STYLE:
            if (gStyleVk) { gDraftKeyStyles[gStyleVk] = {}; FillKeyStyleFields(); MarkEdited(); }
            return 0;
        case IDC_RECORD: BeginKeyCapture(CaptureMode::Sequence); return 0;
        case IDC_REPLACE: BeginKeyCapture(CaptureMode::Replace); return 0;
        case IDC_MOVE_LEFT: case IDC_MOVE_RIGHT: case IDC_REMOVE: EditSelectedLane(id); return 0;
        case IDC_CENTER: CenterOverlayDraft(); return 0;
        case IDC_SIZE_SMALLER: ResizeOverlayDraft(0.9); return 0;
        case IDC_SIZE_LARGER: ResizeOverlayDraft(1.1); return 0;
        case IDC_MOVE_MODE: ToggleOverlayMoveMode(); return 0;
        case IDC_SAVE_POSITION: SavePositionDraft(); return 0;
        case IDC_REPORT_OPEN: {
            const auto directory = ReportDirectory();
            std::error_code error;
            if (!std::filesystem::is_directory(directory, error)) {
                SetStatus(L"아직 리포트 폴더가 없어요. 자동 저장을 켜고 연주를 마쳐 주세요.");
            } else if (!gUiSelfTest && reinterpret_cast<INT_PTR>(ShellExecuteW(hwnd, L"open", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) <= 32) {
                SetStatus(L"리포트 폴더를 열지 못했어요: " + directory.wstring());
            }
            return 0;
        }
        case IDC_REPORT_RETRY:
            gReportSaveFailed = false;
            PollReportWrites(); RefreshReportStatus(); return 0;
        case IDC_DIFFICULTY_RESET:
            ResetDifficulty(static_cast<int>(gConfig.lanes.size()));
            RefreshDifficultyDisplay();
            SetStatus(L"난이도 분석을 초기화했어요. 새 패턴을 입력해 보세요.");
            return 0;
        case IDC_VIS_CLASSIC: case IDC_VIS_NEON: case IDC_VIS_MINIMAL: {
            Config theme = gConfig;
            ApplyVisualPreset(id == IDC_VIS_CLASSIC ? L"Classic" : id == IDC_VIS_NEON ? L"Neon" : L"Minimal", theme);
            FillAppearance(theme);
            MarkEdited();
            SetStatus(L"테마를 선택했어요. 다른 탭에서 편집한 내용은 그대로 유지됩니다.");
            return 0;
        }
        case IDC_APPLY: ReadSettingsControls(); return 0;
        case IDC_SAVE: SaveSettings(); return 0;
        case IDC_TEST: if (ReadSettingsControls()) SpawnTestNotes(); return 0;
        case IDC_REVERT: {
            const bool resetAnalysis = gConfig.lanesText != gSavedConfig.lanesText || gConfig.difficultyEnabled != gSavedConfig.difficultyEnabled ||
                gConfig.reportAutoSave != gSavedConfig.reportAutoSave || gConfig.gameOverlay != gSavedConfig.gameOverlay ||
                gConfig.analysisKeyMode != gSavedConfig.analysisKeyMode;
            if (resetAnalysis)
                ResetDifficulty(static_cast<int>(gSavedConfig.lanes.size()));
            gConfig = gSavedConfig;
            if (resetAnalysis) gAnalysisKeys.Reset(static_cast<int>(gConfig.lanes.size()), gConfig.analysisKeyMode);
            RebuildLaneConfig();
            ResetInputRuntimeState();
            gUnapplied = gUnsaved = false;
            FillSettingsControls(hwnd);
            RefreshDifficultyDisplay();
            if (!gUiSelfTest) ApplyOverlayWindowStyle();
            SetStatus(L"마지막 저장 상태로 오버레이와 편집 내용을 되돌렸어요.");
            return 0;
        }
        case IDC_DEBUG_BG: case IDC_CLICK_THROUGH: case IDC_TOPMOST: case IDC_CIRCLE_NOTES:
        case IDC_LANE_BG: case IDC_ROUNDED_BOXES: case IDC_THIN_OUTLINE: case IDC_KEY_TEXT_BOTTOM:
        case IDC_DIFFICULTY_ENABLED: case IDC_DIFFICULTY_HUD: case IDC_REPORT_AUTO: case IDC_KEY_VIEWER_FOLDED:
            MarkEdited(); return 0;
        case IDC_GAME_OVERLAY:
            if (SendDlgItemMessageW(hwnd, IDC_GAME_OVERLAY, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                SendDlgItemMessageW(hwnd, IDC_CLICK_THROUGH, BM_SETCHECK, BST_CHECKED, 0);
                SendDlgItemMessageW(hwnd, IDC_TOPMOST, BM_SETCHECK, BST_CHECKED, 0);
                SendDlgItemMessageW(hwnd, IDC_DEBUG_BG, BM_SETCHECK, BST_UNCHECKED, 0);
            }
            MarkEdited(); return 0;
        }
        break;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            EndKeyCapture(true);
            if (!IsSettingsInteraction(reinterpret_cast<HWND>(lParam))) SetSettingsForegroundBoost(false);
        } else SetSettingsForegroundBoost(true);
        break;
    case WM_ACTIVATEAPP:
        if (!wParam) SetSettingsForegroundBoost(false);
        break;
    case WM_CLOSE:
        if (gCloseInProgress || gShuttingDown) return 0;
        gCloseInProgress = true;
        EndKeyCapture(true);
        if (gUnapplied || gUnsaved) {
            int choice = gExitMessageBox(hwnd, L"저장하지 않은 설정이 있어요. 적용하고 저장한 뒤 종료할까요?\n\n예: 적용 후 저장 / 아니요: 저장하지 않고 종료 / 취소: 계속 편집",
                L"설정 저장", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (choice == IDCANCEL || (choice == IDYES && !SaveSettings())) {
                gCloseInProgress = false;
                return 0;
            }
        }
        FinishSessionReport("앱 종료 · 진행 중 기록", NowMs());
        PollReportWrites(true);
        if (!gPendingReports.empty()) {
            const int choice = gExitMessageBox(hwnd,
                L"연주 리포트를 저장하지 못했어요. 아직 파일로 저장되지 않은 기록이 있습니다.\n\n예: 미저장 리포트를 버리고 종료 / 아니요: 돌아가서 저장 재시도",
                L"리포트 저장 실패", MB_YESNO | MB_ICONWARNING);
            if (choice != IDYES) { gCloseInProgress = false; RefreshReportStatus(); return 0; }
            gPendingReports.clear();
        }
        ShutdownApplication();
        return 0;
    case WM_ENDSESSION:
        if (wParam) ShutdownApplication();
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_DIFFICULTY);
        RemoveTrayIcon();
        if (gSettingsWnd == hwnd) gSettingsWnd = nullptr;
        if (!gShuttingDown) ShutdownApplication();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool RegisterWindows() {
    gTaskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
    WNDCLASSW overlay{};
    overlay.lpfnWndProc = OverlayProc;
    overlay.hInstance = gInstance;
    overlay.lpszClassName = L"KeyNoteOverlayCppWindow";
    overlay.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&overlay)) {
        return false;
    }

    WNDCLASSW settings{};
    settings.lpfnWndProc = SettingsProc;
    settings.hInstance = gInstance;
    settings.lpszClassName = L"KeyNoteOverlayCppSettings";
    settings.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    settings.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    return RegisterClassW(&settings) != 0;
}

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        // Recording is local to the focused settings window. Never swallow global input.
        if (gCaptureMode != CaptureMode::None && GetForegroundWindow() == gSettingsWnd)
            return CallNextHookEx(gKeyboardHook, code, wParam, lParam);
        const KBDLLHOOKSTRUCT* data = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        UINT vk = data->vkCode;
        bool keyDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
        bool keyUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;

        if (vk < gKeyDown.size() && keyUp) {
            gKeyDown[vk] = false;
            int lane = gVkToLaneIndex[vk];
            if (lane >= 0) {
                gPendingLaneUpMs[static_cast<size_t>(lane)] = NowMs();
                RecordDifficultyInput(lane, false, gPendingLaneUpMs[static_cast<size_t>(lane)]);
                PostMessageW(gOverlayWnd, WM_APP_KEY_UP, static_cast<WPARAM>(lane), 0);
            }
        } else if (vk < gKeyDown.size() && keyDown) {
            if (vk == VK_F12 && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                PostMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
            } else if (!gKeyDown[vk] && !gPickingColor && GameInputAllowed()) {
                gKeyDown[vk] = true;
                int lane = gVkToLaneIndex[vk];
                if (lane >= 0) {
                    gPendingLaneDownMs[static_cast<size_t>(lane)] = NowMs();
                    RecordDifficultyInput(lane, true, gPendingLaneDownMs[static_cast<size_t>(lane)]);
                    PostMessageW(gOverlayWnd, WM_APP_KEY_DOWN, static_cast<WPARAM>(lane), 0);
                }
            }
        }
    }
    return CallNextHookEx(gKeyboardHook, code, wParam, lParam);
}

BOOL CALLBACK CollectMonitorWorkArea(HMONITOR monitor, HDC, LPRECT, LPARAM param) {
    MONITORINFO info{}; info.cbSize = sizeof(info);
    if (GetMonitorInfoW(monitor, &info)) reinterpret_cast<std::vector<RECT>*>(param)->push_back(info.rcWork);
    return TRUE;
}

bool PlaceInitialSettingsWindow() {
    RECT overlay{}, settings{};
    if (!GetWindowRect(gOverlayWnd, &overlay) || !GetWindowRect(gSettingsWnd, &settings)) return false;
    std::vector<RECT> workAreas;
    MONITORINFO nearest{}; nearest.cbSize = sizeof(nearest);
    if (GetMonitorInfoW(MonitorFromRect(&overlay, MONITOR_DEFAULTTONEAREST), &nearest)) workAreas.push_back(nearest.rcWork);
    EnumDisplayMonitors(nullptr, nullptr, CollectMonitorWorkArea, reinterpret_cast<LPARAM>(&workAreas));
    if (workAreas.empty()) return false;
    const auto placement = window_placement::BesideOverlay(overlay,
        SIZE{settings.right - settings.left, settings.bottom - settings.top}, workAreas);
    // This runs before the settings window is shown, avoiding an initial jump.
    return SetWindowPos(gSettingsWnd, nullptr, placement.rect.left, placement.rect.top, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool CreateAppWindows(int showCommand) {
    DWORD overlayEx = static_cast<DWORD>(game_overlay::ExtendedStyle(0, gConfig.gameOverlay, gConfig.clickThrough, false));
    if (gConfig.gameOverlay || gConfig.alwaysOnTop) {
        overlayEx |= WS_EX_TOPMOST;
    }
    gOverlayWnd = CreateWindowExW(
        overlayEx,
        L"KeyNoteOverlayCppWindow",
        L"Key Note Overlay C++ Overlay",
        WS_POPUP,
        gConfig.x,
        gConfig.y,
        gConfig.width,
        gConfig.height,
        nullptr,
        nullptr,
        gInstance,
        nullptr
    );
    if (!gOverlayWnd) {
        return false;
    }
    ApplyOverlayWindowStyle();
    ShowWindow(gOverlayWnd, SW_SHOWNA);

    RECT settingsRect{0, 0, 720, 614};
    const DWORD settingsStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    AdjustWindowRectEx(&settingsRect, settingsStyle, FALSE, WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT);
    gSettingsWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT,
        L"KeyNoteOverlayCppSettings",
        L"Key Note Overlay C++ Settings",
        settingsStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        settingsRect.right - settingsRect.left,
        settingsRect.bottom - settingsRect.top,
        nullptr,
        nullptr,
        gInstance,
        nullptr
    );
    if (!gSettingsWnd) {
        return false;
    }
    PlaceInitialSettingsWindow();
    gAppRunning = true;
    SetWindowDisplayAffinity(gSettingsWnd, WDA_EXCLUDEFROMCAPTURE);
    (void)showCommand;
    ShowWindow(gSettingsWnd, SW_SHOWNORMAL);
    UpdateWindow(gSettingsWnd);
    return true;
}

int RunUiSelfTest(bool blockFirstReport = false) {
    gUiSelfTest = true;
    InitCommonControls();
    gGuiFont = CreateSettingsFont();
    gConfig = Config{};
    RebuildLaneConfig();
    gSavedConfig = gConfig;
    // This mode creates only hidden controls and never installs the keyboard hook.
    // Its caller supplies an isolated working directory for all generated files.
    gJsonConfigPath = CurrentDirectory() + L"ui_selftest.json";
    gIniConfigPath = CurrentDirectory() + L"ui_selftest.ini";
    gConfigPath = gJsonConfigPath;
    std::ofstream log("key_note_cpp_ui_selftest.log", std::ios::trunc);
    log << std::unitbuf; // Preserve the last completed check if a native test aborts.
    int failures = 0, checks = 0;
    auto check = [&](const char* name, bool ok) { log << name << "=" << (ok ? "ok" : "FAIL") << "\n"; ++checks; if (!ok) ++failures; };
    if (!RegisterWindows()) return 1;
    gSettingsWnd = CreateWindowW(L"KeyNoteOverlayCppSettings", L"test", WS_OVERLAPPEDWINDOW,
        0, 0, 760, 680, nullptr, nullptr, gInstance, nullptr);
    if (!gSettingsWnd) return 1;
    check("initial_clean_state", !gUnapplied && !gUnsaved && !IsWindowVisible(gSettingsWnd));
    {
        const auto originalConfig=gConfig;
        const auto originalJson=gJsonConfigPath,originalIni=gIniConfigPath;
        gJsonConfigPath=CurrentDirectory()+L"minimum-size-legacy.json";
        gIniConfigPath=CurrentDirectory()+L"minimum-size-legacy.ini";
        const std::string oldJson="{\"overlay\":{\"width\":450,\"height\":220,\"x\":123,\"y\":45}}";
        const std::string oldIni="[overlay]\nwidth=450\nheight=220\nx=123\ny=45\n";
        check("legacy_small_size_fixtures",WriteTextFile(gJsonConfigPath,oldJson) && WriteTextFile(gIniConfigPath,oldIni));
        gConfig=Config{};
        const bool loaded=LoadJsonConfig();ClampAndRebuildConfig();
        std::string unchanged;
        check("legacy_json_size_normalized_without_rewrite",loaded && gConfig.width==650 && gConfig.height==300 &&
            gConfig.x==123 && gConfig.y==45 && ReadTextFile(gJsonConfigPath,unchanged) && unchanged==oldJson);
        gConfig=Config{};LoadIniConfig();ClampAndRebuildConfig();
        check("legacy_ini_size_normalized_without_rewrite",gConfig.width==650 && gConfig.height==300 &&
            gConfig.x==123 && gConfig.y==45 && ReadTextFile(gIniConfigPath,unchanged) && unchanged==oldIni);
        gConfig=originalConfig;gJsonConfigPath=originalJson;gIniConfigPath=originalIni;RebuildLaneConfig();
    }
    auto click = [&](int id) { SendMessageW(gSettingsWnd, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(gSettingsWnd, id))); };
    auto edit = [&](int id, const wchar_t* value) { SetWindowTextW(GetDlgItem(gSettingsWnd, id), value); };
    auto raw = [&]() { return WindowText(GetDlgItem(gSettingsWnd, IDC_LANES)); };
#if KEYNOTE_OSU_MEMORY_LINK
    check("osu_link_defaults_off", !gOsuLink.Current().enabled &&
        SendDlgItemMessageW(gSettingsWnd,IDC_OSU_ENABLE,BM_GETCHECK,0,0)==BST_UNCHECKED);
    check("osu_risk_banner_visible_and_large", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_WARNING))==osu_link::RiskWarning &&
        reinterpret_cast<HFONT>(SendDlgItemMessageW(gSettingsWnd,IDC_OSU_WARNING,WM_GETFONT,0,0))==gOsuWarningFont);
    gOsuRiskMessageBox=+[](HWND,LPCWSTR text,LPCWSTR,UINT flags)->int {
        return (wcsstr(text,osu_link::RiskWarning) && (flags&MB_DEFBUTTON2)) ? IDNO : IDYES;
    };
    SendDlgItemMessageW(gSettingsWnd,IDC_OSU_ENABLE,BM_SETCHECK,BST_CHECKED,0);click(IDC_OSU_ENABLE);
    check("osu_warning_no_keeps_reader_off", !gOsuLink.Current().enabled &&
        SendDlgItemMessageW(gSettingsWnd,IDC_OSU_ENABLE,BM_GETCHECK,0,0)==BST_UNCHECKED);
    gOsuRiskMessageBox=&MessageBoxW;
    osu_link::View auxiliaryView;
    auxiliaryView.enabled=true; auxiliaryView.ready=true; auxiliaryView.keys=10;
    auxiliaryView.title=L"합성 채보"; auxiliaryView.difficulty=L"10단 수준";
    revive_aux::Summary skillFixture; skillFixture.ready=true; skillFixture.skills={52.07,43.02,14.22,0,57.48,0};
    auxiliaryView.reviveAuxiliary=revive_aux::Label(skillFixture);
    auxiliaryView.reviveCompact=revive_aux::Label(skillFixture,true);
    auxiliaryView.reviveSkills=revive_aux::Details(skillFixture);
    auxiliaryView.revive=skillFixture;
    ApplyOsuLinkView(auxiliaryView);
    check("osu_auxiliary_display_is_separate_from_primary", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE))==auxiliaryView.reviveAuxiliary &&
        WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_RATING))==L"원본 10K · 예상 10단 수준" && !gOsuLink.Current().enabled);
    check("osu_all_six_skills_displayed_without_lv", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_SKILLS))==auxiliaryView.reviveSkills &&
        WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE)).find(L"LV")==std::wstring::npos);
    auxiliaryView.reviveSkills=L"스트림 51  ·  속도 43  ·  연타 14\n동시치기 0  ·  테크닉 57  ·  홀드 0";
    ApplyOsuLinkView(auxiliaryView);
    check("osu_skill_values_only_change_refreshes", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_SKILLS))==auxiliaryView.reviveSkills);
    auxiliaryView.reviveAuxiliary=L"Revive CPP · 요구 스킬: 시간 한도 · 보류";
    ApplyOsuLinkView(auxiliaryView);
    check("osu_auxiliary_only_change_refreshes", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE))==auxiliaryView.reviveAuxiliary);
    auxiliaryView.keys=19; auxiliaryView.reviveAuxiliary=L"Revive CPP · 요구 스킬: 1~18K 지원";
    ApplyOsuLinkView(auxiliaryView);
    check("osu_unsupported_keys_replace_old_auxiliary", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE))==L"Revive CPP · 요구 스킬: 1~18K 지원");
    auxiliaryView={}; auxiliaryView.enabled=true; auxiliaryView.status=L"새 채보 분석 중";
    ApplyOsuLinkView(auxiliaryView);
    check("osu_new_selection_clears_auxiliary", WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE)).empty() &&
        WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_SKILLS)).empty() &&
        WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_RATING))==L"—");
    RefreshOsuLinkDisplay();
    check("osu_off_clears_auxiliary", !gOsuView.enabled && gOsuView.reviveAuxiliary.empty() &&
        WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE)).empty() && DifficultyHeaderHeight(900,220)==140);
#else
    check("no_osu_memory_tab_or_controls", TabCtrl_GetItemCount(GetDlgItem(gSettingsWnd,IDC_TABS))==5 &&
        !GetDlgItem(gSettingsWnd,IDC_OSU_ENABLE) && !GetDlgItem(gSettingsWnd,IDC_OSU_FOLDER) &&
        !GetDlgItem(gSettingsWnd,IDC_OSU_WARNING) && !GetDlgItem(gSettingsWnd,IDC_OSU_REVIVE) && !GetDlgItem(gSettingsWnd,IDC_OSU_SKILLS));
    click(IDC_OSU_ENABLE); click(IDC_OSU_FOLDER);
    check("removed_osu_commands_have_no_ui", !GetDlgItem(gSettingsWnd,IDC_OSU_STATUS));
#endif
    std::vector<std::wstring> tokens;
    std::wstring error;
    check("aliases_and_punctuation", ValidateLaneText(L" q,semicolon,comma,spacebar", tokens, error) && JoinLaneTokens(tokens) == L"Q,SEMICOLON,COMMA,SPACE");
    check("duplicate_alias_rejected", !ValidateLaneText(L"SPACE,SPC", tokens, error));
    check("empty_lane_rejected", !ValidateLaneText(L"D,,F", tokens, error) && !ValidateLaneText(L"D,", tokens, error) && !ValidateLaneText(L"", tokens, error));
    check("malformed_key_rejected", !ValidateLaneText(L"F1junk", tokens, error) && !ValidateLaneText(L"UNKNOWN", tokens, error));
    bool presetsOk = true;
    for (const auto& preset : kPresets) {
        presetsOk = presetsOk && ValidateLaneText(preset.lanes, tokens, error) &&
            tokens.size() == static_cast<size_t>(_wtoi(preset.mode)) && DetectModeForLanes(preset.lanes) == preset.mode;
    }
    check("all_seven_presets", presetsOk);
    int integer = 0;
    check("integer_validation", ParseInteger(L" -250 ", -32000, 32000, integer) && integer == -250 &&
        !ParseInteger(L"900px", 240, 10000, integer) && !ParseInteger(L"", 0, 10000, integer) &&
        !ParseInteger(L"99999999999999999999999999999", 0, 10000, integer) && !ParseInteger(L"101", 20, 100, integer));
    bool captureTokensOk = true;
    for (UINT vk : {static_cast<UINT>(VK_OEM_COMMA), static_cast<UINT>(VK_NUMPAD0), static_cast<UINT>(VK_NUMPAD9),
        static_cast<UINT>(VK_LSHIFT), static_cast<UINT>(VK_RSHIFT), static_cast<UINT>(VK_LCONTROL), static_cast<UINT>(VK_RMENU),
        static_cast<UINT>(VK_F24), static_cast<UINT>(VK_DIVIDE)}) {
        UINT back = 0; std::wstring label;
        captureTokensOk = captureTokensOk && TokenToVk(TokenFromVk(vk), back, label) && back == vk;
    }
    check("captured_key_roundtrip", captureTokensOk);
    edit(IDC_LANES, L"D,F,J,K");
    check("draft_does_not_apply", gUnapplied && gConfig.lanes.size() == 8 && SendDlgItemMessageW(gSettingsWnd, IDC_LANE_LIST, LB_GETCOUNT, 0, 0) == 4);
    SendDlgItemMessageW(gSettingsWnd, IDC_LANE_LIST, LB_SETCURSEL, 1, 0);
    click(IDC_MOVE_RIGHT);
    check("move_lane_right", raw() == L"D,J,F,K");
    click(IDC_MOVE_LEFT);
    check("move_lane_left", raw() == L"D,F,J,K");
    click(IDC_REMOVE);
    check("remove_selected_lane", raw() == L"D,J,K");
    edit(IDC_LANES, L"D"); click(IDC_REMOVE);
    check("last_lane_preserved", raw() == L"D");
    edit(IDC_LANES, L"D,F,J,K");
    check("apply_updates_mapping", ReadSettingsControls() && gConfig.lanes.size() == 4 && !gUnapplied && gUnsaved && gVkToLaneIndex[L'K'] == 3);
    const Config applied = gConfig;
    edit(IDC_X, L"123"); edit(IDC_WIDTH, L"invalid");
    check("invalid_draft_is_atomic", !ReadSettingsControls() && SameConfig(gConfig, applied) && gCurrentPage == 1 && gUnapplied);
    edit(IDC_WIDTH, L"1000"); edit(IDC_LANES, L"D,D");
    check("duplicate_keeps_runtime", !ReadSettingsControls() && SameConfig(gConfig, applied) && gCurrentPage == 0);
    edit(IDC_LANES, L"D,F,J,K");
    click(IDC_VIS_NEON);
    check("theme_preserves_other_edits", WindowText(GetDlgItem(gSettingsWnd, IDC_X)) == L"123" &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_WIDTH)) == L"1000" && raw() == L"D,F,J,K" && gConfig.visualPreset == applied.visualPreset);
    check("save_updates_snapshot", SaveSettings() && !gUnapplied && !gUnsaved && SameConfig(gConfig, gSavedConfig) && gConfig.x == 123 && gConfig.visualPreset == L"Neon");
    const Config saved = gSavedConfig;
    gConfig = Config{};
    check("saved_json_roundtrip", LoadJsonConfig() && (ClampAndRebuildConfig(), SameConfig(gConfig, saved)));
    gConfig = saved;
    RebuildLaneConfig(); FillSettingsControls(gSettingsWnd);
    BeginKeyCapture(CaptureMode::Sequence);
    CaptureKey(L'Q'); CaptureKey(L'W'); CaptureKey(L'Q');
    check("record_duplicate_rejected", raw() == L"Q,W" && gCaptureMode == CaptureMode::Sequence && SameConfig(gConfig, saved));
    CaptureKey(VK_OEM_COMMA);
    check("record_punctuation", raw() == L"Q,W,COMMA");
    CaptureKey(VK_ESCAPE);
    check("record_cancel_restores_draft", raw() == saved.lanesText && gCaptureMode == CaptureMode::None && !gUnapplied && !gUnsaved);
    BeginKeyCapture(CaptureMode::Sequence); CaptureKey(L'A'); CaptureKey(L'S'); EndKeyCapture(false);
    check("record_finish_is_draft", raw() == L"A,S" && gUnapplied && SameConfig(gConfig, saved));
    SendDlgItemMessageW(gSettingsWnd, IDC_LANE_LIST, LB_SETCURSEL, 1, 0);
    BeginKeyCapture(CaptureMode::Replace); CaptureKey(L'A');
    check("replace_duplicate_stays_recording", raw() == L"A,S" && gCaptureMode == CaptureMode::Replace);
    CaptureKey(VK_RSHIFT);
    check("replace_selected_key", raw() == L"A,RSHIFT" && gCaptureMode == CaptureMode::None);
    BeginKeyCapture(CaptureMode::Sequence); CaptureKey(L'Z');
    SendMessageW(gSettingsWnd, WM_ACTIVATE, WA_INACTIVE, 0);
    check("focus_loss_cancels_capture", raw() == L"A,RSHIFT" && gCaptureMode == CaptureMode::None && gUnapplied);
    click(IDC_REVERT);
    check("restore_saved_state", raw() == saved.lanesText && SameConfig(gConfig, saved) && !gUnapplied && !gUnsaved);
    HWND combo = GetDlgItem(gSettingsWnd, IDC_PRESET);
    SendMessageW(combo, CB_SETCURSEL, 4, 0);
    SendMessageW(gSettingsWnd, WM_COMMAND, MAKEWPARAM(IDC_PRESET, CBN_SELCHANGE), reinterpret_cast<LPARAM>(combo));
    check("preset_selection_is_draft", raw() == L"S,D,F,SPACE,J,K,L" && SameConfig(gConfig, saved) && gUnapplied);
    click(IDC_REVERT);
    auto selectPreset = [&](int count) {
        const int target = PresetIndexForCount(static_cast<std::size_t>(count));
        for (int row = 1; row <= static_cast<int>(kPresets.size()); ++row) {
            if (SendMessageW(combo, CB_GETITEMDATA, row, 0) == target) {
                SendMessageW(combo, CB_SETCURSEL, row, 0);
                SendMessageW(gSettingsWnd, WM_COMMAND, MAKEWPARAM(IDC_PRESET, CBN_SELCHANGE), reinterpret_cast<LPARAM>(combo));
                break;
            }
        }
    };
    const std::vector<std::wstring> customKeys{L"Q",L"W",L"E",L"R",L"T",L"Y",L"U",L"I",L"O",L"P"};
    for (int count=4; count<=10; ++count) {
        selectPreset(count);
        edit(IDC_LANES, JoinLaneTokens(std::vector<std::wstring>(customKeys.begin(),customKeys.begin()+count)).c_str());
    }
    selectPreset(4);
    check("preset_switch_retains_all_drafts", raw() == L"Q,W,E,R" && SameConfig(gConfig,saved) && gUnapplied && DetectModeForLanes(raw()) == L"4K");
    BeginKeyCapture(CaptureMode::Sequence);
    for (UINT key : {L'A',L'S',L'K',L'L'}) CaptureKey(key);
    EndKeyCapture(false);
    selectPreset(8); selectPreset(4);
    check("recorded_layout_stays_in_its_preset", raw() == L"A,S,K,L");
    edit(IDC_LANES,L"D,D"); selectPreset(7); selectPreset(4);
    check("invalid_draft_cannot_poison_preset", raw() == L"A,S,K,L");
    check("custom_presets_save", SaveSettings());
    const Config customSaved = gSavedConfig;
    gConfig = Config{};
    check("all_presets_json_reload", LoadJsonConfig() && (ClampAndRebuildConfig(), SameConfig(gConfig,customSaved)));
    FillSettingsControls(gSettingsWnd);
    bool restoredPresets = true;
    for (int count=4; count<=10; ++count) {
        selectPreset(count);
        const auto expected = count == 4 ? L"A,S,K,L" : JoinLaneTokens(std::vector<std::wstring>(customKeys.begin(),customKeys.begin()+count));
        restoredPresets = restoredPresets && raw() == expected;
    }
    check("all_presets_reselected_after_reload", restoredPresets);
    gConfig = Config{}; LoadIniConfig(); ClampAndRebuildConfig();
    check("all_presets_ini_reload", SameConfig(gConfig,customSaved));
    gConfig = customSaved; FillSettingsControls(gSettingsWnd);
    selectPreset(4); edit(IDC_LANES,L"Z,X,C,V"); selectPreset(8); click(IDC_REVERT); selectPreset(4);
    check("revert_restores_saved_preset_collection", raw() == L"A,S,K,L");
    const auto presetsJsonPath = gJsonConfigPath;
    const auto presetsIniPath = gIniConfigPath;
    gJsonConfigPath = CurrentDirectory() + L"legacy_presets.json";
    check("legacy_custom_layout_file", WriteTextFile(gJsonConfigPath, "{\"input\":{\"mode\":\"CUSTOM\",\"lanes\":[\"A\",\"S\",\"K\",\"L\"]}}"));
    gConfig = Config{};
    check("legacy_custom_layout_becomes_its_preset", LoadJsonConfig() && gConfig.presetLanes[PresetIndexForCount(4)] == L"A,S,K,L");
    check("invalid_preset_fixture", WriteTextFile(gJsonConfigPath, "{\"input\":{\"lanes\":[\"D\",\"F\",\"J\",\"K\"],\"preset_5k\":\"D,D,F,J,K\",\"preset_7k\":\"Q,W,E,R\"}}"));
    gConfig = Config{};
    check("invalid_or_wrong_count_presets_use_defaults", LoadJsonConfig() &&
        gConfig.presetLanes[PresetIndexForCount(5)].empty() && gConfig.presetLanes[PresetIndexForCount(7)].empty());
    gIniConfigPath = CurrentDirectory() + L"legacy_presets.ini";
    check("legacy_ini_custom_fixture", WriteTextFile(gIniConfigPath,"[input]\nmode=CUSTOM\nlanes=Q,W,E,SPACE,I,O,P\n"));
    gConfig = Config{}; LoadIniConfig();
    check("legacy_ini_custom_layout_becomes_its_preset", gConfig.presetLanes[PresetIndexForCount(7)] == L"Q,W,E,SPACE,I,O,P");
    gJsonConfigPath = presetsJsonPath; gIniConfigPath = presetsIniPath;
    gConfig = saved; gSavedConfig = saved; RebuildLaneConfig();
    gUnapplied = gUnsaved = false;
    FillSettingsControls(gSettingsWnd); SaveConfig();
    edit(IDC_WIDTH, L"800"); edit(IDC_HEIGHT, L"300");
    CenterOverlayDraft();
    MONITORINFO info{sizeof(info), {}, {}, 0};
    bool centerOk = GetMonitorInfoW(MonitorFromWindow(gSettingsWnd, MONITOR_DEFAULTTONEAREST), &info) &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_X)) == std::to_wstring(info.rcWork.left + (info.rcWork.right - info.rcWork.left - 800) / 2) &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_Y)) == std::to_wstring(info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - 300) / 2);
    check("center_uses_current_monitor", centerOk && SameConfig(gConfig, saved));
    click(IDC_REVERT);
    {
        const Config beforeSizing = gConfig;
        edit(IDC_WIDTH, L"900"); edit(IDC_HEIGHT, L"400");
        edit(IDC_NOTE_WIDTH, L"58"); edit(IDC_NOTE_HEIGHT, L"34"); edit(IDC_TRAVEL, L"180");
        edit(IDC_X, L"123"); edit(IDC_LANE_SPACING, L"85");
        click(IDC_SIZE_SMALLER);
        Config smaller;
        check("resize_scales_window_notes_and_travel", BuildSettingsCandidate(smaller, false) &&
            smaller.width == 810 && smaller.height == 360 && smaller.noteWidth == 52 &&
            smaller.noteHeight == 31 && smaller.travelPx == 162);
        check("resize_is_draft_and_preserves_other_fields", SameConfig(gConfig, beforeSizing) && gUnapplied &&
            smaller.x == 123 && smaller.laneSpacingPercent == 85 && smaller.lanesText == beforeSizing.lanesText);
        click(IDC_SIZE_LARGER);
        Config larger;
        check("resize_grows_current_draft", BuildSettingsCandidate(larger, false) && larger.width == 891 &&
            larger.height == 396 && larger.noteWidth == 57 && larger.noteHeight == 34 && larger.travelPx == 178);
        for (int i = 0; i < 40; ++i) click(IDC_SIZE_SMALLER);
        check("resize_minimum_keeps_note_below_hud", BuildSettingsCandidate(smaller, false) &&
            smaller.width == 650 && smaller.noteWidth == 24 && smaller.noteHeight == 18 && smaller.travelPx == 40 &&
            smaller.height == 300);
        edit(IDC_WIDTH,L"649");
        check("width_below_650_is_rejected", !BuildSettingsCandidate(smaller,false) && SameConfig(gConfig,beforeSizing));
        edit(IDC_WIDTH,L"650");edit(IDC_HEIGHT,L"299");
        check("height_below_300_is_rejected", !BuildSettingsCandidate(smaller,false) && SameConfig(gConfig,beforeSizing));
        edit(IDC_HEIGHT,L"300");
        check("exact_minimum_dimensions_are_accepted", BuildSettingsCandidate(smaller,false) && smaller.width==650 && smaller.height==300);
        edit(IDC_WIDTH, L"invalid");
        const auto heightBefore = WindowText(GetDlgItem(gSettingsWnd, IDC_HEIGHT));
        click(IDC_SIZE_SMALLER);
        check("resize_invalid_draft_is_not_partially_changed", WindowText(GetDlgItem(gSettingsWnd, IDC_WIDTH)) == L"invalid" &&
            WindowText(GetDlgItem(gSettingsWnd, IDC_HEIGHT)) == heightBefore && SameConfig(gConfig, beforeSizing));
        click(IDC_REVERT);
        HWND slider = GetDlgItem(gSettingsWnd, IDC_SPACING_SLIDER);
        check("spacing_slider_range", slider && SendMessageW(slider, TBM_GETRANGEMIN, 0, 0) == 50 &&
            SendMessageW(slider, TBM_GETRANGEMAX, 0, 0) == 160);
        edit(IDC_LANE_SPACING, L"123");
        check("spacing_numeric_updates_slider", SendMessageW(slider, TBM_GETPOS, 0, 0) == 123);
        SendMessageW(slider, TBM_SETPOS, TRUE, 65);
        SendMessageW(gSettingsWnd, WM_HSCROLL, MAKEWPARAM(TB_THUMBTRACK, 65), reinterpret_cast<LPARAM>(slider));
        check("spacing_slider_updates_draft_only", WindowText(GetDlgItem(gSettingsWnd, IDC_LANE_SPACING)) == L"65" &&
            SameConfig(gConfig, beforeSizing) && gUnapplied);
        edit(IDC_WIDTH, L"900"); edit(IDC_HEIGHT, L"300"); click(IDC_SIZE_SMALLER);
        check("size_and_spacing_save", SaveSettings());
        const Config sized = gConfig;
        gConfig = Config{};
        check("size_and_spacing_json_reload", LoadJsonConfig() && gConfig.width == sized.width &&
            gConfig.height == sized.height && gConfig.noteWidth == sized.noteWidth && gConfig.noteHeight == sized.noteHeight &&
            gConfig.travelPx == sized.travelPx && gConfig.laneSpacingPercent == 65);
        gConfig = Config{}; LoadIniConfig();
        check("size_and_spacing_ini_reload", gConfig.width == sized.width && gConfig.height == sized.height &&
            gConfig.noteWidth == sized.noteWidth && gConfig.noteHeight == sized.noteHeight &&
            gConfig.travelPx == sized.travelPx && gConfig.laneSpacingPercent == 65);
        gConfig = beforeSizing; gSavedConfig = beforeSizing; RebuildLaneConfig();
        gUnapplied = gUnsaved = false; FillSettingsControls(gSettingsWnd); SaveConfig();
        check("size_and_spacing_restore", WindowText(GetDlgItem(gSettingsWnd, IDC_WIDTH)) == std::to_wstring(beforeSizing.width) &&
            SendMessageW(slider, TBM_GETPOS, 0, 0) == beforeSizing.laneSpacingPercent);
    }
    ShowSettingsPage(3);
    ResetDifficulty(static_cast<int>(gConfig.lanes.size()));
    for (int i = 0; i < 24; ++i) {
        RecordDifficultyInput(i % 4, true, i * 125.0);
        RecordDifficultyInput(i % 4, false, i * 125.0 + 10);
    }
    RefreshDifficultyDisplay(3000);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(3000);}
    check("registered_four_key_cpp_live",gReviveLive.Current().ready && gAnalysisKeys.Keys()==4);
    check("difficulty_live_settings_display", gDifficultySnapshot.ready && gDifficultySnapshot.samples == 24 &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_SCORE)).find(L"단") != std::wstring::npos &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_PROFILE)) == L"Reform 4K · 분석" &&
        DifficultyDetails(true).find(L"4K Reform v1  |  참고 ★") != std::wstring::npos);
    check("large_current_nps_display", WindowText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_NPS)) == L"NPS  7.0" &&
        reinterpret_cast<HFONT>(SendDlgItemMessageW(gSettingsWnd, IDC_DIFFICULTY_NPS, WM_GETFONT, 0, 0)) == gHudSmallFont);
    check("previous_result_not_published_early", !gPatternSummary.Completed().ready);
    RefreshDifficultyDisplay(6000);
    check("short_pause_keeps_rating_and_live_nps", !gDifficultySnapshot.idle && gDifficultySnapshot.retained &&
        gDifficultySnapshot.ready && gDifficultySnapshot.currentNps == 0 && gDifficultySnapshot.nps > 0 && !gPatternSummary.Completed().ready);
    RefreshDifficultyDisplay(12884);
    check("session_continues_until_ten_seconds_after_release", !gDifficultySnapshot.idle && !gPatternSummary.Completed().ready);
    RefreshDifficultyDisplay(12885);
    const auto completedText = PreviousPatternValue();
    check("finished_pattern_level_visible", gPatternSummary.Completed().ready &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_PREVIOUS)) == completedText &&
        completedText == FormatDifficultyRating(gPatternSummary.Completed()) &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_NPS)) == L"NPS  0.0");
    RefreshDifficultyDisplay(15000);
    check("finished_level_stays_when_idle", PreviousPatternValue() == completedText);
    check("finished_pattern_keeps_auxiliary_axes", !gReviveLive.Current().ready && ShowsCompletedSkills());
    const auto completedLn = pattern::FormatLnDan(gPatternSummary.Completed());
    check("compact_ln_row_shows_finished_pattern", !completedLn.empty() &&
        HudLnValue(450) == L"완료 " + completedLn.substr(3) && HudLnValue(900) == HudLnValue(450));
    const auto beforePendingInput = gDifficultySnapshot;
    gDifficultySnapshot.idle = false; // first few keys after a completed session
    check("pending_input_keeps_completed_axes_visible", ShowsCompletedSkills());
    gDifficultySnapshot = beforePendingInput;
    RECT liveRect{}, previousRect{}, npsRect{};
    GetWindowRect(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_SCORE), &liveRect);
    GetWindowRect(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_PREVIOUS), &previousRect);
    GetWindowRect(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_NPS), &npsRect);
    check("previous_pattern_between_live_and_nps", liveRect.right < previousRect.left && previousRect.right < npsRect.left);
    gOverlayWnd = CreateWindowW(L"KeyNoteOverlayCppWindow", L"hidden preview test", WS_POPUP,
        0, 0, 900, 220, nullptr, nullptr, gInstance, nullptr);
    const auto beforePreview = gDifficulty.Evaluate(3000);
    SpawnTestNotes();
    check("preview_notes_excluded_from_analysis", gOverlayWnd && !gNotes.empty() &&
        gDifficulty.Evaluate(3000).samples == beforePreview.samples);
    if (gOverlayWnd) DestroyWindow(gOverlayWnd);
    gOverlayWnd = nullptr;
    gNotes.clear();
    const auto beforeAppearanceApply = gDifficulty.Evaluate(3000).samples;
    SetEditInt(gSettingsWnd, IDC_OPACITY, 80);
    check("appearance_apply_keeps_analysis_history", ReadSettingsControls() && gDifficulty.Evaluate(3000).samples == beforeAppearanceApply);
    SendDlgItemMessageW(gSettingsWnd, IDC_DIFFICULTY_HUD, BM_SETCHECK, BST_UNCHECKED, 0);
    click(IDC_DIFFICULTY_HUD);
    check("hud_toggle_is_draft", gUnapplied && gConfig.difficultyHud);
    check("hud_can_be_hidden_independently", ReadSettingsControls() && !gConfig.difficultyHud &&
        gConfig.difficultyEnabled && DifficultyHeaderHeight(900, 220) == 0);
    SendDlgItemMessageW(gSettingsWnd, IDC_DIFFICULTY_HUD, BM_SETCHECK, BST_CHECKED, 0);
    check("hud_adapts_to_small_overlay", ReadSettingsControls() && CurrentHudLayout(900).liveLnHeight == 40 &&
        DifficultyHeaderHeight(900, 220) == 180 && DifficultyHeaderHeight(240, 90) == 344 && DifficultyHeaderHeight(600,90) == 216);
    SendDlgItemMessageW(gSettingsWnd, IDC_DIFFICULTY_ENABLED, BM_SETCHECK, BST_UNCHECKED, 0);
    bool disabled = ReadSettingsControls();
    RecordDifficultyInput(0, true, 4000);
    check("disabled_analysis_ignores_inputs", disabled && !gConfig.difficultyEnabled && gDifficulty.BufferedEvents() == 0 && DifficultyHeaderHeight(900, 220) == 0);
    check("difficulty_settings_save", SaveSettings());
    Config savedDifficulty = gConfig;
    gConfig = Config{};
    check("difficulty_json_roundtrip", LoadJsonConfig() && !gConfig.difficultyEnabled && gConfig.difficultyHud);
    gConfig = Config{};
    LoadIniConfig();
    check("difficulty_ini_roundtrip", !gConfig.difficultyEnabled && gConfig.difficultyHud);
    const auto originalJsonPath = gJsonConfigPath;
    const auto originalIniPath = gIniConfigPath;
    gJsonConfigPath = CurrentDirectory() + L"legacy_difficulty.json";
    gIniConfigPath = CurrentDirectory() + L"legacy_difficulty.ini";
    bool legacyJsonWritten = WriteTextFile(gJsonConfigPath, "{\"overlay\":{\"width\":640},\"input\":{\"lanes\":[\"D\",\"F\",\"J\",\"K\"]}}");
    gConfig = Config{};
    check("legacy_json_uses_difficulty_defaults", legacyJsonWritten && LoadJsonConfig() && gConfig.width == 640 && gConfig.difficultyEnabled && gConfig.difficultyHud);
    bool legacyIniWritten = WriteTextFile(gIniConfigPath, "[overlay]\nwidth=640\n[input]\nlanes=D,F,J,K\n");
    gConfig = Config{};
    LoadIniConfig();
    check("legacy_ini_uses_difficulty_defaults", legacyIniWritten && gConfig.width == 640 && gConfig.difficultyEnabled && gConfig.difficultyHud);
    gJsonConfigPath = originalJsonPath;
    gIniConfigPath = originalIniPath;
    gConfig = savedDifficulty;
    FillSettingsControls(gSettingsWnd);
    SendDlgItemMessageW(gSettingsWnd, IDC_DIFFICULTY_ENABLED, BM_SETCHECK, BST_CHECKED, 0);
    check("reenable_starts_fresh", ReadSettingsControls() && gConfig.difficultyEnabled && gDifficulty.BufferedEvents() == 0);
    RecordDifficultyInput(0, true, 5000);
    RecordDifficultyInput(0, false, 5010);
    BeginKeyCapture(CaptureMode::Sequence);
    RecordDifficultyInput(1, true, 5200);
    check("layout_recording_excluded_from_analysis", gDifficulty.BufferedEvents() == 0);
    EndKeyCapture(true);
    ShowSettingsPage(3);
    RecordDifficultyInput(0, true, 6000);
    click(IDC_DIFFICULTY_RESET);
    check("difficulty_reset_button", gDifficulty.BufferedEvents() == 0 && gDifficultySnapshot.idle && !gPatternSummary.Completed().ready);
    RecordDifficultyInput(0, true, 7000);
    RecordDifficultyInput(0, false, 7010);
    edit(IDC_LANES, L"A,S,D,F,J,K,L,SEMICOLON");
    check("layout_change_resets_analysis", ReadSettingsControls() && gDifficulty.BufferedEvents() == 0);

    // Per-key drafts are independent of lane order, theme, and inactive presets.
    const Config beforeStyleTests = gConfig;
    const Config savedBeforeStyles = gSavedConfig;
    auto selectStyle = [&](UINT vk) {
        HWND list = GetDlgItem(gSettingsWnd, IDC_STYLE_KEYS);
        const int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
        for (int i=0; i<count; ++i) if (static_cast<UINT>(SendMessageW(list, LB_GETITEMDATA, i, 0)) == vk) {
            SendMessageW(list, LB_SETCURSEL, i, 0);
            SendMessageW(gSettingsWnd, WM_COMMAND, MAKEWPARAM(IDC_STYLE_KEYS,LBN_SELCHANGE), reinterpret_cast<LPARAM>(list));
            return true;
        }
        return false;
    };
    COLORREF parsedColor = 0;
    check("key_color_hex_validation", ParseColor(L"#aB12EF",parsedColor) && parsedColor==RGB(171,18,239) &&
        !ParseColor(L"#12345Z",parsedColor) && !ParseColor(L"#123",parsedColor) &&
        ParseColor(L"",parsedColor) && parsedColor==CLR_INVALID);
    edit(IDC_LANES,L"D,F,J,K"); selectStyle('D');
    edit(IDC_KEY_COLOR,L"#FF0000"); edit(IDC_KEY_OPACITY,L"50"); selectStyle('F');
    edit(IDC_KEY_COLOR,L"#00FF00"); edit(IDC_KEY_OPACITY,L"0"); selectStyle('D');
    check("key_style_independent_drafts", gStyleVk=='D' && WindowText(GetDlgItem(gSettingsWnd,IDC_KEY_COLOR))==L"#FF0000" &&
        WindowText(GetDlgItem(gSettingsWnd,IDC_KEY_OPACITY))==L"50" && gConfig.keyStyles['D'].color==CLR_INVALID);
    edit(IDC_KEY_COLOR,L"#BAD"); selectStyle('F');
    Config invalidStyle;
    check("invalid_key_color_survives_selection_and_blocks_apply", !BuildSettingsCandidate(invalidStyle,false) && gDraftKeyStyles['D'].color==L"#BAD");
    selectStyle('D'); edit(IDC_KEY_COLOR,L"#FF0000"); edit(IDC_KEY_OPACITY,L"101");
    check("invalid_key_opacity_blocks_apply", !BuildSettingsCandidate(invalidStyle,false));
    edit(IDC_KEY_OPACITY,L"50"); click(IDC_VIS_NEON);
    check("theme_preserves_key_styles", gDraftKeyStyles['D'].color==L"#FF0000" && gDraftKeyStyles['F'].opacity==L"0");
    edit(IDC_LANES,L"F,D,J,K");
    check("key_style_apply_and_reorder", ReadSettingsControls() && LaneColor(1)==RGB(255,0,0) &&
        gConfig.keyStyles[gConfig.lanes[0].vk].opacity==0);
    selectPreset(10); selectPreset(4);
    check("key_style_preset_switch", gDraftKeyStyles['D'].color==L"#FF0000" && gDraftKeyStyles['D'].opacity==L"50");
    check("key_style_save", SaveSettings());
    const Config stylesSaved = gSavedConfig;
    gConfig=Config{};
    check("key_style_json_reload", LoadJsonConfig() && (ClampAndRebuildConfig(), SameConfig(gConfig,stylesSaved)));
    gConfig=Config{}; LoadIniConfig(); ClampAndRebuildConfig();
    check("key_style_ini_reload", SameConfig(gConfig,stylesSaved));
    bool specialStyleKeys = true;
    for (UINT vk : {static_cast<UINT>(VK_ESCAPE),static_cast<UINT>(VK_SHIFT),static_cast<UINT>(VK_CONTROL),static_cast<UINT>(VK_MENU),
        static_cast<UINT>(VK_OEM_COMMA),static_cast<UINT>(VK_LSHIFT),static_cast<UINT>(VK_RSHIFT),static_cast<UINT>(VK_NUMPAD9)}) {
        UINT decoded=0; std::wstring label;
        specialStyleKeys = specialStyleKeys && !TokenFromVk(vk).empty() && TokenToVk(TokenFromVk(vk),decoded,label) && decoded==vk;
    }
    check("special_keys_have_persistent_style_names", specialStyleKeys);
    FillSettingsControls(gSettingsWnd); selectStyle('D'); click(IDC_RESET_KEY_STYLE);
    check("key_style_reset_selected_only", gDraftKeyStyles['D'].color.empty() && gDraftKeyStyles['D'].opacity==L"100" && gDraftKeyStyles['F'].opacity==L"0");
    click(IDC_REVERT);
    check("key_style_revert", gDraftKeyStyles['D'].color==L"#FF0000" && gDraftKeyStyles['D'].opacity==L"50");
    const auto styleJsonPath = gJsonConfigPath;
    gJsonConfigPath=CurrentDirectory()+L"legacy_key_styles.json";
    WriteTextFile(gJsonConfigPath,"{\"input\":{\"lanes\":[\"D\",\"F\",\"J\",\"K\"]}}");
    check("legacy_key_styles_default_palette", LoadJsonConfig() && gConfig.keyStyles['D'].color==CLR_INVALID && gConfig.keyStyles['D'].opacity==100);
    WriteTextFile(gJsonConfigPath,"{\"key_styles\":{\"key_color_D\":\"#oops\",\"key_opacity_D\":-20,\"key_opacity_F\":900}}");
    check("malformed_saved_styles_safe", LoadJsonConfig() && gConfig.keyStyles['D'].color==CLR_INVALID &&
        gConfig.keyStyles['D'].opacity==0 && gConfig.keyStyles['F'].opacity==100);
    gJsonConfigPath=styleJsonPath;

    // Render our own pixels. No desktop capture or physical input is involved.
    gConfig=Config{}; gConfig.lanesText=L"D,F,J,K"; RebuildLaneConfig();
    gConfig.difficultyHud=gConfig.debugBackground=gConfig.laneBackground=false;
    overlay::AlphaSurface frame(900,220);
    Note sample; sample.lane=0; sample.label=L"D"; sample.pressMs=1000; sample.opacity=50; sample.color=RGB(255,0,0);
    gNotes={sample};
    auto hasPixel = [&](DWORD pixel) { return std::find(frame.pixels,frame.pixels+900*220,pixel)!=frame.pixels+900*220; };
    auto onlyAlpha = [&](unsigned alpha) { return std::all_of(frame.pixels,frame.pixels+900*220,[&](DWORD pixel) { return pixel==0 || pixel>>24==alpha; }); };
    check("key_opacity_real_alpha_50", RenderOverlay(frame,1100) && hasPixel(0x7f7f0000) && onlyAlpha(127) && frame.pixels[0]==0);
    gNotes[0].opacity=0;
    check("key_opacity_zero_hides_entire_note", RenderOverlay(frame,1100) && onlyAlpha(0));
    gNotes[0].opacity=100; gNotes[0].color=kTransparentKey;
    check("custom_color_can_equal_old_transparency_key", RenderOverlay(frame,1100) && hasPixel(0xff010203) && onlyAlpha(255));
    gNotes[0].color=RGB(0,0,0);
    check("custom_black_is_visible", RenderOverlay(frame,1100) && hasPixel(0xff000000));
    gNotes={sample,sample};
    check("overlapping_notes_alpha_composite", RenderOverlay(frame,1100) && hasPixel(0xbfbf0000u));
    gNotes={sample}; gConfig.debugBackground=true;
    check("key_opacity_over_opaque_background", RenderOverlay(frame,1100) && onlyAlpha(255));
    gConfig.debugBackground=false; gConfig.circleNotes=true;
    check("circle_note_alpha", RenderOverlay(frame,1100) && hasPixel(0x7f7f0000) && onlyAlpha(127));
    gConfig.circleNotes=false;
    gNotes[0].held=false; gNotes[0].releaseMs=1500;
    check("released_long_note_alpha", RenderOverlay(frame,1600) && hasPixel(0x7f7f0000) && onlyAlpha(127));
    gNotes[0].opacity=0;
    check("released_long_note_zero_hides_shadow_and_text", RenderOverlay(frame,1600) && onlyAlpha(0));
    gNotes={sample}; gConfig.difficultyHud=true;
    check("key_opacity_does_not_fade_hud", RenderOverlay(frame,1100) && frame.pixels[10*900+10]>>24==255 && hasPixel(0x7f7f0000));
    HWND alphaWindow = CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW,L"KeyNoteOverlayCppWindow",L"hidden alpha test",WS_POPUP,
        0,0,900,220,nullptr,nullptr,gInstance,nullptr);
    check("native_layered_alpha_present", alphaWindow && PresentOverlay(alphaWindow,frame) && !IsWindowVisible(alphaWindow));
    gConfig.opacityPercent=50;
    check("native_global_alpha_with_per_key_alpha", PresentOverlay(alphaWindow,frame));
    const HWND previousOverlay = gOverlayWnd;
    gOverlayWnd=alphaWindow; gNotes.clear();
    gConfig.keyStyles['D']={RGB(250,60,90),37};
    BeginLaneHold(0);
    check("physical_note_uses_selected_key_style", gNotes.size()==1 && gNotes[0].color==RGB(250,60,90) && gNotes[0].opacity==37);
    gNotes.clear(); SpawnTapNote(0);
    check("test_note_uses_selected_key_style", gNotes.size()==1 && gNotes[0].color==RGB(250,60,90) && gNotes[0].opacity==37);
    gOverlayWnd=previousOverlay;
    if (alphaWindow) DestroyWindow(alphaWindow);

    // A deterministic full overlay preview is exported from the same renderer.
    gConfig.opacityPercent=100; gConfig.difficultyHud=false; gNotes.clear();
    for (int i=0;i<4;++i) {
        sample.lane=i; sample.label=gConfig.lanes[i].label; sample.opacity=100-i*25;
        sample.color=i==0?RGB(255,70,100):i==1?RGB(70,220,255):i==2?RGB(160,100,255):RGB(255,210,70);
        sample.pressMs=1000-i*100; gNotes.push_back(sample);
    }
    RenderOverlay(frame,1400);
    BITMAPINFOHEADER alphaHeader{};
    alphaHeader.biSize=sizeof(alphaHeader); alphaHeader.biWidth=900; alphaHeader.biHeight=-220;
    alphaHeader.biPlanes=1; alphaHeader.biBitCount=32;
    BITMAPFILEHEADER alphaFile{}; alphaFile.bfType=0x4d42; alphaFile.bfOffBits=sizeof(alphaFile)+sizeof(alphaHeader);
    alphaFile.bfSize=alphaFile.bfOffBits+900*220*4;
    std::string alphaBytes(reinterpret_cast<char*>(&alphaFile),sizeof(alphaFile));
    alphaBytes.append(reinterpret_cast<char*>(&alphaHeader),sizeof(alphaHeader));
    alphaBytes.append(reinterpret_cast<char*>(frame.pixels),900*220*4);
    check("key_style_offscreen_preview", WriteTextFile(CurrentDirectory()+L"key-styles-alpha.bmp",alphaBytes));
    gNotes.clear(); gConfig=beforeStyleTests; gSavedConfig=savedBeforeStyles;
    RebuildLaneConfig(); FillSettingsControls(gSettingsWnd); gUnapplied=false; gUnsaved=!SameConfig(gConfig,gSavedConfig);

    bool pagesOk = true, boundsOk = true;
    for (int i = 0; i < static_cast<int>(gPageControls.size()); ++i) {
        ShowSettingsPage(i);
        for (size_t page = 0; page < gPageControls.size(); ++page) for (HWND control : gPageControls[page]) {
            pagesOk = pagesOk && (((GetWindowLongPtrW(control, GWL_STYLE) & WS_VISIBLE) != 0) == (static_cast<int>(page) == i));
            RECT rect{}; GetWindowRect(control, &rect);
            MapWindowPoints(nullptr, gSettingsWnd, reinterpret_cast<POINT*>(&rect), 2);
            boundsOk = boundsOk && rect.left >= 18 && rect.top >= 78 && rect.right <= 702 && rect.bottom <= 480;
        }
    }
    check("tab_page_visibility", pagesOk);
    check("page_controls_within_bounds", boundsOk);
    bool textFits = true;
    HDC measureDc = GetDC(gSettingsWnd);
    HGDIOBJ previousFont = SelectObject(measureDc, gGuiFont);
    const auto savedSnapshot = gDifficultySnapshot;
    bool hudFits = true, ratingTextOk = true;
    // Measure the actual formatted values against the smallest supported HUD card.
    // Captions may ellipsize; the difficulty and NPS numbers must remain complete.
    SelectObject(measureDc, gHudSmallFont);
    for (auto profile : {pattern::RatingProfile::Osu4, pattern::RatingProfile::Osu7, pattern::RatingProfile::Osu8, pattern::RatingProfile::Osu10,
        pattern::RatingProfile::Aery5, pattern::RatingProfile::Generic}) {
        gDifficultySnapshot = {};
        gDifficultySnapshot.idle = false;
        gDifficultySnapshot.ready = true;
        gDifficultySnapshot.profile = profile;
        gDifficultySnapshot.rating = 24.6;
        gDifficultySnapshot.stars = 4.697745;
        gDifficultySnapshot.score = 100;
        const bool bms = profile == pattern::RatingProfile::Aery5;
        gDifficultySnapshot.outsideCalibration = bms ? 1 : 0;
        const auto value = DifficultyValue();
        SelectObject(measureDc, FitDifficultyFont(measureDc, value, 108, gHudSmallFont));
        RECT needed{0,0,108,31};
        DrawTextW(measureDc, value.c_str(), -1, &needed, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        hudFits = hudFits && needed.right <= 108 && needed.bottom <= 31;
        if (profile == pattern::RatingProfile::Aery5) ratingTextOk = ratingTextOk && value == L">⑤24.6";
        if (profile == pattern::RatingProfile::Osu8) ratingTextOk = ratingTextOk && value.find(L"단") != std::wstring::npos && value.find(L"★") == std::wstring::npos;
    }
    bool danFits = true;
    for (auto profile : {pattern::RatingProfile::Osu4, pattern::RatingProfile::Osu7, pattern::RatingProfile::Osu8, pattern::RatingProfile::Osu10}) {
        for (int sample = 0; sample <= 400; ++sample) {
            const auto value = pattern::FormatDan(profile, pattern::EstimateDan(profile, sample * (profile == pattern::RatingProfile::Osu10 ? 0.075 : 0.025)));
            for (int available : {165, 218, 350}) {
                const int height = available == 350 ? 51 : available == 218 ? 48 : 29;
                SelectObject(measureDc, FitDifficultyFont(measureDc, value, available, gHudValueFont, height));
                SIZE size{}; GetTextExtentPoint32W(measureDc, value.c_str(), static_cast<int>(value.size()), &size);
                if (size.cx > available && danFits) log << "dan_width=" << sample << "," << size.cx << ",available=" << available << "\n";
                danFits = danFits && size.cx <= available && size.cy <= height;
            }
            SetDifficultyText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_SCORE), value);
            SetDifficultyText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_PREVIOUS), value);
            for (int id : {IDC_DIFFICULTY_SCORE, IDC_DIFFICULTY_PREVIOUS}) {
                HWND control = GetDlgItem(gSettingsWnd, id);
                SelectObject(measureDc, reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0)));
                RECT bounds{}; GetClientRect(control, &bounds);
                RECT needed = bounds;
                auto text = WindowText(control);
                DrawTextW(measureDc, text.c_str(), -1, &needed, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                danFits = danFits && needed.right <= bounds.right && needed.bottom <= bounds.bottom;
            }
        }
    }
    check("dan_heights_and_ranges_fit_all_cards", danFits);
    for (int id : {IDC_DIFFICULTY_SCORE, IDC_DIFFICULTY_PREVIOUS}) {
        SetDifficultyText(GetDlgItem(gSettingsWnd, id), L"10단 수준");
        check(id == IDC_DIFFICULTY_SCORE ? "live_level_uses_large_font" : "completed_level_uses_large_font",
            reinterpret_cast<HFONT>(SendDlgItemMessageW(gSettingsWnd, id, WM_GETFONT, 0, 0)) == gHudValueFont);
    }
    check("compact_level_uses_larger_font", FitDifficultyFont(measureDc, L"10단 수준", 165, gHudCompactFont, 29) == gHudCompactFont);
    SelectObject(measureDc, gHudSmallFont);
    RECT npsNeeded{0,0,108,31};
    DrawTextW(measureDc, L"4096.0", -1, &npsNeeded, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    check("hud_numbers_fit_minimum_width", hudFits && npsNeeded.right <= 108 && npsNeeded.bottom <= 31);
    check("table_units_and_range_markers", ratingTextOk);
    gDifficultySnapshot.profile = pattern::RatingProfile::Aery5;
    gDifficultySnapshot.ratingError = 2.3;
    SetDifficultyText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_SCORE), DifficultyValue());
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_METRICS), DifficultyDetails(true).c_str());
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_NPS), L"NPS  4096.0");
    SetDifficultyText(GetDlgItem(gSettingsWnd, IDC_DIFFICULTY_PREVIOUS), L">⑤24.6");
    for (const auto& page : gPageControls) for (HWND control : page) {
        wchar_t className[32]{};
        GetClassNameW(control, className, 32);
        if (std::wstring(className) != L"STATIC" && std::wstring(className) != L"BUTTON") continue;
        HFONT controlFont = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
        SelectObject(measureDc, controlFont ? controlFont : gGuiFont);
        RECT available{}; GetClientRect(control, &available);
        RECT needed = available;
        if (std::wstring(className) == L"BUTTON") needed.right -= 24;
        auto label = WindowText(control);
        DrawTextW(measureDc, label.c_str(), -1, &needed, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        if (needed.bottom > available.bottom || needed.right > available.right) {
            textFits = false;
            log << "text_overflow_control=" << GetDlgCtrlID(control) << "\n";
        }
    }
    SelectObject(measureDc, previousFont);
    ReleaseDC(gSettingsWnd, measureDc);
    check("settings_text_fits_at_default_scale", textFits);
    HDC skillDc=GetDC(gSettingsWnd);
    auto skillOldFont=SelectObject(skillDc,gHudSmallFont);
    revive_aux::Summary maximumSkills;maximumSkills.ready=true;maximumSkills.skills.fill(100);
    bool skillFits=true;
    for (int i=0;i<6;++i) {
        const auto name=revive_aux::SkillName(i), value=revive_aux::SkillValue(maximumSkills,i);
        SelectObject(skillDc,gHudSmallFont);
        SIZE extent{};GetTextExtentPoint32W(skillDc,name.c_str(),static_cast<int>(name.size()),&extent);
        skillFits=skillFits && extent.cx<=90 && extent.cy<=24;
        SelectObject(skillDc,gHudCompactFont);
        GetTextExtentPoint32W(skillDc,value.c_str(),static_cast<int>(value.size()),&extent);
        skillFits=skillFits && extent.cx<=50 && extent.cy<=34;
        SelectObject(skillDc,gHudMediumFont);
        GetTextExtentPoint32W(skillDc,value.c_str(),static_cast<int>(value.size()),&extent);
        skillFits=skillFits && extent.cx<=108 && extent.cy<=34;
    }
    check("six_large_skill_cells_fit_at_minimum_width",skillFits);
    const auto lnFont=FitDifficultyFont(skillDc,L"3~4단 mid low",224,gHudMediumFont,36);
    check("ln_range_keeps_large_font_at_minimum_width",lnFont==gHudMediumFont || lnFont==gHudCompactFont);
    SelectObject(skillDc,skillOldFont);ReleaseDC(gSettingsWnd,skillDc);
    gReviveLive.Reset(10);
    for(int i=0;i<80;++i) {gReviveLive.Key(i%10,true,i*100);gReviveLive.Key(i%10,false,i*100+30);}
    gReviveLive.Update(8000,true,10,false,false,false);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {
        Sleep(2);gReviveLive.Update(8000,true,10,false,false,false);
    }
    check("live_cpp_six_skills_ready_for_hud",gReviveLive.Current().ready);
    // Render our own HUD into an off-screen bitmap for layout inspection. This
    // never captures the desktop, installs a hook, or sends keyboard input.
    const auto savedSummary = gPatternSummary;
    gPatternSummary.Reset();
    auto example = gDifficultySnapshot;
    example.rating = 12.3; example.outsideCalibration = 0;
    gPatternSummary.Observe(example); gPatternSummary.Observe(pattern::Snapshot{});
    gDifficultySnapshot.rating = 10.8;
    gDifficultySnapshot.outsideCalibration = 0;
    gDifficultySnapshot.currentNps = 48.0;
    auto renderHud = [&](int width, int height, const wchar_t* name, bool settings=false) {
        if (!settings) height = DifficultyHeaderHeight(width, height);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width; info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
        void* pixels = nullptr;
        HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        bool ok = dc && bitmap && pixels;
        if (ok) {
            HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
            if(settings) SendMessageW(gSettingsWnd,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
            else DrawDifficultyHud(dc, width, height);
            GdiFlush();
            BITMAPFILEHEADER header{};
            header.bfType = 0x4d42;
            header.bfOffBits = sizeof(header) + sizeof(info.bmiHeader);
            header.bfSize = header.bfOffBits + width * height * 4;
            std::string bytes(reinterpret_cast<const char*>(&header), sizeof(header));
            bytes.append(reinterpret_cast<const char*>(&info.bmiHeader), sizeof(info.bmiHeader));
            bytes.append(static_cast<const char*>(pixels), width * height * 4);
            // Settings and large-font HUD bitmaps can exceed the configuration
            // writer's 1MiB limit. These are owned test images, not config files.
            std::ofstream output(std::filesystem::path(CurrentDirectory()+name),std::ios::binary);
            output.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));output.close();ok=!!output;
            SelectObject(dc, oldBitmap);
        }
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
        return ok;
    };
    const auto previewPage = gCurrentPage;
    RECT settingsPreviewBounds{}; GetClientRect(gSettingsWnd, &settingsPreviewBounds);
    for (const auto& preview : {std::pair<int, const wchar_t*>{1, L"size-settings.bmp"},
            {2, L"spacing-settings.bmp"}, {3, L"level-settings.bmp"}}) {
        ShowSettingsPage(preview.first);
        check("size_spacing_level_settings_render", renderHud(settingsPreviewBounds.right, settingsPreviewBounds.bottom, preview.second, true));
    }
    ShowSettingsPage(previewPage);
#if KEYNOTE_OSU_MEMORY_LINK
    const auto priorPage=gCurrentPage;ShowSettingsPage(5);
    RECT settingsBounds{};GetClientRect(gSettingsWnd,&settingsBounds);
    check("osu_risk_settings_offscreen_render",renderHud(settingsBounds.right,settingsBounds.bottom,L"osu-risk-settings.bmp",true));
    ShowSettingsPage(priorPage);
#endif
    check("hud_offscreen_layout_render", renderHud(900,76,L"hud-wide.bmp") && renderHud(240,76,L"hud-compact.bmp") && renderHud(600,56,L"hud-low.bmp"));
#if KEYNOTE_OSU_MEMORY_LINK
    gOsuView.enabled=true;gOsuView.ready=true;gOsuView.keys=19;
    gOsuView.title=L"선택 곡 미리보기";gOsuView.variant=L"19K Unsupported";gOsuView.difficulty=L"⑤7.2";
    revive_aux::Summary unsupportedSkill;unsupportedSkill.status=L"1~18K 지원";
    gOsuView.reviveAuxiliary=revive_aux::Label(unsupportedSkill);
    gOsuView.reviveCompact=revive_aux::Label(unsupportedSkill,true);
    gOsuView.reviveSkills=revive_aux::Details(unsupportedSkill);
    gOsuView.revive=unsupportedSkill;
    check("osu_selected_chart_hud_render", DifficultyHeaderHeight(900,220)==222 &&
        renderHud(900,104,L"hud-osu-selection.bmp") && renderHud(240,104,L"hud-osu-selection-small.bmp"));
    check("osu_short_overlay_preserves_lanes",DifficultyHeaderHeight(240,90)==510 && OverlayPresentationHeight(240)>=554 && DifficultyHeaderHeight(900,90)==222 &&
        renderHud(240,76,L"hud-osu-short.bmp"));
    gOsuView.keys=10;gOsuView.variant=L"10K Synthetic";gOsuView.difficulty=L"10단 수준";
    gOsuView.status=L"연동 표시 검증 · 합성 채보 (실제 게임 연결 없음)";
    revive_aux::Summary previewSkills;previewSkills.ready=true;previewSkills.skills={50,70,82,30,60,91};
    gOsuView.reviveAuxiliary=revive_aux::Label(previewSkills);
    gOsuView.reviveCompact=revive_aux::Label(previewSkills,true);
    gOsuView.reviveSkills=revive_aux::Details(previewSkills);
    gOsuView.revive=previewSkills;
    check("osu_revive_auxiliary_hud_render", renderHud(900,104,L"hud-revive-wide.bmp") &&
        renderHud(240,104,L"hud-revive-compact.bmp") && renderHud(240,76,L"hud-revive-short.bmp") &&
        renderHud(600,56,L"hud-revive-low.bmp"));
    auto auxiliaryPreview=gOsuView;gOsuView={};ApplyOsuLinkView(auxiliaryPreview);
    ShowSettingsPage(5);
    check("osu_revive_auxiliary_settings_render",renderHud(settingsBounds.right,settingsBounds.bottom,L"revive-settings.bmp",true));
    ShowSettingsPage(priorPage);
    const bool liveEnabled=gConfig.difficultyEnabled;gConfig.difficultyEnabled=false;
    check("osu_hud_independent_of_live_input", DifficultyHeaderHeight(900,220)==82 && renderHud(900,28,L"hud-osu-only.bmp"));
    gConfig.difficultyEnabled=liveEnabled;
    const auto beforeLnPreview=gDifficultySnapshot;
    const auto beforeLnKeys=gAnalysisKeys;
    const auto beforeLnMode=gConfig.analysisKeyMode;
    const auto beforeLnSummary=gPatternSummary;
    gDifficultySnapshot.profile=pattern::RatingProfile::Osu4;gDifficultySnapshot.stars=3.0;
    gDifficultySnapshot.ratingError=0;
    gPatternSummary.Reset();gPatternSummary.Observe(gDifficultySnapshot);gPatternSummary.Observe(pattern::Snapshot{});
    gConfig.analysisKeyMode=4;gAnalysisKeys.Reset(4,4);gReviveLive.Reset(4);
    chartcalc::Chart fourFixture;fourFixture.keys=4;
    for(int i=0;i<32;++i) for(int lane=0;lane<4;++lane) {
        fourFixture.notes.push_back({lane,i*250.0,i*250.0+150});
    }
    for(int i=0;i<32;++i) {
        for(int lane=0;lane<4;++lane)gReviveLive.Key(lane,true,i*250);
        for(int lane=0;lane<4;++lane)gReviveLive.Key(lane,false,i*250+150);
    }
    gReviveLive.Update(8000,true,4,false,false,false);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);gReviveLive.Update(8000,true,4,false,false,false);}
    gOsuView.keys=4;gOsuView.title=L"4K LN v2 합성 표시";gOsuView.variant=L"LN comparison";
    gOsuView.difficulty=FormatDifficultyRating(gDifficultySnapshot);
    gOsuView.lnDifficulty=pattern::FormatLnDan(gDifficultySnapshot);
    gOsuView.revive=revive_aux::Analyze(fourFixture);
    gOsuView.reviveAuxiliary=revive_aux::Label(gOsuView.revive);
    gOsuView.reviveCompact=revive_aux::Label(gOsuView.revive,true);
    gOsuView.reviveSkills=revive_aux::Details(gOsuView.revive);
    const auto fourView=gOsuView;gOsuView={};ApplyOsuLinkView(fourView);
    check("four_key_selected_skill_settings",WindowText(GetDlgItem(gSettingsWnd,IDC_OSU_SKILLS))==revive_aux::Details(fourView.revive));
    check("four_key_live_and_file_cpp_values_render",gReviveLive.Current().ready && gOsuView.revive.ready &&
        gReviveLive.Current().skills[3]>0 && gReviveLive.Current().holdKnown &&
        revive_aux::SkillValue(gReviveLive.Current(),5)==L"0" && gOsuView.revive.holdKnown && gOsuView.revive.skills[5]>0);
    check("ln4_live_and_selected_rows_render",CurrentHudLayout(240).liveLnHeight==60 && CurrentHudLayout(240).selectedLnHeight==60 &&
        renderHud(240,90,L"hud-ln4-compact.bmp") && renderHud(900,220,L"hud-ln4-wide.bmp"));
    gDifficultySnapshot=beforeLnPreview;gAnalysisKeys=beforeLnKeys;
    gConfig.analysisKeyMode=beforeLnMode;gPatternSummary=beforeLnSummary;RefreshOsuLinkDisplay();
#else
    check("no_osu_memory_hud_keeps_live_layout",DifficultyHeaderHeight(900,220)==140 && DifficultyHeaderHeight(900,90)==140);
#endif
    const auto previewLanes = gConfig.lanes;
    gConfig.lanes.resize(4);
    gDifficultySnapshot.profile = pattern::RatingProfile::Osu4;
    gDifficultySnapshot.stars = 4.68;
    gDifficultySnapshot.ratingError = 0;
    gPatternSummary.Reset();
    gPatternSummary.Observe(gDifficultySnapshot); gPatternSummary.Observe(pattern::Snapshot{});
    check("dan_completed_height_matches_live", PreviousPatternValue() == L"8~9단 mid low");
    check("dan_hud_offscreen_layout_render", renderHud(900,76,L"hud-dan-wide.bmp") &&
        renderHud(240,76,L"hud-dan-compact.bmp") && renderHud(600,56,L"hud-dan-low.bmp"));
    gConfig.lanes.resize(8);
    gDifficultySnapshot.profile = pattern::RatingProfile::Osu8;
    gDifficultySnapshot.stars = 8.147425; // Life/Oblatus pooled band.
    gPatternSummary.Reset();
    gPatternSummary.Observe(gDifficultySnapshot); gPatternSummary.Observe(pattern::Snapshot{});
    gDifficultySnapshot.stars = 8.8; // Demise mid low.
    check("8k_live_and_completed_use_dan_labels", DifficultyValue() == L"Demise단 mid low" &&
        PreviousPatternValue() == L"13~14단 수준");
    check("8k_hud_offscreen_layout_render", renderHud(900,76,L"hud-8k-wide.bmp") &&
        renderHud(240,76,L"hud-8k-compact.bmp") && renderHud(600,56,L"hud-8k-low.bmp"));
    gConfig.lanes.resize(10);
    gDifficultySnapshot.profile = pattern::RatingProfile::Osu10;
    gDifficultySnapshot.stars = 5.58939;
    gDifficultySnapshot.rating = 16.3659; // Revive 2026 Dan 10 reference.
    gPatternSummary.Reset();
    gPatternSummary.Observe(gDifficultySnapshot); gPatternSummary.Observe(pattern::Snapshot{});
    gDifficultySnapshot.stars = 5.23186;
    gDifficultySnapshot.rating = 22.7947;
    check("10k_live_and_completed_use_revive_dans", DifficultyProfileName() == L"Revive 10K" &&
        DifficultyValue() == L"개전 수준" && PreviousPatternValue() == L"10단 수준");
    gDifficultySnapshot.rating = 25.668;
    check("10k_overjoy_label", DifficultyValue() == L"오버조이 수준");
    check("10k_revive_hud_offscreen_layout_render", renderHud(900,76,L"hud-10k-revive-wide.bmp") &&
        renderHud(240,76,L"hud-10k-revive-compact.bmp") && renderHud(600,56,L"hud-10k-revive-low.bmp"));
    gConfig.lanes = previewLanes;
    gPatternSummary = savedSummary;
    gDifficultySnapshot = savedSnapshot;
    // Hidden native windows exercise movement and real position-only persistence,
    // without injecting mouse input or changing the user's desktop configuration.
    const Config beforePositionTests = gConfig;
    gConfig.clickThrough = true;
    gConfig.debugBackground = false;
    FillSettingsControls(gSettingsWnd);
    check("position_initial_save", SaveSettings());
    const Config positionBaseline = gSavedConfig;
    gOverlayWnd = CreateWindowW(L"KeyNoteOverlayCppWindow", L"position test", WS_POPUP,
        gConfig.x, gConfig.y, gConfig.width, gConfig.height, nullptr, nullptr, gInstance, nullptr);
    ApplyOverlayWindowStyle();
    const Config beforeInitialPlacement = gConfig;
    check("startup_settings_placement_without_changing_overlay", PlaceInitialSettingsWindow() &&
        SameConfig(gConfig, beforeInitialPlacement) && !IsWindowVisible(gSettingsWnd) && !IsWindowVisible(gOverlayWnd));
    check("position_starts_click_through", IsWindow(gOverlayWnd) && !gMoveMode &&
        !IsWindowVisible(gOverlayWnd) && (GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) &&
        SendMessageW(gOverlayWnd, WM_NCHITTEST, 0, 0) == HTTRANSPARENT);
    SetWindowPos(gOverlayWnd, nullptr, -345, -67, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageW(gOverlayWnd, WM_EXITSIZEMOVE, 0, 0);
    check("programmatic_move_does_not_save", SameConfig(gConfig, positionBaseline) && SameConfig(gSavedConfig, positionBaseline));
    click(IDC_MOVE_MODE);
    check("move_mode_overrides_click_through_temporarily", gMoveMode && gConfig.clickThrough && !gConfig.debugBackground &&
        !(GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) &&
        SendMessageW(gOverlayWnd, WM_NCHITTEST, 0, 0) == HTCAPTION && !IsWindowVisible(gOverlayWnd));
    // One applied, unsaved value plus independent, invalid draft values.
    gConfig.opacityPercent = gConfig.opacityPercent == 72 ? 73 : 72;
    FillSettingsControls(gSettingsWnd);
    edit(IDC_WIDTH, L"invalid draft");
    edit(IDC_LANES, L"Q,W,E,R");
    click(IDC_VIS_NEON);
    const auto positionDraftPresets = gDraftPresetLanes;
    const int positionActiveOpacity = gConfig.opacityPercent;
    auto dragTo = [&](int x, int y) {
        SendMessageW(gOverlayWnd, WM_ENTERSIZEMOVE, 0, 0);
        SetWindowPos(gOverlayWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SendMessageW(gOverlayWnd, WM_EXITSIZEMOVE, 0, 0);
    };
    dragTo(-345, -67);
    check("drag_saves_negative_coordinates", gConfig.x == -345 && gConfig.y == -67 && gSavedConfig.x == -345 &&
        gSavedConfig.y == -67 && WindowText(GetDlgItem(gSettingsWnd, IDC_X)) == L"-345" &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_Y)) == L"-67");
    check("drag_preserves_active_and_invalid_drafts", gConfig.opacityPercent == positionActiveOpacity &&
        gSavedConfig.opacityPercent == positionBaseline.opacityPercent && gUnapplied && gUnsaved &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_WIDTH)) == L"invalid draft" && raw() == L"Q,W,E,R" &&
        gDraftVisualPreset == L"Neon" && gDraftPresetLanes == positionDraftPresets);
    const Config positionActive = gConfig;
    gConfig = Config{};
    const bool positionJsonLoaded = LoadJsonConfig();
    ClampAndRebuildConfig();
    check("position_json_restores_without_saving_other_edits", positionJsonLoaded && SameConfig(gConfig, gSavedConfig));
    gConfig = Config{}; LoadIniConfig(); ClampAndRebuildConfig();
    check("position_ini_restores_without_saving_other_edits", SameConfig(gConfig, gSavedConfig));
    gConfig = positionActive; RebuildLaneConfig();
    HANDLE positionLock = CreateFileW(gJsonConfigPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    const Config positionCommitted = gSavedConfig;
    SetStatus(L"no save expected");
    dragTo(-345, -67);
    check("stationary_drag_does_not_save", WindowText(GetDlgItem(gSettingsWnd, IDC_STATUS)) == L"no save expected");
    SendMessageW(gOverlayWnd, WM_ENTERSIZEMOVE, 0, 0);
    SetWindowPos(gOverlayWnd, nullptr, 99, 88, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(gOverlayWnd, nullptr, -345, -67, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageW(gOverlayWnd, WM_EXITSIZEMOVE, 0, 0);
    check("cancelled_move_does_not_save", WindowText(GetDlgItem(gSettingsWnd, IDC_STATUS)) == L"no save expected");
    dragTo(-222, 111);
    check("position_save_failure_retains_committed_position", positionLock != INVALID_HANDLE_VALUE &&
        SameConfig(gSavedConfig, positionCommitted) && gConfig.x == -222 && gConfig.y == 111 && gUnsaved &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_STATUS)).find(L"위치 저장 실패") != std::wstring::npos);
    if (positionLock != INVALID_HANDLE_VALUE) CloseHandle(positionLock);
    click(IDC_SAVE_POSITION);
    check("position_retry_ignores_unrelated_invalid_draft", gSavedConfig.x == -222 && gSavedConfig.y == 111 &&
        gSavedConfig.opacityPercent == positionBaseline.opacityPercent && raw() == L"Q,W,E,R" && gUnapplied);
    edit(IDC_X, L"bad x");
    check("position_invalid_coordinate_rejected", !SavePositionDraft() && gConfig.x == -222 && gSavedConfig.x == -222);
    edit(IDC_X, L"456"); edit(IDC_Y, L"234"); click(IDC_SAVE_POSITION);
    RECT movedRect{}; GetWindowRect(gOverlayWnd, &movedRect);
    check("position_button_moves_and_saves_only_coordinates", movedRect.left == 456 && movedRect.top == 234 &&
        gSavedConfig.x == 456 && gSavedConfig.y == 234 && gSavedConfig.width == positionBaseline.width && gUnapplied);
    SendMessageW(gSettingsWnd, WM_COMMAND, ID_TRAY_MOVE, 0);
    check("tray_move_toggle_restores_click_through", !gMoveMode && gConfig.clickThrough && !gConfig.debugBackground &&
        (GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) &&
        SendMessageW(gOverlayWnd, WM_NCHITTEST, 0, 0) == HTTRANSPARENT);
    gConfig.clickThrough = false; ApplyOverlayWindowStyle();
    check("non_click_through_overlay_is_directly_draggable", SendMessageW(gOverlayWnd, WM_NCHITTEST, 0, 0) == HTCAPTION);
    using Game = game_overlay::Game;
    check("game_executable_names_exact_and_case_insensitive",
        game_overlay::FromExecutable(L"C:\\Games\\OSU!.EXE") == Game::Osu &&
        game_overlay::FromExecutable(L"osu.exe") == Game::Osu &&
        game_overlay::FromExecutable(L"C:/Games/Qwilight.exe") == Game::Qwilight &&
        game_overlay::FromExecutable(L"not-osu!.exe") == Game::None &&
        game_overlay::FromExecutable(L"Qwilight.exe.backup") == Game::None &&
        game_overlay::FromExecutable(L"Quaver.exe") == Game::None);
    check("game_input_policy", !game_overlay::AllowsInput(true, Game::None, false) &&
        game_overlay::AllowsInput(true, Game::Osu, false) && game_overlay::AllowsInput(true, Game::Qwilight, false) &&
        game_overlay::AllowsInput(true, Game::None, true) && game_overlay::AllowsInput(false, Game::None, false));
    check("game_classifier_ignores_missing_and_own_window",
        gForegroundClassifier.Identify(nullptr) == Game::None && gForegroundClassifier.Identify(gSettingsWnd) == Game::None);
    FillSettingsControls(gSettingsWnd);
    SendDlgItemMessageW(gSettingsWnd, IDC_GAME_OVERLAY, BM_SETCHECK, BST_CHECKED, 0);
    click(IDC_GAME_OVERLAY);
    check("game_checkbox_is_draft", !gConfig.gameOverlay && gUnapplied);
    check("game_apply_and_save", SaveSettings() && gConfig.gameOverlay && gConfig.clickThrough &&
        gConfig.alwaysOnTop && !gConfig.debugBackground);
    const Config gameSaved = gConfig;
    gConfig = Config{};
    check("game_json_roundtrip", LoadJsonConfig() && (ClampAndRebuildConfig(), SameConfig(gConfig, gameSaved)));
    gConfig = Config{}; LoadIniConfig(); ClampAndRebuildConfig();
    check("game_ini_roundtrip", SameConfig(gConfig, gameSaved));
    gConfig.clickThrough = gConfig.alwaysOnTop = false;
    ApplyOverlayWindowStyle();
    const auto gameStyle = GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE);
    check("game_window_forces_topmost_clickthrough_noactivate", !IsWindowVisible(gOverlayWnd) &&
        (gameStyle & WS_EX_TOPMOST) && (gameStyle & WS_EX_TRANSPARENT) && (gameStyle & WS_EX_NOACTIVATE) &&
        (gameStyle & WS_EX_APPWINDOW) && !(gameStyle & WS_EX_TOOLWINDOW) &&
        SendMessageW(gOverlayWnd, WM_NCHITTEST, 0, 0) == HTTRANSPARENT &&
        SendMessageW(gOverlayWnd, WM_MOUSEACTIVATE, 0, 0) == MA_NOACTIVATE);
    DWORD captureAffinity = WDA_NONE;
    check("game_overlay_is_not_excluded_from_capture", GetWindowDisplayAffinity(gOverlayWnd, &captureAffinity) && captureAffinity == WDA_NONE);
    ToggleOverlayMoveMode();
    check("game_move_preview_is_draggable", gMoveMode && !(GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) &&
        SendMessageW(gOverlayWnd, WM_NCHITTEST, 0, 0) == HTCAPTION);
    ToggleOverlayMoveMode();
    const auto gameTime = NowMs();
    RefreshGameOverlay(Game::Osu, gameTime);
    gKeyDown[static_cast<UINT>(L'D')] = true;
    gDifficulty.KeyDown(0, gameTime);
    RefreshGameOverlay(Game::None, gameTime + 1);
    check("game_focus_loss_closes_held_notes", !gKeyDown[static_cast<UINT>(L'D')] && !gDifficulty.IsHeld(0) &&
        gNotes.empty() && gForegroundGame == Game::None);
    check("game_focus_loss_keeps_capture_styles_and_status", GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE) == gameStyle &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_GAME_STATUS)).find(L"표시 유지") != std::wstring::npos);
    gConfig.gameOverlay = false; ApplyOverlayWindowStyle();
    const auto obsStyle = GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE);
    check("obs_mode_restores_original_window_options", (obsStyle & WS_EX_APPWINDOW) &&
        !(obsStyle & (WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST)));
    DestroyWindow(gOverlayWnd);
    gConfig = beforePositionTests; RebuildLaneConfig(); FillSettingsControls(gSettingsWnd);
    check("position_only_draft_clears_dirty_after_save", SaveSettings() && (edit(IDC_X, L"765"), SavePositionDraft()) && !gUnapplied && !gUnsaved);
    // Exercise real filesystem failures in this isolated self-test directory.
    check("save_before_failure_tests", SaveSettings());
    std::string committedJson, committedIni, afterFailure;
    ReadTextFile(gJsonConfigPath, committedJson);
    ReadTextFile(gIniConfigPath, committedIni);
    const Config committedConfig = gSavedConfig;
    HANDLE lockedJson = CreateFileW(gJsonConfigPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    edit(IDC_X, L"321");
    const bool blockedSave = !SaveSettings();
    if (lockedJson != INVALID_HANDLE_VALUE) CloseHandle(lockedJson);
    check("locked_json_preserves_previous_save", lockedJson != INVALID_HANDLE_VALUE && blockedSave && gUnsaved &&
        SameConfig(gSavedConfig, committedConfig) && ReadTextFile(gJsonConfigPath, afterFailure) && afterFailure == committedJson);
    check("failed_primary_save_does_not_touch_ini", ReadTextFile(gIniConfigPath, afterFailure) && afterFailure == committedIni);
    const DWORD oldIniAttributes = GetFileAttributesW(gIniConfigPath.c_str());
    const bool readOnlySet = SetFileAttributesW(gIniConfigPath.c_str(), oldIniAttributes | FILE_ATTRIBUTE_READONLY) != FALSE;
    const bool primarySaved = SaveSettings();
    check("ini_failure_does_not_hide_saved_json", readOnlySet && primarySaved && gIniSaveFailed && !gUnsaved &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_STATUS)).find(L"호환 INI") != std::wstring::npos);
    check("failed_ini_replacement_preserves_old_file", ReadTextFile(gIniConfigPath, afterFailure) && afterFailure == committedIni);
    SetFileAttributesW(gIniConfigPath.c_str(), oldIniAttributes);
    const Config latestSaved = gSavedConfig;
    gConfig = Config{};
    check("saved_primary_restores_after_compatibility_failure", LoadJsonConfig() && (ClampAndRebuildConfig(), SameConfig(gConfig, latestSaved)) && gConfig.x == 321);
    check("save_retry_repairs_compatibility_copy", SaveSettings() && !gIniSaveFailed);
    gConfig = Config{};
    LoadIniConfig(); ClampAndRebuildConfig();
    check("atomic_ini_roundtrip_all_settings", SameConfig(gConfig, latestSaved));
    const std::wstring intactJsonPath = gJsonConfigPath;
    gJsonConfigPath = CurrentDirectory() + L"truncated.json";
    const bool truncatedWritten = WriteTextFile(gJsonConfigPath, "{\"overlay\":{\"width\":777},\"input\":{");
    gConfig = Config{};
    const bool jsonRejected = !LoadJsonConfig();
    if (jsonRejected) LoadIniConfig();
    ClampAndRebuildConfig();
    check("truncated_json_uses_intact_ini", truncatedWritten && jsonRejected && SameConfig(gConfig, latestSaved));
    gJsonConfigPath = intactJsonPath;
    WIN32_FIND_DATAW temporary{};
    HANDLE leftovers = FindFirstFileW((CurrentDirectory() + L"*.tmp.*").c_str(), &temporary);
    check("save_staging_files_cleaned_up", leftovers == INVALID_HANDLE_VALUE);
    if (leftovers != INVALID_HANDLE_VALUE) FindClose(leftovers);
    // A failure must leave the save indicator dirty, even if the active values are valid.
    gJsonConfigPath = CurrentDirectory() + L"missing-directory\\config.json";
    check("save_failure_visible", !SaveSettings() && gUnsaved && WindowText(GetDlgItem(gSettingsWnd, IDC_STATUS)).find(L"저장 실패") != std::wstring::npos);

    // Exercise the opt-in flow against isolated files, including a real worker
    // write failure. No keyboard hook or real input is installed by this test.
    // Validation earlier in this test can focus the settings window. Synthetic
    // presses must use the allowed practice page, independent of Windows focus.
    ShowSettingsPage(3);
    const auto reportTestRoot = std::filesystem::path(CurrentDirectory()) /
        (L"report-ui-" + std::to_wstring(GetTickCount64()));
    std::error_code reportSetupError;
    const bool reportRootCreated = std::filesystem::create_directory(reportTestRoot, reportSetupError);
    check("report_test_isolated_directory", reportRootCreated && !reportSetupError);
    if (!reportRootCreated || reportSetupError) return 1; // Never use another run's files.
    gJsonConfigPath = (reportTestRoot / L"config.json").wstring();
    gIniConfigPath = (reportTestRoot / L"config.ini").wstring();
    gConfig = Config{}; gConfig.lanesText = L"D,F,J,K";
    RebuildLaneConfig(); ResetDifficulty(4); FillSettingsControls(gSettingsWnd);
    RecordDifficultyInput(0, true, 1000); RecordDifficultyInput(0, false, 1010);
    RefreshDifficultyDisplay(12000);
    check("report_default_off_creates_nothing", !gConfig.reportAutoSave && !gReportRecorder.Active() &&
        gPendingReports.empty() && !std::filesystem::exists(ReportDirectory(), reportSetupError) && !reportSetupError);
    SendDlgItemMessageW(gSettingsWnd, IDC_REPORT_AUTO, BM_SETCHECK, BST_CHECKED, 0);
    click(IDC_REPORT_AUTO);
    check("report_checkbox_is_only_a_draft", !gConfig.reportAutoSave && gUnapplied);
    check("report_opt_in_apply_and_save", SaveSettings() && gConfig.reportAutoSave);
    gConfig = Config{};
    check("report_json_roundtrip", LoadJsonConfig() && gConfig.reportAutoSave);
    gConfig = Config{}; LoadIniConfig();
    check("report_ini_roundtrip", gConfig.reportAutoSave);
    ClampAndRebuildConfig(); ResetDifficulty(4); FillSettingsControls(gSettingsWnd);
    if (blockFirstReport) check("report_initial_failure_fixture", WriteTextFile(ReportDirectory().wstring(), "owned test fixture"));
    RecordDifficultyInput(0, true, 1000); RecordDifficultyInput(0, false, 1010);
    SpawnTestNotes();
    RefreshDifficultyDisplay(11009);
    check("report_waits_for_ten_seconds", gReportRecorder.Active() && gPendingReports.empty());
    RefreshDifficultyDisplay(11010); PollReportWrites(true);
    std::string reportHtml;
    std::error_code reportReadError;
    std::vector<std::filesystem::path> reportFiles;
    if (blockFirstReport) {
        // The old throwing directory_iterator aborted here if the first write
        // failed. Exercise the real failure, inspection and retry in that order.
        check("report_initial_failure_is_retained", gReportSaveFailed && gPendingReports.size() == 1);
        check("report_failed_directory_inspection_returns_error",
            !self_test_io::HtmlReports(ReportDirectory(), reportFiles, reportReadError) && reportReadError);
        std::error_code fixtureError;
        check("report_initial_failure_fixture_removed", std::filesystem::remove(ReportDirectory(), fixtureError) && !fixtureError);
        click(IDC_REPORT_RETRY); PollReportWrites(true);
        check("report_initial_failure_recovers", !gReportSaveFailed && gPendingReports.empty());
    }
    const bool reportsReadable = self_test_io::HtmlReports(ReportDirectory(), reportFiles, reportReadError);
    for (const auto& file : reportFiles) ReadTextFile(file.wstring(), reportHtml);
    check("report_autosaves_one_html_and_excludes_preview", reportsReadable && reportFiles.size() == 1 && !gReportRecorder.Active() &&
        gPendingReports.empty() && reportHtml.find("총 누름<span class='value'>1회") != std::string::npos);
    RefreshDifficultyDisplay(30000); PollReportWrites(true);
    check("report_idle_does_not_duplicate", gPendingReports.empty() && !gReportWrite.valid());
    // A file in place of the output directory deterministically denies creation.
    const auto goodJsonPath = gJsonConfigPath;
    const auto blocker = reportTestRoot / L"blocked";
    check("report_failure_fixture", WriteTextFile(blocker.wstring(), "keep"));
    gJsonConfigPath = (blocker / L"config.json").wstring();
    RecordDifficultyInput(1, true, 40000); RecordDifficultyInput(1, false, 40010);
    // Finish without a timer tick: the first key of the next session queues it.
    RecordDifficultyInput(2, true, 51000); RecordDifficultyInput(2, false, 51010);
    check("report_delayed_timer_keeps_sessions_separate", gPendingReports.size() == 1 && gReportRecorder.Active());
    PollReportWrites(true); RefreshReportStatus();
    check("report_failure_keeps_data_and_shows_retry", gReportSaveFailed && gPendingReports.size() == 1 &&
        IsWindowEnabled(GetDlgItem(gSettingsWnd, IDC_REPORT_RETRY)) &&
        WindowText(GetDlgItem(gSettingsWnd, IDC_REPORT_STATUS)).find(L"저장 실패") != std::wstring::npos);
    for (int i = 0; i < 4; ++i) {
        const double t = 70000 + i * 20000;
        RecordDifficultyInput(0, true, t); RecordDifficultyInput(0, false, t + 10);
    }
    check("report_queue_is_bounded_and_pauses", gPendingReports.size() == kMaxPendingReports && !gReportRecorder.Active());
    gJsonConfigPath = goodJsonPath;
    click(IDC_REPORT_RETRY); PollReportWrites(true);
    check("report_retry_saves_retained_sessions", !gReportSaveFailed && gPendingReports.empty());
    RecordDifficultyInput(0, true, 160000); RecordDifficultyInput(0, false, 160010);
    SendDlgItemMessageW(gSettingsWnd, IDC_REPORT_AUTO, BM_SETCHECK, BST_UNCHECKED, 0);
    check("report_opt_out_finishes_current_session", ReadSettingsControls() && !gConfig.reportAutoSave && !gReportRecorder.Active());
    PollReportWrites(true);
    RecordDifficultyInput(1, true, 170000); RecordDifficultyInput(1, false, 170010);
    check("report_opt_out_stops_new_recording", !gReportRecorder.Active() && gPendingReports.empty());
    gConfig = Config{};
    check("legacy_report_json_fixture", WriteTextFile(gJsonConfigPath, "{\"overlay\":{\"width\":640}}"));
    check("legacy_report_json_defaults_off", LoadJsonConfig() && !gConfig.reportAutoSave && !gConfig.gameOverlay);
    check("legacy_report_ini_fixture", WriteTextFile(gIniConfigPath, "[overlay]\nwidth=640\n"));
    gConfig = Config{}; LoadIniConfig();
    check("legacy_report_ini_defaults_off", !gConfig.reportAutoSave && !gConfig.gameOverlay);
    check("legacy_key_mode_defaults_registered", gConfig.analysisKeyMode == 0);
    gConfig = Config{};
    gConfig.lanesText = L"A,S,D,F,V,N,J,K,L,SEMICOLON";
    RebuildLaneConfig(); ResetDifficulty(10); FillSettingsControls(gSettingsWnd);
    gCurrentPage = 3;
    const auto registeredKeys = gConfig.lanesText;
    for (int i=0;i<80;++i) {
        RecordDifficultyInput(i%10,true,180000+i*100.);
        RecordDifficultyInput(i%10,false,180030+i*100.);
    }
    RefreshDifficultyDisplay(188000);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(188000);}
    check("accepted_live_input_reaches_cpp_meter",gReviveLive.Current().ready && gDifficultySnapshot.analysisKeys==10);
    ResetDifficulty(10);
    SendDlgItemMessageW(gSettingsWnd, IDC_ANALYSIS_KEYS, CB_SETCURSEL, 2, 0);
    SendMessageW(gSettingsWnd, WM_COMMAND, MAKEWPARAM(IDC_ANALYSIS_KEYS, CBN_SELCHANGE), 0);
    check("manual_key_mode_is_draft", gConfig.analysisKeyMode == 0 && gUnapplied);
    check("manual_key_mode_applies_without_layout_change", ReadSettingsControls() && gConfig.analysisKeyMode == 4 &&
        gConfig.lanesText == registeredKeys && gConfig.lanes.size() == 10);
    for (int i = 0; i < 64; ++i) {
        RecordDifficultyInput((i%4)*2, true, 200000+i*125.);
        RecordDifficultyInput((i%4)*2, false, 200010+i*125.);
    }
    RefreshDifficultyDisplay(208000);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(208000);}
    check("manual_four_key_cpp_live",gReviveLive.Current().ready);
    check("manual_key_mode_routes_live_profile", gDifficultySnapshot.ready && gDifficultySnapshot.analysisKeys == 4 &&
        gDifficultySnapshot.profile == pattern::RatingProfile::Osu4 && DifficultyProfileName() == L"Reform 4K");
    check("ln4_basis_follows_manual_four_key_analysis",!pattern::FormatLnDan(gDifficultySnapshot).empty() &&
        CurrentHudLayout(240).liveLnHeight==60 && WindowText(GetDlgItem(gSettingsWnd,IDC_DIFFICULTY_METRICS)).find(L"4K LN v2")!=std::wstring::npos);
    for (int mode : {4, -1}) {
        SendDlgItemMessageW(gSettingsWnd, IDC_ANALYSIS_KEYS, CB_SETCURSEL, mode == -1 ? 1 : 2, 0);
        check("analysis_key_mode_saved", SaveSettings());
        const auto savedKeys = gConfig;
        gConfig = Config{};
        check("analysis_key_mode_json_roundtrip", LoadJsonConfig() && gConfig.analysisKeyMode == mode);
        gConfig = Config{}; LoadIniConfig();
        check("analysis_key_mode_ini_roundtrip", gConfig.analysisKeyMode == mode);
        gConfig = savedKeys; RebuildLaneConfig(); ResetDifficulty(10); FillSettingsControls(gSettingsWnd);
    }
    for (int i = 0; i < 64; ++i) {
        RecordDifficultyInput((i%4)*2, true, 220000+i*125.);
        RecordDifficultyInput((i%4)*2, false, 220010+i*125.);
    }
    RefreshDifficultyDisplay(228000);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(228000);}
    check("automatic_four_key_cpp_live",gReviveLive.Current().ready);
    check("automatic_key_mode_locks_live_profile", gDifficultySnapshot.ready && !gDifficultySnapshot.keyModePending &&
        gDifficultySnapshot.analysisKeys == 4 && gConfig.lanes.size() == 10);
    const auto folded = CurrentLaneDisplay(gConfig.width);
    check("locked_subset_folds_four_lanes", folded.compact && folded.visible == 4 && folded.width < gConfig.width &&
        folded.Visible(0) && folded.Visible(2) && folded.Visible(4) && folded.Visible(6) && !folded.Visible(9));
    const int configuredWidth = gConfig.width;
    HWND foldWindow = CreateWindowExW(WS_EX_LAYERED, L"KeyNoteOverlayCppWindow", L"hidden folded lane test", WS_POPUP,
        0, 0, gConfig.width, gConfig.height, nullptr, nullptr, gInstance, nullptr);
    SyncOverlayDisplayWidth(foldWindow);
    RECT foldedRect{}; GetClientRect(foldWindow, &foldedRect);
    check("folding_changes_window_not_saved_width", foldWindow && foldedRect.right == folded.width && gConfig.width == configuredWidth);
    check("folding_respects_minimum_window_size", foldedRect.right>=650 && foldedRect.bottom>=300);
    const bool priorHud = gConfig.difficultyHud, priorBackground = gConfig.debugBackground, priorLanes = gConfig.laneBackground;
    gConfig.difficultyHud = gConfig.debugBackground = gConfig.laneBackground = false;
    Note foldedNote; foldedNote.lane = 6; foldedNote.label = gConfig.lanes[6].label; foldedNote.pressMs = 227900;
    foldedNote.color = RGB(255,0,0); foldedNote.opacity = 100;
    gNotes = {foldedNote};
    overlay::AlphaSurface foldedFrame(folded.width, gConfig.height);
    bool foldedRendered = RenderOverlay(foldedFrame, 228000);
    int minPixel = folded.width, maxPixel = -1;
    for (int y = 0; y < foldedFrame.height; ++y) for (int x = 0; x < foldedFrame.width; ++x)
        if (foldedFrame.pixels[y*foldedFrame.width+x] == 0xffff0000u) { minPixel = std::min(minPixel,x); maxPixel = std::max(maxPixel,x); }
    check("folded_note_keeps_color_width_and_physical_identity", foldedRendered && maxPixel >= minPixel &&
        std::abs((minPixel+maxPixel)/2.0-folded.Center(6,folded.width)) <= 1 &&
        maxPixel-minPixel+1 >= gConfig.noteWidth-4 && gNotes[0].lane == 6 && gNotes[0].label == gConfig.lanes[6].label);
    gNotes[0].pressMs = 227000; // The same sparse physical lane remains visible for a long hold.
    check("folded_long_note_renders", RenderOverlay(foldedFrame,228000));
    gNotes[0].lane = 9;
    check("unselected_lane_not_drawn_before_conflict", RenderOverlay(foldedFrame,228000) &&
        std::all_of(foldedFrame.pixels,foldedFrame.pixels+foldedFrame.width*foldedFrame.height,[](DWORD p){return p==0;}));
    gNotes.clear();
    gConfig.difficultyHud = priorHud; gConfig.debugBackground = priorBackground; gConfig.laneBackground = priorLanes;
    RecordDifficultyInput(9, true, 228100); RecordDifficultyInput(9, false, 228110);
    RefreshDifficultyDisplay(228110);
    check("extra_lane_clears_cpp_axes",!gReviveLive.Current().ready && gReviveLive.Current().status==L"키 추정 중");
    check("automatic_late_key_rechecks_and_preserves_input", !gDifficultySnapshot.ready && !gDifficultySnapshot.keyModeConflict &&
        gDifficultySnapshot.keyModePending && DifficultyValue() == L"키 보정 중" && gDifficultySnapshot.currentNps > 0);
    SyncOverlayDisplayWidth(foldWindow); GetClientRect(foldWindow, &foldedRect);
    check("extra_key_unfolds_all_lanes", !CurrentLaneDisplay(gConfig.width).compact && foldedRect.right == configuredWidth);
    if (foldWindow) DestroyWindow(foldWindow);
    for(int i=0;i<32;++i) {
        const int lanes[]={0,2,4,6,9};
        RecordDifficultyInput(lanes[i%5],true,228200+i*100.);
        RecordDifficultyInput(lanes[i%5],false,228230+i*100.);
    }
    RefreshDifficultyDisplay(231400);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(231400);}
    check("automatic_late_key_recovers_primary_and_auxiliary",gAnalysisKeys.Keys()==5 &&
        gDifficultySnapshot.ready && !gDifficultySnapshot.keyModeConflict && !gDifficultySnapshot.keyModePending &&
        gDifficultySnapshot.profile==pattern::RatingProfile::Aery5 && gReviveLive.Current().ready);
    RefreshDifficultyDisplay(241330);
    check("automatic_corrected_mode_can_complete",gPatternSummary.Completed().ready &&
        gPatternSummary.Completed().analysisKeys==5 && gCompletedSkills.Completed().ready);
    RecordDifficultyInput(0, true, 242000); RecordDifficultyInput(0, false, 242010);
    RefreshDifficultyDisplay(242010);
    check("automatic_new_session_restarts_inference", gDifficultySnapshot.keyModePending && !gDifficultySnapshot.keyModeConflict);
    for(int i=0;i<64;++i) {
        RecordDifficultyInput((i%4)*2,true,244000+i*125.);
        RecordDifficultyInput((i%4)*2,false,244010+i*125.);
    }
    RefreshDifficultyDisplay(252000);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(252000);}
    check("auto_unconfirmed_extra_fixture_has_valid_four_key_result",gAnalysisKeys.Keys()==4 && gDifficultySnapshot.ready && gReviveLive.Current().ready);
    RecordDifficultyInput(9,true,252100);RecordDifficultyInput(9,false,252110);
    // Deliberately skip UI refreshes until completion: input must block the old
    // candidate as soon as an unresolved extra lane is seen.
    RefreshDifficultyDisplay(262110);
    check("auto_unconfirmed_extra_key_never_completes_wrong_basis",gDifficultySnapshot.idle &&
        !gPatternSummary.Completed().ready && !gCompletedSkills.Completed().ready);
    SendDlgItemMessageW(gSettingsWnd, IDC_ANALYSIS_KEYS, CB_SETCURSEL, 0, 0);
    check("registered_key_mode_restores_legacy_basis", ReadSettingsControls() && gConfig.analysisKeyMode == 0 && DifficultyProfileName() == L"Revive 10K");
    const auto beforeAllKeys = gConfig;
    const wchar_t* customTokens[] = {L"A",L"S",L"D",L"F",L"G",L"H",L"J",L"K",L"L",L"Q",L"W",L"E",L"R",L"T",L"Y",L"U",L"I",L"O"};
    for (int keys=1; keys<=revive::MaxKeys; ++keys) {
        gConfig.lanesText.clear(); gConfig.analysisKeyMode=0;
        for(int lane=0;lane<keys;++lane) {if(lane)gConfig.lanesText+=L",";gConfig.lanesText+=customTokens[lane];}
        RebuildLaneConfig(); ResetDifficulty(keys);
        for(int i=0;i<80;++i) {
            RecordDifficultyInput(i%keys,true,260000+i*100.);
            RecordDifficultyInput(i%keys,false,260095+i*100.);
        }
        RefreshDifficultyDisplay(268000);
        for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(268000);}
        check("registered_1_to_18_keys_reach_live_auxiliary",gReviveLive.Current().ready && gDifficultySnapshot.analysisKeys==keys &&
            gReviveLive.Current().holdKnown && revive_aux::SkillValue(gReviveLive.Current(),5)==L"0");
        ResetDifficulty(keys);
        for(int i=0;i<20;++i) {
            RecordDifficultyInput(i%keys,true,270000+i*400.);
            RecordDifficultyInput(i%keys,false,270170+i*400.);
        }
        RefreshDifficultyDisplay(278000);
        for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(278000);}
        check("registered_1_to_18_keys_publish_170ms_hold",gReviveLive.Current().ready &&
            gReviveLive.Current().skills[5]>0 && revive_aux::SkillAvailable(gReviveLive.Current(),5));
        if(keys==5 || keys==7 || keys==9 || keys==18) {
            const auto name=L"hud-"+std::to_wstring(keys)+L"k-compact.bmp";
            check("other_keys_six_large_skills_render",renderHud(240,90,name.c_str()));
        }
    }
    // Reproduce the reported compact-HUD sequence using accepted input only:
    // play -> ten seconds idle -> a few typed keys -> another full play.
    gConfig.lanesText=L"D,F,J,K";gConfig.analysisKeyMode=0;gConfig.gameOverlay=false;gConfig.reportAutoSave=false;
    RebuildLaneConfig();ResetDifficulty(4);ShowSettingsPage(3);
#if KEYNOTE_OSU_MEMORY_LINK
    const auto beforeCompletionPreview=gOsuView;gOsuView={};
#endif
    for(int i=0;i<32;++i) {
        RecordDifficultyInput(i%4,true,300000+i*250.);
        RecordDifficultyInput(i%4,false,300170+i*250.);
    }
    RefreshDifficultyDisplay(308000);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(308000);}
    const auto lastLiveSkills=gReviveLive.Current();
    check("completion_fixture_has_six_axes_and_hold",lastLiveSkills.ready && lastLiveSkills.skills[5]>0);
    RefreshDifficultyDisplay(318000);
    const auto finishedSkills=gCompletedSkills.Completed();
    const auto finishedLevel=PreviousPatternValue(),finishedLn=HudLnValue(450);
    check("completed_input_keeps_all_six_axes",ShowsCompletedSkills() && finishedSkills.skills==lastLiveSkills.skills);
    check("completed_ln_visible_at_all_widths",!finishedLn.empty() && finishedLn.rfind(L"완료 ",0)==0 &&
        HudLnValue(240)==finishedLn && HudLnValue(599)==finishedLn && HudLnValue(600)==finishedLn);
    check("completed_hud_renders_compact_and_wide",renderHud(450,320,L"hud-completed-450.bmp") &&
        renderHud(240,320,L"hud-completed-240.bmp") && renderHud(900,320,L"hud-completed-900.bmp"));
    check("completed_hud_renders_at_new_minimum",renderHud(650,300,L"hud-completed-650.bmp"));
    RecordDifficultyInput(0,true,318100);RecordDifficultyInput(0,false,318130);
    RefreshDifficultyDisplay(318200);
    check("short_input_preserves_completed_display",!gDifficultySnapshot.idle && !gDifficultySnapshot.ready &&
        ShowsCompletedSkills() && gCompletedSkills.Completed().skills==finishedSkills.skills &&
        PreviousPatternValue()==finishedLevel && HudLnValue(450)==finishedLn);
    check("pending_input_hud_renders_completed_values",renderHud(450,320,L"hud-completed-pending-450.bmp"));
    RefreshDifficultyDisplay(328130);
    check("short_session_does_not_replace_completed_results",ShowsCompletedSkills() &&
        gCompletedSkills.Completed().skills==finishedSkills.skills && PreviousPatternValue()==finishedLevel);
    for(int i=0;i<32;++i) {
        RecordDifficultyInput(i%4,true,330000+i*200.);
        RecordDifficultyInput(i%4,false,330030+i*200.);
    }
    RefreshDifficultyDisplay(336400);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(336400);}
    const auto nextLiveSkills=gReviveLive.Current();
    check("valid_new_input_prefers_live_axes",nextLiveSkills.ready && !ShowsCompletedSkills() && nextLiveSkills.skills[5]==0);
    RefreshDifficultyDisplay(346230);
    check("next_completed_play_replaces_old_axes",ShowsCompletedSkills() &&
        gCompletedSkills.Completed().skills==nextLiveSkills.skills && gCompletedSkills.Completed().skills[5]==0);
    for(int i=0;i<32;++i) {
        RecordDifficultyInput(i%4,true,350000+i*200.);
        RecordDifficultyInput(i%4,false,350030+i*200.);
    }
    RefreshDifficultyDisplay(356400);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(356400);}
    const auto firstLiveSkills=gReviveLive.Current();
    for(int i=0;i<32;++i) {
        RecordDifficultyInput(i%4,true,356500+i*250.);
        RecordDifficultyInput(i%4,false,356670+i*250.);
    }
    RefreshDifficultyDisplay(364500);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(364500);}
    const auto updatedLiveSkills=gReviveLive.Current();
    check("live_axes_keep_updating_while_previous_result_exists",firstLiveSkills.ready && firstLiveSkills.skills[5]==0 &&
        updatedLiveSkills.ready && !ShowsCompletedSkills() && updatedLiveSkills.skills!=firstLiveSkills.skills &&
        updatedLiveSkills.skills[5]>0 && gCompletedSkills.Completed().skills==nextLiveSkills.skills);
    check("updated_live_hud_renders_at_new_minimum",renderHud(650,300,L"hud-live-updated-650.bmp"));
    // A peak can occur while the previous CPP result is still on screen.
    // Deliver the new result after the main rating enters its retained state.
    ResetDifficulty(4);
    for(int i=0;i<32;++i) {
        RecordDifficultyInput(i%4,true,380000+i*200.);
        RecordDifficultyInput(i%4,false,380030+i*200.);
    }
    RefreshDifficultyDisplay(386400);
    for(int retry=0;retry<200 && gReviveLive.Busy();++retry) {Sleep(2);RefreshDifficultyDisplay(386400);}
    const auto earlyAxes=gReviveLive.Current();
    const double earlyStrength=gDifficultySnapshot.rating;
    for(int i=0;i<16;++i) {
        for(int lane=0;lane<4;++lane)RecordDifficultyInput(lane,true,387000+i*400.);
        for(int lane=0;lane<4;++lane)RecordDifficultyInput(lane,false,387170+i*400.);
    }
    RefreshDifficultyDisplay(393200);
    const double laterStrength=gDifficultySnapshot.rating;
    check("completion_regression_queues_stronger_window_with_old_axes",earlyAxes.ready &&
        laterStrength>earlyStrength && gReviveLive.Busy() && gReviveLive.Current().skills==earlyAxes.skills);
    for(int retry=0;retry<200 && gReviveLive.Current().skills==earlyAxes.skills;++retry) {Sleep(2);RefreshDifficultyDisplay(394700);}
    const auto lateAxes=gReviveLive.Current();
    check("completion_regression_delivers_changed_axes_during_retention",gDifficultySnapshot.retained &&
        lateAxes.ready && lateAxes.skills!=earlyAxes.skills && lateAxes.skills[5]>0);
    RefreshDifficultyDisplay(403169);
    check("completion_waits_until_ten_seconds_after_release",!gDifficultySnapshot.idle && !gCompletedSkills.Completed().ready);
    RefreshDifficultyDisplay(403170);
    check("completion_uses_later_peak_axes_instead_of_first_result",ShowsCompletedSkills() &&
        gCompletedSkills.Completed().skills==lateAxes.skills);
    const auto beforeViewerFold=gConfig, savedBeforeViewerFold=gSavedConfig;
    const auto originalViewerWidth=OverlayPresentationWidth();
    const auto originalViewerHeight=OverlayPresentationHeight(originalViewerWidth);
    FillSettingsControls(gSettingsWnd);
    SendDlgItemMessageW(gSettingsWnd,IDC_KEY_VIEWER_FOLDED,BM_SETCHECK,BST_CHECKED,0);
    click(IDC_KEY_VIEWER_FOLDED);
    check("viewer_fold_is_opt_in_draft",!Config{}.keyViewerFolded && !gConfig.keyViewerFolded && gUnapplied);
    RecordDifficultyInput(0,true,403200);
    check("viewer_fold_apply_keeps_live_hold_and_completed_values",ReadSettingsControls() && KeyViewerFolded() &&
        gDifficulty.IsHeld(0) && gCompletedSkills.Completed().skills==lateAxes.skills);
    check("viewer_fold_enlarges_width_without_changing_saved_geometry",OverlayPresentationWidth()==FoldedHudPixels(originalViewerWidth) &&
        gConfig.width==beforeViewerFold.width && gConfig.height==beforeViewerFold.height &&
        gConfig.noteWidth==beforeViewerFold.noteWidth && gConfig.lanesText==beforeViewerFold.lanesText);
    check("viewer_fold_saves",SaveSettings());
    const auto foldedConfig=gConfig;
    gConfig=Config{};
    check("viewer_fold_json_roundtrip",LoadJsonConfig() && gConfig.keyViewerFolded && gConfig.width==foldedConfig.width);
    gConfig=Config{};LoadIniConfig();
    check("viewer_fold_ini_roundtrip",gConfig.keyViewerFolded && gConfig.height==foldedConfig.height);
    gConfig=foldedConfig;RebuildLaneConfig();
    RecordDifficultyInput(0,false,403300);
    gDifficultySnapshot={};gDifficultySnapshot.profile=pattern::RatingProfile::Osu4;
    gDifficultySnapshot.ready=true;gDifficultySnapshot.idle=false;gDifficultySnapshot.stars=gDifficultySnapshot.rating=4;
    gDifficultySnapshot.currentNps=32;
    gConfig.debugBackground=false;gConfig.gameOverlay=false;gMoveMode=false;
#if KEYNOTE_OSU_MEMORY_LINK
    const auto foldSelected=gOsuView;gOsuView={};
#endif
    for(int logicalWidth : {650,900}) {
        gConfig.width=logicalWidth;
        const int w=OverlayPresentationWidth(), h=OverlayPresentationHeight(w);
        check("viewer_fold_has_no_reserved_note_area",h==FoldedHudPixels(CurrentHudLayout(logicalWidth).total));
        overlay::AlphaSurface foldedFrame(w,h), emptyFrame(w,h);
        Note foldNote;foldNote.lane=0;foldNote.held=true;foldNote.pressMs=404000;foldNote.label=L"D";
        gNotes={foldNote};gConfig.laneBackground=true;
        const bool withNotes=RenderOverlay(foldedFrame,404100);
        gNotes.clear();gConfig.laneBackground=false;
        const bool withoutNotes=RenderOverlay(emptyFrame,404100);
        check("viewer_fold_hides_notes_keys_and_lane_background",withNotes && withoutNotes &&
            std::equal(foldedFrame.pixels,foldedFrame.pixels+w*h,emptyFrame.pixels));
        BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=w;info.biHeight=-h;info.biPlanes=1;info.biBitCount=32;
        BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info);header.bfSize=header.bfOffBits+w*h*4;
        std::ofstream image(std::filesystem::path(CurrentDirectory()+L"hud-viewer-fold-"+std::to_wstring(logicalWidth)+L".bmp"),std::ios::binary);
        image.write(reinterpret_cast<const char*>(&header),sizeof(header));image.write(reinterpret_cast<const char*>(&info),sizeof(info));
        image.write(reinterpret_cast<const char*>(foldedFrame.pixels),w*h*4);image.close();
        check("viewer_fold_preview_written",!!image);
        const int normalHeight=h;
        gMoveMode=true;
        check("viewer_fold_move_hint_has_its_own_space",OverlayPresentationHeight(w)==normalHeight+42);
        gMoveMode=false;
    }
    gConfig=foldedConfig;gConfig.difficultyHud=false;
    check("viewer_fold_respects_disabled_hud",!KeyViewerFolded() && OverlayPresentationWidth()==originalViewerWidth);
    gConfig=foldedConfig;gConfig.difficultyEnabled=false;
    check("viewer_fold_respects_disabled_analysis",!KeyViewerFolded());
    gConfig=foldedConfig;
    HWND viewerFoldWindow=CreateWindowW(L"KeyNoteOverlayCppWindow",L"fold test",WS_POPUP,
        0,0,originalViewerWidth,originalViewerHeight,nullptr,nullptr,gInstance,nullptr);
    RECT viewerFoldRect{};
    SyncOverlayDisplayWidth(viewerFoldWindow);GetClientRect(viewerFoldWindow,&viewerFoldRect);
    check("viewer_fold_resizes_native_surface",viewerFoldWindow && viewerFoldRect.right==OverlayPresentationWidth() &&
        viewerFoldRect.bottom==OverlayPresentationHeight(viewerFoldRect.right));
    gConfig.keyViewerFolded=false;SyncOverlayDisplayWidth(viewerFoldWindow);GetClientRect(viewerFoldWindow,&viewerFoldRect);
    check("viewer_unfold_restores_native_surface",viewerFoldRect.right==originalViewerWidth && viewerFoldRect.bottom==originalViewerHeight);
    if(viewerFoldWindow)DestroyWindow(viewerFoldWindow);
    gConfig=beforeViewerFold;gSavedConfig=savedBeforeViewerFold;RebuildLaneConfig();FillSettingsControls(gSettingsWnd);
    check("viewer_unfold_restores_original_geometry",!KeyViewerFolded() && OverlayPresentationWidth()==originalViewerWidth &&
        OverlayPresentationHeight(originalViewerWidth)==originalViewerHeight);
#if KEYNOTE_OSU_MEMORY_LINK
    gOsuView=foldSelected;
#endif
    ResetDifficulty(4);
    RefreshDifficultyDisplay(365000);
    check("explicit_reset_clears_completed_display",!ShowsCompletedSkills() && !gCompletedSkills.Completed().ready && HudLnValue(650).empty());
    auto mixedMainInput=[&](double tapMs) {
        ResetDifficulty(4);
        struct Input { double time; int lane; bool down; };
        std::vector<Input> input{{400000,3,true},{400010,3,false},{400100,0,true},{408000,0,false}};
        for(int i=0;i<96;++i) {
            const double time=400200+i*80.;
            input.push_back({time,1+i%3,true});input.push_back({time+tapMs,1+i%3,false});
        }
        std::sort(input.begin(),input.end(),[](const auto& a,const auto& b) {
            if(a.time!=b.time)return a.time<b.time;
            if(a.down!=b.down)return !a.down;
            return a.lane<b.lane;
        });
        std::size_t next=0;
        for(double now=400000;now<=408000;now+=100) {
            while(next<input.size() && input[next].time<=now) {
                const auto event=input[next++];RecordDifficultyInput(event.lane,event.down,event.time);
            }
            RefreshDifficultyDisplay(now);
        }
        const auto current=gDifficultySnapshot;
        RefreshDifficultyDisplay(418000);
        return std::pair<pattern::Snapshot,pattern::Snapshot>{current,gPatternSummary.Completed()};
    };
    const auto fastTaps=mixedMainInput(10),slowTaps=mixedMainInput(120);
    check("mixed_ln_slow_taps_do_not_inflate_main_or_completed_rating",fastTaps.first.ready && slowTaps.first.ready &&
        fastTaps.second.ready && slowTaps.second.ready && std::abs(fastTaps.first.stars-slowTaps.first.stars)<1e-10 &&
        std::abs(fastTaps.second.stars-slowTaps.second.stars)<1e-10);
    check("mixed_ln_top_and_ln_dan_match_short_tap_baseline",PreviousPatternValue()==FormatDifficultyRating(fastTaps.second) &&
        pattern::FormatLnDan(gPatternSummary.Completed())==pattern::FormatLnDan(fastTaps.second));
#if KEYNOTE_OSU_MEMORY_LINK
    gOsuView=beforeCompletionPreview;
#endif
    gConfig=beforeAllKeys; RebuildLaneConfig(); ResetDifficulty(static_cast<int>(gConfig.lanes.size()));
    // Remove only the fresh directory owned by this test invocation.
    if (reportRootCreated) {
        std::error_code cleanupError;
        check("report_test_files_cleaned", self_test_io::RemoveOwnedDirectory(reportTestRoot, CurrentDirectory(), cleanupError));
    }
    gReportStatus.clear();
    DestroyWindow(gSettingsWnd);
    gSettingsWnd = nullptr;
    DeleteObject(gGuiFont);
    DestroyHudFonts();
    log << "checks=" << checks << "\nfailures=" << failures << "\nresult=" << (failures ? "fail" : "ok") << "\n";
    return failures ? 1 : 0;
}

struct LifecycleTestState {
    int adds = 0, deletes = 0, versions = 0, prompts = 0;
    int choice = IDCANCEL;
    bool failAdd = false, failVersion = false, deleteOwnerAlive = false;
    int menuCalls = 0;
    UINT menuCommand = 0;
    HWND lastMenuOwner = nullptr;
    bool menuBoundaryValid = false;
} gLifecycleTest;

BOOL WINAPI TestTrackTrayMenu(HMENU menu, UINT flags, int, int, int, HWND owner, const RECT*) {
    ++gLifecycleTest.menuCalls;
    gLifecycleTest.lastMenuOwner = owner;
    RECT rect{}; GetWindowRect(owner, &rect);
    gLifecycleTest.menuBoundaryValid = IsWindow(owner) && owner != gSettingsWnd &&
        GetAncestor(owner, GA_ROOTOWNER) == owner &&
        (GetWindowLongPtrW(owner, GWL_EXSTYLE) & (WS_EX_TOOLWINDOW | WS_EX_TOPMOST)) ==
            (WS_EX_TOOLWINDOW | WS_EX_TOPMOST) &&
        rect.left == rect.right && rect.top == rect.bottom &&
        (flags & TPM_RETURNCMD) && GetMenuItemCount(menu) == 4 &&
        !IsWindowVisible(gSettingsWnd) && !MayRaiseGameOverlay(nullptr);
    // A tray callback can re-enter through TrackPopupMenu's nested loop.
    ShowTrayMenu(gSettingsWnd);
    return gLifecycleTest.menuCommand;
}

BOOL WINAPI TestNotifyIcon(DWORD action, PNOTIFYICONDATAW data) {
    if (action == NIM_ADD) { ++gLifecycleTest.adds; return !gLifecycleTest.failAdd; }
    if (action == NIM_SETVERSION) { ++gLifecycleTest.versions; return !gLifecycleTest.failVersion; }
    if (action == NIM_DELETE) {
        ++gLifecycleTest.deletes;
        gLifecycleTest.deleteOwnerAlive = IsWindow(data->hWnd) != FALSE;
    }
    return TRUE;
}

int WINAPI TestExitMessageBox(HWND, LPCWSTR, LPCWSTR, UINT) {
    ++gLifecycleTest.prompts;
    // A second close while the save dialog is open must not open another one.
    RequestApplicationClose();
    return gLifecycleTest.choice;
}

struct NativeMenuProbe {
    bool visibleTopmost = false, foregroundOwner = false, trackingOwner = false, overlayRaiseBlocked = false;
} gNativeMenuProbe;

BOOL CALLBACK InspectNativeTrayMenu(HWND window, LPARAM) {
    wchar_t name[32]{};
    GetClassNameW(window, name, ARRAYSIZE(name));
    if (wcscmp(name, L"#32768") == 0 && IsWindowVisible(window))
        gNativeMenuProbe.visibleTopmost = (GetWindowLongPtrW(window, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    return TRUE;
}

void CALLBACK FinishNativeMenuProbe(HWND owner, UINT, UINT_PTR timer, DWORD) {
    KillTimer(owner, timer);
    EnumThreadWindows(GetCurrentThreadId(), InspectNativeTrayMenu, 0);
    gNativeMenuProbe.foregroundOwner = GetForegroundWindow() == owner;
    GUITHREADINFO info{}; info.cbSize = sizeof(info);
    gNativeMenuProbe.trackingOwner = GetGUIThreadInfo(GetCurrentThreadId(), &info) && info.hwndMenuOwner == owner;
    gConfig.gameOverlay = true;
    gNextGameRaiseMs = 0;
    RefreshGameOverlay(game_overlay::Game::None, NowMs());
    gNativeMenuProbe.overlayRaiseBlocked = gNextGameRaiseMs == 0 && !IsWindowVisible(gOverlayWnd);
    EndMenu(); // Close our own menu; no global mouse/keyboard input is generated.
}

BOOL WINAPI ProbeNativeTrayMenu(HMENU menu, UINT flags, int x, int y, int reserved, HWND owner, const RECT* rect) {
    const UINT_PTR timer = SetTimer(owner, 1, 150, FinishNativeMenuProbe);
    if (!timer) return 0;
    const BOOL result = TrackPopupMenu(menu, flags, x, y, reserved, owner, rect);
    KillTimer(owner, timer);
    return result;
}

int RunLifecycleSelfTest(bool realTray, bool nativeMenu = false) {
    gUiSelfTest = true;
    InitCommonControls();
    gGuiFont = CreateSettingsFont();
    if (!RegisterWindows()) return 1;
    gNotifyIcon = realTray ? &Shell_NotifyIconW : &TestNotifyIcon;
    gExitMessageBox = &TestExitMessageBox;
    std::ofstream log(realTray ? "key_note_cpp_tray_smoke.log" : "key_note_cpp_lifecycle_selftest.log", std::ios::trunc);
    log << std::unitbuf;
    int checks = 0, failures = 0;
    auto check = [&](const char* name, bool ok) {
        log << name << "=" << (ok ? "ok" : "FAIL") << '\n'; ++checks; if (!ok) ++failures;
    };
    auto create = [&]() {
        MSG quit{};
        while (PeekMessageW(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE)) {}
        gShuttingDown = gCloseInProgress = gAppRunning = false;
        gTrayMenuActive = gSettingsForegroundBoost = false;
        gUnapplied = gUnsaved = false;
        gLifecycleTest = {};
        gConfig = Config{};
        RebuildLaneConfig(); gSavedConfig = gConfig;
        gJsonConfigPath = CurrentDirectory() + L"lifecycle.json";
        gIniConfigPath = CurrentDirectory() + L"lifecycle.ini";
        gConfigPath = gJsonConfigPath;
        gOverlayWnd = CreateWindowW(L"KeyNoteOverlayCppWindow", L"hidden lifecycle overlay", WS_POPUP,
            0, 0, 900, 220, nullptr, nullptr, gInstance, nullptr);
        gSettingsWnd = CreateWindowW(L"KeyNoteOverlayCppSettings", L"hidden lifecycle settings", WS_OVERLAPPEDWINDOW,
            0, 0, 760, 680, nullptr, nullptr, gInstance, nullptr);
        gAppRunning = IsWindow(gOverlayWnd) && IsWindow(gSettingsWnd);
        return gAppRunning;
    };
    auto stopped = [&](HWND overlay, HWND settings) {
        MSG quit{};
        return gShuttingDown && !gAppRunning && !gTrayAdded && !gKeyboardHook &&
            !gOverlayWnd && !gSettingsWnd && !IsWindow(overlay) && !IsWindow(settings) &&
            PeekMessageW(&quit, nullptr, WM_QUIT, WM_QUIT, PM_NOREMOVE);
    };
    check("hidden_window_pair", create() && !IsWindowVisible(gOverlayWnd) && !IsWindowVisible(gSettingsWnd));
    HWND overlay = gOverlayWnd, settings = gSettingsWnd;
    if (realTray) {
        // Opt-in smoke only: registers a real icon, never a keyboard hook or a
        // visible window. Query shell state before and after normal app exit.
        check("real_tray_add", AddTrayIcon());
        NOTIFYICONIDENTIFIER id{};
        id.cbSize = sizeof(id); id.hWnd = settings; id.uID = TRAY_ICON_ID;
        RECT iconRect{};
        check("real_shell_icon_present", SUCCEEDED(Shell_NotifyIconGetRect(&id, &iconRect)));
        if (nativeMenu) {
            // Explicit desktop smoke only. A real popup is shown for 150 ms;
            // the ordinary CTest path never shows or focuses a desktop window.
            gUiSelfTest = false;
            gTrackTrayMenu = &ProbeNativeTrayMenu;
            ShowTrayMenu(settings);
            gTrackTrayMenu = &TrackPopupMenu;
            gUiSelfTest = true;
            check("native_popup_visible_topmost", gNativeMenuProbe.visibleTopmost);
            check("native_popup_tracking_owner", gNativeMenuProbe.trackingOwner);
            // This synthetic invocation is not a user tray click. Windows may
            // deny activation; record that fact rather than bypass focus rules.
            log << "native_popup_foreground_granted=" << (gNativeMenuProbe.foregroundOwner ? "yes" : "no") << '\n';
            check("native_popup_timer_cannot_raise_overlay", gNativeMenuProbe.overlayRaiseBlocked);
            check("native_popup_leaves_settings_hidden", !IsWindowVisible(settings) && !gTrayMenuActive);
        }
        SendMessageW(overlay, WM_CLOSE, 0, 0);
        check("real_exit_closes_both_windows", stopped(overlay, settings));
        check("real_shell_icon_removed", FAILED(Shell_NotifyIconGetRect(&id, &iconRect)));
    } else {
        check("tray_add_once", AddTrayIcon() && AddTrayIcon() && gLifecycleTest.adds == 1 && gLifecycleTest.versions == 1);
        gTrackTrayMenu = &TestTrackTrayMenu;
        ShowTrayMenu(settings);
        check("tray_popup_independent_topmost_owner", gLifecycleTest.menuBoundaryValid && gLifecycleTest.menuCalls == 1);
        check("tray_cancel_cleans_owner_and_raise_guard", !IsWindow(gLifecycleTest.lastMenuOwner) &&
            !gTrayMenuActive && MayRaiseGameOverlay(nullptr) && !IsWindowVisible(settings));
        // Model a minimized settings owner without showing any desktop window.
        const LONG_PTR settingsStyle = GetWindowLongPtrW(settings, GWL_STYLE);
        SetWindowLongPtrW(settings, GWL_STYLE, settingsStyle | WS_MINIMIZE);
        gLifecycleTest.menuCommand = ID_TRAY_SETTINGS;
        ShowTrayMenu(settings);
        MSG menuCommand{};
        check("tray_minimized_owner_dispatches_settings", gLifecycleTest.menuBoundaryValid &&
            gLifecycleTest.menuCalls == 2 && IsIconic(settings) &&
            PeekMessageW(&menuCommand, settings, WM_COMMAND, WM_COMMAND, PM_REMOVE) &&
            menuCommand.wParam == ID_TRAY_SETTINGS && !IsWindow(gLifecycleTest.lastMenuOwner));
        SetWindowLongPtrW(settings, GWL_STYLE, settingsStyle);
        gTrackTrayMenu = &TrackPopupMenu;
        HWND ownedDialog = CreateWindowExW(0, L"STATIC", L"", WS_POPUP,
            0, 0, 0, 0, settings, nullptr, gInstance, nullptr);
        check("settings_and_owned_dialog_block_overlay_raise", ownedDialog &&
            !MayRaiseGameOverlay(settings) && !MayRaiseGameOverlay(GetDlgItem(settings, IDC_TABS)) &&
            !MayRaiseGameOverlay(ownedDialog) && MayRaiseGameOverlay(overlay));
        SendMessageW(settings, WM_ACTIVATE, WA_ACTIVE, 0);
        check("settings_activation_enters_topmost_band", gSettingsForegroundBoost &&
            (GetWindowLongPtrW(settings, GWL_EXSTYLE) & WS_EX_TOPMOST));
        SendMessageW(settings, WM_ACTIVATE, WA_INACTIVE, reinterpret_cast<LPARAM>(ownedDialog));
        check("settings_owned_dialog_keeps_boost", gSettingsForegroundBoost);
        SendMessageW(settings, WM_ACTIVATEAPP, FALSE, 0);
        check("switching_apps_removes_settings_topmost", !gSettingsForegroundBoost &&
            !(GetWindowLongPtrW(settings, GWL_EXSTYLE) & WS_EX_TOPMOST));
        SendMessageW(settings, WM_ACTIVATE, WA_ACTIVE, 0);
        SendMessageW(settings, WM_ACTIVATE, WA_INACTIVE, reinterpret_cast<LPARAM>(overlay));
        check("settings_deactivation_removes_boost", !gSettingsForegroundBoost &&
            !(GetWindowLongPtrW(settings, GWL_EXSTYLE) & WS_EX_TOPMOST));
        DestroyWindow(ownedDialog);
        check("tray_v4_mouse_keyboard_and_id", TrayEvent(0, MAKELPARAM(NIN_SELECT,TRAY_ICON_ID)) == NIN_SELECT &&
            TrayEvent(0, MAKELPARAM(NIN_KEYSELECT,TRAY_ICON_ID)) == NIN_KEYSELECT &&
            TrayEvent(0, MAKELPARAM(WM_CONTEXTMENU,TRAY_ICON_ID)) == WM_CONTEXTMENU && TrayEvent(0, MAKELPARAM(NIN_SELECT,99)) == 0);
        SendMessageW(settings, gTaskbarCreatedMessage, 0, 0);
        check("explorer_restart_reregisters_once", gTrayAdded && gLifecycleTest.adds == 2 && gLifecycleTest.versions == 2);
        gUnsaved = true;
        gLifecycleTest.choice = IDCANCEL;
        SendMessageW(overlay, WM_CLOSE, 0, 0);
        check("overlay_close_cancel_preserves_app", IsWindow(overlay) && IsWindow(settings) && gTrayAdded &&
            !gShuttingDown && !gCloseInProgress && gLifecycleTest.prompts == 1 && gLifecycleTest.deletes == 0);
        gLifecycleTest.choice = IDYES;
        gJsonConfigPath = CurrentDirectory() + L"missing-directory\\config.json";
        SendMessageW(settings, WM_COMMAND, ID_TRAY_EXIT, 0);
        check("tray_exit_save_failure_preserves_app", gUnsaved && IsWindow(overlay) && IsWindow(settings) &&
            gTrayAdded && !gShuttingDown && !gCloseInProgress);
        gJsonConfigPath = CurrentDirectory() + L"lifecycle.json";
        SendMessageW(settings, WM_COMMAND, ID_TRAY_EXIT, 0);
        check("tray_save_exit_closes_everything", stopped(overlay, settings) && !gUnsaved && !gUnapplied);
        check("tray_deleted_before_owner_destroyed", gLifecycleTest.deletes == 1 && gLifecycleTest.deleteOwnerAlive);
        ShutdownApplication(); RemoveTrayIcon();
        check("cleanup_is_idempotent", gLifecycleTest.deletes == 1);

        check("restart_hidden_pair", create());
        overlay = gOverlayWnd; settings = gSettingsWnd;
        gLifecycleTest.failAdd = true;
        check("tray_add_failure_does_not_claim_success", !AddTrayIcon() && !gTrayAdded);
        gLifecycleTest.failAdd = false; gLifecycleTest.failVersion = true;
        SendMessageW(settings, WM_TIMER, TIMER_TRAY_RETRY, 0);
        check("tray_retry_and_legacy_callback", gTrayAdded && !gTrayVersion4 && gLifecycleTest.adds == 2 &&
            TrayEvent(TRAY_ICON_ID, WM_LBUTTONUP) == WM_LBUTTONUP && TrayEvent(99, WM_LBUTTONUP) == 0);
        gUnsaved = true; gLifecycleTest.choice = IDNO;
        SendMessageW(overlay, WM_CLOSE, 0, 0);
        check("overlay_discard_exit_closes_everything", stopped(overlay, settings) && gLifecycleTest.deletes == 1);

        check("settings_close_setup", create() && AddTrayIcon());
        overlay = gOverlayWnd; settings = gSettingsWnd;
        SendMessageW(settings, WM_CLOSE, 0, 0);
        check("settings_close_closes_everything", stopped(overlay, settings) && gLifecycleTest.deletes == 1);

        check("destroy_path_setup", create() && AddTrayIcon());
        overlay = gOverlayWnd; settings = gSettingsWnd;
        DestroyWindow(overlay);
        check("unexpected_overlay_destroy_closes_settings", stopped(overlay, settings) && gLifecycleTest.deletes == 1);

        check("session_end_setup", create() && AddTrayIcon());
        overlay = gOverlayWnd; settings = gSettingsWnd;
        SendMessageW(settings, WM_ENDSESSION, FALSE, 0);
        check("cancelled_session_end_keeps_app", IsWindow(overlay) && IsWindow(settings) && gTrayAdded);
        SendMessageW(settings, WM_ENDSESSION, TRUE, 0);
        check("session_end_cleans_up", stopped(overlay, settings) && gLifecycleTest.deletes == 1);

        check("report_close_setup", create() && AddTrayIcon());
        const auto reportCloseRoot = std::filesystem::path(CurrentDirectory()) /
            (L"report-close-" + std::to_wstring(GetTickCount64()));
        std::error_code closeError;
        const bool closeRootCreated = std::filesystem::create_directory(reportCloseRoot, closeError);
        check("report_close_isolated_directory", closeRootCreated && !closeError);
        if (!closeRootCreated || closeError) return 1;
        gJsonConfigPath = (reportCloseRoot / L"config.json").wstring();
        gConfig.reportAutoSave = true;
        gReportWaitForFreshSession = false;
        const double start = NowMs();
        RecordDifficultyInput(0, true, start); RecordDifficultyInput(0, false, start + 1);
        gUnsaved = true; gLifecycleTest.choice = IDCANCEL;
        SendMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
        check("report_cancelled_close_keeps_active_session", gReportRecorder.Active() &&
            !std::filesystem::exists(ReportDirectory(), closeError) && !closeError);
        gLifecycleTest.choice = IDNO;
        SendMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
        check("report_normal_close_saves_partial_session", !gReportRecorder.Active() && gPendingReports.empty() &&
            !gReportSaveFailed && std::filesystem::is_directory(ReportDirectory(), closeError) && !closeError);

        check("report_failed_close_setup", create() && AddTrayIcon());
        const auto blocker = reportCloseRoot / L"blocked";
        check("report_failed_close_fixture", WriteTextFile(blocker.wstring(), "keep"));
        gJsonConfigPath = (blocker / L"config.json").wstring();
        gConfig.reportAutoSave = true;
        const double failedStart = NowMs();
        RecordDifficultyInput(1, true, failedStart); RecordDifficultyInput(1, false, failedStart + 1);
        gLifecycleTest.choice = IDNO;
        SendMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
        check("report_failed_close_can_return_without_loss", IsWindow(gSettingsWnd) && !gShuttingDown &&
            gReportSaveFailed && gPendingReports.size() == 1);
        gJsonConfigPath = (reportCloseRoot / L"config.json").wstring();
        SendMessageW(gSettingsWnd, WM_COMMAND, IDC_REPORT_RETRY, 0);
        SendMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
        check("report_close_after_retry_saves_pending", !IsWindow(gSettingsWnd) && gPendingReports.empty() && !gReportSaveFailed);
        if (closeRootCreated)
            check("report_close_files_cleaned", self_test_io::RemoveOwnedDirectory(reportCloseRoot, CurrentDirectory(), closeError));
    }
    CleanupApplication();
    log << "checks=" << checks << "\nfailures=" << failures << "\nresult=" << (failures ? "fail" : "ok") << '\n';
    return failures ? 1 : 0;
}

int RunSelfTest() {
    gConfig.lanesText = L"A,S,D,F,J,K,L,SEMICOLON";
    gConfig.mode = L"8K";
    RebuildLaneConfig();

    UINT semicolonVk = 0;
    std::wstring semicolonLabel;
    bool semicolonOk = TokenToVk(L"SEMICOLON", semicolonVk, semicolonLabel) && semicolonVk == VK_OEM_1;
    bool laneCountOk = gConfig.lanes.size() == 8;
    bool registeredOk = gVkToLaneIndex[static_cast<UINT>(L'A')] >= 0;
    bool unregisteredFiltered = gVkToLaneIndex[static_cast<UINT>(L'Q')] < 0;

    gConfig.lanesText = kPresets[0].lanes;
    RebuildLaneConfig();
    bool fiveKeyOk = gConfig.lanes.size() == 5 && gVkToLaneIndex[VK_SPACE] >= 0;

    gConfig.lanesText = kPresets[2].lanes;
    RebuildLaneConfig();
    bool tenKeyOk = gConfig.lanes.size() == 10 && gVkToLaneIndex[static_cast<UINT>(L'V')] >= 0 && gVkToLaneIndex[static_cast<UINT>(L'N')] >= 0;

    gConfig.lanesText = L"q,w,semicolon,lbracket,comma";
    gConfig.mode = DetectModeForLanes(gConfig.lanesText);
    RebuildLaneConfig();
    gConfig.lanesText = CanonicalLaneText();
    bool canonicalOk = gConfig.lanesText == L"Q,W,SEMICOLON,LBRACKET,COMMA";

    Note hold;
    hold.pressMs = 1000.0;
    hold.releaseMs = 1250.0;
    hold.held = false;
    bool holdLifecycleOk = hold.releaseMs - hold.pressMs >= kHoldThresholdMs;

    gChordAnchorMs = -1000000.0;
    double chordA = AlignChordPressMs(1000.0);
    double chordB = AlignChordPressMs(1015.0);
    double chordC = AlignChordPressMs(1015.1);
    bool chordAlignOk = std::abs(chordA - 1000.0) < 0.001 && std::abs(chordB - 1000.0) < 0.001 && std::abs(chordC - 1015.1) < 0.001;

    ApplyVisualPreset(L"Neon");
    bool visualPresetOk = gConfig.visualPreset == L"Neon" && gConfig.circleNotes && gConfig.laneBackground && gConfig.roundedBoxes && gConfig.thinOutline;
    gConfig.opacityPercent = 73;
    gConfig.laneSpacingPercent = 123;
    gConfig.noteWidth = 77;
    gConfig.noteHeight = 41;
    gConfig.longNoteAlpha = 64;
    gConfig.keyTextBottom = false;
    gJsonConfigPath = CurrentDirectory() + L"key_note_cpp_selftest.json";
    gIniConfigPath = CurrentDirectory() + L"key_note_cpp_selftest.ini";
    gConfigPath = gJsonConfigPath;
    bool saveOk = SaveConfig();
    bool saveReadbackOk = ReadIniString(L"input", L"lanes", L"") == gConfig.lanesText;
    bool circleReadbackOk = ReadIniBool(L"overlay", L"circle_notes", false);
    bool visualReadbackOk = ReadIniString(L"overlay", L"visual_preset", L"") == L"Neon" &&
                            ReadIniInt(L"overlay", L"opacity_percent", 0) == 73 &&
                            ReadIniInt(L"overlay", L"lane_spacing_percent", 0) == 123 &&
                            ReadIniInt(L"overlay", L"note_width", 0) == 77 &&
                            ReadIniInt(L"overlay", L"note_height", 0) == 41 &&
                            ReadIniInt(L"overlay", L"long_note_alpha", 0) == 64 &&
                            ReadIniBool(L"overlay", L"lane_background", false) &&
                            ReadIniBool(L"overlay", L"rounded_boxes", false) &&
                            ReadIniBool(L"overlay", L"thin_outline", false) &&
                            !ReadIniBool(L"overlay", L"key_text_bottom", true);
    std::string jsonText;
    std::string jsonPreset;
    int jsonInt = 0;
    std::vector<std::string> jsonLanes;
    bool jsonReadbackOk = ReadTextFile(gJsonConfigPath, jsonText) &&
                          JsonReadString(jsonText, "visual_preset", jsonPreset) && jsonPreset == "Neon" &&
                          JsonReadInt(jsonText, "opacity_percent", jsonInt) && jsonInt == 73 &&
                          JsonReadInt(jsonText, "lane_spacing_percent", jsonInt) && jsonInt == 123 &&
                          JsonReadInt(jsonText, "note_width", jsonInt) && jsonInt == 77 &&
                          JsonReadInt(jsonText, "note_height", jsonInt) && jsonInt == 41 &&
                          JsonReadStringArray(jsonText, "lanes", jsonLanes) && jsonLanes.size() == 5 && jsonLanes[2] == "SEMICOLON";
    Config savedConfig = gConfig;
    gConfig = Config{};
    bool jsonLoadOk = LoadJsonConfig();
    ClampAndRebuildConfig();
    jsonLoadOk = jsonLoadOk &&
                 gConfig.visualPreset == L"Neon" &&
                 gConfig.opacityPercent == 73 &&
                 gConfig.laneSpacingPercent == 123 &&
                 gConfig.noteWidth == 77 &&
                 gConfig.noteHeight == 41 &&
                 gConfig.lanesText == L"Q,W,SEMICOLON,LBRACKET,COMMA";
    gConfig = savedConfig;
    RebuildLaneConfig();

    std::ofstream log("key_note_cpp_selftest.log", std::ios::out | std::ios::trunc);
    log << "lane_count_8k=" << (laneCountOk ? 8 : 0) << "\n";
    log << "semicolon_vk=" << semicolonVk << "\n";
    log << "A_registered=" << (registeredOk ? 1 : 0) << "\n";
    log << "Q_filtered=" << (unregisteredFiltered ? 1 : 0) << "\n";
    log << "five_key_ok=" << (fiveKeyOk ? 1 : 0) << "\n";
    log << "ten_key_ok=" << (tenKeyOk ? 1 : 0) << "\n";
    log << "canonical_ok=" << (canonicalOk ? 1 : 0) << "\n";
    log << "save_ok=" << (saveOk ? 1 : 0) << "\n";
    log << "save_readback_ok=" << (saveReadbackOk ? 1 : 0) << "\n";
    log << "hold_lifecycle_ok=" << (holdLifecycleOk ? 1 : 0) << "\n";
    log << "circle_readback_ok=" << (circleReadbackOk ? 1 : 0) << "\n";
    log << "visual_preset_ok=" << (visualPresetOk ? 1 : 0) << "\n";
    log << "visual_readback_ok=" << (visualReadbackOk ? 1 : 0) << "\n";
    log << "json_readback_ok=" << (jsonReadbackOk ? 1 : 0) << "\n";
    log << "json_load_ok=" << (jsonLoadOk ? 1 : 0) << "\n";
    log << "chord_align_ok=" << (chordAlignOk ? 1 : 0) << "\n";
    bool ok = semicolonOk && laneCountOk && registeredOk && unregisteredFiltered && fiveKeyOk && tenKeyOk && canonicalOk && saveOk && saveReadbackOk && holdLifecycleOk && circleReadbackOk && visualPresetOk && visualReadbackOk && jsonReadbackOk && jsonLoadOk && chordAlignOk;
    log << "result=" << (ok ? "ok" : "fail") << "\n";

    return ok ? 0 : 1;
}

// An unattended self-test should report a normal failure, not invoke terminate
// or a Windows crash dialog. Keep this boundary separate from normal app startup.
template<class Test> int RunCheckedSelfTest(const char* name, Test test) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try { return test(); }
    catch (const std::exception& error) {
        std::ofstream log("key_note_cpp_selftest_exception.log", std::ios::app);
        log << std::unitbuf << "test=" << name << "\nexception=" << error.what() << "\nresult=fail\nexit_code=2\n";
    }
    catch (...) {
        std::ofstream log("key_note_cpp_selftest_exception.log", std::ios::app);
        log << "test=" << name << "\nexception=unknown\nresult=fail\nexit_code=2\n";
    }
    return 2;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    gInstance = instance;
    const std::wstring command = GetCommandLineW();
    if (command.find(L"--self-test-exception") != std::wstring::npos) {
        return RunCheckedSelfTest("exception-boundary", []() -> int {
            throw std::filesystem::filesystem_error("intentional self-test fixture", std::make_error_code(std::errc::permission_denied));
        });
    }
    if (command.find(L"--self-test") != std::wstring::npos) return RunCheckedSelfTest("core", RunSelfTest);
    if (command.find(L"--ui-self-test") != std::wstring::npos)
        return RunCheckedSelfTest("ui", [&]() { return RunUiSelfTest(command.find(L"--report-output-blocked") != std::wstring::npos); });
    if (command.find(L"--lifecycle-self-test") != std::wstring::npos)
        return RunCheckedSelfTest("lifecycle", [] { return RunLifecycleSelfTest(false); });
    if (command.find(L"--tray-smoke-test") != std::wstring::npos)
        return RunCheckedSelfTest("tray", [] { return RunLifecycleSelfTest(true); });
    if (command.find(L"--tray-menu-smoke-test") != std::wstring::npos)
        return RunCheckedSelfTest("tray-menu", [] { return RunLifecycleSelfTest(true, true); });

    InitCommonControls();
    LoadConfig();
    gConfig.lanesText = CanonicalLaneText();
    gSavedConfig = gConfig;
    ResetDifficulty(static_cast<int>(gConfig.lanes.size()));

    gGuiFont = CreateSettingsFont();
    gNoteFont = CreateFontW(
        18,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    gKeyFont = CreateFontW(
        12,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    if (!RegisterWindows() || !CreateAppWindows(showCommand)) {
        MessageBoxW(nullptr, L"Failed to create Key Note Overlay C++ windows.", L"Key Note Overlay C++", MB_ICONERROR);
        CleanupApplication();
        return 1;
    }
    if (!AddTrayIcon()) SetStatus(L"트레이 등록을 기다리고 있어요. 실행 중 자동으로 다시 시도합니다.");

    gKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(nullptr), 0);
    if (!gKeyboardHook) {
        MessageBoxW(gSettingsWnd, L"Failed to install global keyboard hook.", L"Key Note Overlay C++", MB_ICONERROR);
    }

    SpawnTestNotes();

    MSG msg{};
    BOOL messageResult = 0;
    while ((messageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        if (HandleCaptureMessage(msg)) continue;
        if (HandleDifficultyTestMessage(msg)) continue;
        if (IsDialogMessageW(gSettingsWnd, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CleanupApplication();
    return messageResult < 0 ? 1 : static_cast<int>(msg.wParam);
}
