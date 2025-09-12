# FEAT-0241 — SSR-T refraction in raster preview

## Intent (human summary)
Implement Screen-Space Refraction with Transmission (SSR-T) in the rasterized pipeline to enable real-time glass material previews without requiring raytracing mode. This eliminates the current limitation where users must remember to use `--raytrace` for glass materials, and enables the hybrid auto mode to intelligently choose between raster previews and raytraced finals.

The core architectural change introduces a unified MaterialCore struct that eliminates the current dual material storage problem (PBR + legacy Material causing conversion drift), and layers SSR-T on top of the existing RHI abstraction.

## AI-Editable: Status
- Overall: in_progress
- Current PR: PR1 (in_progress)
- Last Updated: 2025-09-11T22:00:00Z

## AI-Editable: Checklist (mirror of spec.yaml acceptance_criteria)
- [ ] Opaque scenes unchanged (SSIM ≥ 0.995 vs goldens)
- [ ] Transmissive materials show SSR-T with roughness blur
- [ ] --mode auto picks raster for previews, ray for finals
- [x] MaterialCore struct eliminates dual material storage
- [x] RHI abstraction prevents direct OpenGL calls in Engine Core

## AI-Editable: Current PR Progress (PR1: MaterialCore + RHI headers)
- [x] Define MaterialCore unified BSDF struct
- [x] Define RHI interface with GL/WebGL2/Vulkan abstraction
- [ ] Update SceneObject to use MaterialCore instead of dual storage
- [ ] Add transmission and thickness fields to JSON Ops schema
- [ ] Write unit tests for MaterialCore BSDF calculations

## AI-Editable: Implementation Notes

### Key Discovery: Existing Implementation Foundation
The codebase already contains substantial groundwork:
- **MaterialCore**: Internal implementation exists in `engine/include/material_core.h`
- **RHI Layer**: Complete RHI abstraction with OpenGL/Null backends in `engine/include/rhi/`
- **Dual Storage**: SceneObject clearly shows the problem with separate `Material material` and PBR factors

### MaterialCore Design Decisions
- **Public API**: Created `engine/include/glint3d/material_core.h` with comprehensive documentation
- **Namespace Isolation**: Using `glint3d::MaterialCore` vs internal `MaterialCore` 
- **Field Layout**: Optimized for cache-friendly access with core properties first
- **Factory Functions**: Convenience constructors for common material types (metal, glass, emissive)
- **Migration Support**: Conversion functions from/to legacy Material and PBR systems

### RHI Interface Scope Completed
- **Public API**: Created `engine/include/glint3d/rhi.h` and `rhi_types.h`
- **Backend Support**: OpenGL 3.3+, WebGL2, planned Vulkan/WebGPU
- **Type Safety**: Opaque handles prevent resource mix-ups
- **Performance**: Designed for <5% overhead vs direct GL calls
- **Documentation**: Comprehensive Doxygen documentation for all public APIs

### Current Dual Storage Problem (SceneObject analysis)
```cpp
struct SceneObject {
    Material material;           // Legacy (raytracer)
    glm::vec4 baseColorFactor;  // PBR (rasterizer) 
    float metallicFactor;       // PBR (rasterizer)
    float roughnessFactor;      // PBR (rasterizer) 
    float ior;                  // PBR (rasterizer)
};
```
This creates conversion drift and inconsistent material behavior between pipelines.

### SSR-T Algorithm
- TBD: Screen-space ray step size vs quality tradeoffs
- TBD: Roughness-to-blur mapping for transmission
- TBD: Fallback strategy when rays miss geometry

## AI-Editable: Blockers and Risks

### Current Status: PR1 Foundation Complete
- **MaterialCore Public API**: Ready for SceneObject integration
- **RHI Public API**: Ready for RenderSystem migration  
- **Implementation Path Clear**: Both internal and public APIs defined

### Remaining Risks for PR2-PR5
- **Migration Complexity**: SceneObject integration may reveal unexpected dependencies
- **Performance Impact**: SSR-T texture reads and blur passes need careful optimization
- **WebGL2 Limitations**: Texture format constraints may require fallback strategies
- **Quality vs Performance**: SSR-T may not match raytracing quality for complex refractive scenes
- **Testing Coverage**: Need comprehensive golden image tests for material consistency

## Links
- Spec: [ai/tasks/FEAT-0241/spec.yaml](spec.yaml)
- Passes: [ai/tasks/FEAT-0241/passes.yaml](passes.yaml)
- Progress Log: [ai/tasks/FEAT-0241/progress.ndjson](progress.ndjson)
- Architecture Context: [CLAUDE.md](../../CLAUDE.md#dual-rendering-architecture-important)
- Related: Rendering System Refactoring Plan in CLAUDE.md