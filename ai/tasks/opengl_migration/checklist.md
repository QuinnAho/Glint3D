# OpenGL Migration Checklist

## Audit Phase

### OpenGL Call Inventory
- [ ] **Complete Audit** - Document all 112+ OpenGL calls in render_system.cpp with line numbers
- [ ] **Categorization** - Group calls by function: framebuffer, state, resources, rendering
- [ ] **Impact Assessment** - Identify critical vs. optional GL operations
- [ ] **RHI Gap Analysis** - Document missing RHI abstractions needed

## RHI Abstraction Implementation

### Framebuffer Operations (Lines 703-888)
- [ ] **RHI::createFramebuffer()** - Replace glGenFramebuffers, glBindFramebuffer
- [ ] **RHI::createRenderbuffer()** - Replace glGenRenderbuffers, glRenderbufferStorage
- [ ] **RHI::framebufferAttachment()** - Replace glFramebufferTexture2D, glFramebufferRenderbuffer
- [ ] **RHI::blitFramebuffer()** - Replace glBlitFramebuffer for MSAA resolve
- [ ] **RHI::checkFramebufferStatus()** - Replace glCheckFramebufferStatus

### Rendering State Management (Lines 123-126, 475-482, 544-574)
- [ ] **RHI::setDepthTest()** - Replace glEnable/glDisable(GL_DEPTH_TEST)
- [ ] **RHI::setBlending()** - Replace glEnable/glDisable(GL_BLEND), glBlendFuncSeparate
- [ ] **RHI::setPolygonMode()** - Replace glPolygonMode for wireframe rendering
- [ ] **RHI::setLineWidth()** - Replace glLineWidth for wireframe
- [ ] **RHI::setDepthMask()** - Replace glDepthMask for transparency

### Resource Management (Lines 260-276, 960-967, 1762-1773)
- [ ] **RHI::createTexture2D()** - Replace glGenTextures, glTexImage2D
- [ ] **RHI::setTextureParameters()** - Replace glTexParameteri calls
- [ ] **RHI::updateTextureData()** - Replace glTexSubImage2D
- [ ] **RHI::deleteTexture()** - Replace glDeleteTextures
- [ ] **RHI::bindTexture()** - Replace glBindTexture, glActiveTexture

### Vertex Array Operations (Lines 1717-1732, 479-481, 1438-1440)
- [ ] **RHI::createVertexArray()** - Replace glGenVertexArrays
- [ ] **RHI::setVertexAttributes()** - Replace glVertexAttribPointer, glEnableVertexAttribArray
- [ ] **RHI::drawArrays()** - Replace glDrawArrays, glDrawElements
- [ ] **RHI::bindVertexArray()** - Replace glBindVertexArray

### Viewport and Clear Operations (Lines 393, 435, 467, 744-745, 865-867)
- [ ] **RHI::getViewport()** - Replace glGetIntegerv(GL_VIEWPORT)
- [ ] **RHI::setClearColor()** - Replace glClearColor
- [ ] **RHI::clear()** - Replace glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

## Screen-Quad Utility Implementation
- [ ] **RHI Screen-Quad Class** - Create reusable full-screen quad utility
- [ ] **Vertex Buffer Creation** - RHI-based vertex buffer for quad vertices
- [ ] **Pipeline Support** - Support multiple shader pipelines
- [ ] **Rendering Interface** - Simple draw() method for full-screen passes

## Migration Execution

### Incremental Migration Strategy
- [ ] **Phase 1: Framebuffer** - Migrate FBO operations first (highest impact)
- [ ] **Phase 2: State Management** - Migrate rendering state changes
- [ ] **Phase 3: Resources** - Migrate texture and buffer operations
- [ ] **Phase 4: Vertex Arrays** - Migrate VAO operations
- [ ] **Phase 5: Utilities** - Migrate viewport and clear operations

### Validation Per Phase
- [ ] **Build Verification** - Ensure project compiles after each migration phase
- [ ] **Runtime Testing** - Verify rendering output remains identical
- [ ] **Performance Check** - Ensure no significant performance regression
- [ ] **OpenGL Detection** - Run grep to verify target calls eliminated

## Final Validation

### Automated Detection
- [ ] **Zero OpenGL Calls** - `grep -r 'gl[A-Z]' engine/src/render_system.cpp` returns empty
- [ ] **GL Constant Check** - `grep -r 'GL_[A-Z]' engine/src/render_system.cpp` only in comments
- [ ] **Include Cleanup** - Remove unnecessary OpenGL headers from render_system.cpp

### Functional Testing
- [ ] **Raster Pipeline** - Verify raster rendering works with pure RHI
- [ ] **Ray Pipeline** - Verify ray integration works with RHI textures
- [ ] **Offscreen Rendering** - Verify headless rendering via RHI
- [ ] **MSAA Support** - Verify multi-sampling works through RHI

### Documentation
- [ ] **Migration Log** - Document all changes made during migration
- [ ] **RHI Coverage** - Document RHI API coverage achieved
- [ ] **Performance Impact** - Document any performance changes
- [ ] **Future Extensions** - Document areas for future RHI expansion