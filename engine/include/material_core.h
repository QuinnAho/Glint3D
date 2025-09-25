#pragma once

#include <glm/glm.hpp>
#include <string>

// MaterialCore - Unified BSDF material representation
// This single struct is used by BOTH rasterization and raytracing pipelines
// Eliminates the need for dual material storage and conversion between systems
struct MaterialCore {
    // Base material properties
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};  // sRGB base color + alpha
    float metallic{0.0f};                           // 0=dielectric, 1=metal [0,1]
    float roughness{0.5f};                          // 0=mirror, 1=rough [0,1]
    float normalStrength{1.0f};                     // Normal map intensity [0,2]
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};          // Self-emission (linear RGB)
    
    // Transparency and refraction
    float ior{1.5f};                                // Index of refraction [1.0, 3.0]
    float transmission{0.0f};                       // Transparency factor [0,1]
    float thickness{0.001f};                        // Volume thickness (meters)
    float attenuationDistance{1.0f};                // Beer-Lambert falloff distance
    
    // Advanced surface properties
    float clearcoat{0.0f};                          // Clear coat layer strength [0,1]
    float clearcoatRoughness{0.03f};                // Clear coat roughness [0,1]
    
    // Future extensions (placeholder for v0.4.1+)
    float subsurface{0.0f};                         // SSS strength [0,1]
    glm::vec3 subsurfaceColor{1.0f, 1.0f, 1.0f};   // SSS tint
    float anisotropy{0.0f};                         // Anisotropic roughness [-1,1]
    
    // Texture maps (file paths resolved at runtime)
    std::string baseColorTex;                       // Color/albedo (sRGB)
    std::string normalTex;                          // Tangent-space normal
    std::string metallicRoughnessTex;               // Packed: G=roughness, B=metallic
    std::string emissiveTex;                        // Emission map (linear)
    std::string occlusionTex;                       // Ambient occlusion (R=AO)
    std::string transmissionTex;                    // Transmission mask (R=transmission)
    std::string thicknessTex;                       // Thickness map (R=thickness)
    std::string clearcoatTex;                       // Clearcoat strength (R=clearcoat)
    std::string clearcoatRoughnessTex;              // Clearcoat roughness (G=roughness)
    std::string clearcoatNormalTex;                 // Clearcoat normal map
    
    // Material identification
    std::string name;                               // Human-readable material name
    uint32_t id{0};                                 // Unique material ID
    
    // Default constructor creates a basic dielectric material
    MaterialCore() = default;
    
    // Convenience constructors
    static MaterialCore createMetal(const glm::vec3& color, float roughness = 0.1f);
    static MaterialCore createDielectric(const glm::vec3& color, float roughness = 0.5f);
    static MaterialCore createGlass(const glm::vec3& color, float ior = 1.5f, float transmission = 0.9f);
    static MaterialCore createEmissive(const glm::vec3& color, float intensity = 1.0f);
    
    // Utility methods
    bool isTransparent() const { return transmission > 0.01f; }
    bool isEmissive() const { return glm::length(emissive) > 0.01f; }
    bool isMetal() const { return metallic > 0.9f; }
    bool needsRaytracing() const { return isTransparent() && (thickness > 0.0f || ior > 1.05f); }
    
    // Convert from PBR material system (for asset loading)
    static MaterialCore fromPBRMaterial(const struct PBRMaterial& pbr);

    // Convert to PBR system (for export compatibility)
    void toPBRMaterial(struct PBRMaterial& pbr) const;
    
    // Validation
    bool validate() const;
    void clampValues();
    
    // Serialization support
    bool loadFromJSON(const std::string& json);
    std::string saveToJSON() const;
};