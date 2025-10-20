# RHI `setUniform*` Call Audit — 2025-10-19

| Location | Uniform APIs | Call Sites | Notes |
| --- | --- | --- | --- |
| `engine/src/axisrenderer.cpp::AxisRenderer::render` | `setUniformMat4("model")`, `setUniformMat4("view")`, `setUniformMat4("projection")` | 3 | Axis gizmo pushes per-frame transforms directly through the bridge; maps cleanly to transform + camera manager data. |
| `engine/src/gizmo.cpp::Gizmo::render` | `setUniformMat4("uModel")`, `setUniformMat4("uView")`, `setUniformMat4("uProj")` | 3 | Transform gizmo mirrors axis renderer usage with alternate uniform names. |
| `engine/src/grid.cpp::Grid::render` | `setUniformMat4("model")`, `setUniformMat4("view")`, `setUniformMat4("projection")`, `setUniformVec3("gridColor")` | 4 | Grid uses static color plus standard transforms; ideal candidate for manager-provided matrices + color parameters. |
| `engine/src/light.cpp::Light::renderIndicators` | `setUniformMat4("view")`, `setUniformMat4("projection")`, `setUniformMat4("model")` (x5 call sites), `setUniformVec3("indicatorColor")` (x5 call sites) | 12 | All light indicator draws send transforms and colors through the bridge for point, directional, spot, and selection highlight paths. |
| `engine/src/skybox.cpp::Skybox::render` | `setUniformMat4("view")`, `setUniformMat4("projection")`, `setUniformBool("useGradient")`, `setUniformVec3("topColor")`, `setUniformVec3("bottomColor")`, `setUniformVec3("horizonColor")`, `setUniformFloat("intensity")`, `setUniformInt("skybox")` | 8 | Skybox toggles gradient parameters and cubemap slot via direct uniform calls. |
| `engine/src/ibl_system.cpp::loadHDREnvironment` | `setUniformMat4("projection")`, `setUniformMat4("view")` | 2 | Environment capture per cubemap face still binds view/projection matrices via the bridge. |
| `engine/src/ibl_system.cpp::generateIrradianceMap` | `setUniformInt("environmentMap")`, `setUniformMat4("projection")`, `setUniformMat4("view")` | 3 | Irradiance generation relies on the bridge for texture slot + transforms. |
| `engine/src/ibl_system.cpp::generatePrefilterMap` | `setUniformInt("environmentMap")`, `setUniformMat4("projection")`, `setUniformMat4("view")`, `setUniformFloat("roughness")` | 4 | Prefilter pass additionally feeds roughness via the bridge while looping over mip levels and faces. |
| `engine/src/shader.cpp::Shader::setUniform*` | `setUniformMat4`, `setUniformVec3`, `setUniformVec4`, `setUniformFloat`, `setUniformInt`, `setUniformBool` | 6 | Legacy Shader wrapper forwards all uniform updates to `Shader::s_rhi`, keeping the bridge alive even when Shader instances are removed. |

**Totals**

- Direct `m_rhi->setUniform*` invocations: 39 call sites across six renderer/helper modules.
- Legacy `Shader::setUniform*` bridge: 6 forwarding helpers.
- Combined count: **45** active call sites still depending on the GL-backed uniform bridge.

**Observations**

- All remaining usages fall into three buckets: per-draw transforms, per-draw colors/flags, and IBL baking parameters. None require ad-hoc immediate mode updates; each can migrate to manager-provided uniform blocks or bind groups.
- Light indicators and skybox contribute 20 of the 45 calls; migrating these first would cut bridge usage nearly in half.
- The static `Shader::s_rhi` indirection keeps the legacy shader class coupled to the bridge even when not instantiated; removing the bridge requires deleting these helpers alongside the class.

## Pattern Breakdown

- **Transform matrices** (24 calls): AxisRenderer, Gizmo, Grid, Light, Skybox, and all IBL passes request model/view/projection data that already lives in TransformManager and CameraManager; bind a shared transform UBO per pass instead of setting matrices per draw.
- **Per-draw colors and flags** (11 calls): Light indicator colors, selection highlights, and Skybox gradient toggles belong in RenderingManager/MaterialManager uniform structs; can be expressed as small structs in a dynamic UBO or push_consts equivalent.
- **Texture slot & scalar params** (6 calls): Skybox intensity/index and IBL environment slot/roughness align with RenderingManager + EnvironmentManager responsibilities; replace with bind group descriptors or descriptor sets when pipelines are bound.
- **Legacy shader bridge** (6 calls): `Shader::setUniform*` forwarding is solely for compatibility; removing the class eliminates these calls once managers own uniform updates.
