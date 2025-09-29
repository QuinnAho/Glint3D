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
- [ ] glViewport — replace with RHI::setViewport
- [ ] glClearColor — remove; pass color to RHI::clear
- [ ] glClear — replace with RHI::clear

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
- [ ] glTexImage2D / glTexSubImage2D — RHI::updateTexture
- [ ] glTexParameteri — handled by TextureDesc or backend defaults
- [ ] glActiveTexture — RHI bindTexture(slot) handles binding slot

Buffers/VAO
- [ ] glGenBuffers / glDeleteBuffers — RHI createBuffer/destroyBuffer
- [ ] glBindBuffer / glBufferData / glBufferSubData — RHI::updateBuffer
- [ ] VAO usage — replace with `PipelineDesc.vertexBindings/vertexAttributes`

Draw
- [ ] glDrawArrays / glDrawElements — RHI::draw with DrawDesc
- [ ] glReadPixels / glReadBuffer / glPixelStorei — replace with RHI::readback

Utilities (Observed in render_system.cpp)
- [ ] glGetIntegerv(GL_VIEWPORT) — remove; maintain viewport via RHI
- [x] Screen-quad VAO/VBO — replace with RHI utility for full-screen pass
  - ✅ RHI::getScreenQuadBuffer() implemented and available
  - Ready to replace m_screenQuadVAO/VBO in render_system.cpp

Phase A Completion Status (2024-12-29)
- ✅ Framebuffer operations audit completed with line-by-line documentation
- ✅ RHI abstraction layer verified complete for render targets and resolve
- ✅ Screen-quad utility implemented in RHI (eliminates VAO/VBO duplication)
- ✅ Ready for Phase B: renderToTextureRHI and pass implementation migration

Notes
- Track each call-site migration by file and line range in this file as you go.
- Keep legacy GL behind a feature flag during transition to expedite verification.
- Phase A prerequisites for pass_bridging are now complete - unblocks next task.

