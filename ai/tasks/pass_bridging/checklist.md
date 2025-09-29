# Pass Bridging Checklist

## Critical Pass Implementations

### passGBuffer (engine/src/render_system.cpp:~2392)
- [ ] **Render Target Binding** - Bind G-buffer render target via RHI
- [ ] **Viewport Setup** - Set viewport and clear G-buffer attachments
- [ ] **Scene Rendering** - Draw scene using G-buffer shaders + PipelineManager
- [ ] **Per-Object Uniforms** - Set model matrix via RHI::setUniformMat4 per object
- [ ] **Pipeline Validation** - Ensure G-buffer pipeline creation succeeds

### passDeferredLighting (engine/src/render_system.cpp:~2466)
- [ ] **Full-Screen Setup** - Configure full-screen RHI pass
- [ ] **G-Buffer Binding** - Bind G-buffer textures (base color, normal, position, material)
- [ ] **UBO Binding** - Bind manager UBOs (lighting, rendering state)
- [ ] **Screen Quad** - Render full-screen quad to produce lit color output
- [ ] **Output Validation** - Verify litColor texture contains valid lighting result

### passRayIntegrator (engine/src/render_system.cpp:~2564)
- [ ] **Raytracer Setup** - Configure CPU raytracer with scene data
- [ ] **Texture Output** - Write raytraced pixels via RHI::updateTexture only
- [ ] **No Screen Quad** - Skip on-screen quad rendering in this pass
- [ ] **Sample Count** - Respect provided sampleCount and maxDepth parameters
- [ ] **Performance** - Ensure reasonable raytracing performance for test scenes

### passReadback (engine/src/render_system.cpp:~2360)
- [ ] **Source Selection** - Select correct source texture (raster vs ray output)
- [ ] **RHI Readback** - Call RHI::readback for headless texture extraction
- [ ] **Format Validation** - Ensure readback data format matches expectations
- [ ] **Error Handling** - Handle readback failures gracefully

## RHI Screen-Quad Utility
- [ ] **Vertex Buffer** - Create RHI vertex buffer for full-screen quad
- [ ] **Pipeline Support** - Support multiple pipelines (deferred lighting, post-processing)
- [ ] **Minimal Interface** - Keep interface lightweight and reusable
- [ ] **Resource Management** - Proper creation and cleanup of quad resources

## Validation and Testing
- [ ] **Visual Comparison** - Compare RenderGraph output vs legacy rendering
- [ ] **Black Frame Detection** - Ensure no black/empty frames in graph-driven paths
- [ ] **Pass Timing** - Verify reasonable performance for each pass
- [ ] **Memory Usage** - Check for memory leaks in pass execution
- [ ] **Error Logging** - Add appropriate error messages for pass failures

## Integration
- [ ] **Manager Integration** - Ensure passes properly use manager UBOs
- [ ] **RHI Consistency** - All GPU operations go through RHI abstraction
- [ ] **Context Usage** - Properly utilize PassContext for pass communication
- [ ] **Resource Cleanup** - Ensure temporary resources are cleaned up