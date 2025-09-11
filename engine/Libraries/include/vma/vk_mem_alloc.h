#pragma once

// VMA - Vulkan Memory Allocator (painless memory management)
// This is a header-only wrapper for Vulkan memory allocation
// Full implementation should be downloaded from https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

#if defined(VK_VERSION_1_0)
#include <vulkan/vulkan.h>
#else
// Define minimal Vulkan types if vulkan.h is not available
typedef struct VkDevice_T* VkDevice;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkInstance_T* VkInstance;
typedef struct VkBuffer_T* VkBuffer;
typedef struct VkImage_T* VkImage;
typedef struct VkDeviceMemory_T* VkDeviceMemory;
typedef uint32_t VkFlags;
typedef int VkResult;
typedef uint64_t VkDeviceSize;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// VMA handles
typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;
typedef struct VmaPool_T* VmaPool;

// VMA result codes
typedef enum VmaResult {
    VMA_SUCCESS = 0,
    VMA_ERROR_FEATURE_NOT_PRESENT = -1,
    VMA_ERROR_OUT_OF_HOST_MEMORY = -2,
    VMA_ERROR_OUT_OF_DEVICE_MEMORY = -3,
    VMA_ERROR_INITIALIZATION_FAILED = -4,
    VMA_ERROR_LAYER_NOT_PRESENT = -5,
    VMA_ERROR_EXTENSION_NOT_PRESENT = -6,
    VMA_ERROR_INCOMPATIBLE_DRIVER = -7,
    VMA_ERROR_TOO_MANY_OBJECTS = -8,
    VMA_ERROR_FORMAT_NOT_SUPPORTED = -9,
    VMA_ERROR_FRAGMENTED_POOL = -10,
    VMA_ERROR_UNKNOWN = -11
} VmaResult;

// Memory usage types
typedef enum VmaMemoryUsage {
    VMA_MEMORY_USAGE_UNKNOWN = 0,
    VMA_MEMORY_USAGE_GPU_ONLY = 1,
    VMA_MEMORY_USAGE_CPU_ONLY = 2,
    VMA_MEMORY_USAGE_CPU_TO_GPU = 3,
    VMA_MEMORY_USAGE_GPU_TO_CPU = 4,
    VMA_MEMORY_USAGE_CPU_COPY = 5,
    VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED = 6,
    VMA_MEMORY_USAGE_AUTO = 7,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE = 8,
    VMA_MEMORY_USAGE_AUTO_PREFER_HOST = 9
} VmaMemoryUsage;

// Allocation flags
typedef enum VmaAllocationCreateFlagBits {
    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT = 0x00000001,
    VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT = 0x00000002,
    VMA_ALLOCATION_CREATE_MAPPED_BIT = 0x00000004,
    VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT = 0x00000020,
    VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT = 0x00000040,
    VMA_ALLOCATION_CREATE_DONT_BIND_BIT = 0x00000080,
    VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT = 0x00000100,
    VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT = 0x00000200,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT = 0x00000400,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT = 0x00000800,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT = 0x00001000,
    VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT = 0x00010000,
    VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT = 0x00020000,
    VMA_ALLOCATION_CREATE_STRATEGY_MIN_OFFSET_BIT = 0x00040000,
    VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT,
    VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT = VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT,
    VMA_ALLOCATION_CREATE_STRATEGY_MASK = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_OFFSET_BIT
} VmaAllocationCreateFlagBits;

typedef VkFlags VmaAllocationCreateFlags;

// Allocator creation info
typedef struct VmaAllocatorCreateInfo {
    VmaAllocationCreateFlags flags;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkDeviceSize preferredLargeHeapBlockSize;
    const VkAllocationCallbacks* pAllocationCallbacks;
    const void* pDeviceMemoryCallbacks;
    const VkDeviceSize* pHeapSizeLimit;
    const void* pVulkanFunctions;
    VkInstance instance;
    uint32_t vulkanApiVersion;
    const void* pTypeExternalMemoryHandleTypes;
} VmaAllocatorCreateInfo;

// Allocation creation info
typedef struct VmaAllocationCreateInfo {
    VmaAllocationCreateFlags flags;
    VmaMemoryUsage usage;
    VkFlags requiredFlags;
    VkFlags preferredFlags;
    uint32_t memoryTypeBits;
    VmaPool pool;
    void* pUserData;
    float priority;
} VmaAllocationCreateInfo;

// Allocation info
typedef struct VmaAllocationInfo {
    uint32_t memoryType;
    VkDeviceMemory deviceMemory;
    VkDeviceSize offset;
    VkDeviceSize size;
    void* pMappedData;
    void* pUserData;
    const char* pName;
} VmaAllocationInfo;

// Function prototypes (placeholder implementations)
VmaResult vmaCreateAllocator(
    const VmaAllocatorCreateInfo* pCreateInfo,
    VmaAllocator* pAllocator);

void vmaDestroyAllocator(VmaAllocator allocator);

VmaResult vmaCreateBuffer(
    VmaAllocator allocator,
    const void* pBufferCreateInfo, // VkBufferCreateInfo*
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

void vmaDestroyBuffer(
    VmaAllocator allocator,
    VkBuffer buffer,
    VmaAllocation allocation);

VmaResult vmaCreateImage(
    VmaAllocator allocator,
    const void* pImageCreateInfo, // VkImageCreateInfo*
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkImage* pImage,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

void vmaDestroyImage(
    VmaAllocator allocator,
    VkImage image,
    VmaAllocation allocation);

VmaResult vmaMapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation,
    void** ppData);

void vmaUnmapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation);

VmaResult vmaFlushAllocation(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VkDeviceSize offset,
    VkDeviceSize size);

VmaResult vmaInvalidateAllocation(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VkDeviceSize offset,
    VkDeviceSize size);

void vmaGetAllocationInfo(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VmaAllocationInfo* pAllocationInfo);

void vmaSetAllocationUserData(
    VmaAllocator allocator,
    VmaAllocation allocation,
    void* pUserData);

void vmaSetAllocationName(
    VmaAllocator allocator,
    VmaAllocation allocation,
    const char* pName);

#ifdef __cplusplus
}
#endif

// Note: This is a minimal placeholder implementation
// For production use, download the full VMA library from:
// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
// Or install via vcpkg: vcpkg install vulkan-memory-allocator