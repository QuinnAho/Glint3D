#include "render_pass.h"
#include <glint3d/rhi_types.h>
#include "scene_manager.h"
#include "light.h"
#include "raytracer.h"
#include <algorithm>
#include <iostream>

using namespace glint3d;

// RenderGraph implementation
RenderGraph::RenderGraph(RHI* rhi) : m_rhi(rhi) {}

RenderGraph::~RenderGraph() {
    teardown();
}

void RenderGraph::addPass(std::unique_ptr<RenderPass> pass) {
    if (pass) {
        m_passes.push_back(std::move(pass));
        sortPasses(); // Re-sort whenever a new pass is added
    }
}

void RenderGraph::removePass(const std::string& name) {
    m_passes.erase(
        std::remove_if(m_passes.begin(), m_passes.end(),
            [&name](const std::unique_ptr<RenderPass>& pass) {
                return std::string(pass->getName()) == name;
            }),
        m_passes.end()
    );
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
    
    // Create context with graph textures
    PassContext ctx = baseContext;
    ctx.textures = m_textures;
    
    for (auto& pass : m_passes) {
        if (pass->isEnabled()) {
            pass->execute(ctx);
        }
    }
}

void RenderGraph::teardown() {
    for (auto& pass : m_passes) {
        pass->teardown(PassContext{});
    }
    
    // Clean up graph textures
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
        // Texture already exists, destroy old one
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
    // Simple dependency-based sorting
    // For now, we'll use a basic topological sort
    // In a full implementation, this would be more sophisticated
    
    std::stable_sort(m_passes.begin(), m_passes.end(),
        [](const std::unique_ptr<RenderPass>& a, const std::unique_ptr<RenderPass>& b) {
            // Simple heuristic: passes with no inputs come first
            return a->getInputs().empty() && !b->getInputs().empty();
        });
}

// GBufferPass implementation
bool GBufferPass::setup(const PassContext& ctx) {
    // TODO: Create G-buffer shader and pipeline
    // This is a simplified implementation
    std::cout << "[GBufferPass] Setup completed\n";
    return true;
}

void GBufferPass::execute(const PassContext& ctx) {
    if (!ctx.rhi || !ctx.scene) return;
    
    // TODO: Render geometry to G-buffer
    // - Bind G-buffer framebuffer
    // - Render all opaque objects
    // - Output: albedo, normal, material properties, depth
    std::cout << "[GBufferPass] Rendering geometry to G-buffer\n";
}

void GBufferPass::teardown(const PassContext& ctx) {
    if (m_pipeline != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyPipeline(m_pipeline);
        m_pipeline = INVALID_HANDLE;
    }
    if (m_shader != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyShader(m_shader);
        m_shader = INVALID_HANDLE;
    }
}

// LightingPass implementation
bool LightingPass::setup(const PassContext& ctx) {
    std::cout << "[LightingPass] Setup completed\n";
    return true;
}

void LightingPass::execute(const PassContext& ctx) {
    if (!ctx.rhi || !ctx.lights) return;
    
    // TODO: Perform deferred lighting
    // - Sample G-buffer textures
    // - Apply lighting calculations
    // - Output: lit color
    std::cout << "[LightingPass] Performing deferred lighting\n";
}

void LightingPass::teardown(const PassContext& ctx) {
    if (m_pipeline != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyPipeline(m_pipeline);
        m_pipeline = INVALID_HANDLE;
    }
    if (m_shader != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyShader(m_shader);
        m_shader = INVALID_HANDLE;
    }
}

// SSRRefractionPass implementation
bool SSRRefractionPass::setup(const PassContext& ctx) {
    std::cout << "[SSRRefractionPass] Setup completed\n";
    return true;
}

void SSRRefractionPass::execute(const PassContext& ctx) {
    if (!ctx.rhi || !m_hasTransparentObjects) return;
    
    // TODO: Implement screen-space refraction
    // - Ray-march through screen space
    // - Apply refraction based on IOR and normals
    // - Blend with background
    std::cout << "[SSRRefractionPass] Applying screen-space refraction\n";
}

void SSRRefractionPass::teardown(const PassContext& ctx) {
    if (m_pipeline != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyPipeline(m_pipeline);
        m_pipeline = INVALID_HANDLE;
    }
    if (m_shader != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyShader(m_shader);
        m_shader = INVALID_HANDLE;
    }
}

// PostPass implementation
bool PostPass::setup(const PassContext& ctx) {
    std::cout << "[PostPass] Setup completed\n";
    return true;
}

void PostPass::execute(const PassContext& ctx) {
    if (!ctx.rhi) return;
    
    // TODO: Apply post-processing
    // - Tone mapping (Linear/Reinhard/Filmic/ACES)
    // - Gamma correction
    // - Exposure adjustment
    std::cout << "[PostPass] Applying tone mapping and gamma correction\n";
}

void PostPass::teardown(const PassContext& ctx) {
    if (m_pipeline != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyPipeline(m_pipeline);
        m_pipeline = INVALID_HANDLE;
    }
    if (m_shader != INVALID_HANDLE && ctx.rhi) {
        ctx.rhi->destroyShader(m_shader);
        m_shader = INVALID_HANDLE;
    }
}

// ReadbackPass implementation
bool ReadbackPass::setup(const PassContext& ctx) {
    std::cout << "[ReadbackPass] Setup completed\n";
    return true;
}

void ReadbackPass::execute(const PassContext& ctx) {
    if (!ctx.rhi || !m_destination) return;
    
    // Get final color texture
    auto it = ctx.textures.find("final_color");
    if (it == ctx.textures.end()) {
        std::cerr << "[ReadbackPass] Final color texture not found\n";
        return;
    }
    
    ReadbackDesc desc;
    desc.sourceTexture = it->second;
    desc.format = TextureFormat::RGBA8;
    desc.x = m_x;
    desc.y = m_y;
    desc.width = m_width;
    desc.height = m_height;
    desc.destination = m_destination;
    desc.destinationSize = m_destinationSize;
    
    ctx.rhi->readback(desc);
    std::cout << "[ReadbackPass] Reading back final color\n";
}

void ReadbackPass::teardown(const PassContext& ctx) {
    // No resources to clean up
}

// IntegratorPass implementation
bool IntegratorPass::setup(const PassContext& ctx) {
    // TODO: Initialize raytracer if not already done
    std::cout << "[IntegratorPass] Setup completed\n";
    return true;
}

void IntegratorPass::execute(const PassContext& ctx) {
    if (!ctx.rhi || !ctx.scene) return;
    
    // TODO: Perform CPU ray tracing
    // - Trace rays through the scene
    // - Accumulate samples
    // - Output HDR result + auxiliary buffers
    std::cout << "[IntegratorPass] Ray tracing scene with " << m_sampleCount << " samples\n";
}

void IntegratorPass::teardown(const PassContext& ctx) {
    // Raytracer cleanup handled elsewhere
}

// DenoisePass implementation
bool DenoisePass::setup(const PassContext& ctx) {
    // TODO: Check for OIDN availability
    m_oidnAvailable = false; // Placeholder
    std::cout << "[DenoisePass] Setup completed (OIDN available: " << m_oidnAvailable << ")\n";
    return true;
}

void DenoisePass::execute(const PassContext& ctx) {
    if (!ctx.rhi || !m_oidnAvailable) return;
    
    // TODO: Apply AI denoising
    // - Load color, normal, albedo buffers
    // - Run OIDN filter
    // - Output denoised result
    std::cout << "[DenoisePass] Denoising ray traced result\n";
}

void DenoisePass::teardown(const PassContext& ctx) {
    // No resources to clean up
}

// TonemapPass implementation
bool TonemapPass::setup(const PassContext& ctx) {
    std::cout << "[TonemapPass] Setup completed\n";
    return true;
}

void TonemapPass::execute(const PassContext& ctx) {
    if (!ctx.rhi) return;
    
    // TODO: Apply HDR tone mapping
    // - Convert from linear HDR to display-ready LDR
    // - Apply exposure and gamma
    std::cout << "[TonemapPass] Tone mapping HDR to LDR\n";
}

void TonemapPass::teardown(const PassContext& ctx) {
    // No resources to clean up for this simple implementation
}