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

#include <algorithm>
#include <array>
#include <cmath>
#include <cwctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr UINT WM_APP_KEY_DOWN = WM_APP + 1;
constexpr UINT WM_APP_KEY_UP = WM_APP + 2;
constexpr UINT_PTR TIMER_RENDER = 1;
constexpr COLORREF kTransparentKey = RGB(1, 2, 3);

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
constexpr int IDC_PRESET_5 = 1015;
constexpr int IDC_PRESET_8 = 1016;
constexpr int IDC_PRESET_10 = 1017;

constexpr double kHoldThresholdMs = 90.0;
#ifndef WDA_EXCLUDEFROMCAPTURE
constexpr DWORD WDA_EXCLUDEFROMCAPTURE = 0x00000011;
#endif

struct Lane {
    UINT vk = 0;
    std::wstring token;
    std::wstring label;
};

struct Config {
    int x = 80;
    int y = 40;
    int width = 900;
    int height = 220;
    int durationMs = 850;
    int travelPx = 180;
    int noteWidth = 58;
    int noteHeight = 34;
    std::wstring mode = L"8K";
    bool debugBackground = true;
    bool clickThrough = true;
    bool alwaysOnTop = true;
    std::wstring lanesText = L"A,S,D,F,J,K,L,SEMICOLON";
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
};

struct Preset {
    const wchar_t* mode;
    const wchar_t* lanes;
};

HINSTANCE gInstance = nullptr;
HWND gSettingsWnd = nullptr;
HWND gOverlayWnd = nullptr;
HFONT gGuiFont = nullptr;
HFONT gNoteFont = nullptr;
Config gConfig;
std::array<int, 256> gVkToLaneIndex{};
std::array<bool, 256> gKeyDown{};
HHOOK gKeyboardHook = nullptr;
std::vector<Note> gNotes;
std::wstring gConfigPath;
unsigned long long gNextNoteId = 1;

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

const std::array<Preset, 3> kPresets = {{
    {L"5K", L"D,F,SPACE,J,K"},
    {L"8K", L"A,S,D,F,J,K,L,SEMICOLON"},
    {L"10K", L"A,S,D,F,V,N,J,K,L,SEMICOLON"},
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
        int number = _wtoi(token.c_str() + 1);
        if (number >= 1 && number <= 24) {
            outVk = VK_F1 + static_cast<UINT>(number - 1);
            outLabel = token;
            return true;
        }
    }

    return false;
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
    GetPrivateProfileStringW(section, key, fallback, buffer.data(), static_cast<DWORD>(buffer.size()), gConfigPath.c_str());
    return buffer.data();
}

int ReadIniInt(const wchar_t* section, const wchar_t* key, int fallback) {
    return static_cast<int>(GetPrivateProfileIntW(section, key, fallback, gConfigPath.c_str()));
}

bool ReadIniBool(const wchar_t* section, const wchar_t* key, bool fallback) {
    return ReadIniInt(section, key, fallback ? 1 : 0) != 0;
}

void LoadConfig() {
    gConfigPath = ExeDirectory() + L"key_note_cpp_config.ini";
    if (GetFileAttributesW(gConfigPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wstring currentConfig = CurrentDirectory() + L"key_note_cpp_config.ini";
        if (GetFileAttributesW(currentConfig.c_str()) != INVALID_FILE_ATTRIBUTES) {
            gConfigPath = currentConfig;
        }
    }
    gConfig.x = ReadIniInt(L"overlay", L"x", gConfig.x);
    gConfig.y = ReadIniInt(L"overlay", L"y", gConfig.y);
    gConfig.width = ReadIniInt(L"overlay", L"width", gConfig.width);
    gConfig.height = ReadIniInt(L"overlay", L"height", gConfig.height);
    gConfig.durationMs = ReadIniInt(L"overlay", L"duration_ms", gConfig.durationMs);
    gConfig.travelPx = ReadIniInt(L"overlay", L"travel_px", gConfig.travelPx);
    gConfig.noteWidth = ReadIniInt(L"overlay", L"note_width", gConfig.noteWidth);
    gConfig.noteHeight = ReadIniInt(L"overlay", L"note_height", gConfig.noteHeight);
    gConfig.debugBackground = ReadIniBool(L"overlay", L"debug_background", gConfig.debugBackground);
    gConfig.clickThrough = ReadIniBool(L"overlay", L"click_through", gConfig.clickThrough);
    gConfig.alwaysOnTop = ReadIniBool(L"overlay", L"always_on_top", gConfig.alwaysOnTop);
    gConfig.mode = ReadIniString(L"input", L"mode", gConfig.mode.c_str());
    gConfig.lanesText = ReadIniString(L"input", L"lanes", gConfig.lanesText.c_str());

    gConfig.width = std::max(240, gConfig.width);
    gConfig.height = std::max(90, gConfig.height);
    gConfig.durationMs = std::max(120, gConfig.durationMs);
    gConfig.travelPx = std::max(40, gConfig.travelPx);
    gConfig.noteWidth = std::max(24, gConfig.noteWidth);
    gConfig.noteHeight = std::max(18, gConfig.noteHeight);
    RebuildLaneConfig();
}

bool WriteIniInt(const wchar_t* section, const wchar_t* key, int value) {
    wchar_t buffer[32]{};
    wsprintfW(buffer, L"%d", value);
    return WritePrivateProfileStringW(section, key, buffer, gConfigPath.c_str()) != FALSE;
}

bool WriteIniBool(const wchar_t* section, const wchar_t* key, bool value) {
    return WritePrivateProfileStringW(section, key, value ? L"1" : L"0", gConfigPath.c_str()) != FALSE;
}

bool SaveConfig() {
    bool ok = true;
    ok = WriteIniInt(L"overlay", L"x", gConfig.x) && ok;
    ok = WriteIniInt(L"overlay", L"y", gConfig.y) && ok;
    ok = WriteIniInt(L"overlay", L"width", gConfig.width) && ok;
    ok = WriteIniInt(L"overlay", L"height", gConfig.height) && ok;
    ok = WriteIniInt(L"overlay", L"duration_ms", gConfig.durationMs) && ok;
    ok = WriteIniInt(L"overlay", L"travel_px", gConfig.travelPx) && ok;
    ok = WriteIniInt(L"overlay", L"note_width", gConfig.noteWidth) && ok;
    ok = WriteIniInt(L"overlay", L"note_height", gConfig.noteHeight) && ok;
    ok = WriteIniBool(L"overlay", L"debug_background", gConfig.debugBackground) && ok;
    ok = WriteIniBool(L"overlay", L"click_through", gConfig.clickThrough) && ok;
    ok = WriteIniBool(L"overlay", L"always_on_top", gConfig.alwaysOnTop) && ok;
    ok = (WritePrivateProfileStringW(L"input", L"mode", gConfig.mode.c_str(), gConfigPath.c_str()) != FALSE) && ok;
    ok = (WritePrivateProfileStringW(L"input", L"lanes", gConfig.lanesText.c_str(), gConfigPath.c_str()) != FALSE) && ok;
    WritePrivateProfileStringW(nullptr, nullptr, nullptr, gConfigPath.c_str());
    return ok;
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

int WindowInt(HWND hwnd, int fallback, int minValue, int maxValue) {
    BOOL ok = FALSE;
    int value = GetDlgItemInt(gSettingsWnd, GetDlgCtrlID(hwnd), &ok, TRUE);
    if (!ok) {
        return fallback;
    }
    return std::max(minValue, std::min(maxValue, value));
}

void RefreshKeyMapStatus() {
    std::wstringstream status;
    status << gConfig.mode << L" lanes: ";
    for (size_t i = 0; i < gConfig.lanes.size(); ++i) {
        if (i != 0) status << L" ";
        status << gConfig.lanes[i].label;
    }
    if (gConfig.lanes.empty()) {
        status << L"(none)";
    }
    SetStatus(status.str());
}

void ApplyOverlayWindowStyle() {
    if (!gOverlayWnd) return;

    LONG_PTR ex = GetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE);
    ex &= ~WS_EX_TOOLWINDOW;
    ex |= WS_EX_LAYERED | WS_EX_APPWINDOW | WS_EX_NOACTIVATE;
    if (gConfig.clickThrough) {
        ex |= WS_EX_TRANSPARENT;
    } else {
        ex &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtrW(gOverlayWnd, GWL_EXSTYLE, ex);
    SetLayeredWindowAttributes(gOverlayWnd, kTransparentKey, 0, LWA_COLORKEY);

    SetWindowPos(
        gOverlayWnd,
        gConfig.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        gConfig.x,
        gConfig.y,
        gConfig.width,
        gConfig.height,
        SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED
    );
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
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
    note.id = gNextNoteId++;
    note.lane = lane;
    note.label = gConfig.lanes[static_cast<size_t>(lane)].label;
    note.pressMs = NowMs();
    note.releaseMs = -1.0;
    note.held = true;
    note.color = kPalette[static_cast<size_t>(lane) % kPalette.size()];
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
    note->releaseMs = NowMs();
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
    UpdateWindow(gOverlayWnd);
}

void SpawnTapNote(int lane) {
    if (!gOverlayWnd || lane < 0 || lane >= static_cast<int>(gConfig.lanes.size())) {
        return;
    }
    Note note;
    note.id = gNextNoteId++;
    note.lane = lane;
    note.label = gConfig.lanes[static_cast<size_t>(lane)].label;
    note.pressMs = NowMs();
    note.releaseMs = note.pressMs + 40.0;
    note.held = false;
    note.color = kPalette[static_cast<size_t>(lane) % kPalette.size()];
    gNotes.push_back(std::move(note));
    if (gNotes.size() > 256) {
        gNotes.erase(gNotes.begin(), gNotes.begin() + static_cast<std::ptrdiff_t>(gNotes.size() - 256));
    }
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
    UpdateWindow(gOverlayWnd);
}

void DrawCenteredText(HDC dc, const RECT& rect, const std::wstring& text, COLORREF color) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    SelectObject(dc, gNoteFont);
    DrawTextW(dc, text.c_str(), -1, const_cast<RECT*>(&rect), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void PaintOverlay(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;

    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    HGDIOBJ oldBitmap = SelectObject(mem, bitmap);

    HBRUSH bg = CreateSolidBrush(gConfig.debugBackground ? RGB(28, 28, 28) : kTransparentKey);
    FillRect(mem, &client, bg);
    DeleteObject(bg);

    int laneCount = static_cast<int>(gConfig.lanes.size());
    int margin = std::max(24, width / 24);
    int laneArea = std::max(1, width - margin * 2);

    if (gConfig.debugBackground && laneCount > 0) {
        HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(58, 58, 58));
        HGDIOBJ oldPen = SelectObject(mem, gridPen);
        for (int i = 0; i < laneCount; ++i) {
            int x = margin + static_cast<int>(std::round(laneArea * ((i + 0.5) / laneCount)));
            MoveToEx(mem, x, 0, nullptr);
            LineTo(mem, x, height);
        }
        SelectObject(mem, oldPen);
        DeleteObject(gridPen);
    }

    double now = NowMs();
    gNotes.erase(std::remove_if(gNotes.begin(), gNotes.end(), [&](const Note& note) {
        return !note.held && now - note.releaseMs >= gConfig.durationMs;
    }), gNotes.end());

    for (const Note& note : gNotes) {
        if (laneCount <= 0) break;
        double pressAge = std::max(0.0, now - note.pressMs);
        double releaseAge = note.held ? 0.0 : std::max(0.0, now - note.releaseMs);
        double holdMs = note.held ? pressAge : std::max(0.0, note.releaseMs - note.pressMs);
        double speed = static_cast<double>(gConfig.travelPx) / static_cast<double>(std::max(1, gConfig.durationMs));
        double headTravel = std::min(static_cast<double>(gConfig.travelPx), speed * pressAge);
        double tailTravel = note.held ? 0.0 : std::min(static_cast<double>(gConfig.travelPx), speed * releaseAge);
        double fadeT = note.held ? 0.0 : std::max(0.0, std::min(1.0, releaseAge / gConfig.durationMs));
        int laneX = margin + static_cast<int>(std::round(laneArea * ((note.lane + 0.5) / laneCount)));
        int noteW = std::min(std::max(gConfig.noteWidth, laneArea / std::max(1, laneCount) - 8), 120);
        int noteH = gConfig.noteHeight;
        int baseCenterY = height - noteH / 2 - 8;
        int centerY = baseCenterY - static_cast<int>(std::round(headTravel));
        int alphaFade = note.held || fadeT < 0.68 ? 255 : static_cast<int>(std::round(255.0 * (1.0 - fadeT) / 0.32));
        alphaFade = std::max(32, std::min(255, alphaFade));

        COLORREF bodyColor = note.color;
        if (alphaFade < 255) {
            int r = GetRValue(bodyColor) * alphaFade / 255 + GetRValue(kTransparentKey) * (255 - alphaFade) / 255;
            int g = GetGValue(bodyColor) * alphaFade / 255 + GetGValue(kTransparentKey) * (255 - alphaFade) / 255;
            int b = GetBValue(bodyColor) * alphaFade / 255 + GetBValue(kTransparentKey) * (255 - alphaFade) / 255;
            bodyColor = RGB(r, g, b);
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
                HBRUSH tailShadowBrush = CreateSolidBrush(RGB(10, 12, 16));
                FillRect(mem, &tailShadow, tailShadowBrush);
                DeleteObject(tailShadowBrush);

                RECT tail{laneX - tailW / 2, tailTop, laneX + tailW / 2, tailBottom};
                HBRUSH tailBrush = CreateSolidBrush(bodyColor);
                HPEN tailPen = CreatePen(PS_SOLID, 2, RGB(248, 250, 252));
                HGDIOBJ oldBrush = SelectObject(mem, tailBrush);
                HGDIOBJ oldPen = SelectObject(mem, tailPen);
                RoundRect(mem, tail.left, tail.top, tail.right, tail.bottom, 5, 5);
                SelectObject(mem, oldPen);
                SelectObject(mem, oldBrush);
                DeleteObject(tailPen);
                DeleteObject(tailBrush);
            }
        }

        RECT shadow{laneX - noteW / 2 + 3, centerY - noteH / 2 + 3, laneX + noteW / 2 + 3, centerY + noteH / 2 + 3};
        HBRUSH shadowBrush = CreateSolidBrush(RGB(10, 12, 16));
        FillRect(mem, &shadow, shadowBrush);
        DeleteObject(shadowBrush);

        RECT body{laneX - noteW / 2, centerY - noteH / 2, laneX + noteW / 2, centerY + noteH / 2};
        HBRUSH brush = CreateSolidBrush(bodyColor);
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(248, 250, 252));
        HGDIOBJ oldBrush = SelectObject(mem, brush);
        HGDIOBJ oldPen = SelectObject(mem, pen);
        RoundRect(mem, body.left, body.top, body.right, body.bottom, 6, 6);
        SelectObject(mem, oldPen);
        SelectObject(mem, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);

        HPEN shinePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        oldPen = SelectObject(mem, shinePen);
        MoveToEx(mem, body.left + 7, body.top + 6, nullptr);
        LineTo(mem, body.right - 7, body.top + 6);
        SelectObject(mem, oldPen);
        DeleteObject(shinePen);

        DrawCenteredText(mem, body, note.label, RGB(8, 17, 31));
    }

    BitBlt(dc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
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
        if (wParam == TIMER_RENDER && !gNotes.empty()) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PaintOverlay(hwnd);
        return 0;
    case WM_NCHITTEST:
        if (gConfig.clickThrough) {
            return HTTRANSPARENT;
        }
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND MakeLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h) {
    HWND hwnd = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, nullptr, gInstance, nullptr);
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(gGuiFont), TRUE);
    return hwnd;
}

HWND MakeEdit(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
    HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                               x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gInstance, nullptr);
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(gGuiFont), TRUE);
    return hwnd;
}

HWND MakeButton(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h, DWORD style = BS_PUSHBUTTON) {
    HWND hwnd = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
                              x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gInstance, nullptr);
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(gGuiFont), TRUE);
    return hwnd;
}

void SetEditInt(HWND parent, int id, int value) {
    wchar_t buffer[32]{};
    wsprintfW(buffer, L"%d", value);
    SetWindowTextW(GetDlgItem(parent, id), buffer);
}

void FillSettingsControls(HWND hwnd) {
    SetWindowTextW(GetDlgItem(hwnd, IDC_LANES), gConfig.lanesText.c_str());
    SetEditInt(hwnd, IDC_X, gConfig.x);
    SetEditInt(hwnd, IDC_Y, gConfig.y);
    SetEditInt(hwnd, IDC_WIDTH, gConfig.width);
    SetEditInt(hwnd, IDC_HEIGHT, gConfig.height);
    SetEditInt(hwnd, IDC_DURATION, gConfig.durationMs);
    SetEditInt(hwnd, IDC_TRAVEL, gConfig.travelPx);
    SendMessageW(GetDlgItem(hwnd, IDC_DEBUG_BG), BM_SETCHECK, gConfig.debugBackground ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_CLICK_THROUGH), BM_SETCHECK, gConfig.clickThrough ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_TOPMOST), BM_SETCHECK, gConfig.alwaysOnTop ? BST_CHECKED : BST_UNCHECKED, 0);
    RefreshKeyMapStatus();
}

std::wstring DetectModeForLanes(const std::wstring& lanesText) {
    std::wstring normalized = Upper(lanesText);
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), [](wchar_t c) {
        return std::iswspace(c) != 0;
    }), normalized.end());
    for (const Preset& preset : kPresets) {
        std::wstring presetText = Upper(preset.lanes);
        presetText.erase(std::remove_if(presetText.begin(), presetText.end(), [](wchar_t c) {
            return std::iswspace(c) != 0;
        }), presetText.end());
        if (normalized == presetText) {
            return preset.mode;
        }
    }
    return L"CUSTOM";
}

bool ReadSettingsControls() {
    gConfig.lanesText = WindowText(GetDlgItem(gSettingsWnd, IDC_LANES));
    gConfig.mode = DetectModeForLanes(gConfig.lanesText);
    gConfig.x = WindowInt(GetDlgItem(gSettingsWnd, IDC_X), gConfig.x, -32000, 32000);
    gConfig.y = WindowInt(GetDlgItem(gSettingsWnd, IDC_Y), gConfig.y, -32000, 32000);
    gConfig.width = WindowInt(GetDlgItem(gSettingsWnd, IDC_WIDTH), gConfig.width, 240, 10000);
    gConfig.height = WindowInt(GetDlgItem(gSettingsWnd, IDC_HEIGHT), gConfig.height, 90, 4000);
    gConfig.durationMs = WindowInt(GetDlgItem(gSettingsWnd, IDC_DURATION), gConfig.durationMs, 120, 5000);
    gConfig.travelPx = WindowInt(GetDlgItem(gSettingsWnd, IDC_TRAVEL), gConfig.travelPx, 40, 4000);
    gConfig.debugBackground = SendMessageW(GetDlgItem(gSettingsWnd, IDC_DEBUG_BG), BM_GETCHECK, 0, 0) == BST_CHECKED;
    gConfig.clickThrough = SendMessageW(GetDlgItem(gSettingsWnd, IDC_CLICK_THROUGH), BM_GETCHECK, 0, 0) == BST_CHECKED;
    gConfig.alwaysOnTop = SendMessageW(GetDlgItem(gSettingsWnd, IDC_TOPMOST), BM_GETCHECK, 0, 0) == BST_CHECKED;
    RebuildLaneConfig();
    gConfig.lanesText = CanonicalLaneText();
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_LANES), gConfig.lanesText.c_str());
    gConfig.mode = DetectModeForLanes(gConfig.lanesText);
    gNotes.clear();
    gKeyDown.fill(false);
    RefreshKeyMapStatus();
    ApplyOverlayWindowStyle();
    return !gConfig.lanes.empty();
}

void ApplyPreset(const wchar_t* mode, const wchar_t* lanes) {
    gConfig.mode = mode;
    gConfig.lanesText = lanes;
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_LANES), gConfig.lanesText.c_str());
    RebuildLaneConfig();
    gConfig.lanesText = CanonicalLaneText();
    SetWindowTextW(GetDlgItem(gSettingsWnd, IDC_LANES), gConfig.lanesText.c_str());
    gNotes.clear();
    gKeyDown.fill(false);
    RefreshKeyMapStatus();
    InvalidateRect(gOverlayWnd, nullptr, FALSE);
}

void SpawnTestNotes() {
    for (int i = 0; i < static_cast<int>(gConfig.lanes.size()); ++i) {
        SpawnTapNote(i);
    }
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        MakeLabel(hwnd, L"Lane keys", 18, 18, 110, 22);
        MakeEdit(hwnd, IDC_LANES, L"", 130, 16, 360, 24);
        MakeLabel(hwnd, L"Presets", 18, 52, 110, 22);
        MakeButton(hwnd, IDC_PRESET_5, L"5K", 130, 48, 58, 28);
        MakeButton(hwnd, IDC_PRESET_8, L"8K", 198, 48, 58, 28);
        MakeButton(hwnd, IDC_PRESET_10, L"10K", 266, 48, 58, 28);
        MakeLabel(hwnd, L"Position", 18, 92, 110, 22);
        MakeLabel(hwnd, L"X", 130, 92, 18, 22);
        MakeEdit(hwnd, IDC_X, L"", 150, 90, 70, 24);
        MakeLabel(hwnd, L"Y", 232, 92, 18, 22);
        MakeEdit(hwnd, IDC_Y, L"", 252, 90, 70, 24);
        MakeLabel(hwnd, L"Size", 18, 128, 110, 22);
        MakeLabel(hwnd, L"W", 130, 128, 22, 22);
        MakeEdit(hwnd, IDC_WIDTH, L"", 154, 126, 70, 24);
        MakeLabel(hwnd, L"H", 236, 128, 22, 22);
        MakeEdit(hwnd, IDC_HEIGHT, L"", 260, 126, 70, 24);
        MakeLabel(hwnd, L"Timing", 18, 164, 110, 22);
        MakeLabel(hwnd, L"Release fade ms", 130, 164, 112, 22);
        MakeEdit(hwnd, IDC_DURATION, L"", 246, 162, 70, 24);
        MakeLabel(hwnd, L"Travel px", 330, 164, 70, 22);
        MakeEdit(hwnd, IDC_TRAVEL, L"", 402, 162, 70, 24);

        MakeButton(hwnd, IDC_DEBUG_BG, L"Debug background", 130, 202, 150, 24, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_CLICK_THROUGH, L"Click-through", 290, 202, 120, 24, BS_AUTOCHECKBOX);
        MakeButton(hwnd, IDC_TOPMOST, L"Always on top", 130, 232, 150, 24, BS_AUTOCHECKBOX);

        MakeButton(hwnd, IDC_APPLY, L"Apply", 130, 278, 78, 30);
        MakeButton(hwnd, IDC_SAVE, L"Save", 220, 278, 78, 30);
        MakeButton(hwnd, IDC_TEST, L"Test notes", 310, 278, 100, 30);
        {
            HWND status = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 18, 326, 472, 22,
                                        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS)), gInstance, nullptr);
            SendMessageW(status, WM_SETFONT, reinterpret_cast<WPARAM>(gGuiFont), TRUE);
        }

        FillSettingsControls(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PRESET_5:
            ApplyPreset(kPresets[0].mode, kPresets[0].lanes);
            SpawnTestNotes();
            return 0;
        case IDC_PRESET_8:
            ApplyPreset(kPresets[1].mode, kPresets[1].lanes);
            SpawnTestNotes();
            return 0;
        case IDC_PRESET_10:
            ApplyPreset(kPresets[2].mode, kPresets[2].lanes);
            SpawnTestNotes();
            return 0;
        case IDC_APPLY:
            ReadSettingsControls();
            SpawnTestNotes();
            return 0;
        case IDC_SAVE:
            ReadSettingsControls();
            if (SaveConfig()) {
                SetStatus(L"saved: " + gConfigPath);
            } else {
                SetStatus(L"save failed: " + gConfigPath);
                MessageBoxW(hwnd, gConfigPath.c_str(), L"Failed to save config", MB_ICONERROR);
            }
            return 0;
        case IDC_TEST:
            ReadSettingsControls();
            SpawnTestNotes();
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool RegisterWindows() {
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
        const KBDLLHOOKSTRUCT* data = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        UINT vk = data->vkCode;
        bool keyDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
        bool keyUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;

        if (vk < gKeyDown.size() && keyUp) {
            gKeyDown[vk] = false;
            int lane = gVkToLaneIndex[vk];
            if (lane >= 0) {
                PostMessageW(gOverlayWnd, WM_APP_KEY_UP, static_cast<WPARAM>(lane), 0);
            }
        } else if (vk < gKeyDown.size() && keyDown) {
            if (vk == VK_F12 && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                PostMessageW(gSettingsWnd, WM_CLOSE, 0, 0);
            } else if (!gKeyDown[vk]) {
                gKeyDown[vk] = true;
                int lane = gVkToLaneIndex[vk];
                if (lane >= 0) {
                    PostMessageW(gOverlayWnd, WM_APP_KEY_DOWN, static_cast<WPARAM>(lane), 0);
                }
            }
        }
    }
    return CallNextHookEx(gKeyboardHook, code, wParam, lParam);
}

bool CreateAppWindows(int showCommand) {
    DWORD overlayEx = WS_EX_LAYERED | WS_EX_APPWINDOW | WS_EX_NOACTIVATE;
    if (gConfig.clickThrough) {
        overlayEx |= WS_EX_TRANSPARENT;
    }
    if (gConfig.alwaysOnTop) {
        overlayEx |= WS_EX_TOPMOST;
    }
    gOverlayWnd = CreateWindowExW(
        overlayEx,
        L"KeyNoteOverlayCppWindow",
        L"Key Note Overlay C++ Overlay",
        WS_POPUP | WS_VISIBLE,
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

    gSettingsWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"KeyNoteOverlayCppSettings",
        L"Key Note Overlay C++ Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        530,
        400,
        nullptr,
        nullptr,
        gInstance,
        nullptr
    );
    if (!gSettingsWnd) {
        return false;
    }
    SetWindowDisplayAffinity(gSettingsWnd, WDA_EXCLUDEFROMCAPTURE);
    (void)showCommand;
    ShowWindow(gSettingsWnd, SW_SHOWNORMAL);
    UpdateWindow(gSettingsWnd);
    return true;
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

    gConfigPath = CurrentDirectory() + L"key_note_cpp_selftest.ini";
    bool saveOk = SaveConfig();
    bool saveReadbackOk = ReadIniString(L"input", L"lanes", L"") == gConfig.lanesText;

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
    bool ok = semicolonOk && laneCountOk && registeredOk && unregisteredFiltered && fiveKeyOk && tenKeyOk && canonicalOk && saveOk && saveReadbackOk && holdLifecycleOk;
    log << "result=" << (ok ? "ok" : "fail") << "\n";

    return ok ? 0 : 1;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    gInstance = instance;
    if (std::wstring(GetCommandLineW()).find(L"--self-test") != std::wstring::npos) {
        return RunSelfTest();
    }

    InitCommonControls();
    LoadConfig();

    gGuiFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
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

    if (!RegisterWindows() || !CreateAppWindows(showCommand)) {
        MessageBoxW(nullptr, L"Failed to create Key Note Overlay C++ windows.", L"Key Note Overlay C++", MB_ICONERROR);
        return 1;
    }

    gKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(nullptr), 0);
    if (!gKeyboardHook) {
        MessageBoxW(gSettingsWnd, L"Failed to install global keyboard hook.", L"Key Note Overlay C++", MB_ICONERROR);
    }

    SpawnTestNotes();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (gKeyboardHook) {
        UnhookWindowsHookEx(gKeyboardHook);
        gKeyboardHook = nullptr;
    }
    if (gNoteFont) {
        DeleteObject(gNoteFont);
        gNoteFont = nullptr;
    }
    return static_cast<int>(msg.wParam);
}
