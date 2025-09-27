#include "managers/material_manager.h"
#include "scene_manager.h"
#include "material_core.h"
#include <cstring>
#include <iostream>

MaterialManager::MaterialManager()
{
    setDefaultMaterial();
}

MaterialManager::~MaterialManager()
{
    shutdown();
}

bool MaterialManager::init(RHI* rhi)
{
    if (!rhi) {
        std::cerr << "MaterialManager::init: RHI is null" << std::endl;
        return false;
    }

    m_rhi = rhi;
    allocateUBO();
    return true;
}

void MaterialManager::shutdown()
{
    if (m_rhi && m_materialBlock.handle != INVALID_HANDLE) {
        m_rhi->freeUniforms(m_materialBlock);
        m_materialBlock = {};
    }
    m_rhi = nullptr;
}

void MaterialManager::updateMaterialForObject(const SceneObject& obj)
{
    if (!m_rhi) return;

    // Extract material data from SceneObject's MaterialCore
    const auto& mc = obj.materialCore;

    // Copy MaterialCore data to MaterialBlock
    m_materialData.baseColorFactor = mc.baseColor;
    m_materialData.metallicFactor = mc.metallic;
    m_materialData.roughnessFactor = mc.roughness;
    m_materialData.ior = mc.ior;
    m_materialData.transmission = mc.transmission;
    m_materialData.thickness = mc.thickness;
    m_materialData.attenuationDistance = mc.attenuationDistance;
    m_materialData.attenuationColor = glm::vec3(mc.baseColor); // Use base color RGB as attenuation
    m_materialData.clearcoat = mc.clearcoat;
    m_materialData.clearcoatRoughness = mc.clearcoatRoughness;

    // Set texture flags based on MaterialCore texture paths
    m_materialData.hasBaseColorMap = !mc.baseColorTex.empty() ? 1 : 0;
    m_materialData.hasNormalMap = !mc.normalTex.empty() ? 1 : 0;
    m_materialData.hasMRMap = !mc.metallicRoughnessTex.empty() ? 1 : 0;
    m_materialData.hasTangents = 0; // TODO: Determine from mesh data
    m_materialData.useTexture = (m_materialData.hasBaseColorMap || m_materialData.hasNormalMap || m_materialData.hasMRMap) ? 1 : 0;

    // Initialize padding
    m_materialData._padding1[0] = 0.0f;
    m_materialData._padding1[1] = 0.0f;
    m_materialData._padding2[0] = 0.0f;
    m_materialData._padding2[1] = 0.0f;
    m_materialData._padding2[2] = 0.0f;

    updateUBO();
}

void MaterialManager::updateMaterial(const MaterialCore& material)
{
    if (!m_rhi) return;

    // Copy MaterialCore data to MaterialBlock
    m_materialData.baseColorFactor = material.baseColor; // Already a vec4 with alpha
    m_materialData.metallicFactor = material.metallic;
    m_materialData.roughnessFactor = material.roughness;
    m_materialData.transmission = material.transmission;
    m_materialData.ior = material.ior;
    m_materialData.thickness = material.thickness;
    m_materialData.attenuationDistance = material.attenuationDistance;
    m_materialData.attenuationColor = glm::vec3(material.baseColor); // Use base color as default
    m_materialData.clearcoat = material.clearcoat;
    m_materialData.clearcoatRoughness = material.clearcoatRoughness;

    // Set texture flags based on MaterialCore texture paths
    m_materialData.hasBaseColorMap = !material.baseColorTex.empty() ? 1 : 0;
    m_materialData.hasNormalMap = !material.normalTex.empty() ? 1 : 0;
    m_materialData.hasMRMap = !material.metallicRoughnessTex.empty() ? 1 : 0;
    m_materialData.hasTangents = 0; // TODO: Determine from mesh data
    m_materialData.useTexture = (m_materialData.hasBaseColorMap || m_materialData.hasNormalMap || m_materialData.hasMRMap) ? 1 : 0;

    updateUBO();
}

void MaterialManager::bindMaterialUniforms()
{
    if (!m_rhi || m_materialBlock.handle == INVALID_HANDLE) {
        return;
    }

    // Bind the material UBO to binding point 2 (as defined in uniform_blocks.h)
    m_rhi->bindUniformBuffer(m_materialBlock.handle, 2);
}

void MaterialManager::allocateUBO()
{
    if (!m_rhi || m_materialBlock.handle != INVALID_HANDLE) {
        return;
    }

    UniformAllocationDesc desc{};
    desc.size = sizeof(MaterialBlock);
    desc.alignment = 256; // UBO alignment requirement
    desc.debugName = "MaterialBlock";

    m_materialBlock = m_rhi->allocateUniforms(desc);

    if (m_materialBlock.handle == INVALID_HANDLE) {
        std::cerr << "MaterialManager: Failed to allocate material UBO" << std::endl;
    }
}

void MaterialManager::updateUBO()
{
    if (!m_rhi || m_materialBlock.handle == INVALID_HANDLE || !m_materialBlock.mappedPtr) {
        return;
    }

    // Copy material data to the mapped UBO memory
    memcpy(m_materialBlock.mappedPtr, &m_materialData, sizeof(MaterialBlock));
}

void MaterialManager::setDefaultMaterial()
{
    // Set PBR material defaults
    m_materialData.baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
    m_materialData.metallicFactor = 0.0f;
    m_materialData.roughnessFactor = 0.5f;
    m_materialData.ior = 1.5f;
    m_materialData.transmission = 0.0f;
    m_materialData.thickness = 1.0f;
    m_materialData.attenuationDistance = 1000.0f;
    m_materialData.attenuationColor = glm::vec3(1.0f);
    m_materialData.clearcoat = 0.0f;
    m_materialData.clearcoatRoughness = 0.0f;

    // No textures by default
    m_materialData.hasBaseColorMap = 0;
    m_materialData.hasNormalMap = 0;
    m_materialData.hasMRMap = 0;
    m_materialData.hasTangents = 0;
    m_materialData.useTexture = 0;

    // Initialize padding
    m_materialData._padding1[0] = 0.0f;
    m_materialData._padding1[1] = 0.0f;
    m_materialData._padding2[0] = 0.0f;
    m_materialData._padding2[1] = 0.0f;
    m_materialData._padding2[2] = 0.0f;
}