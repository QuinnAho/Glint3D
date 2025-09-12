#include <string>
#include <vector>
#include <glm/glm.hpp>

// SSR-T stub pass: placeholder to enable incremental wiring in PR2.
// Note: Do not include UI, GLFW, or platform headers here (per overlay constraints).

namespace render {

struct SSRPassConfig {
    int width = 0;
    int height = 0;
    float maxDistance = 5.0f;
    int maxSteps = 64;
    float thickness = 0.0f;
    float roughness = 0.0f;
};

class SSRPass {
public:
    bool init(const SSRPassConfig& cfg) {
        m_cfg = cfg;
        // Resources will be created when wired into the render path.
        return true;
    }

    void shutdown() {
        // Destroy resources when implemented.
    }

    void execute() {
        // No-op stub: actual SSR-T will be implemented in subsequent PRs.
    }

private:
    SSRPassConfig m_cfg{};
};

} // namespace render

