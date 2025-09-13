# Direct GL Calls Audit in render_system.cpp

**Total Count: 247 direct GL calls**

## Categories to Migrate to RHI:

### 1. **Viewport/State Management** (HIGH priority)
- `glViewport()` calls → `RHI::setViewport()`
- `glClear()` calls → `RHI::clear()`
- `glEnable/glDisable()` state → RHI state management

### 2. **Framebuffer Operations** (HIGH priority)  
- `glBindFramebuffer()` → RHI render target system
- `glFramebufferTexture2D()` → RHI render target creation
- `glCheckFramebufferStatus()` → RHI validation

### 3. **Texture Management** (MEDIUM priority)
- `glGenTextures/glBindTexture/glTexImage2D` → `RHI::createTexture()`
- `glTexParameteri()` → TextureDesc parameters
- Texture binding → `RHI::bindTexture()`

### 4. **Uniform Updates** (MEDIUM priority)
- `glUniform*()` calls → `RHI::updateBuffer()` with UBOs
- `glGetUniformLocation()` → RHI uniform buffer system

### 5. **VAO/Drawing** (LOW priority - working through RHI already)
- `glBindVertexArray()` calls 
- VAO operations (some already going through RHI)

## Migration Strategy:
1. **Phase 1**: Viewport, clear, basic state (quick wins)
2. **Phase 2**: Framebuffer operations (render targets) 
3. **Phase 3**: Texture creation and binding
4. **Phase 4**: Uniform buffer migration
5. **Phase 5**: Remaining edge cases

## Current Status:
- RHI interface exists and compiles
- RhiGL backend implemented  
- Migration not started - all 247 calls need routing through RHI