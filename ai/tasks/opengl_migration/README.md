# OpenGL Migration Task (RHI-First)

Goal
- Eliminate direct OpenGL usage from the interactive/rendering path, migrating to the RHI with clean, testable abstractions.
- Keep changes incremental, validated, and reversible. Preserve performance and determinism.

Scope
- Primary: `engine/src/render_system.cpp` (largest concentration of GL calls)
- Secondary: overlay utilities (grid, axes, selection outline), offscreen utilities (PNG, render-to-texture), legacy path fallbacks

Categories (Audit → Migrate)
- Framebuffer/RT
  - glGenFramebuffers/glDeleteFramebuffers → `RHI::createRenderTarget/destroyRenderTarget`
  - glBindFramebuffer → `RHI::bindRenderTarget`
  - glFramebufferTexture*/glFramebufferRenderbuffer → described by `RenderTargetDesc`
  - MSAA resolve → `RHI::resolveRenderTarget/resolveToDefaultFramebuffer`
- Pipeline/state
  - glEnable/glDisable (DEPTH_TEST, BLEND, MULTISAMPLE, POLYGON_OFFSET_LINE) → part of `PipelineDesc` (preferred) or temporary per-pass RHI helpers
  - glPolygonMode/glLineWidth/glDepthMask/glBlendFunc → `PipelineDesc` state (preferred)
- Textures
  - glGenTextures/glDeleteTextures/glBindTexture/glTexImage2D/glTexSubImage2D/glTexParameteri → `RHI::createTexture/updateTexture/destroyTexture`
- Buffers
  - glGenBuffers/glDeleteBuffers/glBindBuffer/glBufferData/glBufferSubData → `RHI::createBuffer/updateBuffer/destroyBuffer`
  - VAOs: remove in favor of explicit vertex bindings in `PipelineDesc`
- Viewport/Clear
  - glViewport → `RHI::setViewport`
  - glClearColor/glClear → `RHI::clear`
- Draw
  - glDrawArrays/glDrawElements → `RHI::draw`
- Queries/Introspection
  - glGetIntegerv(GL_VIEWPORT/GL_POLYGON_MODE) → track state in RHI or remove dependency

Deliverables
1) Coverage ledger documenting every call site and its migration status
2) RHI utility surface for missing conveniences (screen-quad, temp state-safe overlays)
3) Incremental PRs removing GL usage per category with passing tests

Validation
- Golden tests render identical (or acceptable) outputs
- No direct GL function usage remains in migrated areas
- No performance regressions in hot paths (verify draw call and timing telemetry)

Execution Plan (Phased)
Phase A — Prereqs for pass bridging ✅ COMPLETED (2024-12-29)
- ✅ Add RHI screen-quad utility (VB+IB, pipeline, simple shader binding)
  - Implemented `RHI::getScreenQuadBuffer()` with cached NDC quad vertices+UVs
  - Available in both OpenGL and Null backends with proper cleanup
- ✅ Ensure `RHI::updateTexture` and `readback` are used where applicable
  - Already implemented and used in existing RHI pipeline
- ✅ Complete framebuffer operations audit with 34+ GL calls documented

Phase B — Pass paths and offscreen (short)
- Replace GL in `renderToTextureRHI`/PNG path with RHI-only logic
- Implement `RenderSystem::passGBuffer/passDeferredLighting/passRayIntegrator/passReadback` using RHI

Phase C — Framebuffer & resolve (medium)
- Replace ad-hoc FBO allocation with `RenderTargetDesc` (MSAA & non-MSAA)
- Use `RHI::resolveRenderTarget/resolveToDefaultFramebuffer`

Phase D — Pipeline/state and overlays (medium)
- Encode depth/blend/polygon state in `PipelineDesc` and re-create pipelines as needed
- Migrate selection outline/grid/axes to RHI draw with no raw GL state toggles

Phase E — Texture/buffer lifecycle (short)
- Remove remaining `glTex*`/`glBindTexture`/buffer calls in favor of RHI calls

De-risking
- Keep legacy code paths behind a compile-time flag during migration
- Instrument with telemetry (per-pass ms_cpu, draws, simple mem counters)

How to Update Coverage
- Search patterns (example):
  - ripgrep: `rg -n "gl(Enable|Disable|Bind|Delete|Gen|Tex|Draw|Viewport|Clear|Framebuffer|Renderbuffer|Polygon|Blend|GetIntegerv)" engine/src`
  - Verify hit list and paste into COVERAGE.md checklist

