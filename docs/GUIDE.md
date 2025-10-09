# Glint3D: The Living Renderer
## Comprehensive Developer Guide

**Last Updated:** 2025-10-08
**Version:** v0.3.0+
**For:** New developers with C++/game dev background, minimal graphics experience

---

## Table of Contents

**Part I: Vision & Product**
- [What is Glint3D?](#what-is-glint3d)
- [Core Innovation: The Living Renderer](#core-innovation-the-living-renderer)
- [Target Audiences](#target-audiences)
- [Platform Roadmap](#platform-roadmap)

**Part II: Quick Start**
- [Building the Project](#building-the-project)
- [Running Examples](#running-examples)
- [Environment Setup](#environment-setup)

**Part III: Architecture**
- [System Overview](#system-overview)
- [RHI: Render Hardware Interface](#rhi-render-hardware-interface)
- [MaterialCore: Unified Material System](#materialcore-unified-material-system)
- [RenderSystem & Pass-Based Rendering](#rendersystem--pass-based-rendering)
- [Raytracer: CPU Path Tracing](#raytracer-cpu-path-tracing)
- [JSON-Ops: Declarative Operations API](#json-ops-declarative-operations-api)

**Part IV: Development**
- [Key Concepts to Learn](#key-concepts-to-learn)
- [Code Navigation](#code-navigation)
- [Adding Features](#adding-features)
- [Current Development Roadmap](#current-development-roadmap)
- [Contributing](#contributing)

---

# Part I: Vision & Product

## What is Glint3D?

Glint3D is a **hybrid 3D rendering platform** designed for both human creativity and machine automation. It combines real-time rasterization with physically-based ray tracing in a unified, agent-friendly architecture.

### Current Capabilities (v0.3.0+)

- **Dual-mode rendering**: Fast OpenGL rasterization + CPU path tracing
- **Cross-platform**: Desktop (Windows/Linux/macOS) + Web (WASM/WebGL2) + Headless CLI
- **Deterministic execution**: Seeded PRNG, fixed time stepping, byte-stable outputs
- **JSON-Ops scripting**: Declarative scene manipulation via JSON operations
- **PBR materials**: Metallic/roughness workflow with transmission, IOR, clearcoat
- **Modern lighting**: Point, directional, spot lights with physically-based attenuation
- **Asset import**: OBJ, glTF, FBX, DAE, PLY via Assimp plugin system
- **IBL & skyboxes**: HDR environment maps with diffuse/specular convolution
- **Web integration**: React/Tailwind UI communicating via WASM bridge

### What Sets It Apart

**1. Deterministic by Design**
- Seeded RNG ensures identical results across platforms
- Fixed time stepping removes frame-rate dependencies
- Byte-stable PNG outputs for automated testing
- Ideal for CI/CD pipelines and regression testing

**2. Agent-Oriented Architecture**
- Declarative JSON-Ops API (no imperative state management)
- Pure functions with explicit inputs/outputs
- Structured telemetry (NDJSON event logs)
- Content-addressed resource caching (planned)

**3. Intelligent Mode Selection**
- Automatic pipeline selection based on scene analysis
- Raster for opaque materials (fast GPU rendering)
- Ray for refractive glass (accurate transmission/IOR)
- Manual override with `--mode raster|ray|auto`

**4. Unified Material System**
- Single `MaterialCore` struct for both raster and ray pipelines
- No conversion overhead or material drift
- Forward-compatible with advanced features (SSS, anisotropy)

---

## Core Innovation: The Living Renderer

Glint3D is a **self-improving rendering system** that learns from usage patterns and refines its capabilities over time.

### The Concept

Traditional 3D tools are static: they ship with fixed features and require manual updates. Glint3D evolves through:

1. **Structured Telemetry**: Every render operation emits NDJSON events (scene complexity, performance metrics, material usage)
2. **Feedback Loops**: Telemetry feeds back into optimization strategies (LOD selection, pipeline mode, resource budgets)
3. **Agent-Driven Workflows**: AI agents analyze logs and suggest improvements (merge draw calls, simplify materials, adjust budgets)
4. **Deterministic Testing**: Byte-stable outputs enable automated visual regression testing

### Why It Matters

- **For Businesses**: Reduces rendering costs through automated optimization
- **For Developers**: Captures best practices in code, not documentation
- **For AI/ML Labs**: Provides deterministic, scriptable rendering for synthetic data generation
- **For DevOps**: Enables headless rendering pipelines with guaranteed reproducibility

---

## Target Audiences

### 1. Creative Teams (Architecture, Product Visualization)

**Use Case**: High-quality product renders and architectural visualizations

**What They Need**:
- Fast iteration cycles (real-time raster preview)
- Photorealistic output (ray-traced glass, metals, transmission)
- Headless batch rendering for client deliverables

**How Glint3D Helps**:
- Hybrid desktop→web workflow (preview in browser, export from desktop)
- Automatic mode selection (raster for modeling, ray for final renders)
- Shareable URL state (client review without file transfers)

**ROI Example**:
- Traditional: 2-hour Maya render farm job → $50/render
- Glint3D: 15-min ray mode on workstation → $0 (or 5-sec raster preview)

---

### 2. AI/ML Labs (Synthetic Data Generation)

**Use Case**: Generate training data for computer vision models

**What They Need**:
- Deterministic outputs (same scene = same pixels)
- Scriptable workflows (batch generation via JSON)
- Headless execution (Docker/cloud deployment)

**How Glint3D Helps**:
- Seeded PRNG ensures reproducibility
- JSON-Ops API for declarative scene construction
- CPU raytracer fallback (no GPU required in cloud)

**ROI Example**:
- Traditional: $5k/month Blender + GPUs on AWS
- Glint3D: $200/month CPU instances (deterministic, no GPU needed)

---

### 3. Game Studios & Tech Artists

**Use Case**: Pre-viz, concept art, asset verification

**What They Need**:
- Fast feedback (sub-second raster mode)
- Physically accurate previews (ray mode for lighting tests)
- Integration with existing pipelines (glTF, FBX import)

**How Glint3D Helps**:
- Pass-based architecture extensible with custom passes
- Unified MaterialCore matches glTF 2.0 PBR spec
- Web viewer for stakeholder review without game builds

**ROI Example**:
- Traditional: 5-hour lighting iteration cycle
- Glint3D: 30-second raster preview → 2-min ray validation

---

### 4. Businesses Needing Automation

**Use Case**: E-commerce product renders, documentation diagrams

**What They Need**:
- Automated pipelines (CI/CD integration)
- Consistent output quality (no manual tweaking)
- Scalability (thousands of products)

**How Glint3D Helps**:
- Headless CLI (`glint --ops batch.json --render out.png`)
- Deterministic execution (regression testing via golden images)
- JSON-Ops templates (standardized render recipes)

**ROI Example**:
- Traditional: $10/product manual renders → $10k for 1000 SKUs
- Glint3D: $0.10/product automated renders → $100 for 1000 SKUs

---

### 5. DevOps & Infrastructure Engineers

**Use Case**: Integrate rendering into CI/CD pipelines

**What They Need**:
- Headless execution (no GUI dependencies)
- Docker/cloud compatibility (CPU-only rendering)
- Exit codes and structured logs (failure detection)

**How Glint3D Helps**:
- Headless CLI with `--render` flag
- CPU raytracer (no GPU drivers required)
- NDJSON telemetry (machine-parseable logs)

**ROI Example**:
- Traditional: Manual QA of render changes → 2 hours/week
- Glint3D: Automated golden image CI → 0 hours/week

---

## Platform Roadmap

Glint3D evolves through a sequence of **platform releases**, each targeting a specific vertical market.

### Phase 1: VizPack (Current - v0.3.0+)

**Target**: Creative professionals (architects, product designers)

**Features**:
- ✅ Desktop UI (ImGui) + Web UI (React/Tailwind)
- ✅ Hybrid raster/ray rendering with auto mode selection
- ✅ Headless CLI for batch rendering
- ✅ JSON-Ops v1 bridge (load, transform, render operations)
- ✅ Asset import (OBJ, glTF, FBX via Assimp)
- ✅ PBR materials with transmission/clearcoat

**Status**: Shipping to early adopters

---

### Phase 2: DataGen (Q1-Q2 2026)

**Target**: AI/ML labs (synthetic data generation)

**Planned Features**:
- 📋 Deterministic execution hardening (full PRNG audit, time determinism)
- 📋 NDJSON telemetry system (structured event logs)
- 📋 Content-addressed resource caching (hash-based asset deduplication)
- 📋 State accessibility (manager APIs for programmatic control)
- 📋 JSON-Ops v2 (expanded operations, batch commands)

**Why This Matters**:
- Enables automated dataset generation for computer vision
- Reproducible results across different machines/platforms
- Scriptable workflows for large-scale rendering

---

### Phase 3: FinViz (Q3-Q4 2026)

**Target**: Financial data visualization (trading floors, analytics dashboards)

**Planned Features**:
- 📋 Real-time data streaming (live market data → 3D charts)
- 📋 GPU ray tracing (OptiX/Vulkan for sub-second renders)
- 📋 Compute shaders (procedural geometry, data transforms)
- 📋 Custom pass system (volumetric rendering, glow effects)

**Why This Matters**:
- Ultra-low latency rendering for live data
- High-density visualizations (millions of points)
- Interactive manipulation (zoom, filter, highlight)

---

### Phase 4: Verticals (2027+)

**Approach**: Platform customization for specific industries

**Potential Verticals**:
- **Medical**: Volume rendering, surgical planning
- **Geospatial**: Terrain visualization, GIS integration
- **Manufacturing**: CAD import, assembly visualization
- **Education**: Interactive 3D textbooks, science simulations

**Strategy**: Partner with domain experts to build industry-specific toolkits on top of Glint3D core.

---

# Part II: Quick Start

## Building the Project

### Prerequisites

- **CMake 3.15+** (build system)
- **C++17 compiler** (MSVC 2019+, GCC 9+, Clang 10+)
- **OpenGL 3.3+** drivers (for desktop rendering)

### Windows (Recommended: vcpkg)

```bash
# 1. Install vcpkg (Microsoft's C++ package manager)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# 2. Install dependencies
.\vcpkg install glfw3:x64-windows assimp:x64-windows openimagedenoise:x64-windows

# 3. Build Glint3D
cd D:\path\to\Glint3D
cmake -S . -B builds/desktop/cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build builds/desktop/cmake --config Release -j

# 4. Run
.\builds\desktop\cmake\Release\glint.exe
```

### Linux

```bash
# 1. Install dependencies (Ubuntu/Debian)
sudo apt install cmake g++ libglfw3-dev libassimp-dev libopenimagedenoise-dev

# 2. Build
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# 3. Run
./builds/desktop/cmake/glint
```

### macOS

```bash
# 1. Install dependencies via Homebrew
brew install cmake glfw assimp oidn

# 2. Build
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# 3. Run
./builds/desktop/cmake/glint
```

### Visual Studio (Windows)

Glint3D uses a **modern CMake workflow**:

```bash
# 1. Generate Visual Studio project files
cmake -S . -B builds/desktop/cmake

# 2. Open in Visual Studio 2022
# File → Open → Folder → builds/desktop/cmake/glint.sln
# Or directly open builds/desktop/cmake/glint.vcxproj

# 3. Select configuration: Release|x64 or Debug|x64

# 4. Build → Build Solution (Ctrl+Shift+B)

# 5. Run from repo root (so engine/shaders/ paths resolve)
cd D:\path\to\Glint3D
.\builds\desktop\cmake\Release\glint.exe
```

**Note**: Legacy `engine/Project1.vcxproj` has been removed. Always use CMake-generated project files.

---

### Web Build (Emscripten + React)

```bash
# 1. Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Add to ~/.bashrc for persistence

# 2. Build WASM engine + React frontend
cd D:\path\to\Glint3D
npm run web:build

# Or use build scripts
./tools/build-web.sh      # Linux/macOS
./tools/build-web.bat     # Windows

# 3. Development mode (auto-reload)
cd platforms/web
npm install
npm run dev
# Opens http://localhost:5173 with React UI + WASM engine

# 4. Production build
npm run build:web
# Outputs to platforms/web/dist/
```

---

## Running Examples

### Interactive Desktop UI

```bash
# Launch desktop application with ImGui UI
./builds/desktop/cmake/Release/glint.exe

# UI features:
# - Load models via File → Open
# - Add lights (point, directional, spot)
# - Adjust camera presets (front, top, orbit)
# - Toggle render modes (raster, ray, auto)
```

### Headless Rendering (JSON-Ops)

```bash
# Basic sphere example (raster mode)
./builds/desktop/cmake/Release/glint.exe \
  --ops examples/json-ops/sphere_basic.json \
  --render output.png \
  --w 512 --h 512

# Directional light test (auto mode selection)
./builds/desktop/cmake/Release/glint.exe \
  --ops examples/json-ops/directional-light-test.json \
  --render output.png \
  --w 256 --h 256

# Glass material (forced ray mode)
./builds/desktop/cmake/Release/glint.exe \
  --ops examples/json-ops/glass-cube.json \
  --render output.png \
  --mode ray

# Studio turntable with AI denoising
./builds/desktop/cmake/Release/glint.exe \
  --ops examples/json-ops/studio-turntable.json \
  --render output.png \
  --denoise

# Security: restrict file access to assets directory
./builds/desktop/cmake/Release/glint.exe \
  --asset-root D:\path\to\Glint3D\assets \
  --ops operations.json \
  --render output.png
```

**Understanding Render Modes**:

| Mode | Pipeline | When to Use | Speed | Quality |
|------|----------|-------------|-------|---------|
| `raster` | OpenGL GPU | Opaque materials, real-time preview | 60+ FPS | Approximations (SSR) |
| `ray` | CPU path tracing | Refractive glass, transmission, caustics | 1-60 sec | Physically accurate |
| `auto` | Intelligent selection | General use (default) | Adaptive | Scene-dependent |

---

## Environment Setup

For easier command-line usage, set up `glint` as a global command:

### Windows

```bash
# Run setup script (creates wrapper and adds to PATH)
setup_glint_env.bat

# After setup, use 'glint' from any directory
glint --ops examples/json-ops/sphere_basic.json --render output.png
```

### Linux/macOS

```bash
# Run setup script (creates wrapper and adds to PATH)
source setup_glint_env.sh

# After setup, use 'glint' from any directory
glint --ops examples/json-ops/sphere_basic.json --render output.png
```

---

# Part III: Architecture

## System Overview

Glint3D's architecture is built around four core pillars:

```
┌──────────────────────────────────────────────────────────────┐
│                        Application                           │
│  (Desktop UI, Web UI, Headless CLI)                         │
└────────────────────┬─────────────────────────────────────────┘
                     │
          ┌──────────▼──────────┐
          │   JSON-Ops Bridge   │  ← Declarative scripting API
          │  (Cross-platform)   │
          └──────────┬──────────┘
                     │
┌────────────────────▼─────────────────────────────────────────┐
│                    RenderSystem                              │
│  (Unified rendering orchestrator)                           │
│                                                              │
│  ┌─────────────────┐        ┌──────────────────┐          │
│  │  RasterGraph    │        │   RayGraph       │          │
│  │  (GPU pipeline) │        │  (CPU pipeline)  │          │
│  └────────┬────────┘        └────────┬─────────┘          │
│           │                          │                     │
│  ┌────────▼──────────────────────────▼─────────┐          │
│  │           RHI Abstraction Layer              │          │
│  │  (OpenGL 3.3, WebGL2, future Vulkan/WebGPU) │          │
│  └──────────────────────────────────────────────┘          │
└──────────────────────────────────────────────────────────────┘
                     │
          ┌──────────▼──────────┐
          │   MaterialCore      │  ← Unified material system
          │  (Single struct)    │
          └─────────────────────┘
```

### Key Architectural Principles

1. **RHI-First Design**: All GPU operations route through RHI abstraction (no direct OpenGL calls in core systems)
2. **Unified Materials**: Single `MaterialCore` struct eliminates dual storage and conversion overhead
3. **Pass-Based Rendering**: Composable render passes via `RenderGraph` framework
4. **Deterministic Execution**: Seeded RNG and fixed time stepping for reproducible results
5. **Agent-Oriented API**: Declarative JSON-Ops with pure functions and structured data

---

## RHI: Render Hardware Interface

### What is RHI?

The **Render Hardware Interface (RHI)** is a thin abstraction layer over graphics APIs (OpenGL, WebGL2, future Vulkan/WebGPU). It provides a consistent API across platforms while maintaining minimal performance overhead (<5%).

**Location**: `engine/include/glint3d/rhi.h`

### Design Goals

- **Cross-platform**: Same code works on desktop OpenGL 3.3, web WebGL2, future Vulkan/Metal
- **Type safety**: Opaque handles (TextureHandle, BufferHandle) prevent resource mix-ups
- **Future-proof**: WebGPU-shaped API (CommandEncoder, RenderPassEncoder, Queue)
- **Minimal overhead**: Thin wrapper with <5% performance cost

### Core API

```cpp
// RHI interface (engine/include/glint3d/rhi.h)
class RHI {
public:
    // Lifecycle
    virtual bool init(const RhiInit& desc) = 0;
    virtual void shutdown() = 0;

    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // Resource creation
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual SamplerHandle createSampler(const SamplerDesc& desc) = 0;

    // Drawing
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void readback(const ReadbackDesc& desc) = 0;

    // Pipeline state
    virtual void setViewport(int x, int y, int w, int h) = 0;
    virtual void setClearColor(float r, float g, float b, float a) = 0;
    virtual void clear(bool color, bool depth, bool stencil) = 0;

    // WebGPU-shaped frontend (future)
    virtual CommandEncoder* beginCommandEncoder() = 0;
    virtual Queue* getQueue() = 0;
};
```

### Backends

| Backend | Status | Platforms | API Version |
|---------|--------|-----------|-------------|
| OpenGL | ✅ Shipping | Windows, Linux, macOS | 3.3+ |
| WebGL2 | ✅ Shipping | Web browsers | WebGL 2.0 |
| Null | ✅ Shipping | Headless testing | N/A |
| Vulkan | 📋 Planned (2026) | Desktop, mobile | 1.3+ |
| WebGPU | 📋 Planned (2026) | Web browsers | Latest |

### Example Usage

```cpp
// Create texture
TextureDesc texDesc;
texDesc.width = 512;
texDesc.height = 512;
texDesc.format = TextureFormat::RGBA8;
texDesc.usage = TextureUsage::RenderTarget | TextureUsage::Sampled;
TextureHandle colorTarget = rhi->createTexture(texDesc);

// Create vertex buffer
BufferDesc bufDesc;
bufDesc.size = vertices.size() * sizeof(Vertex);
bufDesc.type = BufferType::Vertex;
bufDesc.usage = BufferUsage::Static;
bufDesc.data = vertices.data();
BufferHandle vbo = rhi->createBuffer(bufDesc);

// Draw
DrawDesc drawDesc;
drawDesc.shader = myShader;
drawDesc.vertexBuffer = vbo;
drawDesc.vertexCount = vertices.size();
drawDesc.primitiveType = PrimitiveType::Triangles;
rhi->draw(drawDesc);
```

### Why RHI Matters

**Before RHI** (legacy approach):
- Direct OpenGL calls scattered across codebase
- Platform-specific #ifdef blocks everywhere
- Hard to add new backends (months of work)
- Tight coupling between logic and graphics API

**With RHI** (current architecture):
- Single abstraction point for all GPU operations
- Platform differences handled in backend implementations
- New backends = implement RHI interface (~2-4 weeks)
- Clean separation between rendering logic and hardware

---

## MaterialCore: Unified Material System

### What is MaterialCore?

`MaterialCore` is a **single, unified struct** for material properties used by both rasterization and ray tracing pipelines. It eliminates the need for dual material storage and conversion overhead.

**Location**: `engine/include/glint3d/material_core.h`

### Design Goals

- **Single source of truth**: No separate raster/ray material structs
- **PBR workflow**: Industry-standard metallic/roughness parameters
- **Forward compatible**: Extensible for advanced features (SSS, anisotropy, volume scattering)
- **Asset pipeline friendly**: Maps directly to glTF 2.0 PBR spec

### Structure

```cpp
struct MaterialCore {
    // Core PBR properties
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};  // sRGB + alpha
    float metallic{0.0f};                          // [0,1]
    float roughness{0.5f};                         // [0,1]
    float normalStrength{1.0f};                    // [0,2]
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};         // Linear RGB

    // Transparency & refraction
    float ior{1.5f};                               // Index of refraction [1,3]
    float transmission{0.0f};                      // Transparency [0,1]
    float thickness{0.001f};                       // Volume thickness (meters)
    float attenuationDistance{1.0f};               // Beer-Lambert falloff

    // Advanced properties
    float clearcoat{0.0f};                         // Clear coat strength [0,1]
    float clearcoatRoughness{0.03f};               // Clear coat roughness [0,1]

    // Future extensions (v0.4.1+)
    float subsurface{0.0f};                        // SSS strength [0,1]
    glm::vec3 subsurfaceColor{1.0f, 1.0f, 1.0f};  // SSS tint
    float anisotropy{0.0f};                        // Anisotropic roughness [-1,1]

    // Texture maps
    std::string baseColorTex;
    std::string normalTex;
    std::string metallicRoughnessTex;
    std::string emissiveTex;
    std::string occlusionTex;
    std::string transmissionTex;
    std::string clearcoatTex;
    // ... (10+ texture slots)

    // Metadata
    std::string name;
    uint32_t id{0};
};
```

### Factory Functions

```cpp
// Create common material types
auto metal = MaterialCore::createMetal(glm::vec3(0.95f, 0.93f, 0.88f), 0.1f);
auto plastic = MaterialCore::createDielectric(glm::vec3(0.8f, 0.1f, 0.1f), 0.5f);
auto glass = MaterialCore::createGlass(glm::vec3(1.0f), 1.5f, 0.95f);
auto emitter = MaterialCore::createEmissive(glm::vec3(10.0f, 5.0f, 2.0f), 100.0f);
```

### Classification Utilities

```cpp
// Query material properties
if (mat.isTransparent()) {
    // Requires alpha blending or ray tracing
}

if (mat.needsRaytracing()) {
    // Force ray mode for accurate refraction
    // Triggers when transmission > 0.01 && (ior > 1.05 || thickness > 0)
}

if (mat.isEmissive()) {
    // Contributes to indirect lighting
}
```

### Why MaterialCore Matters

**Single Material System**:
- Both raster and ray pipelines use the same `MaterialCore` struct
- No conversion overhead between systems
- No material drift (raster and ray always match)
- Simplifies asset import (glTF → MaterialCore direct mapping)

**Future-Proof Design**:
- Extensible for advanced features (SSS, anisotropy planned for v0.4.1)
- Maps to modern material models (Disney BSDF, glTF 2.0 extensions)
- Cache-friendly memory layout for GPU upload

---

## RenderSystem & Pass-Based Rendering

### What is RenderSystem?

`RenderSystem` is the **main rendering orchestrator** that manages the entire rendering pipeline. It uses a **pass-based architecture** where rendering is broken into composable stages.

**Location**: `engine/include/render_system.h`, `engine/src/render_system.cpp`

### Manager Systems

RenderSystem delegates responsibilities to specialized managers:

- **CameraManager**: View matrices, projection, FOV, near/far planes
- **LightingManager**: Point/directional/spot lights, shadow maps (planned)
- **MaterialManager**: Material database, texture binding, shader uniforms
- **PipelineManager**: Shader compilation, pipeline state objects

### Unified Rendering Entry Point

```cpp
// Single entry point for all rendering
void RenderSystem::renderUnified(
    const SceneManager& scene,
    const std::vector<Light>& lights
) {
    // 1. Build PassContext with scene data
    PassContext ctx;
    ctx.rhi = rhi_;
    ctx.scene = &scene;
    ctx.lights = &lights;
    ctx.renderer = this;

    // 2. Select pipeline mode (raster vs ray)
    RenderPipelineMode mode = modeSelector.selectMode(scene.materials);

    // 3. Execute render graph
    if (mode == RenderPipelineMode::Raster) {
        executeRasterGraph(ctx);
    } else {
        executeRayGraph(ctx);
    }
}
```

### Render Graphs

Glint3D uses two distinct **render graphs** depending on the pipeline mode:

#### RasterGraph (GPU Rendering)

```
FrameSetupPass      → Set viewport, camera UBOs, clear buffers
     ↓
RasterPass          → OpenGL rasterization with PBR shaders
     ↓
OverlayPass         → Debug elements, gizmos, light indicators
     ↓
ResolvePass         → MSAA resolve, tonemapping
     ↓
PresentPass         → Display to screen
```

**Performance**: 60+ FPS @ 1080p on modern GPUs

#### RayGraph (CPU Rendering)

```
FrameSetupPass      → Set viewport, clear buffers
     ↓
RaytracePass        → CPU path tracing with BVH acceleration
     ↓
RayDenoisePass      → AI denoising (Intel Open Image Denoise)
     ↓
ResolvePass         → Tonemap, gamma correction
     ↓
(ReadbackPass)      → Optional: Copy texture to CPU (headless only)
     ↓
PresentPass         → Display to screen
```

**Performance**: 1-60 seconds depending on scene complexity, samples per pixel

### Intelligent Mode Selection

The `RenderPipelineModeSelector` analyzes scene materials and chooses the optimal pipeline:

```cpp
RenderPipelineMode selectMode(const std::vector<MaterialCore>& materials) {
    for (const auto& mat : materials) {
        // Check if material requires ray tracing
        if (mat.transmission > 0.01f && mat.ior > 1.05f) {
            return RenderPipelineMode::Ray;  // Needs accurate refraction
        }
        if (mat.thickness > 0.0f && mat.transmission > 0.01f) {
            return RenderPipelineMode::Ray;  // Needs volume absorption
        }
    }
    return RenderPipelineMode::Raster;  // Use fast GPU rendering
}
```

**Manual Override**:
```bash
# Force specific mode
glint --ops scene.json --render out.png --mode raster   # Fast, approximations
glint --ops scene.json --render out.png --mode ray      # Slow, accurate
glint --ops scene.json --render out.png --mode auto     # Intelligent (default)
```

### Pass-Based Architecture Benefits

**Composability**: Add new passes without modifying existing code
- Example: Add `SSAOPass` between RasterPass and OverlayPass
- Example: Insert `BloomPass` before ResolvePass

**Reusability**: Passes shared across pipelines
- `FrameSetupPass` used by both raster and ray graphs
- `ResolvePass` handles tonemapping for both modes

**Testability**: Each pass is independently testable
- Unit test `passGBuffer()` without full render graph
- Golden image tests for individual pass outputs

---

## Raytracer: CPU Path Tracing

### What is the Raytracer?

Glint3D includes a **CPU-based path tracer** for physically accurate rendering of complex materials (refractive glass, transmission, caustics). It runs alongside the GPU raster pipeline and outputs to RHI textures for seamless composition.

**Location**: `engine/include/raytracer.h`, `engine/src/raytracer.cpp`

### Architecture

```cpp
class Raytracer {
private:
    std::vector<Triangle> triangles;  // Scene geometry (BVH-accelerated)
    BVHNode* bvhRoot;                 // Bounding volume hierarchy
    SeededRng rng;                    // Deterministic random numbers

public:
    void renderImage(
        std::vector<glm::vec3>& out,  // Output pixel buffer (linear RGB)
        int W, int H,                 // Image dimensions
        const Camera& cam,            // View parameters
        const std::vector<Light>& lights,
        int samplesPerPixel = 16,     // Path tracing samples
        int maxBounces = 8            // Ray recursion depth
    );
};
```

### Features

- **BVH acceleration**: Spatial partitioning for fast ray-triangle intersection
- **Deterministic**: Seeded RNG ensures identical results across runs
- **Physically-based**: Cook-Torrance BRDF, Fresnel equations, IOR-correct refraction
- **Multi-sampling**: 1-256 samples per pixel (quality vs speed trade-off)
- **AI denoising**: Intel Open Image Denoise integration (optional `--denoise` flag)

### Why CPU, Not GPU?

**Current Implementation (v0.3.0)**:

Glint3D uses a **pragmatic CPU raytracer** for several reasons:

1. **Deterministic execution**: CPU path tracing with seeded RNG ensures byte-stable outputs across platforms (critical for CI/CD)
2. **Simplicity**: No need for OptiX/DXR/Vulkan ray tracing extensions (reduces dependencies)
3. **Portability**: Works on any platform with a C++ compiler (cloud/headless/macOS)
4. **Development velocity**: Faster iteration on material models and BRDFs

**Future GPU Path (Planned Q3-Q4 2026)**:

When determinism hooks are fully validated and telemetry is in place, Glint3D will add **GPU ray tracing** via:
- OptiX (NVIDIA RTX GPUs)
- Vulkan Ray Tracing (cross-vendor)
- WebGPU Compute Shaders (web platform)

**Trade-offs**:

| Aspect | CPU Raytracer (Current) | GPU Raytracer (Planned) |
|--------|-------------------------|-------------------------|
| Speed | 1-60 sec per frame | <1 sec per frame |
| Determinism | ✅ Guaranteed (seeded RNG) | ⚠️ Requires careful validation |
| Portability | ✅ Runs anywhere | ⚠️ Requires RT cores/compute |
| Complexity | Simple (300 LOC) | Complex (shader code, kernel management) |

The CPU raytracer **will remain** even after GPU path tracing is added, serving as a **reference implementation** for validation and deterministic testing.

### Integration with RHI

The raytracer outputs to RHI textures via the `updateTexture()` API:

```cpp
// Render to CPU buffer
std::vector<glm::vec3> pixels(width * height);
raytracer.renderImage(pixels, width, height, camera, lights);

// Upload to RHI texture
TextureHandle rayOutput = rhi->createTexture(texDesc);
rhi->updateTexture(rayOutput, pixels.data(), pixels.size() * sizeof(glm::vec3));

// Use in raster pipeline (e.g., composite over UI)
rhi->bindTexture(rayOutput, 0);
rhi->draw(screenQuadDesc);
```

---

## JSON-Ops: Declarative Operations API

### What is JSON-Ops?

**JSON-Ops** is a declarative API for scene manipulation via JSON commands. It enables cross-platform scripting, headless rendering, and web integration without imperative C++ code.

**Location**: `engine/src/json_ops.cpp`, `schemas/json_ops_v1.json`

### Design Goals

- **Declarative**: Describe *what* to do, not *how* (no loops, conditionals, state mutation)
- **Cross-platform**: Identical behavior on desktop, web, headless CLI
- **Version-controlled**: JSON schema ensures backward compatibility
- **Agent-friendly**: Machine-parseable for AI-driven workflows

### Core Operations

```json
{
  "version": "0.1",
  "operations": [
    {
      "type": "load",
      "path": "assets/models/teapot.obj",
      "name": "myTeapot"
    },
    {
      "type": "set_transform",
      "target": "myTeapot",
      "position": [0, 1, 0],
      "rotation": [0, 45, 0],
      "scale": [2, 2, 2]
    },
    {
      "type": "add_light",
      "light_type": "point",
      "position": [5, 10, 5],
      "color": [1, 1, 1],
      "intensity": 100
    },
    {
      "type": "set_camera",
      "position": [10, 10, 10],
      "lookAt": [0, 0, 0],
      "fov": 45
    },
    {
      "type": "render",
      "mode": "auto",
      "samples": 16,
      "output": "teapot.png",
      "width": 512,
      "height": 512
    }
  ]
}
```

### Supported Operations (v0.1)

| Operation | Purpose | Parameters |
|-----------|---------|------------|
| `load` | Import 3D model | path, name, transform |
| `duplicate` | Clone existing object | source, newName, transform |
| `set_transform` | Move/rotate/scale | target, position, rotation, scale |
| `add_light` | Create light | type (point/directional/spot), position, color, intensity |
| `set_camera` | Position camera | position, lookAt, fov, near, far |
| `set_camera_preset` | Use camera preset | preset (front/top/orbit/perspective) |
| `set_background` | Skybox/IBL | type (color/skybox), path (for HDR), color |
| `render` | Execute render | mode (raster/ray/auto), samples, output, width, height |

### Web Integration

The web platform uses JSON-Ops to communicate between React UI and WASM engine:

```typescript
// TypeScript (React UI)
const ops = {
  version: "0.1",
  operations: [
    { type: "load", path: "/models/suzanne.glb", name: "monkey" },
    { type: "set_camera_preset", preset: "orbit" }
  ]
};

// Send to WASM engine
const result = Module.app_apply_ops_json(JSON.stringify(ops));

// Get current scene state
const sceneJSON = Module.app_scene_to_json();
const scene = JSON.parse(sceneJSON);
console.log(scene.objects);  // [{ name: "monkey", transform: {...} }]
```

### Headless Rendering

```bash
# Execute JSON ops and render to PNG
glint --ops examples/json-ops/sphere_basic.json --render output.png

# Security: restrict file access to assets directory
glint --asset-root ./assets --ops ops.json --render out.png
```

### Future Enhancements (JSON-Ops v0.2, planned 2026)

- **Batch operations**: `repeat`, `foreach`, `parametric_sweep`
- **State queries**: `get_object`, `get_material`, `get_lights`
- **Conditional logic**: `if_exists`, `switch_mode`
- **Resource management**: `cache_material`, `preload_texture`

---

# Part IV: Development

## Key Concepts to Learn

To contribute to Glint3D, you'll need foundational knowledge in graphics programming. Here are the essential concepts and recommended resources.

### 1. OpenGL Fundamentals

**Why**: Glint3D's RHI is currently OpenGL 3.3+ (desktop) and WebGL2 (web)

**Topics to Learn**:
- Vertex buffers, index buffers, vertex array objects (VAOs)
- Shader compilation (vertex, fragment stages)
- Textures (2D, cubemaps, mipmaps, sampling)
- Framebuffers and render targets
- Depth testing, blending, culling

**Recommended Resources**:
- **[LearnOpenGL.com](https://learnopengl.com/)** - Comprehensive tutorials (Getting Started → Lighting → Model Loading)
- Focus chapters: Hello Triangle, Shaders, Textures, Coordinate Systems, Lighting

**Time Investment**: 1-2 weeks for basics, 1 month for intermediate

---

### 2. Physically-Based Rendering (PBR)

**Why**: Glint3D uses PBR materials with metallic/roughness workflow

**Topics to Learn**:
- Cook-Torrance BRDF (diffuse + specular lobes)
- Fresnel equations (Schlick approximation)
- Normal distribution functions (GGX/Trowbridge-Reitz)
- Image-based lighting (diffuse/specular convolution)
- Gamma correction and tonemapping

**Recommended Resources**:
- **[LearnOpenGL PBR](https://learnopengl.com/PBR/Theory)** - PBR theory and implementation
- **[Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)** - Industry-standard reference
- **[Filament PBR Guide](https://google.github.io/filament/Filament.html)** - Google's material system

**Time Investment**: 2-3 weeks for theory, 1-2 months for shader implementation

---

### 3. Ray Tracing Concepts

**Why**: Glint3D's ray mode uses CPU path tracing for accurate rendering

**Topics to Learn**:
- Ray-sphere, ray-triangle intersection
- Bounding volume hierarchies (BVH)
- Monte Carlo integration and importance sampling
- Path tracing algorithm (recursive bounces)
- Fresnel, refraction, transmission

**Recommended Resources**:
- **[Ray Tracing in One Weekend](https://raytracing.github.io/)** - Hands-on path tracer tutorial
- **[PBRT Book](https://pbr-book.org/)** - Physically-Based Rendering bible (advanced)
- **Scratchapixel**: [Ray Tracing](https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/how-does-it-work.html)

**Time Investment**: 1-2 weeks for basics, 1-2 months for advanced techniques

---

### 4. Render Graphs

**Why**: Glint3D uses pass-based architecture with RenderGraph

**Topics to Learn**:
- Framegraph/RenderGraph concepts (passes, resources, dependencies)
- Deferred rendering (GBuffer, lighting pass)
- Post-processing (tonemapping, bloom, SSAO)
- Resource barriers and synchronization

**Recommended Resources**:
- **[FrameGraph: Extensible Rendering Architecture in Frostbite](https://www.gdcvault.com/play/1024612/)** - GDC talk by Yuriy O'Donnell
- **[Render Graphs and Vulkan](https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/)** - Hans-Kristian Arntzen
- **Glint3D codebase**: `engine/include/render_pass.h` for concrete examples

**Time Investment**: 1-2 weeks for concepts, 1 month for implementation patterns

---

### 5. Deterministic Systems

**Why**: Glint3D guarantees reproducible results for CI/CD and synthetic data generation

**Topics to Learn**:
- Pseudo-random number generators (PRNG) and seeding
- Fixed time stepping vs variable delta time
- Floating-point determinism (IEEE 754, associativity issues)
- Hash functions for content addressing

**Recommended Resources**:
- **[Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/)** - Glenn Fiedler
- **[Deterministic Lockstep](https://gafferongames.com/post/deterministic_lockstep/)** - Multiplayer networking concepts
- **Glint3D codebase**: `ai/tasks/determinism_hooks/checklist.md` for requirements

**Time Investment**: 1 week for theory, 2-3 weeks for debugging edge cases

---

## Code Navigation

Understanding where to find specific functionality in the codebase.

### Directory Structure

```
Glint3D/
├── engine/                       # Core C++ engine
│   ├── include/                  # Public headers
│   │   ├── glint3d/             # Core API (RHI, MaterialCore, UniformBlocks)
│   │   │   ├── rhi.h            # Render Hardware Interface
│   │   │   ├── rhi_types.h      # RHI type definitions
│   │   │   ├── material_core.h  # Unified material system
│   │   │   └── uniform_blocks.h # UBO structures
│   │   ├── rhi/                 # Internal RHI headers
│   │   │   ├── rhi_gl.h         # OpenGL backend
│   │   │   ├── rhi_webgl.h      # WebGL2 backend (planned)
│   │   │   └── rhi_null.h       # Headless/testing backend
│   │   ├── render_system.h      # Main rendering orchestrator
│   │   ├── render_pass.h        # Pass-based architecture
│   │   ├── raytracer.h          # CPU path tracer
│   │   ├── application.h        # Application lifecycle
│   │   ├── json_ops.h           # JSON operations API
│   │   └── ...                  # Other systems
│   ├── src/                     # Implementation files
│   │   ├── rhi/                 # RHI backend implementations
│   │   │   └── rhi_gl.cpp       # OpenGL backend (1200+ LOC)
│   │   ├── importers/           # Asset loading plugins
│   │   │   ├── obj_importer.cpp # OBJ loader
│   │   │   └── assimp_importer.cpp  # glTF/FBX loader
│   │   ├── render_system.cpp    # RenderSystem implementation
│   │   ├── render_pass.cpp      # Pass implementations
│   │   ├── raytracer.cpp        # Path tracing logic
│   │   ├── application_core.cpp # Application loop
│   │   ├── json_ops.cpp         # JSON ops executor
│   │   └── ...
│   ├── shaders/                 # GLSL shader code
│   │   ├── pbr.vert             # PBR vertex shader
│   │   ├── pbr.frag             # PBR fragment shader (300+ LOC)
│   │   ├── gbuffer.{vert,frag}  # Deferred rendering
│   │   ├── deferred.{vert,frag} # Deferred lighting
│   │   ├── skybox.{vert,frag}   # Skybox rendering
│   │   └── ...
│   └── Libraries/               # Third-party dependencies
│       ├── glfw/                # Windowing (desktop)
│       ├── imgui/               # Desktop UI
│       ├── glm/                 # Math library
│       ├── stb/                 # Image I/O (stb_image, stb_image_write)
│       └── ...
├── platforms/                   # Platform-specific code
│   ├── desktop/                 # Desktop platform
│   └── web/                     # React/Tailwind web UI
│       ├── src/
│       │   ├── App.tsx          # Main React app
│       │   ├── components/
│       │   │   ├── CanvasHost.tsx   # WebGL canvas + WASM loader
│       │   │   ├── Inspector.tsx    # Right panel (scene hierarchy)
│       │   │   └── Console.tsx      # Command input
│       │   └── sdk/
│       │       └── viewer.ts    # WASM bridge utilities
│       └── public/
│           └── glint3d.{js,wasm,data}  # Emscripten outputs
├── assets/                      # Runtime content
│   ├── models/                  # 3D models (OBJ, GLB, GLTF)
│   ├── textures/                # Material textures
│   └── img/                     # HDR environments
├── examples/                    # Usage examples
│   └── json-ops/                # JSON operations examples
│       ├── sphere_basic.json
│       ├── directional-light-test.json
│       └── glass-cube.json
├── tests/                       # Test suites
│   ├── unit/                    # C++ unit tests
│   ├── integration/             # JSON ops tests
│   ├── golden/                  # Visual regression tests
│   │   ├── scenes/              # Test scenes (JSON ops)
│   │   └── references/          # Golden reference images
│   └── security/                # Security tests (path traversal)
├── ai/                          # AI-driven development
│   ├── tasks/                   # Task capsule system
│   │   ├── current_index.json   # Active task status
│   │   ├── pass_bridging/       # Current focus
│   │   ├── state_accessibility/ # Next task
│   │   └── ...
│   └── config/                  # AI constraints
├── schemas/                     # JSON schema definitions
│   └── json_ops_v1.json         # JSON-Ops v0.1 schema
├── docs/                        # Documentation
│   ├── GUIDE.md                 # This file
│   ├── README.md                # Documentation index
│   └── ...
└── tools/                       # Build and development tools
    ├── build-web.sh             # Web build script (Linux/macOS)
    └── build-web.bat            # Web build script (Windows)
```

### Quick Lookup: "Where is...?"

| Component | Location |
|-----------|----------|
| RHI interface | `engine/include/glint3d/rhi.h` |
| OpenGL backend | `engine/src/rhi/rhi_gl.cpp` |
| Material definition | `engine/include/glint3d/material_core.h` |
| Main render loop | `engine/src/render_system.cpp:renderUnified()` |
| Pass-based architecture | `engine/include/render_pass.h` |
| CPU raytracer | `engine/src/raytracer.cpp` |
| PBR shader code | `engine/shaders/pbr.frag` |
| JSON-Ops executor | `engine/src/json_ops.cpp` |
| Asset import | `engine/src/importers/` |
| Desktop UI | `engine/src/ui/` (ImGui panels) |
| Web UI | `platforms/web/src/App.tsx` |
| Task roadmap | `ai/tasks/current_index.json` |

---

## Adding Features

### Example: Adding Anisotropy to Materials

Let's walk through adding **anisotropic roughness** to the material system.

#### Step 1: Update MaterialCore

Edit `engine/include/glint3d/material_core.h`:

```cpp
struct MaterialCore {
    // ... existing fields ...

    float anisotropy{0.0f};              // NEW: Anisotropic roughness [-1,1]
    float anisotropyRotation{0.0f};      // NEW: Tangent rotation (radians)
    std::string anisotropyTex;           // NEW: Anisotropy texture (R=strength, G=rotation)

    // ... rest of struct ...
};
```

#### Step 2: Update Uniform Blocks

Edit `engine/include/glint3d/uniform_blocks.h`:

```cpp
struct MaterialUniforms {
    glm::vec4 baseColor;
    float metallic;
    float roughness;
    float anisotropy;         // NEW: Pass to shader
    float anisotropyRotation; // NEW
    // ... rest of uniforms ...
};
```

#### Step 3: Update PBR Shader

Edit `engine/shaders/pbr.frag`:

```glsl
uniform MaterialUniforms {
    vec4 baseColor;
    float metallic;
    float roughness;
    float anisotropy;         // NEW
    float anisotropyRotation; // NEW
    // ...
} uMaterial;

// Anisotropic GGX distribution
float D_GGX_Anisotropic(vec3 N, vec3 H, vec3 T, vec3 B, float roughness, float anisotropy) {
    float alphaX = roughness * (1.0 + anisotropy);
    float alphaY = roughness * (1.0 - anisotropy);

    float NdotH = max(dot(N, H), 0.0);
    float TdotH = dot(T, H);
    float BdotH = dot(B, H);

    float denom = (TdotH * TdotH) / (alphaX * alphaX) +
                  (BdotH * BdotH) / (alphaY * alphaY) +
                  NdotH * NdotH;

    return 1.0 / (PI * alphaX * alphaY * denom * denom);
}

void main() {
    // ... existing PBR code ...

    // Use anisotropic distribution if enabled
    float D = (abs(uMaterial.anisotropy) > 0.01)
        ? D_GGX_Anisotropic(N, H, T, B, uMaterial.roughness, uMaterial.anisotropy)
        : D_GGX(N, H, uMaterial.roughness);

    // ... rest of shader ...
}
```

#### Step 4: Update Raytracer

Edit `engine/src/raytracer.cpp`:

```cpp
glm::vec3 Raytracer::sampleBRDF(const MaterialCore& mat, const glm::vec3& wo, const glm::vec3& normal) {
    if (std::abs(mat.anisotropy) > 0.01f) {
        // Anisotropic sampling: stretch roughness along tangent/bitangent
        glm::vec3 tangent = computeTangent(normal);
        glm::vec3 bitangent = glm::cross(normal, tangent);

        float alphaX = mat.roughness * (1.0f + mat.anisotropy);
        float alphaY = mat.roughness * (1.0f - mat.anisotropy);

        // Sample anisotropic lobe
        return sampleAnisotropicGGX(tangent, bitangent, alphaX, alphaY, rng);
    } else {
        // Isotropic sampling (existing code)
        return sampleIsotropicGGX(normal, mat.roughness, rng);
    }
}
```

#### Step 5: Update JSON Schema

Edit `schemas/json_ops_v1.json`:

```json
{
  "definitions": {
    "Material": {
      "properties": {
        "baseColor": { "type": "array", "items": { "type": "number" }, "minItems": 3, "maxItems": 4 },
        "metallic": { "type": "number", "minimum": 0, "maximum": 1 },
        "roughness": { "type": "number", "minimum": 0, "maximum": 1 },
        "anisotropy": { "type": "number", "minimum": -1, "maximum": 1 },
        "anisotropyRotation": { "type": "number", "minimum": 0, "maximum": 6.28318 }
      }
    }
  }
}
```

#### Step 6: Add UI Controls

Edit `platforms/web/src/components/Inspector.tsx`:

```tsx
<div className="material-editor">
  <label>Anisotropy</label>
  <input
    type="range"
    min="-1"
    max="1"
    step="0.01"
    value={material.anisotropy}
    onChange={(e) => updateMaterial({ anisotropy: parseFloat(e.target.value) })}
  />
  <span>{material.anisotropy.toFixed(2)}</span>
</div>
```

#### Step 7: Test

```bash
# Build and test
cmake --build builds/desktop/cmake --config Release -j
./builds/desktop/cmake/Release/glint.exe --ops tests/materials/anisotropic_metal.json --render test_aniso.png

# Verify output
# Expected: Brushed metal appearance with directional highlights
```

---

## Current Development Roadmap

Glint3D follows a **task-based development model** managed in `ai/tasks/`. All work aligns with the critical path toward agent-driven workflows.

### Critical Path (7 Tasks)

```
[94% Complete] opengl_migration
        ↓
   [CURRENT] pass_bridging          ← Ready to start
        ↓
 state_accessibility                ← Blocked by pass_bridging
        ↓
  ndjson_telemetry                  ← Blocked by state_accessibility
        ↓
 determinism_hooks                  ← Blocked by ndjson_telemetry
        ↓
  resource_model                    ← Blocked by determinism_hooks
        ↓
 json_ops_runner                    ← End goal (agent-driven workflows)
```

### Current Focus: Pass Bridging

**Task**: Implement RHI-only pass system for composable rendering

**Status**: Ready to start (prerequisites met 2024-12-29)

**Checklist** (`ai/tasks/pass_bridging/checklist.md`):
- [ ] `passGBuffer()`: Deferred rendering GBuffer generation
- [ ] `passDeferredLighting()`: Lighting pass with G-buffer inputs
- [ ] `passRayIntegrator()`: Ray tracing integration pass
- [ ] `passReadback()`: Texture readback for offscreen rendering

**Why It Matters**:
- Enables modular rendering architecture
- Simplifies adding new effects (SSAO, bloom, volumetrics)
- Foundation for state accessibility (next task)

---

### Next Task: State Accessibility

**Task**: Expose manager state via `getState()` / `applyState()` APIs

**Blocked By**: pass_bridging

**Preparation Work**:
- Design manager API contracts (`ai/tasks/state_accessibility/coverage.md`)
- Define JSON serialization format for each manager
- Plan rollback/undo mechanisms

**Why It Matters**:
- Enables programmatic control of all rendering state
- Foundation for telemetry (track state changes)
- Required for JSON-Ops v2 (state queries)

---

### Future Tasks

**ndjson_telemetry** (Q1 2026):
- Structured event logging (scene complexity, performance metrics, material usage)
- Buffered writer system (low overhead)
- Schema: `{ "event": "render_start", "timestamp": ..., "scene_complexity": {...} }`

**determinism_hooks** (Q2 2026):
- Extend SeededRng to all randomness sources
- Validate byte-stability across platforms
- Time determinism (fixed stepping, no wall-clock dependencies)

**resource_model** (Q2-Q3 2026):
- Content-addressed cache (hash-based lookup)
- Handle tables (opaque IDs for resources)
- Deduplication (multiple objects sharing textures)

**json_ops_runner** (Q4 2026):
- Pure operation executor (no global state)
- Sandboxed execution (resource limits, timeouts)
- Batch operations (loops, conditionals)

---

## Contributing

### Workflow

1. **Check current task**: `cat ai/tasks/current_index.json`
2. **Read task specification**:
   - `ai/tasks/<task>/README.md` (overview)
   - `ai/tasks/<task>/checklist.md` (atomic steps)
   - `ai/tasks/<task>/coverage.md` (detailed requirements)
3. **Make changes**: Follow code style guidelines
4. **Build and test**:
   ```bash
   cmake --build builds/desktop/cmake --config Release -j
   ./builds/desktop/cmake/Release/glint.exe --ops tests/golden/scenes/raster-pipeline.json --render test_output.png
   ```
5. **Update progress**: Edit `ai/tasks/<task>/progress.ndjson` (append NDJSON entries)
6. **Submit PR**: Reference task in PR title (e.g., "pass_bridging: Implement passGBuffer()")

### Code Style

- **C++17 standards**: Use modern features (std::optional, if constexpr, structured bindings)
- **RAII patterns**: Wrap resources in classes with destructors (TextureHandle, BufferHandle)
- **Const correctness**: Mark read-only parameters as `const`, use `const` methods
- **Naming conventions**:
  - Classes: PascalCase (RenderSystem, MaterialCore)
  - Functions: camelCase (renderUnified, createTexture)
  - Variables: camelCase (baseColor, metallic)
  - Constants: UPPER_SNAKE_CASE (MAX_LIGHTS, PI)
  - Private members: trailing underscore (rhi_, scene_)

### Testing Guidelines

**Golden Image Tests**:
```bash
# Run visual regression tests
cd tests/golden
bash run_golden_tests.sh

# Expected output: PASS for all scenes
# If FAIL, inspect diff images in tests/golden/diffs/
```

**Unit Tests**:
```bash
# Build and run C++ unit tests
cmake --build builds/desktop/cmake --target run_tests
./builds/desktop/cmake/tests/unit_tests
```

**Security Tests**:
```bash
# Verify path traversal protection
bash tests/security/test_path_security.sh
```

### Performance Budget

Maintain these limits for raster mode @ 1080p:

- **Frame time**: <16ms (60 FPS)
- **Draw calls**: <5000
- **Triangles**: <15M
- **VRAM usage**: <2GB

Use HUD counters to monitor:
```cpp
// Enable performance overlay (desktop UI)
ImGui::Begin("Performance");
ImGui::Text("Draw Calls: %d", renderStats.drawCalls);
ImGui::Text("Triangles: %d", renderStats.triangles);
ImGui::Text("Frame Time: %.2f ms", renderStats.frameTimeMs);
ImGui::End();
```

### Documentation Standards

- **Public APIs**: Use Doxygen comments (`/** @brief ... */`)
- **Complex algorithms**: Inline comments explaining "why", not "what"
- **Shader code**: Document uniforms, inputs, outputs at top of file
- **JSON schemas**: Annotate with `description` fields

---

## Appendix: Glossary

| Term | Definition |
|------|------------|
| **RHI** | Render Hardware Interface - abstraction layer over graphics APIs |
| **MaterialCore** | Unified material struct for both raster and ray pipelines |
| **Pass-Based Rendering** | Architecture where rendering is broken into composable stages |
| **RenderGraph** | Framework for managing render passes and dependencies |
| **PBR** | Physically-Based Rendering - material system based on physical properties |
| **BVH** | Bounding Volume Hierarchy - spatial acceleration structure for ray tracing |
| **BRDF** | Bidirectional Reflectance Distribution Function - describes surface reflection |
| **Cook-Torrance** | Microfacet BRDF model (diffuse + specular lobes) |
| **GGX** | Trowbridge-Reitz normal distribution function (anisotropic highlights) |
| **IBL** | Image-Based Lighting - lighting from HDR environment maps |
| **JSON-Ops** | Declarative API for scene manipulation via JSON operations |
| **NDJSON** | Newline-Delimited JSON - streaming JSON format for telemetry |
| **SeededRng** | Deterministic random number generator with explicit seed |
| **Content-Addressed** | Resource lookup by content hash (enables deduplication) |
| **Agent-Oriented** | Architecture designed for AI-driven workflows (declarative, pure functions) |

---

## Appendix: Learning Roadmap

### Week 1: Foundation

**Goals**: Understand the "why" and set up development environment

- [ ] Read `docs/GUIDE.md` Part I (Vision & Product)
- [ ] Read `docs/GUIDE.md` Part II (Quick Start)
- [ ] Build desktop version successfully
- [ ] Run headless example: `glint --ops examples/json-ops/sphere_basic.json --render out.png`
- [ ] Explore web UI: `npm run dev` (platforms/web/)

**External Learning**:
- [ ] LearnOpenGL.com: "Getting Started" section (Hello Triangle, Shaders, Textures)
- [ ] Total time: ~10 hours

---

### Week 2-3: Architecture Deep Dive

**Goals**: Understand core systems and their interactions

- [ ] Read `engine/include/glint3d/rhi.h` (300 lines, well-commented)
- [ ] Read `engine/include/glint3d/material_core.h` (150 lines)
- [ ] Read `engine/include/render_pass.h` (280 lines)
- [ ] Trace code path: `main()` → `Application::run()` → `RenderSystem::renderUnified()`
- [ ] Modify a material property (roughness) and observe raster vs ray output

**External Learning**:
- [ ] LearnOpenGL.com: "Lighting" section (Basic Lighting, Materials)
- [ ] LearnOpenGL.com: "Model Loading" section
- [ ] Total time: ~15 hours

---

### Week 4: First Contribution

**Goals**: Make a small, self-contained contribution

**Beginner Tasks**:
- Add a new camera preset (e.g., "closeup": position = [2, 2, 2])
- Add a new material factory function (e.g., `createPlastic()`)
- Fix a UI issue in Inspector.tsx (alignment, color)

**Intermediate Tasks**:
- Implement `passGBuffer()` (first pass in pass_bridging task)
- Add SSAO pass to RasterGraph
- Write a golden test for a new scene

**External Learning**:
- [ ] Read `ai/tasks/current_index.json` and active task README
- [ ] Total time: ~20 hours

---

### Month 2+: Advanced Topics

**Goals**: Contribute to critical path tasks

- Implement remaining passes (passDeferredLighting, passRayIntegrator, passReadback)
- Design manager state APIs for state_accessibility task
- Prototype NDJSON telemetry event schema
- Extend JSON-Ops with new operations (e.g., `set_material`, `animate_camera`)

**External Learning**:
- [ ] PBRT Book: Chapters 1-7 (Monte Carlo, Light Transport)
- [ ] FrameGraph GDC talk (Yuriy O'Donnell)
- [ ] Filament PBR Guide (Google)
- [ ] Total time: ~40+ hours

---

**End of Guide**

---

*For questions or feedback, file an issue at [github.com/glint3d/glint3d/issues](https://github.com/glint3d/glint3d/issues) (placeholder).*

*Last updated: 2025-10-08*
