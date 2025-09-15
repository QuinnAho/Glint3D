# FEAT-0249 — Uniform System Migration (UBO ring + reflection)

## Intent
Migrate all uniforms to a reflection-validated UBO ring allocator. Remove per-draw `glUniform*` and validate offsets against shader reflection data.

## Acceptance
- 100% migration to UBO ring allocator
- Reflection-validated offsets with runtime asserts
- No `glUniform*` outside GL backend
- Golden tests pass without regressions

## Scope
- Headers: `engine/include/glint3d/rhi*.h`
- Impl: `engine/src/rhi/**`
- Tests: `tests/unit/rhi_test.cpp`
- Docs: `docs/rhi_architecture.md`

## Links
- Spec: `ai/tasks/FEAT-0249/spec.yaml`
- Passes: `ai/tasks/FEAT-0249/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0249/whitelist.txt`

## Progress (AI-Editable Section)

**Status**: In Progress (2025-09-15)
**Current PR**: PR1 - UBO Ring + Reflection Layer

### Current State Analysis
- **Existing System**: RHI uses direct `glUniform*` calls in legacy bridge mode
- **Usage Count**: 141 `setUniform*` calls across 7 files need migration
- **Files with Heavy Usage**:
  - `render_system.cpp` (105 calls)
  - `light.cpp` (9 calls)
  - `material.cpp` (8 calls)
  - `rhi_gl.cpp` (7 calls - implementation)
  - `shader.cpp` (6 calls)
  - `gizmo.cpp` (3 calls)
  - `axisrenderer.cpp` (3 calls)

### Implementation Plan
**PR1**: UBO Ring + Reflection Layer
- [x] Add UBO ring allocator to RHI interface
- [x] Implement shader reflection using OpenGL introspection (temporary, to be enhanced with SPIRV-Reflect)
- [x] Add offset validation system
- [x] Create new UBO-based uniform setting methods
- [x] Implement in RhiGL backend

**PR2**: Migration of Call Sites
- [x] Migrate render_system.cpp (largest consumer) - **COMPLETED**
- [ ] Migrate material and lighting systems
- [ ] Migrate remaining files
- [ ] Remove legacy setUniform* methods
- [ ] Validate golden tests pass

### PR2 Progress Update (September 15, 2025)

**Major Milestone**: RenderSystem successfully migrated to UBO system!

**Implementation Details:**
- **Shader Updates**: Modified `pbr.vert`, `pbr.frag`, `standard.vert`, `standard.frag`, `axis.vert` to use `layout(std140)` uniform blocks
- **Uniform Block Structures**: Created `TransformBlock`, `LightingBlock`, `MaterialBlock` with proper std140 alignment
- **RenderSystem Integration**: Added UBO allocation, update, and binding methods
- **Pipeline Integration**: UBOs automatically bound when shaders are activated
- **Build Status**: ✅ Compiles successfully, no build errors

**Architecture Achievement:**
```cpp
// Old approach (141 calls across codebase):
m_rhi->setUniformMat4("model", modelMatrix);
m_rhi->setUniformMat4("view", viewMatrix);
m_rhi->setUniformMat4("projection", projMatrix);

// New approach (UBO-based, efficient):
updateTransformUniforms();      // Populates UBO once per frame
bindUniformBlocks();           // Binds all UBOs when shader changes
// All transform, lighting, and material data sent via efficient UBOs
```

**Performance Benefits:**
- **Batch Updates**: Transform/lighting/material data updated once per frame vs per-draw
- **Efficient Binding**: UBOs bound once per shader vs per-uniform
- **Memory Efficiency**: std140 layout ensures optimal GPU memory usage
- **Future-Ready**: Foundation for FEAT-0254 (SPIRV-Reflect) integration

### Implementation Status (PR1 - COMPLETED)

**Completed Components:**
1. **RHI Interface Extensions** (`engine/include/glint3d/rhi.h`):
   - Added `UniformAllocation`, `UniformBlockReflection`, `ShaderReflection` types
   - Added UBO ring allocator methods: `allocateUniforms()`, `freeUniforms()`
   - Added reflection-based uniform setters: `setUniformInBlock()`, `setUniformsInBlock()`
   - Added shader reflection query: `getShaderReflection()`

2. **OpenGL Backend Implementation** (`engine/src/rhi/rhi_gl.cpp`):
   - Implemented 1MB persistent-mapped UBO ring buffer
   - Added OpenGL-based shader reflection using `glGetActiveUniformBlock*` APIs
   - Implemented offset-validated uniform setting with automatic type checking
   - Integrated ring buffer lifecycle into RhiGL init/shutdown

3. **Null Backend Stubs** (`engine/include/rhi/rhi_null.h`):
   - Added no-op implementations for all new UBO methods
   - Ensures testing backend compatibility

**Technical Details:**
- **Ring Buffer**: 1MB persistent-mapped UBO with 256-byte alignment (UBO std140 compliant)
- **Reflection**: Real-time OpenGL introspection of uniform blocks and variables
- **Validation**: Runtime offset and size checking against shader reflection data
- **Performance**: Zero-copy uniform updates via persistent mapping (OpenGL 4.4+)

**Build Status**: ✅ Compiles successfully, all new code integrated

### Next Steps (PR2)
1. Migrate call sites from legacy `setUniform*` to UBO ring system
2. Start with `render_system.cpp` (105 calls - largest consumer)
3. Enhance reflection with SPIRV-Reflect for cross-platform consistency
4. Remove legacy uniform methods once migration complete

### PR2 Updates (follow-up)

- Added `bindUniformBlock()` to RHI, implemented in OpenGL and Null backends.
- Removed direct `glUniform*` fallbacks from non-RHI code:
  - Updated `engine/src/shader.cpp` to route only via RHI.
  - Updated `engine/src/gizmo.cpp` and `engine/src/axisrenderer.cpp` to remove direct GL uniform calls.
- Updated docs (`docs/rhi_architecture.md`, `docs/rhi_guide.md`) with UBO ring + reflection flow and examples.
