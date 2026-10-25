#pragma once
#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <string>

namespace game_overlay {
enum class Game { None, Osu, Qwilight };

inline Game FromExecutable(std::wstring path) {
    const auto slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) path.erase(0, slash + 1);
    std::transform(path.begin(), path.end(), path.begin(), [](wchar_t c) { return std::towlower(c); });
    if (path == L"osu!.exe" || path == L"osu.exe") return Game::Osu;
    if (path == L"qwilight.exe") return Game::Qwilight;
    return Game::None;
}

inline bool AllowsInput(bool gameMode, Game foreground, bool settingsPractice) {
    return !gameMode || foreground != Game::None || settingsPractice;
}
inline LONG_PTR ExtendedStyle(LONG_PTR current, bool gameMode, bool clickThrough, bool moving) {
    current &= ~(WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
    // Keep a normal capture-selectable window in both modes. Game focus gates
    // input only; a tool window may disappear from an app/window capture picker.
    current |= WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_APPWINDOW;
    if (!moving && (gameMode || clickThrough)) current |= WS_EX_TRANSPARENT;
    return current;
}

// Inspect only the foreground process executable. Never read window titles,
// game memory or input buffers. Cache briefly to keep the keyboard hook cheap.
class ForegroundClassifier {
public:
    Game Identify(HWND window) {
        DWORD pid = 0;
        if (window) GetWindowThreadProcessId(window, &pid);
        const auto now = GetTickCount64();
        if (window == window_ && pid == pid_ && now < expires_) return game_;
        window_ = window; pid_ = pid; expires_ = now + 1000; game_ = Game::None;
        if (!pid || pid == GetCurrentProcessId()) return game_;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process) return game_;
        wchar_t path[32768]; DWORD length = ARRAYSIZE(path);
        if (QueryFullProcessImageNameW(process, 0, path, &length)) game_ = FromExecutable(std::wstring(path, length));
        CloseHandle(process);
        return game_;
    }
private:
    HWND window_ = nullptr;
    DWORD pid_ = 0;
    ULONGLONG expires_ = 0;
    Game game_ = Game::None;
};
}
