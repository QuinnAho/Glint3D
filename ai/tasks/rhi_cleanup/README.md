# RHI Architecture Cleanup and Manager-Centric Design

## Overview

This task refactors the RHI implementation to eliminate redundant state changes, modularize internal helpers, and establish a manager-centric architecture where all uniform/texture binding goes through manager APIs rather than direct RHI calls.

## Problem Statement

After completing the OpenGL migration (146 → 0 direct GL calls), the RHI implementation has several architectural issues:

1. **Redundant State Changes**: `RhiGL::draw()` emits all pipeline state GL calls on every draw, even when state hasn't changed
2. **Monolithic RhiGL**: VAO setup, shader binding, state management, and uniform allocation all mixed in one class
3. **Ad-hoc Pipeline Creation**: `RenderSystem::ensureObjectPipeline()` manually configures transparency/blending instead of using `PipelineManager`
4. **Direct RHI Calls**: Render passes call `m_rhi->setUniformMat4()` and `bindTexture()` directly instead of using managers
5. **No Command Encoder Pattern**: Render passes interact with RHI directly, not through command encoders

## Goals

### Phase 1: RHI Internals Cleanup
- Introduce `GLPipelineStateCache` to track current GL state and only emit changes
- Extract `GLPipelineManager` module for VAO setup and pipeline state translation
- Carve out `GLUniformRing` helper for uniform buffer allocation
- Keep `RhiGL` as thin coordinator delegating to sub-systems

### Phase 2: Pipeline Consistency
- Move material-driven pipeline logic from `RenderSystem::ensureObjectPipeline()` to `PipelineManager::createPbrPipeline()`
- Include material traits (transparent/opaque) in pipeline cache keys
- Add `PipelineManager::markDirty()` API for runtime material changes
- Ensure batched render path uses `PipelineManager` exclusively

### Phase 3: Manager-Centric Draw Flow
- Replace direct `m_rhi->setUniform*()` calls with manager APIs (`TransformManager::updateModel()`, etc.)
- Create `TextureBinder` helper to convert texture sets to bind groups
- Delete legacy `setUniform*` helpers from `RhiGL`
- Prototype command encoder pattern for G-buffer pass

## Success Criteria

- [ ] GL state changes only emitted when state actually differs
- [ ] `PipelineManager` creates material-aware pipelines with proper blending
- [ ] Pipeline cache keys include material transparency trait
- [ ] All uniform updates go through managers, not RHI
- [ ] At least one render pass uses command encoder pattern
- [ ] Legacy RHI uniform helpers removed

## Dependencies

- **Prerequisite**: `opengl_migration` (completed)
- **Enables**: Cleaner Vulkan/WebGPU backend implementations
- **Blocks**: None (architectural improvement)

## Testing Strategy

1. **State Cache Validation**: GPU traces confirm redundant GL calls eliminated
2. **Material Transparency**: Runtime material alpha changes trigger pipeline rebuild
3. **Manager Integration**: All uniform writes traced to manager APIs
4. **Performance**: Frame time remains stable or improves with state caching

## References

- `engine/src/rhi/rhi_gl.cpp:130-156` - Current redundant state emission
- `engine/src/render_system.cpp:63-88` - Ad-hoc pipeline transparency logic
- `engine/src/managers/pipeline_manager.cpp:239` - Pipeline creation entry point
- `engine/src/render_system.cpp:2213+` - Batched render path needing manager integration