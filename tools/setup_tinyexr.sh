#!/bin/bash
# Setup TinyEXR and miniz for cross-platform builds

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
INCLUDE_DIR="$PROJECT_ROOT/engine/libraries/include"
SRC_DIR="$PROJECT_ROOT/engine/libraries/src"

echo "Setting up TinyEXR and miniz..."

# Create directories
mkdir -p "$INCLUDE_DIR" "$SRC_DIR"

# Download TinyEXR header if not present
TINYEXR_H="$INCLUDE_DIR/tinyexr.h"
if [ ! -f "$TINYEXR_H" ]; then
    echo "Downloading tinyexr.h..."
    curl -L -o "$TINYEXR_H" \
        "https://raw.githubusercontent.com/syoyo/tinyexr/master/tinyexr.h"
    echo "Downloaded tinyexr.h"
else
    echo "tinyexr.h already exists"
fi

# Download miniz header if not present
MINIZ_H="$INCLUDE_DIR/miniz.h"
if [ ! -f "$MINIZ_H" ]; then
    echo "Downloading miniz.h..."
    curl -L -o "$MINIZ_H" \
        "https://raw.githubusercontent.com/richgel999/miniz/master/miniz.h"
    echo "Downloaded miniz.h"
else
    echo "miniz.h already exists"
fi

# Download miniz source if not present
MINIZ_C="$SRC_DIR/miniz.c"
if [ ! -f "$MINIZ_C" ]; then
    echo "Downloading miniz.c..."
    curl -L -o "$MINIZ_C" \
        "https://raw.githubusercontent.com/richgel999/miniz/master/miniz.c"
    echo "Downloaded miniz.c"
else
    echo "miniz.c already exists"
fi

# Download additional miniz files for modular build
MINIZ_FILES=("miniz_tdef.c" "miniz_tinfl.c" "miniz_zip.c")
for file in "${MINIZ_FILES[@]}"; do
    MINIZ_FILE="$SRC_DIR/$file"
    if [ ! -f "$MINIZ_FILE" ]; then
        echo "Downloading $file..."
        curl -L -o "$MINIZ_FILE" \
            "https://raw.githubusercontent.com/richgel999/miniz/master/$file"
        echo "Downloaded $file"
    else
        echo "$file already exists"
    fi
done

echo "TinyEXR and miniz setup complete!"
echo "Files installed:"
echo "  Headers: $INCLUDE_DIR/{tinyexr.h,miniz.h}"
echo "  Sources: $SRC_DIR/{miniz.c,miniz_tdef.c,miniz_tinfl.c,miniz_zip.c}"
echo ""
echo "You can now build with EXR support:"
echo "  cmake -S . -B builds/desktop/cmake -DENABLE_EXR=ON"
echo "  cmake --build builds/desktop/cmake --config Release"