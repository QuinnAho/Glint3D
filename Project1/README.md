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

