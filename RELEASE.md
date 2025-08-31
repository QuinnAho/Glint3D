# Release Guide (v0.3.0)

This guide walks through tagging v0.3.0, building artifacts, and publishing the web demo.

Version v0.3.0 highlights
- Formats: Importer interface with OBJ + Assimp plugin
- Web: Emscripten build with asset preloading; optional KTX2 textures

1) Tag the release
- Ensure main is clean and passes a local build
- Tag and push:
  - git tag v0.3.0
  - git push origin v0.3.0

2) Build Windows desktop zip
- Visual Studio 2022 (x64)
  - Config: Release|x64
  - Build target: Project1
  - Ensure DLLs are copied to OutDir (vcxproj has post-build copy for Assimp and deps)
- Package
  - Create `OpenGLOBJViewer-v0.3.0-win64.zip` containing:
    - `Project1.exe`
    - `Project1/shaders/` folder
    - `Project1/assets/` folder (or a subset if large)
    - Any required DLLs from `x64/Release/`

3) Build web bundle
- Emscripten SDK active
  - mkdir build-web && cd build-web
  - emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
  - cmake --build . -j
- Package
  - Create `OpenGLOBJViewer-v0.3.0-web.zip` containing:
    - `objviewer.html` (+ `.wasm`, `.data`)
    - `Project1/assets/` and `Project1/shaders/` (or ensure preloaded via CMake flags)

4) GitHub Release
- Create a GitHub Release for tag `v0.3.0`
- Attach `OpenGLOBJViewer-v0.3.0-win64.zip` and `OpenGLOBJViewer-v0.3.0-web.zip`
- Notes
  - Include brief changelog and links to README sections

5) GitHub Pages (optional)
- Serve `build-web/` as a static site
  - Option A (gh-pages branch):
    - Copy build-web/ output into a `gh-pages` branch root and push
    - Set Pages to serve from `gh-pages` branch
  - Option B (docs folder):
    - Move/copy web output to `docs/` and set Pages to `main/docs`
- Update README “Try Web Demo” link to the Pages URL (https://<org>.github.io/<repo>/objviewer.html)

6) Verify
- Desktop: run the EXE from repo root, load `examples/` JSON and `assets/` models
- Web: open the Pages URL; try loading `assets/models/cube.obj` and run examples via the embed/debug UI

Changelog template
- Added: Importer registry and plugins (OBJ, Assimp)
- Added: PBR material struct and unified raster/ray usage
- Added: Optional KTX2/Basis textures with offline converters
- Docs: Product-style README, examples, release guide

