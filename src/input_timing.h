#pragma once

namespace input_timing {
// Physical key dwell is not a chart LN flag. Share the live inference boundary
// between the primary difficulty estimator and the six auxiliary traits.
inline constexpr double LiveHoldThresholdMs = 170.0;
// Published chart/reference measurements used 90ms; preserve their scale.
inline constexpr double ReferenceHoldThresholdMs = 90.0;
enum class HoldTiming { LiveInput, ReferenceReplay };
}
