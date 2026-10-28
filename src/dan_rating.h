#pragma once
#include "pattern_difficulty.h"
#include <string>

namespace pattern {
enum class DanHeight { Low, MidLow, Level, High };
struct DanEstimate {
    bool available = false;
    int low = 0;
    int high = 0;
    int outside = 0;
    double progress = 0.0;
    DanHeight height = DanHeight::Level;
};

// A comparison against the same short-window measurement on reference courses.
// This does not evaluate course accuracy, misses, endurance or clear requirements.
DanEstimate EstimateDan(RatingProfile profile, double comparisonLoad);
DanEstimate EstimateDan(const Snapshot& snapshot);
// Independent 4K LN v2 reference comparison; never replaces Reform or mixes
// their rank ladders. Like the main estimate, this is not a clear prediction.
DanEstimate EstimateLnDan(const Snapshot& snapshot);
std::wstring FormatLnDan(const Snapshot& snapshot);
std::wstring DanLabel(RatingProfile profile, int rank);
std::wstring FormatDan(RatingProfile profile, const DanEstimate& estimate);
const wchar_t* DanBasis(RatingProfile profile);
}
