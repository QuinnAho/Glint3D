#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glint3d/rhi_types.h>

// Forward declaration
namespace glint3d { class RHI; }

enum class LightType {
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2
};

struct LightSource {
    LightType type;
    glm::vec3 position;      // For point lights
    glm::vec3 direction;     // For directional/spot lights (normalized)
    glm::vec3 color;
    float intensity;
    bool enabled;
    // Spot light cone angles (degrees)
    float innerConeDeg = 15.0f;
    float outerConeDeg = 25.0f;
    
    LightSource() : type(LightType::POINT), position(0.0f), direction(0.0f, -1.0f, 0.0f), 
                    color(1.0f), intensity(1.0f), enabled(true) {}
};

class Light {
public:
    Light();
    ~Light();

    // Existing functions
    void addLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    void addDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);
    void addSpotLight(const glm::vec3& position, const glm::vec3& direction,
                      const glm::vec3& color, float intensity,
                      float innerConeDeg, float outerConeDeg);

    size_t getLightCount() const;

    // New functions for indicator visualization
    void initIndicator(glint3d::RHI* rhi);  // Create geometry (cube for point lights, arrow for directional)
    void renderIndicators(const glm::mat4& view, const glm::mat4& projection, int selectedIndex) const;
    void cleanup();  // Cleanup RHI resources

    glm::vec4 m_globalAmbient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    std::vector<LightSource> m_lights;

    const LightSource* getFirstLight() const;
    bool removeLightAt(size_t index);

private:
    glint3d::RHI* m_rhi = nullptr;

    // Indicator geometry for point lights (cube)
    glint3d::BufferHandle m_indicatorVBO;

    // Indicator geometry for directional lights (arrow)
    glint3d::BufferHandle m_arrowVBO;

    // Indicator geometry for spot lights (cone outline)
    glint3d::BufferHandle m_spotVBO;

    // Indicator shader and pipelines
    glint3d::ShaderHandle m_indicatorShader;
    glint3d::PipelineHandle m_cubePipeline;
    glint3d::PipelineHandle m_linePipeline;
};
