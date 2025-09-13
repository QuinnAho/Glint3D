#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace glint3d {

// Resource handle types - opaque handles for type safety
using TextureHandle = uint32_t;
using BufferHandle = uint32_t;
using ShaderHandle = uint32_t;
using PipelineHandle = uint32_t;
using RenderTargetHandle = uint32_t;

constexpr uint32_t INVALID_HANDLE = 0;

// RHI Initialization descriptor
struct RhiInit {
    int windowWidth = 800;
    int windowHeight = 600;
    bool enableDebug = false;
    bool enableSRGB = true;
    int samples = 1; // MSAA samples
    const char* applicationName = "Glint3D";
};

// Texture formats - keep compatible with existing engine usage
enum class TextureFormat {
    RGBA8,
    RGBA16F,
    RGBA32F,
    RGB8,
    RGB16F,
    RGB32F,
    RG8,
    RG16F,
    RG32F,
    R8,
    R16F,
    R32F,
    Depth24Stencil8,
    Depth32F
};

// Texture types
enum class TextureType {
    Texture2D,
    TextureCube,
    Texture2DArray,
    Texture3D
};

// Buffer types
enum class BufferType {
    Vertex,
    Index,
    Uniform,
    Storage
};

// Buffer usage patterns
enum class BufferUsage {
    Static,   // Rarely modified
    Dynamic,  // Frequently modified  
    Stream    // Modified every frame
};

// Primitive topology
enum class PrimitiveTopology {
    Triangles,
    TriangleStrip,
    Lines,
    LineStrip,
    Points
};

// Shader stages - bitfield for multi-stage shaders
enum class ShaderStage : uint32_t {
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Geometry = 1 << 2,
    TessControl = 1 << 3,
    TessEvaluation = 1 << 4,
    Compute = 1 << 5
};

// Enable bitwise operations for ShaderStage
inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ShaderStage operator&(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// Texture descriptor
struct TextureDesc {
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::RGBA8;
    int width = 0;
    int height = 0;
    int depth = 1;
    int mipLevels = 1;
    int arrayLayers = 1;
    bool generateMips = false;
    const void* initialData = nullptr;
    size_t initialDataSize = 0;
    std::string debugName;
};

// Buffer descriptor
struct BufferDesc {
    BufferType type = BufferType::Vertex;
    BufferUsage usage = BufferUsage::Static;
    size_t size = 0;
    const void* initialData = nullptr;
    std::string debugName;
};

// Shader descriptor
struct ShaderDesc {
    uint32_t stages = 0; // Bitfield of ShaderStage
    std::string vertexSource;
    std::string fragmentSource;
    std::string geometrySource;
    std::string tessControlSource;
    std::string tessEvaluationSource;
    std::string computeSource;
    std::string debugName;
};

// Vertex attribute description
struct VertexAttribute {
    uint32_t location = 0;
    uint32_t binding = 0;
    TextureFormat format = TextureFormat::RGB32F;
    uint32_t offset = 0;
};

// Vertex binding description
struct VertexBinding {
    uint32_t binding = 0;
    uint32_t stride = 0;
    bool perInstance = false;
    BufferHandle buffer = INVALID_HANDLE;
};

// Pipeline descriptor
struct PipelineDesc {
    ShaderHandle shader = INVALID_HANDLE;
    std::vector<VertexAttribute> vertexAttributes;
    std::vector<VertexBinding> vertexBindings;
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    bool blendEnable = false;
    BufferHandle indexBuffer = INVALID_HANDLE;
    std::string debugName;
};

// Draw command descriptor
struct DrawDesc {
    PipelineHandle pipeline = INVALID_HANDLE;
    BufferHandle vertexBuffer = INVALID_HANDLE;
    BufferHandle indexBuffer = INVALID_HANDLE;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t firstInstance = 0;
};

// Readback descriptor for CPU access to GPU resources
struct ReadbackDesc {
    TextureHandle sourceTexture = INVALID_HANDLE;
    TextureFormat format = TextureFormat::RGBA8;
    int x = 0, y = 0;
    int width = 0, height = 0;
    void* destination = nullptr;
    size_t destinationSize = 0;
};

// Render target attachment types
enum class AttachmentType {
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    Depth,
    DepthStencil
};

// Render target attachment descriptor
struct RenderTargetAttachment {
    AttachmentType type = AttachmentType::Color0;
    TextureHandle texture = INVALID_HANDLE;
    int mipLevel = 0;
    int arrayLayer = 0; // For texture arrays/cubemaps
};

// Render target descriptor
struct RenderTargetDesc {
    std::vector<RenderTargetAttachment> colorAttachments;
    RenderTargetAttachment depthAttachment;
    int width = 0;
    int height = 0;
    int samples = 1; // MSAA samples
    std::string debugName;
};

} // namespace glint3d