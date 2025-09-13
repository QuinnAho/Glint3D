# FEAT-0251 — Compute Pipeline + Storage Buffers

## Intent
Add compute pipeline support and storage buffers aligned to WebGPU; implement GL backend path and unit tests.

## Acceptance
- ComputePipeline and dispatch supported in RHI
- Storage buffers (read/write) in bind groups
- Unit tests cover compute + buffer readback

## Scope
- Headers: `engine/include/glint3d/rhi*.h`
- Impl: `engine/src/rhi/**`
- Tests: `tests/unit/rhi_test.cpp`
- Docs: `docs/rhi_architecture.md`

## Links
- Spec: `ai/tasks/FEAT-0251/spec.yaml`
- Passes: `ai/tasks/FEAT-0251/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0251/whitelist.txt`
