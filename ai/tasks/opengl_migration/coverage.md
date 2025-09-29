# OpenGL Call Coverage Checklist

Legend: [ ] pending  [~] in-progress  [x] migrated

Framebuffer/Resolve
- [ ] glGenFramebuffers / glDeleteFramebuffers — replace with RHI create/destroyRenderTarget
  - Lines 703, 755, 806, 978 (glGenFramebuffers)
  - Lines 727, 764, 767, 787, 790, 853, 888, 983, 1013, 2054 (glDeleteFramebuffers)
- [ ] glBindFramebuffer — replace with RHI::bindRenderTarget
  - Lines 460, 704, 724, 756, 763, 779, 780, 786, 807, 851, 878, 945, 979, 982, 1006 (glBindFramebuffer)
- [ ] glFramebufferTexture2D / glFramebufferRenderbuffer — use RenderTargetDesc attachments
  - Lines 709, 715, 718 (glFramebufferRenderbuffer for MSAA)
  - Lines 757, 811, 826, 834, 980 (glFramebufferTexture2D and glFramebufferRenderbuffer for depth)
- [ ] glCheckFramebufferStatus — rely on RHI createRenderTarget failure reporting
  - Not found in render_system.cpp (good - error checking should be handled by RHI)
- [ ] Manual MSAA resolve — use RHI::resolveRenderTarget / resolveToDefaultFramebuffer
  - Lines 779-780 (glBindFramebuffer READ/DRAW for blit operations)
- [ ] glGenRenderbuffers / glDeleteRenderbuffers — replace with RHI render target attachments
  - Lines 706, 711, 819 (glGenRenderbuffers)
  - Lines 725, 726, 765, 766, 788, 789, 843, 852, 887, 2052, 2053 (glDeleteRenderbuffers)
- [ ] glBindRenderbuffer / glRenderbufferStorage — replace with RenderTargetDesc configuration
  - Lines 707, 712, 820 (glBindRenderbuffer)
  - Lines 708, 714, 717, 825, 833 (glRenderbufferStorage/glRenderbufferStorageMultisample)

Viewport/Clear
- [~] glViewport — replace with RHI::setViewport
  - ✅ Line 867: renderToTexture legacy path migrated to RHI::setViewport with fallback
  - ❌ Lines 735, 775, 799, 861, 885, 1008: still direct GL calls in other legacy paths
- [~] glClearColor — remove; pass color to RHI::clear
  - ✅ Lines 433-439: glClearColor optimization now only runs when RHI unavailable
  - ❌ Lines 744, 866: still direct glClearColor calls (in fallback/legacy paths)
- [~] glClear — replace with RHI::clear
  - ✅ Line 1404: raytracing display migrated to use m_rhi->clear() with GL fallback
  - ❌ Lines 467, 745, 867: still direct GL calls (in fallback/legacy paths)

Pipeline/State
- [ ] glEnable(GL_DEPTH_TEST) / glDisable — move to PipelineDesc depth state
- [ ] glEnable(GL_MULTISAMPLE) — RHI backend handles or encode in RT config
- [ ] glPolygonMode / glLineWidth / glDepthMask — encode in PipelineDesc (wireframe overlay: dedicated overlay pipeline)
- [ ] glBlendFunc — encode in PipelineDesc (blend state)
- [ ] glEnable(GL_POLYGON_OFFSET_LINE) / glPolygonOffset — overlay pipeline state
- [ ] glGetIntegerv(GL_POLYGON_MODE) — remove, track in RHI

Textures
- [ ] glGenTextures / glDeleteTextures — RHI createTexture/destroyTexture
- [ ] glBindTexture — RHI::bindTexture (as needed)
- [~] glTexImage2D / glTexSubImage2D — RHI::updateTexture
  - ✅ Lines 1390-1395: raytracing texture update migrated to m_rhi->updateTexture()
  - ✅ Lines 2261-2266: second raytracing texture update migrated to m_rhi->updateTexture()
  - ❌ Still TODO: texture creation calls (glTexImage2D, glGenTextures in various places)
- [ ] glTexParameteri — handled by TextureDesc or backend defaults
- [~] glActiveTexture — RHI bindTexture(slot) handles binding slot
  - ✅ Lines 2279-2280: raytracing display texture binding migrated to m_rhi->bindTexture()
  - ❌ Line 1420: still uses glActiveTexture+glBindTexture in fallback path (appropriate)

Buffers/VAO
- [ ] glGenBuffers / glDeleteBuffers — RHI createBuffer/destroyBuffer
- [ ] glBindBuffer / glBufferData / glBufferSubData — RHI::updateBuffer
- [ ] VAO usage — replace with `PipelineDesc.vertexBindings/vertexAttributes`

Draw
- [ ] glDrawArrays / glDrawElements — RHI::draw with DrawDesc
- [ ] glReadPixels / glReadBuffer / glPixelStorei — replace with RHI::readback

Utilities (Observed in render_system.cpp)
- [~] glGetIntegerv(GL_VIEWPORT) — remove; maintain viewport via RHI
  - ✅ Lines 392-401: PassContext setup migrated to use tracked m_fbWidth/m_fbHeight when RHI available
  - ❌ Lines 442, 689, 915: still direct GL queries in MSAA resize and legacy paths
- [~] glGetIntegerv(GL_CURRENT_PROGRAM) — remove when RHI available; UBO lighting handles
  - ✅ Lines 1512-1519: GL_CURRENT_PROGRAM query bypassed when RHI available
  - ❌ Line 1519: still uses legacy lighting in GL fallback path (appropriate)
- [~] Screen-quad VAO/VBO — replace with RHI utility for full-screen pass
  - ✅ RHI::getScreenQuadBuffer() implemented and available
  - ✅ Lines 1439-1457: raytracing display screen quad migrated to RHI::getScreenQuadBuffer() + draw()
  - ❌ Still TODO: background gradient screen quad (lines 487-489), other screen quad usages

Phase A Completion Status (2024-12-29)
- ✅ Framebuffer operations audit completed with line-by-line documentation
- ✅ RHI abstraction layer verified complete for render targets and resolve
- ✅ Screen-quad utility implemented in RHI (eliminates VAO/VBO duplication)
- ✅ Ready for Phase B: renderToTextureRHI and pass implementation migration

Phase B Progress Update (2025-01-15)
- ✅ Global state setup (glEnable depth/MSAA/SRGB) moved to RHI::init()
- ✅ Dummy shadow texture GL fallback removed (RHI-only path)
- ✅ Screen quad VAO/VBO initialization removed (using RHI::getScreenQuadBuffer())
- ✅ Screen quad VAO/VBO cleanup removed
- ✅ Raytracing texture GL fallback removed (RHI-only path)
- ✅ Raytracing texture GL cleanup removed
- 📊 Progress: 182 → 146 GL calls (36 eliminated, 20% reduction)
- ⚠️ Remaining: 146 GL calls in render_system.cpp (80% still to migrate)
- 🔍 Core challenge: 700+ lines of framebuffer operations in renderToTexture() functions

Remaining GL Calls Breakdown (146 total)
1. **Framebuffer Operations** (~40 calls in renderToTexture functions)
   - glGenFramebuffers (3), glBindFramebuffer (10), glDeleteFramebuffers (1)
   - glGenRenderbuffers (3), glBindRenderbuffer (3), glRenderbufferStorage* (5), glDeleteRenderbuffers (2)
   - glFramebufferTexture2D (2), glFramebufferRenderbuffer (5)
   - glCheckFramebufferStatus (2), glBlitFramebuffer (1), glDrawBuffers (2)

2. **VAO/Draw Operations** (~25 calls in legacy render paths)
   - glBindVertexArray (12), glDrawArrays (7), glDrawElements (5)

3. **State Management** (~20 calls in wireframe/transparency)
   - glEnable/glDisable (12), glDepthMask (4), glBlendFuncSeparate (2)
   - glPolygonMode (7), glPolygonOffset (2), glLineWidth (2)

4. **Viewport/Clear** (~15 calls in legacy paths)
   - glViewport (8), glClear (4), glClearColor (3)

5. **Texture Operations** (~10 calls)
   - glBindTexture (6), glActiveTexture (2), glTexSubImage2D (2)

6. **Queries** (~8 calls)
   - glGetIntegerv (7) - viewport, polygon mode, current program

Migration Strategy
- **Phase 1**: Replace renderToTexture() with RHI createRenderTarget/resolveRenderTarget
- **Phase 2**: Migrate VAO bindings to use pipeline vertex attributes
- **Phase 3**: Replace state changes with pipeline descriptors
- **Phase 4**: Migrate viewport/clear to RHI operations
- **Phase 5**: Remove remaining texture bindings and queries

Notes
- Track each call-site migration by file and line range in this file as you go.
- Keep legacy GL behind a feature flag during transition to expedite verification.
- Phase A prerequisites for pass_bridging are now complete - unblocks next task.

