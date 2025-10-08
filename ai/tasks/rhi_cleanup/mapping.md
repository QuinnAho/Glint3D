# RHI Cleanup Task Mapping

## File-to-Phase Mapping

### Phase 1: RHI Internals Cleanup

#### GLPipelineStateCache
- **Target**: `engine/src/rhi/rhi_gl.cpp:130-156`
- **Action**: Add state tracking and conditional emission
- **Files**:
  - `engine/include/rhi/rhi_gl.h` - Add `GLPipelineState m_currentState`
  - `engine/src/rhi/rhi_gl.cpp` - Implement `applyPipelineState()`

#### GLPipelineManager
- **Source**: `engine/src/rhi/rhi_gl.cpp:257, 742, 751`
- **Target**: New module `engine/src/rhi/gl_pipeline_manager.{h,cpp}`
- **Extract**:
  - VAO setup logic from `createPipeline()`
  - Conversion helpers: `polygonModeToGL()`, `blendFactorToGL()`, `primitiveTopologyToGL()`
  - `setupVertexArray()` method

#### GLUniformRing
- **Source**: `engine/src/rhi/rhi_gl.cpp:1231+`
- **Target**: New module `engine/src/rhi/gl_uniform_ring.{h,cpp}`
- **Extract**:
  - Uniform buffer allocation logic
  - Ring buffer management
  - Per-frame reset functionality

### Phase 2: Pipeline Consistency

#### Material-Driven Pipelines
- **Source**: `engine/src/render_system.cpp:63-88` (`ensureObjectPipeline`)
- **Target**: `engine/src/managers/pipeline_manager.cpp:239` (`createPbrPipeline`)
- **Migration**:
  - Move transparency detection logic
  - Move blending configuration
  - Add material parameter to pipeline creation

#### Cache Key Enhancement
- **Target**: `engine/src/managers/pipeline_manager.cpp:316` (`generatePipelineKey`)
- **Action**: Include material transparency trait in key
- **Impact**: Pipeline cache respects material state changes

#### Pipeline Invalidation
- **Target**: `engine/include/managers/pipeline_manager.h`
- **Action**: Add `markDirty(SceneObject&)` method
- **Usage**: Call when material properties change at runtime

### Phase 3: Manager-Centric Draw Flow

#### Manager-Based Uniforms
- **Targets**:
  - `engine/src/render_system.cpp:1839` - G-buffer pass uniform updates
  - `engine/src/render_system.cpp:2230, 2235` - Batched pass uniform updates
- **Replace**: `m_rhi->setUniform*()` → Manager APIs
- **Managers**:
  - `TransformManager::updateModel()`
  - `MaterialManager::bindMaterialUniforms()`
  - `LightingManager::updateLights()`

#### TextureBinder
- **Target**: New module `engine/include/texture_binder.h`, `engine/src/texture_binder.cpp`
- **Purpose**: Abstract texture binding from render system
- **Usage**: Replace manual `m_rhi->bindTexture()` calls

#### Legacy Cleanup
- **Remove**: `engine/src/rhi/rhi_gl.cpp:1064-1089`
- **Methods**: All `setUniform*()` helpers
- **Justification**: All uniform updates now go through managers

#### Command Encoder Prototype
- **Target**: `engine/include/glint3d/rhi.h` - Add encoder interfaces
- **Implementation**: `engine/src/rhi/rhi_gl.cpp` - Minimal encoder stub
- **Demo**: Wrap one render pass (G-buffer) to show pattern

## Dependency Graph

```
Phase 1: RHI Internals
├── GLPipelineStateCache (independent)
├── GLPipelineManager (independent)
└── GLUniformRing (independent)
        ↓
Phase 2: Pipeline Consistency
├── Material-Driven Pipelines (depends on Phase 1 complete)
├── Cache Key Enhancement (depends on material pipelines)
└── Pipeline Invalidation (depends on cache keys)
        ↓
Phase 3: Manager-Centric Flow
├── Manager-Based Uniforms (depends on Phase 2)
├── TextureBinder (independent)
├── Legacy Cleanup (depends on manager uniforms)
└── Command Encoder (independent, can be parallel)
```

## Critical Path

1. **GLPipelineStateCache** - Highest performance impact, implement first
2. **GLPipelineManager** - Required before material pipeline work
3. **Material-Driven Pipelines** - Core architectural change
4. **Manager-Based Uniforms** - Large refactor, needs careful testing
5. **Legacy Cleanup** - Final step after all migrations complete

## Testing Checkpoints

- After Phase 1: Full render test, GPU trace validation
- After Phase 2: Material toggle test, pipeline cache test
- After Phase 3: Manager integration test, command encoder demo

## LOC Estimates

- **Phase 1**: ~500 LOC (250 new, 250 modified)
- **Phase 2**: ~300 LOC (50 new, 250 modified)
- **Phase 3**: ~400 LOC (150 new, 250 modified)
- **Total**: ~1200 LOC

## Timeline Estimate

- **Phase 1**: 4-6 hours (state cache + helper extraction)
- **Phase 2**: 2-3 hours (pipeline manager integration)
- **Phase 3**: 3-4 hours (manager migration + encoder prototype)
- **Total**: 9-13 hours for complete implementation