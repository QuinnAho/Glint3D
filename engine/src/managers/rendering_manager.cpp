#include "managers/rendering_manager.h"
#include "render_system.h" // For enums
#include "ibl_system.h"
#include <cstring>
#include <iostream>

RenderingManager::RenderingManager()
{
    // Initialize with sensible defaults
    m_renderingData.exposure = 0.0f;
    m_renderingData.gamma = 2.2f;
    m_renderingData.toneMappingMode = static_cast<int>(RenderToneMapMode::Linear);
    m_renderingData.shadingMode = static_cast<int>(ShadingMode::Gouraud);
    m_renderingData.iblIntensity = 1.0f;
    m_renderingData.objectColor = glm::vec3(1.0f);
}

RenderingManager::~RenderingManager()
{
    shutdown();
}

bool RenderingManager::init(RHI* rhi)
{
    if (!rhi) {
        std::cerr << "RenderingManager::init: RHI is null" << std::endl;
        return false;
    }

    m_rhi = rhi;
    allocateUBO();
    return true;
}

void RenderingManager::shutdown()
{
    if (m_rhi && m_renderingBlock.handle != INVALID_HANDLE) {
        m_rhi->freeUniforms(m_renderingBlock);
        m_renderingBlock = {};
    }
    m_rhi = nullptr;
}

void RenderingManager::updateRenderingState(float exposure, float gamma, RenderToneMapMode tonemap,
                                           ShadingMode shadingMode, const IBLSystem* iblSystem)
{
    if (!m_rhi) return;

    m_renderingData.exposure = exposure;
    m_renderingData.gamma = gamma;
    m_renderingData.toneMappingMode = static_cast<int>(tonemap);
    m_renderingData.shadingMode = static_cast<int>(shadingMode);
    m_renderingData.iblIntensity = iblSystem ? iblSystem->getIntensity() : 1.0f;

    updateUBO();
}

void RenderingManager::setObjectColor(const glm::vec3& color)
{
    if (!m_rhi) return;

    m_renderingData.objectColor = color;
    updateUBO();
}

void RenderingManager::setExposure(float exposure)
{
    if (!m_rhi) return;

    m_renderingData.exposure = exposure;
    updateUBO();
}

void RenderingManager::setGamma(float gamma)
{
    if (!m_rhi) return;

    m_renderingData.gamma = gamma;
    updateUBO();
}

void RenderingManager::setToneMapping(RenderToneMapMode mode)
{
    if (!m_rhi) return;

    m_renderingData.toneMappingMode = static_cast<int>(mode);
    updateUBO();
}

void RenderingManager::setShadingMode(ShadingMode mode)
{
    if (!m_rhi) return;

    m_renderingData.shadingMode = static_cast<int>(mode);
    updateUBO();
}

void RenderingManager::setIBLIntensity(float intensity)
{
    if (!m_rhi) return;

    m_renderingData.iblIntensity = intensity;
    updateUBO();
}

void RenderingManager::bindRenderingUniforms()
{
    if (!m_rhi || m_renderingBlock.handle == INVALID_HANDLE) {
        return;
    }

    // Bind the rendering UBO to binding point 3 (as defined in uniform_blocks.h)
    m_rhi->bindUniformBuffer(m_renderingBlock.handle, 3);
}

float RenderingManager::getExposure() const
{
    return m_renderingData.exposure;
}

float RenderingManager::getGamma() const
{
    return m_renderingData.gamma;
}

RenderToneMapMode RenderingManager::getToneMapping() const
{
    return static_cast<RenderToneMapMode>(m_renderingData.toneMappingMode);
}

ShadingMode RenderingManager::getShadingMode() const
{
    return static_cast<ShadingMode>(m_renderingData.shadingMode);
}

void RenderingManager::allocateUBO()
{
    if (!m_rhi || m_renderingBlock.handle != INVALID_HANDLE) {
        return;
    }

    UniformAllocationDesc desc{};
    desc.size = sizeof(RenderingBlock);
    desc.alignment = 256; // UBO alignment requirement
    desc.debugName = "RenderingBlock";

    m_renderingBlock = m_rhi->allocateUniforms(desc);

    if (m_renderingBlock.handle == INVALID_HANDLE) {
        std::cerr << "RenderingManager: Failed to allocate rendering UBO" << std::endl;
    }
}

void RenderingManager::updateUBO()
{
    if (!m_rhi || m_renderingBlock.handle == INVALID_HANDLE || !m_renderingBlock.mappedPtr) {
        return;
    }

    // Copy rendering data to the mapped UBO memory
    memcpy(m_renderingBlock.mappedPtr, &m_renderingData, sizeof(RenderingBlock));
}