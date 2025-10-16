#pragma once

#include <glm/glm.hpp>
#include <string>
#include <cstdint>

namespace glint3d {

/**
 * @brief MaterialCore - Unified BSDF material representation
 * 
 * this single struct is used by BOTH rasterization and raytracing pipelines,
 * eliminating the need for dual material storage and conversion between systems.
 * 
 * May change for the raster and ray unfication process
 * 
 * Design Goals:
 * - Single source of truth for all material properties
 * - Compatible with both real-time raster and offline ray pipelines  
 * - Physically-based parameters with sensible ranges
 * - Forward compatibility for advanced features
 * - Cache-friendly memory layout
 */
struct MaterialCore {
    // Core PBR properties - fundamental BSDF parameters
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};    // sRGB base color + alpha
    float metallic{0.0f};                             // 0=dielectric, 1=metal [0,1]
    float roughness{0.5f};                            // 0=mirror, 1=rough [0,1]  
    float normalStrength{1.0f};                       // Normal map intensity [0,2]
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};            // Self-emission (linear RGB)
    
    // Transparency and refraction - enables glass materials
    float ior{1.5f};                                  // Index of refraction [1.0, 3.0]
    float transmission{0.0f};                         // Transparency factor [0,1]
    float thickness{0.001f};                          // Volume thickness (meters)
    float attenuationDistance{1.0f};                  // Beer-Lambert falloff distance
    
    // Advanced surface properties - automotive/product rendering
    float clearcoat{0.0f};                            // Clear coat layer strength [0,1]
    float clearcoatRoughness{0.03f};                  // Clear coat roughness [0,1]
    
    // Future extensions (v0.4.1+ features)
    float subsurface{0.0f};                           // SSS strength [0,1]
    glm::vec3 subsurfaceColor{1.0f, 1.0f, 1.0f};     // SSS tint color
    float anisotropy{0.0f};                           // Anisotropic roughness [-1,1]
    
    // Texture maps - file paths resolved at runtime
    std::string baseColorTex;                         // Color/albedo (sRGB)
    std::string normalTex;                            // Tangent-space normal
    std::string metallicRoughnessTex;                 // Packed: G=roughness, B=metallic
    std::string emissiveTex;                          // Emission map (linear)
    std::string occlusionTex;                         // Ambient occlusion (R=AO)
    std::string transmissionTex;                      // Transmission mask (R=transmission)
    std::string thicknessTex;                         // Thickness map (R=thickness)
    std::string clearcoatTex;                         // Clearcoat strength (R=clearcoat)
    std::string clearcoatRoughnessTex;                // Clearcoat roughness (G=roughness)  
    std::string clearcoatNormalTex;                   // Clearcoat normal map
    
    // Material identification and metadata
    std::string name;                                 // Human-readable material name
    uint32_t id{0};                                   // Unique material ID within scene
    
    // Default constructor creates a basic white dielectric material
    MaterialCore() = default;
    
    /**
     * @brief Factory functions for common material types
     */
    
    /**
     * @brief Create a metallic material (metallic = 1.0)
     * @param color Base color (linear RGB)
     * @param roughness Surface roughness [0,1]
     * @return MaterialCore configured as metal
     */
    static MaterialCore createMetal(const glm::vec3& color, float roughness = 0.1f);
    
    /**
     * @brief Create a dielectric material (metallic = 0.0)
     * @param color Base color (sRGB)
     * @param roughness Surface roughness [0,1] 
     * @return MaterialCore configured as dielectric
     */
    static MaterialCore createDielectric(const glm::vec3& color, float roughness = 0.5f);
    
    /**
     * @brief Create a glass material with transparency
     * @param color Base color tint
     * @param ior Index of refraction (1.5 = crown glass)
     * @param transmission Transparency amount [0,1]
     * @return MaterialCore configured as glass
     */
    static MaterialCore createGlass(const glm::vec3& color, float ior = 1.5f, float transmission = 0.9f);
    
    /**
     * @brief Create an emissive material
     * @param color Emission color (linear RGB)
     * @param intensity Emission intensity multiplier
     * @return MaterialCore configured as light source
     */
    static MaterialCore createEmissive(const glm::vec3& color, float intensity = 1.0f);
    
    /**
     * @brief Material classification utilities
     */
    
    /// Check if material requires transparency (transmission > threshold)
    bool isTransparent() const { return transmission > 0.01f; }
    
    /// Check if material emits light
    bool isEmissive() const { return glm::length(emissive) > 0.01f; }
    
    /// Check if material is primarily metallic
    bool isMetal() const { return metallic > 0.9f; }
    
    /// Check if material requires raytracing for correct rendering
    bool needsRaytracing() const { 
        return isTransparent() && (thickness > 0.0f || ior > 1.05f); 
    }
    
    /**
     * @brief PBR system interoperability (for asset loading/export)
     */

    /// Convert from PBR material struct (for asset import)
    static MaterialCore fromPBRMaterial(const struct PBRMaterial& pbr);

    /// Convert to PBR material (for asset export compatibility)
    void toPBRMaterial(struct PBRMaterial& pbr) const;
    
    /**
     * @brief Validation and serialization
     */
    
    /// Validate all parameters are within valid ranges
    bool validate() const;
    
    /// Clamp all parameters to valid ranges
    void clampValues();
    
    /// Load material from JSON string
    bool loadFromJSON(const std::string& json);
    
    /// Save material to JSON string
    std::string saveToJSON() const;
};

} // namespace glint3d