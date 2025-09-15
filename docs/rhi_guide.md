# RHI Public API (Transitioning to WebGPU Shape)

- Overview: Thin Render Hardware Interface for desktop OpenGL and a WebGPU-shaped frontend enabling a WebGPU backend (wgpu-native/Dawn) and web parity.
- Headers: `engine/include/glint3d/rhi.h`, `engine/include/glint3d/rhi_types.h`
- Design: Type‑safe opaque handles, RAII lifecycle, minimal overhead, cross‑platform.

## Core Types (rhi_types.h)
- Handles: `TextureHandle`, `BufferHandle`, `ShaderHandle`, `PipelineHandle` (use `INVALID_HANDLE` for invalid)
- Descriptors: `RhiInit`, `TextureDesc`, `BufferDesc`, `ShaderDesc`, `PipelineDesc`, `DrawDesc`, `ReadbackDesc`
- Enums: `TextureFormat`, `TextureType`, `BufferType`, `BufferUsage`, `PrimitiveTopology`, `ShaderStage` (bitwise ops enabled)

## Interface (rhi.h)
- Backend: `RHI::Backend { OpenGL, WebGL2, Vulkan, WebGPU, Null }`
- Lifecycle: `init()`, `shutdown()`, `beginFrame()`, `endFrame()`
- Resources: `createTexture()`, `createBuffer()`, `createShader()`, `createPipeline()` and matching `destroy*()`
- Draw (current -> migrating): existing `bindPipeline()/draw()` model is migrating to a WebGPU-style encoder model: `CommandEncoder`, `RenderPassEncoder`, `Queue::submit()`.
- State: `setViewport()`, `clear()` move under pass/encoder scope.
- Uniforms: migrating from direct updates to a UBO ring allocator with reflection-driven offsets (see architecture doc).
- Capabilities: `supportsCompute()`, `supportsGeometryShaders()`, `supportsTessellation()`, `getMaxTextureUnits()`, `getMaxSamples()`
- Factory: `createRHI(Backend)`, `createDefaultRHI()` (may return nullptr if unavailable)

## Render Targets (in progress)
- Descriptor: `RenderTargetDesc{ width, height, samples, hasColor, colorFormat, hasDepth, depthFormat }`
- API (internal RHI for engine wiring): `createRenderTarget()`, `bindRenderTarget()`, `destroyRenderTarget()`
- Note: Backends may treat handle 0 as the default framebuffer/backbuffer.

## Usage Sketch
```cpp
#include <glint3d/rhi.h>
using namespace glint3d;

std::unique_ptr<RHI> rhi = createRHI(RHI::Backend::OpenGL); // or createDefaultRHI()
RhiInit init{}; init.windowWidth = 1280; init.windowHeight = 720;
if (rhi && rhi->init(init)) {
    rhi->beginFrame();

    auto encoder = rhi->createCommandEncoder("main");
    RenderPassDesc passDesc{}; passDesc.width = 1280; passDesc.height = 720;
    auto* pass = encoder->beginRenderPass(passDesc);
    pass->setPipeline(/* pipeline */ INVALID_HANDLE);
    // pass->setBindGroup(0, /* bind group */ INVALID_HANDLE);
    DrawDesc draw{}; draw.vertexCount = 3; pass->draw(draw);
    pass->end();
    rhi->getQueue().submit(*encoder);

    rhi->endFrame();
    rhi->shutdown();
}
```

## Textures and Legacy Integration

When running with an active RHI backend, the legacy `Texture` class now carries an optional matching RHI texture handle. On load, `TextureCache` will create an RHI texture (if an RHI instance has been registered) using decoded CPU pixels via `ImageIO` and store the handle in the `Texture` instance. Render paths can then bind via RHI:

```cpp
if (tex && tex->rhiHandle() != glint3d::INVALID_HANDLE) {
    m_rhi->bindTexture(tex->rhiHandle(), unit);
} else {
    tex->bind(unit); // legacy GL path
}
```

Register the active RHI after initialization (e.g., in `RenderSystem`):

```cpp
// After m_rhi->init(...)
Texture::setRHI(m_rhi.get());
```

## Constraints
- No direct `gl*` or `glfw*` calls in Engine Core; use RHI (platform/windowing remains in platform layer).
- Preserve existing opaque rendering behavior; SSR‑T integration comes in later PRs.

## Tooling & Interop Overview
- Shader toolchain: DXC (HLSL→SPIR-V), SPIRV-Tools (validation), SPIRV-Reflect (reflection), SPIRV-Cross (GLSL/MSL where needed). BindGroupLayouts are auto-generated from reflection.
- Textures: Basis Universal / KTX2 via libktx for desktop/web parity.
- Debugging: RenderDoc capture on native; Chrome WebGPU capture in browser builds.
- Profiling: Tracy or Microprofile; add GPU timers per backend where supported.
- Window/UI: GLFW + `webgpu.h` for surface; ImGui via `imgui_impl_opengl3` now, `imgui_impl_wgpu` later. Expose `getNativeTextureHandle()` for UI texture binding.

See `docs/rhi_architecture.md` for the WebGPU-shaped API roadmap and acceptance criteria.

## Uniforms (UBO Ring + Reflection)

All uniform updates flow through the RHI using a ring-allocated uniform buffer and reflection to validate offsets and types. There are no direct `glUniform*` calls outside the GL backend.

Key APIs:
- `allocateUniforms(const UniformAllocationDesc&)` → returns a CPU-writable range.
- `setUniformInBlock` / `setUniformsInBlock` → copy name-addressed data into the allocation using reflection offsets.
- `bindUniformBlock(const UniformAllocation&, ShaderHandle, const char* blockName)` → binds the allocation to the uniform block’s binding point.

Usage sketch:
```cpp
// Assume: shader has a uniform block "MaterialBlock" (std140) with fields
//   vec4 baseColorFactor; float metallicFactor; float roughnessFactor; ...

// 1) Allocate space for the block
UniformAllocationDesc allocDesc{ /*size=*/sizeof_MaterialBlock, /*alignment=*/256, "MaterialBlock" };
auto ua = rhi->allocateUniforms(allocDesc);

// 2) Populate fields via reflection-validated setters
glm::vec4 baseColor = ...;
float metallic = ...;
float roughness = ...;
RHI::UniformNameValue pairs[] = {
  {"baseColorFactor", &baseColor, sizeof(baseColor), UniformType::Vec4},
  {"metallicFactor",  &metallic,  sizeof(metallic),  UniformType::Float},
  {"roughnessFactor", &roughness, sizeof(roughness), UniformType::Float},
};
int ok = rhi->setUniformsInBlock(ua, shaderHandle, "MaterialBlock", pairs, (int)(sizeof(pairs)/sizeof(pairs[0])));

// 3) Bind allocation to the block’s binding point
bool bound = rhi->bindUniformBlock(ua, shaderHandle, "MaterialBlock");

// 4) Issue draws that rely on this block
DrawDesc draw{ /* ... */ };
rhi->draw(draw);
```

Notes:
- Use std140 layout in GLSL/HLSL to ensure predictable offsets across backends.
- Block sizes can be derived from reflection or kept in codegen outputs.
- Freeing an allocation via `freeUniforms()` marks space reusable on ring wrap-around.
