#pragma once

/**
 * @file render_pass.h
 * @brief Render Graph System - Modern pass-based rendering architecture
 *
 * OVERVIEW
 * This file defines Glint3D's render graph system, a declarative framework for organizing
 * rendering work into modular, reusable passes. Unlike traditional monolithic render loops,
 * the render graph allows passes to declare their inputs/outputs and execute sequentially
 * while the GPU processes commands in parallel.
 *
 * ARCHITECTURE
 *
 * RenderGraph:
 *   - Container that owns and orchestrates render passes
 *   - Manages shared texture resources between passes
 *   - Executes passes in defined order with automatic timing
 *   - Supports enabling/disabling passes at runtime
 *
 * RenderPass (abstract base):
 *   - setup()    - Allocate GPU resources (render targets, textures, pipelines)
 *   - execute()  - Record rendering commands to GPU command queue
 *   - teardown() - Free GPU resources
 *   - getInputs()/getOutputs() - Declare data dependencies
 *
 * PassContext:
 *   - Shared state passed to all passes in a frame
 *   - Contains: scene, lights, camera matrices, viewport size, timing data
 *   - ctx.textures map - Named texture pool for inter-pass communication
 *   - ctx.renderer - Callback to RenderSystem for actual draw calls
 *
 * AVAILABLE PASSES
 * ================
 *
 * Raster Pipeline (used for interactive viewport):
 *   FrameSetupPass        → Update managers, bind UBOs, clear screen
 *   GBufferPass           → Geometry pass: write baseColor/normal/position/material
 *   DeferredLightingPass  → Lighting pass: read G-buffer, compute lighting, output litColor
 *   OverlayPass           → Render grid, axes, gizmos
 *   ResolvePass           → MSAA resolve, tonemapping (stub)
 *   PresentPass           → Swap buffers (stub)
 *   ReadbackPass          → GPU→CPU texture readback for PNG export
 *
 * Ray Pipeline (used for physically accurate rendering):
 *   FrameSetupPass        → Update managers, bind UBOs
 *   RayIntegratorPass     → CPU raytracer: cast rays, output rayTraceResult texture
 *   RayDenoisePass        → AI denoising (OIDN): rayTraceResult → denoisedResult
 *   OverlayPass           → Render overlays (usually disabled for ray)
 *   PresentPass           → Swap buffers
 *   ReadbackPass          → GPU→CPU readback for export
 *
 * DATA FLOW EXAMPLE
 * =================
 *
 * Raster Pipeline Frame:
 *
 *   GBufferPass::execute()
 *     └─> ctx.textures["gBaseColor"] = m_baseColorTex      (writes)
 *         ctx.textures["gNormal"] = m_normalTex            (writes)
 *         ctx.textures["gPosition"] = m_positionTex        (writes)
 *         ctx.textures["gMaterial"] = m_materialTex        (writes)
 *
 *   DeferredLightingPass::execute()
 *     └─> auto gBaseColor = ctx.textures.find("gBaseColor") (reads)
 *         auto gNormal = ctx.textures.find("gNormal")       (reads)
 *         // ... compute lighting ...
 *         ctx.textures["litColor"] = m_outputTex             (writes)
 *
 *   ReadbackPass::execute()
 *     └─> auto litColor = ctx.textures.find("litColor")     (reads)
 *         // ... readback to CPU for PNG export ...
 *
 * USAGE
 * =====
 *
 * Creating a render graph (see RenderSystem::initRenderGraphs):
 *
 *   m_rasterGraph = std::make_unique<RenderGraph>(m_rhi.get());
 *   m_rasterGraph->addPass(std::make_unique<FrameSetupPass>());
 *   m_rasterGraph->addPass(std::make_unique<GBufferPass>());
 *   m_rasterGraph->addPass(std::make_unique<DeferredLightingPass>());
 *
 * Executing a frame (see RenderSystem::renderUnified):
 *
 *   PassContext ctx;
 *   ctx.rhi = m_rhi.get();
 *   ctx.scene = &scene;
 *   ctx.lights = &lights;
 *   ctx.renderer = this;
 *   ctx.viewMatrix = viewMatrix;
 *   ctx.projMatrix = projMatrix;
 *   ctx.viewportWidth = width;
 *   ctx.viewportHeight = height;
 *
 *   m_rhi->beginFrame();
 *   m_rasterGraph->setup(ctx);    // Allocate resources once
 *   m_rasterGraph->execute(ctx);  // Execute all passes
 *   m_rhi->endFrame();
 *
 * KEY INSIGHTS
 * ============
 *
 * - Passes execute SEQUENTIALLY on CPU (one after another in order)
 * - GPU executes commands ASYNCHRONOUSLY (CPU queues work, GPU processes in parallel)
 * - Passes communicate via named textures in ctx.textures map
 * - Passes delegate actual rendering to RenderSystem via ctx.renderer->passXXX()
 * - Each pass is timed automatically for performance profiling
 * - Manager layer (TransformManager, LightingManager, etc.) handles UBO state
 *
 * RELATION TO LEGACY CODE
 * =======================
 *
 * This render graph system is used ONLY for interactive rendering (desktop/web viewport).
 * Offscreen rendering (renderToPNG, renderToTextureRHI) currently bypasses the graph
 * and uses legacy direct rendering paths (renderRasterized/renderRaytraced).
 *
 * See CLAUDE.md Phase B for migration status.
 *
 * @see RenderSystem::renderUnified() - Main entry point for graph-based rendering
 * @see RenderSystem::initRenderGraphs() - Graph construction and pass registration
 * @see render_mode_selector.h - Automatic pipeline selection (raster vs ray)
 */

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glint3d/rhi.h>

using namespace glint3d;

struct PassTiming {
    std::string passName;
    float timeMs = 0.0f;
    bool enabled = false;
};

class SceneManager;
class Light;
class RenderGraph;
class RenderSystem;

struct ReadbackRequest {
    void* destination = nullptr;
    size_t size = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    TextureFormat format = TextureFormat::RGBA8;
};

// Pass execution context containing all resources and state
struct PassContext {
    RHI* rhi = nullptr;
    const SceneManager* scene = nullptr;
    const Light* lights = nullptr;
    RenderSystem* renderer = nullptr;
    bool interactive = false;
    bool enableRaster = true;
    bool enableRay = false;
    bool enableOverlays = true;
    bool resolveMsaa = true;
    bool finalizeFrame = true;
    const ReadbackRequest* readback = nullptr;
    RenderTargetHandle renderTarget = INVALID_HANDLE;
    TextureHandle outputTexture = INVALID_HANDLE;

    // Render targets and textures
    std::unordered_map<std::string, TextureHandle> textures;

    // Camera and viewport
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};
    int viewportWidth = 0;
    int viewportHeight = 0;

    // Frame state
    uint32_t frameIndex = 0;
    float deltaTime = 0.0f;

    // Timing support
    bool enableTiming = true;
    std::vector<PassTiming>* passTimings = nullptr; // Pointer to stats collection

    // Pass-specific data (can be extended by derived passes)
    std::unordered_map<std::string, void*> customData;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual bool setup(const PassContext& ctx) = 0;
    virtual void execute(const PassContext& ctx) = 0;
    virtual void teardown(const PassContext& ctx) = 0;

    virtual const char* getName() const = 0;
    virtual bool isEnabled() const { return m_enabled; }
    virtual void setEnabled(bool enabled) { m_enabled = enabled; }

    virtual std::vector<std::string> getInputs() const { return {}; }
    virtual std::vector<std::string> getOutputs() const { return {}; }

    // Execute with timing support
    void executeWithTiming(const PassContext& ctx);

protected:
    bool m_enabled = true;
};

class RenderGraph {
public:
    explicit RenderGraph(RHI* rhi);
    ~RenderGraph();

    void addPass(std::unique_ptr<RenderPass> pass);
    void removePass(const std::string& name);
    RenderPass* getPass(const std::string& name);

    bool setup(const PassContext& baseContext);
    void execute(const PassContext& baseContext);
    void teardown();

    TextureHandle createTexture(const std::string& name, const TextureDesc& desc);
    void destroyTexture(const std::string& name);
    TextureHandle getTexture(const std::string& name) const;

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

private:
    RHI* m_rhi;
    std::vector<std::unique_ptr<RenderPass>> m_passes;
    std::unordered_map<std::string, TextureHandle> m_textures;
    bool m_enabled = true;
    bool m_isSetup = false;

    void sortPasses();
};

class FrameSetupPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "FrameSetupPass"; }
};

class RasterPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "RasterPass"; }
};

class RaytracePass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "RaytracePass"; }

    void setSampleCount(int samples);
    void setMaxDepth(int depth);

private:
    int m_sampleCount = 64;
    int m_maxDepth = 8;
};

class RayDenoisePass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "RayDenoisePass"; }

    std::vector<std::string> getInputs() const override {
        return {"rayTraceResult"};
    }

    std::vector<std::string> getOutputs() const override {
        return {"denoisedResult"};
    }

private:
    TextureHandle m_outputTex = INVALID_HANDLE;
};

class OverlayPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "OverlayPass"; }
};

class ResolvePass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "ResolvePass"; }
};

class PresentPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "PresentPass"; }
};

class ReadbackPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "ReadbackPass"; }

    // Allow specifying which texture to read back
    void setSourceTexture(const std::string& textureName) { m_sourceTexture = textureName; }

    std::vector<std::string> getInputs() const override {
        if (!m_sourceTexture.empty()) {
            return {m_sourceTexture};
        }
        return {"litColor", "rayTraceResult", "denoisedResult"}; // Accept any of these
    }

private:
    std::string m_sourceTexture; // Optional specific source texture
};

class GBufferPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "GBufferPass"; }

    std::vector<std::string> getOutputs() const override {
        return {"gBaseColor", "gNormal", "gPosition", "gMaterial", "gDepth"};
    }

private:
    RenderTargetHandle m_gBufferRT = INVALID_HANDLE;
    TextureHandle m_baseColorTex = INVALID_HANDLE;
    TextureHandle m_normalTex = INVALID_HANDLE;
    TextureHandle m_positionTex = INVALID_HANDLE;
    TextureHandle m_materialTex = INVALID_HANDLE;
    TextureHandle m_depthTex = INVALID_HANDLE;
};

class DeferredLightingPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "DeferredLightingPass"; }

    std::vector<std::string> getInputs() const override {
        return {"gBaseColor", "gNormal", "gPosition", "gMaterial"};
    }

    std::vector<std::string> getOutputs() const override {
        return {"litColor"};
    }

private:
    TextureHandle m_outputTex = INVALID_HANDLE;
    RenderTargetHandle m_outputRT = INVALID_HANDLE;
};

class RayIntegratorPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    const char* getName() const override { return "RayIntegratorPass"; }

    std::vector<std::string> getOutputs() const override {
        return {"rayTraceResult"};
    }

    void setSampleCount(int samples) { m_sampleCount = samples; }
    void setMaxDepth(int depth) { m_maxDepth = depth; }

private:
    TextureHandle m_outputTex = INVALID_HANDLE;
    int m_sampleCount = 64;
    int m_maxDepth = 8;
};



