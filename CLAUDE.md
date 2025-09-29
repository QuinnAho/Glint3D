# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Common Development Commands

### Building (Desktop)
```bash
# CMake build with Release configuration
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# Visual Studio (Windows) - Modern CMake Workflow
# 1. Generate project files: cmake -S . -B builds/desktop/cmake
# 2. Open builds/desktop/cmake/glint.sln in Visual Studio 2022
# 3. Or open builds/desktop/cmake/glint.vcxproj directly
# 4. Build Configuration: Release|x64 or Debug|x64
# 5. Run from repo root so engine/shaders/ and assets/ paths resolve
#
# Note: Legacy engine/Project1.vcxproj has been removed.
# Always use CMake-generated project files to ensure consistency.

# Convenient build scripts
./build-and-run.bat                    # Windows: Debug build + launch UI
./build-and-run.bat release            # Windows: Release build + launch UI
chmod +x build-and-run.sh && ./build-and-run.sh  # Cross-platform equivalent
```

### Dependencies
**Core Requirements:**
- CMake 3.15+, C++17 compiler, OpenGL 3.3+

**Windows (vcpkg recommended):**
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg && .\bootstrap-vcpkg.bat && .\vcpkg integrate install
.\vcpkg install glfw3:x64-windows assimp:x64-windows openimagedenoise:x64-windows
cmake -S . -B builds/desktop/cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
```

### Building (Web/Emscripten)
```bash
# Complete web build (engine + frontend)
npm run web:build              # Builds WASM engine + React frontend
./tools/build-web.sh          # Direct script execution
./tools/build-web.bat         # Windows batch script

# Manual engine-only build (if needed)
emcmake cmake -S . -B builds/web/emscripten -DCMAKE_BUILD_TYPE=Release
cmake --build builds/web/emscripten -j

# Serve web build
emrun --no_browser --port 8080 builds/web/emscripten/glint.html
```

### Web UI (React/Tailwind)
```bash
cd platforms/web
npm install
npm run dev
# Web UI hosts the engine (WASM) and communicates via JSON Ops

# Desktop app via Tauri
npm run tauri:dev    # Development mode
npm run tauri:build  # Production build

# Or from root directory
npm run dev          # Start development server
npm run build:web    # Build production frontend only
```

---

## Current Web UI Status (v0.3.0)

### Architecture
- **Frontend**: React 18 + TypeScript + Tailwind CSS + Vite
- **Desktop**: Tauri v1.6 wrapper for native desktop experience
- **Engine Integration**: Emscripten WASM module (`glint3d.js/.wasm/.data`)
- **Communication**: JSON Ops v1 bridge via `app_apply_ops_json()` and `app_scene_to_json()`

### Component Structure
```
platforms/web/src/
├── App.tsx                 # Main application layout with grid system
├── components/
│   ├── CanvasHost.tsx      # WebGL canvas initialization and engine loading
│   ├── Inspector.tsx       # Right panel: scene hierarchy, object/light management
│   ├── Console.tsx         # Bottom panel: command input and logging
│   ├── CommandPalette.tsx  # Keyboard shortcuts and quick actions
│   └── TopBar.tsx          # Theme toggle, share link, and navigation
├── sdk/
│   └── viewer.ts           # Legacy Emscripten bridge (use @glint3d/web-sdk)
└── main.tsx               # React app entry point
```

### Environment Setup (Optional)
For easier command-line usage, you can set up `glint` as a global command:

**Windows:**
```bash
# Run setup script (creates wrapper and adds to PATH)
setup_glint_env.bat
```

**Linux/macOS:**
```bash
# Run setup script (creates wrapper and adds to PATH)
source setup_glint_env.sh
```

After setup, you can use `glint` from any directory instead of the full path:
```bash
# Before setup
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output.png

# After setup
glint --ops examples/json-ops/sphere_basic.json --render output.png
```

### Running Examples
```bash
# Headless rendering with JSON Ops
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/directional-light-test.json --render output.png --w 256 --h 256

# Release build
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/directional-light-test.json --render output.png --w 256 --h 256

# CLI with denoise (if OIDN available)
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/studio-turntable.json --render output.png --denoise

# Using global command (after environment setup)
glint --ops examples/json-ops/sphere_basic.json --render output.png --w 512 --h 512
glint --ops examples/json-ops/directional-light-test.json --render output.png --denoise
```

---

## Architecture Overview

### Highlights (v0.3.0 + RHI Updates)
- **✅ RHI Modernization**: FEAT-0248 completed - WebGPU-shaped API with CommandEncoder, RenderPassEncoder, Queue, BindGroups. All uniform operations now route through RHI abstraction. Screen-quad utility added for pass bridging.
- **✅ Unified Render System**: RenderSystem::renderUnified() uses RenderGraph with pass-based architecture. Raytracer integrated as RHI texture output.
- **✅ MaterialCore Unification**: Single material system eliminates dual storage confusion. Legacy Material struct deprecated.
- **✅ Intelligent Mode Selection**: --mode raster|ray|auto with automatic scene analysis via RenderPipelineModeSelector.
- **✅ Pass-Based Architecture**: RasterGraph (GBuffer→Lighting→SSR→Post) vs RayGraph (Integrator→Denoise→Tonemap).
- **Lights**: Point, Directional, and Spot supported end-to-end (engine, shaders, UI, JSON ops). Spot has inverse-square attenuation and smooth cone falloff.
- **UI**: Add/select/delete lights; per-light enable, intensity; type-specific edits (position, direction, cones for spot). Directional and spot indicators are rendered in the viewport.
- **JSON Ops**: `add_light` supports `type: point|directional|spot` with fields `position`, `direction`, and `inner_deg`/`outer_deg` for spot. Schema is in `schemas/json_ops_v1.json`.
- **CI**: Golden image comparison on Linux via ImageMagick (SSIM). Includes a workflow_dispatch job to regenerate candidate goldens.
- **Cross-Platform Graphics**: Engine core successfully decoupled from direct OpenGL calls, prepared for WebGPU backend.
- **Shadows**: Temporarily disabled in shaders for deterministic CI until a proper shadow map path is implemented. Re-enable later via a `useShadows` uniform.

### Core Application Structure
- **Application class** (`application.h/cpp`): Main application lifecycle, OpenGL context, scene management
- **SceneObject struct**: Represents loaded models with VAO/VBO data, materials, textures, and transform matrices
- **Unified rendering system**: Pass-based architecture with intelligent mode selection between raster and ray pipelines
- **Modular UI**: Desktop uses ImGui, Web uses React/Tailwind communicating via JSON Ops bridge

### Key Systems

#### 1. Asset Loading & Import Pipeline
- **Importer registry** (`importer_registry.h/cpp`): Plugin-based asset loading system
- **Built-in OBJ importer** (`importers/obj_importer.cpp`): Core OBJ format support
- **Assimp plugin** (`importers/assimp_importer.cpp`): glTF, FBX, DAE, PLY support (optional)
- **Texture cache** (`texture_cache.h/cpp`): Shared texture management with KTX2/Basis optional support

#### 2. Unified Material System
- **MaterialCore** (`engine/include/glint3d/material_core.h`): Single unified material representation
- **PBR workflow**: baseColor, metallic, roughness, normal, emissive, transmission, IOR
- **Legacy compatibility**: Automatic conversion helpers for raytracer Material API
- **No dual storage**: Eliminates conversion overhead and material drift issues

#### 3. JSON Ops v1 Bridge (`json_ops.cpp`)
- **Cross-platform scripting**: Identical scene manipulation on desktop and web
- **Operations**: load, duplicate, transform, add_light (point|directional|spot), set_camera, set_camera_preset, render
- **Web integration**: Emscripten exports for `app_apply_ops_json()`, `app_scene_to_json()`
- **State sharing**: URL-based state encoding for shareable scenes

#### 4. Unified Rendering Pipeline
- **RenderSystem::renderUnified()**: Pass-based rendering using RenderGraph architecture
- **RasterGraph**: OpenGL rasterization with PBR shaders, SSR approximation
- **RayGraph**: CPU ray tracing with BVH acceleration, physically accurate transmission
- **RenderPipelineModeSelector**: Intelligent mode selection based on scene material analysis
- **RHI Integration**: Both pipelines output to RHI textures for seamless composition
- **RHI Utilities**: Screen-quad buffer utility (`getScreenQuadBuffer()`) for full-screen passes

#### 5. UI Architecture
- **Desktop UI**: ImGui panels integrated directly into Application
- **Web UI**: React/Tailwind app (`platforms/web/`) communicating via JSON Ops bridge
- **UI abstraction**: `app_state.h` provides read-only state snapshot for UI layers
- **Command pattern**: `app_commands.h` for UI → Application actions

## Unified Rendering Architecture (Current Implementation)

### Render Entry Point
- **`RenderSystem::renderUnified(scene, lights)`** - Single entry for interactive and headless flows
- Each frame builds a `PassContext` and executes a render graph with these passes:
  1. `FrameSetupPass` - Viewport, camera, uniforms
  2. `RasterPass` or `RaytracePass` - Main rendering (mode-dependent)
  3. `RayDenoisePass` - AI denoising (ray mode only)
  4. `OverlayPass` - Debug elements, gizmos
  5. `ResolvePass` - MSAA resolve, tonemapping
  6. `ReadbackPass` - Offscreen texture readback
  7. `PresentPass` - Display presentation

### Render Mode Selection

#### Automatic Mode (`--mode auto`)
The `RenderPipelineModeSelector` analyzes the scene and chooses the optimal pipeline:

- **Raster Pipeline**: Fast OpenGL rendering for opaque materials and simple transparency
- **Ray Pipeline**: CPU ray tracing for refractive glass, complex transmission, volumetric effects

#### Selection Criteria
```cpp
// In RenderPipelineModeSelector::selectMode()
for (const auto& material : materials) {
    if (material.transmission > 0.01f && material.ior > 1.05f) {
        return RenderPipelineMode::Ray;  // Needs physically accurate refraction
    }
}
return RenderPipelineMode::Raster;  // Use fast rasterization
```

#### Manual Override
```bash
# Force specific pipeline
./glint --ops scene.json --render out.png --mode raster   # Fast, SSR approximation
./glint --ops scene.json --render out.png --mode ray     # Slow, physically accurate
./glint --ops scene.json --render out.png --mode auto    # Smart selection (default)
```

### Render Modes & Config
- **`raster`**: FrameSetup → Raster → Overlay → Resolve → Present
- **`ray`**: FrameSetup → Raytrace → RayDenoise → Resolve → Present
- **`auto`**: Heuristic choice between raster/ray based on scene analysis
- **Offscreen** (`renderToTextureRHI`, `renderToPNG`): Same graph with `interactive=false`, enables `ReadbackPass`, disables overlays/MSAA

### Key Implementation Files
- **RenderSystem**: `engine/src/render_system.cpp:renderUnified()` - Main unified entry point
- **RenderGraph**: `engine/include/render_pass.h` - Pass-based rendering framework
- **RenderPass**: `engine/src/render_pass.cpp` - Pass implementations
- **Mode Selection**: `engine/include/render_mode_selector.h` - Intelligent pipeline selection
- **Material System**: `engine/include/glint3d/material_core.h` - Unified material representation
- **RHI Layer**: `engine/include/glint3d/rhi.h` - Hardware abstraction interface

---

## Directory Structure & File Organization

### Project Layout
```
Glint3D/
├── engine/                     # Core C++ engine
│   ├── src/                    # Core implementation (rendering, raytracing, loaders)
│   │   ├── importers/         # Asset import plugins (OBJ, Assimp)
│   │   ├── rhi/               # Render Hardware Interface (OpenGL/WebGL2 abstraction)
│   │   └── ui/                # Desktop ImGui integration
│   ├── include/               # Engine headers
│   │   ├── glint3d/          # Public API headers (MaterialCore, RHI)
│   │   └── rhi/              # Internal RHI headers
│   ├── shaders/               # GLSL shaders (PBR, standard, post-processing)
│   ├── resources/             # Build resources (Windows icon, RC files)
│   └── Libraries/             # Third-party dependencies (GLFW, ImGui, GLM, stb)
├── platforms/                  # Platform-specific implementations
│   ├── desktop/               # Desktop platform integration
│   └── web/                   # React/Tailwind web platform
├── packages/                   # NPM packages and SDK components
│   ├── ops-sdk/               # TypeScript SDK for JSON operations
│   └── wasm-bindings/         # WASM/JavaScript bindings
├── sdk/                        # Software Development Kits
│   └── web/                   # Web SDK and TypeScript definitions
├── assets/                     # Runtime content
│   ├── models/                # 3D models (OBJ, GLB, GLTF)
│   ├── textures/              # Texture maps
│   └── img/                   # Images and HDR environments
├── examples/                   # Usage examples and demos
│   └── json-ops/              # JSON Operations examples and test cases
├── tests/                      # Test suites
│   ├── unit/                  # C++ unit tests
│   ├── integration/           # JSON Ops integration tests
│   ├── golden/                # Visual regression testing (golden images)
│   └── security/              # Security vulnerability tests
├── schemas/                    # JSON schema definitions
├── docs/                       # Documentation
├── tools/                      # Development and build tools
├── ai/                         # AI-assisted development configuration
│   ├── config/                # AI constraints and requirements
│   ├── tasks/                 # Task capsule management system
│   └── ontology/              # Semantic definitions
└── builds/                     # Build outputs (generated)
    ├── desktop/cmake/         # CMake desktop builds
    └── web/emscripten/        # Emscripten web builds
```

### File Placement Guidelines

#### Engine Core (`engine/`)
- **Headers**: Place in `engine/include/` for public APIs, `engine/include/glint3d/` for core systems
- **Implementation**: Place in `engine/src/` with matching directory structure
- **Shaders**: All GLSL files go in `engine/shaders/`
- **Resources**: Icons, RC files, etc. in `engine/resources/`

#### Platform Integration (`platforms/`)
- **Desktop**: Platform-specific code in `platforms/desktop/`
- **Web**: React components and web-specific code in `platforms/web/`
- **Never mix**: Platform code should not be in `engine/` core

#### Asset Organization (`assets/`)
- **Models**: 3D files (.obj, .glb, .gltf) in `assets/models/`
- **Textures**: Image files for materials in `assets/textures/`
- **Environments**: HDR/EXR skyboxes in `assets/img/`

#### Testing (`tests/`)
- **Unit Tests**: C++ unit tests in `tests/unit/`
- **Integration**: JSON Ops tests in `tests/integration/`
- **Golden Images**: Reference renders in `tests/golden/references/`
- **Security**: Vulnerability tests in `tests/security/`

#### Documentation & Configuration
- **Schemas**: JSON schema files in `schemas/`
- **Documentation**: User docs in `docs/`
- **AI Configuration**: Task management in `ai/`
- **Build Tools**: Scripts and utilities in `tools/`

---

## Recommended Practices

- **Use RHI**: All GPU work uses RHI interfaces (`engine/include/glint3d/rhi.h`) - no raw GL calls.
- **MaterialCore Only**: Single material struct for all pipelines - conversion helpers eliminated.
- **Pass-Based Design**: Add passes to existing `RenderGraph` rather than creating new pipelines.
- **Unified Interface**: Use `renderUnified()` for new features; legacy `render()` maintained for compatibility.
- **Mode Selection**: Let `RenderPipelineModeSelector` choose pipeline based on scene content - avoid hardcoded modes.
- **File Organization**: Follow directory structure guidelines; keep platform code separate from engine core.
- **RHI Abstraction**: Use RHI layer for all graphics operations to maintain cross-platform compatibility.

---

---

## Security Features

#### Asset Root Directory Restriction (`--asset-root`)
- **Purpose**: Prevents path traversal attacks in JSON operations by restricting file access to a specified directory
- **Usage**: `glint --asset-root /path/to/assets --ops operations.json`
- **Behavior**:
  - All file paths in JSON ops (load, render_image, set_background skybox) are validated against the asset root
  - Paths containing `..` or resolving outside the asset root are rejected with clear error messages
  - Relative paths are resolved relative to the asset root directory
  - Absolute paths must be within the asset root directory tree
- **Backward Compatibility**: When `--asset-root` is not specified, all paths are allowed (existing behavior)
- **Security**: Blocks attempts like `../../../etc/passwd`, `..\\..\\system32\\config`, URL-encoded traversals
- **Testing**: Run `bash test_path_security.sh` to verify security controls are working

---

## Development Workflow

### AI-Driven Task Workflow
1. **Check active task**: Read `ai/tasks/current_index.json` for current focus
2. **Follow task specification**: Read task README.md, coverage.md for requirements
3. **Update task progress**: Maintain progress documentation as work proceeds
4. **Apply minimal changes**: Keep edits surgical and task-focused
5. **Build and test**: Verify desktop + web builds, run relevant tests
6. **Update documentation**: Keep task files and project docs synchronized

### Task-Aligned Requests (2024-12-29 Priority)
Current architecture focus based on active tasks:
- "Migrate renderToTextureRHI to use RHI-only calls" (Phase B)
- "Implement passGBuffer/passDeferredLighting using RHI" (Phase B)
- "Replace screen quad VAO with RHI::getScreenQuadBuffer()" (Ready)
- "Add RHI texture readback for offscreen rendering" (Phase B)

### Legacy Useful Requests (General)
- "Add new pass to RenderGraph (e.g., `SSAOPass`, `BloomPass`)"
- "Add material property to MaterialCore and update JSON schema"
- "Write golden test for headless renderer"
- "Optimize mode selection in `RenderPipelineModeSelector`"

### Workflow Guidelines
- **Task-first approach**: Always align work with current active task
- **RHI-only graphics**: No new direct OpenGL calls (use RHI abstraction)
- **Surgical edits**: Avoid repo-wide renames without explicit request
- **API stability**: Don't change public APIs unless required by task
- **Quality gates**: Maintain performance budgets and test coverage

---

## Modularity Rules
- **Engine Core** must not depend on ImGui, GLFW, React, or Emscripten
- **RHI (Render Hardware Interface)**: abstract OpenGL/WebGL away for future Vulkan/Metal backends
- **JSON Ops** is the single source of truth between engine and UIs
- New features tested in headless CLI first, then integrated into UI

---

## Performance Budget & Metrics
- HUD counters: draw calls, triangles, materials, textures, VRAM estimate, CPU frame time
- **Budgets (default desktop)**:
  - Draw calls < 5k
  - Triangles < 15M
  - Frame time < 16ms @ 1080p
- Suggestions: instancing, LOD/decimation, merging static geometry

---

## Footguns & Gotchas
- **WebGL2 limitations**: no geometry/tessellation shaders; limited texture formats
- **sRGB handling**: avoid double-gamma (FB vs texture decode)
- **Normals**: recompute if missing; winding order must match
- **Headless paths**: must run from repo root or use absolute paths
- **ImGui**: keep out of Engine Core; treat as view/controller only
- **Shadows**: disabled by default in shaders for deterministic CI until a real shadow-map implementation is added (see TODO). Re-enable via `useShadows` when implemented.

---

## AI Workflow Integration

### Task-Based Development System
The project uses a modern AI task capsule system located in `ai/` for systematic development:

```bash
# Check current active task
cat ai/tasks/current_index.json

# Follow task specifications
# 1. Read current_index.json to identify active task
# 2. Read ai/tasks/<task>/README.md for task overview
# 3. Read ai/tasks/<task>/coverage.md for detailed requirements
# 4. Update ai/tasks/<task>/progress.ndjson as work progresses
```

### Current Architecture Priorities (2024-12-29)
Based on `ai/tasks/current_index.json`, the system is focused on:

**✅ Phase A Complete**: OpenGL migration prerequisites
- Framebuffer operations audit completed
- RHI abstraction verified complete
- Screen-quad utility implemented (`RHI::getScreenQuadBuffer()`)

**🔄 Current Focus**: OpenGL→RHI migration Phase B
- Replace direct GL calls in `renderToTextureRHI()`/PNG paths
- Implement RHI-only pass system (GBuffer, DeferredLighting, RayIntegrator, Readback)

**🚀 Ready Tasks**: `pass_bridging` now unblocked
- Can implement pass-based rendering using RHI utilities
- Critical path: opengl_migration → pass_bridging → state_accessibility → ndjson_telemetry

### AI Assistant Guidelines
When working with Claude Code on this project:

1. **Always start by reading the current task status**: `ai/tasks/current_index.json`
2. **Follow task specifications**: Each task has detailed README.md, coverage.md, and mapping.md
3. **Use the task capsule system**: Update progress files as work is completed
4. **Respect architectural constraints**: RHI-first, no direct OpenGL, MaterialCore unified
5. **Follow phase-based approach**: Complete current phase before moving to next

### Quality Gates
- All new graphics code must use RHI abstraction
- Golden tests must pass for any rendering changes
- Architecture compliance verified via task validation
- Performance budgets maintained (frame time, draw calls, memory)

---

*End of file.*