#pragma once

// spdlog - Fast C++ logging library
// This is a header-only subset for basic logging functionality
// Full implementation should be downloaded from https://github.com/gabime/spdlog

#include <iostream>
#include <string>
#include <memory>
#include <sstream>

namespace spdlog {

// Log levels
enum class level {
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    err = 4,
    critical = 5,
    off = 6
};

// Basic logger interface
class logger {
public:
    virtual ~logger() = default;
    
    template<typename... Args>
    void trace(const std::string& msg, Args&&... args) {
        if (level_ <= level::trace) {
            log_impl("TRACE", msg, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void debug(const std::string& msg, Args&&... args) {
        if (level_ <= level::debug) {
            log_impl("DEBUG", msg, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void info(const std::string& msg, Args&&... args) {
        if (level_ <= level::info) {
            log_impl("INFO", msg, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void warn(const std::string& msg, Args&&... args) {
        if (level_ <= level::warn) {
            log_impl("WARN", msg, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void error(const std::string& msg, Args&&... args) {
        if (level_ <= level::err) {
            log_impl("ERROR", msg, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void critical(const std::string& msg, Args&&... args) {
        if (level_ <= level::critical) {
            log_impl("CRITICAL", msg, std::forward<Args>(args)...);
        }
    }
    
    void set_level(level lvl) { level_ = lvl; }
    level get_level() const { return level_; }

private:
    level level_ = level::info;
    
    template<typename... Args>
    void log_impl(const char* level_name, const std::string& msg, Args&&... args) {
        std::ostringstream oss;
        oss << "[" << level_name << "] " << msg;
        ((oss << " " << args), ...);
        std::cout << oss.str() << std::endl;
    }
};

// Basic console logger
class console_logger : public logger {
public:
    console_logger(const std::string& name) : name_(name) {}
    const std::string& name() const { return name_; }
    
private:
    std::string name_;
};

// Global logger functions
inline std::shared_ptr<logger> default_logger() {
    static auto logger = std::make_shared<console_logger>("default");
    return logger;
}

inline void set_default_logger(std::shared_ptr<logger> new_logger) {
    // Implementation would set global default logger
}

inline std::shared_ptr<logger> get(const std::string& name) {
    // Implementation would return named logger
    return std::make_shared<console_logger>(name);
}

// Convenience functions that use default logger
template<typename... Args>
inline void trace(const std::string& msg, Args&&... args) {
    default_logger()->trace(msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline void debug(const std::string& msg, Args&&... args) {
    default_logger()->debug(msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline void info(const std::string& msg, Args&&... args) {
    default_logger()->info(msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline void warn(const std::string& msg, Args&&... args) {
    default_logger()->warn(msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(const std::string& msg, Args&&... args) {
    default_logger()->error(msg, std::forward<Args>(args)...);
}

template<typename... Args>
inline void critical(const std::string& msg, Args&&... args) {
    default_logger()->critical(msg, std::forward<Args>(args)...);
}

inline void set_level(level lvl) {
    default_logger()->set_level(lvl);
}

} // namespace spdlog

// Note: This is a minimal placeholder implementation
// For production use, download the full spdlog library from:
// https://github.com/gabime/spdlog/releases
// Or install via vcpkg: vcpkg install spdlog