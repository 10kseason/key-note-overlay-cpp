#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace chartcalc {
struct Note { int lane; double startMs, endMs; };
struct Chart {
    std::wstring title, artist, variant;
    std::string format;
    int keys = 0;
    std::vector<Note> notes;
    std::vector<std::wstring> warnings;
};
// Reads chart text only. It never opens referenced sounds/images or executes commands.
// Throws std::runtime_error on unsupported or ambiguous playable/timing semantics.
Chart LoadChart(const std::filesystem::path& path);
}
