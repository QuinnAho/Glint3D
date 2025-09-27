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
#include "shader.h"
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

using namespace glint3d;

std::string RenderSystem::loadTextFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

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
    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
#ifndef __EMSCRIPTEN__
    if (m_framebufferSRGBEnabled) glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
#endif
    // Minimal RHI init (OpenGL backend)
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

    // Load shaders - unified PBR pipeline only (standard shader eliminated)
    m_pbrShader = std::make_unique<Shader>();
    m_pbrShader->load("engine/shaders/pbr.vert", "engine/shaders/pbr.frag");

    // For backward compatibility, point basicShader to PBR shader
    m_basicShader.reset(); // Clear the unique_ptr
    // Note: m_basicShader calls will use m_pbrShader instead

    m_gridShader = std::make_unique<Shader>();
    if (!m_gridShader->load("engine/shaders/grid.vert", "engine/shaders/grid.frag"))
        std::cerr << "[RenderSystem] Failed to load grid shader.\n";

    // Gradient background shader (for background mode = Gradient)
    m_gradientShader = std::make_unique<Shader>();
    if (!m_gradientShader->load("engine/shaders/gradient.vert", "engine/shaders/gradient.frag"))
        std::cerr << "[RenderSystem] Failed to load gradient shader.\n";

    // Load raytracing screen quad shader
    m_screenQuadShader = std::make_unique<Shader>();
    if (!m_screenQuadShader->load("engine/shaders/rayscreen.vert", "engine/shaders/rayscreen.frag"))
        std::cerr << "[RenderSystem] Failed to load rayscreen shader.\n";

    // Register RHI with Texture so cache can create matching RHI textures
    if (m_rhi) {
        Texture::setRHI(m_rhi.get());
        Shader::setRHI(m_rhi.get()); // Route shader uniforms through RHI
        Gizmo::setRHI(m_rhi.get()); // Route gizmo uniforms through RHI
        AxisRenderer::setRHI(m_rhi.get()); // Route axis renderer uniforms through RHI
        Grid::setRHI(m_rhi.get()); // Route grid uniforms through RHI
        Skybox::setRHI(m_rhi.get()); // Route skybox uniforms through RHI
    }

    // Init helpers
    if (m_grid) m_grid->init(m_gridShader.get(), 200, 1.0f);
    if (m_axisRenderer) m_axisRenderer->init();
    if (m_skybox) m_skybox->init();
    if (m_iblSystem) m_iblSystem->init();
    
    // Create shaders via RHI and minimal pipelines for fallback
    if (m_rhi) {
        // Create RHI shaders - unified PBR pipeline only
        ShaderDesc sdPbr{};
        sdPbr.vertexSource = loadTextFile("engine/shaders/pbr.vert");
        sdPbr.fragmentSource = loadTextFile("engine/shaders/pbr.frag");
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
    }

    // Initialize raytracing resources only when needed
    if (m_renderMode == RenderMode::Raytrace) {
        initScreenQuad();
        initRaytraceTexture();
    }
    if (m_gizmo) m_gizmo->init();

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
    
    // Fallback or parallel GL path for compatibility
    if (m_dummyShadowTexRhi == INVALID_HANDLE) {
        glGenTextures(1, &m_dummyShadowTex);
        glBindTexture(GL_TEXTURE_2D, m_dummyShadowTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depthOne);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifndef __EMSCRIPTEN__
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.f,1.f,1.f,1.f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cerr << "[RenderSystem] Created dummy shadow texture via GL: " << m_dummyShadowTex << std::endl;
    }

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
    m_basicShader.reset();
    m_pbrShader.reset();
    m_gridShader.reset();
    if (m_dummyShadowTex) { glDeleteTextures(1, &m_dummyShadowTex); m_dummyShadowTex = 0; }

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
        // Fallback to legacy if render graphs not initialized
        renderLegacy(scene, lights);
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
        std::cerr << "[RenderSystem] No render graph available for mode " << (int)mode << std::endl;
        renderLegacy(scene, lights);
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
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    ctx.viewportWidth = vp[2];
    ctx.viewportHeight = vp[3];

    // Frame state
    ctx.frameIndex = ++m_frameCounter;
    ctx.deltaTime = 0.016f; // TODO: get real delta time

    // Timing support
    ctx.enableTiming = true;
    ctx.passTimings = &m_stats.passTimings;

    // Setup render graph if needed
    if (!graph->setup(ctx)) {
        std::cerr << "[RenderSystem] Failed to setup render graph" << std::endl;
        renderLegacy(scene, lights);
        return;
    }

    // Execute render graph
    m_rhi->beginFrame();
    graph->execute(ctx);
    m_rhi->endFrame();
}

void RenderSystem::renderLegacy(const SceneManager& scene, const Light& lights)
{
    // Reset per-frame stats counters
    m_stats = {};

    // Update uniform blocks using managers (FEAT-0249)
    m_transformManager.updateTransforms(glm::mat4(1.0f), m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());
    m_lightingManager.updateLighting(lights, m_cameraManager.camera().position);
    // Material updates are per-object, handled in rendering loops
    m_renderingManager.updateRenderingState(m_exposure, m_gamma, m_tonemap, m_shadingMode, m_iblSystem.get());

    // Bind uniform blocks to their binding points (FEAT-0249)
    bindUniformBlocks();

    // Optimize clear operations - only clear if background changed
    static glm::vec3 lastBgColor{-1.0f};
    if (lastBgColor != m_backgroundColor) {
        glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);
        lastBgColor = m_backgroundColor;
    }
    // Recreate MSAA targets if viewport changed or flagged
    GLint vp[4] = {0,0,0,0};
    glGetIntegerv(GL_VIEWPORT, vp);
    if (vp[2] != m_fbWidth || vp[3] != m_fbHeight) {
        m_fbWidth = vp[2];
        m_fbHeight = vp[3];
        m_recreateTargets = true;
    }
    if (m_recreateTargets) {
        createOrResizeTargets(m_fbWidth, m_fbHeight);
        if (m_rhi) m_rhi->setViewport(0, 0, m_fbWidth, m_fbHeight);
        m_recreateTargets = false;
    }
    // Begin RHI frame
    if (m_rhi) m_rhi->beginFrame();

    // Bind target for rendering
    if (m_samples > 1 && m_rhi && m_msaaRenderTarget != INVALID_HANDLE) {
        m_rhi->bindRenderTarget(m_msaaRenderTarget);
    } else {
        // Bind default framebuffer (RHI handles null render target as default)
        if (m_rhi) m_rhi->bindRenderTarget(INVALID_HANDLE);
        else glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    // Use RHI for clearing (RHI should always be initialized at this point)
    if (m_rhi) {
        m_rhi->clear(glm::vec4(m_backgroundColor, 1.0f), 1.0f, 0);
    } else {
        // Fallback for edge cases where RHI failed to initialize
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // If using gradient background and no skybox, draw full-screen gradient
    if (m_bgMode == BackgroundMode::Gradient && !m_showSkybox && m_gradientShader) {
        if (!m_screenQuadVAO) {
            initScreenQuad();
        }
        glDisable(GL_DEPTH_TEST);
        m_gradientShader->use();
        m_gradientShader->setVec3("topColor", m_bgTop);
        m_gradientShader->setVec3("bottomColor", m_bgBottom);
        glBindVertexArray(m_screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        // counts as one draw call
        m_stats.drawCalls += 1;
    }
    
    // If using HDR background mode, render environment map as skybox background
    if (m_bgMode == BackgroundMode::HDR && !m_showSkybox && m_iblSystem) {
        GLuint envMap = m_iblSystem->getEnvironmentMap();
        if (envMap != 0 && m_skybox) {
            // Use the IBL environment map as the skybox texture
            m_skybox->setEnvironmentMap(envMap);
            m_skybox->render(m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());
            // counts as one draw call
            m_stats.drawCalls += 1;
        }
    }
    
    // Choose render path based on mode
    switch (m_renderMode) {
        case RenderMode::Raytrace:
            renderRaytraced(scene, lights);
            break;
        default:
            renderRasterized(scene, lights);
            break;
    }
    
    // Batch debug element rendering for fewer state changes
    renderDebugElements(scene, lights);

    // Selection outline for currently selected object (wireframe overlay)
    {
        int selObj = scene.getSelectedObjectIndex();
        const auto& objs = scene.getObjects();
        if (selObj >= 0 && selObj < (int)objs.size() && m_pbrShader) {
            const auto& obj = objs[selObj];
            if (obj.rhiVboPositions != INVALID_HANDLE) {
                Shader* s = m_pbrShader.get(); // Always use PBR shader
                if (m_rhi) {
                    ensureObjectPipeline(const_cast<SceneObject&>(obj), true); // Always use PBR
                    // FEAT-0249: Use UBO system for structured data

                    // Update transform UBO with object-specific data
                    // Update transform for this object
                    m_transformManager.updateTransforms(obj.modelMatrix, m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());

                    // TODO: Add special overlay lighting/material mode to managers
                    // For now, selection overlay lighting will use current scene lighting

                    // Non-UBO uniforms: only texture bindings
                    if (m_rhi) {
                        // Texture binding
                        if (s_dummyShadowTexRhi != 0) {
                            m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
                            m_rhi->setUniformInt("shadowMap", 7);
                        }
                    }
                }

                // Draw as wireframe overlay with slight depth bias to reduce z-fighting
#ifndef __EMSCRIPTEN__
                GLint prevPolyMode[2];
                glGetIntegerv(GL_POLYGON_MODE, prevPolyMode);
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(1.5f);
#endif
                if (m_rhi && ((obj.rhiPipelineBasic != INVALID_HANDLE) || m_basicPipeline != INVALID_HANDLE)) {
                    DrawDesc dd{};
                    dd.pipeline = (obj.rhiPipelineBasic != INVALID_HANDLE) ? obj.rhiPipelineBasic : m_basicPipeline;
                    bool hasIndex = (obj.rhiEbo != INVALID_HANDLE) || (obj.rhiEbo != INVALID_HANDLE);
                    if (hasIndex) {
                        dd.indexBuffer = obj.rhiEbo;
                        dd.indexCount = obj.objLoader.getIndexCount();
                    } else {
                        dd.vertexCount = obj.objLoader.getVertCount();
                    }
                    m_rhi->draw(dd);
                } else {
                    // TODO: Bind RHI vertex buffers and pipeline instead of legacy VAO
                    if (obj.rhiEbo != INVALID_HANDLE) {
                        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
                    } else {
                        glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
                    }
                    glBindVertexArray(0);
                }
                // Selection overlay adds an extra draw call
                m_stats.drawCalls += 1;
#ifndef __EMSCRIPTEN__
                glPolygonMode(GL_FRONT_AND_BACK, prevPolyMode[0]);
                glDisable(GL_POLYGON_OFFSET_LINE);
#endif
            }
        }
    }

    // Draw gizmo at selected object's or light's center
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
    
    updateRenderStats(scene);

    // Resolve MSAA render to default framebuffer if enabled
    if (m_samples > 1 && m_rhi && m_msaaRenderTarget != INVALID_HANDLE) {
        m_rhi->resolveToDefaultFramebuffer(m_msaaRenderTarget);
    }

    // End RHI frame
    if (m_rhi) m_rhi->endFrame();
}

bool RenderSystem::loadSkybox(const std::string& path)
{
    // Minimal implementation: ensure skybox is initialized and enable it.
    // If needed, future enhancement can parse `path` for cubemap faces.
    if (!m_skybox) return false;
    if (!m_skybox->init()) return false;
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

bool RenderSystem::renderToTexture(const SceneManager& scene, const Light& lights,
                                  GLuint textureId, int width, int height)
{
    // TODO[FEAT-0253]: Legacy GL FBO path
    // This function renders to a provided GL texture via direct FBO operations.
    // It predates the RHI offscreen API and remains as a compatibility layer
    // for call sites that provide a raw GLuint texture. Once call sites migrate
    // to RHI textures + RenderTarget, replace this implementation with an RHI
    // render path and delete the GL framebuffer code below.
    std::cerr << "[RenderSystem] renderToTexture called with textureId=" << textureId << ", width=" << width << ", height=" << height << std::endl;
    if (textureId == 0 || width <= 0 || height <= 0) {
        std::cerr << "[RenderSystem] renderToTexture: invalid parameters" << std::endl;
        return false;
    }

    // Preserve current framebuffer and viewport
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Use offscreen aspect ratio; restore later
    glm::mat4 prevProj = m_cameraManager.projectionMatrix();
    updateProjectionMatrix(width, height);

    std::cerr << "[RenderSystem] m_samples=" << m_samples << std::endl;
    if (m_samples > 1) {
        std::cerr << "[RenderSystem] Using MSAA path" << std::endl;
        // TODO[FEAT-0253]: Legacy GL MSAA resolve path
        // MSAA path: render into multisampled RBOs and resolve into provided texture.
        // This will be replaced by RHI RenderTarget + resolve once call sites
        // provide RHI texture handles instead of raw GL texture IDs.
        GLuint fboMSAA = 0, rboColorMSAA = 0, rboDepthMSAA = 0;
        GLuint fboResolve = 0;

        glGenFramebuffers(1, &fboMSAA);
        glBindFramebuffer(GL_FRAMEBUFFER, fboMSAA);

        glGenRenderbuffers(1, &rboColorMSAA);
        glBindRenderbuffer(GL_RENDERBUFFER, rboColorMSAA);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboColorMSAA);

        glGenRenderbuffers(1, &rboDepthMSAA);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepthMSAA);
#ifndef __EMSCRIPTEN__
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthMSAA);
#else
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH_COMPONENT16, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepthMSAA);
#endif

        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[RenderSystem] MSAA framebuffer not complete: 0x" << std::hex << fboStatus << std::dec << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            if (rboColorMSAA) glDeleteRenderbuffers(1, &rboColorMSAA);
            if (rboDepthMSAA) glDeleteRenderbuffers(1, &rboDepthMSAA);
            if (fboMSAA) glDeleteFramebuffers(1, &fboMSAA);
            m_cameraManager.setProjectionMatrix(prevProj);
            // Restore viewport via RHI
            if (m_rhi) {
                m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            } else {
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            }
            return false;
        }

        // Render to MSAA framebuffer - use RHI for viewport and clear
        if (m_rhi) {
            m_rhi->setViewport(0, 0, width, height);
            m_rhi->clear(glm::vec4(0.10f, 0.11f, 0.12f, 1.0f), 1.0f, 0);
        } else {
            glViewport(0, 0, width, height);
            glClearColor(0.10f, 0.11f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        if (m_renderMode == RenderMode::Raytrace) {
            renderRaytraced(scene, lights);
        } else {
            renderRasterized(scene, lights);
        }

        // Create resolve framebuffer with provided texture
        glGenFramebuffers(1, &fboResolve);
        glBindFramebuffer(GL_FRAMEBUFFER, fboResolve);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
#ifndef __EMSCRIPTEN__
        const GLenum drawBufsR[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBufsR);
#endif
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            if (fboResolve) glDeleteFramebuffers(1, &fboResolve);
            if (rboColorMSAA) glDeleteRenderbuffers(1, &rboColorMSAA);
            if (rboDepthMSAA) glDeleteRenderbuffers(1, &rboDepthMSAA);
            if (fboMSAA) glDeleteFramebuffers(1, &fboMSAA);
            m_cameraManager.setProjectionMatrix(prevProj);
            // Restore viewport via RHI
            if (m_rhi) {
                m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            } else {
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            }
            return false;
        }

        // Resolve
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMSAA);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboResolve);
        glBlitFramebuffer(0, 0, width, height,
                          0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Cleanup and restore
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        if (fboResolve) glDeleteFramebuffers(1, &fboResolve);
        if (rboColorMSAA) glDeleteRenderbuffers(1, &rboColorMSAA);
        if (rboDepthMSAA) glDeleteRenderbuffers(1, &rboDepthMSAA);
        if (fboMSAA) glDeleteFramebuffers(1, &fboMSAA);

        m_cameraManager.setProjectionMatrix(prevProj);
        // Restore viewport via RHI
            if (m_rhi) {
                m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            } else {
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            }
        return true;
    } else {
        std::cerr << "[RenderSystem] Using non-MSAA path" << std::endl;
        // TODO[FEAT-0253]: Legacy GL single-sample path (previous behavior)
        // Replace with RHI RenderTarget when callers migrate off raw GL texture IDs.
        // Create FBO
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Attach provided color texture
        std::cerr << "[RenderSystem] Attaching color texture " << textureId << " to FBO" << std::endl;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
#ifndef __EMSCRIPTEN__
        const GLenum drawBufs[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBufs);
#endif

        // Create and attach depth (and stencil if available) renderbuffer
        GLuint rboDepth = 0;
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        // Try different depth formats for maximum compatibility
        bool depthAttached = false;
        
        // Try GL_DEPTH_COMPONENT24 first (desktop preferred)
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            depthAttached = true;
        }
        
        // Fallback to GL_DEPTH_COMPONENT16 if 24-bit failed
        if (!depthAttached) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
                depthAttached = true;
            }
        }
        
        // Final fallback: try without depth buffer
        if (!depthAttached) {
            std::cerr << "[RenderSystem] Warning: Unable to attach depth buffer, proceeding without depth testing" << std::endl;
            glDeleteRenderbuffers(1, &rboDepth);
            rboDepth = 0;
        }

        // Validate
        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[RenderSystem] Non-MSAA framebuffer not complete: 0x" << std::hex << fboStatus << std::dec << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            if (rboDepth != 0) glDeleteRenderbuffers(1, &rboDepth);
            if (fbo != 0) glDeleteFramebuffers(1, &fbo);
            m_cameraManager.setProjectionMatrix(prevProj);
            // Restore viewport via RHI
            if (m_rhi) {
                m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            } else {
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            }
            return false;
        }

        // Set viewport and clear
        glViewport(0, 0, width, height);
        glClearColor(0.10f, 0.11f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render scene using current mode
        if (m_renderMode == RenderMode::Raytrace) {
            renderRaytraced(scene, lights);
        } else {
            renderRasterized(scene, lights);
        }

        // Restore projection, framebuffer and viewport
        m_cameraManager.setProjectionMatrix(prevProj);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        // Restore viewport via RHI
            if (m_rhi) {
                m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            } else {
                glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            }

        // Cleanup
        glDeleteRenderbuffers(1, &rboDepth);
        glDeleteFramebuffers(1, &fbo);
        return true;
    }
}

bool RenderSystem::renderToPNG(const SceneManager& scene, const Light& lights,
                               const std::string& path, int width, int height)
{
#ifdef __EMSCRIPTEN__
    (void)scene; (void)lights; (void)path; (void)width; (void)height;
    std::cerr << "renderToPNG is not supported on Web builds.\n";
    return false;
#else
    if (width <= 0 || height <= 0) return false;

    // Preserve current framebuffer and viewport
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // RHI-first path: render using the new RHI overload, then readback
    if (m_rhi) {
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

                // Restore GL framebuffer binding and viewport
                glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
                m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

                m_rhi->destroyTexture(colorTexHandle);
                return writeOK != 0;
            } else {
                // Cleanup and fall back to GL path
                m_rhi->destroyTexture(colorTexHandle);
            }
        }
        // If RHI path failed, fall through to GL fallback below
    }

    // GL fallback path: create a GL texture, render via legacy overload, and read back
    GLuint colorTex = 0;
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cerr << "[RenderSystem] About to render to texture (GL fallback), colorTex=" << colorTex << std::endl;
    bool ok = renderToTexture(scene, lights, colorTex, width, height);
    std::cerr << "[RenderSystem] renderToTexture returned " << (ok ? "true" : "false") << std::endl;
    if (!ok) {
        if (colorTex) glDeleteTextures(1, &colorTex);
        return false;
    }

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glDeleteFramebuffers(1, &fbo);
        if (colorTex) glDeleteTextures(1, &colorTex);
        return false;
    }

    glViewport(0, 0, width, height);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    const int comp = 4;
    const int rowStride = width * comp;
    std::vector<std::uint8_t> pixels(height * rowStride);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<std::uint8_t> flipped(height * rowStride);
    for (int y = 0; y < height; ++y) {
        std::memcpy(flipped.data() + (height - 1 - y) * rowStride,
                    pixels.data() + y * rowStride,
                    rowStride);
    }

    int writeOK = stbi_write_png(path.c_str(), width, height, comp, flipped.data(), rowStride);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    if (m_rhi) {
        m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    } else {
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }

    glDeleteFramebuffers(1, &fbo);
    if (colorTex) {
        glDeleteTextures(1, &colorTex);
    }

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

    // Preserve current framebuffer (GL) and viewport to restore; RHI has no query API
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);
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

    // Restore projection and viewport
    m_cameraManager.setProjectionMatrix(prevProj);
    if (m_rhi) {
        m_rhi->setViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    } else {
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }

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

    if (!m_screenQuadShader) {
        std::cerr << "[RenderSystem] Screen quad shader not loaded\n";
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
    
    // Upload raytraced image to texture
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        // Use RHI to update texture data
        m_rhi->updateBuffer(INVALID_HANDLE, raytraceBuffer.data(), raytraceBuffer.size() * sizeof(glm::vec3));
        // TODO: Implement texture update via RHI - for now fall back to GL
        glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_raytraceWidth, m_raytraceHeight,
                        GL_RGB, GL_FLOAT, raytraceBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_raytraceWidth, m_raytraceHeight,
                        GL_RGB, GL_FLOAT, raytraceBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST); // Disable depth testing for screen quad
    
    // Render the raytraced result using screen quad
    m_screenQuadShader->use();
    
    // Set post-processing uniforms
    m_screenQuadShader->setFloat("exposure", m_exposure);
    m_screenQuadShader->setFloat("gamma", m_gamma);
    m_screenQuadShader->setInt("toneMappingMode", static_cast<int>(m_tonemap));
    
    // Bind the raytraced texture
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        m_rhi->bindTexture(m_raytraceTextureRhi, 0);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
    }
    m_screenQuadShader->setInt("rayTex", 0);
    
    // Draw the screen quad
    glBindVertexArray(m_screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    // One draw call for the screen quad
    m_stats.drawCalls += 1;
    
    glEnable(GL_DEPTH_TEST); // Re-enable depth testing
    
    std::cout << "[RenderSystem] Raytracing complete\n";
}

void RenderSystem::renderObject(const SceneObject& obj, const Light& lights)
{
    // Basic object rendering - optimized for minimal state changes
    if (obj.rhiVboPositions == INVALID_HANDLE) return;

    // Always use PBR shader (standard shader eliminated)
    Shader* s = m_pbrShader.get();
    if (!s) return;
    
    // Ensure pipeline is bound before setting uniforms in RHI path
    if (m_rhi) {
        ensureObjectPipeline(const_cast<SceneObject&>(obj), true); // Always use PBR
    } else {
        // Cache shader state to avoid redundant use() calls
        static Shader* lastShader = nullptr;
        if (s != lastShader) { s->use(); lastShader = s; }
    }
    // Set common uniforms
    setupCommonUniforms(s);

    // FEAT-0249: Update UBOs for this object instead of individual uniforms
    if (m_rhi) {
        // Update transform UBO with object-specific model matrix
        // Update transform UBO using manager
        m_transformManager.updateTransforms(obj.modelMatrix, m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());

        // Update material UBO for this object
        m_materialManager.updateMaterialForObject(obj);

        // Lighting UBO is now handled by LightingManager via bindUniformBlocks()

        // Note: shadingMode now in RenderingBlock UBO
    }

    if (s == m_basicShader.get()) {
        // Note: Post-processing uniforms now in RenderingBlock UBO
        // Standard shader uniforms using unified MaterialCore - RHI only
        // FEAT-0249: Material data now uses MaterialBlock UBO instead of individual uniforms
        // Material data is set via updateMaterialUniformsForObject() -> MaterialBlock UBO
    } else {
        // Use unified MaterialCore (eliminates dual storage) - RHI only
        const auto& mc = obj.materialCore;

        // Use MaterialManager for unified material handling
        m_materialManager.updateMaterialForObject(obj);
        // FEAT-0249: PBR material data now uses MaterialBlock UBO instead of individual uniforms
        // Material data is set via updateMaterialUniformsForObject() -> MaterialBlock UBO
        // Note: Texture flags now in MaterialBlock UBO (set by updateMaterialUniformsForObject)
        
        // Note: Post-processing uniforms now in RenderingBlock UBO
        
        int unit = 0;
        if (obj.baseColorTex) {
            // RHI-only texture binding
            m_rhi->bindTexture(obj.baseColorTex->rhiHandle(), unit);
            m_rhi->setUniformInt("baseColorTex", unit);
            unit++;
        }
        if (obj.normalTex) {
            // RHI-only texture binding
            m_rhi->bindTexture(obj.normalTex->rhiHandle(), unit);
            m_rhi->setUniformInt("normalTex", unit);
            unit++;
        }
        if (obj.mrTex) {
            // RHI-only texture binding
            m_rhi->bindTexture(obj.mrTex->rhiHandle(), unit);
            m_rhi->setUniformInt("mrTex", unit);
        }
    }

    // Lights (sets globalAmbient, lights[], numLights) - RHI only
    // Use currently bound program (bound via ensureObjectPipeline)
    GLint prog = 0; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (prog) lights.applyLights(static_cast<GLuint>(prog));

    // Bind dummy shadow map and identity lightSpaceMatrix to avoid undefined sampling - RHI only
    m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
    m_rhi->setUniformInt("shadowMap", 7);
    // FEAT-0249: lightSpaceMatrix now part of TransformBlock UBO

    if (s == m_basicShader.get()) {
        // Texturing or solid color - RHI only
        if (obj.texture) {
            obj.texture->bind(0);
            m_rhi->setUniformInt("cowTexture", 0);
            // Set useTexture in MaterialBlock UBO
            // Material texture flags now handled by MaterialManager
        } else {
            // Material texture flags now handled by MaterialManager
            // Set object color in RenderingBlock UBO
            // Update object color in rendering manager
            m_renderingManager.setObjectColor(obj.color);
        }
        // Material UBO now handled by MaterialManager
    }

    // TODO: Replace with RHI vertex buffer binding and pipeline

    // Optimized render mode handling - cache state
    static RenderMode lastRenderMode = static_cast<RenderMode>(-1);
    if (m_renderMode != lastRenderMode) {
        switch (m_renderMode) {
            case RenderMode::Points:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                break;
            case RenderMode::Wireframe:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
            default:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
        }
        lastRenderMode = m_renderMode;
    }
    
    // Enable blending for transmissive materials (approx; SSR pending)
    bool blendingEnabled = false;
    if (s == m_pbrShader.get()) {
        // Decide using unified MaterialCore transmission and alpha
        float transmission = obj.materialCore.transmission;
        if (transmission > 0.01f || obj.materialCore.baseColor.a < 0.999f) {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            blendingEnabled = true;
        }
    }

    // Optimized draw call
    if (m_rhi) {
        ensureObjectPipeline(const_cast<SceneObject&>(obj), true); // Always use PBR
        DrawDesc dd{};
        dd.pipeline = (s == m_pbrShader.get()) ? (obj.rhiPipelinePbr != INVALID_HANDLE ? obj.rhiPipelinePbr : m_pbrPipeline)
                                               : (obj.rhiPipelineBasic != INVALID_HANDLE ? obj.rhiPipelineBasic : m_basicPipeline);
        // Use indexed draw if either legacy GL EBO exists or RHI index buffer was created
        bool hasIndex = (obj.rhiEbo != INVALID_HANDLE) || (obj.rhiEbo != INVALID_HANDLE);
        if (hasIndex) {
            dd.indexBuffer = obj.rhiEbo;
            dd.indexCount = obj.objLoader.getIndexCount();
        } else {
            dd.vertexCount = obj.objLoader.getVertCount();
        }
        m_rhi->draw(dd);
    } else {
        if (obj.rhiEbo != INVALID_HANDLE) {
            glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
        }
    }

    if (blendingEnabled) {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // Count one draw call for this object
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
    // Full-screen quad vertices with UV coordinates
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,  // top left
        -1.0f, -1.0f,  0.0f, 0.0f,  // bottom left
         1.0f, -1.0f,  1.0f, 0.0f,  // bottom right
        
        -1.0f,  1.0f,  0.0f, 1.0f,  // top left
         1.0f, -1.0f,  1.0f, 0.0f,  // bottom right
         1.0f,  1.0f,  1.0f, 1.0f   // top right
    };
    
    glGenVertexArrays(1, &m_screenQuadVAO);
    glGenBuffers(1, &m_screenQuadVBO);
    
    glBindVertexArray(m_screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_screenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
    
    std::cout << "[RenderSystem] Screen quad initialized for raytracing\n";
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
    
    // GL fallback path
    glGenTextures(1, &m_raytraceTexture);
    glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
    
    // Create texture with RGB floating point format for HDR
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_raytraceWidth, m_raytraceHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::cout << "[RenderSystem] Raytracing texture initialized via GL (" << m_raytraceWidth << "x" << m_raytraceHeight << "): " << m_raytraceTexture << "\n";
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
    if (selObj >= 0 && selObj < (int)objs.size() && m_basicShader) {
        const auto& obj = objs[selObj];
        if (obj.rhiVboPositions != INVALID_HANDLE) {
            Shader* s = m_basicShader.get();
            s->use();
            // Post-processing uniforms for standard shader (selection overlay respects gamma/exposure) - RHI only
            // Update per-object data in UBOs
            // Update transform and rendering state using managers
            m_transformManager.updateTransforms(obj.modelMatrix, m_cameraManager.viewMatrix(), m_cameraManager.projectionMatrix());

            // Set object color for selection
            m_renderingManager.setObjectColor(glm::vec3(0.2f, 0.7f, 1.0f)); // cyan-ish for selection

            // Set material useTexture flag
            // Material UBO now handled by MaterialManager
            // Force bright ambient and no direct lights
            m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
            m_rhi->setUniformInt("shadowMap", 7);
            // Note: lightSpaceMatrix now in TransformBlock UBO
            // Note: numLights, globalAmbient now in LightingBlock UBO
            // TODO: Add selection overlay lighting mode to LightingManager
            // For now, selection will use current scene lighting

            // Note: Legacy Material uniform struct eliminated - using MaterialBlock UBO only

            // Draw as wireframe overlay with slight depth bias to reduce z-fighting
#ifndef __EMSCRIPTEN__
            GLint prevPolyMode[2];
            glGetIntegerv(GL_POLYGON_MODE, prevPolyMode);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.5f);
#endif
            // TODO: Replace with RHI vertex buffer binding and pipeline
            if (obj.rhiEbo != INVALID_HANDLE) {
                glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
            }
            glBindVertexArray(0);
            // Selection overlay adds an extra draw call
            m_stats.drawCalls += 1;
#ifndef __EMSCRIPTEN__
            glPolygonMode(GL_FRONT_AND_BACK, prevPolyMode[0]);
            glDisable(GL_POLYGON_OFFSET_LINE);
#endif
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
    
    // Render all objects with PBR shader
    if (!pbrShaderObjects.empty() && m_pbrShader) {
        m_pbrShader->use();
        setupCommonUniforms(m_pbrShader.get());
        // Apply lighting for this shader
        lights.applyLights(m_pbrShader->getID());
        for (const auto* obj : pbrShaderObjects) {
            renderObjectFast(*obj, lights, m_pbrShader.get());
        }
    }
    
    // Reset VAO binding once at the end
    glBindVertexArray(0);
}

void RenderSystem::setupCommonUniforms(Shader* shader)
{
    // Prefer RHI-bound current program; fall back to shader wrapper
    if (m_rhi) {
        // FEAT-0249: Use manager-based UBO system
        // Note: Transform UBO is now handled by TransformManager, updated per-object
        // This method is legacy and should be phased out in favor of manager calls

        // Lighting UBO now handled by LightingManager

        // Note: shadingMode, exposure, gamma, toneMappingMode now in RenderingBlock UBO
    }
    
    // Bind dummy shadow map - RHI only
    m_rhi->bindTexture(s_dummyShadowTexRhi, 7);
    m_rhi->setUniformInt("shadowMap", 7);
    // FEAT-0249: lightSpaceMatrix now part of TransformBlock UBO
    
    // Bind IBL textures if available - RHI only
    if (m_iblSystem) {
        m_iblSystem->bindIBLTextures();
        m_rhi->setUniformInt("irradianceMap", 3);
        m_rhi->setUniformInt("prefilterMap", 4);
        m_rhi->setUniformInt("brdfLUT", 5);
        // Note: iblIntensity now in RenderingBlock UBO
    }
}

void RenderSystem::renderObjectFast(const SceneObject& obj, const Light& lights, Shader* shader)
{
    // Fast object rendering with minimal per-object state changes
    if (m_rhi) {
        ensureObjectPipeline(const_cast<SceneObject&>(obj), true); // Always use PBR
        // FEAT-0249: model matrix now part of TransformBlock UBO, updated per-object
    }
    
    // Always use PBR shader (standard shader eliminated)
    // FEAT-0249: PBR material data now uses MaterialBlock UBO instead of individual uniforms
    // Material data is set via updateMaterialUniformsForObject() -> MaterialBlock UBO
    // Note: Texture flags now in MaterialBlock UBO (set by updateMaterialUniformsForObject)

    int unit = 0;
    if (obj.baseColorTex) {
        // RHI-only texture binding
        m_rhi->bindTexture(obj.baseColorTex->rhiHandle(), unit);
        m_rhi->setUniformInt("baseColorTex", unit);
        unit++;
    }
    if (obj.normalTex) {
        // RHI-only texture binding
        m_rhi->bindTexture(obj.normalTex->rhiHandle(), unit);
        m_rhi->setUniformInt("normalTex", unit);
        unit++;
    }
    if (obj.mrTex) {
        // RHI-only texture binding
        m_rhi->bindTexture(obj.mrTex->rhiHandle(), unit);
        m_rhi->setUniformInt("mrTex", unit);
    }
    // Simple blending for transmissive materials (approx; SSR pending)
    bool blendingEnabled = false;
    float transmission = obj.materialCore.transmission;
    if (transmission > 0.01f || obj.materialCore.baseColor.a < 0.999f) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        blendingEnabled = true;
    }

    // Draw call
    if (m_rhi) {
        DrawDesc dd{};
        dd.pipeline = obj.rhiPipelinePbr != INVALID_HANDLE ? obj.rhiPipelinePbr : m_pbrPipeline; // Always use PBR pipeline
        bool hasIndex = (obj.rhiEbo != INVALID_HANDLE) || (obj.rhiEbo != INVALID_HANDLE);
        if (hasIndex) {
            dd.indexBuffer = obj.rhiEbo;
            dd.indexCount = obj.objLoader.getIndexCount();
        } else {
            dd.vertexCount = obj.objLoader.getVertCount();
        }
        m_rhi->draw(dd);
    } else {
        // TODO: Replace with RHI vertex buffer binding and pipeline
        if (obj.rhiEbo != INVALID_HANDLE) {
            glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
        }
        glBindVertexArray(0);
    }
    
    if (blendingEnabled) {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    m_stats.drawCalls += 1;
}

void RenderSystem::cleanupRaytracing()
{
    if (m_screenQuadVAO) {
        glDeleteVertexArrays(1, &m_screenQuadVAO);
        m_screenQuadVAO = 0;
    }
    if (m_screenQuadVBO) {
        glDeleteBuffers(1, &m_screenQuadVBO);
        m_screenQuadVBO = 0;
    }
    // Cleanup RHI raytracing texture first
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        m_rhi->destroyTexture(m_raytraceTextureRhi);
        m_raytraceTextureRhi = INVALID_HANDLE;
    }

    // Cleanup legacy GL raytracing texture
    if (m_raytraceTexture) {
        glDeleteTextures(1, &m_raytraceTexture);
        m_raytraceTexture = 0;
    }
}

void RenderSystem::destroyTargets()
{
    // Destroy RHI render target first
    if (m_rhi && m_msaaRenderTarget != INVALID_HANDLE) {
        m_rhi->destroyRenderTarget(m_msaaRenderTarget);
        m_msaaRenderTarget = INVALID_HANDLE;
    }

    // Legacy GL cleanup (TODO: Remove after full migration)
    if (m_msaaColorRBO) { glDeleteRenderbuffers(1, &m_msaaColorRBO); m_msaaColorRBO = 0; }
    if (m_msaaDepthRBO) { glDeleteRenderbuffers(1, &m_msaaDepthRBO); m_msaaDepthRBO = 0; }
    if (m_msaaFBO) { glDeleteFramebuffers(1, &m_msaaFBO); m_msaaFBO = 0; }
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

    if (!m_screenQuadShader) {
        std::cerr << "[RenderSystem] Screen quad shader not loaded\n";
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

    // Upload raytraced image to texture
    if (m_rhi && m_raytraceTextureRhi != INVALID_HANDLE) {
        // Use RHI to update texture data
        m_rhi->updateBuffer(INVALID_HANDLE, raytraceBuffer.data(), raytraceBuffer.size() * sizeof(glm::vec3));
        // TODO: Implement texture update via RHI - for now fall back to GL
        glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_raytraceWidth, m_raytraceHeight,
                        GL_RGB, GL_FLOAT, raytraceBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_raytraceWidth, m_raytraceHeight,
                        GL_RGB, GL_FLOAT, raytraceBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Render full-screen quad with raytraced texture
    if (m_screenQuadVAO && m_screenQuadShader) {
        m_screenQuadShader->use();
        m_screenQuadShader->setInt("screenTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);

        glBindVertexArray(m_screenQuadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

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

void RenderSystem::passReadback(const PassContext& /*ctx*/)
{
    // TODO: perform readback via RHI when requested
}

void RenderSystem::passGBuffer(const PassContext& /*ctx*/, RenderTargetHandle /*gBufferRT*/)
{
    // TODO: move G-buffer rendering into render graph
}

void RenderSystem::passDeferredLighting(const PassContext& /*ctx*/, RenderTargetHandle /*outputRT*/,
                                       TextureHandle /*gBaseColor*/, TextureHandle /*gNormal*/,
                                       TextureHandle /*gPosition*/, TextureHandle /*gMaterial*/)
{
    // TODO: implement deferred lighting pass routing
}

void RenderSystem::passRayIntegrator(const PassContext& /*ctx*/, TextureHandle /*outputTexture*/, int /*sampleCount*/, int /*maxDepth*/)
{
    // TODO: integrate ray integrator pass
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








