#pragma once
// 1K-18K, no-scratch native port of the user-supplied calculator. The original
// experiment is preserved; this copy adds bounded, cancellable execution.
#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <vector>
namespace revive {
inline constexpr int MaxKeys = 18;
inline constexpr bool SupportsKeys(int keys) { return keys >= 1 && keys <= MaxKeys; }
using LaneMatrix = std::array<std::array<double,MaxKeys>,MaxKeys>;
struct Note { double time; int lane; int type; }; // seconds; 0-based lane; rice=0/head=1/tail=2
struct Config {
    int keys = 10; // Fixed capacity; only the selected key count is active.
    std::map<std::string,double> values;
    LaneMatrix weights{}, visual{};
    std::array<std::array<double,3>,3> types{};
    std::vector<std::array<double,9>> judgments; // 6 signed bounds + score/acc/gauge; NaN=null
    char timeDelta = 'C';
    double get(const std::string& key,double fallback) const {
        auto it=values.find(key); return it==values.end()?fallback:it->second;
    }
};
struct Result {
    std::map<std::string,std::vector<double>> arrays;
    std::map<std::string,double> scalars;
};
inline constexpr std::size_t MaxEvents = 40000; // LN heads and tails both count.
Result calculate(std::vector<Note> notes, double duration, const Config& config,
    const std::atomic<bool>* cancelled = nullptr,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(1500));
}
