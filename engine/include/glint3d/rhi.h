#pragma once

#include <glint3d/rhi_types.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace glint3d {

/**
 * @brief Render Hardware Interface (RHI) - Thin abstraction for GPU operations
 * 
 * The RHI provides a clean abstraction layer over graphics APIs (OpenGL, WebGL2,
 * future Vulkan/WebGPU) with minimal performance overhead. It follows RAII patterns
 * for resource management and supports both desktop and web platforms.
 * 
 * Design principles:
 * - Minimal overhead: Thin wrapper with <5% performance cost
 * - Type safety: Opaque handles prevent resource mix-ups
 * - Cross-platform: Consistent API across OpenGL/WebGL2/Vulkan
 * - Future-proof: Designed for modern graphics API patterns
 */
class RHI {
public:
    virtual ~RHI() = default;

    // Backend identification
    enum class Backend { 
        OpenGL,   // Desktop OpenGL 3.3+
        WebGL2,   // Web WebGL 2.0
        Vulkan,   // Future: Desktop/mobile Vulkan
        WebGPU,   // Future: Next-gen web graphics
        Null      // Testing/headless backend
    };
    
    /**
     * @brief Initialize the RHI backend with given parameters
     * @param desc Initialization parameters (window size, debug mode, etc.)
     * @return true if initialization successful, false otherwise
     */
    virtual bool init(const RhiInit& desc) = 0;
    
    /**
     * @brief Shutdown the RHI and cleanup resources
     * Must be called before destructor to ensure proper cleanup
     */
    virtual void shutdown() = 0;
    
    // Frame lifecycle management
    /**
     * @brief Begin a new frame - call once per frame before any drawing
     */
    virtual void beginFrame() = 0;
    
    /**
     * @brief End the current frame and present to display
     */
    virtual void endFrame() = 0;
    
    // Core drawing operations
    /**
     * @brief Execute a draw command with specified parameters
     * @param desc Draw command descriptor with pipeline, buffers, counts
     */
    virtual void draw(const DrawDesc& desc) = 0;
    
    /**
     * @brief Read GPU texture data back to CPU memory
     * @param desc Readback descriptor with source texture and destination
     */
    virtual void readback(const ReadbackDesc& desc) = 0;
    
    // Resource creation and management
    /**
     * @brief Create a GPU texture with specified properties
     * @param desc Texture descriptor (format, size, mips, etc.)
     * @return Handle to created texture, or INVALID_HANDLE on failure
     */
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    
    /**
     * @brief Create a GPU buffer (vertex, index, uniform, etc.)
     * @param desc Buffer descriptor (type, size, usage pattern)
     * @return Handle to created buffer, or INVALID_HANDLE on failure
     */
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    
    /**
     * @brief Create a shader program from source code
     * @param desc Shader descriptor with source for each stage
     * @return Handle to created shader, or INVALID_HANDLE on failure
     */
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    
    /**
     * @brief Create a graphics pipeline (vertex layout + shader + state)
     * @param desc Pipeline descriptor with vertex attributes and shader
     * @return Handle to created pipeline, or INVALID_HANDLE on failure
     */
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
    
    /**
     * @brief Create a render target (framebuffer) with attachments
     * @param desc Render target descriptor with color/depth attachments
     * @return Handle to created render target, or INVALID_HANDLE on failure
     */
    virtual RenderTargetHandle createRenderTarget(const RenderTargetDesc& desc) = 0;
    
    // Resource destruction - RAII style cleanup
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;
    virtual void destroyPipeline(PipelineHandle handle) = 0;
    virtual void destroyRenderTarget(RenderTargetHandle handle) = 0;
    
    // GPU state management
    /**
     * @brief Set the rendering viewport rectangle
     * @param x,y Top-left corner in pixels
     * @param width,height Size in pixels
     */
    virtual void setViewport(int x, int y, int width, int height) = 0;
    
    /**
     * @brief Clear render targets with specified values
     * @param color Clear color (RGBA)
     * @param depth Clear depth value (0.0 to 1.0)
     * @param stencil Clear stencil value
     */
    virtual void clear(const glm::vec4& color, float depth = 1.0f, int stencil = 0) = 0;
    
    /**
     * @brief Bind a graphics pipeline for subsequent draw calls
     * @param pipeline Pipeline handle from createPipeline()
     */
    virtual void bindPipeline(PipelineHandle pipeline) = 0;
    
    /**
     * @brief Bind a texture to a shader texture unit
     * @param texture Texture handle from createTexture()
     * @param slot Texture unit slot (0-based)
     */
    virtual void bindTexture(TextureHandle texture, uint32_t slot) = 0;

    /**
     * @brief Bind a uniform buffer to a binding slot
     * @param buffer Buffer handle created with BufferType::Uniform
     * @param slot Binding slot (0-based), matches shader uniform block binding
     */
    virtual void bindUniformBuffer(BufferHandle buffer, uint32_t slot) = 0;

    /**
     * @brief Update a buffer's contents
     * @param buffer Buffer handle
     * @param data Pointer to source data
     * @param size Number of bytes to write
     * @param offset Byte offset in buffer (default 0)
     */
    virtual void updateBuffer(BufferHandle buffer, const void* data, size_t size, size_t offset = 0) = 0;
    
    /**
     * @brief Bind a render target for subsequent draw commands
     * @param renderTarget Render target handle, or INVALID_HANDLE for default framebuffer
     */
    virtual void bindRenderTarget(RenderTargetHandle renderTarget) = 0;
    
    // Capability queries for feature detection
    /**
     * @brief Check if compute shaders are supported
     * @return true if compute shaders available
     */
    virtual bool supportsCompute() const = 0;
    
    /**
     * @brief Check if geometry shaders are supported
     * @return true if geometry shaders available (false on WebGL2)
     */
    virtual bool supportsGeometryShaders() const = 0;
    
    /**
     * @brief Check if tessellation shaders are supported
     * @return true if tessellation available
     */
    virtual bool supportsTessellation() const = 0;
    
    /**
     * @brief Get maximum number of texture units
     * @return Number of texture units (typically 16-32)
     */
    virtual int getMaxTextureUnits() const = 0;
    
    /**
     * @brief Get maximum MSAA samples supported
     * @return Maximum sample count for multisampling
     */
    virtual int getMaxSamples() const = 0;
    
    // Backend information
    /**
     * @brief Get the active backend type
     * @return Backend enum value (OpenGL, WebGL2, etc.)
     */
    virtual Backend getBackend() const = 0;
    
    /**
     * @brief Get human-readable backend name
     * @return Backend name string for logging/UI
     */
    virtual const char* getBackendName() const = 0;
    
    /**
     * @brief Get backend-specific debug information
     * @return Debug string with driver version, extensions, etc.
     */
    virtual std::string getDebugInfo() const = 0;
};

/**
 * @brief Create an RHI instance for the specified backend
 * @param backend Backend type to create (OpenGL, WebGL2, etc.)
 * @return Unique pointer to RHI instance, or nullptr on failure
 */
std::unique_ptr<RHI> createRHI(RHI::Backend backend);

/**
 * @brief Auto-detect and create the best available RHI backend
 * @return Unique pointer to RHI instance, or nullptr if no backend available
 * 
 * Selection order:
 * 1. OpenGL on desktop platforms
 * 2. WebGL2 on web platforms  
 * 3. Null backend for testing
 */
std::unique_ptr<RHI> createDefaultRHI();

} // namespace glint3d
