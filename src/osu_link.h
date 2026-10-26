#pragma once
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

namespace osu_link {
inline constexpr wchar_t RiskWarning[] = L"이 기능의 위험부담은 전부 사용자 몫입니다!!!";
struct View {
    bool enabled = false, ready = false;
    int keys = 0;
    std::wstring status = L"OFF · 앱을 시작할 때마다 꺼져 있습니다.";
    std::wstring title, variant, difficulty, details;
};
// Only relative single directory/file names from a verified beatmap object are
// accepted. A game-provided string must never select an arbitrary local file.
bool SafeComponent(const std::wstring& value);
std::filesystem::path ResolveChart(const std::filesystem::path& songs,
                                   const std::wstring& folder, const std::wstring& file);

// No constructor-time enumeration or process access. Consent is session-only;
// nothing in this class persists a path, memory contents, or the enabled state.
class Session {
public:
    ~Session();
    bool Start(bool riskAccepted, std::filesystem::path songsOverride = {});
    void Stop();
    View Current() const;
private:
    void Run(std::filesystem::path songsOverride);
    void Publish(View value);
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::thread worker_;
    std::atomic<bool> cancelled_{true};
    View view_;
};
}
