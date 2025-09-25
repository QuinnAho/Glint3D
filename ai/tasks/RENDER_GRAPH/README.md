# Render Graph Follow-ups

## Status: COMPLETED (with critical issue)

**Implementation Date:** December 2024
**Critical Issue:** Black screen rendering in new render graph passes
**Details:** See `IMPLEMENTATION_STATUS.md`

## Tasks Completed:

1. ✅ Implement GPU G-buffer/lighting and migrate object rendering into `RasterPass`.
2. ✅ Replace `passRaytrace` screen-quad flow with a texture-producing pass and feed `RayDenoisePass` + composition.
3. ✅ Flesh out `ReadbackPass` and `RayDenoisePass` with real implementations (format conversions, denoiser integration).
4. ✅ Remove residual OpenGL texture uploads in raytracing and replace with RHI texture updates.
5. ✅ Add per-pass timing/metrics and expose them to HUD + auto-mode heuristics.
6. 🚨 Update golden tests to cover raster/ray graphs and add automated readback verification. (BLOCKED by rendering issue)
