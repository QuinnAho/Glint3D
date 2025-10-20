// Machine Summary Block (ndjson)
// {"file":"engine/include/glint3d/texture_slots.h","purpose":"Defines canonical texture unit bindings shared across render passes","exports":["glint3d::TextureSlots"],"depends_on":[],"notes":["Keep bindings consistent with GLSL layout(binding) qualifiers","Used by forward, deferred, and debug rendering paths"]}
#pragma once

/**
 * @file texture_slots.h
 * @brief Centralized texture unit bindings used by the RHI-based renderer.
 *
 * These constants document and enforce the texture unit layout expected by the
 * GLSL shaders via layout(binding = N) qualifiers. Keeping them in one place
 * prevents drift between CPU binding code and shader expectations.
 */

#include <cstdint>

namespace glint3d::TextureSlots {

// Forward PBR rendering (shared with material manager)
inline constexpr uint32_t BaseColor           = 0;
inline constexpr uint32_t Normal              = 1;
inline constexpr uint32_t MetallicRoughness   = 2;
inline constexpr uint32_t ShadowMap           = 3;

// Image-based lighting resources
inline constexpr uint32_t IrradianceMap       = 4;
inline constexpr uint32_t PrefilterMap        = 5;
inline constexpr uint32_t BrdfLut             = 6;

// Post-process / screen-quad render targets
inline constexpr uint32_t RaytracedOutput     = 7;

// Deferred shading G-buffer inputs (reuse low slots for performance)
inline constexpr uint32_t GBufferBaseColor    = 0;
inline constexpr uint32_t GBufferNormal       = 1;
inline constexpr uint32_t GBufferPosition     = 2;
inline constexpr uint32_t GBufferMaterial     = 3;

} // namespace glint3d::TextureSlots

