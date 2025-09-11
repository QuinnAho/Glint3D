# Dependencies Guide

This document outlines how to install dependencies for Glint3D on different platforms.

## Required Dependencies

### Core Dependencies (All Platforms)
- **CMake 3.15+** - Build system
- **C++17 compiler** - MSVC 2019+, GCC 8+, or Clang 7+
- **OpenGL 3.3+** - Graphics API

### Platform-Specific Dependencies

## Windows

### Using vcpkg (Recommended)
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install glfw3:x64-windows
.\vcpkg install assimp:x64-windows
.\vcpkg install openimagedenoise:x64-windows  # Optional but recommended

# Use with CMake
cmake -S . -B builds/desktop/cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
```

### Manual Installation
- **GLFW**: Download from https://www.glfw.org/download.html
- **Assimp**: Download from https://github.com/assimp/assimp/releases
- **OIDN**: Already included in `engine/libraries/` (Windows binaries)

## Linux (Ubuntu/Debian)

### System Package Manager
```bash
# Essential dependencies
sudo apt update
sudo apt install build-essential cmake git

# Graphics and windowing
sudo apt install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev

# Asset loading (optional but recommended)
sudo apt install libassimp-dev

# AI denoising (optional)
sudo apt install liboidn-dev

# EXR support dependencies
sudo apt install libtinyexr-dev || echo "TinyEXR not available via apt, will use vendored"
```

### Manual Installation (if packages unavailable)
```bash
# OIDN from Intel releases
wget https://github.com/OpenImageDenoise/oidn/releases/download/v2.3.3/oidn-2.3.3.x86_64.linux.tar.gz
tar -xzf oidn-2.3.3.x86_64.linux.tar.gz
sudo cp -r oidn-2.3.3.x86_64.linux/include/* /usr/local/include/
sudo cp -r oidn-2.3.3.x86_64.linux/lib/* /usr/local/lib/
sudo ldconfig
```

## macOS

### Using Homebrew
```bash
# Install Homebrew if needed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake glfw assimp
brew install openimagedenoise  # Optional
```

## Optional Dependencies

### TinyEXR + miniz (HDR/EXR support)
If not available via system packages, these are automatically downloaded:
- Headers placed in `engine/libraries/include/`
- Source files in `engine/libraries/src/`

### KTX2/Basis Support
```bash
# vcpkg (Windows)
vcpkg install ktx:x64-windows

# Linux
sudo apt install libktx-dev  # If available

# macOS
brew install ktx-lib  # If available
```

## Build Instructions

### Quick Start
```bash
# Configure
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build builds/desktop/cmake --config Release -j

# Run
./builds/desktop/cmake/Release/glint --help
```

### With Optional Features
```bash
# Enable all optional features
cmake -S . -B builds/desktop/cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_KTX2=ON \
    -DENABLE_EXR=ON

# Build with parallel jobs
cmake --build builds/desktop/cmake --config Release -j$(nproc)
```

## Troubleshooting

### GLFW Not Found
```bash
# Linux: Install dev packages
sudo apt install libglfw3-dev

# Windows: Use vcpkg or add to CMAKE_PREFIX_PATH
```

### OIDN Not Found
```bash
# Linux: Check if installed
pkg-config --exists OpenImageDenoise
ldconfig -p | grep oidn

# If missing, install manually or disable denoising
```

### Assimp Not Found
```bash
# Linux: Install development headers
sudo apt install libassimp-dev

# Verify installation
pkg-config --exists assimp
```

### Build Fails on Linux
```bash
# Ensure all dev packages installed
sudo apt install build-essential cmake git \
    libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev \
    libassimp-dev liboidn-dev

# Clean and rebuild
rm -rf builds/desktop/cmake
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake --config Release
```

## Dependency Matrix

| Feature | Windows | Linux | macOS | Required |
|---------|---------|-------|-------|----------|
| GLFW | vcpkg/manual | apt | brew | ✅ Yes |
| OpenGL | System | mesa-dev | System | ✅ Yes |
| Assimp | vcpkg/manual | apt | brew | 🔶 Optional |
| OIDN | Vendored | apt/manual | brew | 🔶 Optional |
| TinyEXR | Vendored | vendored | vendored | 🔶 Optional |
| KTX2 | vcpkg | apt | brew | 🔶 Optional |

## Performance Notes

- **OIDN**: Enables AI denoising for raytraced renders (~10x noise reduction)
- **Assimp**: Enables glTF, FBX, DAE, and other advanced model formats
- **TinyEXR**: Enables HDR environment maps and high dynamic range textures
- **KTX2**: Enables GPU-compressed textures for better performance

All optional dependencies can be disabled for basic functionality.