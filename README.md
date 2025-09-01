# OpenGL AI Viewer + Raytracer

A modern **OpenGL viewer and CPU raytracer** with **AI-powered tools** and cross-platform builds. Runs on desktop (GLFW/GLAD/ImGui) and web (WebGL2 via Emscripten) with a new React/Tailwind UI.  

Key features:
- Natural-language commands via AI → JSON Ops v1 scripting  
- Raster path with **PBR (Cook–Torrance)** and flat/Gouraud modes  
- CPU raytracer with BVH + optional OpenImageDenoise  
- **“Why is it black?” diagnostics** with one-click fixes  
- **Performance Coach HUD** with actionable suggestions  
- Shareable scene state (JSON Ops v1) + headless CLI renderer  

<img width="1893" height="1015" alt="image" src="https://github.com/user-attachments/assets/5642caaf-f48f-44dd-8f71-e3a90433a2d0" />

---

## Quick Start

### Desktop (Windows)
- Open `Project1/Project1.vcxproj` in Visual Studio 2022 (x64).  
- Build and run from the repo root so `shaders/` and `assets/` resolve.  
- Use the top menu to load sample models or recipes. Hold RMB to fly the camera.  

### Desktop (CMake, cross-platform)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/objviewer
```

### Web (Emscripten)
```bash
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j
```
Open `build-web/objviewer.html` with a local web server. Assets under `Project1/assets` and `Project1/shaders` are preloaded.  

---

## New Web UI (React + Tailwind) [IN DEVELOPMENT]

<img width="1040" height="900" alt="ui" src="https://github.com/user-attachments/assets/20997c6d-c1c6-44b5-942a-4bb1d666f7a9" />

Located in `ui/`. This is a modern, product-ready interface that hosts the engine (WASM) in the same page.  

1. Build the web engine (see above). Copy outputs to:
   - `ui/public/engine/objviewer.js`  
   - `ui/public/engine/objviewer.wasm`  
   - `ui/public/engine/objviewer.data` (if generated)  

2. Start the UI:
```bash
cd ui
npm install
npm run dev
```
Open the printed URL. Drag & drop `.obj/.ply/.glb` onto the canvas. Use the Console to run JSON Ops.  

---

## JSON Ops v1 (Bridge)

The engine exposes a simple JSON operation API, used by both the ImGui and Web UIs.

Example:
```json
[
  {"op":"load","path":"assets/models/cube.obj","name":"Hero","transform":{"position":[0,0,-4]}},
  {"op":"add_light","type":"point","position":[3,2,2],"intensity":1.2},
  {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45}
]
```

Exports to JS (Emscripten):
- `app_apply_ops_json(json)` — apply a single op or array  
- `app_share_link()` — export a shareable state link  
- `app_scene_to_json()` — get current scene snapshot  

Docs: `docs/json_ops_v1.md` (schema in `schemas/json_ops_v1.json`).  

---

## Highlights

- Native OBJ loader; optional unified loader (glTF/GLB/FBX/DAE/PLY) via Assimp.  
- PBR shader: BaseColor, Normal, Metallic/Roughness; standard raster shader with flat/Gouraud.  
- CPU raytracer with BVH; shows the result on a full-screen quad; optional denoise.  
- Texture cache deduplicates loads; per-object shader choice.  
- Shareable scene state (JSON Ops v1) + headless render CLI.  

---

## New Tools (MVPs)

### 1) Explain-my-render: “Why is it black?”
Detects and offers one-click fixes:
- Missing normals → Recompute angle-weighted normals  
- Bad winding (mostly backfacing) → Flip triangle order + invert normals  
- No lights / tone-mapped black → Add a neutral key light
- sRGB mismatch → Toggle framebuffer sRGB

<img width="1127" height="587" alt="image" src="https://github.com/user-attachments/assets/669093b7-9620-46cb-89d5-0b5803c967eb" />


### 2) Perf Coach HUD
Overlay shows: draw calls, total triangles, materials, textures, VRAM estimate.  
Suggestions:
- Meshes share a material → instancing candidate  
- High draw calls → merge static meshes or instance  
- High triangle count → consider LOD/decimation  
- Optional: **“Ask AI for perf tips”** when AI is enabled  

---

## Controls

- **Camera**: RMB + WASD/E/Q/Space/Ctrl (fly); RMB drag to orbit  
- **Gizmo**: Shift+Q/W/E = Translate/Rotate/Scale; X/Y/Z pick axis  
- **Shortcuts**: L toggles Local/World, N toggles snapping, F11 fullscreen, Delete removes selection  

---

## Menubar

- File → Load Cube/Plane, Copy Share Link, Settings  
- View → Point/Wire/Solid/Raytrace, Fullscreen, Perf HUD  
- Gizmo → Mode, Axis, Local/Snap  
- Samples → Prebuilt recipes (`assets/samples/recipes/`)  
- Toolbar → Mode, Gizmo, Add Light, Denoise, Why is it black?  

---

## Headless + Scripting (CLI)

Apply JSON Ops v1 and render to PNG without a window:
```bash
Project1.exe --ops recipe.json --render out.png --w 1024 --h 768
```

Options:
- `--ops <file>`: apply JSON Ops v1 from file  
- `--render <out.png>`: render to PNG (optionally with `--w`/`--h`)  
- `--denoise`: enable denoiser (if compiled with OIDN)  

---

## Troubleshooting

- **Black frame (raster)**: use “Why is it black?” panel, add lights, check normals, flip winding if needed.  
- **Dark/washed PBR**: avoid double-gamma (disable FB sRGB if needed).  
- **Headless asset paths**: run from repo root or use absolute paths.  

---

## Build Notes

- Toolchain: Visual Studio 2022, C++17, x64.  
- Third-party: GLFW, ImGui, stb, GLM under `Libraries/`.  
- Optional: Assimp via vcpkg (`USE_ASSIMP`) for non-OBJ formats.  
- Optional: KTX2 via `ENABLE_KTX2` if libktx is available.  

---

## Repo Layout

- `Project1/src` — rendering, UI, loaders, raytracer  
- `Project1/include` — headers  
- `Project1/shaders` — GLSL  
- `Project1/assets` — sample models, textures, recipes  
- `docs/json_ops_v1.md`, `schemas/json_ops_v1.json` — scripting docs/schema  

---

## Samples

Use the Samples menu or load recipes manually:
- `assets/samples/recipes/three-point-lighting.json`  
- `assets/samples/recipes/isometric-hero.json`  

---

## License

See repository license if present. Third-party components retain their respective licenses.
