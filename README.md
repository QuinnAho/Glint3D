# AI-Enhanced OBJ Viewer + Raytracer

This project is a flexible 3D OBJ model viewer and CPU-based raytracer written in **C++**, **OpenGL**, and **GLM**, enhanced with **AI-powered natural language controls**.  
It supports real-time rasterized modes (wireframe, shaded, point cloud) and an offline CPU raytracer with BVH acceleration.  
Users can interact with the app using **direct JSON commands** or **plain natural language** (e.g., *"add a white light to the scene"*), which is automatically translated into JSON.

---

## Rendering Modes

**Phong**  
![image](https://github.com/user-attachments/assets/1ab937e4-cb85-4944-b458-3a8d4a2fdd3e)

**Flat**  
![image](https://github.com/user-attachments/assets/e6bac5ab-9dde-42f5-a7c4-b591f3b8970d)

**Raytraced (CPU, Not Real Time)**  
![image](https://github.com/user-attachments/assets/007f764a-b7c3-4a76-926d-adddc16be4f1)

**Wireframe**  
![image](https://github.com/user-attachments/assets/72e6216c-8b49-4d11-8ef6-18a665b1514c)

**Point Cloud**  
![image](https://github.com/user-attachments/assets/0765d651-f80c-47e1-8f95-9e95d194c152)

---

## Features

- Load and render OBJ models (native)
- Optional Assimp import: glTF/GLB, FBX, DAE, PLY (and more)
- Apply textures (e.g., cow texture)
- Wireframe, point cloud, or solid rendering modes
- Real-time Phong shading
- CPU raytracer:
  - BVH acceleration (Ray–AABB + Ray–Triangle)
  - Point light support
  - Textured raytraced surfaces
- Interactive camera movement
- GUI controls (ImGui)
- **Chat-based JSON commands** to load/duplicate models and add lights
- **AI natural language bridge**: type instructions like  
  - *“add a white light to the scene”*  
  - *“load cow.obj and place it at x=2”*  
  - *“duplicate the cow to the left and make it smaller”*  
  → The app automatically generates and applies the correct JSON.

---

## Project Setup and Structure

### Overview
A Visual Studio C++ OpenGL project for viewing OBJ models with a simple raytracing preview and AI-driven scene manipulation.  
Uses GLFW, GLAD, ImGui, GLM, and stb headers vendored under `Libraries/`.

### Folder Layout
- **src/** — C/C++ sources for rendering, input, loaders  
- **include/** — Public headers  
- **Libraries/** — Third-party headers and prebuilt libs (GLFW, ImGui, stb, GLM)  
- **shaders/** — GLSL shaders  
- **assets/** — Models + textures (e.g., cow.obj, cow-tex-fin.jpg)  

### Requirements
- Visual Studio 2022 (Desktop development with C++)  
- Windows 10+ SDK  

### Opening
- Open `Project1.vcxproj` in Visual Studio  

### Configuration
- Platform: **x64** (Debug or Release)  
- Language standard: **C++17**  
- Includes (pre-configured):  
  - `$(ProjectDir)include`  
  - `$(ProjectDir)Libraries\include`  
  - `$(ProjectDir)Libraries\include\imgui`  
  - `$(ProjectDir)Libraries\include\stb`  
- Libraries: `$(ProjectDir)Libraries\lib`  
- Linker: `glfw3.lib; opengl32.lib; gdi32.lib; user32.lib; shell32.lib; legacy_stdio_definitions.lib`  

### Build
- Select **x64-Debug** or **x64-Release** → Build  
- Output under `x64/`  
- Working directory must be project root so shaders/assets resolve  

### Runtime Notes
- Shaders load from `shaders/`  
- Assets load from `assets/`  
- ImGui config persists in `imgui.ini`  

---

## Assimp (Optional; for glTF/FBX/DAE/PLY)

This project can use Assimp to import additional mesh formats. The code builds without Assimp; if not enabled, non-OBJ imports fall back and log an error.

Steps (recommended with vcpkg):

1) Install vcpkg and integrate
- `git clone https://github.com/microsoft/vcpkg`
- `vcpkg\\bootstrap-vcpkg.bat`
- `vcpkg integrate install`

2) Install Assimp
- `vcpkg install assimp:x64-windows`

3) Add to Project Properties (x64 Debug/Release):
- C/C++ > Preprocessor > Preprocessor Definitions: add `USE_ASSIMP`
- C/C++ > Additional Include Directories: add `$(VcpkgRoot)installed\\x64-windows\\include`
- Linker > Additional Library Directories: add `$(VcpkgRoot)installed\\x64-windows\\lib`
- Linker > Additional Dependencies: add `assimp-vc143-mt.lib` (debug may be `assimp-vc143-mtd.lib`)

4) Runtime DLL (if using dynamic libs)
- Copy `assimp-vc143-mt.dll` from vcpkg `installed\\x64-windows\\bin` to your output folder (e.g., `x64/Debug`)

Notes:
- Importer lives in `Project1/src/assimp_loader.cpp` guarded by `#ifdef USE_ASSIMP`.
- `Application::addObject` chooses loader by extension; non-OBJ uses Assimp.
- Command loader tries extensions: `.obj, .gltf, .glb, .fbx, .dae, .ply` under prefixes `['', 'objs/', 'assets/models/']`.

Sample assets:
- `Project1/assets/models/triangle.ply` — minimal triangle to validate Assimp path (PLY).
  Use the GUI button “Load sample triangle (PLY)” under Render Settings → Samples.

---

## Controls

- Hold RMB: camera navigation and orbit
  - W/A/S/D: move forward/left/back/right
  - Space or E: move up; Left Ctrl or Q: move down
  - Right-drag: orbit camera
- Gizmo triad
  - Shift+Q/W/E: Translate / Rotate / Scale
  - X/Y/Z: constrain to axis
  - L: toggle Local vs World gizmo orientation
  - N: toggle snapping (translate/rotate/scale steps adjustable in UI)
- Delete: delete currently selected object or light
- Mouse
  - Left-click: select object or gizmo axis
  - Left-drag on axis: transform along that axis
  - GUI: adjust speed, FOV, clipping planes, shading, and gizmo snap steps

---

## Example Scene

- Two cows in the scene  
- Surrounded by cube “walls”  
- One or more lights  
- Texture applied in both raster + raytraced modes  

---

## How the Raytracer Works

1. Build a **BVH** from model triangles  
2. Cast rays from camera → BVH traversal  
3. Find nearest intersection  
4. Compute hit point + normal  
5. Apply lighting: Ambient + Diffuse (no specular yet)  
6. If textured: interpolate UV via barycentrics → sample texture  

Output is rendered to a fullscreen quad as a texture.  

---

## Chat + Commands

Open the app and use the right-side **Talk panel**.  

**Example:**

<img width="800" height="631" alt="image" src="https://github.com/user-attachments/assets/dc534961-42a9-4ec3-9295-b6d0dfa6692a" />

<img width="803" height="601" alt="image" src="https://github.com/user-attachments/assets/384430a8-36b2-45d8-ad9a-997282afff40" />

<img width="792" height="633" alt="image" src="https://github.com/user-attachments/assets/cd61d91c-10ef-4edb-8fda-18e92bb973be" />


### Example JSON Commands
```json
{ "op": "load_model", "path": "cow.obj", "name": "Cow1", "transform": { "position": [2,0,0], "scale": [1,1,1] } }
{ "op": "duplicate", "source": "Cow1", "name": "Cow2", "transform": { "position": [2,0,0] } }
{ "op": "add_light", "type": "point", "position": [0,5,5], "color": [1,1,1], "intensity": 1.5 }
\n## Natural Language Examples
- place cow in front of me 3
- add light at 0 5 0 intensity 0.8
- create material wood color 0.6 0.4 0.2; assign material wood to Cow1
- place 4 cube objects and arrange them so there are three walls and one floor
- place 6 cube objects long and flat
\nNotes:
- Multi-placement supports adjectives: long, flat, tall, wide; or explicit `scale sx sy sz`.
- The “three walls and one floor” pattern creates a U-shaped room in front of the camera using `cube.obj`.
