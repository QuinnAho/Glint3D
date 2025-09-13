# FEAT-0253 — RHI Framebuffer Migration (MSAA/Offscreen/Readback)

## Intent
Complete the RHI framebuffer abstraction by migrating remaining direct GL framebuffer operations to RHI render targets. This addresses the final FEAT-0248 requirement: "No direct FBO binds at call sites".

## Acceptance
- All glBindFramebuffer/glFramebufferTexture* calls route through RHI render targets
- MSAA rendering uses RHI-based multisampled render targets with resolve
- Offscreen rendering (PNG export) uses RHI render targets + readback
- Raytracer texture integration uses RHI texture handles consistently
- No direct GL framebuffer operations outside RHI backend

## Scope
- Files: `engine/src/render_system.cpp` (primary), `engine/src/ibl_system.cpp`
- Headers: `engine/include/glint3d/rhi.h` (render target extensions)
- Implementation: `engine/src/rhi/rhi_gl.cpp` (render target operations)
- Tests: Verify MSAA, offscreen rendering, raytracing integration still work

## Priority: Critical
This completes the core WebGPU-shaped API requirements and enables true GL-free engine core.

## Dependencies
- FEAT-0248: WebGPU-shaped RHI API (completed)
- FEAT-0250: TextureView + MSAA Resolve Support (provides render target foundation)

## Context
FEAT-0248 successfully migrated all uniform operations to RHI but framebuffer operations remain as direct GL calls. Key areas:

### render_system.cpp Direct FBO Usage:
1. **MSAA Rendering** (~line 400): Multi-sample framebuffer creation/resolve
2. **Offscreen Rendering** (~line 800): PNG export with temporary FBOs
3. **Raytracing Integration** (~line 1200): Texture readback operations
4. **Viewport Management**: FBO binding for resolution changes

### Migration Strategy:
- Extend RHI with MSAA render target support
- Add RHI texture readback for offscreen operations
- Route raytracer through RHI texture handles
- Implement render target resolve operations