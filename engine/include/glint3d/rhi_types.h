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
using BindGroupLayoutHandle = uint32_t;
using BindGroupHandle = uint32_t;
using PipelineLayoutHandle = uint32_t;
using SamplerHandle = uint32_t;

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

// Resource states (WebGPU-shaped)
enum class ResourceState : uint32_t {
    Undefined = 0,
    RenderTarget,
    DepthStencil,
    ShaderRead,
    ShaderWrite,
    CopySrc,
    CopyDst,
    Present
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

// Polygon rasterization mode
enum class PolygonMode {
    Fill,      // Solid polygons (default)
    Line,      // Wireframe
    Point      // Vertices only
};

// Blend factors for source and destination
enum class BlendFactor {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor
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

// Visibility bitset helpers
inline uint32_t shaderStageBits(ShaderStage s) { return static_cast<uint32_t>(s); }

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

// Bind group layout (WebGPU-shaped)
enum class BindingType : uint32_t {
    UniformBuffer,
    StorageBuffer,
    Sampler,
    SampledTexture,
    StorageTexture
};

struct BindGroupLayoutEntry {
    uint32_t binding = 0;
    BindingType type = BindingType::UniformBuffer;
    uint32_t visibility = shaderStageBits(ShaderStage::Vertex) | shaderStageBits(ShaderStage::Fragment);
};

struct BindGroupLayoutDesc {
    std::vector<BindGroupLayoutEntry> entries;
    std::string debugName;
};

// Bind group (resource set) description
struct BufferBinding {
    BufferHandle buffer = INVALID_HANDLE;
    size_t offset = 0;
    size_t size = 0; // 0 means whole buffer from offset
};

struct TextureBinding {
    TextureHandle texture = INVALID_HANDLE;
    SamplerHandle sampler = INVALID_HANDLE;
};

struct BindGroupEntry {
    uint32_t binding = 0;
    // One of the below will be used depending on layout type
    BufferBinding buffer;
    TextureBinding texture;
};

struct BindGroupDesc {
    BindGroupLayoutHandle layout = INVALID_HANDLE;
    std::vector<BindGroupEntry> entries;
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
    BlendFactor srcColorBlendFactor = BlendFactor::One;
    BlendFactor dstColorBlendFactor = BlendFactor::Zero;
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
    PolygonMode polygonMode = PolygonMode::Fill;
    float lineWidth = 1.0f;
    bool polygonOffsetEnable = false;
    float polygonOffsetFactor = 0.0f;
    float polygonOffsetUnits = 0.0f;
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

// Cubemap face enumeration (matches OpenGL/WebGL convention)
enum class CubemapFace : uint32_t {
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5
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

// Render pass description (WebGPU-shaped)
enum class LoadOp { Load, Clear };
enum class StoreOp { Store, Discard };

struct ColorAttachmentDesc {
    TextureHandle texture = INVALID_HANDLE; // bound as color attachment view
    glm::vec4 clearColor = glm::vec4(0, 0, 0, 1);
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
};

struct DepthStencilAttachmentDesc {
    TextureHandle texture = INVALID_HANDLE; // bound as depth/stencil view
    float depthClear = 1.0f;
    uint32_t stencilClear = 0;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
};

struct RenderPassDesc {
    // Either specify a prebuilt render target handle, or direct attachments
    RenderTargetHandle target = INVALID_HANDLE; // if set, overrides attachments below
    std::vector<ColorAttachmentDesc> colorAttachments;
    DepthStencilAttachmentDesc depthStencil;
    int width = 0;
    int height = 0;
    std::string debugName;
};

// Uniform Buffer Ring Allocator Types (FEAT-0249)
// ================================================

/**
 * @brief Handle to a uniform buffer allocation from the ring allocator
 */
using UniformAllocationHandle = uint32_t;

/**
 * @brief Uniform data type enumeration for validation
 */
enum class UniformType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    Int,
    Bool
};

/**
 * @brief Uniform variable reflection information
 */
struct UniformVariableInfo {
    std::string name;
    UniformType type;
    uint32_t offset;        // Byte offset in uniform buffer
    uint32_t size;          // Size in bytes
    uint32_t arraySize;     // 1 for non-arrays
};

/**
 * @brief Uniform buffer reflection information
 */
struct UniformBlockReflection {
    std::string blockName;
    uint32_t blockSize;     // Total size in bytes
    uint32_t binding;       // Binding point
    std::vector<UniformVariableInfo> variables;
};

/**
 * @brief Shader reflection data containing all uniform blocks
 */
struct ShaderReflection {
    std::vector<UniformBlockReflection> uniformBlocks;
    bool isValid = false;
};

/**
 * @brief Parameters for uniform buffer allocation
 */
struct UniformAllocationDesc {
    uint32_t size;          // Size in bytes
    uint32_t alignment = 16; // Required alignment (UBO std140 is 16 bytes)
    const char* debugName = nullptr;
};

/**
 * @brief Result of uniform buffer allocation
 */
struct UniformAllocation {
    UniformAllocationHandle handle = INVALID_HANDLE;
    BufferHandle buffer = INVALID_HANDLE;
    uint32_t offset = 0;    // Offset within the buffer
    void* mappedPtr = nullptr; // CPU-accessible pointer for updates
};

} // namespace glint3d
