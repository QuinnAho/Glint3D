# FEAT-0241 — SSR-T refraction in raster preview

## Intent (human summary)
Implement Screen-Space Refraction with Transmission (SSR‑T) in the rasterized pipeline to enable real‑time glass material previews without requiring raytracing mode. This removes the need to use `--raytrace` for glass and enables hybrid auto mode to choose between raster previews and raytraced finals.

The core architectural change introduces a unified MaterialCore struct (single source of truth for BSDF) and layers SSR‑T on top of the RHI abstraction.

## AI-Editable: Status
- Overall: in_progress
- Current PR: PR1 (in_progress)
- Last Updated: 2025-09-12T17:25:00Z

## AI-Editable: Acceptance & Must-Requirements Status
- Acceptance: Opaque scenes unchanged — Pending validation (golden tests not run yet)
- Acceptance: SSR-T for transmissive materials — Planned (PR3)
- Acceptance: Auto mode selection — Planned (PR5)
- Acceptance: RHI abstraction in Engine Core — In progress; GL/GLFW calls still present
- Must: ARCH.NO_GL_OUTSIDE_RHI — Not yet satisfied (see `render_system.cpp`, `application_core.cpp`)
- Must: API.PUBLIC_HEADERS_PATH — Satisfied (`engine/include/glint3d/**`)
- Must: DET.SEED_REPRO — Satisfied/unchanged (deterministic seed path intact)
- Must: TEST.GOLDEN_SSIM — Pending (integration test placeholder present)
- Must: PERF.FRAME_BUDGET — Pending measurement post-SSR implementation

## AI-Editable: Checklist (mirror of spec.yaml acceptance_criteria)
- [ ] Opaque scenes unchanged (SSIM ≥ 0.995 vs goldens) — Future PRs (PR3–PR5)
- [ ] Transmissive materials show SSR‑T with roughness‑aware blur — Future PR (PR3)
- [ ] --mode auto picks raster for previews, ray for finals — Future PR (PR5)
- [x] MaterialCore struct eliminates dual material storage — Completed in PR1
- [ ] RHI abstraction prevents direct OpenGL/GLFW calls in Engine Core — In progress (violations in render_system.cpp and application_core.cpp)

## AI-Editable: Current PR Progress (PR1: MaterialCore + RHI headers)
- [x] Define MaterialCore unified BSDF struct
- [x] Define RHI interface with GL/WebGL2/Vulkan abstraction
- [x] Update SceneObject to use MaterialCore instead of dual storage
- [x] Add transmission and thickness fields to JSON Ops schema
- [x] Write unit tests for MaterialCore BSDF calculations

## AI-Editable: Current PR Progress (PR2: SSR‑T pass stubs)
- [x] Add SSR refraction shader stub (engine/shaders/ssr_refraction.frag)
- [x] Add SSR refraction vertex shader stub (engine/shaders/ssr_refraction.vert)
- [x] Add SSR pass C++ stub (engine/src/render/ssr_pass.cpp)
- [x] Extend RHI with uniform buffer APIs (bindUniformBuffer/updateBuffer) and update docs/tests

## AI-Editable: Implementation Notes

### Key Discovery: Existing Implementation Foundation
- MaterialCore exists internally (`engine/include/material_core.h`); public header added
- RHI public API exists (`engine/include/glint3d/rhi*.h`)
- JSON schema supports `ior`, `transmission`, `thickness`

### RHI Interface Scope Completed
- Public API: `engine/include/glint3d/rhi.h`, `engine/include/glint3d/rhi_types.h`
- Docs: `docs/rhi_guide.md`

### RESOLVED: Dual Storage Problem Migration
Unified MaterialCore storage in scene state; legacy Material retained temporarily for raytracer compatibility.

**Next Steps for PR2**
1. Wire SSR pass behind feature flag (no visual change)
2. Implement roughness‑aware blur and miss fallback
3. Begin replacing direct GL calls with RHI in non‑behavior‑changing spots

### AI-Editable: RHI Migration Plan (from architecture)
- Replace viewport/clear: use `RHI::setViewport`/`clear` in `render_system.cpp`
- Texture creation: use `createTexture(TextureDesc)` instead of `glTex*`
- Material uniforms: switch to UBO via RHI; remove per‑uniform `glUniform*`
- Draw path: route via `RHI::draw(DrawDesc)`; remove VAO‑specific calls and `glDraw*`
- Engine Core isolation: remove GL/GLFW includes from Engine Core; keep platform/windowing out of Engine Core

## AI-Editable: Blockers and Risks
- Direct GL in Engine Core: `render_system.cpp` uses gl*; needs phased migration
- GLFW in Engine Core: `application_core.cpp` uses glfw*; should be confined to platform layer
- Performance: SSR‑T texture reads and blur need optimization to meet 16 ms at 1080p
- Quality: SSR‑T may not match raytracing in complex cases; define fallbacks

## Links
- Spec: [ai/tasks/FEAT-0241/spec.yaml](spec.yaml)
- Passes: [ai/tasks/FEAT-0241/passes.yaml](passes.yaml)
- Progress Log: [ai/tasks/FEAT-0241/progress.ndjson](progress.ndjson)
- Architecture: [docs/rendering_arch_v1.md](../../docs/rendering_arch_v1.md)
- Materials: [docs/materials.md](../../docs/materials.md)
- RHI Guide: [docs/rhi_guide.md](../../docs/rhi_guide.md)
