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

