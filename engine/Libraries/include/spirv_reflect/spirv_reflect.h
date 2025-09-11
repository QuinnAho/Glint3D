#pragma once

// SPIRV-Reflect - Automatic UBO layout detection and binding inference
// This is a header-only wrapper for basic reflection functionality
// Full implementation should be downloaded from https://github.com/KhronosGroup/SPIRV-Reflect

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Result codes
typedef enum SpvReflectResult {
    SPV_REFLECT_RESULT_SUCCESS = 0,
    SPV_REFLECT_RESULT_NOT_READY = 1,
    SPV_REFLECT_RESULT_ERROR_PARSE_FAILED = -1,
    SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED = -2,
    SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED = -3,
    SPV_REFLECT_RESULT_ERROR_NULL_POINTER = -4,
    SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR = -5,
    SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH = -6,
    SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND = -7,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE = -8,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER = -9,
    SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF = -10,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE = -11,
    SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_DUPLICATE = -12,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS = -13,
    SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION = -14,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION = -15,
    SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA = -16,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE = -17,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT = -18,
    SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE = -19
} SpvReflectResult;

// Storage classes
typedef enum SpvReflectStorageClass {
    SPV_REFLECT_STORAGE_CLASS_UNDEFINED = 0,
    SPV_REFLECT_STORAGE_CLASS_UNIFORM_CONSTANT = 1,
    SPV_REFLECT_STORAGE_CLASS_INPUT = 2,
    SPV_REFLECT_STORAGE_CLASS_UNIFORM = 3,
    SPV_REFLECT_STORAGE_CLASS_OUTPUT = 4,
    SPV_REFLECT_STORAGE_CLASS_WORKGROUP = 5,
    SPV_REFLECT_STORAGE_CLASS_CROSS_WORKGROUP = 6,
    SPV_REFLECT_STORAGE_CLASS_PRIVATE = 7,
    SPV_REFLECT_STORAGE_CLASS_FUNCTION = 8,
    SPV_REFLECT_STORAGE_CLASS_GENERIC = 9,
    SPV_REFLECT_STORAGE_CLASS_PUSH_CONSTANT = 10,
    SPV_REFLECT_STORAGE_CLASS_ATOMIC_COUNTER = 11,
    SPV_REFLECT_STORAGE_CLASS_IMAGE = 12,
    SPV_REFLECT_STORAGE_CLASS_STORAGE_BUFFER = 13
} SpvReflectStorageClass;

// Descriptor types
typedef enum SpvReflectDescriptorType {
    SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER = 0,
    SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 11
} SpvReflectDescriptorType;

// Shader stage flags
typedef enum SpvReflectShaderStageFlagBits {
    SPV_REFLECT_SHADER_STAGE_VERTEX_BIT = 0x00000001,
    SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000002,
    SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
    SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
    SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
    SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT = 0x00000020
} SpvReflectShaderStageFlagBits;

// Forward declarations
typedef struct SpvReflectShaderModule SpvReflectShaderModule;
typedef struct SpvReflectDescriptorBinding SpvReflectDescriptorBinding;
typedef struct SpvReflectDescriptorSet SpvReflectDescriptorSet;
typedef struct SpvReflectInterfaceVariable SpvReflectInterfaceVariable;
typedef struct SpvReflectBlockVariable SpvReflectBlockVariable;

// Descriptor binding structure
typedef struct SpvReflectDescriptorBinding {
    uint32_t spirv_id;
    const char* name;
    uint32_t binding;
    uint32_t set;
    SpvReflectDescriptorType descriptor_type;
    SpvReflectShaderStageFlagBits stage_flags;
    
    union {
        struct {
            uint32_t binding_count;
            SpvReflectDescriptorBinding** bindings;
        } array;
    };
    
    struct {
        uint32_t size;
        uint32_t padded_size;
    } block;
    
    uint32_t count;
    uint32_t accessed;
    uint32_t uav_counter_id;
    
    SpvReflectBlockVariable* type_description;
} SpvReflectDescriptorBinding;

// Descriptor set structure
typedef struct SpvReflectDescriptorSet {
    uint32_t set;
    uint32_t binding_count;
    SpvReflectDescriptorBinding** bindings;
} SpvReflectDescriptorSet;

// Interface variable structure
typedef struct SpvReflectInterfaceVariable {
    uint32_t spirv_id;
    const char* name;
    uint32_t location;
    uint32_t component;
    SpvReflectStorageClass storage_class;
    const char* semantic;
    uint32_t decoration_flags;
    uint32_t built_in;
    
    struct {
        uint32_t id;
        uint32_t type_id;
    } numeric;
    
    struct {
        uint32_t location_count;
        uint32_t component_count;
    } array;
    
    uint32_t member_count;
    struct SpvReflectInterfaceVariable** members;
    
    SpvReflectBlockVariable* type_description;
    
    struct {
        const char* word;
    } word_offset;
} SpvReflectInterfaceVariable;

// Shader module structure (simplified)
typedef struct SpvReflectShaderModule {
    uint32_t generator;
    const char* entry_point_name;
    uint32_t entry_point_id;
    SpvReflectShaderStageFlagBits shader_stage;
    uint32_t descriptor_set_count;
    SpvReflectDescriptorSet* descriptor_sets;
    uint32_t input_variable_count;
    SpvReflectInterfaceVariable** input_variables;
    uint32_t output_variable_count;
    SpvReflectInterfaceVariable** output_variables;
    uint32_t push_constant_block_count;
    SpvReflectBlockVariable** push_constant_blocks;
} SpvReflectShaderModule;

// Main API functions (placeholder implementations)
SpvReflectResult spvReflectCreateShaderModule(
    size_t size,
    const void* p_code,
    SpvReflectShaderModule* p_module);

void spvReflectDestroyShaderModule(SpvReflectShaderModule* p_module);

SpvReflectResult spvReflectEnumerateDescriptorSets(
    const SpvReflectShaderModule* p_module,
    uint32_t* p_count,
    SpvReflectDescriptorSet** pp_sets);

SpvReflectResult spvReflectEnumerateDescriptorBindings(
    const SpvReflectShaderModule* p_module,
    uint32_t* p_count,
    SpvReflectDescriptorBinding** pp_bindings);

SpvReflectResult spvReflectEnumerateInputVariables(
    const SpvReflectShaderModule* p_module,
    uint32_t* p_count,
    SpvReflectInterfaceVariable** pp_variables);

SpvReflectResult spvReflectEnumerateOutputVariables(
    const SpvReflectShaderModule* p_module,
    uint32_t* p_count,
    SpvReflectInterfaceVariable** pp_variables);

SpvReflectDescriptorBinding* spvReflectGetDescriptorBinding(
    const SpvReflectShaderModule* p_module,
    uint32_t binding_number,
    uint32_t set_number,
    SpvReflectResult* p_result);

SpvReflectInterfaceVariable* spvReflectGetInputVariable(
    const SpvReflectShaderModule* p_module,
    uint32_t location,
    SpvReflectResult* p_result);

SpvReflectInterfaceVariable* spvReflectGetOutputVariable(
    const SpvReflectShaderModule* p_module,
    uint32_t location,
    SpvReflectResult* p_result);

const char* spvReflectResultToString(SpvReflectResult result);

#ifdef __cplusplus
}
#endif

// Note: This is a minimal placeholder implementation
// For production use, download the full SPIRV-Reflect library from:
// https://github.com/KhronosGroup/SPIRV-Reflect
// Or install via vcpkg: vcpkg install spirv-reflect