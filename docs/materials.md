# MaterialCore Public API

- Overview: Unified BSDF material representation used by both raster and ray pipelines.
- Header: `engine/include/glint3d/material_core.h`
- Goals: Single source of truth; eliminate PBR ↔ legacy conversion drift; preserve opaque behavior.

## Fields (core)
- `baseColor: vec4` — RGBA albedo (A is author‑controlled alpha, not premultiplied)
- `metallic: float [0,1]` — 1.0 for conductors
- `roughness: float [0,1]` — microfacet roughness
- `ior: float ≥ 1.0` — index of refraction for dielectrics
- `transmission: float [0,1]` — transparency factor
- `thickness: float ≥ 0` — volume thickness in meters
- `emissive: vec3` — emission radiance

## Factories
- `createMetal(color, roughness)`
- `createDielectric(color, roughness)`
- `createGlass(color, ior, transmission)`
- `createEmissive(color, intensity)`

## Validation and Utilities
- `validate()` — check ranges; `clampValues()` — fix out‑of‑range inputs
- `isMetal()`, `isTransparent()`, `isEmissive()`; `needsRaytracing()` helper for mode selection

## JSON Ops Integration
- Schema (`schemas/json_ops_v1.json`) supports `ior`, `transmission`, `thickness` on `set_material`.
- Loader should populate `MaterialCore` directly; legacy material is synchronized only during transition.

## Behavior Guarantees
- Opaque materials (transmission ≈ 0): Raster output remains unchanged vs baseline.
- Transmission/refraction preview is introduced later via SSR‑T; raytracing remains ground truth for finals.
