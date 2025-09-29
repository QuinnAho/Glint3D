# Determinism Hooks Checklist

## PRNG Infrastructure

### Thread-Safe PRNG Extension
- [ ] **Global PRNG Manager** - Extend SeededRng for multi-threaded access
- [ ] **Per-Pass PRNG State** - Scope PRNG by {task_id, frame, pass}
- [ ] **Thread Safety** - Add mutex protection for concurrent access
- [ ] **State Isolation** - Ensure passes don't interfere with each other's randomness

### Sampling Audit and Migration
- [ ] **Raytracer Sampling** - Verify existing SeededRng usage is comprehensive
- [ ] **Material Sampling** - Identify any random material property generation
- [ ] **Animation Sampling** - Check for random animation or interpolation
- [ ] **Shader Sampling** - Audit shader uniforms for random values

## Time-Based Behavior Elimination

### Time Source Audit
- [ ] **Wall Clock Usage** - Find all std::chrono or time() calls in render pipeline
- [ ] **Frame Timing** - Replace frame delta time with deterministic stepping
- [ ] **Animation Timing** - Use frame counter instead of wall clock time
- [ ] **Profiling Timing** - Separate measurement timing from execution timing

### Deterministic Replacements
- [ ] **Frame Counter** - Use deterministic frame index for time-based effects
- [ ] **Fixed Delta Time** - Use constant time step for animations
- [ ] **Seeded Animations** - Base all time-varying effects on PRNG state
- [ ] **Deterministic Physics** - Ensure any physics uses fixed time step

## Byte-Stability Framework

### Output Validation
- [ ] **Pixel-Perfect Comparison** - Byte-level comparison of render outputs
- [ ] **Hash Verification** - SHA-256 hash comparison for large outputs
- [ ] **Floating Point Precision** - Handle FP determinism across platforms
- [ ] **Memory Layout** - Ensure consistent memory layout for structures

### Test Infrastructure
- [ ] **Reference Output Generation** - Create known-good reference renders
- [ ] **Determinism Test Suite** - Automated tests for byte-stability
- [ ] **Cross-Platform Testing** - Verify determinism across Windows/Linux/macOS
- [ ] **Regression Detection** - Detect when determinism breaks

## Global Seed Propagation

### Seed Management
- [ ] **Task ID Integration** - Use task_id as primary seed source
- [ ] **Frame Scoping** - Derive frame-specific seeds from task_id + frame
- [ ] **Pass Scoping** - Derive pass-specific seeds from frame + pass_name
- [ ] **Hierarchy Validation** - Ensure seed hierarchy is consistent

### API Integration
- [ ] **CLI Seed Injection** - Accept --seed parameter for task execution
- [ ] **JSON-Ops Seed** - Include seed in operation metadata
- [ ] **NDJSON Seed Tracking** - Emit seed values in telemetry events
- [ ] **State Seed Storage** - Include seed in state snapshots

## Validation and Testing

### Determinism Tests
- [ ] **Single-Run Consistency** - Same input produces same output
- [ ] **Multi-Run Consistency** - Multiple runs with same seed match
- [ ] **Cross-Session Consistency** - Results consistent across app restarts
- [ ] **Incremental Consistency** - Deterministic with state changes

### Performance Impact
- [ ] **PRNG Overhead** - Measure performance impact of deterministic PRNG
- [ ] **Memory Overhead** - Check memory usage of seed state storage
- [ ] **Threading Impact** - Verify thread synchronization doesn't hurt performance
- [ ] **Optimization** - Optimize hot paths while maintaining determinism