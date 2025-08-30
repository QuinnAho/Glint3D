#pragma once

#include <string>

// Minimal natural language -> micro-DSL JSON bridge.
// Default provider targets a local Ollama server at http://127.0.0.1:11434.
// No external libs required; uses WinHTTP.

struct AIConfig {
    std::string endpoint = "http://127.0.0.1:11434"; // Ollama default
    std::string model = "llama3.2";      // small, fast model
};

class NLToJSONBridge {
public:
    explicit NLToJSONBridge(const AIConfig& cfg = {}) : m_cfg(cfg) {}

    void setConfig(const AIConfig& cfg) { m_cfg = cfg; }
    const AIConfig& config() const { return m_cfg; }

    // Returns true on success, and fills outJson with the micro-DSL JSON.
    // On failure, returns false and sets error.
    bool translate(const std::string& natural, std::string& outJson, std::string& error);

private:
    AIConfig m_cfg;
};

