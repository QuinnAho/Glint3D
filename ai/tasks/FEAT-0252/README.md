# FEAT-0252 — WebGPU Backend (wgpu-native)

## Intent
Implement a WebGPU backend using wgpu-native (and/or Dawn) to deliver desktop/web parity against the WebGPU-shaped RHI frontend.

## Acceptance
- Backend runs core scenes
- GL vs WebGPU image diff < epsilon on 10–20 golden scenes
- Encoders/passes/bind groups semantics match frontend

## Scope
- Headers: `engine/include/glint3d/rhi*.h`
- Impl: `engine/src/rhi/**`
- Tests: `tests/unit/rhi_test.cpp`
- Docs: `docs/rhi_architecture.md`

## Links
- Spec: `ai/tasks/FEAT-0252/spec.yaml`
- Passes: `ai/tasks/FEAT-0252/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0252/whitelist.txt`
