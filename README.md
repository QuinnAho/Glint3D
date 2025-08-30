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

- Load and render OBJ models
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

## Controls

| Key | Action |
|:----|:-------|
| `W A S D` | Move camera forward/left/backward/right |
| `Q` / `E` | Move camera up / down |
| Left-Click + Drag | Rotate model |
| Right-Click + Drag | Rotate camera view |
| GUI Sliders | Adjust speed, FOV, clipping planes |
| Buttons | Switch between wireframe, solid, point cloud, raytrace |

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

### Example JSON Commands
```json
{ "op": "load_model", "path": "cow.obj", "name": "Cow1", "transform": { "position": [2,0,0], "scale": [1,1,1] } }
{ "op": "duplicate", "source": "Cow1", "name": "Cow2", "transform": { "position": [2,0,0] } }
{ "op": "add_light", "type": "point", "position": [0,5,5], "color": [1,1,1], "intensity": 1.5 }
