#include "managers/lighting_manager.h"
#include "light.h"
#include <cstring>
#include <iostream>

LightingManager::LightingManager()
{
    // Initialize with sensible defaults
    m_lightingData.numLights = 0;
    m_lightingData.viewPos = glm::vec3(0.0f);
    m_lightingData.globalAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

    // Initialize all lights to disabled state
    for (int i = 0; i < 10; ++i) {
        auto& lightData = m_lightingData.lights[i];
        lightData.type = 0; // Point light
        lightData.position = glm::vec3(0.0f, 5.0f, 0.0f);
        lightData.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        lightData.color = glm::vec3(1.0f);
        lightData.intensity = 0.0f; // Disabled by default
        lightData.innerCutoff = 0.9f;
        lightData.outerCutoff = 0.8f;
        lightData._padding = 0.0f;
    }
}

LightingManager::~LightingManager()
{
    shutdown();
}

bool LightingManager::init(RHI* rhi)
{
    if (!rhi) {
        std::cerr << "LightingManager::init: RHI is null" << std::endl;
        return false;
    }

    m_rhi = rhi;
    allocateUBO();
    return true;
}

void LightingManager::shutdown()
{
    if (m_rhi && m_lightingBlock.handle != INVALID_HANDLE) {
        m_rhi->freeUniforms(m_lightingBlock);
        m_lightingBlock = {};
    }
    m_rhi = nullptr;
}

void LightingManager::updateLighting(const Light& lights, const glm::vec3& viewPos)
{
    if (!m_rhi) return;

    // Update basic lighting data
    m_lightingData.numLights = static_cast<int>(lights.getLightCount());
    m_lightingData.viewPos = viewPos;
    m_lightingData.globalAmbient = lights.m_globalAmbient;

    // Extract actual light data from Light object
    size_t lightCount = std::min(static_cast<size_t>(10), lights.getLightCount());

    for (size_t i = 0; i < lightCount; ++i) {
        const auto& lightSource = lights.m_lights[i];
        auto& lightData = m_lightingData.lights[i];

        // Convert LightSource to LightData format
        lightData.type = static_cast<int>(lightSource.type);
        lightData.position = lightSource.position;
        lightData.direction = lightSource.direction;
        lightData.color = lightSource.color;
        lightData.intensity = lightSource.enabled ? lightSource.intensity : 0.0f;

        // Convert cone angles from degrees to cosine values for spot lights
        if (lightSource.type == LightType::SPOT) {
            lightData.innerCutoff = glm::cos(glm::radians(lightSource.innerConeDeg));
            lightData.outerCutoff = glm::cos(glm::radians(lightSource.outerConeDeg));
        } else {
            lightData.innerCutoff = 0.0f;
            lightData.outerCutoff = 0.0f;
        }
        lightData._padding = 0.0f;
    }

    // Disable remaining lights
    for (int i = static_cast<int>(lightCount); i < 10; ++i) {
        m_lightingData.lights[i].intensity = 0.0f;
    }

    updateUBO();
}

void LightingManager::bindLightingUniforms()
{
    if (!m_rhi || m_lightingBlock.handle == INVALID_HANDLE) {
        return;
    }

    // Bind the lighting UBO to binding point 1 (as defined in uniform_blocks.h)
    m_rhi->bindUniformBuffer(m_lightingBlock.handle, 1);
}

void LightingManager::allocateUBO()
{
    if (!m_rhi || m_lightingBlock.handle != INVALID_HANDLE) {
        return;
    }

    UniformAllocationDesc desc{};
    desc.size = sizeof(LightingBlock);
    desc.alignment = 256; // UBO alignment requirement
    desc.debugName = "LightingBlock";

    m_lightingBlock = m_rhi->allocateUniforms(desc);

    if (m_lightingBlock.handle == INVALID_HANDLE) {
        std::cerr << "LightingManager: Failed to allocate lighting UBO" << std::endl;
    }
}

void LightingManager::updateUBO()
{
    if (!m_rhi || m_lightingBlock.handle == INVALID_HANDLE || !m_lightingBlock.mappedPtr) {
        return;
    }

    // Copy lighting data to the mapped UBO memory
    memcpy(m_lightingBlock.mappedPtr, &m_lightingData, sizeof(LightingBlock));
}