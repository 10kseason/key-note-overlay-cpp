#pragma once
#include <algorithm>
namespace revive_hud {
struct Layout { int columns, skillHeight, mainHeight, liveLnHeight, selectedTop, selectedLnHeight, total, rowHeight; bool stackedSkills; };
inline Layout Make(int width, bool live, bool selected, bool liveLn = false, bool selectedLn = false) {
    const int columns = width >= 900 ? 6 : width >= 456 ? 3 : 2;
    const bool stacked = (width - 8) / columns < 148;
    const int rowHeight = stacked ? 60 : 36;
    const int skills = 28 + rowHeight * (6 / columns);
    const int main = live ? 76 : 0;
    const int liveLnHeight = live && liveLn ? (width >= 600 ? 40 : 60) : 0;
    const int selectedLnHeight = selected && selectedLn ? (width >= 600 ? 40 : 60) : 0;
    const int selectedTop = main + liveLnHeight + (live ? skills : 0);
    return {columns, skills, main, liveLnHeight, selectedTop, selectedLnHeight,
        selectedTop + (selected ? 18 + selectedLnHeight + skills : 0), rowHeight, stacked};
}
inline int PresentationHeight(int configured, int noteHeight, const Layout& layout) {
    return std::max(configured, layout.total + std::max(36, noteHeight + 16));
}
}
