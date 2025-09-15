# FEAT-0254 — SPIRV-Reflect Shader Reflection System

## Intent
Replace the current OpenGL-based shader reflection system with SPIRV-Reflect for true cross-platform compatibility and enhanced reflection capabilities. This enables backend-agnostic uniform buffer validation and prepares for Vulkan/WebGPU backends.

## Acceptance
- SPIRV-Reflect successfully integrated as engine dependency
- Shader reflection computed from SPIR-V bytecode, not OpenGL introspection
- Cross-platform parity across OpenGL, WebGL2, and future backends
- Enhanced reflection supporting storage buffers, push constants, and texture bindings
- Backward compatibility maintained with existing UBO ring allocator
- Performance optimized with cached reflection, not per-frame computation

## Scope
- Dependencies: Add SPIRV-Reflect to build system
- Compilation: Integrate SPIR-V shader compilation (DXC/shaderc)
- Reflection: Replace `RhiGL::createShaderReflection()` implementation
- Enhancement: Extend reflection beyond basic uniform blocks
- Testing: Validate cross-platform reflection consistency

## Priority: Medium
This enhances the foundation built in FEAT-0249 but is not critical for core functionality.

## Dependencies
- **Blocked by**: FEAT-0249 (UBO ring allocator foundation)
- **Blocks**: FEAT-0252 (WebGPU backend needs SPIR-V reflection)

## Context
FEAT-0249 implemented a functional UBO ring allocator using OpenGL's `glGetActiveUniformBlock*` APIs for reflection. While this works for OpenGL/WebGL2, it has limitations:

### Current OpenGL Reflection Limitations:
1. **Backend Lock-in**: Uses GL-specific introspection APIs
2. **Limited Scope**: Only uniform blocks, no storage buffers or push constants
3. **Runtime Only**: No compile-time validation or optimization
4. **WebGPU Incompatible**: Cannot reflect WGSL or SPIR-V directly

### SPIRV-Reflect Advantages:
1. **Universal**: Works with any SPIR-V bytecode regardless of target API
2. **Comprehensive**: Reflects all resource types (UBOs, SSBOs, textures, samplers)
3. **Compile-time**: Enables shader validation and optimization during build
4. **Future-proof**: Essential for Vulkan and WebGPU backend implementation

## Links
- Spec: `ai/tasks/FEAT-0254/spec.yaml`
- Passes: `ai/tasks/FEAT-0254/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0254/whitelist.txt`

## Progress (AI-Editable Section)

**Status**: Planned (2025-09-15)
**Current PR**: Not Started

### Technical Architecture

**SPIR-V Compilation Pipeline:**
```
GLSL Source → DXC/shaderc → SPIR-V Bytecode → SPIRV-Reflect → Reflection Data
```

**Integration Points:**
1. **Shader Creation**: `RhiGL::createShader()` compiles to SPIR-V first
2. **Reflection Generation**: `SPIRV-Reflect` processes bytecode
3. **Cross-compilation**: `SPIRV-Cross` converts SPIR-V to target API (GLSL/HLSL/MSL)
4. **Caching**: Reflection data cached for performance

**Enhanced Reflection Types:**
```cpp
struct EnhancedShaderReflection {
    std::vector<UniformBlockReflection> uniformBlocks;    // Existing
    std::vector<StorageBufferReflection> storageBuffers;  // New
    std::vector<PushConstantReflection> pushConstants;    // New
    std::vector<TextureBindingReflection> textures;       // New
    std::vector<SamplerBindingReflection> samplers;       // New
    bool isValid = false;
};
```

### Implementation Strategy

**Phase 1: Foundation (PR1)**
- Add SPIRV-Reflect as external dependency
- Integrate with CMake build system
- Create basic wrapper classes

**Phase 2: Compilation (PR2)**
- Add DXC/shaderc for GLSL → SPIR-V compilation
- Modify shader loading pipeline
- Add SPIR-V caching system

**Phase 3: Reflection Migration (PR3)**
- Replace OpenGL reflection in `RhiGL::createShaderReflection()`
- Maintain API compatibility with existing UBO system
- Add comprehensive testing

**Phase 4: Enhancement (PR4)**
- Extend reflection for storage buffers and push constants
- Add texture/sampler binding reflection
- Prepare for advanced rendering techniques

### Dependency Integration

**SPIRV-Reflect** (Khronos Group):
- Header-only C library
- MIT license
- Minimal dependencies
- ~50KB addition to build

**DXC** (Microsoft DirectX Shader Compiler):
- Official HLSL → SPIR-V compiler
- Used by major engines (Unreal, Unity)
- Cross-platform support

**Alternative: shaderc** (Google):
- GLSL → SPIR-V compiler
- Part of Vulkan SDK
- Lighter weight than DXC

### Performance Considerations

**Compilation Time:**
- SPIR-V compilation adds ~10-50ms per shader
- Mitigated by aggressive caching
- Optional: pre-compile shaders at build time

**Runtime Performance:**
- Reflection computed once at shader creation
- Zero per-frame overhead
- Memory usage: +~1-5KB per shader for reflection data

**Caching Strategy:**
- SPIR-V bytecode cached on disk
- Reflection data serialized
- Invalidation based on source file timestamps

### Migration Path

**Backward Compatibility:**
- Existing `ShaderReflection` API unchanged
- Internal implementation swapped from OpenGL to SPIRV-Reflect
- No changes required in call sites

**Testing:**
- Compare reflection results: OpenGL vs SPIRV-Reflect
- Validate uniform offsets and sizes match
- Cross-platform consistency tests

**Rollback Plan:**
- Keep OpenGL reflection as fallback
- Runtime feature detection
- Graceful degradation if SPIR-V compilation fails

### Future Benefits

**WebGPU Readiness:**
- SPIR-V reflection works directly with WebGPU shaders
- No need for separate reflection systems per backend

**Advanced Features:**
- Storage buffer support for compute shaders
- Push constant reflection for high-frequency updates
- Texture binding validation and optimization

**Development Workflow:**
- Shader compilation errors at build time
- Static analysis of resource usage
- Automatic binding point allocation

### External Resources

- [SPIRV-Reflect GitHub](https://github.com/KhronosGroup/SPIRV-Reflect)
- [DXC Documentation](https://github.com/Microsoft/DirectXShaderCompiler)
- [shaderc Documentation](https://github.com/google/shaderc)
- [SPIR-V Specification](https://www.khronos.org/registry/spir-v/)