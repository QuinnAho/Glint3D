#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

// RHI Initialization descriptor
struct RhiInit {
    int windowWidth = 800;
    int windowHeight = 600;
    bool enableDebug = false;
    bool enableSRGB = true;
    int samples = 1; // MSAA samples
    const char* applicationName = "Glint3D";
};

// Texture formats
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

// Shader stages
enum class ShaderStage {
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Geometry = 1 << 2,
    TessControl = 1 << 3,
    TessEvaluation = 1 << 4,
    Compute = 1 << 5
};

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
};

// Pipeline descriptor
struct PipelineDesc {
    ShaderHandle shader = 0;
    std::vector<VertexAttribute> vertexAttributes;
    std::vector<VertexBinding> vertexBindings;
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    bool blendEnable = false;
    std::string debugName;
};

// Draw command descriptor
struct DrawDesc {
    PipelineHandle pipeline = 0;
    BufferHandle vertexBuffer = 0;
    BufferHandle indexBuffer = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t firstInstance = 0;
};

// Readback descriptor
struct ReadbackDesc {
    TextureHandle sourceTexture = 0;
    TextureFormat format = TextureFormat::RGBA8;
    int x = 0, y = 0;
    int width = 0, height = 0;
    void* destination = nullptr;
    size_t destinationSize = 0;
};