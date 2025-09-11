#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "rhi/rhi.h"

// Forward declarations
class SceneManager;
class Light;
class RenderGraph;
struct MaterialCore;

// Pass execution context containing all resources and state
struct PassContext {
    RHI* rhi = nullptr;
    const SceneManager* scene = nullptr;
    const Light* lights = nullptr;
    
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
    
    // Pass-specific data (can be extended by derived passes)
    std::unordered_map<std::string, void*> customData;
};

// Base class for all render passes
class RenderPass {
public:
    virtual ~RenderPass() = default;
    
    // Pass lifecycle
    virtual bool setup(const PassContext& ctx) = 0;
    virtual void execute(const PassContext& ctx) = 0;
    virtual void teardown(const PassContext& ctx) = 0;
    
    // Pass metadata
    virtual const char* getName() const = 0;
    virtual bool isEnabled() const { return m_enabled; }
    virtual void setEnabled(bool enabled) { m_enabled = enabled; }
    
    // Resource dependencies (for automatic ordering)
    virtual std::vector<std::string> getInputs() const { return {}; }
    virtual std::vector<std::string> getOutputs() const { return {}; }
    
protected:
    bool m_enabled = true;
};

// Minimal render graph for pass management
class RenderGraph {
public:
    RenderGraph(RHI* rhi);
    ~RenderGraph();
    
    // Pass management
    void addPass(std::unique_ptr<RenderPass> pass);
    void removePass(const std::string& name);
    RenderPass* getPass(const std::string& name);
    
    // Execution
    bool setup(const PassContext& baseContext);
    void execute(const PassContext& baseContext);
    void teardown();
    
    // Resource management
    TextureHandle createTexture(const std::string& name, const TextureDesc& desc);
    void destroyTexture(const std::string& name);
    TextureHandle getTexture(const std::string& name) const;
    
    // Graph state
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    RHI* m_rhi;
    std::vector<std::unique_ptr<RenderPass>> m_passes;
    std::unordered_map<std::string, TextureHandle> m_textures;
    bool m_enabled = true;
    bool m_isSetup = false;
    
    // Automatic pass ordering based on dependencies
    void sortPasses();
};

// Specific pass implementations

// G-Buffer pass for deferred rendering
class GBufferPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "GBufferPass"; }
    std::vector<std::string> getOutputs() const override { 
        return {"gbuffer_color", "gbuffer_normal", "gbuffer_material", "gbuffer_depth"}; 
    }
    
private:
    PipelineHandle m_pipeline = INVALID_HANDLE;
    ShaderHandle m_shader = INVALID_HANDLE;
};

// Lighting pass for deferred shading
class LightingPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "LightingPass"; }
    std::vector<std::string> getInputs() const override { 
        return {"gbuffer_color", "gbuffer_normal", "gbuffer_material", "gbuffer_depth"}; 
    }
    std::vector<std::string> getOutputs() const override { 
        return {"lighting_result"}; 
    }
    
private:
    PipelineHandle m_pipeline = INVALID_HANDLE;
    ShaderHandle m_shader = INVALID_HANDLE;
};

// Screen-space refraction pass for real-time transparency
class SSRRefractionPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "SSRRefractionPass"; }
    std::vector<std::string> getInputs() const override { 
        return {"lighting_result", "gbuffer_normal", "gbuffer_depth"}; 
    }
    std::vector<std::string> getOutputs() const override { 
        return {"refraction_result"}; 
    }
    
private:
    PipelineHandle m_pipeline = INVALID_HANDLE;
    ShaderHandle m_shader = INVALID_HANDLE;
    bool m_hasTransparentObjects = false;
};

// Post-processing pass (tone mapping, gamma correction)
class PostPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "PostPass"; }
    std::vector<std::string> getInputs() const override { 
        return {"refraction_result"}; 
    }
    std::vector<std::string> getOutputs() const override { 
        return {"final_color"}; 
    }
    
    // Post-processing settings
    void setExposure(float exposure) { m_exposure = exposure; }
    void setGamma(float gamma) { m_gamma = gamma; }
    void setToneMapMode(int mode) { m_toneMapMode = mode; }
    
private:
    PipelineHandle m_pipeline = INVALID_HANDLE;
    ShaderHandle m_shader = INVALID_HANDLE;
    float m_exposure = 0.0f;
    float m_gamma = 2.2f;
    int m_toneMapMode = 0; // 0=Linear, 1=Reinhard, 2=Filmic, 3=ACES
};

// CPU readback pass for headless rendering
class ReadbackPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "ReadbackPass"; }
    std::vector<std::string> getInputs() const override { 
        return {"final_color"}; 
    }
    
    // Readback configuration
    void setDestination(void* dest, size_t size) { m_destination = dest; m_destinationSize = size; }
    void setRegion(int x, int y, int width, int height) { 
        m_x = x; m_y = y; m_width = width; m_height = height; 
    }
    
private:
    void* m_destination = nullptr;
    size_t m_destinationSize = 0;
    int m_x = 0, m_y = 0, m_width = 0, m_height = 0;
};

// Ray tracing integrator pass
class IntegratorPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "IntegratorPass"; }
    std::vector<std::string> getOutputs() const override { 
        return {"raytrace_color", "raytrace_normal", "raytrace_albedo"}; 
    }
    
    // Ray tracing settings
    void setSampleCount(int samples) { m_sampleCount = samples; }
    void setMaxDepth(int depth) { m_maxDepth = depth; }
    
private:
    int m_sampleCount = 64;
    int m_maxDepth = 8;
    class Raytracer* m_raytracer = nullptr;
};

// AI denoising pass
class DenoisePass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "DenoisePass"; }
    std::vector<std::string> getInputs() const override { 
        return {"raytrace_color", "raytrace_normal", "raytrace_albedo"}; 
    }
    std::vector<std::string> getOutputs() const override { 
        return {"denoised_color"}; 
    }
    
private:
    bool m_oidnAvailable = false;
};

// HDR tone mapping pass for ray traced content
class TonemapPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override;
    void execute(const PassContext& ctx) override;
    void teardown(const PassContext& ctx) override;
    
    const char* getName() const override { return "TonemapPass"; }
    std::vector<std::string> getInputs() const override { 
        return {"denoised_color"}; 
    }
    std::vector<std::string> getOutputs() const override { 
        return {"final_color"}; 
    }
    
    // Tone mapping settings
    void setExposure(float exposure) { m_exposure = exposure; }
    void setGamma(float gamma) { m_gamma = gamma; }
    
private:
    float m_exposure = 0.0f;
    float m_gamma = 2.2f;
};