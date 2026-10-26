#pragma once
#include "chart_reader.h"
#include "pattern_difficulty.h"
#include <atomic>
#include <string>

namespace chartcalc {
struct Point { double timeMs = 0; pattern::Snapshot snapshot; };
struct Analysis {
    Chart chart;
    double durationMs = 0;
    double averageNps = 0;
    double maximumNps = 0;
    double holdWeight = pattern::DefaultHoldWeight;
    int strongestWindows = 0;
    bool limited = false;
    pattern::Snapshot summary;
    std::vector<Point> points;
};
Analysis Analyze(Chart chart, double holdWeight = pattern::DefaultHoldWeight,
                 const std::atomic<bool>* cancelled = nullptr);
double DifficultyValue(const pattern::Snapshot& snapshot);
std::wstring DifficultyLabel(const pattern::Snapshot& snapshot);
std::wstring Basis(int keys);
std::string Utf8(const std::wstring& text);
std::wstring Wide(const std::string& text);
std::wstring Number(double value, int precision = 1);
std::string RenderReport(const Analysis& analysis);
void SaveReport(const Analysis& analysis, const std::filesystem::path& path);
}
