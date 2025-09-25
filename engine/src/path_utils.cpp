#include "path_utils.h"
#include <filesystem>
#include <iostream>

namespace PathUtils {

std::string resolveProjectPath(const std::string& relativePath) {
    namespace fs = std::filesystem;

    // Start from current directory and walk up to find project root
    fs::path currentDir = fs::current_path();
    fs::path searchDir = currentDir;

    // Look for project markers (CMakeLists.txt indicates project root)
    for (int i = 0; i < 10; ++i) { // Limit search depth
        fs::path cmakeFile = searchDir / "CMakeLists.txt";
        fs::path engineDir = searchDir / "engine";

        // Check if this looks like the project root
        if (fs::exists(cmakeFile) && fs::exists(engineDir)) {
            fs::path resolvedPath = searchDir / relativePath;
            if (fs::exists(resolvedPath)) {
                return resolvedPath.string();
            }
        }

        // Move up one directory
        fs::path parent = searchDir.parent_path();
        if (parent == searchDir) break; // Reached filesystem root
        searchDir = parent;
    }

    return ""; // Not found
}

std::string resolveAssetPath(const std::string& path) {
    namespace fs = std::filesystem;

    // Try the path directly first
    if (fs::exists(path)) {
        return path;
    }

    // Try to resolve relative to project root
    std::string resolved = resolveProjectPath(path);
    if (!resolved.empty()) {
        return resolved;
    }

    // Return original path if nothing found
    return path;
}

} // namespace PathUtils