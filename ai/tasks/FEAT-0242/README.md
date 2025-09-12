# FEAT-0242 — RHI Abstraction Layer Implementation

## Intent (human summary)
Implement Render Hardware Interface (RHI) abstraction to eliminate direct OpenGL calls from Engine Core. This enables future Vulkan/WebGPU backends and addresses the "Backend Lock-in" problem identified in the refactoring plan by providing a clean abstraction layer.

## AI-Editable: Status
- Overall: in_progress
- Current PR: PR1 (in_progress)
- Last Updated: 2025-09-11T21:40:00Z

## AI-Editable: Checklist (mirror of spec.yaml acceptance_criteria)
- [x] RHI interface headers defined with comprehensive API
- [x] Public API headers created in engine/include/glint3d/
- [x] Type-safe resource handles and descriptors implemented
- [x] OpenGL backend implementation exists (RhiGL class)
- [x] Null backend for testing implemented (RhiNull class)
- [ ] WebGL2 compatibility validated
- [ ] Direct GL calls audited and catalogued outside RHI
- [ ] Performance overhead measured (<5% target)
- [ ] Cross-platform testing completed

## AI-Editable: Current PR Progress (PR1: RHI Interface Definition)
- [x] Define RHI interface base class with pure virtual methods
- [x] Define comprehensive type system (handles, descriptors, enums)
- [x] Create public API headers in engine/include/glint3d/
- [x] Ensure absolute path includes for public headers
- [ ] Validate existing RhiGL implementation against new public API
- [ ] Create unit tests for RHI interface validation
- [ ] Document RHI architecture and usage patterns

## AI-Editable: Implementation Notes

### Current State Analysis
The codebase already has a substantial RHI implementation:

**Existing Implementation:**
- `engine/include/rhi/rhi.h` - Base RHI interface (internal)
- `engine/include/rhi/rhi_gl.h` - OpenGL backend header
- `engine/include/rhi/rhi_null.h` - Null backend for testing
- `engine/src/rhi/rhi.cpp` - Factory function implementation
- `engine/src/rhi/rhi_gl.cpp` - OpenGL backend implementation
- `engine/src/rhi/rhi_null.cpp` - Null backend implementation

**New Public API Created:**
- `engine/include/glint3d/rhi.h` - Public RHI interface with documentation
- `engine/include/glint3d/rhi_types.h` - Public type definitions in glint3d namespace

### Design Decisions Made
- **Namespace Isolation**: Public API uses `glint3d::` namespace vs internal classes
- **Documentation**: Added comprehensive Doxygen documentation to public headers
- **Type Safety**: Maintained opaque handle system with INVALID_HANDLE constants
- **Backend Support**: Planned for OpenGL, WebGL2, Vulkan, WebGPU with auto-detection
- **Factory Pattern**: createRHI() and createDefaultRHI() for backend instantiation

### Integration Tasks Remaining
- **API Alignment**: Ensure existing RhiGL implementation matches public interface
- **Migration Path**: Update RenderSystem to use public glint3d::RHI interface
- **Testing Strategy**: Create comprehensive RHI unit tests
- **Performance Validation**: Benchmark overhead vs direct GL calls

## AI-Editable: Blockers and Risks

### Current Blockers
- Need to validate compatibility between existing internal RHI and new public API
- Existing RenderSystem may have direct GL calls that need cataloging
- Cross-platform WebGL2 testing not yet completed

### Risk Assessment
- **API Mismatch**: Internal vs public RHI interfaces may have subtle differences
- **Performance Overhead**: Abstraction layer might exceed 5% overhead target
- **WebGL2 Limitations**: Some features may not translate cleanly to WebGL2
- **Migration Complexity**: RenderSystem migration may reveal unexpected GL dependencies

## AI-Editable: Next Steps
1. **Audit Phase**: Catalog all direct GL calls in engine/src/render_system.cpp
2. **API Validation**: Ensure RhiGL implements the public interface correctly  
3. **Test Creation**: Write comprehensive unit tests for RHI backends
4. **Integration**: Begin RenderSystem migration to use public RHI interface
5. **Performance Testing**: Benchmark abstraction overhead

## Links
- Spec: [ai/tasks/FEAT-0242/spec.yaml](spec.yaml)
- Passes: [ai/tasks/FEAT-0242/passes.yaml](passes.yaml)
- Progress Log: [ai/tasks/FEAT-0242/progress.ndjson](progress.ndjson)
- Architecture Context: [CLAUDE.md](../../CLAUDE.md#rendering-system-refactoring-plan)
- Existing RHI: engine/include/rhi/ and engine/src/rhi/