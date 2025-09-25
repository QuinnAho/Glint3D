#include "render_pass.h"
#include "render_system.h"
#include "scene_manager.h"
#include "light.h"

#include <glint3d/rhi_types.h>

#include <algorithm>
#include <iostream>
#include <chrono>

using namespace glint3d;

// RenderPass timing implementation
void RenderPass::executeWithTiming(const PassContext& ctx) {
    if (!isEnabled()) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Execute the actual pass
    execute(ctx);

    // Calculate elapsed time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    float timeMs = duration.count() / 1000.0f;

    // Record timing if enabled and timing collection is available
    if (ctx.enableTiming && ctx.passTimings) {
        PassTiming timing{};
        timing.passName = getName();
        timing.timeMs = timeMs;
        timing.enabled = isEnabled();
        ctx.passTimings->push_back(timing);
    }
}

RenderGraph::RenderGraph(RHI* rhi) : m_rhi(rhi) {}

RenderGraph::~RenderGraph() {
    teardown();
}

void RenderGraph::addPass(std::unique_ptr<RenderPass> pass) {
    if (pass) {
        m_passes.push_back(std::move(pass));
        sortPasses();
    }
}

void RenderGraph::removePass(const std::string& name) {
    m_passes.erase(
        std::remove_if(m_passes.begin(), m_passes.end(),
            [&name](const std::unique_ptr<RenderPass>& pass) {
                return std::string(pass->getName()) == name;
            }),
        m_passes.end());
}

RenderPass* RenderGraph::getPass(const std::string& name) {
    auto it = std::find_if(m_passes.begin(), m_passes.end(),
        [&name](const std::unique_ptr<RenderPass>& pass) {
            return std::string(pass->getName()) == name;
        });
    return (it != m_passes.end()) ? it->get() : nullptr;
}

bool RenderGraph::setup(const PassContext& baseContext) {
    if (m_isSetup) {
        teardown();
    }

    bool success = true;
    for (auto& pass : m_passes) {
        if (pass->isEnabled()) {
            if (!pass->setup(baseContext)) {
                std::cerr << "[RenderGraph] Failed to setup pass: " << pass->getName() << std::endl;
                success = false;
            }
        }
    }

    m_isSetup = success;
    return success;
}

void RenderGraph::execute(const PassContext& baseContext) {
    if (!m_enabled || !m_isSetup) {
        return;
    }

    PassContext ctx = baseContext;
    ctx.textures = m_textures;

    for (auto& pass : m_passes) {
        if (pass->isEnabled()) {
            pass->executeWithTiming(ctx);
        }
    }
}

void RenderGraph::teardown() {
    for (auto& pass : m_passes) {
        pass->teardown(PassContext{});
    }

    for (auto& [name, handle] : m_textures) {
        if (handle != INVALID_HANDLE) {
            m_rhi->destroyTexture(handle);
        }
    }
    m_textures.clear();
    m_isSetup = false;
}

TextureHandle RenderGraph::createTexture(const std::string& name, const TextureDesc& desc) {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        m_rhi->destroyTexture(it->second);
    }

    TextureHandle handle = m_rhi->createTexture(desc);
    m_textures[name] = handle;
    return handle;
}

void RenderGraph::destroyTexture(const std::string& name) {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        m_rhi->destroyTexture(it->second);
        m_textures.erase(it);
    }
}

TextureHandle RenderGraph::getTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : INVALID_HANDLE;
}

void RenderGraph::sortPasses() {
    std::stable_sort(m_passes.begin(), m_passes.end(),
        [](const std::unique_ptr<RenderPass>& a, const std::unique_ptr<RenderPass>& b) {
            return a->getInputs().empty() && !b->getInputs().empty();
        });
}

namespace {
    bool ensureRenderer(const PassContext& ctx, const char* passName) {
        if (!ctx.renderer) {
            std::cerr << "[" << passName << "] Missing RenderSystem reference" << std::endl;
            return false;
        }
        if (!ctx.rhi) {
            std::cerr << "[" << passName << "] Missing RHI reference" << std::endl;
            return false;
        }
        return true;
    }
}

bool FrameSetupPass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void FrameSetupPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    ctx.renderer->passFrameSetup(ctx);
}

void FrameSetupPass::teardown(const PassContext&) {}

bool RasterPass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void RasterPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableRaster) return;
    ctx.renderer->passRaster(ctx);
}

void RasterPass::teardown(const PassContext&) {}

bool RaytracePass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void RaytracePass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableRay) return;
    ctx.renderer->passRaytrace(ctx, m_sampleCount, m_maxDepth);
}


void RaytracePass::teardown(const PassContext&) {}

void RaytracePass::setSampleCount(int samples) {
    m_sampleCount = std::max(1, samples);
}

void RaytracePass::setMaxDepth(int depth) {
    m_maxDepth = std::max(1, depth);
}


bool RayDenoisePass::setup(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return false;
    if (!ctx.rhi) return false;

    // Create output texture for denoised result
    int width = ctx.viewportWidth > 0 ? ctx.viewportWidth : 1024;
    int height = ctx.viewportHeight > 0 ? ctx.viewportHeight : 768;

    TextureDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA32F; // High precision for HDR denoising
    desc.debugName = "RayDenoise_Output";
    m_outputTex = ctx.rhi->createTexture(desc);

    return m_outputTex != INVALID_HANDLE;
}

void RayDenoisePass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableRay || m_outputTex == INVALID_HANDLE) return;

    // Get ray traced input texture
    auto rayTraceResult = ctx.textures.find("rayTraceResult");
    if (rayTraceResult == ctx.textures.end()) {
        std::cerr << "[RayDenoisePass] Missing ray trace result texture" << std::endl;
        return;
    }

    // Apply denoising to the ray traced result
    ctx.renderer->passRayDenoise(ctx, rayTraceResult->second, m_outputTex);

    // Add denoised result to context for other passes
    PassContext* mutableCtx = const_cast<PassContext*>(&ctx);
    mutableCtx->textures["denoisedResult"] = m_outputTex;
}

void RayDenoisePass::teardown(const PassContext& ctx) {
    if (!ctx.rhi) return;

    if (m_outputTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_outputTex);
        m_outputTex = INVALID_HANDLE;
    }
}

bool OverlayPass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void OverlayPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableOverlays) return;
    ctx.renderer->passOverlays(ctx);
}

void OverlayPass::teardown(const PassContext&) {}

bool ResolvePass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void ResolvePass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    ctx.renderer->passResolve(ctx);
}

void ResolvePass::teardown(const PassContext&) {}

bool PresentPass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void PresentPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.finalizeFrame) return;
    ctx.renderer->passPresent(ctx);
}

void PresentPass::teardown(const PassContext&) {}

bool ReadbackPass::setup(const PassContext& ctx) {
    return ensureRenderer(ctx, getName());
}

void ReadbackPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.readback) return;

    // Determine source texture - either specified or auto-select
    TextureHandle sourceTexture = ctx.outputTexture;

    if (!m_sourceTexture.empty()) {
        // Use specified source texture
        auto it = ctx.textures.find(m_sourceTexture);
        if (it != ctx.textures.end()) {
            sourceTexture = it->second;
        }
    } else {
        // Auto-select based on enabled render mode
        if (ctx.enableRay) {
            // For ray mode, prefer denoised result, fallback to raw ray trace
            auto denoised = ctx.textures.find("denoisedResult");
            if (denoised != ctx.textures.end()) {
                sourceTexture = denoised->second;
            } else {
                auto rayTrace = ctx.textures.find("rayTraceResult");
                if (rayTrace != ctx.textures.end()) {
                    sourceTexture = rayTrace->second;
                }
            }
        } else if (ctx.enableRaster) {
            // For raster mode, use lit color
            auto litColor = ctx.textures.find("litColor");
            if (litColor != ctx.textures.end()) {
                sourceTexture = litColor->second;
            }
        }
    }

    // Create modified context with the correct source texture
    PassContext modifiedCtx = ctx;
    modifiedCtx.outputTexture = sourceTexture;

    ctx.renderer->passReadback(modifiedCtx);
}

void ReadbackPass::teardown(const PassContext&) {}

// G-Buffer Pass Implementation
bool GBufferPass::setup(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return false;
    if (!ctx.rhi) return false;

    // Create G-buffer render targets
    int width = ctx.viewportWidth > 0 ? ctx.viewportWidth : 1024;
    int height = ctx.viewportHeight > 0 ? ctx.viewportHeight : 768;

    // Create textures for G-buffer
    TextureDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA8;
    desc.debugName = "GBuffer_BaseColor";
    m_baseColorTex = ctx.rhi->createTexture(desc);

    desc.debugName = "GBuffer_Normal";
    m_normalTex = ctx.rhi->createTexture(desc);

    desc.format = TextureFormat::RGBA32F; // Need higher precision for world positions
    desc.debugName = "GBuffer_Position";
    m_positionTex = ctx.rhi->createTexture(desc);

    desc.format = TextureFormat::RGBA8;
    desc.debugName = "GBuffer_Material";
    m_materialTex = ctx.rhi->createTexture(desc);

    // Create depth texture
    desc.format = TextureFormat::Depth24Stencil8;
    desc.debugName = "GBuffer_Depth";
    m_depthTex = ctx.rhi->createTexture(desc);

    // Create render target with attachments
    RenderTargetDesc rtDesc{};
    rtDesc.width = width;
    rtDesc.height = height;

    // Set up color attachments
    RenderTargetAttachment baseColorAttachment{};
    baseColorAttachment.type = AttachmentType::Color0;
    baseColorAttachment.texture = m_baseColorTex;
    rtDesc.colorAttachments.push_back(baseColorAttachment);

    RenderTargetAttachment normalAttachment{};
    normalAttachment.type = AttachmentType::Color1;
    normalAttachment.texture = m_normalTex;
    rtDesc.colorAttachments.push_back(normalAttachment);

    RenderTargetAttachment positionAttachment{};
    positionAttachment.type = AttachmentType::Color2;
    positionAttachment.texture = m_positionTex;
    rtDesc.colorAttachments.push_back(positionAttachment);

    RenderTargetAttachment materialAttachment{};
    materialAttachment.type = AttachmentType::Color3;
    materialAttachment.texture = m_materialTex;
    rtDesc.colorAttachments.push_back(materialAttachment);

    // Set up depth attachment
    rtDesc.depthAttachment.type = AttachmentType::Depth;
    rtDesc.depthAttachment.texture = m_depthTex;

    rtDesc.debugName = "GBufferRT";
    m_gBufferRT = ctx.rhi->createRenderTarget(rtDesc);

    return m_gBufferRT != INVALID_HANDLE;
}

void GBufferPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableRaster || m_gBufferRT == INVALID_HANDLE) return;

    // Add G-buffer textures to context for use by other passes
    PassContext* mutableCtx = const_cast<PassContext*>(&ctx);
    mutableCtx->textures["gBaseColor"] = m_baseColorTex;
    mutableCtx->textures["gNormal"] = m_normalTex;
    mutableCtx->textures["gPosition"] = m_positionTex;
    mutableCtx->textures["gMaterial"] = m_materialTex;
    mutableCtx->textures["gDepth"] = m_depthTex;

    ctx.renderer->passGBuffer(ctx, m_gBufferRT);
}

void GBufferPass::teardown(const PassContext& ctx) {
    if (!ctx.rhi) return;

    if (m_gBufferRT != INVALID_HANDLE) {
        ctx.rhi->destroyRenderTarget(m_gBufferRT);
        m_gBufferRT = INVALID_HANDLE;
    }
    if (m_baseColorTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_baseColorTex);
        m_baseColorTex = INVALID_HANDLE;
    }
    if (m_normalTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_normalTex);
        m_normalTex = INVALID_HANDLE;
    }
    if (m_positionTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_positionTex);
        m_positionTex = INVALID_HANDLE;
    }
    if (m_materialTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_materialTex);
        m_materialTex = INVALID_HANDLE;
    }
    if (m_depthTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_depthTex);
        m_depthTex = INVALID_HANDLE;
    }
}

// Deferred Lighting Pass Implementation
bool DeferredLightingPass::setup(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return false;
    if (!ctx.rhi) return false;

    // Create output texture for lit result
    int width = ctx.viewportWidth > 0 ? ctx.viewportWidth : 1024;
    int height = ctx.viewportHeight > 0 ? ctx.viewportHeight : 768;

    TextureDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA8;
    desc.debugName = "DeferredLighting_Output";
    m_outputTex = ctx.rhi->createTexture(desc);

    // Create render target
    RenderTargetDesc rtDesc{};
    rtDesc.width = width;
    rtDesc.height = height;

    RenderTargetAttachment colorAttachment{};
    colorAttachment.type = AttachmentType::Color0;
    colorAttachment.texture = m_outputTex;
    rtDesc.colorAttachments.push_back(colorAttachment);

    rtDesc.debugName = "DeferredLightingRT";
    m_outputRT = ctx.rhi->createRenderTarget(rtDesc);

    return m_outputRT != INVALID_HANDLE;
}

void DeferredLightingPass::execute(const PassContext& ctx) {
    std::cout << "[DeferredLightingPass] execute called, enableRaster=" << ctx.enableRaster << ", outputRT=" << m_outputRT << std::endl;

    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableRaster) {
        std::cout << "[DeferredLightingPass] Skipping - raster disabled" << std::endl;
        return;
    }
    if (m_outputRT == INVALID_HANDLE) {
        std::cout << "[DeferredLightingPass] ERROR: Invalid output render target" << std::endl;
        return;
    }

    // Get G-buffer textures from context
    auto gBaseColor = ctx.textures.find("gBaseColor");
    auto gNormal = ctx.textures.find("gNormal");
    auto gPosition = ctx.textures.find("gPosition");
    auto gMaterial = ctx.textures.find("gMaterial");

    std::cout << "[DeferredLightingPass] Available textures in context: " << ctx.textures.size() << std::endl;

    if (gBaseColor == ctx.textures.end() || gNormal == ctx.textures.end() ||
        gPosition == ctx.textures.end() || gMaterial == ctx.textures.end()) {
        std::cerr << "[DeferredLightingPass] Missing G-buffer textures" << std::endl;
        return;
    }

    std::cout << "[DeferredLightingPass] Calling passDeferredLighting" << std::endl;
    ctx.renderer->passDeferredLighting(ctx, m_outputRT,
        gBaseColor->second, gNormal->second, gPosition->second, gMaterial->second);
    std::cout << "[DeferredLightingPass] passDeferredLighting completed" << std::endl;

    // Add lit color texture to context for use by other passes
    PassContext* mutableCtx = const_cast<PassContext*>(&ctx);
    mutableCtx->textures["litColor"] = m_outputTex;
}

void DeferredLightingPass::teardown(const PassContext& ctx) {
    if (!ctx.rhi) return;

    if (m_outputRT != INVALID_HANDLE) {
        ctx.rhi->destroyRenderTarget(m_outputRT);
        m_outputRT = INVALID_HANDLE;
    }
    if (m_outputTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_outputTex);
        m_outputTex = INVALID_HANDLE;
    }
}

// Ray Integrator Pass Implementation
bool RayIntegratorPass::setup(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return false;
    if (!ctx.rhi) return false;

    // Create output texture for raytraced result
    int width = ctx.viewportWidth > 0 ? ctx.viewportWidth : 1024;
    int height = ctx.viewportHeight > 0 ? ctx.viewportHeight : 768;

    TextureDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.format = TextureFormat::RGBA32F; // High precision for HDR raytracing
    desc.debugName = "RayIntegrator_Output";
    m_outputTex = ctx.rhi->createTexture(desc);

    return m_outputTex != INVALID_HANDLE;
}

void RayIntegratorPass::execute(const PassContext& ctx) {
    if (!ensureRenderer(ctx, getName())) return;
    if (!ctx.enableRay || m_outputTex == INVALID_HANDLE) return;

    // Call the updated raytracer pass that produces a texture
    ctx.renderer->passRayIntegrator(ctx, m_outputTex, m_sampleCount, m_maxDepth);

    // Add ray traced result texture to context for use by other passes
    PassContext* mutableCtx = const_cast<PassContext*>(&ctx);
    mutableCtx->textures["rayTraceResult"] = m_outputTex;
}

void RayIntegratorPass::teardown(const PassContext& ctx) {
    if (!ctx.rhi) return;

    if (m_outputTex != INVALID_HANDLE) {
        ctx.rhi->destroyTexture(m_outputTex);
        m_outputTex = INVALID_HANDLE;
    }
}


