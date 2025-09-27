#pragma once
#include <glm/glm.hpp>
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>

using namespace glint3d;

// Forward declarations
struct SceneObject;
struct MaterialCore;

/**
 * MaterialManager encapsulates material state and uniform buffer management.
 * This includes the material UBO and per-object material updates.
 */
class MaterialManager {
public:
    MaterialManager();
    ~MaterialManager();

    // Initialize/shutdown
    bool init(RHI* rhi);
    void shutdown();

    // Update material data for a specific object
    void updateMaterialForObject(const SceneObject& obj);

    // Update material data from MaterialCore
    void updateMaterial(const MaterialCore& material);

    // Bind material UBO to the current shader pipeline
    void bindMaterialUniforms();

    // Access to material block data (read-only)
    const MaterialBlock& getMaterialData() const { return m_materialData; }

    // Configuration helpers
    void setBaseColor(const glm::vec4& color) { m_materialData.baseColorFactor = color; }
    void setMetallicRoughness(float metallic, float roughness) {
        m_materialData.metallicFactor = metallic;
        m_materialData.roughnessFactor = roughness;
    }
    void setTransmission(float transmission) { m_materialData.transmission = transmission; }
    void setIOR(float ior) { m_materialData.ior = ior; }
    void setThickness(float thickness) { m_materialData.thickness = thickness; }
    void setClearcoat(float clearcoat, float roughness) {
        m_materialData.clearcoat = clearcoat;
        m_materialData.clearcoatRoughness = roughness;
    }

private:
    RHI* m_rhi = nullptr;

    // UBO allocation and data
    UniformAllocation m_materialBlock = {};
    MaterialBlock m_materialData = {};

    // Helper methods
    void allocateUBO();
    void updateUBO();
    void setDefaultMaterial();
};