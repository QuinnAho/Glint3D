#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

class Application;

// Small execution context for handlers
struct CommandCtx {
    Application& app;
    std::string  line;                 // original input
    std::vector<std::string> tokens;   // tokenized (argv-like)
};

// Command handler signature:
// return true if a visible change happened; push human-readable lines into logs
using CommandHandler = std::function<bool(const CommandCtx&, std::vector<std::string>&)>;

class AppCommands {
public:
    AppCommands() = default;

    // Bind to an application instance
    void attach(Application& app);

    // Add/override a command
    void add(const std::string& name, CommandHandler fn, const std::string& helpLine);

    // Parse + dispatch a line of text
    bool execute(const std::string& line, std::vector<std::string>& logs);

    // Get one-line help for each command
    std::vector<std::string> helpLines() const;

private:
    Application* app_ = nullptr;
    std::unordered_map<std::string, CommandHandler> cmds_;
    std::unordered_map<std::string, std::string> help_;

    static std::vector<std::string> tokenize(const std::string& line);
    static bool strEq(const std::string& a, const char* b);
    static bool parseVec3(const std::vector<std::string>& t, size_t i, glm::vec3& out);
    static bool parseFloat(const std::vector<std::string>& t, size_t i, float& out);
    static bool parseInt(const std::vector<std::string>& t, size_t i, int& out);
};

// These register all the built-in commands on an AppCommands
// Implemented in app_commands.cpp and called from Application::bindUiCommands_()
void RegisterDefaultCommands(AppCommands& R);
