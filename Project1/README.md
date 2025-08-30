OpenGLOBJViewer — Project Setup and Structure

Overview
- A Visual Studio C++ OpenGL project to view OBJ models and a simple raytracing preview.
- Uses GLFW, GLAD, ImGui, GLM, and stb headers vendored under `Libraries/`.

Folder Layout
- src: C/C++ sources for the app (rendering, input, loaders).
- include: Public headers for the app.
- Libraries: Third‑party headers and prebuilt libs (GLFW, ImGui, stb, GLM).
- shaders: GLSL shaders used at runtime.
- assets: Models and textures (e.g., OBJ files and images).

Getting Started (Windows + Visual Studio 2022)
1) Requirements
   - Visual Studio 2022 (Desktop development with C++)
   - Windows 10+ SDK

2) Open the project
   - Open `Project1.vcxproj` in Visual Studio.

3) Configuration
   - Platform: `x64` (Debug or Release)
   - Language standard: C++17 (already set in the project)
   - Include dirs (already configured in the project):
     - `$(ProjectDir)include`
     - `$(ProjectDir)Libraries\include`
     - `$(ProjectDir)Libraries\include\imgui`
     - `$(ProjectDir)Libraries\include\imgui\backends`
     - `$(ProjectDir)Libraries\include\stb`
   - Library dirs (already configured in the project):
     - `$(ProjectDir)Libraries\lib`
   - Linker inputs (already configured): `glfw3.lib; opengl32.lib; gdi32.lib; user32.lib; shell32.lib; legacy_stdio_definitions.lib`

4) Build
   - Select `x64-Debug` or `x64-Release` and Build.
   - Output appears under `x64/`. Ensure runtime working directory is the project root so shader and asset relative paths resolve.

Runtime Notes
- Shaders are loaded from the `shaders/` directory.
- Sample assets live under `assets/` and textures like `cow-tex-fin.jpg` are in the project root.
- ImGui configuration persists in `imgui.ini` (created/updated at runtime).

Troubleshooting
- Missing headers/libraries: Confirm the `Libraries/` folder exists and the project include/library directories match paths listed above.
- Link errors for GLFW: Verify `Libraries/lib/glfw3.lib` exists and the platform is set to `x64`.
- Black screen: Check that GLAD initializes successfully and GPU supports required OpenGL version; also verify shaders compile (see console output).

Chat + Commands
- Open the app and use the right-side Talk panel to type JSON-like commands. Phase 1 supports only add operations:
  - load_model: { "op": "load_model", "path": "cow.obj", "name": "Cow1", "transform": { "position": [2,0,0], "scale": [1,1,1] } }
  - duplicate:  { "op": "duplicate", "source": "Cow1", "name": "Cow2", "transform": { "position": [2,0,0] } }
  - add_light:  { "op": "add_light", "type": "point", "position": [0,5,5], "color": [1,1,1], "intensity": 1.5 }
- Multiple commands can be submitted at once by wrapping them in an array: [ { ... }, { ... } ]
- Preview only toggle shows parsed JSON and planned actions without applying.
- Per‑batch confirmation modal appears after Submit; choose Apply or Cancel before execution.

Schema (Phase 1)
- load_model: path (string, required); name (string, optional); transform.position/scale/rotation (vec3, optional)
- duplicate: source (string, required); name (string, optional); transform.position/scale (vec3, optional)
- add_light: type ("point"|"directional", default point); position, direction, color (vec3, optional); intensity (float, default 1)

Notes
- Parser is lightweight and tolerant, but expects double-quoted keys/strings and vec3 arrays like [x,y,z].
- Undo/redo is not implemented yet; keep Preview only checked while experimenting.

Natural Language (Free, Local)
- This project can accept natural language and translate it to the JSON commands via a local LLM:
  - Provider: Ollama (free, local server). Install from https://ollama.com/ (Windows installer available).
  - Start the server (runs on http://127.0.0.1:11434 by default).
  - Pull a small instruct model, e.g.:
    - `ollama pull llama3.2:3b-instruct` (default used by the app)
    - Alternatives: `phi3:3.8b-mini-instruct`, `mistral:7b-instruct` (slower)
- In the app’s Talk panel, enable "Use AI", keep endpoint as `http://127.0.0.1:11434`, and set your model.
- Type natural language like: "load cow.obj at x=2 and duplicate it left by 1; add a light above"
  - The AI bridge will ask the model to emit strict JSON only and show the resulting JSON before applying.

Runtime Behavior Notes
- Rasterized modes (Solid/Wireframe/Point): new objects/lights appear immediately after Apply.
- Raytrace mode: the screen shows the last traced image. After Apply, re‑trigger a trace (press Raytrace again or nudge the camera) to include changes.
- Light indicators render in all modes (including raytrace) so add_light is visible as a small cube at the light’s position.

Troubleshooting (AI)
- If you see "AI error: WinHttp...": make sure Ollama is running and the endpoint/model are correct.
- If output contains prose or code fences: the app attempts to extract the JSON block; verify the previewed JSON looks correct before applying.

Troubleshooting (Scene Updates)
- Model did not appear: verify the path is valid relative to the working directory; in raytrace mode, re‑run the trace as noted above.
- Light not visible: a light at [0,0,0] can be occluded; try position [0,5,5]. Indicators should still render as small cubes.
