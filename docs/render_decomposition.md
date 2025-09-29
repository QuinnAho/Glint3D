# Render System Decomposition Audit

This document captures the current state of the rendering pipeline and identifies opportunities for modularization, telemetry, and capability-driven execution.

## Executive Summary

The current rendering system uses a unified path through `RenderSystem::renderUnified()` with pass-based architecture via `RenderGraph`. The audit reveals clear surfaces for AI control and identifies key technical debt that needs addressing for full modularity.

## Go / No-Go Plan

| Stage | Function / Pass | File:line | Notes |
|-------|----------------|-----------|-------|
| Render entry | RenderSystem::render | engine/src/render_system.cpp:342 | Public entry point; immediately dispatches to renderUnified. |
| Unified path | RenderSystem::renderUnified | engine/src/render_system.cpp:347 | Builds per-frame state, selects pipeline, executes render graph. |
| Graph setup | RenderGraph::setup | engine/src/render_pass.cpp:68 | Materialises pass sequence; failure drops to legacy path. |
| Graph execute | RenderGraph::execute | engine/src/render_pass.cpp:87 | Iterates passes, calling executeWithTiming. |
| Frame prep pass | FrameSetupPass::execute → RenderSystem::passFrameSetup | engine/src/render_pass.cpp:165 → engine/src/render_system.cpp:2157 | Clears buffers, updates managers, rebinds UBOs. |
| Raster pass | RasterPass::execute → RenderSystem::passRaster | engine/src/render_pass.cpp:176 → engine/src/render_system.cpp:2178 | Binds manager UBOs, renders skybox, then renderObjectsBatchedWithManagers. |
| Object batch | RenderSystem::renderObjectsBatchedWithManagers | engine/src/render_system.cpp:2389 | Per-object pipeline ensure, direct RHI uniform calls, stats accumulation. |
| Ray pass | RaytracePass::execute → RenderSystem::passRaytrace | engine/src/render_pass.cpp:188 → engine/src/render_system.cpp:2199 | Rebuilds raytracer, uploads texture via GL fallback. |
| Overlays | OverlayPass::execute → RenderSystem::passOverlays | engine/src/render_pass.cpp:256 → engine/src/render_system.cpp:2319 | Renders grid, axes, lights. |
| Resolve / present / readback | ResolvePass, PresentPass, ReadbackPass → RenderSystem::pass* | engine/src/render_pass.cpp:268/279/300 → engine/src/render_system.cpp:2348/2357/2367 | Currently stubs; rely on implicit GL behaviour. |
| Legacy fallback | RenderSystem::renderLegacy | engine/src/render_system.cpp:425 | Hands-on GL path, still invoked if graph setup fails. |

## Surface Audit for AI Control

### Camera (Status: Green ✅)
- **Data**: CameraState struct owned by CameraManager (`engine/include/managers/camera_manager.h:6-23`)
- **Mutators**: setCamera, updateViewMatrix, updateProjectionMatrix, setProjectionMatrix (`engine/src/managers/camera_manager.cpp:11-48`)
- **RHI/GL usage**: None; matrices consumed by TransformManager/RenderSystem
- **Snapshot/apply**: Expose CameraState snapshotCamera() and applyCamera(const CameraState&)

**JSON Schema:**
```json
{
  "cameraState": {
    "position": [float, float, float],
    "front": [float, float, float],
    "up": [float, float, float],
    "fov": float,
    "near": float,
    "far": float
  }
}
```

### Lighting (Status: Yellow ⚠️)
- **Data**: LightingBlock m_lightingData inside LightingManager (`engine/include/managers/lighting_manager.h:34-45`)
- **Mutators**: updateLighting, setGlobalAmbient (`engine/src/managers/lighting_manager.cpp:53-83`)
- **GL leakage**: RenderSystem::renderObject still calls lights.applyLights() (GL uniforms) (`engine/src/render_system.cpp:1529-1534`; `engine/src/light.cpp:72-118`)
- **Snapshot/apply**: Feasible once GL direct uniforms retire
- **Needs**: Migrate remaining GL calls to manager before exposing JSON patch

**Proposed Schema:**
```json
{
  "lighting": {
    "ambient": [float, float, float, float],
    "lights": [
      {
        "type": "point|directional|spot",
        "position": [float, float, float],
        "direction": [float, float, float],
        "color": [float, float, float],
        "intensity": float,
        "innerCutoff": float,
        "outerCutoff": float,
        "enabled": bool
      }
    ]
  }
}
```

### Materials (Status: Yellow ⚠️)
- **Data**: MaterialBlock m_materialData in MaterialManager (`engine/include/managers/material_manager.h:36-47`)
- **Mutators**: updateMaterialForObject, updateMaterial (`engine/src/managers/material_manager.cpp:38-98`); per-object extraction depends on SceneObject.materialCore
- **RHI usage**: Only, but per-object updates run hot in loops (`engine/src/render_system.cpp:2389-2455`)
- **Snapshot/apply**: Global defaults simple; object-level material requires diff through SceneManager

**Schema for defaults:**
```json
{
  "materialDefaults": {
    "baseColor": [float, float, float, float],
    "metallic": float,
    "roughness": float,
    "transmission": float,
    "ior": float,
    "clearcoat": float,
    "textureFlags": {
      "baseColor": bool,
      "normal": bool,
      "metallicRoughness": bool
    }
  }
}
```

### RenderConfig / Presentation (Status: Yellow ⚠️)
- **Data**: RenderSystem fields (m_exposure, m_gamma, m_tonemap, m_backgroundColor, m_renderMode, etc.) (`engine/include/render_system.h:110-158,233-237`)
- **Mutators**: Inline setters, plus RenderingManager::updateRenderingState (`engine/src/managers/rendering_manager.cpp:44-78`)
- **GL leakage**: Polygon mode & blending toggles in renderObjectFast (`engine/src/render_system.cpp:1556-2019`)
- **Snapshot/apply**: Group fields into config struct; ensure rendering state changes route through RenderingManager

**Schema:**
```json
{
  "renderConfig": {
    "mode": "solid|wireframe|points",
    "shading": "gouraud|pbr|...",
    "exposure": float,
    "gamma": float,
    "toneMap": "linear|aces|filmic",
    "background": {
      "mode": "solid|gradient|hdr",
      "color": [float, float, float],
      "top": [float, float, float],
      "bottom": [float, float, float],
      "hdrPath": "string"
    },
    "msaaSamples": int,
    "show": { "grid": bool, "axes": bool, "skybox": bool }
  }
}
```

## Pass Catalog v0

| Pass | Current entry | Inputs | Outputs | Params | Capability flags |
|------|---------------|--------|---------|--------|------------------|
| FrameSetup | RenderSystem::passFrameSetup | scene, lights, viewport (PassContext) | cleared default RT | background color | requires RHI.clear |
| Raster | RenderSystem::passRaster | scene, lights, manager state | implicit swapchain color | none (hard-coded) | requires raster pipeline |
| Raytrace | RenderSystem::passRaytrace | scene, lights, camera | ray traced texture | sampleCount, maxDepth | requires raytracer + texture updates |
| RayDenoise | RenderSystem::passRayDenoise | rayTraceResult (future) | denoised texture | enable flag | optional denoiser |
| Overlay | RenderSystem::passOverlays | scene, lights, view/proj | overlay draws (onto current RT) | flags (grid/axes) | requires raster overlays |
| Resolve | RenderSystem::passResolve | MSAA RT | resolved color | none | requires MSAA support |
| Present | RenderSystem::passPresent | final color | swapchain | finalizeFrame | requires swapchain present |
| Readback | RenderSystem::passReadback | target texture | CPU buffer | ReadbackRequest | requires readback |
| GBuffer (stub) | RenderSystem::passGBuffer | scene | g-buffer attachments | RT desc | requires MRT |
| DeferredLighting (stub) | RenderSystem::passDeferredLighting | g-buffer | litColor | lighting config | requires raster compute |
| RayIntegrator (stub) | RenderSystem::passRayIntegrator | scene, camera | ray result | sampleCount, maxDepth | requires ray tracing |

### Minimal wrapper structure to decouple monolith:

```cpp
struct PassDescriptor {
    std::string name;
    std::vector<ResourceRef> inputs;
    std::vector<ResourceRef> outputs;
    CapabilityFlags caps;
    std::function<void(const PassContext&, RenderSystem&)> execute;
};
```

Each existing pass* becomes a std::bind-backed execute. Dependencies declared in descriptor feed the scheduler.

## Blueprint v1 + IR v0

### Blueprint schema (JSON)
```json
{
  "version": 1,
  "resources": {
    "swapchain": { "type": "swapchain" },
    "gbuffer": {
      "type": "renderTarget",
      "attachments": ["gBaseColor", "gNormal", "gPosition", "gMaterial", "gDepth"]
    },
    "litColor": { "type": "texture2D", "format": "RGBA16F" }
  },
  "passes": [
    { "name": "FrameSetup", "stage": "frame_setup", "outputs": ["swapchain"] },
    { "name": "GBuffer", "stage": "raster", "inputs": ["swapchain"], "outputs": ["gbuffer.*"] },
    { "name": "DeferredLighting", "stage": "compute", "inputs": ["gbuffer.*"], "outputs": ["litColor"] },
    { "name": "Overlay", "stage": "overlay", "inputs": ["litColor"], "outputs": ["swapchain"] },
    { "name": "Present", "stage": "present", "inputs": ["swapchain"] }
  ],
  "policies": {
    "msaaSamples": 4,
    "toneMap": "aces",
    "debugOverlays": true
  }
}
```

### IR v0 (C++ structs)
```cpp
struct ResourceNode {
    std::string name;
    ResourceType type;
    ResourceDesc desc;
};

struct RenderPassNode {
    std::string name;
    PassKind kind;
    std::vector<ResourceEdge> inputs;
    std::vector<ResourceEdge> outputs;
    CapabilityFlags caps;
};
```

### Mapping rules
- Blueprint resources → ResourceNode instances; attach swapchain alias to default framebuffer
- Pass stage selects PassKind; inputs/outputs convert to ResourceEdge { resourceId, usage }
- Policies become context flags used when instantiating pass descriptors
- IR → GL command list: for RenderPassNode in raster stage, compile to sequence:
  1. bindRenderTarget (from outputs)
  2. setViewport (from policy)
  3. Manager updates (snapshot/apply)
  4. Call bound pass callback (existing RenderSystem::pass*)

Example: DeferredLighting IR node emits bound shader pipeline, g-buffer textures as inputs, m_rhi->draw call.

## Telemetry Plan

### Per-pass NDJSON event
```json
{
  "type": "pass",
  "frame": 1234,
  "pass": "Raster",
  "ms_cpu": 1.72,
  "draws": 128,
  "ubo_writes": 64,
  "gpu_ts": { "start": 123456789, "end": 123457963 },
  "mem": { "texture_mb": 48.0, "buffer_mb": 12.5 },
  "scene_id": "sponza",
  "commit": "abc1234"
}
```

### Run metadata (once per frame)
```json
{ "type": "frame", "frame": 1234, "seed": 42, "commit": "abc1234", "policy": { "msaa": 4 } }
```

### Instrumentation points
- Wrap RenderPass::executeWithTiming to emit pass events (`engine/src/render_pass.cpp:15-34`)
- Count draw calls and UBO writes inside RenderSystem::renderObjectsBatchedWithManagers (`engine/src/render_system.cpp:2389-2455`) and passRaytrace (`engine/src/render_system.cpp:2199-2453`)
- Frame metadata at end of RenderSystem::renderUnified (`engine/src/render_system.cpp:420-422`)
- Sink: NDJSON file under build/telemetry/<timestamp>.ndjson; optional WebSocket publisher toggled via JSON-Ops flag

## Sandbox Transactions

### Snapshot API sketch
```cpp
struct RenderSnapshot {
    CameraState camera;
    LightingBlock lighting;
    RenderingConfig renderConfig;
    std::vector<MaterialOverride> materials;
};

RenderSnapshot RenderSystem::snapshot() const;
void RenderSystem::apply(const RenderSnapshot& delta);
```

### Transaction flow
1. `auto snap = renderer.snapshot();`
2. `auto trial = snap; applyJsonPatch(trial, patch);`
3. `renderer.apply(trial);`
4. Render single frame; collect telemetry/metrics (PSNR/SSIM via post hooks)
5. If accepted: `commit(trial)` (updates baseline snapshot). Else: `renderer.apply(snap);`

### Guardrails
- Snapshot copies manager data via LightingManager::getLightingData() etc. (needs new const accessors)
- Material deltas limited to identified objects via JSON pointer scene/materials/<id>
- Transaction locked to main thread; forbid nested transactions
- Ensure GPU resources reused; avoid reallocations inside transaction path

## Capability Registry

### Schema
```json
{
  "backend": "OpenGL",
  "version": "4.6",
  "caps": {
    "supportsCompute": false,
    "supportsGeometry": true,
    "supportsTessellation": true,
    "maxTextureUnits": 16,
    "maxSamples": 8,
    "supportsSRGB": true,
    "supportsAniso": true
  },
  "extensions": ["GL_ARB_debug_output", "..."]
}
```

### Probe sites
- Extend RhiGL::queryCapabilities to capture vendor/version/extension list (`engine/src/rhi/rhi_gl.cpp:761-779`)
- Store registry in RenderSystem on init; expose via JSON-Ops endpoint
- Pass modules consult registry before enabling features (e.g., RaytracePass checks supportsCompute)

## Performance Considerations

- Cache blueprint hash → IR → compiled command list; rebuild only on blueprint diff (store in RenderGraph)
- Hot loops to keep allocation-free: renderObjectsBatchedWithManagers (`engine/src/render_system.cpp:2389-2455`), RaytracePass inner loops (`engine/src/render_system.cpp:2199-2451`). No virtual dispatch inside per-object draws
- Determinism checklist:
  - Seed-controlled RNG (RenderSystem::setSeed, `engine/include/render_system.h:140-142`)

## Decision & Plan

**Recommendation**: Go, with staged integration to mitigate risk while maintaining RHI performance.

### Milestones (≤2 weeks each)

#### 1. State Surfaces & Telemetry (Week 1-2)
- Add snapshot/apply for Camera, Lighting, Rendering (read-only accessors)
- Instrument NDJSON telemetry at pass level
- Deliver JSON-Ops hooks for telemetry streaming

#### 2. Pass Descriptors & Blueprint v1 (Week 3-4)
- Wrap existing pass* in PassDescriptor
- Implement Blueprint parser + IR builder (no runtime change yet)
- Drive RenderGraph execution from IR while maintaining old path for fallback

#### 3. Sandbox & Capability Registry (Week 5-6)
- Implement transaction API with rollback
- Emit capability registry from RHI init and expose to passes
- Add minimal policy evaluation (e.g., disable ray pass when unsupported)

### Key Risks & Mitigations

1. **Performance regression from extra indirection**
   - Mitigate with blueprint caching, inline pass callbacks, benchmarks per milestone

2. **Inconsistent state between managers and scene objects**
   - Enforce single source of truth via snapshot/apply validation; add assertions when GL fallbacks invoked

3. **Telemetry overhead / I/O contention**
   - Buffer NDJSON writes asynchronously, allow sampling rate control via JSON-Ops

## Current Status

This audit reflects the state of the codebase as of the RHI modernization (FEAT-0248) completion. The unified render system provides a solid foundation for the proposed modularization, with clear interfaces and well-defined pass boundaries that can be incrementally enhanced without disrupting the existing functionality.