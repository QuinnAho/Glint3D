# Render System Decomposition Plan

## Phase 0 – Inventory & Boundaries
- Capture current RHI + legacy GL touch points in RenderSystem
- Document render pass call graph and ownership of UBO/state
- Define data ownership rules before migrating subsystems

## Phase 1 – Core Decomposition
1. CameraManager
   - Move camera state, view/projection helpers
   - Provide viewport resize hooks
2. LightingManager
   - Encapsulate lighting UBO + update routines
3. MaterialManager
   - Handle material UBOs & per-object uploads
4. Shader/Pipeline Manager
   - Centralise shader creation & `ensureObjectPipeline`
5. RenderSystem integration
   - Replace legacy members with manager interfaces
   - Update passes to consume managers

## Phase 1A – Rendering Entry Cleanup
1. Reinstate `renderUnified()` built on RenderGraph
   - Reintroduce `m_rasterGraph`, `m_rayGraph`, and pipeline selector
   - Implement pass hook methods (`passFrameSetup`, `passRaster`, …)
   - Build `PassContext` and execute the graph each frame
2. Remove legacy entry points
   - Delete `render()` / `renderToTexture()` GL code paths
   - Update `ApplicationCore` and other call sites to use `renderUnified()`
3. Replace GL per-object helpers
   - Convert selection overlays, gizmo rendering, debug draws to RHI
   - Drop VAO/EBO-dependent members from `SceneObject`
4. Offscreen rendering
   - Ensure `renderToTextureRHI()` covers PNG/offscreen use cases
   - Remove GL fallback logic once verified

## Phase 2 – RenderGraph & Pass Cleanup
- Decouple `PassContext` from RenderSystem internals
- Merge redundant passes (Raster vs GBuffer)
- Document pass ordering contracts

## Phase 3 – Legacy Removal & Testing
- Eliminate remaining GL fallbacks
- Expand regression coverage for manager workflows
- Audit and remove dead code/material duplicates


## AI-Tunable Renderer Addendum

The current decomposition work is moving you closer to an AI-tunable renderer, but a few structural tweaks will make it friendlier for automated agents and self-improvement loops.

### What’s Already Helpful
- Render graph + `PassContext` (`engine/include/render_pass.h:30-99`) gives you a declarative, inspectable pipeline; AI tooling can reason about passes as discrete nodes and inject instrumentation.
- RHI abstraction (`engine/include/glint3d/rhi.h:28-199`) isolates backends, so agents can swap configuration or simulate runs without diving into raw GL/WebGL.
- Manager split direction (`ai/tasks/render_system_decomposition.md`) recognises the need to decouple state (camera, lighting, materials) from the monolith, a prerequisite for parameter sweeps or targeted tuning.

### Where It Falls Short For AI Loops
- Monolithic `RenderSystem` (`engine/src/render_system.cpp:640-2200`) still hides most decisions and mutable state; an agent can’t easily observe or override per-pass parameters without patching big functions.
- Pass methods are stubs (`engine/src/render_system.cpp:2462-2496`), so the graph is unusable. That forces everything through legacy code paths, blocking fine-grained intervention.
- No explicit parameter surfaces. Lighting/material/raster options live in member variables (e.g., `m_materialData`, `m_renderMode`) scattered through the class. There’s no schema an AI can query/update atomically.
- Telemetry gaps. You collect per-pass timings (`RenderStats::passTimings`), but there’s no structured output for resource usage, shader variants, or error states that an optimisation loop could consume.
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

These adjustments align with Phases 1–3 above: complete the manager split (Phase 1), migrate real work into pass methods and the RenderGraph (Phase 1A/2), and add the parameter registry + telemetry + scripting surfaces as you remove legacy paths (Phase 2/3).
