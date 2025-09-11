# Glint3D Rendering System Refactoring Guide

**Target Audience**: Graphics engineers new to the Glint3D codebase  
**Goal**: Migrate from dual-pipeline architecture to unified RHI-based system  
**Timeline**: ~3.5 weeks for complete refactor

## 0. Quick Start - Understanding the Current System

### Build & Run (Desktop)
```bash
# From repo root
cmake -S . -B builds/desktop/cmake/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build builds/desktop/cmake/Debug --config Debug

# Launch interactive UI
./builds/desktop/cmake/Debug/glint.exe

# Headless render (raster default - no refraction)
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/sphere_basic.json --render out.png

# Headless render (raytracer - full refraction)
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/glass-sphere-refraction.json --render out.png --raytrace
```

### Codebase Structure
```
engine/
├── include/               # Headers: scene, material, render system
├── src/                   # Implementation: raster pipeline, CLI, JSON ops
├── shaders/               # GLSL: pbr.frag, standard.frag, .vert files
└── src/                   # CPU raytracer: raytracer.cpp, refraction.cpp

examples/json-ops/         # Sample scenes for testing
docs/                      # Documentation (you'll add rendering_arch_v1.md)
```

## 1. Current (Old) System - What You're Inheriting

### Two Completely Separate Pipelines

#### **Rasterized Pipeline (Default)**
- **API**: OpenGL/WebGL2 direct calls
- **Entry Point**: `engine/src/render_system.cpp`
- **Shaders**: `engine/shaders/standard.vert|frag`, `engine/shaders/pbr.vert|frag`
- **Materials**: PBR (baseColor, metallic, roughness) or legacy Blinn-Phong
- **Transparency**: Alpha blending only - **NO REFRACTION**
- **IOR Usage**: Only computes F0 = ((n-1)/(n+1))² for basic Fresnel
- **Performance**: Real-time (< 1 second)

#### **Raytraced Pipeline (--raytrace flag)**
- **API**: CPU path tracer with BVH acceleration
- **Entry Point**: `engine/src/raytracer.cpp`
- **Features**: Full reflection, refraction (Snell's law), Fresnel, Total Internal Reflection
- **Materials**: Converts PBR → "legacy Material" struct
- **Performance**: Offline (30+ seconds for test scenes)
- **Files**: `engine/src/raytracer.cpp`, `engine/src/refraction.cpp`

### Material Storage Problem (Critical Issue)

**SceneObject currently holds TWO material representations:**

```cpp
struct SceneObject {
    // Legacy Material (used by raytracer)
    Material material;
    ├── glm::vec3 diffuse, specular, ambient;
    ├── float shininess, roughness, metallic;
    ├── float ior;              // Used for Snell's law
    └── float transmission;     // Used for transparency
    
    // PBR fields (used by rasterizer)  
    glm::vec4 baseColorFactor;  // sRGB + alpha
    float metallicFactor;       // 0.0-1.0
    float roughnessFactor;      // 0.0-1.0
    float ior;                  // Used ONLY for F0 calculation
};
```

**Problems with this approach:**
- Material properties can diverge between pipelines
- PBR→legacy conversion adds maintenance burden  
- `transmission` is completely ignored by rasterizer
- No unified BSDF model

### Known Limitations

1. **Raster cannot do refraction** - No screen-space refraction or transmission model
2. **Material duplication** - PBR + legacy causing sync issues
3. **Backend lock-in** - Direct OpenGL calls, hard to port to Vulkan/WebGPU
4. **No pass system** - Pipeline steps are interleaved, hard to extend
5. **Manual mode selection** - Users must remember `--raytrace` for glass

## 2. Target (New) Architecture - What We're Building

### Design Goals
- **AI-accessible**: JSON Ops v2 + REST /jobs wrapper for deterministic, idempotent operations
- **Renderer focus**: Not a DCC tool - optimize for headless CLI and web deployment
- **Backend agnostic**: GL/WebGL2 now → Vulkan/WebGPU later via clean RHI abstraction
- **Single material truth**: MaterialCore BSDF drives both raster and ray (no conversions)
- **Hybrid auto mode**: Raster+SSR-T for previews, ray for finals when scene needs true physics
- **Lightweight & scalable**: Two builds - web-preview (2MB WASM) and desktop-final (full features)

### External Dependencies & Toolchain

The refactored system leverages proven, lightweight packages organized in `engine/external/`:

#### **Core Dependencies (Always Required)**
- **fmt + spdlog**: Fast, structured logging with minimal overhead
- **cgltf**: Lightweight, robust glTF 2.0 loader (prefer over heavy Assimp)
- **tinyexr**: HDR/floating-point image support for environment maps
- **GLM**: Already integrated - continue using for all math

#### **Shader Pipeline (Future-Proofing)**
- **shaderc**: GLSL → SPIR-V compilation for cross-platform shaders
- **SPIRV-Cross**: SPIR-V → GLSL ES/MSL/HLSL cross-compilation  
- **SPIRV-Reflect**: Automatic UBO layout detection and binding inference

#### **Backend Abstraction (Future)**
- **volk**: Vulkan function loader (when adding Vulkan backend)
- **VMA**: Vulkan Memory Allocator (painless memory management)
- **Dawn** or **wgpu-native**: WebGPU desktop implementation

#### **Optional Acceleration (Desktop Only)**
- **Intel Embree**: 2-10x faster CPU ray tracing vs custom BVH
- **Intel OIDN**: Already integrated - AI-based denoising for ray tracing

#### **Build Strategy**
```cmake
# Web Preview Build (Minimal WASM)
GLINT_WEB_PREVIEW=ON      # <2MB WASM, raster+SSR-T only
GLINT_ENABLE_EMBREE=OFF   # No CPU ray tracing
GLINT_ENABLE_OIDN=OFF     # No denoising

# Desktop Final Build (Full Features)  
GLINT_ENABLE_EMBREE=ON    # Fast CPU ray tracing
GLINT_ENABLE_OIDN=ON      # AI denoising
GLINT_ENABLE_SPIRV=ON     # Future shader pipeline
```

### Core Components

#### **RHI (Render Hardware Interface)**
```cpp
// Thin abstraction for GPU operations
class RHI {
public:
    virtual bool init(const RhiInit& desc) = 0;
    virtual void beginFrame() = 0;
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void readback(const ReadbackDesc& desc) = 0;
    virtual void endFrame() = 0;
    
    // Resource management
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
};

// Concrete implementations:
class RhiGL : public RHI { /* OpenGL backend */ };
class RhiWebGL2 : public RHI { /* WebGL2 backend */ };
// Future: RhiVulkan, RhiWebGPU
```

#### **MaterialCore (Unified BSDF)**
```cpp
struct MaterialCore {
    glm::vec4 baseColor;           // sRGB base color + alpha
    float metallic;                // 0=dielectric, 1=metal
    float roughness;               // 0=mirror, 1=rough
    float normalStrength;          // Normal map intensity [0,2]
    glm::vec3 emissive;           // Self-emission (linear RGB)
    float ior;                     // Index of refraction [1.0, 3.0]
    float transmission;            // Transparency factor [0,1]
    float thickness;               // Volume thickness (meters)
    float attenuationDistance;     // Beer-Lambert falloff distance
    float clearcoat;               // Clear coat layer strength [0,1]
    float clearcoatRoughness;      // Clear coat roughness [0,1]
    
    // Future extensions:
    // float subsurface;           // SSS strength
    // glm::vec3 subsurfaceColor;  // SSS tint
    // float anisotropy;           // Anisotropic roughness
};

// This SINGLE struct is used by BOTH pipelines - no conversion needed
```

#### **RenderGraph (Minimal Pass System)**
```cpp
class RenderPass {
public:
    virtual void setup(const PassContext& ctx) = 0;
    virtual void execute(const PassContext& ctx) = 0;
    virtual void teardown(const PassContext& ctx) = 0;
};

// Raster pipeline passes:
class GBufferPass : public RenderPass { /* Geometry + materials to G-buffer */ };
class LightingPass : public RenderPass { /* Deferred lighting */ };
class SSRRefractionPass : public RenderPass { /* Screen-space refraction */ };
class PostPass : public RenderPass { /* Tone mapping, gamma */ };
class ReadbackPass : public RenderPass { /* CPU readback */ };

// Ray pipeline passes:
class IntegratorPass : public RenderPass { /* Path tracing */ };
class DenoisePass : public RenderPass { /* OIDN denoising */ };
class TonemapPass : public RenderPass { /* HDR → LDR */ };
```

#### **Hybrid Auto Mode**
```cpp
enum class RenderMode { Raster, Ray, Auto };

class ModeSelector {
public:
    RenderMode selectMode(const Scene& scene, const RenderConfig& config) {
        if (config.mode != RenderMode::Auto) return config.mode;
        
        // Auto mode heuristics:
        for (const auto& material : scene.materials) {
            if (material.transmission > 0.01f && 
                (material.thickness > 0.0f || material.ior > 1.05f)) {
                return config.isPreview ? RenderMode::Raster : RenderMode::Ray;
            }
        }
        return RenderMode::Raster; // Default to faster raster
    }
};

// CLI usage:
// --mode raster  → Force raster (fast, SSR approximation)
// --mode ray     → Force ray (slow, physically accurate)  
// --mode auto    → Smart selection based on scene content
```

## 3. Migration Plan - 20 Sequential Tasks

### **CURRENT PROGRESS SUMMARY**

**Phase 1: Foundation (Tasks 1-3) - COMPLETED**
- **Task 1**: RHI Interface defined with complete GPU abstraction
- **Task 2**: RhiGL implemented with full OpenGL backend
- **Task 3**: Build system integration completed

**Phase 2: Unified Materials (Task 4) -  COMPLETED**  
-  **Task 4**: MaterialCore implemented with unified BSDF representation (not yet adopted by SceneObject/raytracer)

**Phase 3: Pass System (Task 6) -  COMPLETED (FOUNDATION ONLY)**
-  **Task 6**: RenderGraph and pass types created; not wired into live render loop

**Phase 4: Screen-Space Refraction (Tasks 8–9) - NOT STARTED**
-  SSR-T and roughness-aware blur are pending; temporary transmissive blending is in place as a stopgap

**Phase 5: Hybrid Pipeline (Task 10) -  PARTIAL**
-  **Task 10**: Render mode selector implemented;
   - CLI `--mode raster|ray|auto` added; Auto selection integrated for headless renders and honored at UI startup
   - Remaining: connect to runtime pipeline/graph and expose in UI with selection reasoning

**Implementation Status**: **8 tasks complete + 3 partial (≈55%)**
-  **Completed/Implemented**: RHI interface + GL backend (Tasks 1–2), MaterialCore (4), RenderGraph foundation (6), Material binder (23), Shader updates for transmission/clearcoat initial (24), Transmissive blending pre-SSR (24.5)
-  **Partially Implemented**: Hybrid Auto (10), RHI threading (21), RHI shader creation/binding for object rendering (31)
-  **Newly Integrated**: CLI mode selection (headless Auto gating raster vs ray); RHI draw and offscreen textures; per-geometry RHI pipelines with buffer→attribute binding
-  **Major Gaps**: RenderGraph not yet wired to runtime; SSR/OIT for transmission; SceneObject still holds legacy material fields (MaterialCore not yet the sole source); some subsystems (skybox/grid) still use legacy GL shader path; textures still bound as legacy GL samplers
-  **Next Focus**: Finish Task 31 across subsystems (skybox/grid), adopt MaterialCore end-to-end (Task 22/5), wire passes (Task 25), implement SSR/OIT, and capability detection (Task 29)

**Files Added**:
```
engine/include/rhi/rhi.h                     RHI interface
engine/include/rhi/rhi-types.h               Type definitions  
engine/include/rhi/rhi_gl.h                  OpenGL backend header
engine/src/rhi/rhi-gl.cpp                    OpenGL implementation
engine/include/material-core.h               Unified materials
engine/src/material-core.cpp                 Material utilities
engine/include/render-pass.h                 Pass system
engine/src/render-pass.cpp                   Pass implementation
engine/include/render-mode-selector.h        Hybrid mode selector
engine/src/render-mode-selector.cpp          Mode selection logic

**Recent Changes (since last status)**:
- CLI: Added `--mode raster|ray|auto`; deprecated `--raytrace` in help text
- Headless: Auto mode now analyzes scene materials (bridged to MaterialCore) to pick raster vs ray
 - Shaders: Added uniforms and minimal logic for transmission (thickness/attenuation) and clearcoat in `engine/shaders/pbr.frag`
 - Engine: Introduced `bindMaterialUniforms(Shader&, const MaterialCore&)` helper to set PBR+extensions uniforms (`engine/include/material_binding.h`, `engine/src/material_binding.cpp`)
 - Raster Path: Integrated MaterialCore binder when using PBR shader; enabled alpha blending for transmissive materials (approx; SSR pending)
- RHI: Minimal integration in RenderSystem (OpenGL backend) for viewport/clear and per-frame begin/end
 - RHI: Offscreen color texture creation and draw calls now go through RHI (legacy GL draw removed from engine code)
 - RHI: Buffer-to-attribute binding added (VertexBinding.buffer + indexBuffer in PipelineDesc); RhiGL::setupVertexArray now binds buffers and configures attribs (R/RG/RGB/RGBA float and 8-bit)
 - SceneManager (RHI path): Creates per-geometry RHI buffers and builds per-geometry pipelines capturing vertex layout + index buffer; RenderSystem uses `obj.rhiPipeline` for draws
 - Fix: Draw path now detects indexed draw when either legacy GL EBO or RHI index buffer exists (prevents missing geometry when using RHI-only setup)
 - RHI Shader Binding (Object Rendering): Shaders for raster objects are now created via RHI and attached to pipelines. Uniforms are applied to the currently bound program (via helpers), avoiding program/VAO mismatches that could yield invisible geometry.
```

**Dependencies Added**:
```
engine/libraries/include/fmt/                Fast formatting
engine/libraries/include/spdlog/             Structured logging
engine/libraries/include/cgltf/              Lightweight glTF
engine/libraries/include/tinyexr/            HDR images
engine/libraries/include/shaderc/            Shader compilation
engine/libraries/include/spirv_cross/        Cross-compilation
engine/libraries/include/spirv_reflect/      Reflection
engine/libraries/include/volk/               Vulkan loader
engine/libraries/include/VulkanMemoryAllocator/  VMA
engine/libraries/include/embree3/            Ray acceleration
engine/libraries/include/OpenImageDenoise/   AI denoising
```

---

Each task is designed to be a single PR with clear acceptance criteria.

### **Phase 1: Foundation (Week 1, Days 1-3)**

#### **Task 1: Define RHI Interface**  **COMPLETED**
**Files**: `engine/include/rhi/rhi.h`, `engine/include/rhi/rhi-types.h`

**What was done:**
```cpp
// Created abstract RHI interface with complete resource management
class RHI {
public:
    virtual ~RHI() = default;
    virtual bool init(const RhiInit& desc) = 0;
    virtual void beginFrame() = 0;
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void readback(const ReadbackDesc& desc) = 0;
    virtual void endFrame() = 0;
    
    // Complete resource management
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;
    virtual void destroyPipeline(PipelineHandle handle) = 0;
    
    // State management and capability queries
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void clear(const glm::vec4& color, float depth = 1.0f, int stencil = 0) = 0;
    virtual bool supportsCompute() const = 0;
    virtual Backend getBackend() const = 0;
};

// Complete type definitions with proper enums and structures
using TextureHandle = uint32_t;
using BufferHandle = uint32_t;
using ShaderHandle = uint32_t;
using PipelineHandle = uint32_t;

// Comprehensive descriptor structs with full features
struct TextureDesc { TextureType type; TextureFormat format; int width, height, depth; /* etc. */ };
struct BufferDesc { BufferType type; BufferUsage usage; size_t size; /* etc. */ };
```

**Status:**  **COMPLETED** - Project compiles with new headers and type definitions.

#### **Task 2: Implement RhiGL (Desktop OpenGL)**
**Files**: `engine/rhi/RhiGL.h` (new), `engine/rhi/RhiGL.cpp` (new)

**What to do:**
```cpp
class RhiGL : public RHI {
private:
    struct TextureGL { GLuint id; GLenum target; /* ... */ };
    struct BufferGL { GLuint id; GLenum target; /* ... */ };
    std::vector<TextureGL> m_textures;
    std::vector<BufferGL> m_buffers;
    
public:
    bool init(const RhiInit& desc) override {
        // Initialize OpenGL context, check extensions
        return true;
    }
    
    TextureHandle createTexture(const TextureDesc& desc) override {
        GLuint id;
        glGenTextures(1, &id);
        // Configure texture based on desc
        return storeTexture(id, desc);
    }
    
    void draw(const DrawDesc& desc) override {
        // Bind pipeline, resources, issue draw call
    }
    
    // ... implement all RHI methods
};
```

**Accept when:** A minimal test can init RhiGL, clear a render target, and read back pixels.

#### **Task 3: Thread Existing Raster Through RHI**
**Files**: `engine/src/render_system.cpp`, shader loading utilities, mesh upload code

**What to do:**
- Replace direct `glGenTextures`, `glBindTexture`, etc. with `m_rhi->createTexture()`, etc.
- Update shader loading to use `RHI::createShader()`
- Update mesh upload to use `RHI::createBuffer()`
- Keep rendering behavior **identical** to current raster output

**Accept when:** Core sample scenes render with no visual regression (± tiny floating-point noise).

### **Phase 2: Unified Materials (Week 1, Days 4-5)**

#### **Task 4: Introduce MaterialCore and Migrate SceneObject**  **COMPLETED**
**Files**: `engine/include/material-core.h`, `engine/src/material-core.cpp`

**What was done:**
- Created comprehensive MaterialCore struct with complete BSDF parameter set
- Implemented material validation and utility functions
- Added texture handle integration for future RHI texture binding
- Prepared structure for both raster and ray pipeline compatibility

**Files implemented:**
```cpp
// engine/include/material-core.h - Unified material representation
struct MaterialCore {
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic{0.0f};
    float roughness{0.5f};
    float normalStrength{1.0f};
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};
    float ior{1.5f};
    float transmission{0.0f};
    float thickness{0.001f};
    float attenuationDistance{1.0f};
    glm::vec3 attenuationColor{1.0f, 1.0f, 1.0f};
    float clearcoat{0.0f};
    float clearcoatRoughness{0.0f};
    
    // Texture handles for RHI integration
    uint32_t baseColorTexture{0};
    uint32_t metallicRoughnessTexture{0};
    uint32_t normalTexture{0};
    uint32_t emissiveTexture{0};
    
    // Utility functions
    bool isTransparent() const;
    bool requiresRaytracing() const;
    void validate();
};

// engine/src/material-core.cpp - Implementation with validation logic
```

**Status:** Structural implementation complete. SceneObject migration pending integration with existing render system.

#### **Task 5: Adapt Raytracer to MaterialCore**
**Files**: `engine/src/raytracer.cpp`, `engine/src/raytracer_lighting.cpp`

**What to do:**
- Remove PBR→legacy conversion in `loadModel()`
- Update raytracer to read `MaterialCore` directly:

```cpp
// In raytracer.cpp
void Raytracer::loadModel(const ObjLoader& obj, const glm::mat4& M, const MaterialCore& mat) {
    // Direct use - no conversion needed
    for (size_t i = 0; i < Nt; ++i) {
        triangles.emplace_back(v0, v1, v2, mat); // Pass MaterialCore directly
    }
}

// In Triangle constructor
Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const MaterialCore& mat)
    : v0(a), v1(b), v2(c), material(mat) {}
```

**Accept when:** Glass/refraction test from `examples/json-ops/glass-sphere-refraction.json --raytrace` looks the same or better.

### **Phase 3: Pass System (Week 2, Days 1-2)**

#### **Task 6: Add Minimal RenderGraph**  **COMPLETED**
**Files**: `engine/include/render-pass.h`, `engine/src/render-pass.cpp`

**What was done:**
- Implemented complete RenderPass base class with setup/execute/teardown lifecycle
- Created RenderGraph class for pass management and execution order
- Added PassContext for sharing resources and state between passes
- Implemented specific pass types for modular rendering pipeline

**Files implemented:**
```cpp
// engine/include/render-pass.h - Pass system foundation
struct PassContext {
    RHI* rhi;
    const Scene* scene;
    const CameraState* camera;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    std::unordered_map<std::string, uint32_t> resources;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;
    virtual void setup(const PassContext& ctx) = 0;
    virtual void execute(const PassContext& ctx) = 0;
    virtual void teardown(const PassContext& ctx) = 0;
};

class RenderGraph {
private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
public:
    void addPass(std::unique_ptr<RenderPass> pass);
    void execute(const PassContext& ctx);
    void clear();
};

// Specific pass implementations:
class GBufferPass : public RenderPass { /* Geometry to G-buffer */ };
class LightingPass : public RenderPass { /* Deferred lighting */ };
class SSRRefractionPass : public RenderPass { /* Screen-space refraction */ };
class PostPass : public RenderPass { /* Tone mapping, gamma */ };
class ReadbackPass : public RenderPass { /* CPU readback */ };
```

**Status:** Foundation complete. Integration with existing render system pending.

#### **Task 7: Wire Material Uniforms (Prep for Transmission)**
**Files**: `engine/shaders/pbr.frag`, uniform buffer object (UBO) layout, material binding code

**What to do:**
```glsl
// In pbr.frag - add new uniforms
uniform float transmission;
uniform float thickness;
uniform float ior;
uniform float attenuationDistance;
uniform float clearcoat;
uniform float clearcoatRoughness;
```

```cpp
// In material binding code
void bindMaterialUniforms(const MaterialCore& mat, Shader* shader) {
    shader->setVec4("baseColorFactor", mat.baseColor);
    shader->setFloat("metallicFactor", mat.metallic);
    shader->setFloat("roughnessFactor", mat.roughness);
    shader->setFloat("ior", mat.ior);
    shader->setFloat("transmission", mat.transmission);
    shader->setFloat("thickness", mat.thickness);
    shader->setFloat("attenuationDistance", mat.attenuationDistance);
    shader->setFloat("clearcoat", mat.clearcoat);
    shader->setFloat("clearcoatRoughness", mat.clearcoatRoughness);
}
```

**Accept when:** No visible change yet; uniforms visible in RenderDoc/debug tools.

### **Phase 4: Screen-Space Refraction (Week 2, Days 3-5)**

#### **Task 8: Implement Screen-Space Refraction (SSR-T)**
**Files**: `engine/shaders/pbr.frag`, shader utilities

**What to do:**
```glsl
// In pbr.frag - simplified SSR-T implementation
vec3 computeSSRRefraction(vec3 viewDir, vec3 normal, float ior, float transmission) {
    if (transmission < 0.01) return vec3(0.0); // Skip if opaque
    
    // Compute refracted direction using Snell's law
    vec3 refractionDir = refract(-viewDir, normal, 1.0 / ior);
    
    // Handle total internal reflection
    if (length(refractionDir) < 0.01) {
        return vec3(0.0); // TIR - no refraction
    }
    
    // Project refracted ray to screen space
    vec3 refractionEnd = vWorldPos + refractionDir * thickness;
    vec4 refractionClip = uProjectionMatrix * uViewMatrix * vec4(refractionEnd, 1.0);
    vec2 refractionUV = (refractionClip.xy / refractionClip.w) * 0.5 + 0.5;
    
    // Sample background color (scene color buffer or environment)
    if (refractionUV.x >= 0.0 && refractionUV.x <= 1.0 && 
        refractionUV.y >= 0.0 && refractionUV.y <= 1.0) {
        return texture(sceneColorTex, refractionUV).rgb;
    } else {
        // Fallback to environment map or background color
        return texture(environmentMap, refractionDir).rgb;
    }
}

// In main():
vec3 refractedColor = computeSSRRefraction(viewDir, normal, ior, transmission);

// Fresnel mixing
float fresnel = fresnelSchlick(max(dot(normal, viewDir), 0.0), 1.0, ior);
vec3 finalColor = mix(refractedColor, reflectedColor, fresnel);
finalColor = mix(baseColor.rgb, finalColor, transmission);
```

**Accept when:** Transmissive materials refract in raster mode; artifacts acceptable; performance reasonable (<16ms).

#### **Task 9: Roughness-Aware Refraction Blur**
**Files**: Same as Task 8

**What to do:**
```glsl
// Approximate micro-roughness by mip bias or small blur
vec3 computeSSRRefractionBlurred(vec3 viewDir, vec3 normal, float ior, float transmission, float roughness) {
    vec3 refractionDir = refract(-viewDir, normal, 1.0 / ior);
    
    // ... same projection logic ...
    
    // Blur based on roughness
    float mipLevel = roughness * 6.0; // Assuming 6 mip levels
    return textureLod(sceneColorTex, refractionUV, mipLevel).rgb;
    
    // Alternative: multi-sample approach for higher quality
    // vec3 blurredColor = vec3(0.0);
    // for (int i = 0; i < 4; ++i) {
    //     vec2 offset = poissonDisk[i] * roughness * 0.01;
    //     blurredColor += texture(sceneColorTex, refractionUV + offset).rgb;
    // }
    // return blurredColor * 0.25;
}
```

**Accept when:** Higher roughness yields softer refraction in raster pipeline.

### **Phase 5: Hybrid Pipeline (Week 3, Days 1-3)**

#### **Task 10: Hybrid Auto Mode + CLI Flag**  **COMPLETED**
**Files**: `engine/include/render-mode-selector.h`, `engine/src/render-mode-selector.cpp`

**What was done:**
- Implemented comprehensive RenderMode enum with Raster, Ray, and Auto options
- Created intelligent mode selection based on material analysis
- Added performance vs quality balancing heuristics
- Prepared CLI integration for mode override functionality

**Files implemented:**
```cpp
// engine/include/render-mode-selector.h - Intelligent pipeline selection
enum class RenderMode {
    Raster,     // Real-time OpenGL/WebGL
    Ray,        // Offline CPU raytracing
    Auto        // Intelligent selection
};

class RenderModeSelector {
public:
    struct AnalysisResult {
        RenderMode recommendedMode;
        std::string reason;
        bool hasTransparentMaterials;
        bool hasComplexOptics;
        float estimatedRenderTime;
    };
    
    static AnalysisResult analyzeScene(const Scene& scene);
    static RenderMode selectMode(const Scene& scene, 
                                RenderMode preferredMode = RenderMode::Auto,
                                bool prioritizeQuality = false);
    
private:
    static bool requiresRaytracing(const MaterialCore& material);
    static float estimateComplexity(const Scene& scene);
};

// Implementation with sophisticated heuristics:
// - Checks for transmission > 0.01 and ior deviation > 0.05
// - Analyzes thickness for volume scattering requirements
// - Estimates performance impact based on triangle count
// - Provides detailed reasoning for mode selection
```

**Status:** Core implementation complete. CLI integration pending.

#### **Task 11: Align BSDF Models Between Raster & Ray**
**Files**: `engine/shaders/pbr.frag`, `engine/src/raytracer.cpp`, shared BRDF utilities

**What to do:**
- Ensure both pipelines use identical BRDF models:
  - **GGX distribution** for microfacet normals
  - **Smith masking-shadowing** function  
  - **Schlick Fresnel** approximation
  - **Energy compensation** for multiple scattering

```cpp
// Shared BRDF utilities (can be used by both shader and CPU code)
namespace BRDF {
    inline float DistributionGGX(float NdotH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
        return a2 / (M_PI * denom * denom);
    }
    
    inline float GeometrySmith(float NdotV, float NdotL, float roughness) {
        float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
        float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
        float ggx2 = NdotL / (NdotL * (1.0f - k) + k);
        return ggx1 * ggx2;
    }
    
    inline glm::vec3 FresnelSchlick(float cosTheta, const glm::vec3& F0) {
        return F0 + (glm::vec3(1.0f) - F0) * std::pow(1.0f - cosTheta, 5.0f);
    }
}
```

**Accept when:** A metallic/roughness sphere grid closely matches between raster and ray pipelines.

#### **Task 12: Clearcoat & Attenuation Support**
**Files**: Shaders, raytracer volume integration

**What to do:**
```glsl
// In pbr.frag - clearcoat lobe
vec3 computeClearcoat(vec3 normal, vec3 viewDir, vec3 lightDir, float clearcoat, float clearcoatRoughness) {
    if (clearcoat < 0.01) return vec3(0.0);
    
    vec3 H = normalize(viewDir + lightDir);
    float NdotH = max(dot(normal, H), 0.0);
    float VdotH = max(dot(viewDir, H), 0.0);
    
    float D = DistributionGGX(NdotH, clearcoatRoughness);
    float G = GeometrySmith(max(dot(normal, viewDir), 0.0), max(dot(normal, lightDir), 0.0), clearcoatRoughness);
    vec3 F = FresnelSchlick(VdotH, vec3(0.04)); // Fixed F0 for clearcoat
    
    return D * G * F * clearcoat;
}

// Beer-Lambert attenuation for transmission
vec3 applyVolumeAttenuation(vec3 color, float distance, float attenuationDistance, vec3 attenuationColor) {
    if (distance <= 0.0 || attenuationDistance <= 0.0) return color;
    
    float attenuation = exp(-distance / attenuationDistance);
    return mix(attenuationColor, color, attenuation);
}
```

**Accept when:** Clearcoat lobes visible; colored glass attenuates with thickness consistently across pipelines.

### **Phase 6: Production Features (Week 3, Days 4-5 + Week 4, Day 1)**

#### **Task 13: Readback of Auxiliary Attachments**
**Files**: FBO setup, readback utilities, CLI extensions

**What to do:**
```cpp
// Add optional auxiliary outputs
struct ReadbackConfig {
    bool exportDepth = false;
    bool exportNormals = false;
    bool exportInstanceIds = false;
    std::string outputPrefix; // e.g., "render" → "render_depth.png", "render_normals.exr"
};

// CLI: --export depth,normals,ids
// Output: scene.png, scene_depth.png, scene_normals.exr, scene_ids.png
```

**Accept when:** Files are emitted on demand and align spatially with color output.

#### **Task 14: Deterministic Rendering + Metadata**
**Files**: Render entry points, metadata writer

**What to do:**
```cpp
// Accept --seed parameter, propagate to all stochastic code
struct RenderMetadata {
    uint32_t seed;
    std::string engineVersion;
    RenderMode mode;
    float renderTimeMs;
    std::vector<std::string> passNames;
    std::vector<float> passTimesMs;
};

// Write metadata sidecar: render.json
void writeRenderMetadata(const std::string& basePath, const RenderMetadata& meta);
```

**Accept when:** Re-running with same seed yields identical pixels (within floating-point noise); metadata JSON present.

#### **Task 15: Golden Test Suite + Validator CLI**
**Files**: `tests/golden/`, validator implementation

**What to do:**
```bash
# Add canonical test scenes
tests/golden/scenes/
├── glass_spheres.json          # IOR variation (water, glass, diamond)
├── metallic_roughness_grid.json # Material parameter sweep
├── hdri_environment.json       # IBL + environment mapping
├── clearcoat_car_paint.json    # Clearcoat + base layer
└── volume_attenuation.json     # Colored glass thickness test

# Validator CLI
glint validate --suite core --tolerance 0.995  # SSIM threshold
```

**Accept when:** Validator runs locally and reports PASS for current implementation.

### **Phase 7: Platform Support (Week 4, Days 2-3)**

#### **Task 16: WebGL2 Compliance**
**Files**: Shader preprocessing, web build configuration

**What to do:**
- Add precision qualifiers for WebGL2:
```glsl
#ifdef GL_ES
precision highp float;
precision mediump sampler2D;
#endif
```
- Handle NPOT texture limitations
- Ensure MRT (Multiple Render Targets) compatibility
- Add fallback paths for unsupported features

**Accept when:** Web build compiles and runs core scenes with SSR-T or graceful fallback.

#### **Task 17: Performance Profiling Hooks**
**Files**: Timing utilities, profiler integration

**What to do:**
```cpp
class ScopedTimer {
    std::chrono::high_resolution_clock::time_point m_start;
    float& m_output;
public:
    ScopedTimer(float& output) : m_output(output), m_start(std::chrono::high_resolution_clock::now()) {}
    ~ScopedTimer() { 
        auto end = std::chrono::high_resolution_clock::now();
        m_output = std::chrono::duration<float, std::milli>(end - m_start).count();
    }
};

#define PROFILE_SCOPE(name) ScopedTimer timer_##name(passTimings[#name])
```

**Accept when:** Validator outputs include per-pass timing; no measurable performance regression.

#### **Task 18: Error Handling & Graceful Fallbacks**
**Files**: Capability detection, error logging

**What to do:**
```cpp
class RenderCapabilities {
public:
    bool supportsSSR = false;
    bool supportsFloatTextures = false;
    bool supportsMRT = false;
    
    static RenderCapabilities detect(RHI* rhi) {
        RenderCapabilities caps;
        // Query GPU capabilities
        return caps;
    }
};

// Graceful degradation
if (!caps.supportsSSR && material.transmission > 0.01f) {
    Logger::warn("SSR not supported, falling back to alpha blending for transmission");
    // Use alpha blending path
}
```

**Accept when:** No crashes; clear warning logs; sensible degraded output when features unavailable.

### **Phase 8: Cleanup (Week 4, Days 4-5)**

#### **Task 19: Architecture Documentation**
**Files**: `docs/rendering_arch_v1.md`

**What to do:**
- Diagram: Scene → MaterialCore → Mode Selector → RenderGraph → Passes → Readback
- Document RHI interface and backend implementations
- Explain MaterialCore fields and usage
- Describe SSR-T implementation and limitations
- Document auto-mode heuristics and CLI flags

**Accept when:** A new graphics engineer can read the doc and understand the architecture you built.

#### **Task 20: Remove Legacy Material Conversions**
**Files**: Dead code cleanup, include updates

**What to do:**
- Remove old PBR→legacy conversion functions
- Delete unused material structs and adapters
- Update includes and forward declarations
- Clean up any remaining dual-material code paths

**Accept when:** Repo compiles cleanly; validator still passes; no dead code remains.

## 4. Implementation Guidelines

### Development Workflow
1. **Create feature branch** for each task: `git checkout -b task-01-rhi-interface`
2. **Small, focused PRs** - each task should be reviewable in 30-60 minutes
3. **Run validator after each task** to catch regressions early
4. **Update golden images** if intentional changes improve quality
5. **Performance budget**: Keep frame time <16ms for real-time targets

### Testing Strategy
```bash
# After each task, run regression tests
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/sphere_basic.json --render test_raster.png
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/glass-sphere-refraction.json --render test_ray.png --raytrace

# Compare with reference images (manual or automated)
```

### Key Architecture Principles
1. **RHI abstraction**: No direct OpenGL calls outside of RhiGL implementation
2. **Single material truth**: MaterialCore is the only source of material properties
3. **Pass isolation**: Each RenderPass should be self-contained and testable
4. **Graceful degradation**: Always provide fallbacks for unsupported features
5. **Performance awareness**: Profile critical paths and avoid unnecessary allocations

### Common Pitfalls to Avoid
- **Don't break existing CLI behavior** - maintain backward compatibility
- **Don't optimize prematurely** - focus on correctness first, then performance
- **Don't hardcode backend assumptions** - keep RHI interface truly abstract  
- **Don't ignore WebGL constraints** - test on web builds regularly
- **Don't skip validation** - run the golden test suite after significant changes

## 5. Expected Outcomes

### User Experience Improvements
-  Glass materials work identically in both raster and ray modes
-  No need to remember `--raytrace` flag (auto mode handles it)
-  Real-time preview with offline-quality final renders
-  Consistent material behavior eliminates confusion

### Developer Experience Improvements
-  Single MaterialCore struct (no dual storage synchronization)
-  Clean RHI abstraction (easy to add Vulkan/WebGPU backends)
-  Modular pass system (easy to add new effects like SSS, volumetrics)
-  Comprehensive test suite (catch regressions early)

### Performance Characteristics
-  Real-time refraction via SSR-T (<16ms frame budget maintained)
-  GPU backend abstraction with minimal overhead (<5% perf cost)
-  Hybrid mode automatically balances quality vs performance

### Technical Debt Reduction
-  Eliminate material duplication and conversion complexity
-  Replace ad-hoc rendering with structured pass system
-  Remove platform-specific rendering code paths
-  Standardize error handling and capability detection

## 6. Post-Refactor: What You'll Have Built

After completing all 20 tasks, Glint3D will have:

- **Modern Architecture**: RHI abstraction ready for Vulkan/WebGPU
- **Unified Materials**: Single MaterialCore driving both pipelines
- **Hybrid Rendering**: Automatic quality/performance balancing
- **Real-time Refraction**: SSR-T providing fast glass approximation
- **Production Ready**: Deterministic rendering with full metadata
- **Platform Agnostic**: Clean separation between API and implementation
- **Maintainable**: Pass-based system easy to extend and debug

The refactored system will be significantly more maintainable, extensible, and user-friendly while preserving all existing functionality and adding substantial new capabilities.

---

## Advanced Implementation Details

### Enhanced SSR-T (Screen-Space Refraction with Transmission)

#### Practical WebGL2-Compatible Implementation
```glsl
// Enhanced SSR-T for order-independent transparency
vec3 computeSSRTransmission(vec3 viewDir, vec3 normal, float ior, float transmission, 
                           float thickness, float roughness, vec3 attenuationColor) {
    if (transmission < 0.01) return vec3(0.0);
    
    // Depth-aware screen-space refraction with thickness support
    vec3 refractionDir = refract(-viewDir, normal, 1.0/ior);
    vec3 worldRefractEnd = vWorldPos + refractionDir * thickness;
    
    // Project to screen space with proper depth testing
    vec4 clipRefract = uProjectionMatrix * uViewMatrix * vec4(worldRefractEnd, 1.0);
    vec2 refractionUV = (clipRefract.xy / clipRefract.w) * 0.5 + 0.5;
    
    // Weighted Blended OIT for multiple transparent layers
    float sceneDepth = texture(depthBuffer, refractionUV).r;
    float surfaceDepth = gl_FragCoord.z;
    float depthDiff = max(0.0, sceneDepth - surfaceDepth);
    float weight = transmission * (1.0 - exp(-depthDiff * thickness * 10.0));
    
    // Roughness-aware sampling (mip bias or multi-sample blur)
    vec3 refractedColor;
    if (capabilities.supportsMipLevels) {
        float mipLevel = roughness * 6.0;
        refractedColor = textureLod(sceneColorBuffer, refractionUV, mipLevel).rgb;
    } else {
        // Fallback: 4-sample Poisson disk blur
        refractedColor = vec3(0.0);
        const vec2 offsets[4] = vec2[](vec2(-0.7,-0.7), vec2(0.7,-0.7), vec2(-0.7,0.7), vec2(0.7,0.7));
        for (int i = 0; i < 4; ++i) {
            vec2 sampleUV = refractionUV + offsets[i] * roughness * 0.01;
            refractedColor += texture(sceneColorBuffer, sampleUV).rgb;
        }
        refractedColor *= 0.25;
    }
    
    // Beer-Lambert volume attenuation for colored glass
    if (thickness > 0.001) {
        vec3 attenuation = exp(-thickness / attenuationDistance * (vec3(1.0) - attenuationColor));
        refractedColor *= attenuation;
    }
    
    return refractedColor * weight;
}

// Capability detection and graceful fallbacks
bool detectSSRCapabilities() {
    bool hasDepthTexture = checkExtension("GL_OES_depth_texture") || isWebGL2();
    bool hasMipLevels = checkExtension("GL_OES_texture_float_linear");
    bool hasMRT = checkExtension("GL_EXT_draw_buffers") || isWebGL2();
    
    if (!hasDepthTexture) {
        logWarning("Depth textures not supported, SSR-T disabled");
        return false;
    }
    
    return true;
}
```

### AI-Accessible JSON Ops v2

#### Idempotent, Versioned Operations
```cpp
// Enhanced JSON Ops for AI workflows
struct JsonOpsV2 {
    int version = 2;                    // Schema version for compatibility
    uint32_t seed = 0;                  // Deterministic seed for reproducible results
    std::string operationId;            // UUID for deduplication and caching
    std::vector<Operation> operations;  // Idempotent operation sequence
    RenderConfig renderConfig;          // Quality/performance preferences
    
    // Validation and normalization
    bool validate() const;
    void normalize();  // Ensure deterministic operation order
};

// REST API wrapper for batch processing
struct JobRequest {
    JsonOpsV2 operations;
    OutputConfig outputs;
    QualityConfig quality;
    
    // AI-friendly features
    std::string callbackUrl;    // Webhook for completion notification
    int timeoutMs = 300000;     // 5-minute default timeout
    int retryAttempts = 3;      // Automatic retry on failure
};

// Example API usage:
POST /api/v1/jobs
{
    "operations": {
        "version": 2,
        "seed": 42,
        "operationId": "uuid-here",
        "operations": [
            {"op": "load", "path": "models/glass_sphere.obj", "name": "sphere"},
            {"op": "set_material", "target": "sphere", "material": {
                "ior": 1.5, "transmission": 0.9, "thickness": 0.1
            }},
            {"op": "set_camera", "position": [0, 0, 5], "target": [0, 0, 0]},
            {"op": "add_light", "type": "directional", "direction": [-1, -1, -1]}
        ],
        "renderConfig": {
            "mode": "auto",          // Intelligent pipeline selection
            "quality": "preview",    // preview|final
            "width": 512,
            "height": 512
        }
    },
    "outputs": {
        "formats": ["png", "exr"],
        "channels": ["color", "depth", "normals", "ids"],
        "compression": "lz4"
    },
    "callbackUrl": "https://ai-system.example.com/render-complete"
}

Response: { 
    "jobId": "uuid", 
    "status": "queued", 
    "estimatedTimeMs": 15000 
}

// Status polling:
GET /api/v1/jobs/{jobId}/status
{
    "status": "processing",
    "progress": 0.75,
    "currentPass": "ssr_transmission",
    "passProgress": 0.4,
    "elapsedMs": 11200,
    "remainingMs": 3800
}

// Output retrieval:
GET /api/v1/jobs/{jobId}/outputs
{
    "status": "completed",
    "outputs": {
        "color": "https://signed-url/render_color.png",
        "depth": "https://signed-url/render_depth.exr", 
        "normals": "https://signed-url/render_normals.exr",
        "metadata": "https://signed-url/render_metadata.json"
    },
    "metadata": {
        "engineVersion": "0.4.0",
        "renderMode": "hybrid_auto",
        "actualMode": "raster_ssr",
        "seed": 42,
        "totalTimeMs": 14750,
        "passTimings": {
            "gbuffer": 5.2,
            "lighting": 8.7,
            "ssr_transmission": 12.1,
            "post": 3.4,
            "readback": 2.1
        },
        "qualityMetrics": {
            "ssim": 0.987,           // Structural similarity vs golden
            "psnr": 42.3,            // Peak signal-to-noise ratio
            "frameTimeMs": 14.7      // Render performance
        },
        "capabilities": {
            "backend": "OpenGL 3.3",
            "ssrSupported": true,
            "oitSupported": true,
            "embreeEnabled": false,
            "oidnEnabled": false
        }
    }
}
```

### Lightweight Build Matrix

#### Web Preview Build (Optimized for AI)
```cmake
# Minimal WASM build for web deployment
set(GLINT_WEB_PREVIEW ON)
set(GLINT_TARGET_SIZE_MB 2)          # Hard size limit

# Disable heavy features
set(GLINT_ENABLE_EMBREE OFF)         # No CPU ray tracing
set(GLINT_ENABLE_OIDN OFF)           # No AI denoising
set(GLINT_ENABLE_SPIRV OFF)          # No advanced shader pipeline
set(GLINT_ENABLE_ASSIMP OFF)         # Basic OBJ/glTF only

# Enable lightweight features
set(GLINT_ENABLE_SSR_T ON)           # Screen-space refraction
set(GLINT_ENABLE_OIT ON)             # Order-independent transparency
set(GLINT_ENABLE_JSON_OPS_V2 ON)     # Enhanced JSON operations

# Compression and optimization
set(GLINT_COMPRESS_ASSETS ON)        # Brotli-compress embedded assets
set(GLINT_OPTIMIZE_SIZE ON)          # -Os optimization
set(GLINT_STRIP_DEBUG ON)            # Remove debug symbols

# Result: ~2MB WASM + ~500KB assets
```

#### Desktop Final Build (Full Features)
```cmake
# Full-featured desktop build
set(GLINT_DESKTOP_FINAL ON)

# Enable all acceleration
set(GLINT_ENABLE_EMBREE ON)          # Intel Embree ray tracing
set(GLINT_ENABLE_OIDN ON)            # Intel OIDN denoising
set(GLINT_ENABLE_SPIRV ON)           # Future shader pipeline
set(GLINT_ENABLE_ASSIMP ON)          # Full format support

# Production features
set(GLINT_ENABLE_PROFILING ON)       # Performance profiling
set(GLINT_ENABLE_VALIDATION ON)      # Debug validation layers
set(GLINT_ENABLE_TELEMETRY ON)       # Usage metrics

# Future backend preparation
set(GLINT_ENABLE_VULKAN_PREP ON)     # Vulkan headers and validation
set(GLINT_ENABLE_WEBGPU_PREP ON)     # WebGPU headers for future

# Result: ~50MB executable with full capabilities
```

### Quick Milestones (Week-Sized Deliverables)

#### **Week 1: Foundation**
-  RHI (OpenGL backend) + thread existing raster through it
-  MaterialCore unification + raytracer adaptation
-  **Deliverable**: Current functionality preserved with clean abstraction

#### **Week 2: Core Features**  
-  RenderGraph (minimal pass system)
-  SSR-T implementation + roughness-aware blur
-  **Deliverable**: Real-time glass materials in raster pipeline

#### **Week 3: Intelligence**
-  Hybrid Auto mode + `--mode raster|ray|auto` CLI
-  JSON Ops v2 + deterministic metadata
-  **Deliverable**: Intelligent pipeline selection, reproducible renders

#### **Week 4: Production**
- Validator + golden test suite + CI integration
- REST /jobs wrapper + signed URL outputs
- **Deliverable**: Production-ready AI-accessible rendering service

## 3A. Audit Gaps & Newly Identified Tasks

This subsection captures gaps observed in the current repo and adds concrete follow-ups beyond the original list.

### Gaps Identified
- Raster path still issues OpenGL calls directly (not via RHI) in `engine/src/render_system.cpp`.
- Scene uses legacy `Material` plus ad-hoc PBR fields in `SceneObject` (dual material source of truth).
- `RenderGraph` and pass classes exist but are not executed by the render loop.
- `pbr.frag` has no uniforms/logic for transmission, thickness, attenuation, clearcoat.
- Mode selector not surfaced in UI (no toggle/telemetry of selection reason).
- CLI and examples still reference `--raytrace` (now deprecated in help text).

### New Tasks (21+)

#### Task 21: Thread Raster Through RHI (Initial Cut)
- Scope: Introduce RHI usage for texture creation, shader programs, and vertex/index buffers in `render_system.cpp`.
- Acceptance: Opaque scenes render identically; no direct `glGenTextures`/`glCreateShader` in engine code outside RhiGL.
 - Status: Partially implemented. RHI used for viewport/clear, per-frame begin/end, and draw calls; offscreen render target created via RHI. SceneManager creates RHI vertex/index buffers; RenderSystem builds per-geometry pipelines (with shader + vertex layout + index buffer) and prefers them when present.

#### Task 22: SceneObject → MaterialCore Adoption
- Scope: Add a `MaterialCore` field to `SceneObject` and map legacy/PBR fields into it during load; use this struct everywhere.
- Acceptance: Both raster binding and raytracer read from `MaterialCore` (no PBR→legacy conversions).

#### Task 23: Material Uniform Binder
- Scope: Implement a binder to set shader uniforms from `MaterialCore` (baseColor, metallic, roughness, ior, transmission, thickness, attenuation, clearcoat...)
- Acceptance: Centralized function used by raster path; easy to extend and test.
- Status: Implemented and integrated in raster PBR path.

#### Task 24: Shader Updates for Transmission & Clearcoat
 - Scope: Add uniforms and minimal logic to `engine/shaders/pbr.frag` for transmission, thickness, attenuation, clearcoat, clearcoatRoughness.
 - Acceptance: Visual parity on metallic/roughness grid; clearcoat lobe visibly contributes; attenuation applies with thickness.
 - Status: Implemented (initial). Alpha/transparency integration and SSR remain future work.

#### Task 23.5: MaterialCore Uniform Binder
- Scope: Provide a centralized helper to map MaterialCore fields to shader uniforms used by PBR.
- Acceptance: One-line call to bind all required uniforms for PBR + extensions.
- Status: Implemented (`bindMaterialUniforms`) and added to build; integrated in raster PBR path.

#### Task 24.5: Transmissive Blending (Pre-SSR)
- Scope: Enable alpha blending and depth-write disable for transmissive materials when using the PBR shader.
- Acceptance: Transparent objects composite with background; order issues acceptable until SSR/OIT lands.
- Status: Implemented (approximation). SSR/OIT remain future tasks.

#### Task 31: RHI Shader Creation & Binding (Raster Objects)
- Scope: Create RHI shaders from GLSL source files via `ShaderDesc`, store `ShaderHandle` in `PipelineDesc.shader`, and bind GL programs via pipeline binding.
- Acceptance: Raster object rendering compiles shaders via RHI, pipelines have valid `shader` handles, uniforms apply to the bound program, and no legacy shader `use()` calls are required in object draw paths.
- Status: Implemented for raster object rendering. Remaining: port skybox/grid/aux pipelines to RHI and migrate texture creation/binds.

#### Task 25: Integrate RenderGraph (Raster Path)
- Scope: Replace monolithic raster render with passes: GBuffer → Lighting → (SSR-T placeholder) → Post → Readback.
- Acceptance: Frame renders via `RenderGraph` with same output as legacy path on opaque scenes.

#### Task 26: Hybrid Mode in UI
- Scope: Add a UI toggle for `raster|ray|auto`, surface auto-selection reason, and allow preview/final presets.
- Acceptance: Mode switch works; “reason” string visible; persists across sessions.

#### Task 27: CLI/Docs Migration
- Scope: Update docs and examples to use `--mode`; log deprecation warning when `--raytrace` is used; add examples for auto mode.
- Acceptance: README and help align; examples run using `--mode`.

#### Task 28: Determinism Harness Hook
- Scope: Wire `--seed` through all stochastic paths (glossy reflection sampling, raytracer); provide regression using `tests/determinism`.
- Acceptance: Re-runs with same seed are pixel-stable within tolerance; script passes locally.

#### Task 29: Capability Detection & Fallbacks (Raster)
- Scope: Implement capability probe; warn/fallback when SSR/OIT/float textures are missing.
- Acceptance: No crashes on low-cap devices; clear logs and graceful visuals.

#### Task 30: Legacy Cleanup Plan
- Scope: Track and gradually remove legacy material structs/conversions once both pipelines fully read `MaterialCore`.
- Acceptance: Compile without legacy conversions; no duplicate material fields in `SceneObject`.

#### **Optional: Advanced Features**
-  Intel Embree integration (2-10x faster CPU ray tracing)
-  Vulkan/WebGPU backend preparation
-  Advanced volume rendering (subsurface scattering, volumetrics)

#### Task 31: RHI Shader Creation & Binding
- Scope: Create RHI shaders from GLSL source files via `ShaderDesc`, store `ShaderHandle` in `PipelineDesc.shader`, and bind GL programs via `bindPipeline` (retire legacy `Shader` wrapper in raster path).
- Acceptance: Raster path compiles shaders via RHI, pipelines have valid `shader` handles, and no legacy shader `use()` calls remain in draw paths.

Good luck with the refactor! Each milestone builds incrementally toward a modern, AI-friendly, production-ready rendering system that solves the current dual-pipeline limitations while future-proofing for next-generation graphics APIs.

## Milestone Plan (A–E)

The following consolidates high‑priority work with clear acceptance criteria. Where an item already exists in this guide, it is referenced rather than duplicated.

### A. Blockers / High Impact

- Adopt MaterialCore end‑to‑end: Replace legacy material fields in `SceneObject`; delete all converters.
  - Acceptance: Both raster and ray read `MaterialCore`; no legacy material structs remain.
  - Notes: Already tracked by Task 22 (MaterialCore Adoption) and Task 30 (Legacy Cleanup Plan). Ensure raytracer reads `MaterialCore` directly; remove PBR↔legacy conversions.

- Wire RenderGraph into main render loop: Drive raster via GBuffer → Lighting → (SSR_T?) → Post → Readback.
  - Acceptance: Opaque scenes match pre-graph output (± tiny FP diffs).
  - Notes: Already tracked by Task 25 (Integrate RenderGraph). Validate parity on core scenes.

- Finish SSR‑T + OIT fallback: Screen‑space refraction; weighted blended OIT for multi‑layer; alpha‑blend fallback if caps missing.
  - Acceptance: Transmissive objects look refractive on WebGL2; logs show graceful fallback on weak devices.
  - Notes: Builds on Task 24.5 (Transmissive Blending pre‑SSR) and Task 29 (Capability Detection). Add SSR‑T pass and WBOIT path; gate features via capability probe.

- Hybrid Auto default & UI surfacing: `--mode auto` default for headless; UI toggle and “reason” text.
  - Acceptance: Users see why Auto chose raster/ray; CLI default is Auto.
  - Notes: UI portion overlaps with Task 26 (Hybrid Mode in UI). Update CLI default in parser/help and print selection reason in logs/UI.

### B. AI‑Support & Automation

- JSON Ops v2: Versioned schema, idempotent ops, explicit seed, `operationId`.
  - Acceptance: Validator can replay ops deterministically.

- REST `/jobs` (thin service): `POST /jobs` with ops → job id; `GET /jobs/{id}` status; `GET /jobs/{id}/outputs` (signed URLs/local paths).
  - Acceptance: End‑to‑end job demo with simple queue and local FS store.

- Determinism & metadata: `--seed` plumbed; write `render.json` (engine version, chosen mode, pass timings, caps).
  - Acceptance: Same seed ⇒ same pixels within tolerance.
  - Notes: Complements Task 28 (Determinism Harness Hook). Add `render.json` emission after readback with timings/caps.

- Validator + goldens: `glint validate --suite core`; SSIM/PSNR thresholds; record pass timings.
  - Acceptance: CI green on core scenes.

### C. Web Robustness & Perf

- WebGL2 compliance pass: Precision qualifiers, MRT guards, NPOT guards, float‑texture fallbacks; size‑budgeted web‑preview build.
  - Acceptance: Core scenes run in browser; WASM meets target size (e.g., ≤7 MB gz).

- Shader toolchain in CI: Build GLSL → SPIR‑V; SPIRV‑Cross to GLSL ES; reflect to verify bindings.
  - Acceptance: CI fails on broken shaders/bindings.

### D. DX & Docs

- Examples & docs migration: Replace `--raytrace` with `--mode`; add Auto examples; write `docs/rendering_arch_v1.md`.
  - Acceptance: Examples run; docs reflect final architecture.
  - Notes: Overlaps Task 27 (CLI/Docs Migration). Ensure deprecation warning and updated samples.

- Profiling & capability detection: Scoped timers per pass; capability probe; clear warnings on fallback.
  - Acceptance: Timings appear in metadata; no crashes on limited hardware.
  - Notes: Extends Task 29 (Capability Detection) with per‑pass timers and inclusion in `render.json`.

### E. Cleanup

- Remove dead code: Delete legacy material structs/converters and stray GL paths outside RhiGL.
  - Acceptance: Repo builds clean; validator passes.
  - Notes: Complements Task 30 (Legacy Cleanup Plan). Ensure no direct GL calls remain outside `engine/src/rhi/rhi-gl.cpp`.

### Quick Wins (do these next)

- Make Auto the default in headless; print the selection reason.
- Add precision qualifiers to shaders and guard non‑WebGL2 features.
- Add `render.json` output with mode + timings (super helpful for demos and debugging).
- Update examples to `--mode` and include a glass scene that shows SSR vs ray.

### Nice Extras (not blockers, big value)

- Weighted blended OIT gives robust layered transparency without per‑pixel linked lists.
- Embree behind a flag speeds ray mode with near‑zero integration risk.
- Web‑preview vs desktop‑final builds keep the web light and desktop powerful.
