#pragma once

#include <string>
#include <vector>
#include <optional>
#include <glm/vec3.hpp>

// Micro-DSL command schema (Phase 1 subset)
// Supported ops: load_model, duplicate, add_light

struct CmdTransform {
    std::optional<glm::vec3> position;
    std::optional<glm::vec3> scale;
    std::optional<glm::vec3> rotationEuler; // degrees, XYZ order
};

struct CmdLoadModel {
    std::string path;                 // required
    std::optional<std::string> name;  // optional
    CmdTransform transform;           // optional fields inside
};

struct CmdDuplicate {
    std::string source;               // name/id of source object
    std::optional<std::string> name;  // new name
    CmdTransform transform;           // delta transform to apply
};

struct CmdAddLight {
    std::string type;                 // "point" (default) or "directional"
    std::optional<glm::vec3> position;
    std::optional<glm::vec3> direction;
    std::optional<glm::vec3> color;   // default (1,1,1)
    std::optional<float> intensity;   // default 1.0
};

enum class CommandOp {
    LoadModel,
    Duplicate,
    AddLight
};

struct Command {
    CommandOp op;
    CmdLoadModel loadModel{};
    CmdDuplicate duplicate{};
    CmdAddLight addLight{};
};

struct CommandBatch {
    std::vector<Command> commands;
};

// Parsing/serialization API (minimal JSON-like, tolerant to whitespace)
// Returns true on success; on failure, fills error with a short message.
bool ParseCommandBatch(const std::string& json, CommandBatch& out, std::string& error);

// Convert a batch back to canonical JSON for preview/logging.
std::string ToJson(const CommandBatch& batch);

