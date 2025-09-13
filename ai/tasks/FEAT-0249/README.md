# FEAT-0249 — Uniform System Migration (UBO ring + reflection)

## Intent
Migrate all uniforms to a reflection-validated UBO ring allocator. Remove per-draw `glUniform*` and validate offsets against shader reflection data.

## Acceptance
- 100% migration to UBO ring allocator
- Reflection-validated offsets with runtime asserts
- No `glUniform*` outside GL backend
- Golden tests pass without regressions

## Scope
- Headers: `engine/include/glint3d/rhi*.h`
- Impl: `engine/src/rhi/**`
- Tests: `tests/unit/rhi_test.cpp`
- Docs: `docs/rhi_architecture.md`

## Links
- Spec: `ai/tasks/FEAT-0249/spec.yaml`
- Passes: `ai/tasks/FEAT-0249/passes.yaml`
- Whitelist: `ai/tasks/FEAT-0249/whitelist.txt`
