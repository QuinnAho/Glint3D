#pragma once
#include <string>

namespace PathUtils {

/**
 * Resolve a relative path by finding the project root directory.
 * Searches up the directory tree for CMakeLists.txt and engine/ directory.
 * @param relativePath Path relative to project root (e.g., "engine/shaders/pbr.vert")
 * @return Absolute path if found, empty string if not found
 */
std::string resolveProjectPath(const std::string& relativePath);

/**
 * Resolve an asset path by trying direct access first, then project-relative.
 * @param path File path to resolve
 * @return Resolved path if found, original path otherwise
 */
std::string resolveAssetPath(const std::string& path);

} // namespace PathUtils