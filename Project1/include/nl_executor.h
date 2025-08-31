#pragma once

#include <string>
#include <vector>
#include <optional>
#include <glm/vec3.hpp>

class Application;

namespace nl {

// Very small natural-language command executor.
// Pattern-based parsing for common actions at runtime:
// - place/load OBJ in front of camera or at coordinates
// - add point light
// - create material and assign it to an object
// - toggle fullscreen
//
// TODO: Expand with primitives generation (cube, sphere, plane) and modifiers.
class Executor {
public:
    explicit Executor(Application& app) : m_app(app) {}

    // Execute a single natural-language instruction. Appends logs into outLog.
    // Returns true if something was recognized and executed.
    bool execute(const std::string& text, std::vector<std::string>& outLog);

private:
    Application& m_app;

    // Helpers
    static std::string trim(const std::string& s);
    static bool iequalsPrefix(const std::string& s, const std::string& prefix);
    static std::optional<glm::vec3> parseVec3(const std::string& s);
    static std::vector<std::string> tokenize(const std::string& s);
};

} // namespace nl

