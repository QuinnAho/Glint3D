# Pass Bridging Artifacts

This directory contains generated outputs and test cases for pass bridging implementation.

## Expected Artifacts

### Implementation Outputs
- **before_after_frames/** - Visual comparisons showing RenderGraph vs legacy output
- **pass_timing_validation.json** - Performance metrics for each implemented pass
- **rhi_screen_quad_utility.cpp** - Reusable full-screen quad implementation

### Test Cases
- **test_gbuffer_output.png** - Reference G-buffer rendering result
- **test_deferred_lighting.png** - Reference deferred lighting result
- **test_raytracer_integration.png** - Reference raytraced output
- **test_readback_validation.bin** - Binary data from successful readback

### Validation Tools
- **frame_comparison_tool.cpp** - Automated visual comparison utility
- **pass_performance_profiler.cpp** - Performance measurement tool
- **black_frame_detector.cpp** - Utility to detect rendering failures

## Usage

Generate artifacts by running pass bridging implementation with validation enabled:

```bash
# Generate visual comparisons
./glint --task-id pass_bridging_validation --ops examples/json-ops/sphere_basic.json --render artifacts/test_output.png

# Profile pass performance
./glint --task-id pass_bridging_perf --ops examples/json-ops/directional-light-test.json --profile artifacts/timing.json

# Validate readback functionality
./glint --task-id pass_bridging_readback --ops examples/json-ops/cube_basic.json --readback artifacts/readback.bin
```