// machine summary block
// {"file":"engine/include/glint3d/rhi_types.h","purpose":"defines rhi type system with resource handles, descriptors, and enums","exports":["TextureHandle","BufferHandle","ShaderHandle","PipelineHandle","RenderTargetHandle","BindGroupHandle","RhiInit","TextureDesc","BufferDesc","PipelineDesc","DrawDesc","RenderPassDesc","UniformAllocation","ShaderReflection"],"depends_on":["glm"],"notes":["provides webgpu-shaped api types for cross-platform rendering","includes uniform buffer ring allocator types","all resource handles are opaque uint32_t for type safety"]}

/**
 * @file rhi_types.h
 * @brief type definitions for the render hardware interface abstraction layer.
 *
 * defines all handles, enums, and descriptor structures used by the rhi system. includes
 * resource handles (textures, buffers, shaders, pipelines), initialization and draw descriptors,
 * render pass configuration, and uniform buffer reflection types. designed to be backend-agnostic
 * and compatible with both opengl and future backends (vulkan, metal, webgpu).
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace glint3d {

// resource handle types (opaque for type safety)
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

// initialization and configuration
struct RhiInit {
    int windowWidth = 800;
    int windowHeight = 600;
    bool enableDebug = false;
    bool enableSRGB = true;
    int samples = 1; // MSAA samples
    const char* applicationName = "Glint3D";
};

// resource states and formats
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

enum class TextureType {
    Texture2D,
    TextureCube,
    Texture2DArray,
    Texture3D
};

// buffer and rendering configuration
enum class BufferType {
    Vertex,
    Index,
    Uniform,
    Storage
};

enum class BufferUsage {
    Static,   // rarely modified
    Dynamic,  // frequently modified  
    Stream    // modified every frame
};

enum class PrimitiveTopology {
    Triangles,
    TriangleStrip,
    Lines,
    LineStrip,
    Points
};

enum class PolygonMode {
    Fill,      // solid polygons (default)
    Line,      // wireframe
    Point      // vertices only
};

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

// shader configuration
enum class ShaderStage : uint32_t {
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Geometry = 1 << 2,
    TessControl = 1 << 3,
    TessEvaluation = 1 << 4,
    Compute = 1 << 5
};

// bitwise operations for shader stage flags
inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ShaderStage operator&(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline uint32_t shaderStageBits(ShaderStage s) { return static_cast<uint32_t>(s); }

// resource descriptors
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

struct BufferDesc {
    BufferType type = BufferType::Vertex;
    BufferUsage usage = BufferUsage::Static;
    size_t size = 0;
    const void* initialData = nullptr;
    std::string debugName;
};

// bind group layout (webgpu-shaped)
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

// bind group (resource set) description

struct BufferBinding {
    BufferHandle buffer = INVALID_HANDLE;
    size_t offset = 0;
    size_t size = 0; // 0 = whole buffer from offset
};

struct TextureBinding {
    TextureHandle texture = INVALID_HANDLE;
    SamplerHandle sampler = INVALID_HANDLE;
};

struct BindGroupEntry {
    uint32_t binding = 0;
    BufferBinding buffer;   // used for uniform/storage buffers
    TextureBinding texture; // used for sampled textures
};

struct BindGroupDesc {
    BindGroupLayoutHandle layout = INVALID_HANDLE;
    std::vector<BindGroupEntry> entries;
    std::string debugName;
};

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

// vertex input configuration
struct VertexAttribute {
    uint32_t location = 0;
    uint32_t binding = 0;
    TextureFormat format = TextureFormat::RGB32F;
    uint32_t offset = 0;
};

struct VertexBinding {
    uint32_t binding = 0;
    uint32_t stride = 0;
    bool perInstance = false;
    BufferHandle buffer = INVALID_HANDLE;
};

// pipeline configuration
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

// draw and readback commands
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

struct ReadbackDesc {
    TextureHandle sourceTexture = INVALID_HANDLE;
    TextureFormat format = TextureFormat::RGBA8;
    int x = 0, y = 0;
    int width = 0, height = 0;
    void* destination = nullptr;
    size_t destinationSize = 0;
};

// render target configuration
enum class CubemapFace : uint32_t {
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5
};

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

struct RenderTargetAttachment {
    AttachmentType type = AttachmentType::Color0;
    TextureHandle texture = INVALID_HANDLE;
    int mipLevel = 0;
    int arrayLayer = 0; // for texture arrays/cubemaps
};

struct RenderTargetDesc {
    std::vector<RenderTargetAttachment> colorAttachments;
    RenderTargetAttachment depthAttachment;
    int width = 0;
    int height = 0;
    int samples = 1; // msaa samples
    std::string debugName;
};

// render pass description (webgpu-shaped)
enum class LoadOp { Load, Clear };
enum class StoreOp { Store, Discard };

struct ColorAttachmentDesc {
    TextureHandle texture = INVALID_HANDLE;
    glm::vec4 clearColor = glm::vec4(0, 0, 0, 1);
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
};

struct DepthStencilAttachmentDesc {
    TextureHandle texture = INVALID_HANDLE;
    float depthClear = 1.0f;
    uint32_t stencilClear = 0;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
};

struct RenderPassDesc {
    RenderTargetHandle target = INVALID_HANDLE; // if set, overrides attachments below
    std::vector<ColorAttachmentDesc> colorAttachments;
    DepthStencilAttachmentDesc depthStencil;
    int width = 0;
    int height = 0;
    std::string debugName;
};

// uniform buffer ring allocator types
using UniformAllocationHandle = uint32_t;

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

struct UniformVariableInfo {
    std::string name;
    UniformType type;
    uint32_t offset;        // byte offset in uniform buffer
    uint32_t size;          // size in bytes
    uint32_t arraySize;     // 1 for non-arrays
};

struct UniformBlockReflection {
    std::string blockName;
    uint32_t blockSize;     // total size in bytes
    uint32_t binding;       // binding point
    std::vector<UniformVariableInfo> variables;
};

struct ShaderReflection {
    std::vector<UniformBlockReflection> uniformBlocks;
    bool isValid = false;
};

struct UniformAllocationDesc {
    uint32_t size;          // size in bytes
    uint32_t alignment = 16; // required alignment (ubo std140 is 16 bytes)
    const char* debugName = nullptr;
};

struct UniformAllocation {
    UniformAllocationHandle handle = INVALID_HANDLE;
    BufferHandle buffer = INVALID_HANDLE;
    uint32_t offset = 0;    // Offset within the buffer
    void* mappedPtr = nullptr; // CPU-accessible pointer for updates
};

} // namespace glint3d
