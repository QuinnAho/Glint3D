#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "render_settings.h"

enum class CLIExitCode : int {
    Success = 0,
    SchemaValidationError = 2,
    FileNotFound = 3,
    RuntimeError = 4,
    UnknownFlag = 5
};

enum class LogLevel {
    Quiet,
    Warn,
    Info,
    Debug
};

struct CLIOptions {
    bool showHelp = false;
    bool showVersion = false;
    bool headlessMode = false;
    bool enableDenoise = false;
    bool forceRaytrace = false;
    bool strictSchema = false;
    // New unified render mode flag (raster|ray|auto). '--mode' overrides '--raytrace'
    std::string mode = "auto";
    
    std::string opsFile;
    std::string outputFile;
    std::string schemaVersion = "v1.3";
    std::string assetRoot;
    LogLevel logLevel = LogLevel::Info;
    
    int outputWidth = 1024;
    int outputHeight = 1024;
    int reflectionSpp = 8; // Default reflection samples per pixel
    
    // Render settings
    RenderSettings renderSettings;
};

struct CLIParseResult {
    CLIExitCode exitCode = CLIExitCode::Success;
    std::string errorMessage;
    CLIOptions options;
};

class CLIParser {
public:
    static CLIParseResult parse(int argc, char** argv);
    static void printHelp();
    static void printVersion();
    static const char* exitCodeToString(CLIExitCode code);
    
private:
    static bool isValidFlag(const std::string& flag);
    static bool isValidLogLevel(const std::string& level);
    static bool isValidSchemaVersion(const std::string& version);
    static bool isValidSeed(const std::string& seed);
    static bool isValidExposure(const std::string& exposure);
    static bool isValidGamma(const std::string& gamma);
    static LogLevel parseLogLevel(const std::string& level);
    static uint32_t parseSeed(const std::string& seed);
    static float parseExposure(const std::string& exposure);
    static float parseGamma(const std::string& gamma);
    static std::vector<std::string> getValidFlags();
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    
private:
    static LogLevel s_currentLevel;
    static void log(LogLevel level, const std::string& prefix, const std::string& message);
    static std::string getCurrentTimestamp();
};
