#pragma once
#include <filesystem>
#include <vector>

namespace self_test_io {
// Both construction and increment can fail. A range-for would still use the
// throwing increment even when its directory_iterator was given an error_code.
inline bool HtmlReports(const std::filesystem::path& directory,
    std::vector<std::filesystem::path>& files, std::error_code& error) {
    files.clear();
    std::filesystem::directory_iterator it(directory, error), end;
    while (!error && it != end) {
        if (it->path().extension() == L".html") files.push_back(it->path());
        it.increment(error);
    }
    return !error;
}

// Only a newly created, direct child of the caller's test directory is eligible.
inline bool RemoveOwnedDirectory(const std::filesystem::path& path,
    const std::filesystem::path& parent, std::error_code& error) {
    const auto expected = std::filesystem::weakly_canonical(parent, error);
    if (error) return false;
    const auto target = std::filesystem::weakly_canonical(path, error);
    if (error) return false;
    if (target == expected || target.parent_path() != expected) {
        error = std::make_error_code(std::errc::invalid_argument);
        return false;
    }
    std::filesystem::remove_all(target, error);
    return !error;
}
}
