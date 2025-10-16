// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/lighting_manager.h","purpose":"Manages light data and uploads lighting uniform buffers","exports":["LightingManager"],"depends_on":["glm","glint3d::uniform blocks","SceneManager"],"notes":["Builds light arrays for the GPU","Tracks active light selection state"]}
#pragma once

/**
 * @file lighting_manager.h
 * @brief Aggregates scene lights and pushes them into the lighting UBO.
 */

#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>

using glint3d::LightingBlock;
using glint3d::RHI;
using glint3d::UniformAllocation;

// forward declarations
class Light;

/**
 * LightingManager encapsulates lighting state and uniform buffer management.
 * This includes the lighting UBO and all related update routines.
 */
class LightingManager {
public:
    LightingManager();
    ~LightingManager();

    // initialize/shutdown
    bool init(RHI* rhi);
    void shutdown();

    // update lighting data from light system
    void updateLighting(const Light& lights, const glm::vec3& viewPos);

    // bind lighting UBO to the current shader pipeline
    void bindLightingUniforms();

    // access to lighting block data (read-only)
    const LightingBlock& getLightingData() const { return m_lightingData; }

    // configuration
    void setGlobalAmbient(const glm::vec4& ambient) { m_lightingData.globalAmbient = ambient; }
    glm::vec4 getGlobalAmbient() const { return m_lightingData.globalAmbient; }

private:
    RHI* m_rhi = nullptr;

    // UBO allocation and data
    UniformAllocation m_lightingBlock = {};
    LightingBlock m_lightingData = {};

    // helper methods
    void allocateUBO();
    void updateUBO();
};
