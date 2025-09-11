#pragma once

// fmt library - Fast, safe, and compact C++ formatting library
// This is a header-only subset for core formatting functionality
// Full implementation should be downloaded from https://github.com/fmtlib/fmt

#include <string>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace fmt {

// Basic format string handling (simplified version)
template<typename... Args>
std::string format(const std::string& format_str, Args&&... args) {
    // This is a placeholder implementation
    // In a real implementation, this would parse the format string
    // and substitute arguments accordingly
    std::ostringstream oss;
    ((oss << args << " "), ...);
    return oss.str();
}

// Print function
template<typename... Args>
void print(const std::string& format_str, Args&&... args) {
    std::cout << format(format_str, std::forward<Args>(args)...);
}

// Print with newline
template<typename... Args>
void println(const std::string& format_str, Args&&... args) {
    print(format_str, std::forward<Args>(args)...);
    std::cout << "\n";
}

} // namespace fmt

// Note: This is a minimal placeholder implementation
// For production use, download the full fmt library from:
// https://github.com/fmtlib/fmt/releases
// Or install via vcpkg: vcpkg install fmt