#include "render_system.h"
#include <glint3d/uniform_blocks.h>
#include "scene_manager.h"
#include "light.h"
#include "axisrenderer.h"
#include "grid.h"
#include "gizmo.h"
#include "skybox.h"
#include "ibl_system.h"
#include "raytracer.h"
#include "texture.h"
#include "material_core.h"
#include "render_mode_selector.h"
#include "render_pass.h"
#include <glint3d/rhi.h>
#include <glint3d/rhi_types.h>
#include "rhi/rhi_gl.h"
#include "gl_platform.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace glint3d;

// FEAT-0249: Legacy uniform helpers REMOVED
// All transform, lighting, and material data now uses UBO system
// Only non-UBO uniforms (texture units, toggles) should use direct RHI calls

// UBO Helper Methods (FEAT-0249)
// ===============================

// Legacy UBO helper methods removed - functionality moved to managers

void RenderSystem::bindUniformBlocks()
{
    if (!m_rhi) return;

    // Delegate all UBO binding to managers
    m_transformManager.bindTransformUniforms();      // Binding point 0
    m_lightingManager.bindLightingUniforms();        // Binding point 1
    m_materialManager.bindMaterialUniforms();        // Binding point 2
    m_renderingManager.bindRenderingUniforms();      // Binding point 3
}

void RenderSystem::ensureObjectPipeline(SceneObject& obj, bool usePbr)
{
    if (!m_rhi) return;
    bool hasNormals = (obj.rhiVboNormals != INVALID_HANDLE);
    bool hasUVs = (obj.rhiVboTexCoords != INVALID_HANDLE);

    // Always use PBR pipeline (standard shader eliminated)
    PipelineHandle& target = obj.rhiPipelinePbr;
    if (target != INVALID_HANDLE) {
        m_rhi->bindPipeline(target);
        bindUniformBlocks(); // Bind UBOs when using existing pipeline
        return;
    }

    PipelineDesc pd{};
    pd.topology = PrimitiveTopology::Triangles;
    pd.shader = m_pbrShaderRhi; // Always use PBR shader
    pd.debugName = obj.name + ":pipeline_pbr";

    // Configure blending for transparent/transmissive materials
    float transmission = obj.materialCore.transmission;
    bool needsBlending = (transmission > 0.01f || obj.materialCore.baseColor.a < 0.999f);
    if (needsBlending) {
        pd.blendEnable = true;
        pd.srcColorBlendFactor = BlendFactor::SrcAlpha;
        pd.dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
        pd.srcAlphaBlendFactor = BlendFactor::One;
        pd.dstAlphaBlendFactor = BlendFactor::OneMinusSrcAlpha;
        pd.depthWriteEnable = false; // Disable depth writes for transparency
    }

    VertexBinding bPos{}; bPos.binding = 0; bPos.stride = 3 * sizeof(float); bPos.buffer = obj.rhiVboPositions; pd.vertexBindings.push_back(bPos);
    if (hasNormals) { VertexBinding bN{}; bN.binding = 1; bN.stride = 3 * sizeof(float); bN.buffer = obj.rhiVboNormals; pd.vertexBindings.push_back(bN); }
    if (hasUVs) { VertexBinding bUV{}; bUV.binding = 2; bUV.stride = 2 * sizeof(float); bUV.buffer = obj.rhiVboTexCoords; pd.vertexBindings.push_back(bUV); }

    VertexAttribute aPos{}; aPos.location = 0; aPos.binding = 0; aPos.format = TextureFormat::RGB32F; aPos.offset = 0; pd.vertexAttributes.push_back(aPos);
    if (hasNormals) { VertexAttribute aN{}; aN.location = 1; aN.binding = 1; aN.format = TextureFormat::RGB32F; aN.offset = 0; pd.vertexAttributes.push_back(aN); }
    if (hasUVs) { VertexAttribute aUV{}; aUV.location = 2; aUV.binding = 2; aUV.format = TextureFormat::RG32F; aUV.offset = 0; pd.vertexAttributes.push_back(aUV); }

    pd.indexBuffer = obj.rhiEbo;
    target = m_rhi->createPipeline(pd);
    // Bind immediately so subsequent uniform calls apply to correct program
    m_rhi->bindPipeline(target);
    // Bind uniform blocks to their binding points
    bindUniformBlocks();
}

// PNG writer
#ifndef __EMSCRIPTEN__
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "stb_image_write.h"
#endif

#ifdef OIDN_ENABLED
#include <OpenImageDenoise/oidn.hpp>
#endif

RenderSystem::RenderSystem()
    : m_activePipelineMode(RenderPipelineMode::Raster)
{
    // Initialize renderer components (GL resources are created in init)
    m_axisRenderer = std::make_unique<AxisRenderer>();
    m_grid = std::make_unique<Grid>();
    m_raytracer = std::make_unique<Raytracer>();
    m_gizmo = std::make_unique<Gizmo>();
    m_skybox = std::make_unique<Skybox>();
    m_iblSystem = std::make_unique<IBLSystem>();
}

RenderSystem::~RenderSystem()
{
    shutdown();
}

// File-scope RHI resources created alongside legacy GL placeholders
static TextureHandle s_dummyShadowTexRhi = 0;

bool RenderSystem::init(int windowWidth, int windowHeight)
{
    // Minimal RHI init (OpenGL backend) - handles depth test, MSAA, sRGB setup
    if (!m_rhi) {
        m_rhi = createRHI(RHI::Backend::OpenGL);
        if (m_rhi) {
            RhiInit init{}; init.windowWidth = windowWidth; init.windowHeight = windowHeight; init.enableSRGB = m_framebufferSRGBEnabled;
            m_rhi->init(init);

            // Initialize managers after RHI is ready
            if (!m_lightingManager.init(m_rhi.get())) {
                std::cerr << "Failed to initialize LightingManager" << std::endl;
                return false;
            }

            if (!m_materialManager.init(m_rhi.get())) {
                std::cerr << "Failed to initialize MaterialManager" << std::endl;
                return false;
            }

            if (!m_pipelineManager.init(m_rhi.get())) {
                std::cerr << "Failed to initialize PipelineManager" << std::endl;
                return false;
            }

            if (!m_transformManager.init(m_rhi.get())) {
                std::cerr << "Failed to initialize TransformManager" << std::endl;
                return false;
            }

            if (!m_renderingManager.init(m_rhi.get())) {
                std::cerr << "Failed to initialize RenderingManager" << std::endl;
                return false;
            }
        }
    }
    
    // Use RHI for viewport and clear color instead of direct GL
    if (m_rhi) {
        m_rhi->setViewport(0, 0, windowWidth, windowHeight);
        // Note: glClearColor is now handled in render loop via m_rhi->clear()
    }

    // Legacy Shader wrappers removed - using RHI shaders and pipelines exclusively
    // m_pbrShader removed - now using m_pbrShaderRhi with m_pbrPipeline
    // m_basicShader removed - legacy comparison logic eliminated
    // m_gridShader removed - Grid class now self-contained with RHI shader
    // m_gradientShader removed - never used, gradient background should use RHI directly
    // m_screenQuadShader removed - now using m_screenQuadShaderRhi with m_screenQuadPipeline

    // Register RHI with Texture so cache can create matching RHI textures
    if (m_rhi) {
        Texture::setRHI(m_rhi.get());
        Shader::setRHI(m_rhi.get()); // Route shader uniforms through RHI
    }

    // Init helpers
    if (m_grid) m_grid->init(m_rhi.get(), 200, 1.0f);
    if (m_axisRenderer) m_axisRenderer->init(m_rhi.get());
    if (m_skybox) m_skybox->init(m_rhi.get());
    if (m_iblSystem) m_iblSystem->init(m_rhi.get());
    
    // Create shaders via RHI and minimal pipelines for fallback
    if (m_rhi) {
        // Create RHI shaders - unified PBR pipeline only
        ShaderDesc sdPbr{};
        sdPbr.vertexSource = loadTextFileRhi("engine/shaders/pbr.vert");
        sdPbr.fragmentSource = loadTextFileRhi("engine/shaders/pbr.frag");
        sdPbr.debugName = "pbr";
        m_pbrShaderRhi = m_rhi->createShader(sdPbr);

        // For backward compatibility, point basicShaderRhi to PBR shader
        m_basicShaderRhi = m_pbrShaderRhi;

        PipelineDesc pd{};
        pd.topology = PrimitiveTopology::Triangles;
        pd.debugName = "pbr_pipeline";
        pd.shader = m_pbrShaderRhi;
        m_pbrPipeline = m_rhi->createPipeline(pd);

        // For backward compatibility, point basicPipeline to PBR pipeline
        m_basicPipeline = m_pbrPipeline;

        // Create rayscreen shader and pipeline for screen quad display
        ShaderDesc sdRayscreen{};
        sdRayscreen.vertexSource = loadTextFileRhi("engine/shaders/rayscreen.vert");
        sdRayscreen.fragmentSource = loadTextFileRhi("engine/shaders/rayscreen.frag");
        sdRayscreen.debugName = "rayscreen";
        m_screenQuadShaderRhi = m_rhi->createShader(sdRayscreen);

        PipelineDesc pdRayscreen{};
        pdRayscreen.shader = m_screenQuadShaderRhi;
        pdRayscreen.topology = PrimitiveTopology::Triangles;
        pdRayscreen.debugName = "rayscreen_pipeline";

        // Vertex attributes matching screen quad buffer format (vec2 pos + vec2 uv)
        VertexAttribute posAttr{};
        posAttr.location = 0;
        posAttr.binding = 0;
        posAttr.format = TextureFormat::RG32F;  // vec2 position
        posAttr.offset = 0;
        pdRayscreen.vertexAttributes.push_back(posAttr);

        VertexAttribute uvAttr{};
        uvAttr.location = 1;
        uvAttr.binding = 0;
        uvAttr.format = TextureFormat::RG32F;  // vec2 UV
        uvAttr.offset = sizeof(float) * 2;
        pdRayscreen.vertexAttributes.push_back(uvAttr);

        VertexBinding binding{};
        binding.binding = 0;
        binding.stride = sizeof(float) * 4;  // 2 floats pos + 2 floats UV
        binding.perInstance = false;
        binding.buffer = m_rhi->getScreenQuadBuffer();
        pdRayscreen.vertexBindings.push_back(binding);

        pdRayscreen.depthTestEnable = false;  // Screen quad doesn't need depth test
        pdRayscreen.depthWriteEnable = false;

        m_screenQuadPipeline = m_rhi->createPipeline(pdRayscreen);
    }

    // Initialize raytracing resources only when needed
    if (m_renderMode == RenderMode::Raytrace) {
        initScreenQuad();
        initRaytraceTexture();
    }
    if (m_gizmo) m_gizmo->init(m_rhi.get());

    // Create a 1x1 depth texture as a dummy shadow map to satisfy shaders
    float depthOne = 1.0f;
    
    if (m_rhi) {
        // Use RHI path
        TextureDesc desc{};
        desc.type = TextureType::Texture2D;
        desc.format = TextureFormat::Depth32F;
        desc.width = 1;
        desc.height = 1;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.initialData = &depthOne;
        desc.initialDataSize = sizeof(float);
        desc.debugName = "DummyShadowTexture";
        
        m_dummyShadowTexRhi = m_rhi->createTexture(desc);
        if (m_dummyShadowTexRhi != INVALID_HANDLE) {
            std::cerr << "[RenderSystem] Created dummy shadow texture via RHI: " << m_dummyShadowTexRhi << std::endl;
        } else {
            std::cerr << "[RenderSystem] Failed to create dummy shadow texture via RHI, falling back to GL" << std::endl;
        }
    }
    
    // Note: Dummy shadow texture now created exclusively via RHI (lines 236-251)

    // Update global RHI dummy shadow texture
    if (m_rhi && m_dummyShadowTexRhi != INVALID_HANDLE) {
        s_dummyShadowTexRhi = m_dummyShadowTexRhi;
    } else if (m_rhi && s_dummyShadowTexRhi == 0) {
        // Fallback creation if member creation failed
        TextureDesc td{};
        td.type = TextureType::Texture2D;
        td.format = TextureFormat::Depth24Stencil8;
        td.width = 1; td.height = 1; td.mipLevels = 1;
        td.debugName = "dummyShadowTexRhi";
        s_dummyShadowTexRhi = m_rhi->createTexture(td);
    }

    // Initialize sub-systems' matrices
    updateProjectionMatrix(windowWidth, windowHeight);
    updateViewMatrix();

    // Initialize MSAA targets for onscreen rendering if enabled
    m_fbWidth = windowWidth;
    m_fbHeight = windowHeight;
    createOrResizeTargets(windowWidth, windowHeight);

    initRenderGraphs();

    return true;
}

void RenderSystem::shutdown()
{
    // Cleanup OpenGL resources
    if (m_axisRenderer) { m_axisRenderer->cleanup(); }
    if (m_grid) { m_grid->cleanup(); }
    if (m_gizmo) { m_gizmo->cleanup(); }
    m_raytracer.reset();
    // Legacy Shader wrappers removed - RHI shaders cleaned up by RHI::shutdown()
    // m_basicShader, m_pbrShader, m_gridShader removed - using RHI shaders exclusively
    // Note: m_dummyShadowTexRhi cleanup handled by RHI shutdown

    // Shutdown managers (will handle UBO cleanup)
    m_lightingManager.shutdown();
    m_materialManager.shutdown();
    m_pipelineManager.shutdown();
    m_transformManager.shutdown();
    m_renderingManager.shutdown();

    // UBO cleanup now handled entirely by managers
    if (m_rhi && s_dummyShadowTexRhi != 0) { m_rhi->destroyTexture(s_dummyShadowTexRhi); s_dummyShadowTexRhi = 0; }
    destroyTargets();

    m_rasterGraph.reset();
    m_rayGraph.reset();
    m_pipelineSelector.reset();
}

void RenderSystem::render(const SceneManager& scene, const Light& lights)
{
    renderUnified(scene, lights);
}

void RenderSystem::renderUnified(const SceneManager& scene, const Light& lights)
{
    if (!m_rhi || !m_pipelineSelector) {
        std::cerr << "[RenderSystem] ERROR: RHI or pipeline selector not initialized. Cannot render." << std::endl;
        return;
    }

    // Reset per-frame stats counters
    m_stats = {};

    // Update uniform blocks using managers
    m_transformManager.updateTransforms(glm::mat4(1.0f), m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());
    m_lightingManager.updateLighting(lights, m_cameraManager.camera().position);
    // Material updates are per-object, handled in rendering loops
    m_renderingManager.updateRenderingState(m_exposure, m_gamma, m_tonemap, m_shadingMode, m_iblSystem.get());
    bindUniformBlocks();

    // Select appropriate pipeline mode
    std::vector<MaterialCore> materials;
    for (const auto& obj : scene.getObjects()) {
        materials.push_back(obj.materialCore);
    }
    RenderConfig config;
    config.mode = m_pipelineOverride;
    RenderPipelineMode mode = m_pipelineSelector->selectMode(materials, config);
    m_activePipelineMode = mode;

    // Get the active render graph
    RenderGraph* graph = getActiveGraph(mode);
    if (!graph) {
        std::cerr << "[RenderSystem] ERROR: No render graph available for mode " << (int)mode << std::endl;
        return;
    }

    // Setup pass context
    PassContext ctx{};
    ctx.rhi = m_rhi.get();
    ctx.scene = &scene;
    ctx.lights = &lights;
    ctx.renderer = this;
    ctx.interactive = true;
    ctx.enableRaster = (mode == RenderPipelineMode::Raster);
    ctx.enableRay = (mode == RenderPipelineMode::Ray);
    ctx.enableOverlays = m_showGrid || m_showAxes;
    ctx.resolveMsaa = (m_samples > 1);
    ctx.finalizeFrame = true;

    // Camera and viewport
    ctx.viewMatrix = m_cameraManager.viewMatrix();
    ctx.projMatrix = m_cameraManager.projectionMatrix();
    // Use tracked framebuffer dimensions (RHI is required for renderUnified)
    ctx.viewportWidth = m_fbWidth;
    ctx.viewportHeight = m_fbHeight;

    // Frame state
    ctx.frameIndex = ++m_frameCounter;
    ctx.deltaTime = 0.016f; // TODO: get real delta time

    // Timing support
    ctx.enableTiming = true;
    ctx.passTimings = &m_stats.passTimings;

    // Setup render graph if needed
    if (!graph->setup(ctx)) {
        std::cerr << "[RenderSystem] ERROR: Failed to setup render graph" << std::endl;
        return;
    }

    // Execute render graph
    m_rhi->beginFrame();
    graph->execute(ctx);
    m_rhi->endFrame();
}

// renderLegacy() removed - all rendering now uses renderUnified() with RenderGraph
// Legacy GL-based rendering path no longer supported (opengl_migration task completion)

bool RenderSystem::loadSkybox(const std::string& path)
{
    // Minimal implementation: ensure skybox is initialized and enable it.
    // If needed, future enhancement can parse `path` for cubemap faces.
    if (!m_skybox) return false;
    if (!m_skybox->init(m_rhi.get())) return false;
    setShowSkybox(true);
    return true;
}

void RenderSystem::setBackgroundHDR(const std::string& hdrPath)
{
    m_bgHDRPath = hdrPath;
    m_bgMode = BackgroundMode::HDR;
    
    // Automatically load the HDR environment into the IBL system
    if (m_iblSystem && !hdrPath.empty()) {
        loadHDREnvironment(hdrPath);
    }
}

bool RenderSystem::loadHDREnvironment(const std::string& hdrPath)
{
    if (!m_iblSystem) return false;
    
    if (m_iblSystem->loadHDREnvironment(hdrPath)) {
        // Generate IBL maps
        m_iblSystem->generateIrradianceMap();
        m_iblSystem->generatePrefilterMap();
        m_iblSystem->generateBRDFLUT();
        
        // Also use the environment map for skybox rendering if possible
        // For now, we'll keep the procedural skybox for compatibility
        return true;
    }
    return false;
}

void RenderSystem::setIBLIntensity(float intensity)
{
    if (m_iblSystem) {
        m_iblSystem->setIntensity(intensity);
    }
}

// renderToTexture() REMOVED (FEAT-0253 completion - opengl_migration task)
// Legacy GL FBO-based rendering path with 69 GL calls eliminated
// Use renderToTextureRHI() instead for all offscreen rendering

bool RenderSystem::renderToPNG(const SceneManager& scene, const Light& lights,
                               const std::string& path, int width, int height)
{
#ifdef __EMSCRIPTEN__
    (void)scene; (void)lights; (void)path; (void)width; (void)height;
    std::cerr << "renderToPNG is not supported on Web builds.\n";
    return false;
#else
    if (width <= 0 || height <= 0) return false;

    // RHI is required for offscreen rendering
    if (!m_rhi) {
        std::cerr << "[RenderSystem] renderToPNG requires RHI initialization\n";
        return false;
    }

    // RHI path: render using renderToTextureRHI, then readback
    {
        TextureHandle colorTexHandle = INVALID_HANDLE;
        TextureDesc td{};
        td.type = TextureType::Texture2D;
        td.format = TextureFormat::RGBA8;
        td.width = width; td.height = height; td.depth = 1;
        td.generateMips = false;
        td.debugName = "renderToPNG_color";
        colorTexHandle = m_rhi->createTexture(td);

        if (colorTexHandle != INVALID_HANDLE) {
            bool okRhi = renderToTextureRHI(scene, lights, colorTexHandle, width, height);
            if (okRhi) {
                const int comp = 4;
                const int rowStride = width * comp;
                std::vector<std::uint8_t> pixels(height * rowStride);
                ReadbackDesc rb{};
                rb.sourceTexture = colorTexHandle;
                rb.format = TextureFormat::RGBA8;
                rb.x = 0; rb.y = 0; rb.width = width; rb.height = height;
                rb.destination = pixels.data();
                rb.destinationSize = pixels.size();
                m_rhi->readback(rb);

                // Flip vertically for conventional image orientation
                std::vector<std::uint8_t> flipped(height * rowStride);
                for (int y = 0; y < height; ++y) {
                    std::memcpy(flipped.data() + (height - 1 - y) * rowStride,
                                pixels.data() + y * rowStride,
                                rowStride);
                }

                int writeOK = stbi_write_png(path.c_str(), width, height, comp, flipped.data(), rowStride);

                // Viewport restoration handled by RenderSystem internally
                // No framebuffer restoration needed since we used RHI render targets

                m_rhi->destroyTexture(colorTexHandle);
                return writeOK != 0;
            } else {
                // Cleanup and fall back to GL path
                m_rhi->destroyTexture(colorTexHandle);
            }
        }
        // If RHI path failed, fall through to GL fallback below
    }

    // RHI fallback path: create RHI texture, render via RHI, and read back via RHI
    TextureHandle colorTexHandle = INVALID_HANDLE;
    TextureDesc td{};
    td.type = TextureType::Texture2D;
    td.format = TextureFormat::RGBA8;
    td.width = width; td.height = height; td.depth = 1;
    td.generateMips = false;
    td.debugName = "renderToPNG_fallback_color";
    colorTexHandle = m_rhi->createTexture(td);

    if (colorTexHandle == INVALID_HANDLE) {
        std::cerr << "[RenderSystem] renderToPNG fallback: failed to create RHI texture" << std::endl;
        return false;
    }

    std::cerr << "[RenderSystem] About to render to texture (RHI fallback)" << std::endl;
    bool ok = renderToTextureRHI(scene, lights, colorTexHandle, width, height);
    std::cerr << "[RenderSystem] renderToTextureRHI returned " << (ok ? "true" : "false") << std::endl;
    if (!ok) {
        m_rhi->destroyTexture(colorTexHandle);
        return false;
    }

    const int comp = 4;
    const int rowStride = width * comp;
    std::vector<std::uint8_t> pixels(height * rowStride);
    ReadbackDesc rb{};
    rb.sourceTexture = colorTexHandle;
    rb.format = TextureFormat::RGBA8;
    rb.x = 0; rb.y = 0; rb.width = width; rb.height = height;
    rb.destination = pixels.data();
    rb.destinationSize = pixels.size();
    m_rhi->readback(rb);

    std::vector<std::uint8_t> flipped(height * rowStride);
    for (int y = 0; y < height; ++y) {
        std::memcpy(flipped.data() + (height - 1 - y) * rowStride,
                    pixels.data() + y * rowStride,
                    rowStride);
    }

    int writeOK = stbi_write_png(path.c_str(), width, height, comp, flipped.data(), rowStride);

    // Cleanup RHI texture
    m_rhi->destroyTexture(colorTexHandle);

    return writeOK != 0;
#endif
}

bool RenderSystem::renderToTextureRHI(const SceneManager& scene, const Light& lights,
                                     TextureHandle textureHandle, int width, int height)
{
    // RHI variant: renders into an existing RHI texture by binding a render target
    // - If MSAA is enabled, render to a multisampled render target and resolve into the texture
    // - Otherwise, attach the provided texture directly as the render target color attachment
    if (!m_rhi || textureHandle == INVALID_HANDLE || width <= 0 || height <= 0) {
        std::cerr << "[RenderSystem] renderToTexture(RHI): invalid params or RHI unavailable\n";
        return false;
    }

    // Preserve current projection matrix; viewport restoration not needed for offscreen RHI rendering
    glm::mat4 prevProj = m_cameraManager.projectionMatrix();
    updateProjectionMatrix(width, height);

    bool ok = true;

    if (m_samples > 1) {
        // Create multisampled offscreen RT (renderbuffer-style attachments)
        RenderTargetDesc msaaRt{};
        msaaRt.width = width; msaaRt.height = height; msaaRt.samples = m_samples;
        msaaRt.debugName = "renderToTexture_msaaRT";
        RenderTargetAttachment ca{}; ca.type = AttachmentType::Color0; ca.texture = INVALID_HANDLE; msaaRt.colorAttachments.push_back(ca);
#ifndef __EMSCRIPTEN__
        RenderTargetAttachment da{}; da.type = AttachmentType::DepthStencil; da.texture = INVALID_HANDLE; msaaRt.depthAttachment = da;
#else
        RenderTargetAttachment da{}; da.type = AttachmentType::Depth; da.texture = INVALID_HANDLE; msaaRt.depthAttachment = da;
#endif
        RenderTargetHandle msaaHandle = m_rhi->createRenderTarget(msaaRt);
        if (msaaHandle == INVALID_HANDLE) {
            std::cerr << "[RenderSystem] renderToTexture(RHI): failed to create MSAA RT" << std::endl;
            ok = false;
        } else {
            // Render to multisampled RT
            m_rhi->bindRenderTarget(msaaHandle);
            m_rhi->setViewport(0, 0, width, height);
            m_rhi->clear(glm::vec4(0.10f, 0.11f, 0.12f, 1.0f), 1.0f, 0);
            if (m_renderMode == RenderMode::Raytrace) {
                renderRaytraced(scene, lights);
            } else {
                renderRasterized(scene, lights);
            }
            // Resolve into provided non-MSAA texture
            m_rhi->resolveRenderTarget(msaaHandle, textureHandle);
            // Cleanup
            m_rhi->bindRenderTarget(INVALID_HANDLE);
            m_rhi->destroyRenderTarget(msaaHandle);
        }
    } else {
        // Attach provided texture directly and render
        RenderTargetDesc rt{};
        rt.width = width; rt.height = height; rt.samples = 1; rt.debugName = "renderToTexture_RT";
        RenderTargetAttachment ca{}; ca.type = AttachmentType::Color0; ca.texture = textureHandle; rt.colorAttachments.push_back(ca);
#ifndef __EMSCRIPTEN__
        RenderTargetAttachment da{}; da.type = AttachmentType::DepthStencil; da.texture = INVALID_HANDLE; rt.depthAttachment = da;
#else
        RenderTargetAttachment da{}; da.type = AttachmentType::Depth; da.texture = INVALID_HANDLE; rt.depthAttachment = da;
#endif
        RenderTargetHandle rtHandle = m_rhi->createRenderTarget(rt);
        if (rtHandle == INVALID_HANDLE) {
            std::cerr << "[RenderSystem] renderToTexture(RHI): failed to create RT with provided texture" << std::endl;
            ok = false;
        } else {
            m_rhi->bindRenderTarget(rtHandle);
            m_rhi->setViewport(0, 0, width, height);
            m_rhi->clear(glm::vec4(0.10f, 0.11f, 0.12f, 1.0f), 1.0f, 0);
            if (m_renderMode == RenderMode::Raytrace) {
                renderRaytraced(scene, lights);
            } else {
                renderRasterized(scene, lights);
            }
            m_rhi->bindRenderTarget(INVALID_HANDLE);
            m_rhi->destroyRenderTarget(rtHandle);
        }
    }

    // Restore projection matrix; viewport restoration handled by caller/main render loop
    m_cameraManager.setProjectionMatrix(prevProj);

    return ok;
}

void RenderSystem::updateViewMatrix()
{
    m_cameraManager.updateViewMatrix();
}


void RenderSystem::updateProjectionMatrix(int windowWidth, int windowHeight)
{
    m_cameraManager.updateProjectionMatrix(windowWidth, windowHeight);
    if (m_rhi) {
        m_rhi->setViewport(0, 0, windowWidth, windowHeight);
    }
}


void RenderSystem::setReflectionSpp(int spp)
{
    m_reflectionSpp = std::max(1, spp); // Ensure at least 1 sample
    if (m_raytracer) {
        m_raytracer->setReflectionSpp(m_reflectionSpp);
    }
}

int RenderSystem::getReflectionSpp() const
{
    return m_reflectionSpp;
}

bool RenderSystem::denoise(std::vector<glm::vec3>& color,
                          const std::vector<glm::vec3>* normal,
                          const std::vector<glm::vec3>* albedo)
{
#ifdef OIDN_ENABLED
    try {
        if (color.empty()) {
            std::cerr << "[RenderSystem::denoise] Empty color buffer\n";
            return false;
        }

        // Create OIDN device
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();
        
        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN device error: " << errorMessage << "\n";
            return false;
        }

        // Try to guess square dimensions (common for raytracer)
        int width = static_cast<int>(std::sqrt(color.size()));
        int height = width;
        
        // Verify we have a proper square image
        if (width * height != static_cast<int>(color.size())) {
            std::cerr << "[RenderSystem::denoise] Cannot determine image dimensions from buffer size " 
                      << color.size() << ". Use the overload with explicit width/height.\n";
            return false;
        }

        // Create the filter for ray tracing denoising
        oidn::FilterRef filter = device.newFilter("RT"); // Ray tracing filter
        
        // Set input images
        filter.setImage("color", const_cast<void*>(static_cast<const void*>(color.data())), oidn::Format::Float3, width, height);
        
        // Optional: Set auxiliary buffers if available
        if (normal && normal->size() == color.size()) {
            filter.setImage("normal", const_cast<void*>(static_cast<const void*>(normal->data())), oidn::Format::Float3, width, height);
        }
        
        if (albedo && albedo->size() == color.size()) {
            filter.setImage("albedo", const_cast<void*>(static_cast<const void*>(albedo->data())), oidn::Format::Float3, width, height);
        }
        
        // Set output image (in-place denoising)
        filter.setImage("output", static_cast<void*>(color.data()), oidn::Format::Float3, width, height);
        
        // Set filter quality (optional - can be "fast", "balanced", "high")
        filter.set("hdr", true); // Enable HDR processing for better quality
        filter.set("srgb", false); // We're working in linear space
        
        // Commit the filter
        filter.commit();
        
        // Check for errors
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN filter setup error: " << errorMessage << "\n";
            return false;
        }

        // Execute the filter
        filter.execute();
        
        // Check for errors after execution
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN execution error: " << errorMessage << "\n";
            return false;
        }

        std::cout << "[RenderSystem::denoise] Successfully denoised " 
                  << width << "x" << height << " image\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[RenderSystem::denoise] Exception: " << e.what() << "\n";
        return false;
    }
#else
    // Fallback for builds without OIDN - just return false to indicate no denoising was done
    std::cout << "[RenderSystem::denoise] Intel Open Image Denoise not available in this build\n";
    return false;
#endif
}

bool RenderSystem::denoise(std::vector<glm::vec3>& color, int width, int height,
                          const std::vector<glm::vec3>* normal,
                          const std::vector<glm::vec3>* albedo)
{
#ifdef OIDN_ENABLED
    try {
        if (color.empty()) {
            std::cerr << "[RenderSystem::denoise] Empty color buffer\n";
            return false;
        }
        
        if (width <= 0 || height <= 0) {
            std::cerr << "[RenderSystem::denoise] Invalid dimensions: " << width << "x" << height << "\n";
            return false;
        }

        // Verify buffer size matches dimensions
        if (static_cast<int>(color.size()) != width * height) {
            std::cerr << "[RenderSystem::denoise] Buffer size " << color.size() 
                      << " doesn't match dimensions " << width << "x" << height << "\n";
            return false;
        }

        // Create OIDN device
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();
        
        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN device error: " << errorMessage << "\n";
            return false;
        }

        // Create the filter for ray tracing denoising
        oidn::FilterRef filter = device.newFilter("RT"); // Ray tracing filter
        
        // Set input images
        filter.setImage("color", const_cast<void*>(static_cast<const void*>(color.data())), oidn::Format::Float3, width, height);
        
        // Optional: Set auxiliary buffers if available
        if (normal && static_cast<int>(normal->size()) == width * height) {
            filter.setImage("normal", const_cast<void*>(static_cast<const void*>(normal->data())), oidn::Format::Float3, width, height);
        }
        
        if (albedo && static_cast<int>(albedo->size()) == width * height) {
            filter.setImage("albedo", const_cast<void*>(static_cast<const void*>(albedo->data())), oidn::Format::Float3, width, height);
        }
        
        // Set output image (in-place denoising)
        filter.setImage("output", static_cast<void*>(color.data()), oidn::Format::Float3, width, height);
        
        // Set filter quality
        filter.set("hdr", true); // Enable HDR processing for better quality
        filter.set("srgb", false); // We're working in linear space
        
        // Commit the filter
        filter.commit();
        
        // Check for errors
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN filter setup error: " << errorMessage << "\n";
            return false;
        }

        // Execute the filter
        filter.execute();
        
        // Check for errors after execution
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN execution error: " << errorMessage << "\n";
            return false;
        }

        std::cout << "[RenderSystem::denoise] Successfully denoised " 
                  << width << "x" << height << " image with explicit dimensions\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[RenderSystem::denoise] Exception: " << e.what() << "\n";
        return false;
    }
#else
    // Fallback for builds without OIDN
    std::cout << "[RenderSystem::denoise] Intel Open Image Denoise not available in this build (explicit dimensions version)\n";
    return false;
#endif
}

void RenderSystem::renderRasterized(const SceneManager& scene, const Light& lights)
{
    // Render skybox first as background
    if (m_showSkybox && m_skybox) {
        m_skybox->render(m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());
        m_stats.drawCalls += 1;
    }
    
    // Optimized object rendering with batching by material/shader
    renderObjectsBatched(scene, lights);
}

void RenderSystem::renderRaytraced(const SceneManager& scene, const Light& lights)
{
    if (!m_raytracer) {
        std::cerr << "[RenderSystem] Raytracer not initialized\n";
        return;
    }

    if (m_screenQuadPipeline == INVALID_HANDLE) {
        std::cerr << "[RenderSystem] Screen quad pipeline not created\n";
        return;
    }

    if (!m_screenQuadVAO)
        initScreenQuad();
    if (!m_raytraceTexture)
        initRaytraceTexture();

    // Clear existing raytracer data and load all scene objects
    m_raytracer = std::make_unique<Raytracer>();
    
    // Set the seed for deterministic rendering
    m_raytracer->setSeed(m_seed);
    
    // Set reflection samples per pixel for glossy reflections
    m_raytracer->setReflectionSpp(m_reflectionSpp);
    
    const auto& objects = scene.getObjects();
    std::cout << "[RenderSystem] Loading " << objects.size() << " objects into raytracer\n";
    
    for (const auto& obj : objects) {
        if (obj.objLoader.getVertCount() == 0) continue; // Skip objects with no geometry
        
        // Load object into raytracer with its transform and material
        // Calculate reflectivity from unified MaterialCore
        const auto& mc = obj.materialCore;
        float reflectivity = 0.1f; // Default reflectivity

        // For metallic materials, use metallic value as reflectivity multiplier
        if (mc.metallic > 0.1f) {
            reflectivity = 0.3f + (mc.metallic * 0.7f); // Range 0.3 to 1.0 based on metallic
        }

        // ðŸš¨ DEPRECATED CONVERSION: MaterialCore â†’ legacy Material for raytracer API
        // TODO CLEANUP: Update Raytracer::loadModel() to accept MaterialCore directly
        // CLEANUP TASK: Modify raytracer.h signature and eliminate this conversion
        // Using MaterialCore directly (no conversion needed)
        

        m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, reflectivity, mc);
    }

    // Create output buffer for raytraced image
    std::vector<glm::vec3> raytraceBuffer(m_raytraceWidth * m_raytraceHeight);
    
    std::cout << "[RenderSystem] Raytracing " << m_raytraceWidth << "x" << m_raytraceHeight << " image...\n";
    
    // Render the image using the raytracer
    if (m_raytracer) {
        m_raytracer->setSeed(m_seed);
    }
    m_raytracer->renderImage(raytraceBuffer, m_raytraceWidth, m_raytraceHeight,
                            m_cameraManager.camera().position, m_cameraManager.camera().front, m_cameraManager.camera().up, 
                            m_cameraManager.camera().fov, lights);
    
    // Apply OIDN denoising if enabled
    if (m_denoiseEnabled) {
        std::cout << "[RenderSystem] Applying OIDN denoising...\n";
        if (!denoise(raytraceBuffer, m_raytraceWidth, m_raytraceHeight)) {
            std::cerr << "[RenderSystem] Denoising failed, using raw raytraced image\n";
        }
    }

    // Upload raytraced image to texture (RHI required)
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        m_rhi->updateTexture(m_raytraceTextureRhi, raytraceBuffer.data(),
                           m_raytraceWidth, m_raytraceHeight, TextureFormat::RGB32F);
    }

    // Clear the screen (RHI required)
    if (m_rhi) {
        m_rhi->clear(glm::vec4(m_backgroundColor, 1.0f), 1.0f, 0);
    }
    // Note: Depth testing state managed by pipeline
    
    // Render the raytraced result using screen quad with RHI pipeline
    // Set post-processing uniforms via RHI
    if (m_rhi) {
        m_rhi->setUniformFloat("exposure", m_exposure);
        m_rhi->setUniformFloat("gamma", m_gamma);
        m_rhi->setUniformInt("toneMappingMode", static_cast<int>(m_tonemap));
    }

    // Bind the raytraced texture (RHI required)
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        m_rhi->bindTexture(m_raytraceTextureRhi, 0);
        m_rhi->setUniformInt("rayTex", 0);

        // Draw the screen quad using RHI with rayscreen pipeline
        BufferHandle screenQuadBuffer = m_rhi->getScreenQuadBuffer();
        if (screenQuadBuffer != INVALID_HANDLE) {
            DrawDesc drawDesc{};
            drawDesc.pipeline = m_screenQuadPipeline;  // Use RHI pipeline instead of legacy shader
            drawDesc.vertexBuffer = screenQuadBuffer;
            drawDesc.vertexCount = 6;
            drawDesc.instanceCount = 1;
            m_rhi->draw(drawDesc);
            m_stats.drawCalls += 1;
        }
    }
    // Note: Depth testing state managed by pipeline
    
    std::cout << "[RenderSystem] Raytracing complete\n";
}

void RenderSystem::renderObject(const SceneObject& obj, const Light& lights)
{
    // RHI-only object rendering with PBR pipeline
    if (obj.rhiVboPositions == INVALID_HANDLE) return;
    if (!m_rhi) return;

    // Ensure PBR pipeline is bound before setting uniforms
    ensureObjectPipeline(const_cast<SceneObject&>(obj), true); // Always use PBR

    // Set common uniforms (texture binding only)
    setupCommonUniforms();

    // FEAT-0249: Update UBOs for this object via managers
    // Update transform UBO with object-specific model matrix
    m_transformManager.updateTransforms(obj.modelMatrix, m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());

    // Update material UBO for this object
    m_materialManager.updateMaterialForObject(obj);

    // Lighting UBO is now handled by LightingManager via bindUniformBlocks()
    // Note: shadingMode now in RenderingBlock UBO

    // Bind PBR textures - RHI texture binding
    int unit = 0;
    if (obj.baseColorTex && obj.baseColorTex->rhiHandle() != INVALID_HANDLE) {
        m_rhi->bindTexture(obj.baseColorTex->rhiHandle(), unit);
        m_rhi->setUniformInt("baseColorTex", unit);
        unit++;
    }
    if (obj.normalTex && obj.normalTex->rhiHandle() != INVALID_HANDLE) {
        m_rhi->bindTexture(obj.normalTex->rhiHandle(), unit);
        m_rhi->setUniformInt("normalTex", unit);
        unit++;
    }
    if (obj.mrTex && obj.mrTex->rhiHandle() != INVALID_HANDLE) {
        m_rhi->bindTexture(obj.mrTex->rhiHandle(), unit);
        m_rhi->setUniformInt("mrTex", unit);
    }

    // Bind dummy shadow map - RHI texture binding
    if (s_dummyShadowTexRhi != INVALID_HANDLE) {
        m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
        m_rhi->setUniformInt("shadowMap", 7);
    }
    // FEAT-0249: lightSpaceMatrix now part of TransformBlock UBO

    // Draw call using RHI with PBR pipeline
    DrawDesc dd{};
    dd.pipeline = obj.rhiPipelinePbr != INVALID_HANDLE ? obj.rhiPipelinePbr : m_pbrPipeline;

    // Use indexed draw if RHI index buffer was created
    bool hasIndex = (obj.rhiEbo != INVALID_HANDLE);
    if (hasIndex) {
        dd.indexBuffer = obj.rhiEbo;
        dd.indexCount = obj.objLoader.getIndexCount();
    } else {
        dd.vertexCount = obj.objLoader.getVertCount();
    }
    m_rhi->draw(dd);

    m_stats.drawCalls += 1;
}

void RenderSystem::updateRenderStats(const SceneManager& scene)
{
    // Update rendering statistics (non-invasive; keep existing drawCalls accumulated during render)
    const auto& objects = scene.getObjects();

    // Triangles in scene geometry
    size_t tris = 0;
    for (const auto& obj : objects) {
        tris += static_cast<size_t>(obj.objLoader.getIndexCount()) / 3u;
    }
    m_stats.totalTriangles = tris;

    // Unique textures and total texture memory estimate
    std::unordered_set<const Texture*> uniqueTex;
    size_t textureBytes = 0;
    for (const auto& obj : objects) {
        const Texture* texes[4] = { obj.texture, obj.baseColorTex, obj.normalTex, obj.mrTex };
        for (const Texture* t : texes) {
            if (!t) continue;
            if (uniqueTex.insert(t).second) {
                // New unique texture; bytes = W * H * channels (approx; compressed formats use channels=4)
                const int w = t->width();
                const int h = t->height();
                const int c = std::max(1, t->channels());
                textureBytes += static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c);
            }
        }
    }
    m_stats.uniqueTextures = uniqueTex.size();
    m_stats.texturesMB = static_cast<float>(textureBytes) / (1024.0f * 1024.0f);

    // Geometry memory estimate (positions + normals + uvs + tangents + indices)
    size_t geoBytes = 0;
    for (const auto& obj : objects) {
        const size_t vcount = static_cast<size_t>(obj.objLoader.getVertCount());
        const size_t icount = static_cast<size_t>(obj.objLoader.getIndexCount());
        // positions (3 floats)
        geoBytes += vcount * 3u * sizeof(float);
        // normals if present
        if (obj.objLoader.getNormals()) {
            geoBytes += vcount * 3u * sizeof(float);
        }
        // uvs if present
        if (obj.objLoader.hasTexcoords()) {
            geoBytes += vcount * 2u * sizeof(float);
        }
        // tangents if present
        if (obj.objLoader.hasTangents()) {
            geoBytes += vcount * 3u * sizeof(float);
        }
        // indices (uint32)
        geoBytes += icount * sizeof(unsigned int);
    }
    m_stats.geometryMB = static_cast<float>(geoBytes) / (1024.0f * 1024.0f);

    // Unique material keys and top-shared material key
    struct MatKeyHash {
        size_t operator()(const std::string& s) const noexcept { return std::hash<std::string>()(s); }
    };
    std::unordered_map<std::string, int, MatKeyHash> matCounts;
    matCounts.reserve(objects.size());
    auto makeMatKey = [](const SceneObject& o) -> std::string {
        // Build compact string key from unified MaterialCore + texture presence
        char buf[256];
        const auto& mc = o.materialCore;
        // Reduce precision to keep keys compact; 2 decimals is enough for grouping
        snprintf(buf, sizeof(buf),
                 "BC%.2f,%.2f,%.2f,%.2f|M%.2f|R%.2f|IOR%.2f|T%.2f|E%.2f,%.2f,%.2f|t%d%d%d",
                 mc.baseColor.x, mc.baseColor.y, mc.baseColor.z, mc.baseColor.w,
                 mc.metallic, mc.roughness, mc.ior, mc.transmission,
                 mc.emissive.x, mc.emissive.y, mc.emissive.z,
                 o.baseColorTex ? 1 : 0, o.normalTex ? 1 : 0, o.mrTex ? 1 : 0);
        return std::string(buf);
    };
    for (const auto& obj : objects) {
        std::string key = makeMatKey(obj);
        matCounts[key] += 1;
    }
    m_stats.uniqueMaterialKeys = static_cast<int>(matCounts.size());
    // Compute most shared key
    m_stats.topSharedCount = 0;
    m_stats.topSharedKey.clear();
    for (const auto& kv : matCounts) {
        if (kv.second > m_stats.topSharedCount) {
            m_stats.topSharedCount = kv.second;
            m_stats.topSharedKey = kv.first;
        }
    }

    // Final VRAM estimate
    m_stats.vramMB = m_stats.texturesMB + m_stats.geometryMB;
}

void RenderSystem::initScreenQuad()
{
    // Note: Screen quad now provided by RHI::getScreenQuadBuffer() - no manual setup needed
    std::cout << "[RenderSystem] Screen quad initialization delegated to RHI\n";
}

void RenderSystem::initRaytraceTexture()
{
    if (m_rhi) {
        // Use RHI path for raytracing texture
        TextureDesc desc{};
        desc.type = TextureType::Texture2D;
        desc.format = TextureFormat::RGB32F;
        desc.width = m_raytraceWidth;
        desc.height = m_raytraceHeight;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.initialData = nullptr;
        desc.initialDataSize = 0;
        desc.debugName = "RaytraceTexture";
        
        m_raytraceTextureRhi = m_rhi->createTexture(desc);
        if (m_raytraceTextureRhi != INVALID_HANDLE) {
            std::cout << "[RenderSystem] Raytracing texture initialized via RHI (" << m_raytraceWidth << "x" << m_raytraceHeight << "): " << m_raytraceTextureRhi << "\n";
            return;
        } else {
            std::cerr << "[RenderSystem] Failed to create raytracing texture via RHI, falling back to GL" << std::endl;
        }
    }
    
    // Note: GL fallback removed - raytracing texture must be created via RHI (lines 1710-1724)
    std::cerr << "[RenderSystem] ERROR: Failed to create raytracing texture via RHI - no GL fallback available\n";
}

void RenderSystem::renderDebugElements(const SceneManager& scene, const Light& lights)
{
    // Batch debug element rendering to minimize state changes
    if (m_showGrid && m_grid) {
        m_grid->render(m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());
        m_stats.drawCalls += 1;
    }
    if (m_showAxes && m_axisRenderer) {
        glm::mat4 I(1.0f);
        glm::mat4 view = m_cameraManager.viewMatrix();
        glm::mat4 proj = m_cameraManager.projectionMatrix();
        m_axisRenderer->render(I, view, proj);
        m_stats.drawCalls += 1;
    }
    
    // Light indicators
    lights.renderIndicators(m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix(), m_selectedLightIndex);
    m_stats.drawCalls += 1;

    // Selection outline for currently selected object (wireframe overlay)
    renderSelectionOutline(scene);
    
    // Draw gizmo at selected object's or light's center
    renderGizmo(scene, lights);
}

void RenderSystem::renderSelectionOutline(const SceneManager& scene)
{
    int selObj = scene.getSelectedObjectIndex();
    const auto& objs = scene.getObjects();
    if (selObj >= 0 && selObj < (int)objs.size()) {
        const auto& obj = objs[selObj];
        if (obj.rhiVboPositions != INVALID_HANDLE && m_rhi) {
            // Post-processing uniforms for standard shader (selection overlay respects gamma/exposure) - RHI only
            // Update per-object data in UBOs
            // Update transform and rendering state using managers
            m_transformManager.updateTransforms(obj.modelMatrix, m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());

            // Set object color for selection
            m_renderingManager.setObjectColor(glm::vec3(0.2f, 0.7f, 1.0f)); // cyan-ish for selection

            // Set material useTexture flag
            // Material UBO now handled by MaterialManager
            // Force bright ambient and no direct lights
            if (s_dummyShadowTexRhi != INVALID_HANDLE) {
                m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
                m_rhi->setUniformInt("shadowMap", 7);
            }
            // Note: lightSpaceMatrix now in TransformBlock UBO
            // Note: numLights, globalAmbient now in LightingBlock UBO
            // TODO: Add selection overlay lighting mode to LightingManager
            // For now, selection will use current scene lighting

            // Note: Legacy Material uniform struct eliminated - using MaterialBlock UBO only

            // Draw as wireframe overlay
            if (m_rhi) {
                // Create wireframe selection pipeline if needed
                PipelineHandle wireframePipeline = INVALID_HANDLE;
                auto it = m_wireframePipelines.find(&obj);
                if (it != m_wireframePipelines.end()) {
                    wireframePipeline = it->second;
                } else {
                    PipelineDesc pd{};
                    pd.topology = PrimitiveTopology::Triangles;
                    pd.shader = m_pbrShaderRhi;
                    pd.debugName = obj.name + ":pipeline_wireframe";
                    pd.polygonMode = PolygonMode::Line;
                    pd.lineWidth = 1.5f;
                    pd.polygonOffsetEnable = true;
                    pd.polygonOffsetFactor = -1.0f;
                    pd.polygonOffsetUnits = -1.0f;

                    bool hasNormals = (obj.rhiVboNormals != INVALID_HANDLE);
                    bool hasUVs = (obj.rhiVboTexCoords != INVALID_HANDLE);
                    VertexBinding bPos{}; bPos.binding = 0; bPos.stride = 3 * sizeof(float); bPos.buffer = obj.rhiVboPositions; pd.vertexBindings.push_back(bPos);
                    if (hasNormals) { VertexBinding bN{}; bN.binding = 1; bN.stride = 3 * sizeof(float); bN.buffer = obj.rhiVboNormals; pd.vertexBindings.push_back(bN); }
                    if (hasUVs) { VertexBinding bUV{}; bUV.binding = 2; bUV.stride = 2 * sizeof(float); bUV.buffer = obj.rhiVboTexCoords; pd.vertexBindings.push_back(bUV); }

                    VertexAttribute aPos{}; aPos.location = 0; aPos.binding = 0; aPos.format = TextureFormat::RGB32F; aPos.offset = 0; pd.vertexAttributes.push_back(aPos);
                    if (hasNormals) { VertexAttribute aN{}; aN.location = 1; aN.binding = 1; aN.format = TextureFormat::RGB32F; aN.offset = 0; pd.vertexAttributes.push_back(aN); }
                    if (hasUVs) { VertexAttribute aUV{}; aUV.location = 2; aUV.binding = 2; aUV.format = TextureFormat::RG32F; aUV.offset = 0; pd.vertexAttributes.push_back(aUV); }

                    pd.indexBuffer = obj.rhiEbo;
                    wireframePipeline = m_rhi->createPipeline(pd);
                    m_wireframePipelines[&obj] = wireframePipeline;
                }

                DrawDesc dd{};
                dd.pipeline = wireframePipeline;
                if (obj.rhiEbo != INVALID_HANDLE) {
                    dd.indexBuffer = obj.rhiEbo;
                    dd.indexCount = obj.objLoader.getIndexCount();
                } else {
                    dd.vertexCount = obj.objLoader.getVertCount();
                }
                m_rhi->draw(dd);
                m_stats.drawCalls += 1;
            }
        }
    }
}

void RenderSystem::renderGizmo(const SceneManager& scene, const Light& lights)
{
    if (m_gizmo) {
        int selObj = -1;
        const auto& objs = scene.getObjects();
        // Use SceneManager selection if available
        selObj = scene.getSelectedObjectIndex();
        bool haveObj = (selObj >= 0 && selObj < (int)objs.size());
        bool haveLight = (m_selectedLightIndex >= 0 && m_selectedLightIndex < (int)lights.m_lights.size());
        if (haveObj || haveLight) {
            glm::vec3 center(0.0f);
            glm::mat3 R(1.0f);
            if (haveObj) {
                const auto& obj = objs[selObj];
                center = glm::vec3(obj.modelMatrix[3]);
                if (m_gizmoLocal) {
                    glm::mat3 M3(obj.modelMatrix);
                    R[0] = glm::normalize(glm::vec3(M3[0]));
                    R[1] = glm::normalize(glm::vec3(M3[1]));
                    R[2] = glm::normalize(glm::vec3(M3[2]));
                }
            } else {
                center = lights.m_lights[(size_t)m_selectedLightIndex].position;
                R = glm::mat3(1.0f); // lights are treated as world-aligned
            }
            float dist = glm::length(m_cameraManager.camera().position - center);
            float gscale = glm::clamp(dist * 0.15f, 0.5f, 10.0f);
            m_gizmo->render(m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix(), center, R, gscale, m_gizmoAxis, m_gizmoMode);
            // Approximate draw call for gizmo
            m_stats.drawCalls += 1;
        }
    }
}

void RenderSystem::renderObjectsBatched(const SceneManager& scene, const Light& lights)
{
    const auto& objects = scene.getObjects();
    if (objects.empty()) return;
    
    // All objects use PBR shader (standard shader eliminated)
    std::vector<const SceneObject*> pbrShaderObjects;

    for (const auto& obj : objects) {
        if (obj.rhiVboPositions == INVALID_HANDLE) continue;
        pbrShaderObjects.push_back(&obj);
    }
    
    // Render all objects with PBR shader via RHI pipeline
    if (!pbrShaderObjects.empty()) {
        setupCommonUniforms(); // RHI texture binding only
        // Apply lighting via LightingManager UBO (no direct GL uniforms)
        if (m_rhi) {
            m_lightingManager.updateLighting(lights, m_cameraManager.camera().position);
            m_lightingManager.bindLightingUniforms();
        }
        for (const auto* obj : pbrShaderObjects) {
            renderObjectFast(*obj, lights);
        }
    }

    // Reset VAO binding once at the end (RHI handles binding cleanup automatically)
    // TODO[FEAT-0248]: Remove when all draw calls use RHI::draw() (no legacy VAO binding)
}

void RenderSystem::setupCommonUniforms()
{
    // FEAT-0249: Use manager-based UBO system
    // Transform UBO handled by TransformManager, updated per-object
    // Lighting UBO handled by LightingManager
    // Rendering params (shadingMode, exposure, gamma, toneMappingMode) in RenderingBlock UBO

    if (!m_rhi) return;

    // Bind dummy shadow map - RHI texture binding
    if (s_dummyShadowTexRhi != INVALID_HANDLE) {
        m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
        m_rhi->setUniformInt("shadowMap", 7);
    }
    // FEAT-0249: lightSpaceMatrix now part of TransformBlock UBO

    // Bind IBL textures if available - RHI texture binding
    if (m_iblSystem) {
        m_iblSystem->bindIBLTextures();
        m_rhi->setUniformInt("irradianceMap", 3);
        m_rhi->setUniformInt("prefilterMap", 4);
        m_rhi->setUniformInt("brdfLUT", 5);
        // Note: iblIntensity now in RenderingBlock UBO
    }
}

void RenderSystem::renderObjectFast(const SceneObject& obj, const Light& lights)
{
    // Fast object rendering with minimal per-object state changes via RHI pipelines
    if (m_rhi) {
        ensureObjectPipeline(const_cast<SceneObject&>(obj), true); // Always use PBR
        // FEAT-0249: model matrix now part of TransformBlock UBO, updated per-object
    }

    // FEAT-0249: PBR material data uses MaterialBlock UBO instead of individual uniforms
    // Material data set via updateMaterialUniformsForObject() -> MaterialBlock UBO
    // Texture flags in MaterialBlock UBO (set by updateMaterialUniformsForObject)

    int unit = 0;
    if (obj.baseColorTex && obj.baseColorTex->rhiHandle() != INVALID_HANDLE) {
        // RHI-only texture binding
        m_rhi->bindTexture(obj.baseColorTex->rhiHandle(), unit);
        m_rhi->setUniformInt("baseColorTex", unit);
        unit++;
    }
    if (obj.normalTex && obj.normalTex->rhiHandle() != INVALID_HANDLE) {
        // RHI-only texture binding
        m_rhi->bindTexture(obj.normalTex->rhiHandle(), unit);
        m_rhi->setUniformInt("normalTex", unit);
        unit++;
    }
    if (obj.mrTex && obj.mrTex->rhiHandle() != INVALID_HANDLE) {
        // RHI-only texture binding
        m_rhi->bindTexture(obj.mrTex->rhiHandle(), unit);
        m_rhi->setUniformInt("mrTex", unit);
    }
    // Draw call using RHI
    DrawDesc dd{};
    dd.pipeline = obj.rhiPipelinePbr != INVALID_HANDLE ? obj.rhiPipelinePbr : m_pbrPipeline; // Always use PBR pipeline
    bool hasIndex = (obj.rhiEbo != INVALID_HANDLE);
    if (hasIndex) {
        dd.indexBuffer = obj.rhiEbo;
        dd.indexCount = obj.objLoader.getIndexCount();
    } else {
        dd.vertexCount = obj.objLoader.getVertCount();
    }
    m_rhi->draw(dd);

    m_stats.drawCalls += 1;
}

void RenderSystem::cleanupRaytracing()
{
    // Note: Screen quad cleanup now handled by RHI::getScreenQuadBuffer() lifecycle

    // Cleanup RHI raytracing texture first
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        m_rhi->destroyTexture(m_raytraceTextureRhi);
        m_raytraceTextureRhi = INVALID_HANDLE;
    }

    // Note: Raytracing texture cleanup handled by RHI destroyTexture above
}

void RenderSystem::destroyTargets()
{
    // Destroy RHI render target first
    if (m_rhi && m_msaaRenderTarget != INVALID_HANDLE) {
        m_rhi->destroyRenderTarget(m_msaaRenderTarget);
        m_msaaRenderTarget = INVALID_HANDLE;
    }
}

void RenderSystem::createOrResizeTargets(int width, int height)
{
    // Destroy existing
    destroyTargets();

    if (m_samples <= 1 || !m_rhi) {
        // No offscreen MSAA path needed
        return;
    }

    // Create RHI-based MSAA render target
    RenderTargetDesc rtDesc;
    rtDesc.width = width;
    rtDesc.height = height;
    rtDesc.samples = m_samples;
    rtDesc.debugName = "MSAA Primary Render Target";

    // Add color attachment (RGBA8 renderbuffer-style)
    RenderTargetAttachment colorAttach;
    colorAttach.type = AttachmentType::Color0;
    colorAttach.texture = INVALID_HANDLE; // RHI will create renderbuffer automatically
    rtDesc.colorAttachments.push_back(colorAttach);

    // Add depth attachment
    RenderTargetAttachment depthAttach;
#ifndef __EMSCRIPTEN__
    depthAttach.type = AttachmentType::DepthStencil;
#else
    depthAttach.type = AttachmentType::Depth;
#endif
    depthAttach.texture = INVALID_HANDLE; // RHI will create renderbuffer automatically
    rtDesc.depthAttachment = depthAttach;

    // Create the render target through RHI
    m_msaaRenderTarget = m_rhi->createRenderTarget(rtDesc);

    if (m_msaaRenderTarget == INVALID_HANDLE) {
        std::cerr << "[RenderSystem] Failed to create MSAA render target, disabling MSAA\n";
        m_samples = 1;
        return;
    }

    std::cout << "[RenderSystem] Created RHI MSAA render target (" << width << "x" << height
              << ", " << m_samples << "x samples): " << m_msaaRenderTarget << std::endl;
}

void RenderSystem::initRenderGraphs()
{
    if (!m_rhi || m_pipelineSelector) {
        return;
    }

    std::cout << "[RenderSystem] Initializing render graphs" << std::endl;

    m_pipelineSelector = std::make_unique<RenderPipelineModeSelector>();

    // Create raster pipeline graph
    m_rasterGraph = std::make_unique<RenderGraph>(m_rhi.get());
    m_rasterGraph->addPass(std::make_unique<FrameSetupPass>());
    m_rasterGraph->addPass(std::make_unique<GBufferPass>());
    m_rasterGraph->addPass(std::make_unique<DeferredLightingPass>());
    m_rasterGraph->addPass(std::make_unique<OverlayPass>());
    m_rasterGraph->addPass(std::make_unique<ResolvePass>());
    m_rasterGraph->addPass(std::make_unique<PresentPass>());
    m_rasterGraph->addPass(std::make_unique<ReadbackPass>());

    // Create ray pipeline graph
    m_rayGraph = std::make_unique<RenderGraph>(m_rhi.get());
    m_rayGraph->addPass(std::make_unique<FrameSetupPass>());
    m_rayGraph->addPass(std::make_unique<RayIntegratorPass>());
    m_rayGraph->addPass(std::make_unique<RayDenoisePass>());
    m_rayGraph->addPass(std::make_unique<OverlayPass>());
    m_rayGraph->addPass(std::make_unique<PresentPass>());
    m_rayGraph->addPass(std::make_unique<ReadbackPass>());

    m_activePipelineMode = RenderPipelineMode::Raster;

    std::cout << "[RenderSystem] Render graphs initialized successfully" << std::endl;
}

RenderGraph* RenderSystem::getActiveGraph(RenderPipelineMode mode)
{
    switch (mode) {
    case RenderPipelineMode::Ray:
        return m_rayGraph.get();
    case RenderPipelineMode::Raster:
    default:
        return m_rasterGraph.get();
    }
}

void RenderSystem::passFrameSetup(const PassContext& ctx)
{
    if (!ctx.rhi) return;

    // Clear screen with background color using RHI
    ctx.rhi->clear(glm::vec4(m_backgroundColor, 1.0f), 1.0f, 0);

    // Update uniform blocks using managers
    m_transformManager.updateTransforms(glm::mat4(1.0f), m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());
    m_lightingManager.updateLighting(*ctx.lights, m_cameraManager.camera().position);
    // Material updates are per-object, handled in rendering loops
    m_renderingManager.updateRenderingState(m_exposure, m_gamma, m_tonemap, m_shadingMode, m_iblSystem.get());

    // Bind uniform blocks (now delegates to managers)
    bindUniformBlocks();

    // Render state is now handled by RHI pipeline configuration
    // Depth testing, culling, and winding order are set per-pipeline in createPipeline()
    // No direct GL state calls needed here
}

void RenderSystem::passRaster(const PassContext& ctx)
{
    if (!ctx.scene || !ctx.lights) return;

    // Update managers for this frame
    m_lightingManager.updateLighting(*ctx.lights, m_cameraManager.camera().position);

    // Bind all uniform blocks via managers
    m_lightingManager.bindLightingUniforms();
    m_materialManager.bindMaterialUniforms();

    // Render skybox first as background
    if (m_showSkybox && m_skybox) {
        m_skybox->render(ctx.viewMatrix, ctx.projMatrix);
        m_stats.drawCalls += 1;
    }

    // Render scene objects with batched approach using manager pipeline
    renderObjectsBatchedWithManagers(*ctx.scene, *ctx.lights);
}

void RenderSystem::passRaytrace(const PassContext& ctx, int sampleCount, int maxDepth)
{
    if (!ctx.scene || !ctx.lights) return;

    if (!m_raytracer) {
        std::cerr << "[RenderSystem] Raytracer not initialized\n";
        return;
    }

    if (m_screenQuadPipeline == INVALID_HANDLE) {
        std::cerr << "[RenderSystem] Screen quad pipeline not created\n";
        return;
    }

    if (!m_screenQuadVAO)
        initScreenQuad();
    if (!m_raytraceTexture)
        initRaytraceTexture();

    // Clear existing raytracer data and load all scene objects
    m_raytracer = std::make_unique<Raytracer>();

    // Set the seed for deterministic rendering
    m_raytracer->setSeed(m_seed);

    // Set reflection samples per pixel for glossy reflections
    m_raytracer->setReflectionSpp(m_reflectionSpp);

    const auto& objects = ctx.scene->getObjects();
    std::cout << "[RenderSystem] Loading " << objects.size() << " objects into raytracer\n";

    for (const auto& obj : objects) {
        if (obj.objLoader.getVertCount() == 0) continue; // Skip objects with no geometry

        // Load object into raytracer with its transform and material
        // Calculate reflectivity from unified MaterialCore
        const auto& mc = obj.materialCore;
        float reflectivity = 0.1f; // Default reflectivity

        // For metallic materials, use metallic value as reflectivity multiplier
        if (mc.metallic > 0.1f) {
            reflectivity = 0.3f + (mc.metallic * 0.7f); // Range 0.3 to 1.0 based on metallic
        }

        // 🚨 DEPRECATED CONVERSION: MaterialCore → legacy Material for raytracer API
        // TODO CLEANUP: Update Raytracer::loadModel() to accept MaterialCore directly
        // CLEANUP TASK: Modify raytracer.h signature and eliminate this conversion
        // Using MaterialCore directly (no conversion needed)
        

        m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, reflectivity, mc);
    }

    // Create output buffer for raytraced image
    std::vector<glm::vec3> raytraceBuffer(m_raytraceWidth * m_raytraceHeight);

    std::cout << "[RenderSystem] Raytracing " << m_raytraceWidth << "x" << m_raytraceHeight << " image...\n";

    // Render the image using the raytracer
    if (m_raytracer) {
        m_raytracer->setSeed(m_seed);
    }

    // Use camera from CameraManager directly
    const auto& cameraState = m_cameraManager.camera();

    m_raytracer->renderImage(raytraceBuffer, m_raytraceWidth, m_raytraceHeight,
                            cameraState.position, cameraState.front, cameraState.up,
                            cameraState.fov, *ctx.lights);

    // Apply OIDN denoising if enabled
    if (m_denoiseEnabled) {
        std::cout << "[RenderSystem] Applying OIDN denoising...\n";
        if (!denoise(raytraceBuffer, m_raytraceWidth, m_raytraceHeight)) {
            std::cerr << "[RenderSystem] Denoising failed, using raw raytraced image\n";
        }
    }

    // Upload raytraced image to RHI texture
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        m_rhi->updateTexture(m_raytraceTextureRhi, raytraceBuffer.data(),
                           m_raytraceWidth, m_raytraceHeight, TextureFormat::RGB32F);
    }

    // Render full-screen quad with raytraced texture using RHI pipeline
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE && m_screenQuadPipeline != INVALID_HANDLE) {
        m_rhi->setUniformInt("screenTexture", 0);
        m_rhi->bindTexture(m_raytraceTextureRhi, 0);

        // Use RHI screen quad buffer with rayscreen pipeline
        BufferHandle screenQuadBuffer = m_rhi->getScreenQuadBuffer();
        DrawDesc dd{};
        dd.pipeline = m_screenQuadPipeline;  // Use RHI pipeline instead of legacy shader
        dd.vertexBuffer = screenQuadBuffer;
        dd.vertexCount = 6;
        dd.instanceCount = 1;
        m_rhi->draw(dd);

        m_stats.drawCalls += 1;
    }
}

void RenderSystem::passRayDenoise(const PassContext& ctx, TextureHandle inputTexture, TextureHandle outputTexture)
{
    // Denoising is currently integrated into the raytracing pass itself
    // In the future, this could be separated to operate on texture handles
    // For now, denoising happens inline with ray rendering
    if (m_denoiseEnabled) {
        // Denoising logic is handled within passRaytrace for now
        // TODO: Extract to separate pass that operates on RHI textures
    }
}

void RenderSystem::passOverlays(const PassContext& ctx)
{
    if (!ctx.scene || !ctx.lights) return;
    if (!ctx.enableOverlays) return;

    // Batch debug element rendering to minimize state changes
    if (m_showGrid && m_grid) {
        m_grid->render(ctx.viewMatrix, ctx.projMatrix);
        m_stats.drawCalls += 1;
    }
    if (m_showAxes && m_axisRenderer) {
        glm::mat4 I(1.0f);
        glm::mat4 view = ctx.viewMatrix;
        glm::mat4 proj = ctx.projMatrix;
        m_axisRenderer->render(I, view, proj);
        m_stats.drawCalls += 1;
    }

    // Light indicators
    ctx.lights->renderIndicators(ctx.viewMatrix, ctx.projMatrix, m_selectedLightIndex);
    m_stats.drawCalls += 1;

    // Selection outline for currently selected object (wireframe overlay)
    renderSelectionOutline(*ctx.scene);

    // Draw gizmo at selected object's or light's center
    renderGizmo(*ctx.scene, *ctx.lights);
}

void RenderSystem::passResolve(const PassContext& ctx)
{
    // For now, MSAA resolve is handled automatically by OpenGL default framebuffer
    // In the future, we would implement explicit resolve from MSAA rendertarget
    if (ctx.resolveMsaa) {
        // TODO: Implement explicit MSAA resolve via RHI when custom render targets are used
    }
}

void RenderSystem::passPresent(const PassContext& ctx)
{
    // Present operation is handled by RHI endFrame() in renderUnified()
    // This pass is primarily for future custom framebuffer presentation
    if (ctx.finalizeFrame) {
        // Frame finalization is already handled at the RenderSystem level
        // Future: implement custom present operations here
    }
}

void RenderSystem::passReadback(const PassContext& ctx)
{
    if (!m_rhi) return;

    // Only perform readback if requested and we have an output texture
    if (!ctx.readback || ctx.outputTexture == INVALID_HANDLE) {
        return;
    }

    // Use RHI to perform readback from the output texture
    if (ctx.readback->destination && ctx.readback->size > 0) {
        // Create ReadbackDesc from the ReadbackRequest
        ReadbackDesc readbackDesc{};
        readbackDesc.sourceTexture = ctx.outputTexture;
        readbackDesc.format = ctx.readback->format;
        readbackDesc.x = ctx.readback->x;
        readbackDesc.y = ctx.readback->y;
        readbackDesc.width = ctx.readback->width;
        readbackDesc.height = ctx.readback->height;
        readbackDesc.destination = ctx.readback->destination;
        readbackDesc.destinationSize = ctx.readback->size;

        // Perform the actual readback via RHI
        m_rhi->readback(readbackDesc);

        std::cout << "[RenderSystem::passReadback] Successfully read back "
                  << ctx.readback->size << " bytes from output texture\n";
    } else {
        std::cerr << "[RenderSystem::passReadback] No readback buffer provided\n";
    }
}

void RenderSystem::passGBuffer(const PassContext& ctx, RenderTargetHandle gBufferRT)
{
    if (!m_rhi) return;

    // Bind G-buffer render target
    m_rhi->bindRenderTarget(gBufferRT);

    // Set viewport
    m_rhi->setViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    // Clear G-buffer attachments
    m_rhi->clear(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0);

    // Get or create G-buffer pipeline
    PipelineHandle gBufferPipeline = getOrCreateGBufferPipeline();
    if (gBufferPipeline == INVALID_HANDLE) {
        std::cerr << "RenderSystem::passGBuffer: Failed to create G-buffer pipeline" << std::endl;
        return;
    }

    // Bind the G-buffer pipeline
    m_rhi->bindPipeline(gBufferPipeline);

    // Render all objects to G-buffer using the managers for uniform data
    const auto& objects = ctx.scene->getObjects();
    for (const auto& obj : objects) {
        if (obj.rhiVboPositions == INVALID_HANDLE) continue;

        // Update material for this object via MaterialManager
        m_materialManager.updateMaterialForObject(obj);

        // Update per-object transform (model matrix)
        glm::mat4 model = obj.modelMatrix;
        m_rhi->setUniformMat4("model", model);

        // Bind textures if available
        int textureUnit = 0;
        if (obj.baseColorTex && obj.baseColorTex->rhiHandle() != INVALID_HANDLE) {
            m_rhi->bindTexture(obj.baseColorTex->rhiHandle(), textureUnit);
            m_rhi->setUniformInt("baseColorTex", textureUnit);
            textureUnit++;
        }

        if (obj.normalTex && obj.normalTex->rhiHandle() != INVALID_HANDLE) {
            m_rhi->bindTexture(obj.normalTex->rhiHandle(), textureUnit);
            m_rhi->setUniformInt("normalTex", textureUnit);
            textureUnit++;
        }

        if (obj.mrTex && obj.mrTex->rhiHandle() != INVALID_HANDLE) {
            m_rhi->bindTexture(obj.mrTex->rhiHandle(), textureUnit);
            m_rhi->setUniformInt("mrTex", textureUnit);
            textureUnit++;
        }

        // Draw the object
        DrawDesc drawDesc{};
        drawDesc.pipeline = gBufferPipeline;
        if (obj.rhiEbo != INVALID_HANDLE) {
            drawDesc.indexBuffer = obj.rhiEbo;
            drawDesc.indexCount = obj.objLoader.getIndexCount();
            drawDesc.vertexCount = 0;
        } else {
            drawDesc.vertexCount = obj.objLoader.getVertCount();
            drawDesc.indexCount = 0;
        }
        m_rhi->draw(drawDesc);

        // Update stats
        m_stats.drawCalls++;
        m_stats.totalTriangles += obj.objLoader.getIndexCount() / 3;
    }
}

void RenderSystem::passDeferredLighting(const PassContext& ctx, RenderTargetHandle outputRT,
                                       TextureHandle gBaseColor, TextureHandle gNormal,
                                       TextureHandle gPosition, TextureHandle gMaterial)
{
    if (!m_rhi) return;

    // Ensure screen quad is created
    createScreenQuad();
    if (m_screenQuadVBORhi == INVALID_HANDLE) {
        std::cerr << "RenderSystem::passDeferredLighting: Failed to create screen quad" << std::endl;
        return;
    }

    // Get or create deferred lighting pipeline
    PipelineHandle deferredPipeline = getOrCreateDeferredLightingPipeline();
    if (deferredPipeline == INVALID_HANDLE) {
        std::cerr << "RenderSystem::passDeferredLighting: Failed to create deferred lighting pipeline" << std::endl;
        return;
    }

    // Bind output render target
    m_rhi->bindRenderTarget(outputRT);

    // Set viewport
    m_rhi->setViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    // Clear output
    m_rhi->clear(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);

    // Bind deferred lighting pipeline
    m_rhi->bindPipeline(deferredPipeline);

    // Bind G-buffer textures to texture units
    int textureUnit = 0;
    if (gBaseColor != INVALID_HANDLE) {
        m_rhi->bindTexture(gBaseColor, textureUnit);
        m_rhi->setUniformInt("gBaseColor", textureUnit);
        textureUnit++;
    }

    if (gNormal != INVALID_HANDLE) {
        m_rhi->bindTexture(gNormal, textureUnit);
        m_rhi->setUniformInt("gNormal", textureUnit);
        textureUnit++;
    }

    if (gPosition != INVALID_HANDLE) {
        m_rhi->bindTexture(gPosition, textureUnit);
        m_rhi->setUniformInt("gPosition", textureUnit);
        textureUnit++;
    }

    if (gMaterial != INVALID_HANDLE) {
        m_rhi->bindTexture(gMaterial, textureUnit);
        m_rhi->setUniformInt("gMaterial", textureUnit);
        textureUnit++;
    }

    // Bind IBL textures if available and loaded (now that IBL system is RHI-based)
    // Note: IBL textures are only created after loadHDREnvironment() is called
    if (m_iblSystem) {
        auto irradianceMap = m_iblSystem->getIrradianceMap();
        auto prefilterMap = m_iblSystem->getPrefilterMap();
        auto brdfLUT = m_iblSystem->getBRDFLUT();

        // Only bind if textures are actually loaded (not just initialized to INVALID_HANDLE)
        if (irradianceMap != INVALID_HANDLE) {
            m_rhi->bindTexture(irradianceMap, textureUnit);
            m_rhi->setUniformInt("irradianceMap", textureUnit);
            m_rhi->setUniformInt("useIBL", 1);  // Enable IBL in shader
            textureUnit++;
        } else {
            m_rhi->setUniformInt("useIBL", 0);  // Disable IBL in shader if not loaded
        }

        if (prefilterMap != INVALID_HANDLE) {
            m_rhi->bindTexture(prefilterMap, textureUnit);
            m_rhi->setUniformInt("prefilterMap", textureUnit);
            textureUnit++;
        }

        if (brdfLUT != INVALID_HANDLE) {
            m_rhi->bindTexture(brdfLUT, textureUnit);
            m_rhi->setUniformInt("brdfLUT", textureUnit);
            textureUnit++;
        }
    } else {
        // No IBL system available
        m_rhi->setUniformInt("useIBL", 0);
    }

    // Draw screen quad (6 vertices, no index buffer)
    DrawDesc drawDesc{};
    drawDesc.pipeline = deferredPipeline;
    drawDesc.vertexBuffer = m_screenQuadVBORhi;
    drawDesc.vertexCount = 6;
    drawDesc.indexCount = 0;
    m_rhi->draw(drawDesc);

    // Update stats
    m_stats.drawCalls++;
}

void RenderSystem::passRayIntegrator(const PassContext& ctx, TextureHandle outputTexture, int sampleCount, int maxDepth)
{
    if (!ctx.scene || !ctx.lights || outputTexture == INVALID_HANDLE) return;

    if (!m_raytracer) {
        std::cerr << "[RenderSystem::passRayIntegrator] Raytracer not initialized\n";
        return;
    }

    if (!m_rhi) {
        std::cerr << "[RenderSystem::passRayIntegrator] RHI not available\n";
        return;
    }

    // Clear existing raytracer data and load all scene objects
    m_raytracer = std::make_unique<Raytracer>();

    // Set the seed for deterministic rendering
    m_raytracer->setSeed(m_seed);

    // Set reflection samples per pixel for glossy reflections
    m_raytracer->setReflectionSpp(m_reflectionSpp);

    const auto& objects = ctx.scene->getObjects();
    std::cout << "[RenderSystem::passRayIntegrator] Loading " << objects.size() << " objects into raytracer\n";

    for (const auto& obj : objects) {
        if (obj.objLoader.getVertCount() == 0) continue; // Skip objects with no geometry

        // Load object into raytracer with its transform and material
        // Calculate reflectivity from unified MaterialCore
        const auto& mc = obj.materialCore;
        float reflectivity = 0.1f; // Default reflectivity

        // For metallic materials, use metallic value as reflectivity multiplier
        if (mc.metallic > 0.1f) {
            reflectivity = 0.3f + (mc.metallic * 0.7f); // Range 0.3 to 1.0 based on metallic
        }

        m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, reflectivity, mc);
    }

    // Create output buffer for raytraced image
    std::vector<glm::vec3> raytraceBuffer(ctx.viewportWidth * ctx.viewportHeight);

    std::cout << "[RenderSystem::passRayIntegrator] Raytracing " << ctx.viewportWidth << "x" << ctx.viewportHeight << " image...\n";

    // Render the image using the raytracer with provided samples and max depth
    if (m_raytracer) {
        m_raytracer->setSeed(m_seed);
    }

    // Use camera from CameraManager directly
    const auto& cameraState = m_cameraManager.camera();

    m_raytracer->renderImage(raytraceBuffer, ctx.viewportWidth, ctx.viewportHeight,
                            cameraState.position, cameraState.front, cameraState.up,
                            cameraState.fov, *ctx.lights);

    // Upload raytraced image to output texture using RHI
    m_rhi->updateTexture(outputTexture, raytraceBuffer.data(),
                        ctx.viewportWidth, ctx.viewportHeight, TextureFormat::RGB32F);

    std::cout << "[RenderSystem::passRayIntegrator] Ray integration complete\n";
}

PipelineHandle RenderSystem::getOrCreateGBufferPipeline()
{
    if (m_gBufferPipeline != INVALID_HANDLE) {
        return m_gBufferPipeline;
    }

    if (!m_rhi) {
        std::cerr << "RenderSystem::getOrCreateGBufferPipeline: RHI is null" << std::endl;
        return INVALID_HANDLE;
    }

    // Load G-buffer shaders
    std::string vertexSource = loadTextFileRhi("engine/shaders/gbuffer.vert");
    std::string fragmentSource = loadTextFileRhi("engine/shaders/gbuffer.frag");

    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to load G-buffer shader files" << std::endl;
        return INVALID_HANDLE;
    }

    // Create shader using both vertex and fragment sources
    ShaderDesc gBufferShaderDesc{};
    gBufferShaderDesc.stages = shaderStageBits(ShaderStage::Vertex) | shaderStageBits(ShaderStage::Fragment);
    gBufferShaderDesc.vertexSource = vertexSource;
    gBufferShaderDesc.fragmentSource = fragmentSource;
    gBufferShaderDesc.debugName = "GBufferShader";

    ShaderHandle gBufferShader = m_rhi->createShader(gBufferShaderDesc);
    if (gBufferShader == INVALID_HANDLE) {
        std::cerr << "Failed to create G-buffer shader" << std::endl;
        return INVALID_HANDLE;
    }

    // Create pipeline descriptor
    PipelineDesc pipelineDesc{};
    pipelineDesc.shader = gBufferShader;

    // Set vertex attributes to match gbuffer.vert
    pipelineDesc.vertexAttributes = {
        {0, 0, TextureFormat::RGB32F, 0},  // aPos (vec3) at location 0
        {1, 0, TextureFormat::RGB32F, 12}, // aNormal (vec3) at location 1, offset 3*4 bytes
        {2, 0, TextureFormat::RG32F, 24},  // aUV (vec2) at location 2, offset 6*4 bytes
        {3, 0, TextureFormat::RGB32F, 32}  // aTangent (vec3) at location 3, offset 8*4 bytes
    };

    // Enable depth testing for G-buffer pass
    pipelineDesc.depthTestEnable = true;
    pipelineDesc.depthWriteEnable = true;

    m_gBufferPipeline = m_rhi->createPipeline(pipelineDesc);

    // Clean up shader handle (pipeline holds references)
    m_rhi->destroyShader(gBufferShader);

    if (m_gBufferPipeline == INVALID_HANDLE) {
        std::cerr << "Failed to create G-buffer pipeline" << std::endl;
    }

    return m_gBufferPipeline;
}

std::string RenderSystem::loadTextFileRhi(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void RenderSystem::createScreenQuad()
{
    if (m_screenQuadVBORhi != INVALID_HANDLE || !m_rhi) {
        return; // Already created or no RHI
    }

    // Screen quad vertices (NDC coordinates with UVs)
    float quadVertices[] = {
        // positions   // UVs
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    BufferDesc bufferDesc{};
    bufferDesc.type = BufferType::Vertex;
    bufferDesc.usage = BufferUsage::Static;
    bufferDesc.size = sizeof(quadVertices);
    bufferDesc.initialData = quadVertices;

    m_screenQuadVBORhi = m_rhi->createBuffer(bufferDesc);
}

PipelineHandle RenderSystem::getOrCreateDeferredLightingPipeline()
{
    if (m_deferredLightingPipeline != INVALID_HANDLE) {
        return m_deferredLightingPipeline;
    }

    if (!m_rhi) {
        std::cerr << "RenderSystem::getOrCreateDeferredLightingPipeline: RHI is null" << std::endl;
        return INVALID_HANDLE;
    }

    // Load deferred lighting shaders
    std::string vertexSource = loadTextFileRhi("engine/shaders/deferred.vert");
    std::string fragmentSource = loadTextFileRhi("engine/shaders/deferred.frag");

    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to load deferred lighting shader files" << std::endl;
        return INVALID_HANDLE;
    }

    // Create shader using both vertex and fragment sources
    ShaderDesc deferredShaderDesc{};
    deferredShaderDesc.stages = shaderStageBits(ShaderStage::Vertex) | shaderStageBits(ShaderStage::Fragment);
    deferredShaderDesc.vertexSource = vertexSource;
    deferredShaderDesc.fragmentSource = fragmentSource;
    deferredShaderDesc.debugName = "DeferredLightingShader";

    ShaderHandle deferredShader = m_rhi->createShader(deferredShaderDesc);
    if (deferredShader == INVALID_HANDLE) {
        std::cerr << "Failed to create deferred lighting shader" << std::endl;
        return INVALID_HANDLE;
    }

    // Create pipeline descriptor for screen quad
    PipelineDesc pipelineDesc{};
    pipelineDesc.shader = deferredShader;

    // Set vertex attributes for screen quad (position + UV)
    pipelineDesc.vertexAttributes = {
        {0, 0, TextureFormat::RG32F, 0},  // aPos (vec2) at location 0
        {1, 0, TextureFormat::RG32F, 8}   // aUV (vec2) at location 1, offset 2*4 bytes
    };

    // Disable depth testing for full-screen pass
    pipelineDesc.depthTestEnable = false;
    pipelineDesc.depthWriteEnable = false;

    m_deferredLightingPipeline = m_rhi->createPipeline(pipelineDesc);

    // Clean up shader handle
    m_rhi->destroyShader(deferredShader);

    if (m_deferredLightingPipeline == INVALID_HANDLE) {
        std::cerr << "Failed to create deferred lighting pipeline" << std::endl;
    }

    return m_deferredLightingPipeline;
}

void RenderSystem::renderObjectsBatchedWithManagers(const SceneManager& scene, const Light& lights)
{
    if (!m_rhi) return;

    const auto& objects = scene.getObjects();
    if (objects.empty()) return;

    // Batch objects by material/pipeline to minimize state changes
    for (const auto& obj : objects) {
        if (obj.rhiVboPositions == INVALID_HANDLE) continue; // Skip objects without valid geometry

        // Ensure the object has a valid pipeline using PipelineManager
        // Cast away const since we need to modify pipeline handles
        SceneObject& mutableObj = const_cast<SceneObject&>(obj);
        m_pipelineManager.ensureObjectPipeline(mutableObj, true);  // Use PBR pipeline

        // Update material for this specific object
        m_materialManager.updateMaterialForObject(obj);

        // Use the object's PBR pipeline
        PipelineHandle pipeline = m_pipelineManager.getObjectPipeline(obj, true);
        if (pipeline == INVALID_HANDLE) {
            continue;  // Skip if no valid pipeline
        }

        // Bind pipeline and render
        m_rhi->bindPipeline(pipeline);

        // Set per-object transform uniform (model matrix)
        // TODO: This should also be managed by a TransformManager
        m_rhi->setUniformMat4("model", obj.modelMatrix);

        // Bind textures if available
        int textureUnit = 0;
        if (obj.baseColorTex && obj.baseColorTex->rhiHandle() != INVALID_HANDLE) {
            m_rhi->bindTexture(obj.baseColorTex->rhiHandle(), textureUnit);
            m_rhi->setUniformInt("baseColorTex", textureUnit);
            textureUnit++;
        }

        if (obj.normalTex && obj.normalTex->rhiHandle() != INVALID_HANDLE) {
            m_rhi->bindTexture(obj.normalTex->rhiHandle(), textureUnit);
            m_rhi->setUniformInt("normalTex", textureUnit);
            textureUnit++;
        }

        if (obj.mrTex && obj.mrTex->rhiHandle() != INVALID_HANDLE) {
            m_rhi->bindTexture(obj.mrTex->rhiHandle(), textureUnit);
            m_rhi->setUniformInt("metallicRoughnessTex", textureUnit);
            textureUnit++;
        }

        // Draw the object using RHI draw command
        DrawDesc drawDesc{};
        drawDesc.pipeline = pipeline;
        if (obj.rhiEbo != INVALID_HANDLE) {
            // Indexed drawing
            drawDesc.indexBuffer = obj.rhiEbo;
            drawDesc.indexCount = obj.objLoader.getIndexCount();
            drawDesc.vertexCount = 0;  // Not used for indexed drawing
        } else {
            // Non-indexed drawing
            drawDesc.vertexCount = obj.objLoader.getVertCount();
            drawDesc.indexCount = 0;  // Not used for non-indexed drawing
        }
        m_rhi->draw(drawDesc);

        m_stats.drawCalls++;
        m_stats.totalTriangles += obj.objLoader.getIndexCount() / 3;  // Convert indices to triangles
    }
}