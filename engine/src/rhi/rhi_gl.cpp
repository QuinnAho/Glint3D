#include "rhi/rhi_gl.h"
#include "path_utils.h"
#include <iostream>
#include <sstream>

using namespace glint3d;

RhiGL::RhiGL() = default;
RhiGL::~RhiGL() = default;

bool RhiGL::init(const RhiInit& desc) {
    // Query OpenGL capabilities
    queryCapabilities();
    
    // Set initial viewport
    setViewport(0, 0, desc.windowWidth, desc.windowHeight);
    
    // Enable depth testing by default
    glEnable(GL_DEPTH_TEST);
    
    // Enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Configure sRGB if requested
    if (desc.enableSRGB) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
    
    // Enable debug output if available and requested
    if (desc.enableDebug) {
#ifdef GL_DEBUG_OUTPUT
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        }
#endif
    }

    // Initialize uniform buffer ring allocator
    if (!initializeUniformRing()) {
        return false;
    }

    return true;
}

void RhiGL::shutdown() {
    // Clean up all resources
    for (auto& [handle, pipeline] : m_pipelines) {
        if (pipeline.vao != 0) {
            glDeleteVertexArrays(1, &pipeline.vao);
        }
    }
    m_pipelines.clear();
    
    for (auto& [handle, shader] : m_shaders) {
        if (shader.program != 0) {
            glDeleteProgram(shader.program);
        }
    }
    m_shaders.clear();
    
    for (auto& [handle, buffer] : m_buffers) {
        if (buffer.id != 0) {
            glDeleteBuffers(1, &buffer.id);
        }
    }
    m_buffers.clear();
    
    for (auto& [handle, texture] : m_textures) {
        if (texture.id != 0) {
            glDeleteTextures(1, &texture.id);
        }
    }
    m_textures.clear();

    // Clean up utility resources
    if (m_screenQuadBuffer != INVALID_HANDLE) {
        destroyBuffer(m_screenQuadBuffer);
        m_screenQuadBuffer = INVALID_HANDLE;
    }

    // Shutdown uniform buffer ring allocator
    shutdownUniformRing();
}

void RhiGL::beginFrame() {
    // Reset state tracking
    m_currentPipeline = INVALID_HANDLE;
}

void RhiGL::endFrame() {
    // Nothing specific needed for OpenGL
}

void RhiGL::draw(const DrawDesc& desc) {
    // If a valid pipeline is provided, bind and use its VAO; otherwise, use currently bound VAO
    GLuint vaoToUse = 0;
    GLenum topology = GL_TRIANGLES;
    if (desc.pipeline != INVALID_HANDLE) {
        if (desc.pipeline != m_currentPipeline) {
            bindPipeline(desc.pipeline);
        }
        auto pipelineIt = m_pipelines.find(desc.pipeline);
        if (pipelineIt == m_pipelines.end()) {
            std::cerr << "[RhiGL] Invalid pipeline handle in draw call\n";
            return;
        }
        const auto& pipeline = pipelineIt->second;
        vaoToUse = pipeline.vao;
        topology = primitiveTopologyToGL(pipeline.desc.topology);
    } else {
        // Fallback: use GL_TRIANGLES and current VAO
        topology = GL_TRIANGLES;
    }

    if (vaoToUse != 0) glBindVertexArray(vaoToUse);

    // Optional VBO/IBO binding if provided
    if (desc.vertexBuffer != INVALID_HANDLE) {
        auto bufferIt = m_buffers.find(desc.vertexBuffer);
        if (bufferIt != m_buffers.end()) {
            glBindBuffer(GL_ARRAY_BUFFER, bufferIt->second.id);
        }
    }

    if (desc.indexBuffer != INVALID_HANDLE && desc.indexCount > 0) {
        auto indexIt = m_buffers.find(desc.indexBuffer);
        if (indexIt != m_buffers.end()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexIt->second.id);
            glDrawElementsInstanced(topology, desc.indexCount, GL_UNSIGNED_INT,
                                    reinterpret_cast<void*>(static_cast<uintptr_t>(desc.firstIndex * sizeof(uint32_t))),
                                    desc.instanceCount);
        }
    } else if (desc.vertexCount > 0) {
        glDrawArraysInstanced(topology, desc.firstVertex, desc.vertexCount, desc.instanceCount);
    }

    if (vaoToUse != 0) glBindVertexArray(0);
}

void RhiGL::readback(const ReadbackDesc& desc) {
    auto textureIt = m_textures.find(desc.sourceTexture);
    if (textureIt == m_textures.end()) {
        std::cerr << "[RhiGL] Invalid texture handle in readback\n";
        return;
    }
    
    const auto& texture = textureIt->second;
    
    // Create temporary framebuffer for readback
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.id, 0);
    
    if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        GLenum format = textureFormatToGL(desc.format);
        GLenum type = GL_UNSIGNED_BYTE; // Simplified for now
        
        glReadPixels(desc.x, desc.y, desc.width, desc.height, format, type, desc.destination);
    } else {
        std::cerr << "[RhiGL] Framebuffer incomplete for readback\n";
    }
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}

TextureHandle RhiGL::createTexture(const TextureDesc& desc) {
    GLTexture glTexture;
    glTexture.desc = desc;
    
    glGenTextures(1, &glTexture.id);
    
    GLenum target = textureTypeToGL(desc.type);
    glBindTexture(target, glTexture.id);
    
    GLenum internalFormat = textureFormatToGL(desc.format);
    GLenum format, type;
    getTextureFormatAndType(desc.format, format, type);
    
    // Allocate texture storage
    switch (desc.type) {
        case TextureType::Texture2D:
            glTexImage2D(target, 0, internalFormat, desc.width, desc.height, 0, format, type, desc.initialData);
            break;
        case TextureType::TextureCube:
            // Allocate all 6 faces
            for (int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, 
                           desc.width, desc.height, 0, format, type, nullptr);
            }
            break;
        default:
            std::cerr << "[RhiGL] Unsupported texture type\n";
            break;
    }
    
    // Set default filtering
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Generate mipmaps if requested
    if (desc.generateMips) {
        glGenerateMipmap(target);
    }
    
    glBindTexture(target, 0);
    
    TextureHandle handle = m_nextTextureHandle++;
    m_textures[handle] = glTexture;
    return handle;
}

BufferHandle RhiGL::createBuffer(const BufferDesc& desc) {
    GLBuffer glBuffer;
    glBuffer.desc = desc;
    
    glGenBuffers(1, &glBuffer.id);
    
    GLenum target = bufferTypeToGL(desc.type);
    GLenum usage = bufferUsageToGL(desc.usage);
    
    glBindBuffer(target, glBuffer.id);
    glBufferData(target, desc.size, desc.initialData, usage);
    glBindBuffer(target, 0);
    
    BufferHandle handle = m_nextBufferHandle++;
    m_buffers[handle] = glBuffer;
    return handle;
}

ShaderHandle RhiGL::createShader(const ShaderDesc& desc) {
    GLShader glShader;
    glShader.desc = desc;
    
    if (!compileShader(glShader.program, desc)) {
        return INVALID_HANDLE;
    }

    ShaderHandle handle = m_nextShaderHandle++;
    m_shaders[handle] = glShader;

    // Create shader reflection data for UBO validation
    createShaderReflection(handle, desc);

    return handle;
}

PipelineHandle RhiGL::createPipeline(const PipelineDesc& desc) {
    GLPipeline glPipeline;
    glPipeline.desc = desc;
    glPipeline.shader = desc.shader;
    // Create VAO only if vertex attributes are provided
    if (!desc.vertexAttributes.empty()) {
        glGenVertexArrays(1, &glPipeline.vao);
        setupVertexArray(glPipeline.vao, desc);
    } else {
        glPipeline.vao = 0;
    }
    
    PipelineHandle handle = m_nextPipelineHandle++;
    m_pipelines[handle] = glPipeline;
    return handle;
}

RenderTargetHandle RhiGL::createRenderTarget(const RenderTargetDesc& desc) {
    GLRenderTarget glRenderTarget;
    glRenderTarget.desc = desc;
    
    glGenFramebuffers(1, &glRenderTarget.fbo);
    if (!setupRenderTarget(glRenderTarget.fbo, desc)) {
        glDeleteFramebuffers(1, &glRenderTarget.fbo);
        return INVALID_HANDLE;
    }
    
    RenderTargetHandle handle = m_nextRenderTargetHandle++;
    m_renderTargets[handle] = glRenderTarget;
    return handle;
}

BindGroupLayoutHandle RhiGL::createBindGroupLayout(const BindGroupLayoutDesc& desc) {
    BindGroupLayoutHandle h = m_nextBindGroupLayoutHandle++;
    m_bindGroupLayouts[h] = GLBindGroupLayout{desc};
    return h;
}

BindGroupHandle RhiGL::createBindGroup(const BindGroupDesc& desc) {
    BindGroupHandle h = m_nextBindGroupHandle++;
    m_bindGroups[h] = GLBindGroup{desc};
    return h;
}

void RhiGL::destroyTexture(TextureHandle handle) {
    auto it = m_textures.find(handle);
    if (it != m_textures.end()) {
        glDeleteTextures(1, &it->second.id);
        m_textures.erase(it);
    }
}

void RhiGL::destroyBuffer(BufferHandle handle) {
    auto it = m_buffers.find(handle);
    if (it != m_buffers.end()) {
        glDeleteBuffers(1, &it->second.id);
        m_buffers.erase(it);
    }
}

void RhiGL::destroyShader(ShaderHandle handle) {
    auto it = m_shaders.find(handle);
    if (it != m_shaders.end()) {
        glDeleteProgram(it->second.program);
        m_shaders.erase(it);
    }
}

void RhiGL::destroyPipeline(PipelineHandle handle) {
    auto it = m_pipelines.find(handle);
    if (it != m_pipelines.end()) {
        glDeleteVertexArrays(1, &it->second.vao);
        m_pipelines.erase(it);
    }
}

void RhiGL::destroyRenderTarget(RenderTargetHandle handle) {
    auto it = m_renderTargets.find(handle);
    if (it != m_renderTargets.end()) {
        glDeleteFramebuffers(1, &it->second.fbo);
        m_renderTargets.erase(it);
    }
}

void RhiGL::destroyBindGroupLayout(BindGroupLayoutHandle handle) {
    m_bindGroupLayouts.erase(handle);
}

void RhiGL::destroyBindGroup(BindGroupHandle handle) {
    m_bindGroups.erase(handle);
}

void RhiGL::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void RhiGL::clear(const glm::vec4& color, float depth, int stencil) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearDepth(depth);
    glClearStencil(stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RhiGL::bindPipeline(PipelineHandle pipeline) {
    auto it = m_pipelines.find(pipeline);
    if (it == m_pipelines.end()) {
        std::cerr << "[RhiGL] Invalid pipeline handle\n";
        return;
    }
    
    const auto& glPipeline = it->second;
    
    // Bind shader program
    auto shaderIt = m_shaders.find(glPipeline.shader);
    if (shaderIt != m_shaders.end()) {
        glUseProgram(shaderIt->second.program);
    }
    
    // Pipeline state will be applied during draw
    m_currentPipeline = pipeline;
}

void RhiGL::bindTexture(TextureHandle texture, uint32_t slot) {
    auto it = m_textures.find(texture);
    if (it == m_textures.end()) {
        std::cerr << "[RhiGL] Invalid texture handle\n";
        return;
    }
    
    glActiveTexture(GL_TEXTURE0 + slot);
    GLenum target = textureTypeToGL(it->second.desc.type);
    glBindTexture(target, it->second.id);
}

void RhiGL::bindUniformBuffer(BufferHandle buffer, uint32_t slot) {
    auto it = m_buffers.find(buffer);
    if (it == m_buffers.end()) {
        std::cerr << "[RhiGL] Invalid buffer handle\n";
        return;
    }
    
    // Bind uniform buffer to binding point
    glBindBufferBase(GL_UNIFORM_BUFFER, slot, it->second.id);
}

void RhiGL::updateBuffer(BufferHandle buffer, const void* data, size_t size, size_t offset) {
    auto it = m_buffers.find(buffer);
    if (it == m_buffers.end()) {
        std::cerr << "[RhiGL] Invalid buffer handle\n";
        return;
    }

    GLenum target = bufferTypeToGL(it->second.desc.type);
    glBindBuffer(target, it->second.id);
    glBufferSubData(target, offset, size, data);
    glBindBuffer(target, 0);
}

void RhiGL::updateTexture(TextureHandle texture, const void* data,
                         int width, int height, TextureFormat format,
                         int x, int y, int mipLevel) {
    auto it = m_textures.find(texture);
    if (it == m_textures.end()) {
        std::cerr << "[RhiGL] Invalid texture handle\n";
        return;
    }

    // Convert format to OpenGL format and type
    GLenum glFormat, glType;
    switch (format) {
        case TextureFormat::RGBA8:
            glFormat = GL_RGBA;
            glType = GL_UNSIGNED_BYTE;
            break;
        case TextureFormat::RGB8:
            glFormat = GL_RGB;
            glType = GL_UNSIGNED_BYTE;
            break;
        case TextureFormat::RGBA32F:
            glFormat = GL_RGBA;
            glType = GL_FLOAT;
            break;
        case TextureFormat::RGB32F:
            glFormat = GL_RGB;
            glType = GL_FLOAT;
            break;
        default:
            std::cerr << "[RhiGL] Unsupported texture format for updateTexture\n";
            return;
    }

    glBindTexture(GL_TEXTURE_2D, it->second.id);
    glTexSubImage2D(GL_TEXTURE_2D, mipLevel, x, y, width, height, glFormat, glType, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RhiGL::bindRenderTarget(RenderTargetHandle renderTarget) {
    if (renderTarget == INVALID_HANDLE) {
        // Bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_currentRenderTarget = INVALID_HANDLE;
    } else {
        auto it = m_renderTargets.find(renderTarget);
        if (it == m_renderTargets.end()) {
            std::cerr << "[RhiGL] Invalid render target handle\n";
            return;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
        m_currentRenderTarget = renderTarget;
    }
}

void RhiGL::resolveRenderTarget(RenderTargetHandle srcRenderTarget, TextureHandle dstTexture,
                              const int* srcRect, const int* dstRect) {
    auto srcIt = m_renderTargets.find(srcRenderTarget);
    auto dstIt = m_textures.find(dstTexture);

    if (srcIt == m_renderTargets.end()) {
        std::cerr << "[RhiGL] Invalid source render target handle for resolve\n";
        return;
    }
    if (dstIt == m_textures.end()) {
        std::cerr << "[RhiGL] Invalid destination texture handle for resolve\n";
        return;
    }

    const auto& srcRT = srcIt->second;
    const auto& dstTex = dstIt->second;

    // Create temporary framebuffer for destination texture
    GLuint tempFBO;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTex.id, 0);

    // Bind source as read framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcRT.fbo);

    // Determine rectangles
    int srcX = 0, srcY = 0, srcW = srcRT.desc.width, srcH = srcRT.desc.height;
    int dstX = 0, dstY = 0, dstW = srcW, dstH = srcH;

    if (srcRect) {
        srcX = srcRect[0]; srcY = srcRect[1]; srcW = srcRect[2]; srcH = srcRect[3];
    }
    if (dstRect) {
        dstX = dstRect[0]; dstY = dstRect[1]; dstW = dstRect[2]; dstH = dstRect[3];
    }

    // Perform resolve
    glBlitFramebuffer(srcX, srcY, srcX + srcW, srcY + srcH,
                     dstX, dstY, dstX + dstW, dstY + dstH,
                     GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Cleanup
    glDeleteFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void RhiGL::resolveToDefaultFramebuffer(RenderTargetHandle srcRenderTarget,
                                      const int* srcRect, const int* dstRect) {
    auto srcIt = m_renderTargets.find(srcRenderTarget);
    if (srcIt == m_renderTargets.end()) {
        std::cerr << "[RhiGL] Invalid source render target handle for default resolve\n";
        return;
    }

    const auto& srcRT = srcIt->second;

    // Bind source as read, default as draw
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcRT.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Determine rectangles
    int srcX = 0, srcY = 0, srcW = srcRT.desc.width, srcH = srcRT.desc.height;
    int dstX = 0, dstY = 0, dstW = srcW, dstH = srcH;

    if (srcRect) {
        srcX = srcRect[0]; srcY = srcRect[1]; srcW = srcRect[2]; srcH = srcRect[3];
    }
    if (dstRect) {
        dstX = dstRect[0]; dstY = dstRect[1]; dstW = dstRect[2]; dstH = dstRect[3];
    }

    // Perform resolve
    glBlitFramebuffer(srcX, srcY, srcX + srcW, srcY + srcH,
                     dstX, dstY, dstX + dstW, dstY + dstH,
                     GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Restore previous framebuffer bindings
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

std::unique_ptr<CommandEncoder> RhiGL::createCommandEncoder(const char* debugName) {
    return std::make_unique<SimpleCommandEncoderGL>(*this, debugName ? debugName : "");
}

Queue& RhiGL::getQueue() {
    return m_queue;
}

bool RhiGL::supportsCompute() const {
    return m_supportsCompute;
}

bool RhiGL::supportsGeometryShaders() const {
    return m_supportsGeometry;
}

bool RhiGL::supportsTessellation() const {
    return m_supportsTessellation;
}

int RhiGL::getMaxTextureUnits() const {
    return m_maxTextureUnits;
}

int RhiGL::getMaxSamples() const {
    return m_maxSamples;
}

GLuint RhiGL::getGLTexture(TextureHandle handle) const {
    auto it = m_textures.find(handle);
    return (it != m_textures.end()) ? it->second.id : 0;
}

GLuint RhiGL::getGLBuffer(BufferHandle handle) const {
    auto it = m_buffers.find(handle);
    return (it != m_buffers.end()) ? it->second.id : 0;
}

GLuint RhiGL::getGLShader(ShaderHandle handle) const {
    auto it = m_shaders.find(handle);
    return (it != m_shaders.end()) ? it->second.program : 0;
}

// Helper method implementations
GLenum RhiGL::textureFormatToGL(TextureFormat format) const {
    switch (format) {
        case TextureFormat::RGBA8: return GL_RGBA8;
        case TextureFormat::RGBA16F: return GL_RGBA16F;
        case TextureFormat::RGBA32F: return GL_RGBA32F;
        case TextureFormat::RGB8: return GL_RGB8;
        case TextureFormat::RGB16F: return GL_RGB16F;
        case TextureFormat::RGB32F: return GL_RGB32F;
        case TextureFormat::RG8: return GL_RG8;
        case TextureFormat::RG16F: return GL_RG16F;
        case TextureFormat::RG32F: return GL_RG32F;
        case TextureFormat::R8: return GL_R8;
        case TextureFormat::R16F: return GL_R16F;
        case TextureFormat::R32F: return GL_R32F;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT32F;
        default: return GL_RGBA8;
    }
}

void RhiGL::getTextureFormatAndType(TextureFormat format, GLenum& outFormat, GLenum& outType) const {
    switch (format) {
        case TextureFormat::RGBA8:
            outFormat = GL_RGBA; outType = GL_UNSIGNED_BYTE; break;
        case TextureFormat::RGBA16F:
            outFormat = GL_RGBA; outType = GL_HALF_FLOAT; break;
        case TextureFormat::RGBA32F:
            outFormat = GL_RGBA; outType = GL_FLOAT; break;
        case TextureFormat::RGB8:
            outFormat = GL_RGB; outType = GL_UNSIGNED_BYTE; break;
        case TextureFormat::RGB16F:
            outFormat = GL_RGB; outType = GL_HALF_FLOAT; break;
        case TextureFormat::RGB32F:
            outFormat = GL_RGB; outType = GL_FLOAT; break;
        case TextureFormat::RG8:
            outFormat = GL_RG; outType = GL_UNSIGNED_BYTE; break;
        case TextureFormat::RG16F:
            outFormat = GL_RG; outType = GL_HALF_FLOAT; break;
        case TextureFormat::RG32F:
            outFormat = GL_RG; outType = GL_FLOAT; break;
        case TextureFormat::R8:
            outFormat = GL_RED; outType = GL_UNSIGNED_BYTE; break;
        case TextureFormat::R16F:
            outFormat = GL_RED; outType = GL_HALF_FLOAT; break;
        case TextureFormat::R32F:
            outFormat = GL_RED; outType = GL_FLOAT; break;
        case TextureFormat::Depth24Stencil8:
            outFormat = GL_DEPTH_STENCIL; outType = GL_UNSIGNED_INT_24_8; break;
        case TextureFormat::Depth32F:
            outFormat = GL_DEPTH_COMPONENT; outType = GL_FLOAT; break;
        default:
            outFormat = GL_RGBA; outType = GL_UNSIGNED_BYTE; break;
    }
}

GLenum RhiGL::textureTypeToGL(TextureType type) const {
    switch (type) {
        case TextureType::Texture2D: return GL_TEXTURE_2D;
        case TextureType::TextureCube: return GL_TEXTURE_CUBE_MAP;
        case TextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
        case TextureType::Texture3D: return GL_TEXTURE_3D;
        default: return GL_TEXTURE_2D;
    }
}

GLenum RhiGL::bufferTypeToGL(BufferType type) const {
    switch (type) {
        case BufferType::Vertex: return GL_ARRAY_BUFFER;
        case BufferType::Index: return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Uniform: return GL_UNIFORM_BUFFER;
        case BufferType::Storage: 
#ifdef GL_SHADER_STORAGE_BUFFER
            return GL_SHADER_STORAGE_BUFFER;
#else
            return GL_ARRAY_BUFFER; // Fallback for older OpenGL
#endif
        default: return GL_ARRAY_BUFFER;
    }
}

GLenum RhiGL::bufferUsageToGL(BufferUsage usage) const {
    switch (usage) {
        case BufferUsage::Static: return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream: return GL_STREAM_DRAW;
        default: return GL_STATIC_DRAW;
    }
}

GLenum RhiGL::primitiveTopologyToGL(PrimitiveTopology topology) const {
    switch (topology) {
        case PrimitiveTopology::Triangles: return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveTopology::Lines: return GL_LINES;
        case PrimitiveTopology::LineStrip: return GL_LINE_STRIP;
        case PrimitiveTopology::Points: return GL_POINTS;
        default: return GL_TRIANGLES;
    }
}

bool RhiGL::compileShader(GLuint& program, const ShaderDesc& desc) {
    program = glCreateProgram();
    
    std::vector<GLuint> shaders;
    
    // Compile vertex shader
    if (!desc.vertexSource.empty()) {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        const char* source = desc.vertexSource.c_str();
        glShaderSource(vs, 1, &source, nullptr);
        glCompileShader(vs);
        
        GLint success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(vs, 512, nullptr, infoLog);
            std::cerr << "[RhiGL] Vertex shader compilation failed: " << infoLog << std::endl;
            glDeleteShader(vs);
            return false;
        }
        
        glAttachShader(program, vs);
        shaders.push_back(vs);
    }
    
    // Compile fragment shader
    if (!desc.fragmentSource.empty()) {
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        const char* source = desc.fragmentSource.c_str();
        glShaderSource(fs, 1, &source, nullptr);
        glCompileShader(fs);
        
        GLint success;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(fs, 512, nullptr, infoLog);
            std::cerr << "[RhiGL] Fragment shader compilation failed: " << infoLog << std::endl;
            glDeleteShader(fs);
            return false;
        }
        
        glAttachShader(program, fs);
        shaders.push_back(fs);
    }
    
    // Link program
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[RhiGL] Shader program linking failed: " << infoLog << std::endl;
        
        for (GLuint shader : shaders) {
            glDeleteShader(shader);
        }
        glDeleteProgram(program);
        program = 0;
        return false;
    }
    
    // Clean up individual shaders
    for (GLuint shader : shaders) {
        glDeleteShader(shader);
    }
    
    return true;
}

void RhiGL::queryCapabilities() {
    // Query OpenGL version and extensions
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    // Compute shaders require OpenGL 4.3+
    m_supportsCompute = (major > 4) || (major == 4 && minor >= 3);
    
    // Geometry shaders require OpenGL 3.2+
    m_supportsGeometry = (major > 3) || (major == 3 && minor >= 2);
    
    // Tessellation requires OpenGL 4.0+
    m_supportsTessellation = (major >= 4);
    
    // Query limits
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_maxTextureUnits);
    glGetIntegerv(GL_MAX_SAMPLES, &m_maxSamples);
}

void RhiGL::setupVertexArray(GLuint vao, const PipelineDesc& desc) {
    glBindVertexArray(vao);
    
    auto componentsFromFormat = [](TextureFormat fmt) -> GLint {
        switch (fmt) {
            case TextureFormat::R32F:
            case TextureFormat::R16F:
            case TextureFormat::R8: return 1;
            case TextureFormat::RG32F:
            case TextureFormat::RG16F:
            case TextureFormat::RG8: return 2;
            case TextureFormat::RGB32F:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB8: return 3;
            case TextureFormat::RGBA32F:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA8: return 4;
            default: return 3;
        }
    };
    auto typeFromFormat = [](TextureFormat fmt) -> GLenum {
        switch (fmt) {
            case TextureFormat::RGBA8:
            case TextureFormat::RGB8:
            case TextureFormat::RG8:
            case TextureFormat::R8: return GL_UNSIGNED_BYTE;
            default: return GL_FLOAT;
        }
    };

    // Configure vertex attributes with bound buffers per binding
    for (const auto& attr : desc.vertexAttributes) {
        // Find the binding parameters and buffer
        const VertexBinding* vb = nullptr;
        for (const auto& binding : desc.vertexBindings) {
            if (binding.binding == attr.binding) { vb = &binding; break; }
        }
        if (!vb) continue;

        // Bind buffer for this attribute
        auto bufIt = m_buffers.find(vb->buffer);
        if (bufIt != m_buffers.end()) {
            glBindBuffer(GL_ARRAY_BUFFER, bufIt->second.id);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        glEnableVertexAttribArray(attr.location);
        const GLint comps = componentsFromFormat(attr.format);
        const GLenum glType = typeFromFormat(attr.format);
        const GLboolean normalized = (glType != GL_FLOAT) ? GL_TRUE : GL_FALSE;
        glVertexAttribPointer(attr.location, comps, glType, normalized, vb->stride,
                              reinterpret_cast<void*>(static_cast<uintptr_t>(attr.offset)));
        // Instancing support
        glVertexAttribDivisor(attr.location, vb->perInstance ? 1 : 0);
    }

    // Bind index buffer to VAO if provided
    if (desc.indexBuffer != 0) {
        auto ibIt = m_buffers.find(desc.indexBuffer);
        if (ibIt != m_buffers.end()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibIt->second.id);
        }
    }
    
    glBindVertexArray(0);
}

std::string RhiGL::getDebugInfo() const {
    std::ostringstream info;
    info << "OpenGL RHI Debug Info:\n";
    info << "  Vendor: " << (const char*)glGetString(GL_VENDOR) << "\n";
    info << "  Renderer: " << (const char*)glGetString(GL_RENDERER) << "\n";
    info << "  Version: " << (const char*)glGetString(GL_VERSION) << "\n";
    info << "  GLSL Version: " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    info << "  Max Texture Units: " << getMaxTextureUnits() << "\n";
    info << "  Max Samples: " << getMaxSamples() << "\n";
    return info.str();
}

BufferHandle RhiGL::getScreenQuadBuffer() {
    if (m_screenQuadBuffer != INVALID_HANDLE) {
        return m_screenQuadBuffer; // Already created
    }

    // Screen quad vertices (NDC coordinates with UVs)
    // Two triangles forming a full-screen quad
    float quadVertices[] = {
        // positions   // UVs
        -1.0f,  1.0f,  0.0f, 1.0f,  // top-left
        -1.0f, -1.0f,  0.0f, 0.0f,  // bottom-left
         1.0f, -1.0f,  1.0f, 0.0f,  // bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f,  // top-left
         1.0f, -1.0f,  1.0f, 0.0f,  // bottom-right
         1.0f,  1.0f,  1.0f, 1.0f   // top-right
    };

    BufferDesc bufferDesc{};
    bufferDesc.type = BufferType::Vertex;
    bufferDesc.usage = BufferUsage::Static;
    bufferDesc.size = sizeof(quadVertices);
    bufferDesc.initialData = quadVertices;
    bufferDesc.debugName = "RHI_ScreenQuad";

    m_screenQuadBuffer = createBuffer(bufferDesc);
    return m_screenQuadBuffer;
}

GLenum RhiGL::attachmentTypeToGL(AttachmentType type) const {
    switch (type) {
        case AttachmentType::Color0: return GL_COLOR_ATTACHMENT0;
        case AttachmentType::Color1: return GL_COLOR_ATTACHMENT1;
        case AttachmentType::Color2: return GL_COLOR_ATTACHMENT2;
        case AttachmentType::Color3: return GL_COLOR_ATTACHMENT3;
        case AttachmentType::Color4: return GL_COLOR_ATTACHMENT4;
        case AttachmentType::Color5: return GL_COLOR_ATTACHMENT5;
        case AttachmentType::Color6: return GL_COLOR_ATTACHMENT6;
        case AttachmentType::Color7: return GL_COLOR_ATTACHMENT7;
        case AttachmentType::Depth: return GL_DEPTH_ATTACHMENT;
        case AttachmentType::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
        default: return GL_COLOR_ATTACHMENT0;
    }
}

bool RhiGL::setupRenderTarget(GLuint fbo, const RenderTargetDesc& desc) {
    GLint prevFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // Attach color attachments
    for (const auto& attachment : desc.colorAttachments) {
        auto texIt = m_textures.find(attachment.texture);
        if (texIt == m_textures.end()) {
            std::cerr << "[RhiGL] Invalid texture handle for render target attachment\n";
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            return false;
        }
        
        GLenum attachPoint = attachmentTypeToGL(attachment.type);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, GL_TEXTURE_2D, 
                              texIt->second.id, attachment.mipLevel);
    }
    
    // Attach depth attachment if specified
    if (desc.depthAttachment.texture != INVALID_HANDLE) {
        auto texIt = m_textures.find(desc.depthAttachment.texture);
        if (texIt == m_textures.end()) {
            std::cerr << "[RhiGL] Invalid depth texture handle for render target\n";
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            return false;
        }
        
        GLenum attachPoint = attachmentTypeToGL(desc.depthAttachment.type);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, GL_TEXTURE_2D,
                              texIt->second.id, desc.depthAttachment.mipLevel);
    }
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[RhiGL] Framebuffer not complete: " << status << "\n";
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    return true;
}

// Simple WebGPU-shaped adapters

RhiGL::SimpleRenderPassEncoderGL::SimpleRenderPassEncoderGL(RhiGL& rhi, const RenderPassDesc& desc)
    : m_rhi(rhi), m_desc(desc), m_active(true) {
    // Bind target or build from attachments
    if (desc.target != INVALID_HANDLE) {
        m_rhi.bindRenderTarget(desc.target);
    } else {
        // Fallback: if a color attachment with texture is provided and matches an existing RT, ignore for now
        // In GL path we require a RenderTarget; callers should pass one for now.
        m_rhi.bindRenderTarget(INVALID_HANDLE);
    }

    // Clear/load based on first color attachment
    if (!desc.colorAttachments.empty()) {
        const auto& ca = desc.colorAttachments[0];
        if (ca.loadOp == LoadOp::Clear) {
            m_rhi.clear(ca.clearColor, m_desc.depthStencil.depthClear, (int)m_desc.depthStencil.stencilClear);
        }
    } else if (desc.depthStencil.texture != INVALID_HANDLE && desc.depthStencil.depthLoadOp == LoadOp::Clear) {
        m_rhi.clear(glm::vec4(0,0,0,0), m_desc.depthStencil.depthClear, (int)m_desc.depthStencil.stencilClear);
    }

    if (desc.width > 0 && desc.height > 0) {
        m_rhi.setViewport(0, 0, desc.width, desc.height);
    }
}

void RhiGL::SimpleRenderPassEncoderGL::setPipeline(PipelineHandle pipeline) { m_rhi.bindPipeline(pipeline); }

void RhiGL::SimpleRenderPassEncoderGL::setBindGroup(uint32_t index, BindGroupHandle group) {
    m_rhi.applyBindGroup(index, group);
}

void RhiGL::SimpleRenderPassEncoderGL::setViewport(int x, int y, int width, int height) {
    m_rhi.setViewport(x, y, width, height);
}

void RhiGL::SimpleRenderPassEncoderGL::draw(const DrawDesc& desc) { m_rhi.draw(desc); }

void RhiGL::SimpleRenderPassEncoderGL::end() { m_active = false; }

RhiGL::SimpleCommandEncoderGL::SimpleCommandEncoderGL(RhiGL& rhi, const char* name)
    : m_rhi(rhi), m_name(name ? name : "") {}

RenderPassEncoder* RhiGL::SimpleCommandEncoderGL::beginRenderPass(const RenderPassDesc& desc) {
    m_activePass = std::make_unique<SimpleRenderPassEncoderGL>(m_rhi, desc);
    return m_activePass.get();
}

void RhiGL::SimpleCommandEncoderGL::finish() {
    // Immediate mode: nothing to do; ensure pass ended
    if (m_activePass) { m_activePass->end(); m_activePass.reset(); }
}

void RhiGL::SimpleQueueGL::submit(CommandEncoder& encoder) {
    // GL path executes immediately; just call finish to satisfy lifecycle
    encoder.finish();
}

void RhiGL::applyBindGroup(uint32_t /*index*/, BindGroupHandle group) {
    auto it = m_bindGroups.find(group);
    if (it == m_bindGroups.end()) return;
    const auto& bg = it->second.desc;
    for (const auto& e : bg.entries) {
        if (e.buffer.buffer != INVALID_HANDLE) {
            bindUniformBuffer(e.buffer.buffer, e.binding);
        }
        if (e.texture.texture != INVALID_HANDLE) {
            bindTexture(e.texture.texture, e.binding);
        }
    }
}

// Legacy uniform helpers - transitional bridge to proper UBOs
// TODO: Convert to dynamic UBO system for true RHI compliance
void RhiGL::setUniformMat4(const char* name, const glm::mat4& value) {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (!prog) return;
    GLint loc = glGetUniformLocation(prog, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void RhiGL::setUniformVec3(const char* name, const glm::vec3& value) {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (!prog) return;
    GLint loc = glGetUniformLocation(prog, name);
    if (loc >= 0) glUniform3fv(loc, 1, &value[0]);
}

void RhiGL::setUniformVec4(const char* name, const glm::vec4& value) {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (!prog) return;
    GLint loc = glGetUniformLocation(prog, name);
    if (loc >= 0) glUniform4fv(loc, 1, &value[0]);
}

void RhiGL::setUniformFloat(const char* name, float value) {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (!prog) return;
    GLint loc = glGetUniformLocation(prog, name);
    if (loc >= 0) glUniform1f(loc, value);
}

void RhiGL::setUniformInt(const char* name, int value) {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (!prog) return;
    GLint loc = glGetUniformLocation(prog, name);
    if (loc >= 0) glUniform1i(loc, value);
}

void RhiGL::setUniformBool(const char* name, bool value) {
    setUniformInt(name, value ? 1 : 0);
}

// Uniform Buffer Ring Allocator Implementation (FEAT-0249)
// =========================================================

UniformAllocation RhiGL::allocateUniforms(const UniformAllocationDesc& desc) {
    UniformAllocation result{};

    // Align the size to required alignment
    uint32_t alignedSize = alignOffset(desc.size, desc.alignment);

    // Check if we have space in the current ring buffer
    if (m_uniformRing.offset + alignedSize > m_uniformRing.size) {
        // Ring buffer is full, wrap around to beginning
        m_uniformRing.offset = 0;
    }

    // Create allocation record
    GLUniformAllocation allocation{};
    allocation.handle = m_nextUniformHandle++;
    allocation.bufferHandle = INVALID_HANDLE; // We use the ring buffer directly
    allocation.offset = static_cast<uint32_t>(m_uniformRing.offset);
    allocation.size = alignedSize;
    allocation.inUse = true;

    // Calculate mapped pointer
    if (m_uniformRing.mappedPtr) {
        allocation.mappedPtr = static_cast<char*>(m_uniformRing.mappedPtr) + allocation.offset;
    }

    // Update ring buffer offset
    m_uniformRing.offset += alignedSize;

    // Store allocation
    m_uniformAllocations[allocation.handle] = allocation;

    // Fill result
    result.handle = allocation.handle;
    result.buffer = allocation.bufferHandle;
    result.offset = allocation.offset;
    result.mappedPtr = allocation.mappedPtr;

    return result;
}

void RhiGL::freeUniforms(const UniformAllocation& allocation) {
    auto it = m_uniformAllocations.find(allocation.handle);
    if (it != m_uniformAllocations.end()) {
        it->second.inUse = false;
        // Note: Ring buffer allocation space is reused on wrap-around
        // We don't explicitly free individual allocations
    }
}

ShaderReflection RhiGL::getShaderReflection(ShaderHandle shader) {
    auto it = m_shaderReflections.find(shader);
    if (it != m_shaderReflections.end()) {
        return it->second;
    }

    // Return empty reflection if not found
    ShaderReflection reflection{};
    reflection.isValid = false;
    return reflection;
}

bool RhiGL::setUniformInBlock(const UniformAllocation& allocation, ShaderHandle shader,
                            const char* blockName, const char* varName,
                            const void* data, size_t dataSize) {
    // Get shader reflection
    auto reflection = getShaderReflection(shader);
    if (!reflection.isValid) {
        return false;
    }

    // Find the uniform block
    UniformBlockReflection* block = nullptr;
    for (auto& ub : reflection.uniformBlocks) {
        if (ub.blockName == blockName) {
            block = &ub;
            break;
        }
    }

    if (!block) {
        return false;
    }

    // Find the variable within the block
    UniformVariableInfo* variable = nullptr;
    for (auto& var : block->variables) {
        if (var.name == varName) {
            variable = &var;
            break;
        }
    }

    if (!variable) {
        return false;
    }

    // Validate data size
    if (dataSize != variable->size) {
        return false;
    }

    // Get allocation
    auto allocIt = m_uniformAllocations.find(allocation.handle);
    if (allocIt == m_uniformAllocations.end() || !allocIt->second.inUse) {
        return false;
    }

    // Copy data to mapped buffer
    if (allocIt->second.mappedPtr) {
        char* dest = static_cast<char*>(allocIt->second.mappedPtr) + variable->offset;
        memcpy(dest, data, dataSize);
        return true;
    }

    return false;
}

int RhiGL::setUniformsInBlock(const UniformAllocation& allocation, ShaderHandle shader,
                            const char* blockName, const UniformNameValue* uniforms, int count) {
    int successCount = 0;

    for (int i = 0; i < count; ++i) {
        if (setUniformInBlock(allocation, shader, blockName, uniforms[i].name,
                            uniforms[i].data, uniforms[i].dataSize)) {
            successCount++;
        }
    }

    return successCount;
}

bool RhiGL::bindUniformBlock(const UniformAllocation& allocation, ShaderHandle shader,
                             const char* blockName) {
    // Validate shader reflection
    auto reflection = getShaderReflection(shader);
    if (!reflection.isValid) {
        return false;
    }

    // Find the uniform block by name
    const UniformBlockReflection* block = nullptr;
    for (const auto& ub : reflection.uniformBlocks) {
        if (ub.blockName == blockName) { block = &ub; break; }
    }
    if (!block) {
        return false;
    }

    // Validate allocation
    auto it = m_uniformAllocations.find(allocation.handle);
    if (it == m_uniformAllocations.end() || !it->second.inUse) {
        return false;
    }

    // Bind the ring buffer range to the block binding point
    glBindBufferRange(GL_UNIFORM_BUFFER,
                      block->binding,
                      m_uniformRing.buffer,
                      it->second.offset,
                      block->blockSize);
    return true;
}

// UBO Helper Methods
// ==================

bool RhiGL::initializeUniformRing() {
    // Generate uniform buffer
    glGenBuffers(1, &m_uniformRing.buffer);
    if (m_uniformRing.buffer == 0) {
        return false;
    }

    // Allocate buffer storage
    glBindBuffer(GL_UNIFORM_BUFFER, m_uniformRing.buffer);
    glBufferData(GL_UNIFORM_BUFFER, UBO_RING_SIZE, nullptr, GL_DYNAMIC_DRAW);

    m_uniformRing.size = UBO_RING_SIZE;
    m_uniformRing.offset = 0;

    // Try to get persistent mapping if available (OpenGL 4.4+)
#ifdef GL_MAP_PERSISTENT_BIT
    if (GLAD_GL_ARB_buffer_storage) {
        glBufferStorage(GL_UNIFORM_BUFFER, UBO_RING_SIZE, nullptr,
                       GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        m_uniformRing.mappedPtr = glMapBufferRange(GL_UNIFORM_BUFFER, 0, UBO_RING_SIZE,
                                                  GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        m_uniformRing.persistent = (m_uniformRing.mappedPtr != nullptr);
    }
#endif

    // Fallback to non-persistent mapping
    if (!m_uniformRing.persistent) {
        m_uniformRing.mappedPtr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return true;
}

void RhiGL::shutdownUniformRing() {
    if (m_uniformRing.buffer) {
        if (m_uniformRing.mappedPtr) {
            glBindBuffer(GL_UNIFORM_BUFFER, m_uniformRing.buffer);
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
        glDeleteBuffers(1, &m_uniformRing.buffer);
        m_uniformRing = {};
    }

    m_uniformAllocations.clear();
    m_shaderReflections.clear();
}

uint32_t RhiGL::alignOffset(uint32_t offset, uint32_t alignment) const {
    return (offset + alignment - 1) & ~(alignment - 1);
}

bool RhiGL::createShaderReflection(ShaderHandle shader, const ShaderDesc& desc) {
    auto shaderIt = m_shaders.find(shader);
    if (shaderIt == m_shaders.end() || shaderIt->second.program == 0) {
        return false;
    }

    GLuint program = shaderIt->second.program;
    ShaderReflection reflection{};

    // Query uniform blocks using OpenGL introspection
    GLint numBlocks = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);

    for (GLint blockIndex = 0; blockIndex < numBlocks; ++blockIndex) {
        UniformBlockReflection blockReflection{};

        // Get block name
        GLint nameLength = 0;
        glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength);
        if (nameLength > 0) {
            blockReflection.blockName.resize(nameLength);
            glGetActiveUniformBlockName(program, blockIndex, nameLength, nullptr, blockReflection.blockName.data());
            if (!blockReflection.blockName.empty() && blockReflection.blockName.back() == '\0') {
                blockReflection.blockName.pop_back(); // Remove null terminator
            }
        }

        // Get block size
        GLint blockSize = 0;
        glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
        blockReflection.blockSize = static_cast<uint32_t>(blockSize);

        // Get binding point
        GLint binding = 0;
        glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_BINDING, &binding);
        blockReflection.binding = static_cast<uint32_t>(binding);

        // Get uniforms in this block
        GLint numActiveUniforms = 0;
        glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numActiveUniforms);

        if (numActiveUniforms > 0) {
            std::vector<GLint> uniformIndices(numActiveUniforms);
            glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniformIndices.data());

            for (GLint uniformIndex : uniformIndices) {
                UniformVariableInfo varInfo{};

                // Get uniform name
                GLint maxNameLength = 0;
                glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
                if (maxNameLength > 0) {
                    std::vector<char> nameBuffer(maxNameLength);
                    GLsizei actualLength = 0;
                    GLenum glType = 0;
                    GLint size = 0;
                    glGetActiveUniform(program, uniformIndex, maxNameLength, &actualLength, &size, &glType, nameBuffer.data());
                    varInfo.name = std::string(nameBuffer.data(), actualLength);
                    varInfo.arraySize = static_cast<uint32_t>(size);

                    // Convert GL type to UniformType
                    switch (glType) {
                        case GL_FLOAT: varInfo.type = UniformType::Float; varInfo.size = 4; break;
                        case GL_FLOAT_VEC2: varInfo.type = UniformType::Vec2; varInfo.size = 8; break;
                        case GL_FLOAT_VEC3: varInfo.type = UniformType::Vec3; varInfo.size = 12; break;
                        case GL_FLOAT_VEC4: varInfo.type = UniformType::Vec4; varInfo.size = 16; break;
                        case GL_FLOAT_MAT3: varInfo.type = UniformType::Mat3; varInfo.size = 36; break;
                        case GL_FLOAT_MAT4: varInfo.type = UniformType::Mat4; varInfo.size = 64; break;
                        case GL_INT: case GL_BOOL: varInfo.type = UniformType::Int; varInfo.size = 4; break;
                        default: varInfo.type = UniformType::Float; varInfo.size = 4; break;
                    }

                    // Adjust size for arrays
                    varInfo.size *= varInfo.arraySize;

                    // Get uniform offset
                    GLint offset = 0;
                    glGetActiveUniformsiv(program, 1, reinterpret_cast<const GLuint*>(&uniformIndex), GL_UNIFORM_OFFSET, &offset);
                    varInfo.offset = static_cast<uint32_t>(offset);

                    blockReflection.variables.push_back(varInfo);
                }
            }
        }

        reflection.uniformBlocks.push_back(blockReflection);
    }

    reflection.isValid = true;
    m_shaderReflections[shader] = reflection;
    return true;
}
