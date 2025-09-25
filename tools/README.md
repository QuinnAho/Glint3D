# Glint3D Tools Directory

Essential build scripts for the Glint3D project.

## Build Scripts

### `build-and-run.sh` / `build-and-run.bat`
Primary build script for desktop applications.
- Cross-platform CMake compilation
- Debug and Release build modes
- Usage: `./build-and-run.sh [debug|release] [args...]`

### `build-web.sh` / `build-web.bat`
Complete web build pipeline (WASM engine + React frontend).
- Emscripten WASM compilation
- Asset copying and package building
- Usage: `./tools/build-web.sh [Debug|Release]` or `npm run web:build`