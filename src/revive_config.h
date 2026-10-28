#pragma once
// Frozen 1K-18K / fixed placement / auto / qwilight_bms_ez parameters.
// config.dat SHA-256: c767bddacee5347a67a9e7c6a9daa95bf9d3e3dcf6f45a9e20bc593cedaf21df
// 1K-18K matrices: original config.yaml SHA-256 bcb3cebad97604cd7ffe323ad009c2e9c4d8913bd4b3212365dc08ca033fab59.
#include "revive_native.h"
#include "revive_key_matrices.h"
#include <limits>
#include <stdexcept>
namespace revive {
inline const Config& DefaultConfig() {
    static const Config config = [] {
        Config c;
        c.values = {
            {"weighted_nps.time_window_ms", 1000},
            {"jack_difficulty.coefficients.j75", 1},
            {"jack_difficulty.coefficients.j100", 0.5},
            {"jack_difficulty.coefficients.j125", 0.40000000000000002},
            {"jack_difficulty.coefficients.j150", 0.10000000000000001},
            {"jack_difficulty.ln_half_interval", 1},
            {"jack_difficulty.fd_weight", 0.5},
            {"jack_difficulty.rd_weight", 0.5},
            {"jack_difficulty.lfd_weight", 1},
            {"jack_difficulty.lrd_weight", 1},
            {"jack_difficulty.distance_weight", 1},
            {"jack_difficulty.vibro_nerf_nps_condition", 10},
            {"jack_difficulty.vibro_nps_nerf_weight", 0.90000000000000002},
            {"jack_difficulty.vibro_relation_nerf_weight", 0},
            {"jack_difficulty.vibro_j150_nerf_weight", 0},
            {"jack_difficulty.vibro_j125_nerf_weight", 0},
            {"jack_difficulty.vibro_j100_nerf_weight", 1},
            {"jack_difficulty.vibro_j75_nerf_weight", 0.5},
            {"spider_distance_nerf.enabled", 1},
            {"spider_distance_nerf.max_nerf", 0.22},
            {"spider_distance_nerf.window_ms", 1000},
            {"spider_distance_nerf.min_interval_ms", 120},
            {"spider_distance_nerf.max_interval_ms", 190},
            {"spider_distance_nerf.max_interval_cv", 0.28000000000000003},
            {"spider_distance_nerf.min_unique_lanes", 4},
            {"spider_distance_nerf.min_notes", 6},
            {"spider_distance_nerf.min_entropy_ratio", 0.69999999999999996},
            {"spider_distance_nerf.max_same_line_ratio", 0.55000000000000004},
            {"spider_distance_nerf.max_jack_ratio", 0.34999999999999998},
            {"spider_distance_nerf.min_other_distance_ratio", 0.55000000000000004},
            {"spider_distance_nerf.min_distance_sum", 16},
            {"skill_radar.enabled", 1},
            {"skill_radar.chord_eps_ms", 2},
            {"skill_radar.release_eps_ms", 2},
            {"skill_radar.chord_alpha", 0.69999999999999996},
            {"skill_radar.stream_scale", 16},
            {"skill_radar.speed_scale", 18},
            {"skill_radar.jack_scale", 70},
            {"skill_radar.chord_scale", 14},
            {"skill_radar.tech_scale", 45},
            {"skill_radar.hold_scale", 12},
            {"ldb_weight", 1},
            {"score_weight", 0.10000000000000001},
            {"acc_weight", 0.90000000000000002},
            {"rating_weight", 0.050147842383428128},
            {"rating_power", 0.35163702826872595},
            {"avg_rating_power", 1},
            {"avg_lv_power", 1},
            {"revive_score_weight", 0.10000000000000001},
            {"revive_acc_weight", 0.90000000000000002},
            {"revive_level_scale", 0.052379807720288357},
            {"revive_level_power", 0.61514792344117442},
            {"revive_max_level", 24},
        };
        c.timeDelta = 'C';
        c.weights = {{
            {{1.2, 1.2, 0.59999999999999998, 0.59999999999999998, 0.59999999999999998, 0.12, 0.12, 0.12, 0.12, 0.12}},
            {{0.82499999999999996, 1.1000000000000001, 0.82499999999999996, 0.55000000000000004, 0.55000000000000004, 0.11, 0.11, 0.11, 0.11, 0.11}},
            {{0.5, 0.75, 1, 0.75, 0.5, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001}},
            {{0.5, 0.5, 0.75, 1, 0.75, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001}},
            {{0.55000000000000004, 0.55000000000000004, 0.55000000000000004, 0.82499999999999996, 1.1000000000000001, 0.38500000000000001, 0.11, 0.11, 0.11, 0.11}},
            {{0.11, 0.11, 0.11, 0.11, 0.38500000000000001, 1.1000000000000001, 0.82499999999999996, 0.55000000000000004, 0.55000000000000004, 0.55000000000000004}},
            {{0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.75, 1, 0.75, 0.5, 0.5}},
            {{0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.10000000000000001, 0.5, 0.75, 1, 0.75, 0.5}},
            {{0.11, 0.11, 0.11, 0.11, 0.11, 0.55000000000000004, 0.55000000000000004, 0.82499999999999996, 1.1000000000000001, 0.82499999999999996}},
            {{0.12, 0.12, 0.12, 0.12, 0.12, 0.59999999999999998, 0.59999999999999998, 0.59999999999999998, 1.2, 1.2}},
        }};
        c.visual = {{
            {{1, 1, 2, 3, 2.5, 1.5, 2.5, 3, 2.25, 1.25}},
            {{1, 1, 1, 2, 3, 2.5, 1.5, 2.25, 1.25, 2.25}},
            {{2, 1, 1, 1, 2, 3, 2.25, 1.25, 2.25, 3}},
            {{3, 2, 1, 1, 1, 2, 1.25, 2.25, 1.5, 2.5}},
            {{2.5, 3, 2, 1, 1, 1, 2, 3, 2.5, 1.5}},
            {{1.5, 2.5, 3, 2, 1, 1, 1, 2, 3, 2.5}},
            {{2.5, 1.5, 2.25, 1.25, 2, 1, 1, 1, 2, 3}},
            {{3, 2.25, 1.25, 2.25, 3, 2, 1, 1, 1, 2}},
            {{2.25, 1.25, 2.25, 1.5, 2.5, 3, 2, 1, 1, 1}},
            {{1.25, 2.25, 3, 2.5, 1.5, 2.5, 3, 2, 1, 1}},
        }};
        c.types = {{
            {{0, 1, 0}},
            {{1, 0, 0}},
            {{0, 0, 0}},
        }};
        c.judgments = {
            {{19.231000000000002, -19.231000000000002, 19.231000000000002, -19.231000000000002, 28.846499999999999, -28.846499999999999, 100, 100, 100}},
            {{46.154000000000003, -46.154000000000003, 46.154000000000003, -46.154000000000003, 69.230999999999995, -69.230999999999995, 90, 100, 100}},
            {{73.076999999999998, -73.076999999999998, 73.076999999999998, -73.076999999999998, 109.6155, -109.6155, 10, 70, 100}},
            {{100, -100, 100, -100, 150, -150, 1, 50, 100}},
            {{126.923, -126.923, 126.923, -126.923, std::numeric_limits<double>::quiet_NaN(), -190.3845, 0, 30, 100}},
            {{std::numeric_limits<double>::quiet_NaN(), -153.846, std::numeric_limits<double>::quiet_NaN(), -153.846, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), 0, 0, 0}},
            {{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()}},
        };
        return c;
    }();
    return config;
}
inline const Config& ConfigForKeys(int keys) {
    if (!SupportsKeys(keys)) throw std::invalid_argument("revive unsupported keys");
    // Each normal key layout uses its original matrices, never a scaled 10K
    // approximation. Common judgment/radar coefficients remain frozen.
    static const auto configurations = [] {
        std::array<Config, MaxKeys> result;
        for (int i = 0; i < MaxKeys; ++i) {
            result[i] = DefaultConfig(); result[i].keys = i + 1;
            result[i].weights = KeyWeights[i]; result[i].visual = KeyVisualDistances[i];
        }
        return result;
    }();
    return configurations[keys - 1];
}
} // namespace revive
