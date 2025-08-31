# OpenGLOBJViewer

Problem → Simple viewers rarely scale from quick lookdev to shareable outputs, and they don’t run identically on desktop and web.

Solution → A compact OpenGL viewer + CPU raytracer with JSON scripting and a web build. Drop in a model, tweak lighting/materials, and share or render — same ops on both platforms.

60-sec tour → [GIF: Lookdev by prompt] (add docs/media/lookdev.gif) • [GIF: Turntable render] (add docs/media/turntable.gif)

Try the web demo → build-web/objviewer.html (serve the folder) or publish to GitHub Pages and link it here.

Getting started
- Desktop (Windows): open `Project1/Project1.vcxproj`, build x64, run from repo root so `shaders/` and `assets/` resolve.
- Web: see `WEB_BUILD.md` (Emscripten). Outputs `objviewer.html` in `build-web/` with preloaded `assets/` and `shaders/`.
- Examples: load JSON from `examples/` (below) or from `Project1/assets/samples/recipes/`.

JSON Ops quickstart
- Format: a list of ops. Docs: `docs/json_ops_v1.md` (schema: `schemas/json_ops_v1.json`).
- Example:
  [
    {"op":"load","path":"assets/models/cube.obj","name":"Hero","transform":{"position":[0,0,-4]}},
    {"op":"add_light","type":"point","position":[3,2,2],"intensity":1.2},
    {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45}
  ]
- Headless render:
  `Project1.exe --ops Project1/assets/samples/recipes/three-point-lighting.json --render renders/out.png --w 1280 --h 720`

Examples
- Project1/assets/samples/recipes/three-point-lighting.json: key/fill/rim lights and camera.
- Project1/assets/samples/recipes/turntable-cli.json: base scene for scripted turntable.
  - Windows (PowerShell):
    `for ($i=0; $i -lt 120; $i++) { $ang = [int](360*$i/120); $ops = @(
      @{ op='load'; path='assets/models/cube.obj'; name='Hero'; transform=@{ position=@(0,0,0) }}
      @{ op='add_light'; type='point'; position=@(4,6,4); intensity=1.0 }
      @{ op='set_camera'; position=@(0,1,6); target=@(0,1,0); fov_deg=45 }
      @{ op='transform'; target='Hero'; mode='set'; transform=@{ rotation_deg=@(0,$ang,0) }}
      @{ op='render'; target='image'; path=("renders/frame_{0:D4}.png" -f $i); width=1280; height=720 }
    ) | ConvertTo-Json -Depth 5; Set-Content -Path out.turn.json -Value $ops; .\x64\Release\Project1.exe --ops out.turn.json }`
  - Bash:
    `for i in $(seq 0 119); do ang=$(( 360*i/120 )); cat > out.turn.json <<JSON
[
 {"op":"load","path":"assets/models/cube.obj","name":"Hero","transform":{"position":[0,0,0]}},
 {"op":"add_light","type":"point","position":[4,6,4],"intensity":1.0},
 {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45},
 {"op":"transform","target":"Hero","mode":"set","transform":{"rotation_deg":[0,$ang,0]}},
 {"op":"render","target":"image","path":"renders/frame_$(printf %04d $i).png","width":1280,"height":720}
]
JSON
 ./Project1.exe --ops out.turn.json; done`

Key features
- Importers: plugin interface with OBJ builtin and optional Assimp for glTF/FBX/DAE/PLY.
- Materials: simple PBR struct; raster PBR shader; raytracer approximates from PBR for parity.
- Diagnostics: "Why is it black?" quick fixes; Perf HUD with VRAM estimate.
- Headless CLI: apply JSON and render to PNG.
- Web: WebGL2 build; optional KTX2 textures with fallback to PNG/JPG.

Troubleshooting
- Black frame: add a light, or run the "Why is it black?" panel.
- PBR looking off: avoid double-gamma; disable FB sRGB if needed.
- Paths: run from repo root; use `assets/...` and `shaders/...`.

Build notes
- Toolchain: Visual Studio 2022, C++17, x64.
- Third-party: GLFW, ImGui, stb, GLM under `Libraries/`.
- Optional: Assimp via vcpkg (`USE_ASSIMP`) for non-OBJ formats.
- Optional: KTX2 via `ENABLE_KTX2` if libktx is available.

Release
- See `RELEASE.md` for tagging v0.3.0, bundling desktop/web artifacts, and GitHub Pages.
