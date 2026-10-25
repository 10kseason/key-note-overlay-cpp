// Offline feature extraction; never linked into the overlay executable.
#include "pattern_difficulty.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

struct Event { double time; int lane; bool down; };
int main(int argc, char** argv) {
    const auto root = std::filesystem::u8path(argc > 1 ? argv[1] : ".");
    std::ifstream manifest(root / "decoded.tsv");
    std::ofstream output(root / "features.tsv");
    if (!manifest || !output) return 1;
    // Optional diagnostics stay in the caller's ignored reference directory.
    std::ofstream audit;
    if (argc > 2 && std::string(argv[2]) == "--windows") {
        audit.open(root / "windows.tsv");
        if (!audit) return 1;
        audit << "id\tkeys\trank\ttime\tstars\tnps\tburst\tchord\tjack\thold\tmax_chord\trating\tvariety\trhythm\tcolumn_repeat\tadjacent_repeat\n";
    }
    std::string line;
    int accepted = 0;
    while (std::getline(manifest, line)) {
        std::istringstream row(line);
        std::string id, group; int keys; double level;
        if (!(row >> id >> keys >> level >> group)) continue;
        std::ifstream input(root / "notes" / (id + ".txt"));
        std::vector<Event> events;
        int lane; double start, end;
        while (input >> lane >> start >> end) {
            events.push_back({start, lane, true});
            events.push_back({end > start ? end : start + 10.0, lane, false});
        }
        if (events.empty()) continue;
        std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
            if (a.time != b.time) return a.time < b.time;
            if (a.down != b.down) return !a.down;
            return a.lane < b.lane;
        });
        const double origin = events.front().time;
        // Keep the published reference scale stable. Live LN feedback applies a
        // separate 2/3 multiplier; rebuilding references with it would offset
        // the requested reduction and also move tap-only Dan/table thresholds.
        pattern::DifficultyEstimator estimator(keys, 1.0);
        std::vector<pattern::Snapshot> windows;
        std::size_t next = 0;
        for (double time = origin + 8000.0; time <= events.back().time; time += 2000.0) {
            while (next < events.size() && events[next].time <= time) {
                const auto& event = events[next++];
                if (event.down) estimator.KeyDown(event.lane, event.time - origin);
                else estimator.KeyUp(event.lane, event.time - origin);
            }
            auto snapshot = estimator.Evaluate(time - origin);
            if (snapshot.ready) {
                windows.push_back(snapshot);
                if (audit) audit << id << '\t' << keys << '\t' << level << '\t' << time << '\t' << snapshot.stars
                    << '\t' << snapshot.nps << '\t' << snapshot.burstNps << '\t' << snapshot.chordPercent
                    << '\t' << snapshot.jackPercent << '\t' << snapshot.holdOverlapPercent << '\t' << snapshot.maxChord
                    << '\t' << snapshot.rating << '\t' << snapshot.patternVariety << '\t' << snapshot.rhythmComplexity
                    << '\t' << snapshot.columnRepeatLoad << '\t' << snapshot.adjacentRepeatLoad << '\n';
            }
        }
        if (windows.size() < 3) continue;
        auto comparison = [keys](const auto& s) { return keys == 10 ? s.rating : s.stars; };
        std::sort(windows.begin(), windows.end(), [&](const auto& a, const auto& b) { return comparison(a) > comparison(b); });
        double load = 0, nps = 0;
        for (int i = 0; i < 3; ++i) { load += comparison(windows[i]) / 3.0; nps += windows[i].nps / 3.0; }
        std::sort(windows.begin(), windows.end(), [](const auto& a, const auto& b) { return a.score > b.score; });
        double baseline = 0;
        for (int i = 0; i < 3; ++i) baseline += windows[i].score / 3.0;
        output << id << '\t' << keys << '\t' << level << '\t' << group << '\t' << load << '\t' << baseline << '\t' << nps << '\n';
        ++accepted;
    }
    std::cout << "feature_rows=" << accepted << '\n';
}
