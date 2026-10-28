// Offline reference construction. No game process or user configuration access.
#include "analysis.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    const auto root = std::filesystem::u8path(argc > 1 ? argv[1] : ".");
    std::ifstream manifest(root / "decoded.tsv");
    std::ofstream output(root / "measurements.tsv");
    if (!manifest || !output) return 1;
    output << std::setprecision(17);
    std::string id; int rank, count = 0;
    while (manifest >> id >> rank) {
        chartcalc::Chart chart; chart.keys = 4;
        std::ifstream notes(root / "notes" / (id + ".txt"));
        chartcalc::Note note;
        while (notes >> note.lane >> note.startMs >> note.endMs) chart.notes.push_back(note);
        try {
            const auto analysis = chartcalc::Analyze(std::move(chart));
            if (!analysis.summary.ready || analysis.limited) return 2;
            output << id << '\t' << rank << '\t' << analysis.summary.stars << '\t' << analysis.summary.nps << '\n';
            std::cout << "LN " << rank << ": " << analysis.summary.stars << '\n';
            ++count;
        } catch (const std::exception& error) { std::cerr << "Rank " << rank << ": " << error.what() << '\n'; return 3; }
    }
    return count == 15 && output ? 0 : 4;
}
