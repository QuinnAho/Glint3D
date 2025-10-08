#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <cstdint>
#include "camera_state.h"
#include "managers/camera_manager.h"
#include "managers/lighting_manager.h"
#include "managers/material_manager.h"
#include "managers/pipeline_manager.h"
#include "managers/transform_manager.h"
#include "managers/rendering_manager.h"
#include "gl_platform.h"
#include "gizmo.h"
// RHI types for pipeline handles
#include <glint3d/rhi.h>
#include <glint3d/uniform_blocks.h>
#include "render_mode_selector.h"
#include "render_pass.h"

using namespace glint3d;

// Forward declarations
class SceneManager;
class Light;
class Raytracer;
class AxisRenderer;
class Grid;
class Gizmo;
class Skybox;
class IBLSystem;
class RenderGraph;
class RenderPipelineModeSelector;
enum class RenderPipelineMode;
struct PassContext;
struct SceneObject;

class Shader;

enum class RenderToneMapMode {
    Linear = 0,
    Reinhard,
    Filmic,
    ACES
};

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

// PassTiming is defined in render_pass.h

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
    std::vector<PassTiming> passTimings;
};

class RenderSystem 
{
public:
    RenderSystem();
    ~RenderSystem();

    bool init(int windowWidth, int windowHeight);
    void shutdown();

    // Main render calls
    void render(const SceneManager& scene, const Light& lights);
    void renderUnified(const SceneManager& scene, const Light& lights);
    
    // Offscreen rendering
    // renderToTexture() REMOVED (FEAT-0253) - use renderToTextureRHI() instead
    // RHI path: renders into an RHI TextureHandle using RHI render targets
    bool renderToTextureRHI(const SceneManager& scene, const Light& lights,
                           TextureHandle textureHandle, int width, int height);
    bool renderToPNG(const SceneManager& scene, const Light& lights,
                    const std::string& path, int width, int height);

    // Camera management
    void setCamera(const CameraState& camera) { m_cameraManager.setCamera(camera); }
    const CameraState& getCamera() const { return m_cameraManager.camera(); }
    CameraState& getCamera() { return m_cameraManager.camera(); }
    
    void updateViewMatrix();
    void updateProjectionMatrix(int windowWidth, int windowHeight);
    
    glm::mat4 getViewMatrix() const { return m_cameraManager.viewMatrix(); }
    glm::mat4 getProjectionMatrix() const { return m_cameraManager.projectionMatrix(); }


    // Render modes
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }
    
    void setShadingMode(ShadingMode mode) { m_shadingMode = mode; }
    ShadingMode getShadingMode() const { return m_shadingMode; }

    // Settings
    void setFramebufferSRGBEnabled(bool enabled) { m_framebufferSRGBEnabled = enabled; }
    bool isFramebufferSRGBEnabled() const { return m_framebufferSRGBEnabled; }

    // Background / Presentation
    enum class BackgroundMode { Solid = 0, Gradient = 1, HDR = 2 };
    void setBackgroundColor(const glm::vec3& c) { setBackgroundSolid(c); }
    const glm::vec3& getBackgroundColor() const { return m_backgroundColor; }
    void setBackgroundSolid(const glm::vec3& c) { m_backgroundColor = c; m_bgMode = BackgroundMode::Solid; }
    void setBackgroundGradient(const glm::vec3& top, const glm::vec3& bottom) { m_bgTop = top; m_bgBottom = bottom; m_bgMode = BackgroundMode::Gradient; }
    void setBackgroundHDR(const std::string& hdrPath);
    BackgroundMode getBackgroundMode() const { return m_bgMode; }
    glm::vec3 getBackgroundTopColor() const { return m_bgTop; }
    glm::vec3 getBackgroundBottomColor() const { return m_bgBottom; }
    const std::string& getBackgroundHDRPath() const { return m_bgHDRPath; }
    bool loadSkybox(const std::string& path); // simple loader/enable toggle
    void setExposure(float v) { m_exposure = v; }
    float getExposure() const { return m_exposure; }
    void setToneMapping(RenderToneMapMode m) { m_tonemap = m; }
    RenderToneMapMode getToneMapping() const { return m_tonemap; }
    void setGamma(float g) { m_gamma = g; }
    float getGamma() const { return m_gamma; }
    
    // Random seed for deterministic rendering
    void setSeed(uint32_t seed) { m_seed = seed; }
    uint32_t getSeed() const { return m_seed; }

    // Debug/utility rendering
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowAxes(bool show) { m_showAxes = show; }
    void setShowSkybox(bool show) { m_showSkybox = show; }
    bool isShowGrid() const { return m_showGrid; }
    bool isShowAxes() const { return m_showAxes; }
    bool isShowSkybox() const { return m_showSkybox; }

    // Statistics
    const RenderStats& getLastFrameStats() const { return m_stats; }

    // MSAA sample control
    void setSampleCount(int samples) { m_samples = samples < 1 ? 1 : samples; m_recreateTargets = true; }
    int  getSampleCount() const { return m_samples; }

    // Raytracing
    void setDenoiseEnabled(bool enabled) { m_denoiseEnabled = enabled; }
    bool isDenoiseEnabled() const { return m_denoiseEnabled; }
    
    // Reflection samples per pixel for glossy reflections
    void setReflectionSpp(int spp);
    int getReflectionSpp() const;
    bool denoise(std::vector<glm::vec3>& color,
                const std::vector<glm::vec3>* normal = nullptr,
                const std::vector<glm::vec3>* albedo = nullptr);
    bool denoise(std::vector<glm::vec3>& color, int width, int height,
                const std::vector<glm::vec3>* normal = nullptr,
                const std::vector<glm::vec3>* albedo = nullptr);

    // Gizmo support - forward declared, implemented in cpp
    class Gizmo* getGizmo() { return m_gizmo.get(); }
    const class Gizmo* getGizmo() const { return m_gizmo.get(); }
    
    // Skybox access
    class Skybox* getSkybox() { return m_skybox.get(); }
    const class Skybox* getSkybox() const { return m_skybox.get(); }
    
    // IBL system access
    class IBLSystem* getIBLSystem() { return m_iblSystem.get(); }
    const class IBLSystem* getIBLSystem() const { return m_iblSystem.get(); }
    bool loadHDREnvironment(const std::string& hdrPath);
    void setIBLIntensity(float intensity);
    
    // RHI access (for SceneManager to create buffers)
    RHI* getRHI() const { return m_rhi.get(); }

    // Manager access
    PipelineManager& getPipelineManager() { return m_pipelineManager; }
    const PipelineManager& getPipelineManager() const { return m_pipelineManager; }
    
    // Gizmo/selection configuration
    void setGizmoMode(GizmoMode mode) { m_gizmoMode = mode; }
    void setGizmoAxis(GizmoAxis axis) { m_gizmoAxis = axis; }
    void setGizmoLocalSpace(bool local) { m_gizmoLocal = local; }
    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    void setSelectedLightIndex(int idx) { m_selectedLightIndex = idx; }
    GizmoMode gizmoMode() const { return m_gizmoMode; }
    GizmoAxis gizmoAxis() const { return m_gizmoAxis; }
    bool gizmoLocalSpace() const { return m_gizmoLocal; }
    bool snapEnabled() const { return m_snapEnabled; }
    int selectedLightIndex() const { return m_selectedLightIndex; }

private:
    // Core rendering state
    CameraManager m_cameraManager;
    LightingManager m_lightingManager;
    MaterialManager m_materialManager;
    PipelineManager m_pipelineManager;
    TransformManager m_transformManager;
    RenderingManager m_renderingManager;

    std::unique_ptr<RenderGraph> m_rasterGraph;
    std::unique_ptr<RenderGraph> m_rayGraph;
    std::unique_ptr<RenderPipelineModeSelector> m_pipelineSelector;
    RenderPipelineMode m_activePipelineMode = RenderPipelineMode::Raster;
    RenderPipelineMode m_pipelineOverride = RenderPipelineMode::Auto;

    RenderTargetHandle m_activeRenderTarget = INVALID_HANDLE;
    TextureHandle m_activeOutputTexture = INVALID_HANDLE;
    uint64_t m_frameCounter = 0;

    RenderMode m_renderMode = RenderMode::Solid;
    ShadingMode m_shadingMode = ShadingMode::Gouraud;
    bool m_framebufferSRGBEnabled = true;
    glm::vec3 m_backgroundColor{0.10f, 0.11f, 0.12f};
    BackgroundMode m_bgMode = BackgroundMode::Solid;
    glm::vec3 m_bgTop{0.10f, 0.11f, 0.12f};
    glm::vec3 m_bgBottom{0.10f, 0.11f, 0.12f};
    std::string m_bgHDRPath; // accepted via ops; hook/stub
    float m_exposure = 0.0f;
    float m_gamma = 2.2f;
    RenderToneMapMode m_tonemap = RenderToneMapMode::Linear;
    uint32_t m_seed = 0;

    // Debug rendering
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showSkybox = false;

    // RHI - MUST be declared before systems that depend on it for proper destruction order
    std::unique_ptr<RHI> m_rhi;

    // Utility renderers (depend on m_rhi)
    std::unique_ptr<AxisRenderer> m_axisRenderer;
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<Gizmo> m_gizmo;
    std::unique_ptr<Skybox> m_skybox;
    std::unique_ptr<IBLSystem> m_iblSystem;
    
    // Raytracer
    std::unique_ptr<Raytracer> m_raytracer;
    bool m_denoiseEnabled = false;
    int m_reflectionSpp = 8; // Default reflection samples per pixel
    
    // Raytracing screen quad resources
    GLuint m_screenQuadVAO = 0;
    GLuint m_screenQuadVBO = 0;
    GLuint m_raytraceTexture = 0;
    TextureHandle m_raytraceTextureRhi = INVALID_HANDLE;
    std::unique_ptr<Shader> m_screenQuadShader;
    int m_raytraceWidth = 512;
    int m_raytraceHeight = 512;

    // RHI pipelines (shader binding handled by legacy Shader for now)
    PipelineHandle m_basicPipeline = INVALID_HANDLE;
    PipelineHandle m_pbrPipeline = INVALID_HANDLE;
    ShaderHandle m_basicShaderRhi = INVALID_HANDLE;
    ShaderHandle m_pbrShaderRhi = INVALID_HANDLE;
    std::unordered_map<const SceneObject*, PipelineHandle> m_wireframePipelines; // Cache for selection wireframes

    // Helpers
    void ensureObjectPipeline(struct SceneObject& obj, bool usePbr);
    // Uniform helpers targeting current GL program
    void setUniformMat4(const char* name, const glm::mat4& m);
    void setUniformVec3(const char* name, const glm::vec3& v);
    void setUniformVec4(const char* name, const glm::vec4& v);
    void setUniformFloat(const char* name, float v);
    void setUniformInt(const char* name, int v);
    void setUniformBool(const char* name, bool v);

    // UBO binding - delegates entirely to managers
    void bindUniformBlocks(); // Delegates to all managers

    // Shaders
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_pbrShader;
    std::unique_ptr<Shader> m_gridShader;
    std::unique_ptr<Shader> m_gradientShader;
    
    // Fallback shadow map to satisfy shaders that sample shadowMap
    GLuint m_dummyShadowTex = 0;
    TextureHandle m_dummyShadowTexRhi = INVALID_HANDLE;
    
    // Statistics
    RenderStats m_stats;
    
    // Private methods
    // renderLegacy() removed - now using renderUnified() with RenderGraph exclusively
    void renderRasterized(const SceneManager& scene, const Light& lights);
    void renderRaytraced(const SceneManager& scene, const Light& lights);
    void renderObject(const SceneObject& obj, const Light& lights);
    void updateRenderStats(const SceneManager& scene);
    
    // Optimized rendering methods
    void renderDebugElements(const SceneManager& scene, const Light& lights);
    void renderSelectionOutline(const SceneManager& scene);
    void renderGizmo(const SceneManager& scene, const Light& lights);
    void renderObjectsBatched(const SceneManager& scene, const Light& lights);
    void renderObjectsBatchedWithManagers(const SceneManager& scene, const Light& lights);  // New manager-based method
    void setupCommonUniforms(Shader* shader);
    void renderObjectFast(const SceneObject& obj, const Light& lights, Shader* shader);
    
    // Raytracing support methods
    void initScreenQuad();
    void initRaytraceTexture();
    void cleanupRaytracing();

    // Gizmo state
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    GizmoAxis m_gizmoAxis = GizmoAxis::None;
    bool m_gizmoLocal = true;
    bool m_snapEnabled = false;
    int  m_selectedLightIndex = -1;

    // MSAA offscreen pipeline for onscreen rendering
    int   m_samples = 1;
    bool  m_recreateTargets = false;
    int   m_fbWidth = 0;
    int   m_fbHeight = 0;

    // RHI-based MSAA resources (replacing GL FBO/RBO objects)
    RenderTargetHandle m_msaaRenderTarget = INVALID_HANDLE;

    // Legacy GL objects (deprecated, use RHI render targets instead)
    GLuint m_msaaFBO = 0;  // TODO: Remove after full migration
    GLuint m_msaaColorRBO = 0;  // TODO: Remove after full migration
    GLuint m_msaaDepthRBO = 0;  // TODO: Remove after full migration

    // Internal helpers
    void createOrResizeTargets(int width, int height);
    void destroyTargets();

    // Render Graph System
    void initializeRenderGraphs();
    void destroyRenderGraphs();

    // Render graph management
    void initRenderGraphs();
    RenderGraph* getActiveGraph(RenderPipelineMode mode);

public:
    // Render Pass Methods (called by render graph passes)
    void passFrameSetup(const PassContext& ctx);
    void passRaster(const PassContext& ctx);
    void passRaytrace(const PassContext& ctx, int sampleCount = 1, int maxDepth = 8);
    void passGBuffer(const PassContext& ctx, RenderTargetHandle gBufferRT);
    void passDeferredLighting(const PassContext& ctx, RenderTargetHandle outputRT,
                             TextureHandle gBaseColor, TextureHandle gNormal,
                             TextureHandle gPosition, TextureHandle gMaterial);
    void passRayIntegrator(const PassContext& ctx, TextureHandle outputTex, int sampleCount, int maxDepth);
    void passRayDenoise(const PassContext& ctx, TextureHandle inputTex, TextureHandle outputTex);
    void passOverlays(const PassContext& ctx);
    void passResolve(const PassContext& ctx);
    void passPresent(const PassContext& ctx);
    void passReadback(const PassContext& ctx);

private:

    // Note: m_rhi moved earlier in declaration order for proper cleanup sequencing

    // Render pass pipeline handles
    PipelineHandle m_gBufferPipeline = INVALID_HANDLE;
    PipelineHandle m_deferredLightingPipeline = INVALID_HANDLE;

    // Screen quad for full-screen passes
    BufferHandle m_screenQuadVBORhi = INVALID_HANDLE;

    // Helper methods for pass implementations
    PipelineHandle getOrCreateGBufferPipeline();
    PipelineHandle getOrCreateDeferredLightingPipeline();
    void createScreenQuad();
    std::string loadTextFileRhi(const std::string& path);
};











