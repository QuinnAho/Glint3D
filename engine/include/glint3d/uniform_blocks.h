#pragma once

#include <glm/glm.hpp>
#include <glint3d/rhi_types.h>

namespace glint3d {

/**
 * @brief Standard uniform block structures for UBO ring allocator system
 *
 * These structures match the layout(std140) uniform blocks in shaders.
 * All blocks are padded to 16-byte alignment as required by std140.
 */

// Transform matrices (used by all vertex shaders)
struct TransformBlock {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 lightSpaceMatrix;  // For shadow mapping

    static constexpr const char* BLOCK_NAME = "TransformBlock";
    static constexpr uint32_t BINDING_POINT = 0;
};

// Light data structure (matches shader Light struct)
struct LightData {
    int type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float innerCutoff;  // cos(inner)
    float outerCutoff;  // cos(outer)
    float _padding;     // std140 alignment
};

// Lighting uniform block (used by fragment shaders)
struct LightingBlock {
    int numLights;
    float _padding1[3];  // vec3 alignment
    glm::vec3 viewPos;
    float _padding2;     // vec4 alignment
    glm::vec4 globalAmbient;
    LightData lights[10]; // MAX_LIGHTS = 10

    static constexpr const char* BLOCK_NAME = "LightingBlock";
    static constexpr uint32_t BINDING_POINT = 1;
};

// Material properties (used by PBR fragment shader)
struct MaterialBlock {
    glm::vec4 baseColorFactor;  // rgba
    float metallicFactor;
    float roughnessFactor;
    float ior;              // Index of refraction
    float transmission;     // [0,1]
    float thickness;        // meters
    float attenuationDistance; // meters (Beer-Lambert)
    float _padding1[2];     // vec3 alignment
    glm::vec3 attenuationColor; // tint for attenuation
    float clearcoat;        // [0,1]
    float clearcoatRoughness; // [0,1]
    float _padding2[3];     // std140 alignment padding

    static constexpr const char* BLOCK_NAME = "MaterialBlock";
    static constexpr uint32_t BINDING_POINT = 2;
};

/**
 * @brief Helper functions for working with uniform blocks
 */
namespace UniformBlocks {

/**
 * @brief Calculate the std140 layout size of a uniform block
 * @tparam T Uniform block type
 * @return Size in bytes with std140 padding
 */
template<typename T>
constexpr uint32_t getBlockSize() {
    return sizeof(T);
}

/**
 * @brief Get the binding point for a uniform block type
 * @tparam T Uniform block type
 * @return Binding point index
 */
template<typename T>
constexpr uint32_t getBindingPoint() {
    return T::BINDING_POINT;
}

/**
 * @brief Get the block name for a uniform block type
 * @tparam T Uniform block type
 * @return Block name string
 */
template<typename T>
constexpr const char* getBlockName() {
    return T::BLOCK_NAME;
}

/**
 * @brief Allocate and set uniforms for a typed uniform block
 * @tparam T Uniform block type
 * @param rhi RHI instance
 * @param shader Shader handle
 * @param data Uniform block data
 * @return Uniform allocation handle
 */
template<typename T>
UniformAllocation allocateAndSetBlock(RHI* rhi, ShaderHandle shader, const T& data) {
    UniformAllocationDesc desc{};
    desc.size = getBlockSize<T>();
    desc.alignment = 256; // UBO alignment
    desc.debugName = getBlockName<T>();

    auto allocation = rhi->allocateUniforms(desc);
    if (allocation.handle != INVALID_HANDLE && allocation.mappedPtr) {
        memcpy(allocation.mappedPtr, &data, sizeof(T));

        // Bind the UBO to the appropriate binding point
        rhi->bindUniformBuffer(allocation.buffer, getBindingPoint<T>());
    }

    return allocation;
}

} // namespace UniformBlocks

} // namespace glint3d