#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "gl_platform.h"

// Forward declarations
class SceneManager;
class Light; 
class Raytracer;
class AxisRenderer;
class Grid;
class Gizmo;
struct SceneObject;

class Shader;

enum class RenderMode {
    Points = 0,
    Wireframe = 1, 
    Solid = 2,
    Raytrace = 3
};

enum class ShadingMode {
    Flat = 0,
    Gouraud = 1
};

struct CameraState {
    glm::vec3 position{0.0f, 0.0f, 10.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fov = 45.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
};

struct RenderStats {
    int drawCalls = 0;
    size_t totalTriangles = 0;
    int uniqueMaterialKeys = 0;
    size_t uniqueTextures = 0;
    float texturesMB = 0.0f;
    float geometryMB = 0.0f;
    float vramMB = 0.0f;
    int topSharedCount = 0;
    std::string topSharedKey;
};

class RenderSystem 
{
public:
    RenderSystem();
    ~RenderSystem();

    bool init(int windowWidth, int windowHeight);
    void shutdown();

    // Main render call
    void render(const SceneManager& scene, const Light& lights);
    
    // Offscreen rendering
    bool renderToTexture(const SceneManager& scene, const Light& lights, 
                        GLuint textureId, int width, int height);
    bool renderToPNG(const SceneManager& scene, const Light& lights,
                    const std::string& path, int width, int height);

    // Camera management
    void setCamera(const CameraState& camera) { m_camera = camera; }
    const CameraState& getCamera() const { return m_camera; }
    CameraState& getCamera() { return m_camera; }
    
    void updateViewMatrix();
    void updateProjectionMatrix(int windowWidth, int windowHeight);
    
    glm::mat4 getViewMatrix() const { return m_viewMatrix; }
    glm::mat4 getProjectionMatrix() const { return m_projectionMatrix; }

    // Render modes
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }
    
    void setShadingMode(ShadingMode mode) { m_shadingMode = mode; }
    ShadingMode getShadingMode() const { return m_shadingMode; }

    // Settings
    void setFramebufferSRGBEnabled(bool enabled) { m_framebufferSRGBEnabled = enabled; }
    bool isFramebufferSRGBEnabled() const { return m_framebufferSRGBEnabled; }

    // Debug/utility rendering
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowAxes(bool show) { m_showAxes = show; }
    bool isShowGrid() const { return m_showGrid; }
    bool isShowAxes() const { return m_showAxes; }

    // Statistics
    const RenderStats& getLastFrameStats() const { return m_stats; }

    // Raytracing
    void setDenoiseEnabled(bool enabled) { m_denoiseEnabled = enabled; }
    bool isDenoiseEnabled() const { return m_denoiseEnabled; }
    bool denoise(std::vector<glm::vec3>& color,
                const std::vector<glm::vec3>* normal = nullptr,
                const std::vector<glm::vec3>* albedo = nullptr);

    // Gizmo support - forward declared, implemented in cpp
    class Gizmo* getGizmo() { return m_gizmo.get(); }
    const class Gizmo* getGizmo() const { return m_gizmo.get(); }

private:
    // Core rendering state
    CameraState m_camera;
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_projectionMatrix{1.0f};
    
    RenderMode m_renderMode = RenderMode::Solid;
    ShadingMode m_shadingMode = ShadingMode::Gouraud;
    bool m_framebufferSRGBEnabled = true;
    
    // Debug rendering
    bool m_showGrid = true;
    bool m_showAxes = true;
    
    // Utility renderers
    std::unique_ptr<AxisRenderer> m_axisRenderer;
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<Gizmo> m_gizmo;
    
    // Raytracer
    std::unique_ptr<Raytracer> m_raytracer;
    bool m_denoiseEnabled = false;
    
    // Shaders
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_pbrShader;
    
    // Statistics
    RenderStats m_stats;
    
    // Private methods
    void renderRasterized(const SceneManager& scene, const Light& lights);
    void renderRaytraced(const SceneManager& scene, const Light& lights);
    void renderObject(const SceneObject& obj, const Light& lights);
    void updateRenderStats(const SceneManager& scene);
};