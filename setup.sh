#!/bin/bash
set -e

echo "================================================================"
echo "                    Glint3D Development Setup"
echo "================================================================"
echo

# Check if vcpkg is already available globally
if command -v vcpkg &> /dev/null; then
    echo "✅ vcpkg found globally - using system installation"
    VCPKG_ROOT=$(dirname $(which vcpkg))
elif [ -f "vcpkg/vcpkg" ]; then
    echo "✅ Using local vcpkg installation"
    VCPKG_ROOT="$(pwd)/vcpkg"
elif [ -f "engine/libraries/include/vcpkg/vcpkg" ]; then
    echo "✅ Using project vcpkg installation"  
    VCPKG_ROOT="$(pwd)/engine/libraries/include/vcpkg"
else
    # Install vcpkg locally if not found
    echo "📦 vcpkg not found - installing locally..."
    echo
    
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    cd ..
    VCPKG_ROOT="$(pwd)/vcpkg"
fi

echo
echo "🔧 Installing dependencies via vcpkg manifest..."
echo "Dependencies defined in vcpkg.json will be installed automatically during CMake configure"
echo

# Set up cmake toolchain file
CMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

echo "🏗️  Configuring CMake project..."
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" -DCMAKE_BUILD_TYPE=Release

echo
echo "✅ Setup complete!"
echo
echo "To build the project:"
echo "  cmake --build build"
echo
echo "To run the application:"
echo "  ./build/glint"
echo