#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

// Forward declarations
struct RhiInit;
struct DrawDesc;
struct ReadbackDesc;
struct TextureDesc;
struct BufferDesc;
struct ShaderDesc;
struct PipelineDesc;

// Resource handle types
using TextureHandle = uint32_t;
using BufferHandle = uint32_t;
using ShaderHandle = uint32_t;
using PipelineHandle = uint32_t;

constexpr uint32_t INVALID_HANDLE = 0;

// RHI (Render Hardware Interface) - Thin abstraction for GPU operations
class RHI {
public:
    virtual ~RHI() = default;
    
    // Lifecycle
    virtual bool init(const RhiInit& desc) = 0;
    virtual void shutdown() = 0;
    
    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    // Drawing
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void readback(const ReadbackDesc& desc) = 0;
    
    // Resource management
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
    
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;
    virtual void destroyPipeline(PipelineHandle handle) = 0;
    
    // State management
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void clear(const glm::vec4& color, float depth = 1.0f, int stencil = 0) = 0;
    virtual void bindPipeline(PipelineHandle pipeline) = 0;
    virtual void bindTexture(TextureHandle texture, uint32_t slot) = 0;
    
    // Query capabilities
    virtual bool supportsCompute() const = 0;
    virtual bool supportsGeometryShaders() const = 0;
    virtual bool supportsTessellation() const = 0;
    virtual int getMaxTextureUnits() const = 0;
    virtual int getMaxSamples() const = 0;
    
    // Backend identification
    enum class Backend { OpenGL, WebGL2, Vulkan, WebGPU };
    virtual Backend getBackend() const = 0;
    virtual const char* getBackendName() const = 0;
};

// Factory function
std::unique_ptr<RHI> createRHI(RHI::Backend backend);