#pragma once

#include "rhi.h"
#include "rhi-types.h"
#include "../gl_platform.h"
#include <unordered_map>
#include <vector>

// OpenGL implementation of RHI
class RhiGL : public RHI {
public:
    RhiGL();
    ~RhiGL() override;
    
    // RHI interface implementation
    bool init(const RhiInit& desc) override;
    void shutdown() override;
    
    void beginFrame() override;
    void endFrame() override;
    
    void draw(const DrawDesc& desc) override;
    void readback(const ReadbackDesc& desc) override;
    
    TextureHandle createTexture(const TextureDesc& desc) override;
    BufferHandle createBuffer(const BufferDesc& desc) override;
    ShaderHandle createShader(const ShaderDesc& desc) override;
    PipelineHandle createPipeline(const PipelineDesc& desc) override;
    
    void destroyTexture(TextureHandle handle) override;
    void destroyBuffer(BufferHandle handle) override;
    void destroyShader(ShaderHandle handle) override;
    void destroyPipeline(PipelineHandle handle) override;
    
    void setViewport(int x, int y, int width, int height) override;
    void clear(const glm::vec4& color, float depth, int stencil) override;
    void bindPipeline(PipelineHandle pipeline) override;
    void bindTexture(TextureHandle texture, uint32_t slot) override;
    
    bool supportsCompute() const override;
    bool supportsGeometryShaders() const override;
    bool supportsTessellation() const override;
    int getMaxTextureUnits() const override;
    int getMaxSamples() const override;
    
    Backend getBackend() const override { return Backend::OpenGL; }
    const char* getBackendName() const override { return "OpenGL"; }
    
    // OpenGL-specific methods
    GLuint getGLTexture(TextureHandle handle) const;
    GLuint getGLBuffer(BufferHandle handle) const;
    GLuint getGLShader(ShaderHandle handle) const;

private:
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
    
    // Resource storage
    std::unordered_map<TextureHandle, GLTexture> m_textures;
    std::unordered_map<BufferHandle, GLBuffer> m_buffers;
    std::unordered_map<ShaderHandle, GLShader> m_shaders;
    std::unordered_map<PipelineHandle, GLPipeline> m_pipelines;
    
    // Handle generation
    uint32_t m_nextTextureHandle = 1;
    uint32_t m_nextBufferHandle = 1;
    uint32_t m_nextShaderHandle = 1;
    uint32_t m_nextPipelineHandle = 1;
    
    // Current state
    PipelineHandle m_currentPipeline = INVALID_HANDLE;
    
    // OpenGL capabilities
    bool m_supportsCompute = false;
    bool m_supportsGeometry = false;
    bool m_supportsTessellation = false;
    int m_maxTextureUnits = 16;
    int m_maxSamples = 1;
    
    // Helper methods
    GLenum textureFormatToGL(TextureFormat format) const;
    GLenum textureTypeToGL(TextureType type) const;
    GLenum bufferTypeToGL(BufferType type) const;
    GLenum bufferUsageToGL(BufferUsage usage) const;
    GLenum primitiveTopologyToGL(PrimitiveTopology topology) const;
    
    bool compileShader(GLuint& program, const ShaderDesc& desc);
    void queryCapabilities();
    void setupVertexArray(GLuint vao, const PipelineDesc& desc);
};