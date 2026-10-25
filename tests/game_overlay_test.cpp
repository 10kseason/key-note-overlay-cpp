#include "game_overlay.h"
#include "self_test_io.h"
#include "window_placement.h"
#include <filesystem>
#include <iostream>

namespace {
struct FindWindowState { DWORD pid; HWND window = nullptr; };
BOOL CALLBACK FindFixture(HWND window, LPARAM param) {
    auto& state = *reinterpret_cast<FindWindowState*>(param);
    DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
    if (pid != state.pid) return TRUE;
    state.window = window;
    return FALSE;
}
void CALLBACK FixtureTimeout(HWND, UINT, UINT_PTR, DWORD) { PostQuitMessage(0); }
}

int main(int argc, char**) {
    if (argc > 1) {
        // Child fixture: a real but hidden native window. No focus, keyboard
        // hook, injected input or real game process is involved.
        HWND window = CreateWindowW(L"STATIC", L"overlay classifier fixture", WS_POPUP,
            0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!window) return 1;
        const auto timer = SetTimer(nullptr, 0, 10000, FixtureTimeout);
        if (!timer) return 1;
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        KillTimer(nullptr, timer); DestroyWindow(window);
        return 0;
    }

    int checks = 0, failures = 0;
    auto check = [&](const char* name, bool ok) {
        std::cout << name << '=' << (ok ? "ok" : "FAIL") << std::endl;
        ++checks; if (!ok) ++failures;
    };
    auto placementOk = [](const RECT& overlay, SIZE settings, const std::vector<RECT>& areas) {
        const auto placed = window_placement::BesideOverlay(overlay, settings, areas);
        bool within = false;
        for (const auto& work : areas) within = within || (placed.rect.left >= work.left && placed.rect.top >= work.top &&
            placed.rect.right <= work.right && placed.rect.bottom <= work.bottom);
        return placed.separated && within && window_placement::OverlapArea(overlay, placed.rect) == 0;
    };
    check("startup_default_1080p_nonoverlap", placementOk({80,40,980,260}, {740,660}, {{0,0,1920,1040}}));
    check("startup_default_1440p_nonoverlap", placementOk({80,40,980,260}, {740,660}, {{0,0,2560,1400}}));
    check("startup_bottom_overlay_places_settings_above", placementOk({300,850,1200,1040}, {740,660}, {{0,0,1920,1040}}));
    check("startup_negative_monitor_nonoverlap", placementOk({-1800,50,-900,270}, {740,660}, {{-1920,0,0,1040}}));
    check("startup_uses_second_monitor_if_needed", placementOk({0,0,1280,680}, {740,660}, {{0,0,1280,680}, {1280,0,3200,1040}}));
    const auto constrained = window_placement::BesideOverlay({0,0,1280,680}, {740,660}, {{0,0,1280,680}});
    check("startup_constrained_fallback_stays_on_screen", !constrained.separated && constrained.rect.left >= 0 &&
        constrained.rect.top >= 0 && constrained.rect.right <= 1280 && constrained.rect.bottom <= 680);
    std::error_code error;
    const auto parent = std::filesystem::current_path(error);
    const auto root = parent / (L"game-fixtures-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    const bool created = !error && std::filesystem::create_directory(root, error);
    check("isolated_fixture_directory", created && !error);
    if (!created || error) return 1;
    wchar_t executable[32768];
    const DWORD length = GetModuleFileNameW(nullptr, executable, ARRAYSIZE(executable));
    check("fixture_executable_path", length > 0 && length < ARRAYSIZE(executable));
    game_overlay::ForegroundClassifier classifier;
    HWND ownWindow = CreateWindowW(L"STATIC", L"own test window", WS_POPUP,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    check("own_process_is_excluded", ownWindow && classifier.Identify(ownWindow) == game_overlay::Game::None);
    DestroyWindow(ownWindow);
    for (const auto& name : {L"osu!.exe", L"osu.exe", L"Qwilight.exe", L"Quaver.exe"}) {
        const auto fixture = root / name;
        const bool copied = length && length < ARRAYSIZE(executable) &&
            std::filesystem::copy_file(executable, fixture, std::filesystem::copy_options::none, error);
        check("fixture_copied_without_overwriting", copied && !error);
        if (!copied || error) continue;
        std::wstring command = L"\"" + fixture.wstring() + L"\" --hidden-host";
        STARTUPINFOW startup{}; startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        const bool started = CreateProcessW(fixture.c_str(), command.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, root.c_str(), &startup, &process) != FALSE;
        check("hidden_fixture_started", started);
        if (!started) continue;
        FindWindowState state{process.dwProcessId};
        for (int i = 0; i < 200 && !state.window; ++i) {
            EnumWindows(FindFixture, reinterpret_cast<LPARAM>(&state));
            if (!state.window && WaitForSingleObject(process.hProcess, 10) != WAIT_TIMEOUT) break;
        }
        check("fixture_window_is_hidden", state.window && !IsWindowVisible(state.window));
        check("fixture_did_not_activate", state.window && GetForegroundWindow() != state.window);
        const auto expected = game_overlay::FromExecutable(name);
        check("real_process_path_classified", state.window && classifier.Identify(state.window) == expected);
        check("cached_classification_matches", state.window && classifier.Identify(state.window) == expected);
        // End only the child created above. The timer also bounds its lifetime
        // if the parent encounters an unexpected error.
        PostThreadMessageW(process.dwThreadId, WM_QUIT, 0, 0);
        const bool stopped = WaitForSingleObject(process.hProcess, 2000) == WAIT_OBJECT_0;
        if (!stopped) { TerminateProcess(process.hProcess, 1); WaitForSingleObject(process.hProcess, 1000); }
        check("fixture_exits_normally", stopped);
        CloseHandle(process.hThread); CloseHandle(process.hProcess);
    }
    check("owned_fixtures_removed", self_test_io::RemoveOwnedDirectory(root, parent, error));
    std::cout << "checks=" << checks << "\nfailures=" << failures << std::endl;
    return failures ? 1 : 0;
}
