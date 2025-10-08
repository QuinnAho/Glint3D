# OpenGL Migration Checklist

**Last Updated**: 2025-10-07
**Status**: 94% Complete (426/454 GL calls eliminated)

---

## Phase 1: Render System Migration ✅ COMPLETED (2025-01-15)

### OpenGL Call Inventory ✅
- [x] **Complete Audit** - Documented all 146 OpenGL calls in render_system.cpp
- [x] **Categorization** - Grouped by function: framebuffer, state, resources, rendering
- [x] **Impact Assessment** - Identified critical vs. optional GL operations
- [x] **RHI Gap Analysis** - Documented and implemented missing RHI abstractions

### RHI Abstraction Implementation ✅
- [x] **Framebuffer Operations** - createRenderTarget, setRenderTarget, destroyRenderTarget
- [x] **Rendering State** - Pipeline-based state (depth, blend, polygon mode, line width)
- [x] **Resource Management** - createTexture, createBuffer, updateTexture, updateBuffer
- [x] **Vertex Operations** - Pipeline vertex attributes, draw() with DrawDesc
- [x] **Viewport and Clear** - setViewport(), clear() with color/depth/stencil

### Screen-Quad Utility Implementation ✅
- [x] **RHI Screen-Quad Utility** - getScreenQuadBuffer() returns cached quad
- [x] **Vertex Buffer Creation** - NDC vertices + UVs in single interleaved buffer
- [x] **Multiple Backends** - Implemented in RhiGL and RhiNull
- [x] **Resource Cleanup** - Proper destruction in RHI destructor

### Migration Execution ✅
- [x] **Phase 1a: Legacy Path Removal** - Removed renderLegacy() and renderToTexture()
- [x] **Phase 1b: Unified Path Cleanup** - Removed GL calls from renderUnified()
- [x] **Phase 1c: Active Render Paths** - Migrated renderRasterized(), renderRaytraced()
- [x] **Phase 1d: Debug/Overlay Paths** - Migrated renderDebugElements() GL calls
- [x] **Phase 1e: Fallback Elimination** - Removed all GL fallback code

### Validation ✅
- [x] **Build Verification** - Project compiles cleanly
- [x] **Runtime Testing** - Rendering output validated (sphere_basic, directional-light-test)
- [x] **Performance Check** - No significant performance regression
- [x] **OpenGL Detection** - Zero GL calls in render_system.cpp (verified via grep)

---

## Phase 2: Helper Systems Migration ✅ COMPLETED (2025-10-02)

### AxisRenderer (engine/src/axisrenderer.cpp) ✅
- [x] **Audit GL Calls** - 26 OpenGL calls identified (VAO/VBO/shader/draw)
- [x] **Create RHI Buffers** - Replaced glGenVertexArrays/glGenBuffers with RHI::createBuffer
- [x] **Create RHI Shader** - Replaced shader compilation with RHI::createShader
- [x] **Create RHI Pipeline** - Setup pipeline with vertex attributes (Float3 position + color)
- [x] **Migrate render()** - Replaced glDrawArrays with RHI::draw
- [x] **Update init() signature** - Changed to init(RHI*), removed static setRHI
- [x] **Test** - Build successful, 0 OpenGL calls remaining (verified)

### Grid (engine/src/grid.cpp) ✅
- [x] **Audit GL Calls** - 10 OpenGL calls identified (geometry/shader/VAO)
- [x] **Create RHI Resources** - Migrated vertex buffers and embedded shaders
- [x] **Create RHI Pipeline** - Setup grid rendering pipeline with line topology
- [x] **Migrate render()** - Use RHI::draw for grid lines
- [x] **Update init() signature** - Changed to init(RHI*, Shader*, int, float)
- [x] **Test** - Build successful, 0 OpenGL calls remaining (verified)
- [x] **Note** - Has legacy `Shader* m_shader` member (may be unused, to be verified in Phase 6)

### Gizmo (engine/src/gizmo.cpp) ✅
- [x] **Audit GL Calls** - Already migrated to RHI (0 GL calls found)
- [x] **Verify RHI Resources** - Uses RHI buffers, shaders, pipelines
- [x] **Verify RHI Pipeline** - Setup complete with vertex attributes
- [x] **Verify render()** - Uses RHI::draw for gizmo elements
- [x] **Test** - Build successful, verified zero GL calls

### Skybox (engine/src/skybox.cpp) ✅
- [x] **Audit GL Calls** - Already migrated to RHI (0 GL calls found)
- [x] **Verify RHI Cubemap** - Uses RHI::createTexture with TextureType::TextureCube
- [x] **Verify RHI Pipeline** - Setup skybox rendering pipeline
- [x] **Verify render()** - Uses RHI::draw for skybox cube
- [x] **Test** - Build successful, verified zero GL calls

### Light (engine/src/light.cpp) ✅
- [x] **Audit GL Calls** - 76 OpenGL calls found in initIndicator/renderIndicators
- [x] **Migrate initIndicator()** - Replaced VAO/VBO/shader compilation with RHI equivalents
- [x] **Remove initIndicatorShader()** - Merged into initIndicator() using RHI::createShader
- [x] **Migrate renderIndicators()** - Replaced all GL draw calls with RHI::draw
- [x] **Update Call Sites** - Updated application_core.cpp to call initIndicator(rhi)
- [x] **Fix handle checks** - Replaced .id checks with INVALID_HANDLE comparisons
- [x] **Test** - Build successful, 76 GL calls eliminated (verified zero remaining)

### IBLSystem (engine/src/ibl_system.cpp) ✅ INFRASTRUCTURE COMPLETE
- [x] **Audit GL Calls** - 148 OpenGL calls identified (56 infrastructure + 92 generation)
- [x] **Add RHI Member** - Added m_rhi pointer, updated init() to accept RHI*
- [x] **Create RHI Shaders** - Migrated createShaders() to use RHI::createShader
- [x] **Create RHI Buffers** - Migrated setupCube/setupQuad to RHI buffers and pipelines
- [x] **Migrate Rendering** - renderCube/renderQuad use RHI::draw
- [x] **Migrate Binding** - bindIBLTextures uses RHI::bindTexture
- [x] **Migrate Cleanup** - cleanup() uses RHI destroy methods
- [x] **Update Call Sites** - RenderSystem::init() passes RHI pointer
- [x] **Test** - Build successful, infrastructure 100% RHI-based

---

## Phase 3: Texture System Migration ✅ COMPLETED (2025-10-02)

### Texture (engine/src/texture.cpp) ✅
- [x] **Audit GL Calls** - 18 OpenGL calls identified
- [x] **RHI-Only Migration** - Removed all GL fallbacks for consistency
- [x] **Migrate loadFromFile()** - Now uses RHI::createTexture exclusively, requires RHI
- [x] **Migrate loadFromKTX2()** - Uses RHI texture creation, no GL upload
- [x] **Migrate bind()** - Forwards to RHI::bindTexture
- [x] **Remove GL texture ID** - m_textureID removed, only m_rhiTex remains
- [x] **Migrate destructor** - Uses RHI::destroyTexture only
- [x] **Test** - Build successful, rendering works (sphere_basic.json verified)

### TextureCache (engine/src/texture_cache.cpp) ✅
- [x] **Remove duplicate creation** - Removed redundant RHI texture creation
- [x] **Simplify caching** - Cache calls Texture::loadFromFile which uses RHI internally
- [x] **Test** - Texture caching and reuse works via RHI (verified)

---

## Phase 4: Application Loop Migration ✅ COMPLETED (2025-10-03)

### ApplicationCore (engine/src/application_core.cpp) ✅
- [x] **Audit GL Calls** - 2 GL calls identified (glClear, glViewport)
- [x] **Migrate glClear** - Line 163-166: Replaced with m_rhi->clear()
- [x] **Migrate glViewport** - Line 519-522: Replaced with m_rhi->setViewport()
- [x] **Remove GL Fallbacks** - RHI-only implementation, no conditional GL code
- [x] **Test** - Build successful, rendering verified (zero GL calls remaining)

---

## Phase 5: IBL Cubemap Generation ✅ COMPLETED (2025-10-05)

### RHI Extensions Implementation ✅
- [x] **CubemapFace Enum** - Added PositiveX through NegativeZ (0-5)
- [x] **generateMipmaps() Interface** - Added to RHI base class
- [x] **RhiGL::generateMipmaps()** - Implemented with cubemap support
- [x] **RhiNull::generateMipmaps()** - Implemented no-op version
- [x] **RenderTarget Cubemap Attachment** - arrayLayer specifies cubemap face (0-5)
- [x] **Test** - Build successful, RHI extensions working

### IBL Generation Methods ✅
- [x] **loadHDREnvironment()** - 25 GL calls eliminated, 100% RHI-based
  - Create cubemap texture via RHI
  - Per-face render targets with arrayLayer
  - Equirect-to-cubemap conversion via RHI rendering
  - Mipmap generation via RHI::generateMipmaps()
- [x] **generateIrradianceMap()** - 20 GL calls eliminated, 100% RHI-based
  - 32x32 RGB16F cubemap creation
  - 6 face renders with convolution shader
  - On-demand RT creation/destruction pattern
- [x] **generatePrefilterMap()** - 30 GL calls eliminated, 100% RHI-based
  - 128x128 RGB16F cubemap with 5 mip levels
  - 30 render passes (5 mips × 6 faces)
  - Per-mip roughness parameter
- [x] **generateBRDFLUT()** - 17 GL calls eliminated, 100% RHI-based
  - 512x512 RG16F 2D texture
  - Single full-screen quad render
  - Integration LUT generation

### Validation ✅
- [x] **Build Verification** - Project compiles cleanly
- [x] **Runtime Testing** - IBL generation works correctly
- [x] **OpenGL Detection** - Zero GL calls in ibl_system.cpp (verified via grep)
- [x] **Visual Verification** - IBL lighting renders correctly
- [x] **Note** - Legacy Shader pointers still present for uniform setting (to be removed in Phase 6)

---

## Phase 6: Final Cleanup ⏸️ DEFERRED (Optional)

### Remove Legacy Shader Class (NOT STARTED)
- [ ] **Migrate IBLSystem uniforms** - Replace Shader::use()/setInt/setMat4 with RHI approach
- [ ] **Verify Grid Shader usage** - Check if m_shader member is actually used
- [ ] **Delete shader.h** - Remove header file
- [ ] **Delete shader.cpp** - Remove implementation (24 GL calls)
- [ ] **Update includes** - Remove #include "shader.h" from IBLSystem, Grid, others
- [ ] **Test** - Build successful, zero GL calls outside rhi_gl.cpp

### Remove setUniform* Bridge (DEFERRED TO SEPARATE TASK)
- [ ] **Audit setUniform* usage** - Find all RHI::setUniform* calls (60+ sites)
- [ ] **Design manager API** - TransformManager, MaterialManager, LightingManager approach
- [ ] **Migrate call sites** - Replace RHI::setUniform* with manager APIs
- [ ] **Remove bridge methods** - Delete RhiGL::setUniform* (lines ~1064-1089)
- [ ] **Test** - Verify all uniform updates work via managers

**Deferral Reason**: setUniform* migration requires major refactor across entire codebase (60+ call sites). Not blocking for RHI architecture goals or critical path tasks. Can be completed as separate polish task.

### Backend Swap Validation (BLOCKED)
- [ ] **Create RhiNull stub** - Minimal no-op backend for testing
- [ ] **Test backend switch** - Verify engine initializes without GL dependencies
- [ ] **Document RHI contract** - Ensure all required methods implemented
- [ ] **Final acceptance** - Migration complete when backend swap works

**Blocker**: Legacy Shader class dependency (24 GL calls). Once Phase 6 Shader cleanup is complete, backend swap validation can proceed.

---

## Final Validation Checklist

### Automated Detection ✅
- [x] **Zero OpenGL Calls in RenderSystem** - `grep "gl[A-Z]" render_system.cpp` returns only comment
- [x] **Zero OpenGL Calls in Helpers** - axisrenderer.cpp, grid.cpp, light.cpp all GL-free
- [x] **Zero OpenGL Calls in IBL** - ibl_system.cpp is GL-free
- [x] **Zero OpenGL Calls in Textures** - texture.cpp, texture_cache.cpp are GL-free
- [x] **Zero OpenGL Calls in App Loop** - application_core.cpp is GL-free
- [ ] **Zero OpenGL Calls Repo-Wide** - 24 GL calls remain in shader.cpp (deferred)

### Functional Testing ✅
- [x] **Raster Pipeline** - Verified raster rendering works with pure RHI
- [x] **Ray Pipeline** - Verified ray integration works with RHI textures
- [x] **Offscreen Rendering** - Verified headless rendering via RHI (renderToPNG)
- [x] **MSAA Support** - Verified multi-sampling works through RHI
- [x] **IBL Lighting** - Verified IBL generation and rendering works

### Documentation ✅
- [x] **Migration Log** - Documented in progress.ndjson with all phases
- [x] **RHI Coverage** - Documented in README.md and artifacts
- [x] **Performance Impact** - No significant regressions observed
- [x] **Future Extensions** - Phase 6 optional work documented

---

## Completion Status

### Achieved (94% - 426/454 GL calls eliminated) ✅
1. ✅ RenderSystem has zero direct GL calls (146 eliminated)
2. ✅ All helper systems use RHI exclusively (112 eliminated)
3. ✅ IBL generation fully migrated to RHI (92 eliminated)
4. ✅ Texture system 100% RHI-based (18 eliminated)
5. ✅ Application loop uses RHI exclusively (2 eliminated)
6. ✅ Rendering works correctly via RHI (visual validation)
7. ✅ Performance maintained (no regressions)

### Deferred (6% - 24 GL calls in Shader class) ⏸️
1. ⏸️ Legacy Shader class removal (24 GL calls in shader.cpp)
2. ⏸️ Backend swap validation (blocked by Shader dependency)
3. ⏸️ setUniform* bridge removal (60+ call sites, separate task)

---

## Recommendation

**OpenGL migration is functionally complete** for all core rendering systems. The remaining 24 GL calls are isolated in the legacy Shader class and don't block RHI architecture goals or downstream tasks.

**Next Steps**:
- **Recommended**: Proceed to `pass_bridging` task on critical path
- **Alternative**: Complete Phase 6 Shader cleanup (2-4 hours) for 100% achievement

**Decision**: Recommend proceeding to critical path. Phase 6 can be completed as optional polish work if/when backend swap becomes a priority.

---

*Last verification: 2025-10-07 via `grep -rn "gl[A-Z]" engine/src/*.cpp`*
