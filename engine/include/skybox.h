#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <glint3d/rhi_types.h>

namespace glint3d { class RHI; }

class Skybox {
public:
    Skybox();
    ~Skybox();

    // Initialize skybox with RHI instance
    bool init(glint3d::RHI* rhi);

    // Load skybox from cubemap textures (6 faces)
    bool loadCubemap(const std::vector<std::string>& faces);

    // Set procedural gradient colors
    void setGradient(const glm::vec3& topColor, const glm::vec3& bottomColor, const glm::vec3& horizonColor = glm::vec3(0.0f));

    // Render the skybox
    void render(const glm::mat4& view, const glm::mat4& projection);

    // Enable/disable skybox rendering
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // Set skybox intensity/brightness
    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }

    // Use external environment map (e.g., from IBL system)
    void setEnvironmentMap(glint3d::TextureHandle envMap);

    // Cleanup
    void cleanup();

private:
    glint3d::RHI* m_rhi = nullptr;
    glint3d::BufferHandle m_vertexBuffer;
    glint3d::ShaderHandle m_shader;
    glint3d::TextureHandle m_cubemapTexture;
    glint3d::PipelineHandle m_pipeline;

    bool m_enabled;
    bool m_initialized;
    bool m_useGradient;
    float m_intensity;

    // Gradient colors
    glm::vec3 m_topColor;
    glm::vec3 m_bottomColor;
    glm::vec3 m_horizonColor;

    void setupCube();
    void createProceduralSkybox();
};