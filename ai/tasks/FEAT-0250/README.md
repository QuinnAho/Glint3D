# FEAT-0250 — TextureView + MSAA Resolve Support

## Intent
Add TextureView abstraction and explicit MSAA resolve matching WebGPU semantics; implement in GL backend and validate with tests.

## Acceptance
- TextureView supported in descriptors and bind groups
- Resolve attachments via RenderPassEncoder semantics
- GL backend implements resolve with validation
- Unit tests cover resolve and view formats

## Scope
- Headers: `engine/include/glint3d/rhi*.h`
- Impl: `engine/src/rhi/**`
- Tests: `tests/unit/rhi_test.cpp`
- Docs: `docs/rhi_architecture.md`

## Links
- Spec: `ai/tasks/FEAT-0250/spec.yaml`
- Passes: `ai/tasks/FEAT-0250/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0250/whitelist.txt`
