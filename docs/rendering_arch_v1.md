# Rendering Architecture v1

- Overview: Hybrid renderer with raster preview and raytraced finals. MaterialCore unifies BSDF across both pipelines. RHI abstracts GPU backends.

## Layers
- Scene → MaterialCore → Mode Selector → RenderGraph → Passes → Readback
- RHI backs all GPU resource creation and drawing (OpenGL/WebGL2 now; Vulkan/WebGPU later).

## MaterialCore
- Single source of truth for material parameters (baseColor, metallic, roughness, ior, transmission, thickness, emissive).
- Public header: `engine/include/glint3d/material_core.h` (see docs/materials.md).

## RHI
- Public headers: `engine/include/glint3d/rhi.h`, `engine/include/glint3d/rhi_types.h` (see docs/rhi_guide.md).
- Engine Core must not call `gl*`/`glfw*` directly; use RHI (platform/windowing code excluded).

## SSR‑T (Screen‑Space Refraction with Transmission)
- Goal: Real‑time glass previews in raster pipeline.
- Current: PR2 scaffolding added (`engine/src/render/ssr_pass.cpp`, `engine/shaders/ssr_refraction.frag`).
- Next: Implement roughness‑aware blur, miss fallback, and RHI wiring; preserve opaque scene parity.

## Parity and Performance Targets
- Visual parity: Opaque scenes unchanged (SSIM ≥ 0.995 vs goldens).
- Performance: ≤ 16 ms/frame at 1080p with SSR‑T enabled; memory overhead ≤ 50 MB.

