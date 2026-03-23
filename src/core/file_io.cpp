/**
 * @file file_io.cpp
 * @brief Implementation of FileIO helpers (POSIX).
 */

#include "mpcc/core/file_io.h"

#include <fstream>
#include <cstdio>      // rename()
#include <sys/stat.h>  // mkdir
#include <cerrno>

namespace FileIO {
    std::string parent_dir(const std::string& path) {
        std::size_t slash = path.rfind('/');
        if (slash == std::string::npos) return "";
        if (slash == 0) return "/";
        return path.substr(0, slash);
    }

    static bool mkdir_one(const std::string& dir) {
        if (dir.empty()) return true;
        if (::mkdir(dir.c_str(), 0755) == 0) return true;
        return errno == EEXIST;
    }

    bool ensure_dir_recursive(const std::string& dir) {
        if (dir.empty()) return true;
        if (dir == "/") return true;

        // Build incrementally: a/b/c
        std::string cur;
        cur.reserve(dir.size());

        std::size_t i = 0;
        if (!dir.empty() && dir[0] == '/') {
            cur = "/";
            i = 1;
        }

        for (; i < dir.size(); ++i) {
            char ch = dir[i];
            if (ch == '/') {
                if (!mkdir_one(cur)) return false;
            }
            cur.push_back(ch);
        }
        if (!mkdir_one(cur)) return false;
        return true;
    }

    bool ensure_parent_dir(const std::string& path) {
        std::string parent = parent_dir(path);
        if (parent.empty()) return true;
        return ensure_dir_recursive(parent);
    }

    std::vector<std::string> read_all_lines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream ifs(path);
        if (!ifs.is_open()) return lines;

        std::string line;
        while (std::getline(ifs, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    bool atomic_write_lines(const std::string& path,
                            const std::vector<std::string>& lines) {
        if (!ensure_parent_dir(path)) return false;

        const std::string tmp_path = path + ".tmp";
        {
            std::ofstream ofs(tmp_path);
            if (!ofs.is_open()) return false;
            for (const auto& l : lines) {
                ofs << l << "\n";
            }
            if (!ofs.good()) return false;
        }
        return ::rename(tmp_path.c_str(), path.c_str()) == 0;
    }
}

