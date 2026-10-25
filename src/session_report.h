#pragma once
#include "pattern_difficulty.h"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace report {
struct Note { int lane; double startMs; double endMs; bool interrupted = false; };
struct Point { double timeMs; pattern::Snapshot snapshot; };
struct Session {
    int lanes = 0;
    std::string startedLocal;
    std::string reason;
    double durationMs = 0;
    std::size_t presses = 0;
    bool truncated = false;
    bool analysisLimited = false;
    std::vector<Note> notes;
    std::vector<Point> points;
    Point peak{};
};

// Opt-in ownership belongs to the caller. Only lane indices and relative times
// enter this module; physical key names, text and application titles never do.
class Recorder {
public:
    static constexpr std::size_t MaxNotes = 50000;
    static constexpr std::size_t MaxPoints = 7200;
    void Begin(int lanes, double nowMs, const std::string& startedLocal);
    void Key(int lane, bool down, double nowMs);
    void Observe(const pattern::Snapshot& snapshot, double nowMs);
    void ReleaseAll(double nowMs);
    bool Active() const { return startMs_ >= 0; }
    bool Finished(double nowMs) const;
    Session Take(const std::string& reason, double nowMs);
private:
    Session session_;
    std::array<int, 256> held_{};
    double startMs_ = -1, lastInputMs_ = -1, lastPointMs_ = -1;
};

double DifficultyValue(const pattern::Snapshot& snapshot);
std::string DifficultyLabel(const pattern::Snapshot& snapshot);
std::string RenderHtml(const Session& session);
struct SaveResult { bool ok = false; std::filesystem::path path; unsigned long error = 0; };
// Exclusive creation + same-directory atomic rename. Existing reports are never
// replaced. Safe to run on a worker with an immutable session and directory.
SaveResult Save(const Session& session, const std::filesystem::path& directory);
}
