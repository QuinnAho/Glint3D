#include "rhi/rhi.h"
#include "rhi/rhi-gl.h"
#include <memory>

std::unique_ptr<RHI> createRHI(RHI::Backend backend) {
    switch (backend) {
        case RHI::Backend::OpenGL:
        case RHI::Backend::WebGL2:
            return std::make_unique<RhiGL>();
        
        case RHI::Backend::Vulkan:
        case RHI::Backend::WebGPU:
            // Not implemented yet
            return nullptr;
        
        default:
            return nullptr;
    }
}