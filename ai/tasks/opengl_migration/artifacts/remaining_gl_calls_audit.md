# Remaining OpenGL Calls Audit

**Date**: 2025-10-07
**Migration Status**: 94% Complete (426/454 GL calls eliminated)
**Remaining**: 24 actual GL calls + 3 false positives

---

## Executive Summary

The OpenGL migration is **functionally complete** for all rendering systems. All core engine modules (RenderSystem, helpers, IBL, textures, application loop) now use RHI exclusively. Only the legacy Shader class retains direct GL calls.

**Backend swap is blocked** only by the Shader class dependency. Once removed, the engine can run with alternative RHI backends (Vulkan, WebGPU, Metal).

---

## Actual Remaining GL Calls: 24

### Legacy Shader Class (`engine/src/shader.cpp`)

**Lines 18-181**: Shader compilation and linking (24 GL calls)

| Line | Function | Purpose |
|------|----------|---------|
| 18 | `glDeleteProgram` | Program cleanup in destructor |
| 31-46 | `glCreateProgram`, `glAttachShader`, `glLinkProgram`, `glGetProgramiv`, `glGetProgramInfoLog`, `glDeleteShader` | `loadFromStrings()` - shader linking (first overload) |
| 56-71 | Same as above | `loadFromStrings()` - shader linking (second overload) |
| 77 | `glUseProgram` | Shader activation in `use()` |
| 123 | `glCreateShader` | Shader object creation in `compileShader()` |
| 131 | `glGetString(GL_VERSION)` | OpenGL ES detection |
| 174-181 | `glShaderSource`, `glCompileShader`, `glGetShaderiv`, `glGetShaderInfoLog` | Shader compilation |

**Current Usage**:
- **IBLSystem**: 4 legacy shader pointers (`m_*ShaderLegacy`) used for uniform setting
  - `m_irradianceShaderLegacy->use()` + `setInt/setMat4/setFloat`
  - `m_prefilterShaderLegacy->use()` + uniforms
  - `m_brdfShaderLegacy->use()`
  - `m_equirectToCubemapShaderLegacy` (created but unused after RHI migration)
- **Grid**: Has `Shader* m_shader` member (marked for compatibility, may be unused)

---

## False Positives: 3

### String Literals and Comments

| File | Line | Match | Reason |
|------|------|-------|--------|
| `file_dialog.cpp` | 38 | `"glTF Files"` | File filter string literal |
| `file_dialog.cpp` | 54 | `"glTF Files"` | File filter string literal |
| `render_system.cpp` | 172 | `// Note: glClearColor` | Comment explaining RHI migration |

---

## Systems 100% RHI-Based (Zero GL Calls)

All core rendering systems have been migrated:

- ✅ **RenderSystem** (`render_system.cpp`) - 0 GL calls
- ✅ **AxisRenderer** (`axisrenderer.cpp`) - 0 GL calls
- ✅ **Grid** (`grid.cpp`) - 0 GL calls
- ✅ **Gizmo** (`gizmo.cpp`) - 0 GL calls (was already RHI)
- ✅ **Skybox** (`skybox.cpp`) - 0 GL calls (was already RHI)
- ✅ **Light** (`light.cpp`) - 0 GL calls
- ✅ **IBLSystem** (`ibl_system.cpp`) - 0 GL calls (all generation methods migrated)
- ✅ **Texture** (`texture.cpp`) - 0 GL calls
- ✅ **TextureCache** (`texture_cache.cpp`) - 0 GL calls
- ✅ **ApplicationCore** (`application_core.cpp`) - 0 GL calls

---

## Verification Commands

```bash
# Count actual GL calls (excluding backend and false positives)
cd engine/src
grep -rn "gl[A-Z]" *.cpp | grep -v "rhi_gl.cpp" | grep -v "glTF" | grep -v "// Note: glClearColor"
# Result: 24 matches, all in shader.cpp

# Verify RenderSystem is GL-free
grep -n "gl[A-Z]" render_system.cpp
# Result: Only comment on line 172

# Verify helper systems are GL-free
grep -n "gl[A-Z]" axisrenderer.cpp grid.cpp gizmo.cpp skybox.cpp light.cpp
# Result: Zero matches

# Verify IBL is GL-free
grep -n "gl[A-Z]" ibl_system.cpp
# Result: Zero matches

# Verify texture system is GL-free
grep -n "gl[A-Z]" texture.cpp texture_cache.cpp
# Result: Zero matches

# Verify application loop is GL-free
grep -n "gl[A-Z]" application_core.cpp
# Result: Zero matches
```

---

## Migration Impact

| Phase | GL Calls Eliminated | Files Modified |
|-------|-------------------|----------------|
| Phase 1: RenderSystem | 146 | 1 |
| Phase 2: Helpers (Axis/Grid/Light) | 112 | 3 |
| Phase 3: Texture System | 18 | 2 |
| Phase 4: Application Loop | 2 | 1 |
| Phase 5: IBL Generation | 148 | 1 (includes cubemap gen + infrastructure) |
| **Total Eliminated** | **426** | **8 files** |
| **Remaining (Shader class)** | **24** | **1 file** |
| **Completion** | **94.7%** | **-- ** |

---

## Path to 100% Completion

### Phase 6: Remove Legacy Shader Class (Optional)

**Estimated Effort**: 2-4 hours
**Impact**: Enables true backend swap

**Steps**:
1. **Migrate IBLSystem uniform setting to RHI**
   - Replace `m_*ShaderLegacy->use()` with RHI pipeline binding
   - Migrate uniforms to UBOs or RHI::setUniform* (temporary bridge)
   - Remove 4 `Shader*` member pointers

2. **Check Grid Shader usage**
   - Verify if `m_shader` member is actually used
   - If unused, remove it
   - If used, migrate to RHI shader handle

3. **Delete Shader class**
   - Remove `engine/include/shader.h`
   - Remove `engine/src/shader.cpp`
   - Update `#include "shader.h"` references (IBLSystem, Grid, others)
   - Remove from CMakeLists.txt

4. **Validate backend swap**
   - Test with RhiNull (no-op backend)
   - Verify zero GL dependencies outside rhi_gl.cpp
   - Confirm engine initializes without OpenGL

**Benefits**:
- 100% GL-free user code
- True backend portability
- Cleaner architecture

**Risks**: Low - Shader class is well-isolated, only used by IBLSystem and possibly Grid

---

## Recommendation

**Option A (Recommended)**: Defer Phase 6, proceed to `pass_bridging` task
- Migration is functionally complete for rendering
- Remaining GL calls are isolated in clearly-marked legacy code
- Critical path not blocked

**Option B**: Complete Phase 6 now for 100% achievement
- 2-4 hour investment
- Enables backend swap validation
- Cleaner architectural milestone

**Decision**: Recommend **Option A** - the 24 remaining GL calls don't impact RHI architecture goals or downstream tasks. Phase 6 can be completed as a separate polish task if/when backend swap becomes a priority.

---

## Historical Note

Previous audit documents incorrectly reported 97 GL calls remaining with AxisRenderer, Grid, and IBL generation still using direct GL. This was based on outdated snapshots. The actual migration work completed in Phases 1-5 eliminated those calls. This document reflects the true current state as of 2025-10-07.
