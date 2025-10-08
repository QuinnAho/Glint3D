# RHI Cleanup Implementation Checklist

## Phase 1: RHI Internals Cleanup

### 1.1 GLPipelineStateCache Implementation
- [ ] Create `struct GLPipelineState` with fields: `polygonMode`, `blendEnable`, `blendFactors[4]`, `depthTest`, `depthWrite`, `polygonOffset[3]`, `lineWidth`
- [ ] Add `GLPipelineState m_currentState` member to `RhiGL` class
- [ ] Implement `void applyPipelineState(const PipelineDesc& desc)` helper that:
  - [ ] Compares `desc` fields against `m_currentState`
  - [ ] Only emits GL calls for changed fields
  - [ ] Updates `m_currentState` after applying changes
- [ ] Refactor `RhiGL::draw()` to call `applyPipelineState()` instead of unconditional GL calls
- [ ] Test: Verify redundant state changes eliminated via GPU trace or call logging

### 1.2 GLPipelineManager Extraction
- [ ] Create `engine/src/rhi/gl_pipeline_manager.h` and `.cpp`
- [ ] Define `class GLPipelineManager` with methods:
  - [ ] `void setupVertexArray(GLuint vao, const PipelineDesc& desc)`
  - [ ] `GLenum polygonModeToGL(PolygonMode mode)`
  - [ ] `GLenum blendFactorToGL(BlendFactor factor)`
  - [ ] `GLenum primitiveTopologyToGL(PrimitiveTopology topology)`
- [ ] Move VAO setup logic from `RhiGL::createPipeline()` to `GLPipelineManager::setupVertexArray()`
- [ ] Move conversion helpers from `RhiGL` to `GLPipelineManager`
- [ ] Update `RhiGL` to own `std::unique_ptr<GLPipelineManager> m_pipelineManager`
- [ ] Delegate VAO/state operations to `m_pipelineManager` in `RhiGL::createPipeline()`
- [ ] Test: Verify pipeline creation and VAO setup still works correctly

### 1.3 GLUniformRing Extraction
- [ ] Create `engine/src/rhi/gl_uniform_ring.h` and `.cpp`
- [ ] Define `class GLUniformRing` with methods:
  - [ ] `bool init(size_t bufferSize, uint32_t alignment)`
  - [ ] `void shutdown()`
  - [ ] `UniformAllocation allocate(const UniformAllocationDesc& desc)`
  - [ ] `void reset()` (for per-frame reset)
- [ ] Move uniform ring allocator logic from `RhiGL` to `GLUniformRing`
- [ ] Update `RhiGL` to own `std::unique_ptr<GLUniformRing> m_uniformRing`
- [ ] Delegate uniform allocation calls to `m_uniformRing`
- [ ] Test: Verify uniform buffer allocation and updates work correctly

### 1.4 RhiGL Simplification
- [ ] Remove implementation details from `RhiGL` that were moved to helpers
- [ ] Update `RhiGL::init()` to initialize sub-systems (`m_pipelineManager`, `m_uniformRing`)
- [ ] Update `RhiGL::shutdown()` to clean up sub-systems
- [ ] Verify `RhiGL` is now primarily a coordinator class
- [ ] Test: Full render test to ensure everything still works

## Phase 2: Pipeline Consistency

### 2.1 Material-Driven Pipeline Creation
- [ ] Read `PipelineManager::createPbrPipeline()` and `RenderSystem::ensureObjectPipeline()`
- [ ] Add `const MaterialCore& material` parameter to `PipelineManager::createPbrPipeline()`
- [ ] Move transparency/blending logic from `RenderSystem::ensureObjectPipeline()` to `PipelineManager::createPbrPipeline()`:
  - [ ] Check `material.transmission > 0.01f || material.baseColor.a < 0.999f`
  - [ ] Set `desc.blendEnable`, `desc.blendFactors`, `desc.depthWriteEnable` accordingly
- [ ] Update call sites to pass `materialCore` when creating PBR pipelines
- [ ] Test: Verify transparent materials render with correct blending

### 2.2 Material Traits in Cache Keys
- [ ] Extend `PipelineManager::generatePipelineKey()` to include material transparency trait
- [ ] Add `bool isTransparent` flag to key based on transmission/alpha
- [ ] Update cache key struct/hash to incorporate transparency flag
- [ ] Test: Toggle material alpha and verify pipeline cache miss triggers rebuild

### 2.3 Pipeline Invalidation API
- [ ] Add `void markDirty(SceneObject& obj)` to `PipelineManager`
- [ ] Implementation clears `obj.rhiPipelinePbr` and evicts key from `m_pipelineCache`
- [ ] Document usage: "Call when material properties change at runtime"
- [ ] Test: Change material transmission, call `markDirty()`, verify pipeline recreates

### 2.4 Batched Render Path Audit
- [ ] Audit `RenderSystem::renderObjectsBatched()` (line ~2213)
- [ ] Ensure it calls `PipelineManager::ensureObjectPipeline()` not `RenderSystem::ensureObjectPipeline()`
- [ ] Remove `RenderSystem::ensureObjectPipeline()` once `PipelineManager` is authoritative
- [ ] Test: Batched rendering with mixed opaque/transparent objects

## Phase 3: Manager-Centric Draw Flow

### 3.1 Manager-Based Uniform Updates
- [ ] Identify direct `m_rhi->setUniform*()` calls in render passes (G-buffer, batched, etc.)
- [ ] Replace with manager APIs:
  - [ ] `TransformManager::updateModel(mat4)` for model matrix
  - [ ] `MaterialManager::bindMaterialUniforms(material)` for material data
  - [ ] `LightingManager::updateLights(lights)` for lighting data
- [ ] Test: Verify uniforms still update correctly via managers

### 3.2 TextureBinder Helper
- [ ] Create `engine/include/texture_binder.h` and `engine/src/texture_binder.cpp`
- [ ] Define `class TextureBinder` with:
  - [ ] `void bindObjectTextures(const SceneObject& obj, uint32_t startSlot)`
  - [ ] Converts object's texture set to bind group or descriptor
  - [ ] Handles albedo, normal, metallic/roughness textures
- [ ] Update render passes to use `TextureBinder` instead of manual `m_rhi->bindTexture()`
- [ ] Test: Verify textures bind correctly via helper

### 3.3 Remove Legacy RHI Uniform Helpers
- [ ] Remove `RhiGL::setUniformMat4()` (line ~1064)
- [ ] Remove `RhiGL::setUniformVec3()`, `setUniformVec4()`, `setUniformFloat()`, `setUniformInt()`, `setUniformBool()`
- [ ] Ensure no remaining call sites after manager migration
- [ ] Test: Build succeeds, all render paths work

### 3.4 Command Encoder Prototype
- [ ] Implement `RHI::createCommandEncoder()` stub (returns simple struct)
- [ ] Wrap G-buffer pass to use command encoder pattern:
  - [ ] `encoder = m_rhi->createCommandEncoder()`
  - [ ] `renderPass = encoder->beginRenderPass(desc)`
  - [ ] Record draws via `renderPass->draw()`
  - [ ] `commandBuffer = encoder->finish()`
- [ ] Document command encoder API for future backends
- [ ] Test: G-buffer pass renders correctly via encoder

## Validation

### Build and Test
- [ ] Full build succeeds (Debug and Release)
- [ ] Render test: `glint --ops examples/json-ops/sphere_basic.json --render test_rhi_cleanup.png`
- [ ] Visual inspection: Rendering output unchanged
- [ ] Performance test: Frame time stable or improved

### Code Quality
- [ ] All new code documented with clear comments
- [ ] Helper classes have single, well-defined responsibilities
- [ ] No memory leaks (verify with sanitizers if available)
- [ ] Consistent naming and style with existing codebase

### Progress Tracking
- [ ] Update `progress.ndjson` after each major milestone
- [ ] Update `current_index.json` with new status
- [ ] Mark task as complete in `task.json` when done