#include <cassert>
#include <cstdint>
#include "../../engine/include/texture.h"
#include "../../engine/include/glint3d/rhi_types.h"

int main() {
    // By default, no RHI is registered and handle is INVALID_HANDLE
    Texture tex;
    assert(tex.rhiHandle() == glint3d::INVALID_HANDLE);

    // Setting and reading back the handle works without an RHI present
    tex.setRHIHandle(123u);
    assert(tex.rhiHandle() == 123u);
    // Reset to invalid for cleanup expectations
    tex.setRHIHandle(glint3d::INVALID_HANDLE);
    return 0;
}

