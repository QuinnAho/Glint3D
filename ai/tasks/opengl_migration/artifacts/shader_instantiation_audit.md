# RenderSystem Legacy Shader Instantiation Audit

**Date**: 2025-10-11
**Task**: opengl_migration Phase 6 - Critical Issue: RenderSystem Legacy Shader Wrappers
**Scope**: Identify all `unique_ptr<Shader>` members and their usage patterns

---

## Legacy Shader Members (engine/include/render_system.h)

| Member | Line | Status |
|--------|------|--------|
| `m_screenQuadShader` | 262 | ⚠️ USED - raytracing screen quad display |
| `m_basicShader` | 287 | ⚠️ USED - selection/comparison logic |
| `m_pbrShader` | 288 | 🔴 HEAVILY USED - primary object rendering |
| `m_gridShader` | 289 | ✅ UNUSED - Grid class now self-contained |
| `m_gradientShader` | 290 | ✅ UNUSED - loaded but never referenced |

---

## Instantiation & Loading (engine/src/render_system.cpp)

### Lines 175-194 in RenderSystem::init()

```cpp
// Line 175-176: PBR shader (USED)
m_pbrShader = std::make_unique<Shader>();
m_pbrShader->load("engine/shaders/pbr.vert", "engine/shaders/pbr.frag");

// Line 179: Basic shader (USED but reset to null)
m_basicShader.reset(); // Uses m_pbrShader instead

// Line 182-184: Grid shader (UNUSED)
m_gridShader = std::make_unique<Shader>();
if (!m_gridShader->load("engine/shaders/grid.vert", "engine/shaders/grid.frag"))
    std::cerr << "[RenderSystem] Failed to load grid shader.\n";

// Line 187-189: Gradient shader (UNUSED)
m_gradientShader = std::make_unique<Shader>();
if (!m_gradientShader->load("engine/shaders/gradient.vert", "engine/shaders/gradient.frag"))
    std::cerr << "[RenderSystem] Failed to load gradient shader.\n";

// Line 192-194: Screen quad shader (USED)
m_screenQuadShader = std::make_unique<Shader>();
if (!m_screenQuadShader->load("engine/shaders/rayscreen.vert", "engine/shaders/rayscreen.frag"))
    std::cerr << "[RenderSystem] Failed to load rayscreen shader.\n";
```

---

## Usage Analysis

### m_pbrShader (🔴 HEAVILY USED)
**Usage sites**:
- Line 1374: `m_pbrShader->use()` - Activates PBR shader for rendering
- Line 1375: `setupCommonUniforms(m_pbrShader.get())` - Sets uniforms
- Line 1382: `renderObjectFast(*obj, lights, m_pbrShader.get())` - Per-object rendering

**Impact**: PRIMARY rendering path - requires careful migration to RHI pipelines

**Replacement strategy**:
- Already has RHI equivalent: `m_pbrShaderRhi` (created at line 215)
- Need to replace `->use()` calls with pipeline binding via managers
- Need to replace `setupCommonUniforms()` with manager UBO updates
- Need to replace `renderObjectFast()` shader parameter with pipeline handle

---

### m_screenQuadShader (⚠️ USED)
**Usage sites**:
- Line 947: `m_screenQuadShader->use()` - Raytracing display
- Lines 950-952, 957: `setFloat()`, `setInt()` - Uniforms for tonemapping/exposure
- Line 1707: `m_screenQuadShader->use()` - Another display path
- Line 1708: `setInt()` - Screen texture binding

**Impact**: Raytracing display path - requires RHI shader + pipeline

**Replacement strategy**:
- Create RHI shader handle for rayscreen shaders
- Replace `->use()` with RHI pipeline binding
- Replace `setFloat/setInt` with RHI uniform setters or UBO

---

### m_basicShader (⚠️ USED - selection logic)
**Usage sites**:
- Line 179: `.reset()` - Cleared to null, uses m_pbrShader instead
- Line 1009: `if (s == m_basicShader.get())` - Shader comparison
- Line 1056: `if (s == m_basicShader.get())` - Shader comparison
- Line 1250: `if (selObj >= 0 && selObj < (int)objs.size() && m_basicShader)` - Selection check
- Line 1253: `Shader* s = m_basicShader.get()` - Pointer retrieval

**Impact**: LOW - mostly null, used for legacy comparisons

**Replacement strategy**:
- Remove comparison logic (obsolete with RHI pipelines)
- Already has RHI equivalent: `m_basicShaderRhi = m_pbrShaderRhi` (line 218)

---

### m_gridShader (✅ UNUSED)
**Usage sites**: NONE (only loaded, never referenced)

**Impact**: ZERO - safe to remove

**Replacement strategy**:
- Delete declaration from render_system.h:289
- Delete instantiation/loading from render_system.cpp:182-184
- Grid class is now self-contained with its own RHI shader

---

### m_gradientShader (✅ UNUSED)
**Usage sites**: NONE (only loaded, never referenced)

**Impact**: ZERO - safe to remove

**Replacement strategy**:
- Delete declaration from render_system.h:290
- Delete instantiation/loading from render_system.cpp:187-189
- If gradient background is implemented, it should use RHI directly

---

## Removal Priority

### Phase 1: Remove Unused Shaders (LOW RISK) ✅
1. ✅ Remove `m_gridShader` declaration + instantiation
2. ✅ Remove `m_gradientShader` declaration + instantiation

### Phase 2: Migrate m_screenQuadShader (MEDIUM RISK)
3. Create `m_screenQuadShaderRhi` RHI handle
4. Create `m_screenQuadPipeline` RHI pipeline
5. Replace `m_screenQuadShader->use()` with pipeline binding
6. Replace `setFloat/setInt` calls with RHI uniform setters
7. Test raytracing display path

### Phase 3: Migrate m_pbrShader (HIGH RISK)
8. Remove `m_pbrShader->use()` calls (use pipeline manager instead)
9. Refactor `setupCommonUniforms()` to use manager UBOs exclusively
10. Refactor `renderObjectFast()` to not require Shader* parameter
11. Test primary rendering path extensively

### Phase 4: Clean Up m_basicShader (LOW RISK)
12. Remove shader comparison logic (lines 1009, 1056)
13. Remove selection check logic (lines 1250, 1253)
14. Remove declaration

---

## Estimated Effort

- **Phase 1**: 15 minutes (simple deletion)
- **Phase 2**: 1-2 hours (screen quad migration + testing)
- **Phase 3**: 3-4 hours (PBR migration + setupCommonUniforms refactor + testing)
- **Phase 4**: 30 minutes (cleanup + testing)

**Total**: 5-7 hours for complete Shader removal

---

## Dependencies

**Blocked by**:
- None (can proceed immediately)

**Blocks**:
- `setUniform*` bridge removal (60+ call sites)
- Legacy Shader class deletion (shader.cpp - 24 GL calls)
- Backend swap validation

---

## Next Steps

1. Start with Phase 1 (remove unused shaders) - immediate win
2. Proceed to Phase 2 (screen quad) - isolated subsystem
3. Tackle Phase 3 (PBR) - requires careful testing
4. Finish with Phase 4 (cleanup) - final polish

---

*Generated: 2025-10-11 as part of opengl_migration Phase 6 critical integration fixes*
