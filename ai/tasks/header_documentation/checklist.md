# Header Documentation Task Checklist

## Phase 1: Audit & Planning
- [x] **Audit all header files** - Scan all 64 header files in engine/include/ and categorize by documentation status
- [x] **Generate header audit report** - Create artifacts/documentation/header_audit.md with list of headers and current documentation state
- [x] **Identify priority headers** - Mark critical public API headers (glint3d/*, managers/*) as high priority

## Phase 2: Core Public API Headers (glint3d/*)
- [ ] **Document glint3d/rhi.h** - Add Machine Summary Block, Doxygen @file, and API documentation
- [ ] **Document glint3d/rhi_types.h** - Add Machine Summary Block, Doxygen @file, and type documentation
- [ ] **Document glint3d/material_core.h** - Add Machine Summary Block, Doxygen @file, and struct documentation
- [ ] **Document glint3d/uniform_blocks.h** - Add Machine Summary Block, Doxygen @file, and uniform documentation

## Phase 3: Manager Headers (managers/*)
- [ ] **Document managers/camera_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document managers/lighting_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document managers/material_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document managers/pipeline_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document managers/rendering_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document managers/scene_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document managers/transform_manager.h** - Add Machine Summary Block and Doxygen documentation

## Phase 4: Rendering System Headers
- [ ] **Document render_system.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document render_pass.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document render_mode_selector.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document render_settings.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document render_utils.h** - Add Machine Summary Block and Doxygen documentation

## Phase 5: RHI Backend Headers (rhi/*)
- [ ] **Document rhi/rhi_gl.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document rhi/rhi_null.h** - Add Machine Summary Block and Doxygen documentation

## Phase 6: Scene & Asset System Headers
- [ ] **Document scene_manager.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document importer.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document importer_registry.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document mesh_loader.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document texture.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document texture_cache.h** - Add Machine Summary Block and Doxygen documentation

## Phase 7: Ray Tracing Headers
- [ ] **Document raytracer.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document raytracer_lighting.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document bvh_node.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document ray.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document ray_utils.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document intersection.h** - Add Machine Summary Block and Doxygen documentation
- [ ] **Document triangle.h** - Add Machine Summary Block and Doxygen documentation

## Phase 8: Remaining Headers (Utilities, UI, etc.)
- [ ] **Document remaining 30+ headers** - Add Machine Summary Blocks and Doxygen documentation to all remaining headers in engine/include/

## Phase 9: Validation & Verification
- [ ] **Run documentation verification checklist** - Verify all headers pass FOR_MACHINES.md §0C checklist
- [ ] **Generate documentation coverage report** - Create artifacts/documentation/documentation_coverage_report.md
- [ ] **Regenerate Doxygen documentation** - Run docs/generate-docs.sh and verify no errors
- [ ] **Create validation artifact** - Generate artifacts/validation/documentation_validation.json with results

## Phase 10: Final Review
- [ ] **Review all Machine Summary Blocks** - Ensure NDJSON format is valid and complete
- [ ] **Review all Doxygen comments** - Ensure @file, @brief, @param, @return are present
- [ ] **Verify acceptance criteria** - Confirm all 9 acceptance criteria are met
- [ ] **Mark task complete** - Update task.json status and log task_completed event
