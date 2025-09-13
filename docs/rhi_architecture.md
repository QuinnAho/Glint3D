# RHI Architecture — WebGPU-Shaped Frontend

## Direction
The Render Hardware Interface (RHI) is transitioning to a WebGPU-shaped API to ensure consistent semantics, strong validation, and parity between desktop and web platforms. While OpenGL remains the primary production backend in the near term, the frontend API adopts WebGPU concepts: PipelineLayout, BindGroupLayout, BindGroup, CommandEncoder, RenderPassEncoder, Queue, Fence, and explicit ResourceState transitions.

## Rationale — Why a WebGPU-Shaped API
- Consistent semantics: WebGPU’s model (bind groups, passes, queues) enforces clear lifetimes and ordering, reducing backend-specific ambiguity.
- Strong validation: WebGPU’s validation rules surface misuse early, improving reliability and testability.
- Desktop/Web parity: A single API shape maps cleanly to native (wgpu-native/Dawn) and browser implementations, minimizing divergence.
- Maintainability: A smaller, modern core concept set improves long-term maintainability and AI-assisted refactors.

## Scope and Immediate Goals
1) Complete the RHI interface surface with:
   - PipelineLayout, BindGroupLayout, BindGroup
   - CommandEncoder, RenderPassEncoder
   - Queue, Fence
   - ResourceState (explicit transitions)
2) Migrate all legacy uniforms to a UBO ring allocator with reflection-driven offsets.
3) Prepare for a WebGPU backend (wgpu-native/Dawn) that targets desktop and web.

## Backend Plan
- Primary backend (now): OpenGL. The GL backend adapts the WebGPU-shaped frontend to current GL calls.
- Next backend: WebGPU (wgpu-native preferred, Dawn optional). Vulkan backend is deferred.
- Null backend: Retained for testing and CI.

## Backends & API Layer
- WebGPU (desktop): Prefer `wgpu-native` (C API aligned with WebGPU) for straightforward desktop integration and an easy path to browser builds.
- WebGPU (alternative): Support Google Dawn (C++ WebGPU) if C++ integration or Chromium toolchain benefits are desired.
- Surface creation: Use GLFW for windowing (already present) and `webgpu.h` helpers to create the surface/swap chain where needed.

## API Shape Principles
- Bind groups define all shader-visible resources; no direct `glUniform*` calls.
- Encoders record work; passes scope state and attachments; queues submit recorded work.
- No direct FBO binds at call sites; all attachments go through RenderPassEncoder descriptors.
- All resource transitions are explicit in the API (or validated by the encoder when implicit).

## Migration Plan
- Phase 4 (current):
  - FEAT-0248 — RHI API Shape (BindGroups/Encoders/Queues)
  - FEAT-0249 — Uniform System Migration (UBO ring + reflection)
- Phase 5 (next):
  - FEAT-0250 — TextureView + MSAA Resolve Support
  - FEAT-0251 — Compute Pipeline + Storage Buffers
- Phase 6 (after):
  - FEAT-0252 — WebGPU Backend (wgpu-native/Dawn) for desktop/web

## Acceptance Highlights
- FEAT-0248: All GL call sites route via CommandEncoder/RenderPassEncoder; no direct FBO binds; no `glUniform*`.
- FEAT-0249: 100% uniform migration to UBO ring allocator with reflection-validated offsets; golden tests pass.
- FEAT-0252: GL vs WebGPU image diff < epsilon across 10–20 golden scenes.

## Notes
- Remove Vulkan-first language in planning; Vulkan is deferred until after WebGPU backend.
- Documentation and tests should use the WebGPU terminology where applicable (bind groups, passes, queues).

## Shader Toolchain (Lock-In Now)
- Compiler: DXC for HLSL to DXIL/SPIR-V.
- Validation/Reflection: SPIRV-Tools for validation; SPIRV-Reflect to extract reflection and auto-generate BindGroupLayouts and binding offsets.
- Cross-compilation: SPIRV-Cross to GLSL/MSL where required (to keep GL and Apple paths happy).
- Outcome: BindGroupLayout descriptors are generated from compiled shaders, feeding the RHI pipeline and uniform system.

## Texture & Transcoding (Web Parity)
- Use Basis Universal / KTX2 (UASTC/ETC1S) with libktx.
- Single source texture packaged as KTX2 can be transcoded to GPU-friendly compressed formats on desktop and web.

## Debugging & Profiling
- Frame capture: RenderDoc (GL, Vulkan, D3D12; compatible with Dawn/wgpu through native backends).
- CPU profiling: Tracy or Microprofile for CPU timelines.
- GPU timers: Add where supported per backend to profile passes.
- Browser: Chrome WebGPU capture when running via Web backend for command/pipeline inspection.

## Test & Golden Pipeline
- Unit tests: GoogleTest (or Catch2).
- Image I/O: stb_image/stb_image_write for PNG load/save.
- Golden comparisons: SSIM/PSNR compare in CI (ImageMagick or a small SSIM implementation).

## Window/Surface & UI Interop
- GLFW remains the window system with WebGPU surface via `webgpu.h` helpers.
- Dear ImGui: use `imgui_impl_opengl3` now and plan for `imgui_impl_wgpu` when WebGPU backend is available.
- RHI Interop: expose `getNativeTextureHandle()` to allow UI backends to bind textures directly when needed.

## Current Status (FEAT-0248 PR1)
- Public headers expose WebGPU-shaped types: `BindGroupLayout`, `BindGroup`, `CommandEncoder`, `RenderPassEncoder`, `Queue`, and `ResourceState`.
- OpenGL backend provides thin adapter classes (`SimpleCommandEncoderGL`, `SimpleRenderPassEncoderGL`, `SimpleQueueGL`) that map encoder calls to existing immediate-mode GL operations.
- Null backend stubs implemented to satisfy API and enable headless tests.
