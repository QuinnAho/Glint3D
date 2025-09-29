# JSON-Ops Runner Checklist

## Minimal Schema Definition (v0.1)

### Core Operation Types
- [ ] **SceneOp** - { op: "load|transform|duplicate", target: object_id, params: {...} }
- [ ] **MaterialOp** - { op: "set_pbr", target: object_id, baseColor, metallic, roughness, ... }
- [ ] **CameraOp** - { op: "set_camera", position: [x,y,z], rotation: [x,y,z], fov: degrees }
- [ ] **PassScheduleOp** - { op: "set_passes", passes: ["GBuffer", "DeferredLighting"], config: {...} }
- [ ] **ReadbackOp** - { op: "readback", format: "png|exr|raw", path: "output.png" }

### Schema Validation
- [ ] **JSON Schema File** - Define formal JSON schema for v0.1 operations
- [ ] **Version Header** - All operation files must include { "version": "0.1" }
- [ ] **Strict Validation** - Reject operations that don't match schema exactly
- [ ] **Error Messages** - Clear error messages for schema violations

## CLI Job Runner

### Command Line Interface
- [ ] **--task-id Parameter** - Required task ID for all executions
- [ ] **--ops-file Parameter** - JSON operations file to execute
- [ ] **--output-dir Parameter** - Directory for output artifacts
- [ ] **--telemetry-file Parameter** - NDJSON telemetry output file

### Execution Pipeline
- [ ] **Schema Validation** - Validate operations file before execution
- [ ] **State Initialization** - Initialize clean render state
- [ ] **Deterministic Setup** - Set global seed from task_id
- [ ] **Operation Execution** - Execute operations in sequence
- [ ] **Output Generation** - Generate all requested outputs

### Pure Operations (No Side Effects)
- [ ] **State Transitions** - Operations only modify explicit state
- [ ] **No Global State** - Operations don't depend on hidden global state
- [ ] **Idempotent Operations** - Operations can be safely repeated
- [ ] **Rollback Support** - Failed operations don't leave partial state

## Deterministic Execution

### Seed Management
- [ ] **Task ID Seeding** - Use task_id as primary seed source
- [ ] **Operation Seeding** - Each operation gets deterministic sub-seed
- [ ] **Frame Seeding** - Animation frames use deterministic timing
- [ ] **PRNG Isolation** - Operations don't interfere with each other's randomness

### State Isolation
- [ ] **Clean Initialization** - Start each job with fresh state
- [ ] **State Snapshots** - Take state snapshots before operations
- [ ] **State Validation** - Validate state consistency after operations
- [ ] **Error Isolation** - Failed operations don't corrupt subsequent ones

## Integration with Manager System

### State Management Integration
- [ ] **Manager State APIs** - Use getState/applyState for all manager operations
- [ ] **Atomic Transactions** - Use transaction system for complex operations
- [ ] **State Validation** - Validate all state changes through manager APIs
- [ ] **JSON Serialization** - Use manager JSON serialization for state ops

### Telemetry Integration
- [ ] **Operation Events** - Emit NDJSON events for each operation
- [ ] **Timing Events** - Track operation execution time
- [ ] **State Events** - Emit events for state changes
- [ ] **Error Events** - Emit events for operation failures

## Output Validation

### Reproducible Outputs
- [ ] **Byte-Stable Images** - Same operations produce identical images
- [ ] **Hash Verification** - Verify output hashes match references
- [ ] **Cross-Platform** - Same results on Windows/Linux/macOS
- [ ] **Version Consistency** - Same results across compatible versions

### Reference Testing
- [ ] **Golden Images** - Maintain reference images for standard operations
- [ ] **Regression Detection** - Detect when outputs change unexpectedly
- [ ] **Visual Diff Tools** - Tools to compare rendered outputs
- [ ] **Performance Baselines** - Track performance regression

## Error Handling and Safety

### Operation Safety
- [ ] **Timeout Protection** - Kill operations that run too long
- [ ] **Memory Limits** - Prevent operations from using excessive memory
- [ ] **Resource Limits** - Limit texture sizes, polygon counts, etc.
- [ ] **Malformed Input** - Handle malformed operations gracefully

### Error Recovery
- [ ] **Partial Failure** - Continue with remaining operations after failure
- [ ] **Error Reporting** - Detailed error messages with operation context
- [ ] **Diagnostic Output** - Generate diagnostic information on failure
- [ ] **State Cleanup** - Clean up resources after failures

## Agent Testing Framework

### Automated Testing
- [ ] **Operation Generation** - Generate test operations programmatically
- [ ] **Result Validation** - Automatically validate operation results
- [ ] **Performance Testing** - Measure operation execution performance
- [ ] **Stress Testing** - Test with large numbers of operations

### Test Infrastructure
- [ ] **Test Operation Library** - Collection of standard test operations
- [ ] **Reference Results** - Known-good results for test operations
- [ ] **Comparison Tools** - Automated comparison of results
- [ ] **CI Integration** - Integration with continuous integration systems