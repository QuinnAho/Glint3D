// machine summary block
// {"file":"engine/include/rhi/rhi_gl.h","purpose":"opengl 3.3+ backend implementation of the rhi abstraction layer","exports":["RhiGL","SimpleRenderPassEncoderGL","SimpleCommandEncoderGL","SimpleQueueGL"],"depends_on":["glint3d::RHI","glint3d::rhi_types","gl_platform"],"notes":["implements webgpu-shaped api using opengl immediate mode","provides uniform buffer ring allocator for persistent mapping","converts rhi types to opengl enums and manages gl resource handles"]}

/**
 * @file rhi_gl.h
 * @brief opengl backend for the render hardware interface (rhi).
 *
 * RhiGL translates the platform-agnostic rhi api into opengl 3.3+ calls. it manages
 * gl resources (textures, buffers, shaders, pipelines, framebuffers) via handle-based
 * storage and provides simple webgpu-shaped command encoder/queue wrappers for immediate
 * mode execution. includes a uniform buffer ring allocator for efficient ubo management.
 */

#pragma once

#include <glint3d/rhi.h>
#include <glint3d/rhi_types.h>
#include "../gl_platform.h"
#include <unordered_map>
#include <vector>

using namespace glint3d;

// opengl implementation of rhi
class RhiGL : public RHI {
public:
    RhiGL();
    ~RhiGL() override;

    // lifecycle
    bool init(const RhiInit& desc) override;
    void shutdown() override;

    // frame management
    void beginFrame() override;
    void endFrame() override;

    // draw and readback
    void draw(const DrawDesc& desc) override;
    void readback(const ReadbackDesc& desc) override;

    // resource creation
    TextureHandle createTexture(const TextureDesc& desc) override;
    BufferHandle createBuffer(const BufferDesc& desc) override;
    ShaderHandle createShader(const ShaderDesc& desc) override;
    PipelineHandle createPipeline(const PipelineDesc& desc) override;
    RenderTargetHandle createRenderTarget(const RenderTargetDesc& desc) override;
    BindGroupLayoutHandle createBindGroupLayout(const BindGroupLayoutDesc& desc) override;
    BindGroupHandle createBindGroup(const BindGroupDesc& desc) override;

    // resource destruction
    void destroyTexture(TextureHandle handle) override;
    void destroyBuffer(BufferHandle handle) override;
    void destroyShader(ShaderHandle handle) override;
    void destroyPipeline(PipelineHandle handle) override;
    void destroyRenderTarget(RenderTargetHandle handle) override;
    void destroyBindGroupLayout(BindGroupLayoutHandle handle) override;
    void destroyBindGroup(BindGroupHandle handle) override;

    // state management
    void setViewport(int x, int y, int width, int height) override;
    void clear(const glm::vec4& color, float depth, int stencil) override;
    void bindPipeline(PipelineHandle pipeline) override;
    void bindTexture(TextureHandle texture, uint32_t slot) override;
    void bindUniformBuffer(BufferHandle buffer, uint32_t slot) override;

    // buffer and texture updates
    void updateBuffer(BufferHandle buffer, const void* data, size_t size, size_t offset = 0) override;
    void updateTexture(TextureHandle texture, const void* data,
                      int width, int height, TextureFormat format,
                      int x = 0, int y = 0, int mipLevel = 0) override;
    void generateMipmaps(TextureHandle texture) override;

    // render target operations
    void bindRenderTarget(RenderTargetHandle renderTarget) override;
    void resolveRenderTarget(RenderTargetHandle srcRenderTarget, TextureHandle dstTexture,
                           const int* srcRect = nullptr, const int* dstRect = nullptr) override;
    void resolveToDefaultFramebuffer(RenderTargetHandle srcRenderTarget,
                                   const int* srcRect = nullptr, const int* dstRect = nullptr) override;

    // legacy uniform helpers (transitional bridge) REMOVE
    void setUniformMat4(const char* name, const glm::mat4& value) override;
    void setUniformVec3(const char* name, const glm::vec3& value) override;
    void setUniformVec4(const char* name, const glm::vec4& value) override;
    void setUniformFloat(const char* name, float value) override;
    void setUniformInt(const char* name, int value) override;
    void setUniformBool(const char* name, bool value) override;

    // uniform buffer ring allocator
    UniformAllocation allocateUniforms(const UniformAllocationDesc& desc) override;
    void freeUniforms(const UniformAllocation& allocation) override;
    ShaderReflection getShaderReflection(ShaderHandle shader) override;
    bool setUniformInBlock(const UniformAllocation& allocation, ShaderHandle shader,
                         const char* blockName, const char* varName,
                         const void* data, size_t dataSize) override;
    int setUniformsInBlock(const UniformAllocation& allocation, ShaderHandle shader,
                         const char* blockName, const UniformNameValue* uniforms, int count) override;
    bool bindUniformBlock(const UniformAllocation& allocation, ShaderHandle shader,
                          const char* blockName) override;

    // command encoder and queue
    std::unique_ptr<CommandEncoder> createCommandEncoder(const char* debugName = nullptr) override;
    Queue& getQueue() override;

    // capability queries
    bool supportsCompute() const override;
    bool supportsGeometryShaders() const override;
    bool supportsTessellation() const override;
    int getMaxTextureUnits() const override;
    int getMaxSamples() const override;
    
    Backend getBackend() const override { return Backend::OpenGL; }
    const char* getBackendName() const override { return "OpenGL"; }
    std::string getDebugInfo() const override;

    // utility functions
    BufferHandle getScreenQuadBuffer() override;

    // opengl-specific accessors
    GLuint getGLTexture(TextureHandle handle) const;
    GLuint getGLBuffer(BufferHandle handle) const;
    GLuint getGLShader(ShaderHandle handle) const;

private:
    // webgpu-shaped adapters for immediate mode

    class SimpleRenderPassEncoderGL : public RenderPassEncoder {
    public:
        SimpleRenderPassEncoderGL(RhiGL& rhi, const RenderPassDesc& desc);
        ~SimpleRenderPassEncoderGL() override = default;
        void setPipeline(PipelineHandle pipeline) override;
        void setBindGroup(uint32_t index, BindGroupHandle group) override;
        void setViewport(int x, int y, int width, int height) override;
        void draw(const DrawDesc& desc) override;
        void end() override;
    private:
        RhiGL& m_rhi;
        RenderPassDesc m_desc;
        bool m_active = false;
    };

    class SimpleCommandEncoderGL : public CommandEncoder {
    public:
        SimpleCommandEncoderGL(RhiGL& rhi, const char* name);
        ~SimpleCommandEncoderGL() override = default;
        RenderPassEncoder* beginRenderPass(const RenderPassDesc& desc) override;
        void finish() override;
    private:
        RhiGL& m_rhi;
        std::unique_ptr<SimpleRenderPassEncoderGL> m_activePass;
        std::string m_name;
    };

    class SimpleQueueGL : public Queue {
    public:
        explicit SimpleQueueGL(RhiGL& rhi) : m_rhi(rhi) {}
        void submit(CommandEncoder& encoder) override;
    private:
        RhiGL& m_rhi;
    };

    // internal resource structures

    struct GLTexture {
        GLuint id = 0;
        TextureDesc desc;
    };

    struct GLBuffer {
        GLuint id = 0;
        BufferDesc desc;
    };
    
    struct GLShader {
        GLuint program = 0;
        ShaderDesc desc;
    };
    
    struct GLPipeline {
        GLuint vao = 0;
        ShaderHandle shader = 0;
        PipelineDesc desc;
    };
    
    struct GLRenderTarget {
        GLuint fbo = 0;
        RenderTargetDesc desc;
    };

    // resource storage

    std::unordered_map<TextureHandle, GLTexture> m_textures;
    std::unordered_map<BufferHandle, GLBuffer> m_buffers;
    std::unordered_map<ShaderHandle, GLShader> m_shaders;
    std::unordered_map<PipelineHandle, GLPipeline> m_pipelines;
    std::unordered_map<RenderTargetHandle, GLRenderTarget> m_renderTargets;
    struct GLBindGroupLayout { BindGroupLayoutDesc desc; };
    struct GLBindGroup { BindGroupDesc desc; };
    std::unordered_map<BindGroupLayoutHandle, GLBindGroupLayout> m_bindGroupLayouts;
    std::unordered_map<BindGroupHandle, GLBindGroup> m_bindGroups;

    // handle generation

    uint32_t m_nextTextureHandle = 1;
    uint32_t m_nextBufferHandle = 1;
    uint32_t m_nextShaderHandle = 1;
    uint32_t m_nextPipelineHandle = 1;
    uint32_t m_nextRenderTargetHandle = 1;
    uint32_t m_nextBindGroupLayoutHandle = 1;
    uint32_t m_nextBindGroupHandle = 1;

    // current state tracking

    PipelineHandle m_currentPipeline = INVALID_HANDLE;
    RenderTargetHandle m_currentRenderTarget = INVALID_HANDLE;

    // cached utility resources

    BufferHandle m_screenQuadBuffer = INVALID_HANDLE;

    // opengl capability flags

    bool m_supportsCompute = false;
    bool m_supportsGeometry = false;
    bool m_supportsTessellation = false;
    int m_maxTextureUnits = 16;
    int m_maxSamples = 1;

    // queue instance
    SimpleQueueGL m_queue{*this};

    // uniform buffer ring allocator
    struct UniformRingBuffer {
        GLuint buffer = 0;
        size_t size = 0;
        size_t offset = 0;
        void* mappedPtr = nullptr;
        bool persistent = false;
    };

    struct GLUniformAllocation {
        UniformAllocationHandle handle;
        BufferHandle bufferHandle;
        uint32_t offset;
        uint32_t size;
        void* mappedPtr;
        bool inUse;
    };

    static constexpr size_t UBO_RING_SIZE = 1024 * 1024; // 1MB ring buffer
    static constexpr size_t UBO_ALIGNMENT = 256; // UBO alignment on most GPUs

    UniformRingBuffer m_uniformRing;
    std::unordered_map<UniformAllocationHandle, GLUniformAllocation> m_uniformAllocations;
    uint32_t m_nextUniformHandle = 1;
    std::unordered_map<ShaderHandle, ShaderReflection> m_shaderReflections;

    // helper methods

    // ubo helpers
    bool initializeUniformRing();
    void shutdownUniformRing();
    uint32_t alignOffset(uint32_t offset, uint32_t alignment) const;
    bool createShaderReflection(ShaderHandle shader, const ShaderDesc& desc);

    // type conversion helpers
    GLenum textureFormatToGL(TextureFormat format) const;
    void getTextureFormatAndType(TextureFormat format, GLenum& outFormat, GLenum& outType) const;
    GLenum textureTypeToGL(TextureType type) const;
    GLenum bufferTypeToGL(BufferType type) const;
    GLenum bufferUsageToGL(BufferUsage usage) const;
    GLenum primitiveTopologyToGL(PrimitiveTopology topology) const;
    GLenum polygonModeToGL(PolygonMode mode) const;
    GLenum blendFactorToGL(BlendFactor factor) const;

    // setup and initialization helpers
    bool compileShader(GLuint& program, const ShaderDesc& desc);
    void queryCapabilities();
    void setupVertexArray(GLuint vao, const PipelineDesc& desc);
    GLenum attachmentTypeToGL(AttachmentType type) const;
    bool setupRenderTarget(GLuint fbo, const RenderTargetDesc& desc);
    void applyBindGroup(uint32_t index, BindGroupHandle group);
};
