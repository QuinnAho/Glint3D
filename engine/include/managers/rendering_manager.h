#pragma once

#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>

using namespace glint3d;

// Forward declarations
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