#include <glint3d/rhi.h>
#include "rhi/rhi_gl.h"
#include "rhi/rhi_null.h"
#include <memory>

namespace glint3d {

std::unique_ptr<RHI> createRHI(RHI::Backend backend) {
    switch (backend) {
        case RHI::Backend::OpenGL:
        case RHI::Backend::WebGL2:
            return std::make_unique<RhiGL>();
        case RHI::Backend::Null:
            return std::make_unique<RhiNull>();
        
        case RHI::Backend::Vulkan:
        case RHI::Backend::WebGPU:
            // Not implemented yet
            return nullptr;
        
        default:
            return nullptr;
    }
}

} // namespace glint3d