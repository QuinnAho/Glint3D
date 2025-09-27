# Render System Decomposition Plan

## Phase 0 � Inventory & Boundaries ✅ COMPLETED
- ✅ Captured current RHI + legacy GL touch points in RenderSystem
- ✅ Documented render pass call graph and ownership of UBO/state
- ✅ Defined data ownership rules before migrating subsystems

## Phase 1 - Core Decomposition ✅ **COMPLETED - ALL MANAGERS IMPLEMENTED**
1. ✅ CameraManager - Already implemented and integrated
   - ✅ Moved camera state, view/projection helpers
   - ✅ Provides viewport resize hooks
2. ✅ LightingManager - **FULLY IMPLEMENTED AND INTEGRATED**
   - ✅ Created manager class with UBO lifecycle (`engine/include/managers/lighting_manager.h`)
   - ✅ Handles LightingBlock UBO allocation and binding point 1
   - ✅ `updateLighting()` extracts real Light properties (type, position, direction, color, intensity)
   - ✅ Converts spot light cone angles from degrees to cosine values
   - ✅ Legacy `m_lightingBlock` removed from RenderSystem
3. ✅ MaterialManager - **FULLY IMPLEMENTED AND INTEGRATED**
   - ✅ Created manager class with UBO lifecycle (`engine/include/managers/material_manager.h`)
   - ✅ Manages MaterialBlock UBO allocation and binding point 2
   - ✅ `updateMaterialForObject()` extracts real MaterialCore from SceneObject
   - ✅ Full PBR workflow connected to actual scene materials (baseColor, metallic, roughness, transmission, etc.)
   - ✅ Legacy `m_materialBlock` removed from RenderSystem
4. ✅ PipelineManager - **COMPLETED**
   - ✅ Centralized shader creation & `ensureObjectPipeline` (`engine/include/managers/pipeline_manager.h`)
   - ✅ RHI and legacy shader management with caching
   - ✅ Pipeline creation based on object vertex attributes
   - ✅ Shader resource management and cleanup
5. ✅ TransformManager - **FULLY IMPLEMENTED AND INTEGRATED**
   - ✅ Created manager class with UBO lifecycle (`engine/include/managers/transform_manager.h`)
   - ✅ Handles TransformBlock UBO allocation and binding point 0
   - ✅ `updateTransforms()` manages model, view, projection matrices with per-object updates
   - ✅ Legacy `m_transformBlock` and `m_transformData` removed from RenderSystem
6. ✅ RenderingManager - **FULLY IMPLEMENTED AND INTEGRATED**
   - ✅ Created manager class with UBO lifecycle (`engine/include/managers/rendering_manager.h`)
   - ✅ Handles RenderingBlock UBO allocation and binding point 3
   - ✅ `updateRenderingState()` manages exposure, gamma, tone mapping, shading mode, IBL intensity
   - ✅ Legacy `m_renderingBlock` and `m_renderingData` removed from RenderSystem
7. ✅ RenderSystem integration - **COMPLETED**
   - ✅ All managers instantiated with proper initialization/shutdown
   - ✅ `bindUniformBlocks()` delegates entirely to managers - no legacy UBO handling
   - ✅ All legacy UBO members removed from RenderSystem header
   - ✅ Static material UBO fallbacks eliminated - all material handling via MaterialManager
   - ✅ Pass methods use RHI abstraction - direct GL state calls removed from `passFrameSetup()`
   - ✅ Core library builds successfully with complete manager architecture
   - ✅ Single UBO system achieved - all UBOs managed exclusively by managers

## Phase 1A � Rendering Entry Cleanup 🚧 **IN PROGRESS**
1. ✅ Reinstate `renderUnified()` built on RenderGraph - **COMPLETED**
   - ✅ `m_rasterGraph`, `m_rayGraph`, and pipeline selector already present
   - ✅ Pass hook methods (`passFrameSetup`, `passRaster`, etc.) signatures updated
   - ✅ `PassContext` built and graph executed each frame
2. 🚧 Remove legacy entry points - **IN PROGRESS**
   - 🚧 Delete `render()` / `renderToTexture()` GL code paths
   - 🚧 Update `ApplicationCore` and other call sites to use `renderUnified()`
3. 🚧 Replace GL per-object helpers - **PENDING**
   - 🚧 Convert selection overlays, gizmo rendering, debug draws to RHI
   - 🚧 Drop VAO/EBO-dependent members from `SceneObject`
4. 🚧 Offscreen rendering - **PENDING**
   - 🚧 Ensure `renderToTextureRHI()` covers PNG/offscreen use cases
   - 🚧 Remove GL fallback logic once verified

### **IMPLEMENTATION STATUS UPDATE:**
- ✅ **MANAGERS FULLY IMPLEMENTED** (`LightingManager`, `MaterialManager`, `PipelineManager`, `TransformManager`, `RenderingManager`)
  - ✅ All manager classes created with proper UBO lifecycle management
  - ✅ Data flow wired for all subsystems - managers process real scene data
  - ✅ TransformManager and RenderingManager: Complete UBO management implementation
- ✅ **MANAGER INTEGRATION COMPLETE**:
  - ✅ All managers instantiated with proper initialization/shutdown
  - ✅ Single UBO system achieved - all UBOs managed exclusively by managers
  - ✅ `bindUniformBlocks()` delegates entirely to managers - no legacy UBO handling
  - ✅ Static material UBO fallbacks eliminated - MaterialManager handles all material operations
- ✅ **PASS SYSTEM MIGRATION CORE COMPLETE**:
  - ✅ `passRaster` calls `renderObjectsBatchedWithManagers()` with functional managers
  - ✅ `passFrameSetup()` uses RHI abstraction - direct GL state calls eliminated
  - ✅ RHI migration for core pass system complete
- ✅ **BUILD VERIFICATION**: Full application builds successfully with complete manager architecture
- ✅ **UBO MANAGEMENT**: Single system - all UBOs managed exclusively by managers
- ✅ **RHI INTEGRATION**: Manager interfaces use RHI abstraction correctly throughout

### **IMMEDIATE NEXT STEPS:**
✅ **Phase 1 Manager Integration - COMPLETED**:
  1. ✅ **TransformManager Created**: Transform UBO management moved to dedicated manager
  2. ✅ **RenderingManager Created**: Rendering state UBO management moved to dedicated manager
  3. ✅ **Static material UBO bypass eliminated**: Removed `s_materialUbo` in `renderObjectFast()`, routed through MaterialManager
  4. ✅ **Pass method RHI migration**: Replaced GL calls in `passFrameSetup()` with RHI abstraction
  5. ✅ **Single UBO system achieved**: All UBOs managed exclusively by managers

🚧 **Phase 1A.2 - Next Priority**: Remove legacy rendering entry points
  1. Delete `render()`, `renderLegacy()`, and legacy `renderToTexture()` methods
  2. Update `ApplicationCore` and other callers to use `renderUnified()` exclusively

🚧 **Phase 1A.3 - Following**: Complete remaining GL elimination
  1. Convert remaining GL calls in object rendering methods to RHI
  2. Eliminate legacy VAO/VBO dependencies in favor of RHI buffer handles

## Phase 2 � RenderGraph & Pass Cleanup
- Decouple `PassContext` from RenderSystem internals
- Merge redundant passes (Raster vs GBuffer)
- Document pass ordering contracts

## Phase 3 � Legacy Removal & Testing
- Eliminate remaining GL fallbacks
- Expand regression coverage for manager workflows
- Audit and remove dead code/material duplicates


## AI-Tunable Renderer Addendum

The current decomposition work is moving you closer to an AI-tunable renderer, but a few structural tweaks will make it friendlier for automated agents and self-improvement loops.

### What�s Already Helpful
- Render graph + `PassContext` (`engine/include/render_pass.h:30-99`) gives you a declarative, inspectable pipeline; AI tooling can reason about passes as discrete nodes and inject instrumentation.
- RHI abstraction (`engine/include/glint3d/rhi.h:28-199`) isolates backends, so agents can swap configuration or simulate runs without diving into raw GL/WebGL.
- Manager split direction (`ai/tasks/render_system_decomposition.md`) recognises the need to decouple state (camera, lighting, materials) from the monolith, a prerequisite for parameter sweeps or targeted tuning.

### Where It Falls Short For AI Loops
- Monolithic `RenderSystem` (`engine/src/render_system.cpp:640-2200`) still hides most decisions and mutable state; an agent can�t easily observe or override per-pass parameters without patching big functions.
- Pass methods are stubs (`engine/src/render_system.cpp:2462-2496`), so the graph is unusable. That forces everything through legacy code paths, blocking fine-grained intervention.
- No explicit parameter surfaces. Lighting/material/raster options live in member variables (e.g., `m_materialData`, `m_renderMode`) scattered through the class. There�s no schema an AI can query/update atomically.
- Telemetry gaps. You collect per-pass timings (`RenderStats::passTimings`), but there�s no structured output for resource usage, shader variants, or error states that an optimisation loop could consume.
- Mixed responsibilities. Resource creation, frame orchestration, and policy (pipeline selection) are all glued together, making it harder to swap in AI-driven strategies.

### Adjustments To Enable AI Accessibility
1. Finish the manager refactor and extend it.
   - Promote `LightingManager`, `MaterialManager`, `PipelineManager` out of the TODO list so each exposes a small, serialisable state object (JSON/protobuf). That becomes the control surface for AI tuning.
   - Keep `RenderSystem` focused on orchestration; managers provide `getState()`/`applyState()` so agents can snapshot, mutate, and roll back experiments safely.
2. Make passes first-class modules.
   - Move the body of `renderRasterized`, `renderDebugElements`, etc., into the corresponding `pass*` methods (`engine/src/render_system.cpp:2482-2496`).
3. Introduce an explicit parameter registry.
   - Add a small `RenderConfig` service exposing a typed map of tweakable values (tone mapping mode, MSAA level, etc.).
4. Standardise telemetry.
   - Extend `RenderStats` (`engine/include/render_system.h:52-70`) with per-pass resource metrics and JSON serialisation. Emit one record per frame and per test scene so an optimiser can score changes automatically.
5. Provide scripting hooks.
   - Once the managers exist, expose them via your existing ops JSON or a lightweight RPC so an AI agent can, for example, set material roughness on object X to 0.3 without recompiling.

These adjustments align with Phases 1�3 above: complete the manager split (Phase 1), migrate real work into pass methods and the RenderGraph (Phase 1A/2), and add the parameter registry + telemetry + scripting surfaces as you remove legacy paths (Phase 2/3).
