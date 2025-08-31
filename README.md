# OpenGL OBJ Viewer + Raytracer (with AI and Tools)

This is a C++ OpenGL viewer and CPU raytracer with modern quality‑of‑life tools:
- Natural‑language commands (via AI) and JSON Ops v1 scripting
- PBR (Cook–Torrance) and a standard raster path
- “Why is it black?” diagnostics with one‑click fixes
- Perf HUD coach with counters + actionable hints

Runs as a desktop app (GLFW/GLAD/ImGui). A headless path supports CLI rendering and scripted scene edits.

---

## Quick Start

- Open `Project1/Project1.vcxproj` in Visual Studio 2022 (x64).
- Build + run from the repo root so `shaders/` and `assets/` resolve.
- Use the top menu to load sample models or recipes. Hold RMB to fly the camera.

---

## Highlights

- Native OBJ loader; optional unified loader (glTF/GLB/FBX/DAE/PLY) via Assimp.
- PBR shader: BaseColor, Normal, Metallic/Roughness; Standard shader with flat/Gouraud.
- Texture cache deduplicates loads; per‑object shader choice (Standard vs PBR).
- CPU raytracer with BVH; shows the result on a full‑screen quad; optional denoise.
- Shareable scene state (JSON Ops v1) + headless render CLI.

---

## New Tools (MVPs)

1) Explain‑my‑render: “Why is it black?”
- Detects and offers one‑click fixes:
  - Missing normals → Recompute angle‑weighted normals
  - Bad winding (mostly backfacing) → Flip triangle order + invert normals
  - No lights / tone‑mapped black → Add a neutral key light
  - sRGB mismatch (PBR gamma + sRGB FB) → Toggle framebuffer sRGB
- Open via the menubar button “Why is it black?”.

2) Perf Coach HUD
- Overlay shows: draw calls, total triangles, materials, textures, texture MB, geometry MB, VRAM estimate.
- Suggestions:
  - “X meshes share material Y → instancing candidate.”
  - High draw calls → merge static meshes or instance
  - High triangle count → consider LOD/decimation
- Toggle under View → “Perf HUD”.
- Optional “Ask AI for perf tips” button when AI is enabled.

---

## Controls

- RMB + WASD/E/Q/Space/Ctrl: fly camera; RMB drag to orbit
- Gizmo: Shift+Q/W/E → Translate/Rotate/Scale; X/Y/Z pick axis; L toggles Local/World; N toggles snapping
- F11: fullscreen
- Delete: delete selection

---

## Menubar

- File → Load Cube / Load Plane, Copy Share Link, Toggle Settings Panel
- View → Point/Wire/Solid/Raytrace, Fullscreen, Perf HUD
- Gizmo → Mode, Axis, Local/Snap
- Samples → Prebuilt recipes (see `assets/samples/recipes/`)
- Toolbar quick buttons: Mode, Gizmo, Add Light, Denoise, Why is it black?

---

## “Talk & AI” Panel

- Send natural language or JSON Ops v1 lines.
- Enable “Use AI” and configure endpoint/model to let the planner generate a plan from your request.
- Scene snapshots can be copied as JSON.

Docs: `docs/json_ops_v1.md` (schema in `schemas/json_ops_v1.json`).

---

## Headless + Scripting (CLI)

Apply JSON Ops v1 and render to PNG without a window:

- `--ops <file>`: apply JSON Ops v1 from file
- `--render <out.png>`: render to PNG (optionally with `--w`/`--h`)
- `--w <int> --h <int>`: output resolution
- `--denoise`: enable denoiser (if compiled with OIDN)

Example:
```
Project1.exe --ops recipe.json --render out.png --w 1024 --h 768
```

---

## Troubleshooting

- Black frame (raster):
  - Use “Why is it black?” panel.
  - Add at least one light; check intensities are > 0.
  - For PBR, if colors look crushed, disable framebuffer sRGB in the panel.
  - Recompute angle‑weighted normals if the model lacks normals.
  - Flip winding if the object is mostly backfacing.

- Dark/washed PBR:
  - Avoid double‑gamma: PBR shader outputs gamma‑corrected color; disable FB sRGB.

- Headless asset paths:
  - Run from repo root or provide absolute paths so `assets/` resolves.

---

## Build Notes

- Toolchain: Visual Studio 2022, C++17, x64.
- Third‑party: GLFW, ImGui, stb, GLM under `Libraries/`.
- Optional: Assimp via vcpkg (`USE_ASSIMP`) for non‑OBJ formats.
  - Include/Lib dirs and DLL steps are typical for vcpkg.

---

## Repo Layout

- `Project1/src` – rendering, UI, loaders, raytracer
- `Project1/include` – headers
- `Project1/shaders` – GLSL
- `Project1/assets` – sample models, textures, recipes
- `docs/json_ops_v1.md`, `schemas/json_ops_v1.json` – scripting docs/schema

---

## Samples

Use the Samples menu or load recipes manually:
- `assets/samples/recipes/three-point-lighting.json`
- `assets/samples/recipes/isometric-hero.json`

---

## License

See repository license if present. Third‑party components retain their respective licenses.

