# IBL Cubemap RHI Implementation Plan

**Date**: 2025-10-03
**Status**: Planning
**Estimated Effort**: 12-16 hours
**Priority**: Medium (deferred from Phase 4)
**Prerequisite**: ApplicationCore migration complete ✓

## Executive Summary

The IBL (Image-Based Lighting) system requires rendering to cubemap faces for environment map generation, irradiance computation, and prefilter map creation. This currently uses **73 direct OpenGL calls** that must be migrated to RHI abstractions to meet the zero-GL acceptance criteria.

**Current GL Dependencies**:
- Cubemap texture creation (6 faces × multiple maps)
- Framebuffer attachment to individual cubemap faces
- Per-face rendering with different view matrices
- Mipmap generation for prefilter maps (5 mip levels × 6 faces)
- Dynamic viewport resizing for different mip levels

---

## Current Implementation Analysis

### 1. Environment Map Generation (`loadHDREnvironment`)
**GL Calls**: ~25
- Creates 512×512 RGB16F cubemap
- Renders equirectangular HDR texture to 6 cubemap faces
- Generates mipmaps

```cpp
// Current GL implementation
glGenTextures(1, &m_environmentMap);
glBindTexture(GL_TEXTURE_CUBE_MAP, m_environmentMap);
for (unsigned int i = 0; i < 6; ++i) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, ...);
}
glBindFramebuffer(GL_FRAMEBUFFER, m_captureFramebuffer);
for (unsigned int i = 0; i < 6; ++i) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_environmentMap, 0);
    glClear(...);
    renderCube(); // Uses legacy Shader
}
glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
```

### 2. Irradiance Map Generation (`generateIrradianceMap`)
**GL Calls**: ~20
- Creates 32×32 RGB16F cubemap
- Convolves environment map for diffuse lighting
- Single mip level

```cpp
// Current GL implementation
glGenTextures(1, &m_irradianceMap);
for (unsigned int i = 0; i < 6; ++i) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, ...);
}
for (unsigned int i = 0; i < 6; ++i) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceMap, 0);
    renderCube(); // 6 face renders
}
```

### 3. Prefilter Map Generation (`generatePrefilterMap`)
**GL Calls**: ~25
- Creates 128×128 RGB16F cubemap with 5 mip levels
- Renders 6 faces × 5 mip levels = 30 render passes
- Different roughness value per mip level

```cpp
// Current GL implementation
glGenTextures(1, &m_prefilterMap);
for (unsigned int i = 0; i < 6; ++i) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, ...);
}
glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

for (unsigned int mip = 0; mip < 5; ++mip) {
    unsigned int mipSize = 128 * std::pow(0.5, mip);
    glViewport(0, 0, mipSize, mipSize);
    for (unsigned int i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_prefilterMap, mip);
        renderCube(); // 30 face renders total
    }
}
```

### 4. BRDF LUT Generation (`generateBRDFLUT`)
**GL Calls**: ~8
- Creates 512×512 RG16F 2D texture (not a cubemap)
- Single render pass with full-screen quad
- Already partially RHI-compatible (2D texture, not cubemap)

---

## Required RHI Extensions

### 1. Cubemap Texture Creation

**New API**: Extend `TextureDesc` for cubemap support

```cpp
// In rhi_types.h
struct TextureDesc {
    TextureType type = TextureType::Texture2D;  // Already exists
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;        // ADD: Support for mipmaps
    uint32_t arrayLayers = 1;
    const void* initialData = nullptr;
    bool generateMipmaps = false;  // ADD: Auto-generate mipmaps
    // Cubemap-specific: type = TextureCube, arrayLayers = 6
};
```

**Implementation**:
```cpp
// In RhiGL::createTexture()
if (desc.type == TextureType::TextureCube) {
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texId);
    for (uint32_t face = 0; face < 6; ++face) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0,
                     glFormat, desc.width, desc.height, 0,
                     glFormat, glType, desc.initialData);
    }
    if (desc.generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
    return texId;
}
```

### 2. Cubemap Face Attachment

**New Type**: `CubemapFace` enum

```cpp
// In rhi_types.h
enum class CubemapFace : uint32_t {
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5
};

struct RenderTargetAttachment {
    TextureHandle texture = INVALID_HANDLE;
    uint32_t mipLevel = 0;
    uint32_t arrayLayer = 0;          // For texture arrays
    CubemapFace cubemapFace = CubemapFace::PositiveX;  // ADD: For cubemaps
    bool isCubemap = false;            // ADD: Flag for cubemap handling
};

struct RenderTargetDesc {
    std::vector<RenderTargetAttachment> colorAttachments;
    RenderTargetAttachment depthStencilAttachment;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t samples = 1;
};
```

**Implementation**:
```cpp
// In RhiGL::createRenderTarget()
for (const auto& colorAttachment : desc.colorAttachments) {
    if (colorAttachment.isCubemap) {
        GLenum cubeFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X +
                         static_cast<uint32_t>(colorAttachment.cubemapFace);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                              cubeFace, colorAttachment.texture,
                              colorAttachment.mipLevel);
    } else {
        // Existing 2D texture path
    }
}
```

### 3. Mipmap Generation

**New API**: `RHI::generateMipmaps()`

```cpp
// In rhi.h
virtual void generateMipmaps(TextureHandle texture) = 0;

// In rhi_gl.cpp
void RhiGL::generateMipmaps(TextureHandle texture) {
    auto it = m_textures.find(texture);
    if (it == m_textures.end()) return;

    GLenum target = (it->second.type == TextureType::TextureCube)
                   ? GL_TEXTURE_CUBE_MAP
                   : GL_TEXTURE_2D;
    glBindTexture(target, it->second.glId);
    glGenerateMipmap(target);
}
```

### 4. Dynamic Render Target Recreation

**Challenge**: Prefilter map needs different viewports per mip level

**Solution A**: Multiple render targets (one per mip level)
```cpp
// Create 5 render targets for 5 mip levels
for (uint32_t mip = 0; mip < 5; ++mip) {
    RenderTargetDesc rtDesc;
    rtDesc.width = 128 >> mip;
    rtDesc.height = 128 >> mip;
    rtDesc.colorAttachments.push_back({prefilterTexture, mip, 0, face, true});
    m_prefilterRenderTargets[mip][face] = m_rhi->createRenderTarget(rtDesc);
}
```

**Solution B**: Single render target with dynamic attachment (requires RHI extension)
```cpp
// New API
virtual void updateRenderTargetAttachment(RenderTargetHandle rt,
                                          uint32_t attachmentIndex,
                                          const RenderTargetAttachment& attachment) = 0;
```

---

## Implementation Steps

### Phase 1: RHI Type Extensions (2-3 hours)

1. **Add to `rhi_types.h`**:
   - `CubemapFace` enum
   - `RenderTargetAttachment::cubemapFace` field
   - `RenderTargetAttachment::isCubemap` flag
   - `TextureDesc::mipLevels` field
   - `TextureDesc::generateMipmaps` flag

2. **Extend `RHI` interface**:
   - `virtual void generateMipmaps(TextureHandle texture) = 0;`
   - Optional: `virtual void updateRenderTargetAttachment(...)` for dynamic mip levels

3. **Update `RhiGL` implementation**:
   - Implement cubemap texture creation
   - Implement cubemap face attachment in `createRenderTarget()`
   - Implement `generateMipmaps()`

4. **Update `RhiNull` stub implementation**:
   - Add no-op implementations for new methods

**Validation**: Build succeeds, RHI interface compiles

---

### Phase 2: Environment Map Migration (3-4 hours)

1. **Replace `loadHDREnvironment` GL calls**:
   ```cpp
   // Create cubemap texture via RHI
   TextureDesc cubemapDesc;
   cubemapDesc.type = TextureType::TextureCube;
   cubemapDesc.format = TextureFormat::RGB16F;
   cubemapDesc.width = 512;
   cubemapDesc.height = 512;
   cubemapDesc.mipLevels = 5;
   m_environmentMapRHI = m_rhi->createTexture(cubemapDesc);

   // Create render targets for each face
   for (uint32_t face = 0; face < 6; ++face) {
       RenderTargetDesc rtDesc;
       rtDesc.width = 512;
       rtDesc.height = 512;
       RenderTargetAttachment colorAttachment;
       colorAttachment.texture = m_environmentMapRHI;
       colorAttachment.mipLevel = 0;
       colorAttachment.cubemapFace = static_cast<CubemapFace>(face);
       colorAttachment.isCubemap = true;
       rtDesc.colorAttachments.push_back(colorAttachment);
       m_envCaptureRTs[face] = m_rhi->createRenderTarget(rtDesc);
   }

   // Render each face
   for (uint32_t face = 0; face < 6; ++face) {
       m_rhi->bindRenderTarget(m_envCaptureRTs[face]);
       m_rhi->setViewport(0, 0, 512, 512);
       m_rhi->clear(glm::vec4(0.0f), 1.0f, 0);

       // Set view matrix for this face
       m_rhi->setUniformMat4("view", captureViews[face]);

       // Draw cube (already uses RHI)
       renderCube();
   }

   // Generate mipmaps
   m_rhi->generateMipmaps(m_environmentMapRHI);
   ```

2. **Migrate legacy Shader usage**:
   - Convert `m_equirectToCubemapShaderLegacy` to RHI shader handle
   - Use RHI pipeline instead of `shader->use()`
   - Use RHI uniform setters (already available)

3. **Clean up GL texture handles**:
   - Remove `m_environmentMap` (GLuint)
   - Use `m_environmentMapRHI` (TextureHandle) everywhere

**Validation**: Environment map renders correctly, no GL calls in `loadHDREnvironment()`

---

### Phase 3: Irradiance Map Migration (2-3 hours)

1. **Create irradiance cubemap via RHI**:
   ```cpp
   TextureDesc irradianceDesc;
   irradianceDesc.type = TextureType::TextureCube;
   irradianceDesc.format = TextureFormat::RGB16F;
   irradianceDesc.width = 32;
   irradianceDesc.height = 32;
   irradianceDesc.mipLevels = 1;
   m_irradianceMapRHI = m_rhi->createTexture(irradianceDesc);
   ```

2. **Render 6 faces using RHI render targets**:
   - Similar pattern to environment map
   - No mipmap generation needed

3. **Migrate irradiance shader to RHI**:
   - Convert `m_irradianceShaderLegacy` to RHI pipeline

**Validation**: Irradiance map renders correctly, diffuse lighting works

---

### Phase 4: Prefilter Map Migration (4-5 hours)

**Complexity**: 30 render passes (6 faces × 5 mip levels)

1. **Create prefilter cubemap with mipmaps**:
   ```cpp
   TextureDesc prefilterDesc;
   prefilterDesc.type = TextureType::TextureCube;
   prefilterDesc.format = TextureFormat::RGB16F;
   prefilterDesc.width = 128;
   prefilterDesc.height = 128;
   prefilterDesc.mipLevels = 5;
   m_prefilterMapRHI = m_rhi->createTexture(prefilterDesc);
   ```

2. **Create render targets for each mip level × face**:
   ```cpp
   for (uint32_t mip = 0; mip < 5; ++mip) {
       uint32_t mipSize = 128 >> mip;
       for (uint32_t face = 0; face < 6; ++face) {
           RenderTargetDesc rtDesc;
           rtDesc.width = mipSize;
           rtDesc.height = mipSize;
           RenderTargetAttachment colorAttachment;
           colorAttachment.texture = m_prefilterMapRHI;
           colorAttachment.mipLevel = mip;
           colorAttachment.cubemapFace = static_cast<CubemapFace>(face);
           colorAttachment.isCubemap = true;
           rtDesc.colorAttachments.push_back(colorAttachment);
           m_prefilterRTs[mip][face] = m_rhi->createRenderTarget(rtDesc);
       }
   }
   ```

3. **Render all mip levels**:
   ```cpp
   for (uint32_t mip = 0; mip < 5; ++mip) {
       uint32_t mipSize = 128 >> mip;
       float roughness = (float)mip / 4.0f;

       for (uint32_t face = 0; face < 6; ++face) {
           m_rhi->bindRenderTarget(m_prefilterRTs[mip][face]);
           m_rhi->setViewport(0, 0, mipSize, mipSize);
           m_rhi->clear(glm::vec4(0.0f), 1.0f, 0);
           m_rhi->setUniformFloat("roughness", roughness);
           m_rhi->setUniformMat4("view", captureViews[face]);
           renderCube();
       }
   }
   ```

4. **Resource management**:
   - Store 30 render target handles (5 mips × 6 faces)
   - Clean up in destructor

**Validation**: Prefilter map renders correctly, specular highlights work at all roughness values

---

### Phase 5: BRDF LUT Migration (1-2 hours)

**Note**: Already mostly RHI-compatible (2D texture, not cubemap)

1. **Migrate remaining GL calls**:
   - `glGenTextures` → `RHI::createTexture()`
   - `glBindFramebuffer` → Use existing RHI render target
   - `glFramebufferTexture2D` → `RenderTargetDesc` with 2D texture
   - `glViewport` → `RHI::setViewport()`
   - `glClear` → `RHI::clear()`

2. **Use RHI screen quad**:
   - Already available via `RHI::getScreenQuadBuffer()`

**Validation**: BRDF LUT renders correctly, PBR lighting unchanged

---

### Phase 6: Legacy Shader Removal (2-3 hours)

1. **Remove legacy Shader class usage from IBLSystem**:
   - `m_equirectToCubemapShaderLegacy` → RHI ShaderHandle
   - `m_irradianceShaderLegacy` → RHI ShaderHandle
   - `m_prefilterShaderLegacy` → RHI ShaderHandle
   - `m_brdfShaderLegacy` → RHI ShaderHandle

2. **Create RHI pipelines**:
   ```cpp
   // Environment map pipeline
   ShaderDesc equirectShaderDesc;
   equirectShaderDesc.vertexSource = equirectToCubemapVertSrc;
   equirectShaderDesc.fragmentSource = equirectToCubemapFragSrc;
   m_equirectShader = m_rhi->createShader(equirectShaderDesc);

   PipelineDesc equirectPipelineDesc;
   equirectPipelineDesc.shader = m_equirectShader;
   equirectPipelineDesc.topology = PrimitiveTopology::Triangles;
   // ... vertex attributes for cube mesh
   m_equirectPipeline = m_rhi->createPipeline(equirectPipelineDesc);
   ```

3. **Remove Shader.h dependency from IBLSystem**:
   - Update `ibl_system.h` to remove `#include "shader.h"`
   - Remove all `Shader*` member variables

**Validation**: Build succeeds without Shader class in IBLSystem

---

### Phase 7: Validation & Cleanup (2-3 hours)

1. **GL call audit**:
   ```bash
   rg "gl[A-Z]" engine/src/ibl_system.cpp
   # Expected: Zero matches
   ```

2. **Visual validation**:
   - Load HDR environment (e.g., `assets/img/studio.hdr`)
   - Verify environment map displays correctly
   - Verify diffuse lighting from irradiance map
   - Verify specular reflections at all roughness values
   - Verify BRDF integration correct

3. **Performance check**:
   - Measure IBL generation time (should be <1 second for typical HDR)
   - Ensure no frame drops during generation

4. **Update documentation**:
   - Mark IBL generation as RHI-only in progress.ndjson
   - Update task.json completion percentage
   - Update acceptance criteria checklist

**Validation**: All IBL features work identically to GL version

---

## Resource Management Considerations

### Render Target Handle Storage

**Option A**: Store all render targets upfront
```cpp
class IBLSystem {
private:
    RenderTargetHandle m_envCaptureRTs[6];        // 6 faces
    RenderTargetHandle m_irradianceRTs[6];        // 6 faces
    RenderTargetHandle m_prefilterRTs[5][6];      // 5 mips × 6 faces = 30 handles
    RenderTargetHandle m_brdfRT;                   // 1 handle
    // Total: 43 render target handles
};
```

**Option B**: Create on-demand and destroy immediately
```cpp
// In generatePrefilterMap()
for (uint32_t mip = 0; mip < 5; ++mip) {
    for (uint32_t face = 0; face < 6; ++face) {
        auto rt = m_rhi->createRenderTarget(rtDesc);
        m_rhi->bindRenderTarget(rt);
        // ... render ...
        m_rhi->destroyRenderTarget(rt);  // Clean up immediately
    }
}
```

**Recommendation**: Option B - on-demand creation
- Lower memory footprint
- IBL generation is one-time operation (not per-frame)
- Simpler lifetime management

---

## WebGL2 Compatibility Considerations

**Cubemap Support**: ✅ Fully supported in WebGL2
- `gl.TEXTURE_CUBE_MAP` available
- `gl.framebufferTexture2D()` with cubemap faces supported
- `gl.generateMipmap()` works on cubemaps

**Potential Issues**:
1. **Float texture support**: Requires `EXT_color_buffer_float` extension
   - Check availability: `gl.getExtension('EXT_color_buffer_float')`
   - Fallback: Use RGBA8 instead of RGB16F (lower quality)

2. **Framebuffer completeness**: Some mobile GPUs don't support rendering to float cubemaps
   - Add fallback path for WebGL2 if needed

---

## Testing Strategy

### Unit Tests (Optional)
```cpp
TEST(IBLSystem, CreateEnvironmentMapRHI) {
    auto rhi = createRHI(RHI::Backend::OpenGL);
    IBLSystem ibl;
    ibl.init(rhi.get());
    EXPECT_TRUE(ibl.loadHDREnvironment("test.hdr"));
    EXPECT_NE(ibl.getEnvironmentMap(), INVALID_HANDLE);
}
```

### Integration Tests
```bash
# Test IBL generation with various HDR files
./glint --ops tests/ibl_cubemap_test.json --render ibl_output.png

# Verify output matches reference image
compare -metric SSIM ibl_output.png tests/golden/ibl_reference.png diff.png
```

### Visual Regression Tests
- Render PBR sphere with IBL in multiple roughness levels
- Compare against golden reference images
- Acceptable difference threshold: SSIM > 0.99

---

## Acceptance Criteria

- [ ] Zero GL calls in `engine/src/ibl_system.cpp`
- [ ] All IBL features work identically to GL version
- [ ] Visual regression tests pass (SSIM > 0.99)
- [ ] IBL generation completes in <2 seconds for 1K HDR
- [ ] WebGL2 compatibility maintained (with float texture extension)
- [ ] Legacy Shader class removed from IBLSystem
- [ ] Documentation updated with new RHI cubemap API

---

## Estimated Effort Breakdown

| Phase | Description | Hours |
|-------|-------------|-------|
| 1 | RHI Type Extensions | 2-3 |
| 2 | Environment Map Migration | 3-4 |
| 3 | Irradiance Map Migration | 2-3 |
| 4 | Prefilter Map Migration | 4-5 |
| 5 | BRDF LUT Migration | 1-2 |
| 6 | Legacy Shader Removal | 2-3 |
| 7 | Validation & Cleanup | 2-3 |
| **Total** | | **16-23 hours** |

---

## Dependencies

**Blocked By**: None (ApplicationCore complete ✓)

**Blocks**:
- Final OpenGL migration completion (95 → 22 remaining GL calls after IBL)
- Backend swap capability (removes 73 of 95 blocking GL calls)

**Related Tasks**:
- `setUniform*` bridge removal (22 GL calls in Shader class remain)
- Backend swap validation (requires zero GL outside rhi_gl.cpp)

---

## Risks & Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Float texture support missing on WebGL2 | High | Add fallback to RGBA8, detect extension availability |
| Performance degradation with 43 render targets | Medium | Use on-demand creation (Option B) |
| Visual differences vs GL implementation | Medium | Implement side-by-side comparison tool |
| Shader conversion complexity | Low | Reuse existing shader sources, minimal changes |

---

## Notes

- This implementation completes the OpenGL → RHI migration for IBL generation
- After completion: 22 GL calls remain (all in legacy Shader class)
- Shader class can be removed once setUniform* bridge is eliminated
- Final migration: 336 → 409 GL calls eliminated (95% complete)

---

**Status**: Ready for implementation when prioritized
**Next Action**: Await task prioritization decision (Phase 5 setUniform* vs IBL cubemap)
