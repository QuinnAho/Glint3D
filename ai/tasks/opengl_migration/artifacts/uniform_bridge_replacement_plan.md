# Phase 6 Uniform Bridge Replacement Plan (2025-10-19)

## Objective
Eliminate every `RHI::setUniform*` usage by routing data through uniform blocks or bind groups managed by the existing manager subsystem. This unlocks deletion of `shader.{h,cpp}` and the GL-based bridge inside `RhiGL`.

## Buckets & Owners

| Bucket | Call Sites | Proposed Owner | Notes |
| --- | --- | --- | --- |
| Transform matrices (model/view/projection) | AxisRenderer, Gizmo, Grid, Light indicators, Skybox, IBL passes | `TransformManager` | Already publishes `TransformBlock` UBO; extend usage to debug helpers & IBL capture by exposing scoped update helpers. |
| Debug colors & toggles | Light indicator colors, selection highlight, Skybox gradient toggles, intensity | `RenderingManager` | `RenderingBlock` carries exposure/object color; extend with gradient flags and selection tint. |
| Environment capture params | IBL cube/irradiance/prefilter loops (`environmentMap`, `roughness`) | New `EnvironmentCaptureBlock` allocated per pass | Local helper inside `IBLSystem` to allocate/bind a tiny std140 block instead of scalar uniforms. |
| Legacy bridge forwards (`Shader::setUniform*`) | 6 wrapper methods | Delete with class removal | Goes away once managers own uniform updates and debug shaders use blocks. |

## Design Details

### 1. Transform Updates
- Add `TransformManager::ScopedOverride` (RAII style) that snapshots/restores previous matrices while writing to the shared `TransformBlock`.
- RenderSystem will wrap debug draws (`AxisRenderer`, `Grid`, `Gizmo`, `Light::renderIndicators`, `Skybox::render`) with the scoped override and call `bindTransformUniforms()` once before issuing draw calls.
- Update GLSL for all affected helpers to use:
  ```glsl
  layout(std140, binding = 0) uniform TransformBlock {
      mat4 uModel;
      mat4 uView;
      mat4 uProjection;
  };
  ```
  (Aliases to `TransformBlock` fields; adjust naming to match struct.)

### 2. Rendering Parameters
- Extend `RenderingBlock` with:
  ```cpp
  int useSkyboxGradient;
  glm::vec3 skyboxTopColor;
  glm::vec3 skyboxBottomColor;
  glm::vec3 skyboxHorizonColor;
  glm::vec3 debugHighlightColor;
  ```
  (pad as needed for std140).
- `RenderingManager` gains setters mirroring the new fields and keeps previous values for restoration.
- RenderSystem invokes these setters before debug draws (e.g., selection highlight) and calls `bindRenderingUniforms()` once per pass.
- GLSL for skybox & debug shaders switch to reading from the rendering block instead of standalone uniforms.

### 3. IBL Capture Parameters
- Define `struct EnvironmentCaptureBlock` in `uniform_blocks.h` with projection, face view, roughness, and texture slot index. Assign binding point 4 (next free slot).
- Inside `IBLSystem`, allocate a persistent `UniformAllocation` for this block during `init()`.
- Replace `setUniform*` calls with:
  ```cpp
  m_captureBlockData.projection = captureProjection;
  m_captureBlockData.view = captureViews[face];
  m_captureBlockData.roughness = roughness;
  m_captureBlockData.environmentSlot = Slots::IrradianceMap /* or 0 */;
  m_rhi->updateUniformBlock(m_captureBlockAlloc, m_captureBlockData);
  m_rhi->bindUniformBlock(m_captureBlockAlloc);
  ```
  (API details to follow; see Section 4.)
- Update the GLSL shaders used during capture (`equirectangular_to_cubemap`, `irradiance`, `prefilter`, `brdf`) to consume the new block.

### 4. RHI Support Adjustments
- Introduce lightweight helpers on `RHI` for uniform block staging that all managers can share:
  - `void updateUniformBlock(const UniformAllocation&, const void* data, size_t size);`
  - `void bindUniformBlock(const UniformAllocation&);`
  (Backed by existing `setUniformInBlock`/`setUniformsInBlock` machinery in `RhiGL`.)
- Deprecate and later delete the scalar `setUniform*` methods once every caller migrates.

### 5. Deletion Sequence
1. Implement Transform & Rendering manager integrations + shader updates.
2. Migrate `IBLSystem` to `EnvironmentCaptureBlock`.
3. Remove all residual `m_rhi->setUniform*` calls and the static bridge usage.
4. Delete `shader.h/cpp` and purge `Shader::s_rhi`.
5. Remove `RHI::setUniform*` declarations and definitions (backends + null).
6. Update docs/checklist, run full build, and grep for `setUniform`.

## Acceptance & Validation
- `rg "setUniform" engine/src` returns no matches outside backend-specific UBO helpers (`setUniformInBlock` etc.).
- New uniform blocks documented in `uniform_blocks.h` with Doxygen.
- All touched shaders declare matching `layout(binding = ...)` values and rely on uniform blocks, validated via `glslangValidator`.
- Regression passes:
  - Axis/Gizmo/Grid rendered with correct transforms.
  - Light indicators + selection highlight colors accurate.
  - Skybox gradient toggles behave correctly.
  - IBL bake reproduces existing environment results (visual diff or checksum).

## Open Questions / Follow-Ups
- Confirm binding point availability across backends (WebGL2 cap on UBO bindings). If limited, reuse `RenderingBlock` binding instead of new block.
- Assess whether to cache multiple transform overrides simultaneously (e.g., light selection + gizmo). Scoped RAII approach must support nesting.
- Plan cleanup of any temporary pipelines created for debug draws if they now rely on uniform blocks (ensure they still receive correct data after bridge removal).
