# OpenGL OBJ Viewer + Basic Raytracer

This project is a simple but flexible 3D viewer and CPU-based raytracer written in **C++**, **OpenGL**, and **GLM**.  
It allows loading `.obj` models, viewing them in real-time (wireframe, shaded, or point cloud), and raytracing them offline with basic lighting.



![image](https://github.com/user-attachments/assets/1ab937e4-cb85-4944-b458-3a8d4a2fdd3e)

---

## Features

- ✅ Load and render OBJ models
- ✅ Apply textures (e.g., cow texture)
- ✅ Wireframe, point cloud, or solid rendering modes
- ✅ Basic Phong shading in real-time
- ✅ Basic CPU raytracer:
  - Ray-AABB (Bounding Volume Hierarchy) acceleration
  - Ray-triangle intersection
  - Simple point lights
  - Textured raytraced objects
- ✅ Interactive camera movement
- ✅ GUI controls (via ImGui)

---

## Project Structure

| Folder/File | Purpose |
|:------------|:--------|
| `application.h/cpp` | Main Application loop (OpenGL + Windowing + GUI) |
| `raytracer.h/cpp` | CPU Raytracer (using BVH acceleration) |
| `BVHNode.h/cpp` | Bounding Volume Hierarchy for faster raytracing |
| `Triangle.h/cpp` | Triangle representation for intersection tests |
| `ObjLoader.h/cpp` | Simple OBJ model loader |
| `Shader.h/cpp` | GLSL Shader helper |
| `Light.h/cpp` | Manage multiple lights |
| `Texture.h/cpp` | Texture loading and sampling (for OpenGL + CPU) |
| `Grid.h/cpp` | Optional scene grid |
| `UserInput.h/cpp` | Camera control with mouse and keyboard |
| `shaders/` | OpenGL vertex/fragment shaders |
| `textures/` | Model textures (e.g., cow-tex-fin.jpg) |
| `models/` | OBJ models (e.g., cow.obj, cube.obj) |

---

## Controls

| Key | Action |
|:----|:-------|
| `W A S D` | Move camera forward/left/backward/right |
| `Q` / `E` | Move camera up / down |
| Left-Click + Drag | Rotate model |
| Right-Click + Drag | Rotate camera view |
| GUI Sliders | Change camera speed, FOV, clipping planes |
| Buttons | Switch between wireframe, solid, point cloud, and raytrace modes |

---

## Example Scene

- Two Cows on either side of the scene.
- Surrounded by walls (cubes) for context.
- One or more lights illuminating the scene.
- Texture applied both in real-time and raytraced mode.

---

## How the Raytracer Works

- Build a **Bounding Volume Hierarchy (BVH)** from the triangles.
- Cast primary rays from the camera into the scene.
- Find the nearest triangle hit using the BVH.
- Calculate intersection point and surface normal.
- Apply simple lighting:
  - Ambient + Diffuse (no specular yet).
  - Supports multiple light sources.
- If texture mapping is enabled:
  - Compute barycentric coordinates at hit point.
  - Sample the object's texture using interpolated UV coordinates.

The output is rendered to a full-screen quad as a texture.

---

## Dependencies

| Library | Usage |
|:--------|:------|
| **GLFW** | Windowing and input |
| **GLAD** | OpenGL function loading |
| **GLM** | Math library for vectors/matrices |
| **ImGui** | User Interface |
| **stb_image** | Texture loading |
| **stb_image_write** | (Optional) For saving screenshots |

---

## Building

### Prerequisites

- C++17 compiler
- CMake or Visual Studio
- OpenGL 3.3+

### How to Build

```bash
git clone <this-repo>
cd Project1/
mkdir build
cd build
cmake ..
make
./Project1
