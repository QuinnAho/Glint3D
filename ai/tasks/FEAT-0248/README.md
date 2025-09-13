# FEAT-0248 — RHI API Shape (BindGroups/Encoders/Queues)

## Intent
Adopt a WebGPU-shaped RHI frontend to guarantee desktop/web parity: introduce PipelineLayout, BindGroupLayout, BindGroup, CommandEncoder, RenderPassEncoder, Queue, Fence, and ResourceState.

## Acceptance
- All GL call sites route through CommandEncoder/RenderPassEncoder
- No direct FBO binds at call sites
- No glUniform* usage outside GL backend
- Queue/submit model implemented with basic validation
- Docs updated to WebGPU terminology

## Scope
- Headers: `engine/include/glint3d/rhi*.h`
- Impl: `engine/src/rhi/**`
- Tests: `tests/unit/rhi_test.cpp`
- Docs: `docs/rhi_architecture.md`

## Links
- Spec: `ai/tasks/FEAT-0248/spec.yaml`
- Passes: `ai/tasks/FEAT-0248/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0248/whitelist.txt`
