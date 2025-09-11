#include "material_core.h"
#include "material.h"
#include "pbr_material.h"
#include <algorithm>
#include <iostream>

MaterialCore MaterialCore::createMetal(const glm::vec3& color, float roughness) {
    MaterialCore material;
    material.baseColor = glm::vec4(color, 1.0f);
    material.metallic = 1.0f;
    material.roughness = std::clamp(roughness, 0.0f, 1.0f);
    material.ior = 1.0f; // Metals don't use IOR in the traditional sense
    material.transmission = 0.0f;
    material.name = "Metal";
    return material;
}

MaterialCore MaterialCore::createDielectric(const glm::vec3& color, float roughness) {
    MaterialCore material;
    material.baseColor = glm::vec4(color, 1.0f);
    material.metallic = 0.0f;
    material.roughness = std::clamp(roughness, 0.0f, 1.0f);
    material.ior = 1.5f;
    material.transmission = 0.0f;
    material.name = "Dielectric";
    return material;
}

MaterialCore MaterialCore::createGlass(const glm::vec3& color, float ior, float transmission) {
    MaterialCore material;
    material.baseColor = glm::vec4(color, 1.0f);
    material.metallic = 0.0f;
    material.roughness = 0.0f; // Perfect glass is smooth
    material.ior = std::clamp(ior, 1.0f, 3.0f);
    material.transmission = std::clamp(transmission, 0.0f, 1.0f);
    material.thickness = 0.005f; // 5mm default thickness
    material.name = "Glass";
    return material;
}

MaterialCore MaterialCore::createEmissive(const glm::vec3& color, float intensity) {
    MaterialCore material;
    material.baseColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Black base
    material.emissive = color * intensity;
    material.metallic = 0.0f;
    material.roughness = 1.0f;
    material.name = "Emissive";
    return material;
}

MaterialCore MaterialCore::fromLegacyMaterial(const Material& legacy) {
    MaterialCore material;
    
    // Map legacy diffuse to base color
    material.baseColor = glm::vec4(legacy.diffuse, 1.0f);
    
    // Convert legacy properties
    material.metallic = legacy.metallic;
    material.roughness = legacy.roughness;
    material.ior = legacy.ior;
    material.transmission = legacy.transmission;
    
    // Legacy materials don't have these advanced properties
    material.normalStrength = 1.0f;
    material.emissive = glm::vec3(0.0f);
    material.thickness = 0.001f;
    material.attenuationDistance = 1.0f;
    material.clearcoat = 0.0f;
    material.clearcoatRoughness = 0.03f;
    
    material.name = "Legacy";
    return material;
}

MaterialCore MaterialCore::fromPBRMaterial(const PBRMaterial& pbr) {
    MaterialCore material;
    
    // Direct mapping of PBR properties
    material.baseColor = pbr.baseColorFactor;
    material.metallic = pbr.metallicFactor;
    material.roughness = pbr.roughnessFactor;
    material.ior = pbr.ior;
    
    // Map texture paths
    material.baseColorTex = pbr.baseColorTex;
    material.normalTex = pbr.normalTex;
    material.metallicRoughnessTex = pbr.mrTex;
    material.occlusionTex = pbr.aoTex;
    
    // PBR materials don't have transmission by default
    material.transmission = 0.0f;
    material.thickness = 0.001f;
    material.attenuationDistance = 1.0f;
    
    material.name = "PBR";
    return material;
}

void MaterialCore::toLegacyMaterial(Material& legacy) const {
    legacy.diffuse = glm::vec3(baseColor);
    legacy.specular = glm::vec3(1.0f); // Simplified
    legacy.ambient = glm::vec3(0.1f); // Simplified
    legacy.shininess = (1.0f - roughness) * 128.0f; // Rough approximation
    legacy.metallic = metallic;
    legacy.roughness = roughness;
    legacy.ior = ior;
    legacy.transmission = transmission;
}

void MaterialCore::toPBRMaterial(PBRMaterial& pbr) const {
    pbr.baseColorFactor = baseColor;
    pbr.metallicFactor = metallic;
    pbr.roughnessFactor = roughness;
    pbr.ior = ior;
    
    pbr.baseColorTex = baseColorTex;
    pbr.normalTex = normalTex;
    pbr.mrTex = metallicRoughnessTex;
    pbr.aoTex = occlusionTex;
}

bool MaterialCore::validate() const {
    // Check value ranges
    if (metallic < 0.0f || metallic > 1.0f) {
        std::cerr << "[MaterialCore] Invalid metallic value: " << metallic << std::endl;
        return false;
    }
    
    if (roughness < 0.0f || roughness > 1.0f) {
        std::cerr << "[MaterialCore] Invalid roughness value: " << roughness << std::endl;
        return false;
    }
    
    if (ior < 1.0f || ior > 3.0f) {
        std::cerr << "[MaterialCore] Invalid IOR value: " << ior << std::endl;
        return false;
    }
    
    if (transmission < 0.0f || transmission > 1.0f) {
        std::cerr << "[MaterialCore] Invalid transmission value: " << transmission << std::endl;
        return false;
    }
    
    if (clearcoat < 0.0f || clearcoat > 1.0f) {
        std::cerr << "[MaterialCore] Invalid clearcoat value: " << clearcoat << std::endl;
        return false;
    }
    
    if (clearcoatRoughness < 0.0f || clearcoatRoughness > 1.0f) {
        std::cerr << "[MaterialCore] Invalid clearcoat roughness value: " << clearcoatRoughness << std::endl;
        return false;
    }
    
    // Check for NaN values
    if (glm::any(glm::isnan(baseColor))) {
        std::cerr << "[MaterialCore] Base color contains NaN values" << std::endl;
        return false;
    }
    
    if (glm::any(glm::isnan(emissive))) {
        std::cerr << "[MaterialCore] Emissive color contains NaN values" << std::endl;
        return false;
    }
    
    return true;
}

void MaterialCore::clampValues() {
    metallic = std::clamp(metallic, 0.0f, 1.0f);
    roughness = std::clamp(roughness, 0.0f, 1.0f);
    normalStrength = std::clamp(normalStrength, 0.0f, 2.0f);
    ior = std::clamp(ior, 1.0f, 3.0f);
    transmission = std::clamp(transmission, 0.0f, 1.0f);
    thickness = std::max(thickness, 0.0f);
    attenuationDistance = std::max(attenuationDistance, 0.001f);
    clearcoat = std::clamp(clearcoat, 0.0f, 1.0f);
    clearcoatRoughness = std::clamp(clearcoatRoughness, 0.0f, 1.0f);
    subsurface = std::clamp(subsurface, 0.0f, 1.0f);
    anisotropy = std::clamp(anisotropy, -1.0f, 1.0f);
    
    // Clamp color values to reasonable ranges
    baseColor = glm::clamp(baseColor, glm::vec4(0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    emissive = glm::max(emissive, glm::vec3(0.0f)); // Emissive can be > 1.0 for HDR
    subsurfaceColor = glm::clamp(subsurfaceColor, glm::vec3(0.0f), glm::vec3(1.0f));
}

bool MaterialCore::loadFromJSON(const std::string& json) {
    // TODO: Implement JSON deserialization
    // This would use a JSON library like nlohmann::json or similar
    std::cerr << "[MaterialCore] JSON loading not yet implemented" << std::endl;
    return false;
}

std::string MaterialCore::saveToJSON() const {
    // TODO: Implement JSON serialization
    // This would use a JSON library like nlohmann::json or similar
    std::cerr << "[MaterialCore] JSON saving not yet implemented" << std::endl;
    return "{}";
}