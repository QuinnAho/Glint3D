#pragma once

// WebGPU - Next-generation web graphics API
// This is a header-only wrapper for WebGPU functionality
// Full implementation should be downloaded from https://github.com/gfx-rs/wgpu-native or Dawn

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct WGPUInstanceImpl* WGPUInstance;
typedef struct WGPUAdapterImpl* WGPUAdapter;
typedef struct WGPUDeviceImpl* WGPUDevice;
typedef struct WGPUQueueImpl* WGPUQueue;
typedef struct WGPUBufferImpl* WGPUBuffer;
typedef struct WGPUTextureImpl* WGPUTexture;
typedef struct WGPUTextureViewImpl* WGPUTextureView;
typedef struct WGPUSamplerImpl* WGPUSampler;
typedef struct WGPUBindGroupImpl* WGPUBindGroup;
typedef struct WGPUBindGroupLayoutImpl* WGPUBindGroupLayout;
typedef struct WGPUPipelineLayoutImpl* WGPUPipelineLayout;
typedef struct WGPURenderPipelineImpl* WGPURenderPipeline;
typedef struct WGPUComputePipelineImpl* WGPUComputePipeline;
typedef struct WGPUCommandEncoderImpl* WGPUCommandEncoder;
typedef struct WGPUCommandBufferImpl* WGPUCommandBuffer;
typedef struct WGPURenderPassEncoderImpl* WGPURenderPassEncoder;
typedef struct WGPUComputePassEncoderImpl* WGPUComputePassEncoder;
typedef struct WGPUShaderModuleImpl* WGPUShaderModule;
typedef struct WGPUSurfaceImpl* WGPUSurface;
typedef struct WGPUSwapChainImpl* WGPUSwapChain;

// Enums
typedef enum WGPUAdapterType {
    WGPUAdapterType_DiscreteGPU = 0x00000000,
    WGPUAdapterType_IntegratedGPU = 0x00000001,
    WGPUAdapterType_CPU = 0x00000002,
    WGPUAdapterType_Unknown = 0x00000003
} WGPUAdapterType;

typedef enum WGPUBackendType {
    WGPUBackendType_Null = 0x00000000,
    WGPUBackendType_WebGPU = 0x00000001,
    WGPUBackendType_D3D11 = 0x00000002,
    WGPUBackendType_D3D12 = 0x00000003,
    WGPUBackendType_Metal = 0x00000004,
    WGPUBackendType_Vulkan = 0x00000005,
    WGPUBackendType_OpenGL = 0x00000006,
    WGPUBackendType_OpenGLES = 0x00000007
} WGPUBackendType;

typedef enum WGPUBufferUsage {
    WGPUBufferUsage_None = 0x00000000,
    WGPUBufferUsage_MapRead = 0x00000001,
    WGPUBufferUsage_MapWrite = 0x00000002,
    WGPUBufferUsage_CopySrc = 0x00000004,
    WGPUBufferUsage_CopyDst = 0x00000008,
    WGPUBufferUsage_Index = 0x00000010,
    WGPUBufferUsage_Vertex = 0x00000020,
    WGPUBufferUsage_Uniform = 0x00000040,
    WGPUBufferUsage_Storage = 0x00000080,
    WGPUBufferUsage_Indirect = 0x00000100,
    WGPUBufferUsage_QueryResolve = 0x00000200
} WGPUBufferUsage;

typedef enum WGPUTextureUsage {
    WGPUTextureUsage_None = 0x00000000,
    WGPUTextureUsage_CopySrc = 0x00000001,
    WGPUTextureUsage_CopyDst = 0x00000002,
    WGPUTextureUsage_TextureBinding = 0x00000004,
    WGPUTextureUsage_StorageBinding = 0x00000008,
    WGPUTextureUsage_RenderAttachment = 0x00000010
} WGPUTextureUsage;

typedef enum WGPUTextureFormat {
    WGPUTextureFormat_Undefined = 0x00000000,
    WGPUTextureFormat_R8Unorm = 0x00000001,
    WGPUTextureFormat_R8Snorm = 0x00000002,
    WGPUTextureFormat_R8Uint = 0x00000003,
    WGPUTextureFormat_R8Sint = 0x00000004,
    WGPUTextureFormat_R16Uint = 0x00000005,
    WGPUTextureFormat_R16Sint = 0x00000006,
    WGPUTextureFormat_R16Float = 0x00000007,
    WGPUTextureFormat_RG8Unorm = 0x00000008,
    WGPUTextureFormat_RG8Snorm = 0x00000009,
    WGPUTextureFormat_RG8Uint = 0x0000000A,
    WGPUTextureFormat_RG8Sint = 0x0000000B,
    WGPUTextureFormat_R32Float = 0x0000000C,
    WGPUTextureFormat_R32Uint = 0x0000000D,
    WGPUTextureFormat_R32Sint = 0x0000000E,
    WGPUTextureFormat_RG16Uint = 0x0000000F,
    WGPUTextureFormat_RG16Sint = 0x00000010,
    WGPUTextureFormat_RG16Float = 0x00000011,
    WGPUTextureFormat_RGBA8Unorm = 0x00000012,
    WGPUTextureFormat_RGBA8UnormSrgb = 0x00000013,
    WGPUTextureFormat_RGBA8Snorm = 0x00000014,
    WGPUTextureFormat_RGBA8Uint = 0x00000015,
    WGPUTextureFormat_RGBA8Sint = 0x00000016,
    WGPUTextureFormat_BGRA8Unorm = 0x00000017,
    WGPUTextureFormat_BGRA8UnormSrgb = 0x00000018,
    WGPUTextureFormat_RGB10A2Unorm = 0x00000019,
    WGPUTextureFormat_RG11B10Ufloat = 0x0000001A,
    WGPUTextureFormat_RGB9E5Ufloat = 0x0000001B,
    WGPUTextureFormat_RG32Float = 0x0000001C,
    WGPUTextureFormat_RG32Uint = 0x0000001D,
    WGPUTextureFormat_RG32Sint = 0x0000001E,
    WGPUTextureFormat_RGBA16Uint = 0x0000001F,
    WGPUTextureFormat_RGBA16Sint = 0x00000020,
    WGPUTextureFormat_RGBA16Float = 0x00000021,
    WGPUTextureFormat_RGBA32Float = 0x00000022,
    WGPUTextureFormat_RGBA32Uint = 0x00000023,
    WGPUTextureFormat_RGBA32Sint = 0x00000024,
    WGPUTextureFormat_Depth32Float = 0x00000025,
    WGPUTextureFormat_Depth24Plus = 0x00000026,
    WGPUTextureFormat_Depth24PlusStencil8 = 0x00000027
} WGPUTextureFormat;

typedef enum WGPUPresentMode {
    WGPUPresentMode_Immediate = 0x00000000,
    WGPUPresentMode_Mailbox = 0x00000001,
    WGPUPresentMode_Fifo = 0x00000002
} WGPUPresentMode;

// Structures
typedef struct WGPUExtent3D {
    uint32_t width;
    uint32_t height;
    uint32_t depthOrArrayLayers;
} WGPUExtent3D;

typedef struct WGPULimits {
    uint32_t maxTextureDimension1D;
    uint32_t maxTextureDimension2D;
    uint32_t maxTextureDimension3D;
    uint32_t maxTextureArrayLayers;
    uint32_t maxBindGroups;
    uint32_t maxDynamicUniformBuffersPerPipelineLayout;
    uint32_t maxDynamicStorageBuffersPerPipelineLayout;
    uint32_t maxSampledTexturesPerShaderStage;
    uint32_t maxSamplersPerShaderStage;
    uint32_t maxStorageBuffersPerShaderStage;
    uint32_t maxStorageTexturesPerShaderStage;
    uint32_t maxUniformBuffersPerShaderStage;
    uint64_t maxUniformBufferBindingSize;
    uint64_t maxStorageBufferBindingSize;
    uint32_t minUniformBufferOffsetAlignment;
    uint32_t minStorageBufferOffsetAlignment;
    uint32_t maxVertexBuffers;
    uint32_t maxVertexAttributes;
    uint32_t maxVertexBufferArrayStride;
    uint32_t maxInterStageShaderComponents;
    uint32_t maxComputeWorkgroupStorageSize;
    uint32_t maxComputeInvocationsPerWorkgroup;
    uint32_t maxComputeWorkgroupSizeX;
    uint32_t maxComputeWorkgroupSizeY;
    uint32_t maxComputeWorkgroupSizeZ;
    uint32_t maxComputeWorkgroupsPerDimension;
} WGPULimits;

typedef struct WGPUBufferDescriptor {
    const char* label;
    WGPUBufferUsage usage;
    uint64_t size;
    bool mappedAtCreation;
} WGPUBufferDescriptor;

typedef struct WGPUTextureDescriptor {
    const char* label;
    WGPUTextureUsage usage;
    WGPUExtent3D size;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    WGPUTextureFormat format;
} WGPUTextureDescriptor;

// Callback types
typedef void (*WGPUBufferMapCallback)(int status, void* userdata);
typedef void (*WGPUDeviceLostCallback)(int reason, const char* message, void* userdata);
typedef void (*WGPUErrorCallback)(int type, const char* message, void* userdata);

// Function prototypes (placeholder implementations)
WGPUInstance wgpuCreateInstance(const void* descriptor);
void wgpuInstanceRelease(WGPUInstance instance);

void wgpuInstanceRequestAdapter(WGPUInstance instance, const void* options, void* callback, void* userdata);

WGPUDevice wgpuAdapterRequestDevice(WGPUAdapter adapter, const void* descriptor);
void wgpuDeviceRelease(WGPUDevice device);

WGPUQueue wgpuDeviceGetQueue(WGPUDevice device);
void wgpuQueueRelease(WGPUQueue queue);

WGPUBuffer wgpuDeviceCreateBuffer(WGPUDevice device, const WGPUBufferDescriptor* descriptor);
void wgpuBufferRelease(WGPUBuffer buffer);

WGPUTexture wgpuDeviceCreateTexture(WGPUDevice device, const WGPUTextureDescriptor* descriptor);
void wgpuTextureRelease(WGPUTexture texture);

WGPUCommandEncoder wgpuDeviceCreateCommandEncoder(WGPUDevice device, const void* descriptor);
WGPUCommandBuffer wgpuCommandEncoderFinish(WGPUCommandEncoder encoder, const void* descriptor);
void wgpuCommandEncoderRelease(WGPUCommandEncoder encoder);
void wgpuCommandBufferRelease(WGPUCommandBuffer commandBuffer);

void wgpuQueueSubmit(WGPUQueue queue, uint32_t commandCount, const WGPUCommandBuffer* commands);

// Buffer operations
void wgpuBufferMapAsync(WGPUBuffer buffer, uint32_t mode, size_t offset, size_t size, WGPUBufferMapCallback callback, void* userdata);
void* wgpuBufferGetMappedRange(WGPUBuffer buffer, size_t offset, size_t size);
void wgpuBufferUnmap(WGPUBuffer buffer);

#ifdef __cplusplus
}
#endif

// Note: This is a minimal placeholder implementation
// For production use, download a full WebGPU implementation:
// - Dawn: https://github.com/google/dawn
// - wgpu-native: https://github.com/gfx-rs/wgpu-native
// Or install via vcpkg: vcpkg install wgpu-native