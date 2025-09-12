#include "rhi/rhi_gl.h"
#include <iostream>
#include <sstream>

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
    GLenum format = internalFormat; // Simplified mapping
    GLenum type = GL_UNSIGNED_BYTE; // Simplified for now
    
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
