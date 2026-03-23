#pragma once
/**
 * @file file_io.h
 * @brief Small POSIX-friendly helpers for file IO patterns used in MPCC.
 *
 * Provides:
 *  - ensure_parent_dir(path): mkdir -p for the parent directory
 *  - read_all_lines(path)
 *  - atomic_write_lines(path, lines): write to .tmp then rename()
 */

#include <string>
#include <vector>

namespace FileIO {
    // Returns parent directory of a path, or empty string.
    std::string parent_dir(const std::string& path);

    // Create directory (and parents) if needed. Returns true on success.
    bool ensure_dir_recursive(const std::string& dir);

    // Ensure parent directory exists. Returns true on success or if no parent.
    bool ensure_parent_dir(const std::string& path);

    // Read file into lines (without trailing '\n'). Missing file => empty vector.
    std::vector<std::string> read_all_lines(const std::string& path);

    // Atomically write lines to path using path+".tmp" and rename().
    bool atomic_write_lines(const std::string& path,
                            const std::vector<std::string>& lines);
}

