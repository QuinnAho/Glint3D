# RHI Public API

- Overview: Thin Render Hardware Interface for desktop OpenGL, WebGL2, and future backends.
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
- Draw: `bindPipeline()`, `bindTexture()`, `draw()`; State: `setViewport()`, `clear()`
- Uniforms: `bindUniformBuffer(buffer, slot)`, `updateBuffer(buffer, data, size, offset)`
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
    // setup resources, pipelines, and issue rhi->draw({...})
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
