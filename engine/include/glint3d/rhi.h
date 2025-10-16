#pragma once

#include <glint3d/rhi_types.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace glint3d {

// forward declarations for WebGPU-shaped frontend
class CommandEncoder;
class RenderPassEncoder;
class Queue;

/**
 * @brief Render Hardware Interface (RHI) - Thin abstraction for GPU operations
 * 
 * the RHI provides a clean abstraction layer over graphics APIs (OpenGL, WebGL2,
 * future Vulkan/WebGPU) with minimal performance overhead. It follows RAII patterns
 * for resource management and supports both desktop and web platforms.
 * 
 * Design principles:
 * - minimal overhead: Thin wrapper with <5% performance cost
 * - type safety: Opaque handles prevent resource mix-ups
 * - cross-platform: Consistent API across OpenGL/WebGL2/Vulkan
 * - future-proof: Designed for modern graphics API patterns
 */
class RHI {
public:
    virtual ~RHI() = default;

    // Backend identification
    enum class Backend { 
        OpenGL,   // desktop OpenGL 3.3+
        WebGL2,   // web WebGL 2.0
        Vulkan,   // future: Desktop/mobile Vulkan
        WebGPU,   // future: Next-gen web graphics
        Null      // testing/headless backend
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

    // WebGPU-shaped resource grouping
    virtual BindGroupLayoutHandle createBindGroupLayout(const BindGroupLayoutDesc& desc) = 0;
    virtual BindGroupHandle createBindGroup(const BindGroupDesc& desc) = 0;
    virtual void destroyBindGroupLayout(BindGroupLayoutHandle handle) = 0;
    virtual void destroyBindGroup(BindGroupHandle handle) = 0;
    
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
     * @brief Update texture data with new pixel data
     * @param texture Texture handle to update
     * @param data Pointer to pixel data
     * @param width Width of the update region
     * @param height Height of the update region
     * @param format Pixel format of the source data
     * @param x X offset in texture (default 0)
     * @param y Y offset in texture (default 0)
     * @param mipLevel Mip level to update (default 0)
     */
    virtual void updateTexture(TextureHandle texture, const void* data,
                              int width, int height, TextureFormat format,
                              int x = 0, int y = 0, int mipLevel = 0) = 0;

    /**
     * @brief Generate mipmaps for a texture
     * @param texture Texture handle (must have been created with mipLevels > 1)
     *
     * Automatically generates mipmap chain from the base level (mip 0).
     * Supports both 2D textures and cubemaps.
     */
    virtual void generateMipmaps(TextureHandle texture) = 0;

    /**
     * @brief Bind a render target for subsequent draw commands
     * @param renderTarget Render target handle, or INVALID_HANDLE for default framebuffer
     */
    virtual void bindRenderTarget(RenderTargetHandle renderTarget) = 0;

    /**
     * @brief Resolve a multisampled render target to a non-multisampled texture
     * @param srcRenderTarget Source multisampled render target
     * @param dstTexture Destination texture (must be non-multisampled)
     * @param srcRect Source rectangle (x, y, width, height), or nullptr for full resolve
     * @param dstRect Destination rectangle, or nullptr to match srcRect
     */
    virtual void resolveRenderTarget(RenderTargetHandle srcRenderTarget, TextureHandle dstTexture,
                                   const int* srcRect = nullptr, const int* dstRect = nullptr) = 0;

    /**
     * @brief Resolve multisampled render target to default framebuffer
     * @param srcRenderTarget Source multisampled render target
     * @param srcRect Source rectangle, or nullptr for full resolve
     * @param dstRect Destination rectangle in default framebuffer
     */
    virtual void resolveToDefaultFramebuffer(RenderTargetHandle srcRenderTarget,
                                           const int* srcRect = nullptr, const int* dstRect = nullptr) = 0;

    // Legacy uniform helpers (transitional - creates dynamic UBOs behind the scenes)
    /**
     * @brief Set a mat4 uniform via dynamic uniform buffer (legacy bridge)
     * @param name Uniform name in currently bound shader
     * @param value Matrix value to set
     */
    virtual void setUniformMat4(const char* name, const glm::mat4& value) = 0;

    /**
     * @brief Set a vec3 uniform via dynamic uniform buffer (legacy bridge)
     * @param name Uniform name in currently bound shader
     * @param value Vector value to set
     */
    virtual void setUniformVec3(const char* name, const glm::vec3& value) = 0;

    /**
     * @brief Set a vec4 uniform via dynamic uniform buffer (legacy bridge)
     * @param name Uniform name in currently bound shader
     * @param value Vector value to set
     */
    virtual void setUniformVec4(const char* name, const glm::vec4& value) = 0;

    /**
     * @brief Set a float uniform via dynamic uniform buffer (legacy bridge)
     * @param name Uniform name in currently bound shader
     * @param value Float value to set
     */
    virtual void setUniformFloat(const char* name, float value) = 0;

    /**
     * @brief Set an int uniform via dynamic uniform buffer (legacy bridge)
     * @param name Uniform name in currently bound shader
     * @param value Integer value to set
     */
    virtual void setUniformInt(const char* name, int value) = 0;

    /**
     * @brief Set a bool uniform via dynamic uniform buffer (legacy bridge)
     * @param name Uniform name in currently bound shader
     * @param value Boolean value to set
     */
    virtual void setUniformBool(const char* name, bool value) = 0;

    // Uniform Buffer Ring Allocator (FEAT-0249)
    // ===========================================

    /**
     * @brief Allocate uniform buffer space from the ring allocator
     * @param desc Allocation descriptor with size and alignment requirements
     * @return Allocation result with buffer handle and mapped pointer
     */
    virtual UniformAllocation allocateUniforms(const UniformAllocationDesc& desc) = 0;

    /**
     * @brief Free a uniform buffer allocation (allows reuse in ring)
     * @param allocation Previously allocated uniform buffer space
     */
    virtual void freeUniforms(const UniformAllocation& allocation) = 0;

    /**
     * @brief Get shader reflection data for validation and offset calculation
     * @param shader Shader handle to reflect
     * @return Reflection data with uniform block layouts and variable offsets
     */
    virtual ShaderReflection getShaderReflection(ShaderHandle shader) = 0;

    /**
     * @brief Set uniform data in allocated buffer with reflection validation
     * @param allocation Target uniform buffer allocation
     * @param shader Shader handle for reflection validation
     * @param blockName Uniform block name (e.g., "MaterialBlock")
     * @param varName Variable name within block (e.g., "diffuseColor")
     * @param data Pointer to uniform data
     * @param dataSize Size of data in bytes
     * @return true if successfully set, false if validation failed
     */
    virtual bool setUniformInBlock(const UniformAllocation& allocation, ShaderHandle shader,
                                 const char* blockName, const char* varName,
                                 const void* data, size_t dataSize) = 0;

    /**
     * @brief Set multiple uniform variables in a block efficiently
     * @param allocation Target uniform buffer allocation
     * @param shader Shader handle for reflection validation
     * @param blockName Uniform block name
     * @param uniforms Array of name-value pairs to set
     * @param count Number of uniforms to set
     * @return Number of uniforms successfully set
     */
    struct UniformNameValue {
        const char* name;
        const void* data;
        size_t dataSize;
        UniformType type;
    };
    virtual int setUniformsInBlock(const UniformAllocation& allocation, ShaderHandle shader,
                                 const char* blockName, const UniformNameValue* uniforms, int count) = 0;

    /**
     * @brief Bind an allocated uniform range to a named uniform block in the given shader
     * @param allocation Uniform allocation previously obtained from allocateUniforms()
     * @param shader Shader handle providing reflection to locate the block binding
     * @param blockName Uniform block name to bind (e.g., "MaterialBlock")
     * @return true on success, false if validation fails
     */
    virtual bool bindUniformBlock(const UniformAllocation& allocation, ShaderHandle shader,
                                  const char* blockName) = 0;

    // Encoders/Queue (WebGPU-shaped)
    virtual std::unique_ptr<CommandEncoder> createCommandEncoder(const char* debugName = nullptr) = 0;
    virtual Queue& getQueue() = 0;
    
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

    // Utility functions for common operations
    /**
     * @brief Get or create a screen quad buffer for full-screen passes
     * @return Buffer handle containing screen-aligned quad vertices with UVs
     *
     * Returns a cached vertex buffer containing 6 vertices forming 2 triangles
     * that cover the entire screen in NDC coordinates [-1,1] with UV mapping [0,1].
     * Vertex format: position (vec2), UV (vec2).
     */
    virtual BufferHandle getScreenQuadBuffer() = 0;
};

// Command recording API (WebGPU-shaped)
class RenderPassEncoder {
public:
    virtual ~RenderPassEncoder() = default;
    virtual void setPipeline(PipelineHandle pipeline) = 0;
    virtual void setBindGroup(uint32_t index, BindGroupHandle group) = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void end() = 0;
};

class CommandEncoder {
public:
    virtual ~CommandEncoder() = default;
    virtual RenderPassEncoder* beginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void resourceBarrier(TextureHandle, ResourceState /*before*/, ResourceState /*after*/) { /* optional */ }
    virtual void finish() = 0; // finalize recorded commands
};

class Queue {
public:
    virtual ~Queue() = default;
    virtual void submit(CommandEncoder& encoder) = 0;
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
