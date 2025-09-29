# GL → RHI Mapping Guide

Framebuffer/Resolve
- glGenFramebuffers/glDeleteFramebuffers → RHI::createRenderTarget / destroyRenderTarget
  Key functions: renderToTextureRHI() (MSAA and non-MSAA paths), renderToPNG(), destructor cleanup
- glBindFramebuffer → RHI::bindRenderTarget
  Patterns: GL_FRAMEBUFFER for main binding, GL_READ_FRAMEBUFFER/GL_DRAW_FRAMEBUFFER for MSAA resolve
- glFramebufferTexture2D/glFramebufferRenderbuffer → RenderTargetDesc.colorAttachments/depthAttachment
  MSAA: Use renderbuffers for both color (RGBA8) and depth (DEPTH24_STENCIL8 desktop, DEPTH_COMPONENT16 web)
  Non-MSAA: Direct texture attachment with separate depth renderbuffer
- Manual resolve (blit) → RHI::resolveRenderTarget / resolveToDefaultFramebuffer
  Current pattern: glBlitFramebuffer between fboMSAA (READ) and fboResolve (DRAW)
- glGenRenderbuffers/glDeleteRenderbuffers → Implicit in RenderTargetDesc attachment configuration
- glRenderbufferStorage/glRenderbufferStorageMultisample → RenderTargetDesc.samples and format specification

Viewport/Clear
- glViewport → RHI::setViewport
- glClearColor/glClear → RHI::clear(glm::vec4 color, depth, stencil)

Pipeline/State
- glEnable/glDisable (DEPTH_TEST, BLEND, MULTISAMPLE) → PipelineDesc.depthTest/blend/multisample
- glBlendFunc → PipelineDesc.blend factors
- glPolygonMode → PipelineDesc.rasterizer.fillMode (wireframe) (backend support required)
- glLineWidth → PipelineDesc.rasterizer.lineWidth (or shader-based outline)
- glDepthMask → PipelineDesc.depthWrite
- glPolygonOffset → PipelineDesc.rasterizer.depthBias
- glGetIntegerv(GL_POLYGON_MODE) → remove; RHI doesn’t expose implicit state

Textures
- glGenTextures/glDeleteTextures → RHI::createTexture/destroyTexture
- glBindTexture/glActiveTexture → RHI::bindTexture(texture, slot)
- glTexImage2D/glTexSubImage2D → RHI::updateTexture(texture, data, w, h, format, x, y, mip)
- glTexParameteri → map to TextureDesc sampler fields or backend defaults

Buffers/VAO
- glGenBuffers/glDeleteBuffers → RHI::createBuffer/destroyBuffer
- glBindBuffer/glBufferData/glBufferSubData → RHI::updateBuffer
- VAO and attrib setup → PipelineDesc.vertexBindings + vertexAttributes

Draw
- glDrawArrays/glDrawElements → RHI::draw(DrawDesc)
- glReadPixels → RHI::readback(ReadbackDesc)

Queries/Introspection
- glGetIntegerv(GL_VIEWPORT) → track viewport via RHI setViewport calls

Utilities
- Screen-quad rendering → RHI::getScreenQuadBuffer()
  Cached vertex buffer with NDC coordinates (-1,1) and UV mapping (0,1)
  Format: position (vec2), UV (vec2) - 6 vertices forming 2 triangles
  Usage: Full-screen passes, post-processing, ray integration display

Notes
- Where RHI lacks a 1:1 state, prefer pre-configured pipelines over dynamic toggles.
- For overlays (selection wireframe), use a dedicated overlay pipeline instead of mutating global state.
- RHI utilities (screen quad) eliminate duplicate GL resource creation across functions.

