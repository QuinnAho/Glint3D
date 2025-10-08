# RHI Cleanup Coverage Analysis

## Scope Definition

This task refactors the RHI implementation to improve performance, modularity, and maintainability by introducing state caching, extracting helper modules, and establishing manager-centric architecture.

## Files Modified

### Core RHI Implementation
- **`engine/src/rhi/rhi_gl.cpp`** (primary refactor target)
  - Lines 100-180: `draw()` method with redundant state emission
  - Lines 257-350: `createPipeline()` with embedded VAO setup
  - Lines 697-722: Conversion helpers (polygonModeToGL, blendFactorToGL)
  - Lines 742-800: VAO setup logic
  - Lines 1064-1089: Legacy setUniform* helpers (to be removed)
  - Lines 1231+: Uniform ring allocator logic (to be extracted)

- **`engine/include/rhi/rhi_gl.h`**
  - Add forward declarations for GLPipelineManager, GLUniformRing
  - Add `GLPipelineState m_currentState` member
  - Add `std::unique_ptr<GLPipelineManager>`, `std::unique_ptr<GLUniformRing>` members
  - Remove setUniform* method declarations

### New Helper Modules
- **`engine/src/rhi/gl_pipeline_manager.h`** (new file)
  - Class declaration for VAO setup and state translation

- **`engine/src/rhi/gl_pipeline_manager.cpp`** (new file)
  - Implementation of VAO setup
  - Pipeline state conversion helpers (polygonModeToGL, etc.)
  - Estimated ~200 LOC

- **`engine/src/rhi/gl_uniform_ring.h`** (new file)
  - Class declaration for uniform buffer ring allocator

- **`engine/src/rhi/gl_uniform_ring.cpp`** (new file)
  - Uniform allocation, reset, cleanup logic
  - Estimated ~150 LOC

### Pipeline Manager Integration
- **`engine/src/managers/pipeline_manager.cpp`**
  - Line 239+: `createPbrPipeline()` - add material parameter and blending logic
  - Line 316+: `generatePipelineKey()` - include material transparency trait
  - Add `markDirty()` method for pipeline invalidation
  - Estimated ~80 LOC added/modified

- **`engine/include/managers/pipeline_manager.h`**
  - Add `markDirty(SceneObject&)` declaration
  - Update `createPbrPipeline()` signature

### Render System Updates
- **`engine/src/render_system.cpp`**
  - Lines 63-88: Remove `ensureObjectPipeline()` or delegate to PipelineManager
  - Lines 1839, 2230, 2235: Replace direct RHI uniform calls with manager APIs
  - Line 2213+: Audit batched render path
  - Estimated ~100 LOC modified

### New Helper: TextureBinder
- **`engine/include/texture_binder.h`** (new file)
  - Declaration for texture binding helper

- **`engine/src/texture_binder.cpp`** (new file)
  - Implementation of texture binding abstraction
  - Estimated ~80 LOC

## Affected Systems

### Direct Impact
- **RHI Layer**: Core refactor with state caching and modularization
- **Pipeline Management**: Material-aware pipeline creation
- **Uniform Management**: Centralized through managers instead of direct RHI
- **Texture Binding**: Abstracted through TextureBinder helper

### Indirect Impact
- **Render Performance**: Reduced redundant GL state changes
- **Code Maintainability**: Better separation of concerns
- **Backend Portability**: Easier to add Vulkan/WebGPU backends
- **Material System**: Runtime material changes trigger proper pipeline updates

## Testing Coverage

### Unit Testing
- [ ] GLPipelineStateCache: State comparison and change detection
- [ ] GLPipelineManager: VAO setup with various vertex layouts
- [ ] GLUniformRing: Allocation, reset, overflow handling
- [ ] PipelineManager: Material-driven pipeline creation and cache keys

### Integration Testing
- [ ] Full render with state cache: Verify output unchanged
- [ ] Material transparency toggle: Verify pipeline rebuild
- [ ] Manager-based uniforms: Verify all render paths work
- [ ] Command encoder prototype: Verify G-buffer pass works

### Performance Testing
- [ ] Capture GPU trace before/after to confirm state change reduction
- [ ] Frame time comparison with complex scenes
- [ ] Pipeline cache hit rate measurement

## Risk Assessment

### High Risk
- **State Cache Bugs**: Incorrect state comparison could cause rendering artifacts
  - *Mitigation*: Comprehensive testing with varied pipeline states

- **Manager Integration Breakage**: Moving uniform logic could break existing paths
  - *Mitigation*: Incremental migration with validation at each step

### Medium Risk
- **Pipeline Cache Invalidation**: Incorrect cache keys could cause stale pipelines
  - *Mitigation*: Include all relevant material traits in keys

- **Memory Management**: New helper classes require proper lifecycle
  - *Mitigation*: Use RAII and smart pointers

### Low Risk
- **Command Encoder Prototype**: Limited scope as proof-of-concept
  - *Mitigation*: Don't migrate all passes, just demonstrate pattern

## Success Metrics

### Performance
- **Target**: 20-40% reduction in redundant GL calls (measured via GPU trace)
- **Baseline**: Current implementation emits ~30-50 state changes per draw
- **Goal**: State cache reduces to ~5-10 changes per draw (only actual changes)

### Code Quality
- **Modularity**: RhiGL class size reduced by ~40% (delegate to helpers)
- **Maintainability**: Clear separation between VAO, state, uniform concerns
- **Reusability**: Helper modules usable for future backends

### Functional
- **Correctness**: All existing render paths produce identical output
- **Material Responsiveness**: Runtime material changes reflect immediately
- **Manager Integration**: Zero direct RHI uniform/texture calls in render system

## Rollback Plan

If critical issues arise:
1. **Phase 1 Rollback**: Revert state cache, keep old draw() implementation
2. **Phase 2 Rollback**: Keep RenderSystem::ensureObjectPipeline(), defer manager integration
3. **Phase 3 Rollback**: Keep direct RHI calls, defer manager migration

Each phase is independently reversible with minimal impact on other phases.