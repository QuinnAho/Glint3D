// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/rendering_manager.h","purpose":"Tracks frame-level rendering parameters and writes the rendering UBO","exports":["RenderingManager"],"depends_on":["glint3d::uniform blocks"],"notes":["Maintains exposure, gamma, temporal state","Handles frame delta timing for shaders"]}
#pragma once

/**
 * @file rendering_manager.h
 * @brief Stores frame-level rendering settings and updates the rendering UBO.
 */

#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>

using glint3d::RenderingBlock;
using glint3d::RHI;
using glint3d::UniformAllocation;

// forward declarations
class IBLSystem;

enum class RenderToneMapMode;
enum class ShadingMode;

class RenderingManager {
public:
    RenderingManager();
    ~RenderingManager();

    // Initialization and cleanup
    bool init(RHI* rhi);
    void shutdown();

    // Rendering state management
    void updateRenderingState(float exposure, float gamma, RenderToneMapMode tonemap,
                             ShadingMode shadingMode, const IBLSystem* iblSystem = nullptr);
    void setObjectColor(const glm::vec3& color);
    void setExposure(float exposure);
    void setGamma(float gamma);
    void setToneMapping(RenderToneMapMode mode);
    void setShadingMode(ShadingMode mode);
    void setIBLIntensity(float intensity);

    // UBO binding
    void bindRenderingUniforms();

    // Getters for current state
    float getExposure() const;
    float getGamma() const;
    RenderToneMapMode getToneMapping() const;
    ShadingMode getShadingMode() const;
    const glm::vec3& getObjectColor() const { return m_renderingData.objectColor; }

private:
    RHI* m_rhi = nullptr;

    // UBO management
    UniformAllocation m_renderingBlock = {};
    RenderingBlock m_renderingData = {};

    void allocateUBO();
    void updateUBO();
};
