# Glint3D Setup Guide

## Quick Start

### Windows
```cmd
./setup.bat
```

### Linux/macOS  
```bash
./setup.sh
```

That's it! The setup script will:
- Install/detect vcpkg automatically
- Configure dependencies via vcpkg manifest
- Generate build files with CMake

## What You Get

### ✅ **Modern Dependency Management**
- **vcpkg manifest mode**: Dependencies defined in `vcpkg.json`
- **Automatic resolution**: CMake installs dependencies during configure
- **Clean repository**: No large binary files in version control
- **Reproducible builds**: Locked dependency versions

### ✅ **Supported 3D Formats**
- **OBJ** (.obj) - Built-in support
- **glTF/GLB** (.gltf, .glb) - Industry standard
- **FBX** (.fbx) - Autodesk format
- **COLLADA** (.dae) - Open standard
- **PLY** (.ply) - Point cloud format
- **3DS, Blender, X3D** - And many more via Assimp

### ✅ **Graphics Features**
- **PBR Rendering**: Physically-based materials
- **CPU Raytracer**: High-quality offline rendering
- **Real-time Rasterizer**: Interactive viewport
- **IBL Support**: Image-based lighting
- **Post-processing**: Denoising, tone mapping

## Dependencies (Automatic)

These are installed automatically via `vcpkg.json`:

| Package | Purpose | Status |
|---------|---------|---------|
| **assimp** | 3D model loading | ✅ Required |
| **glfw3** | Window/input management | ✅ Required |  
| **glm** | Mathematics library | ✅ Required |
| **openimagedenoise** | AI denoising | 📦 Optional |

## Manual Setup (Alternative)

If you prefer manual setup or have an existing vcpkg installation:

### 1. Install vcpkg globally
```bash
# Clone vcpkg anywhere
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Windows
.\bootstrap-vcpkg.bat

# Linux/macOS
./bootstrap-vcpkg.sh

# Add to PATH or set CMAKE_TOOLCHAIN_FILE
```

### 2. Configure project
```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
```

### 3. Build
```bash
cmake --build build --config Release
```

## Project Architecture

```
glint3d/
├── vcpkg.json                   # 📋 Dependency manifest (version controlled)
├── vcpkg-configuration.json     # ⚙️  vcpkg settings
├── setup.bat/.sh               # 🚀 One-command setup
├── CMakeLists.txt               # 🏗️  Build configuration
├── engine/
│   ├── src/                     # C++ source code  
│   ├── include/                 # Header files
│   ├── shaders/                 # GLSL shaders
│   ├── assets/                  # Sample models/textures
│   └── libraries/               # Essential libraries only
├── web/                         # React frontend (optional)
├── build/                       # Build outputs (gitignored)
└── vcpkg/                       # Local vcpkg (gitignored)
```

## Repository Size

**Before optimization**: >2GB (vcpkg artifacts included)  
**After optimization**: ~50MB (clean, manifest-based)

### What's Excluded from Git
- ❌ `vcpkg/` - Local installations  
- ❌ `build/` - Build artifacts
- ❌ `*.lib, *.dll` - Binary libraries
- ❌ `packages/`, `buildtrees/` - vcpkg cache
- ✅ `vcpkg.json` - Dependency manifest
- ✅ Source code and essential assets

## Best Practices Applied

### 🏗️ **Build System**
- **CMake 3.15+**: Modern CMake with vcpkg integration
- **Manifest mode**: Dependencies as code, not artifacts  
- **Multi-platform**: Windows, Linux, macOS, Web
- **IDE support**: Visual Studio, VS Code, CLion

### 📦 **Dependency Management**
- **Semantic versioning**: Locked baseline for reproducibility
- **Feature flags**: Optional components (web, tools)
- **Platform conditionals**: Different deps per platform
- **No vendor lock-in**: Standard vcpkg, not proprietary

### 🔄 **Development Workflow**
1. **Clone repository** (fast, small size)
2. **Run setup script** (one command)
3. **Start developing** (everything ready)
4. **CI/CD ready** (reproducible builds)

## Troubleshooting

### "CMake could not find vcpkg"
- Run `./setup.bat` to install vcpkg locally
- Or set `CMAKE_TOOLCHAIN_FILE` manually

### "Package installation failed"
- Check internet connection (vcpkg downloads packages)
- Verify compiler is installed (Visual Studio 2019+ on Windows)
- Try `vcpkg integrate install` for global integration

### "Build errors on Linux"
```bash
# Install build tools
sudo apt update && sudo apt install build-essential cmake ninja-build

# Then re-run setup
./setup.sh
```

### Repository too large after clone
If you cloned an older version:
```bash
# Clean vcpkg artifacts
git rm -r --cached engine/libraries/include/vcpkg/buildtrees/
git rm -r --cached engine/libraries/include/vcpkg/packages/
git commit -m "Remove vcpkg artifacts"
```

## Performance

- **Setup time**: 5-10 minutes (first time)
- **Rebuild time**: 1-2 minutes (incremental)  
- **Repository size**: ~50MB (clean)
- **Memory usage**: <4GB during build

## Support

For issues with:
- **Setup/Build**: Check this guide first
- **vcpkg**: [vcpkg documentation](https://vcpkg.io/)
- **CMake**: [CMake documentation](https://cmake.org/documentation/)
- **Project bugs**: [GitHub Issues](https://github.com/your-repo/glint3d/issues)