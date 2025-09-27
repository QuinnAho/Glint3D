#pragma once
#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>

using namespace glint3d;

// Forward declarations
class Light;

/**
 * LightingManager encapsulates lighting state and uniform buffer management.
 * This includes the lighting UBO and all related update routines.
 */
class LightingManager {
public:
    LightingManager();
    ~LightingManager();

    // Initialize/shutdown
    bool init(RHI* rhi);
    void shutdown();

    // Update lighting data from Light system
    void updateLighting(const Light& lights, const glm::vec3& viewPos);

    // Bind lighting UBO to the current shader pipeline
    void bindLightingUniforms();

    // Access to lighting block data (read-only)
    const LightingBlock& getLightingData() const { return m_lightingData; }

    // Configuration
    void setGlobalAmbient(const glm::vec4& ambient) { m_lightingData.globalAmbient = ambient; }
    glm::vec4 getGlobalAmbient() const { return m_lightingData.globalAmbient; }

private:
    RHI* m_rhi = nullptr;

    // UBO allocation and data
    UniformAllocation m_lightingBlock = {};
    LightingBlock m_lightingData = {};

    // Helper methods
    void allocateUBO();
    void updateUBO();
};