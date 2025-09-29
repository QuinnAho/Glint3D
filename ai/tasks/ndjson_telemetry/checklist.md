# NDJSON Telemetry Checklist

## Event Schema Definition

### Core Event Types
- [ ] **pass_start** - { ts, task_id, graph_pass, rhi_backend, frame, seed }
- [ ] **pass_end** - { ts, task_id, graph_pass, ms_cpu, draws, counters }
- [ ] **resource_alloc** - { ts, task_id, resource_type, size_bytes, handle_id }
- [ ] **cache_hit/miss** - { ts, task_id, asset_hash, cache_type }
- [ ] **quality_metric** - { ts, task_id, metric_type, value, reference }
- [ ] **warning/error** - { ts, task_id, level, message, context }

### Required Fields Standard
- [ ] **Timestamp** - ISO 8601 timestamp with microsecond precision
- [ ] **Task ID** - Unique identifier for task execution
- [ ] **Frame Context** - Frame number, seed, scene_id, commit hash
- [ ] **Pass Context** - Current render pass, RHI backend info
- [ ] **Counters** - Draw calls, triangles, texture memory, etc.

## Event Emission Infrastructure

### Buffered Writer System
- [ ] **Ring Buffer** - In-memory ring buffer for low-latency event storage
- [ ] **Background Flush** - Background thread for writing to disk
- [ ] **Buffer Overflow** - Handle buffer overflow gracefully
- [ ] **Memory Limits** - Configurable memory limits for telemetry

### File Output System
- [ ] **NDJSON Format** - One JSON object per line format
- [ ] **File Rotation** - Rotate log files based on size/time
- [ ] **Compression** - Optional gzip compression for archived logs
- [ ] **Atomic Writes** - Ensure log file consistency

### Headless Mode Integration
- [ ] **Opt-in File Sink** - Enable file output only in headless mode
- [ ] **CLI Configuration** - Command line options for telemetry output
- [ ] **Performance Mode** - Disable telemetry for performance testing
- [ ] **Debug Mode** - Verbose telemetry for debugging

## Performance Requirements

### Low Overhead Design
- [ ] **<1% Frame Time** - Telemetry overhead under 1% of total frame time
- [ ] **Lock-Free Writes** - Lock-free event emission where possible
- [ ] **Batch Operations** - Batch multiple events for efficiency
- [ ] **Hot Path Optimization** - Optimize critical rendering path events

### Memory Management
- [ ] **Fixed Allocation** - Pre-allocate buffers to avoid runtime allocation
- [ ] **Event Pooling** - Reuse event objects to reduce GC pressure
- [ ] **String Interning** - Intern repeated strings (pass names, etc.)
- [ ] **Memory Monitoring** - Track telemetry memory usage

## Integration Points

### Pass Timing Integration
- [ ] **PassTiming Migration** - Convert existing PassTiming to NDJSON events
- [ ] **Microsecond Precision** - High-precision timing for performance analysis
- [ ] **Pass Hierarchy** - Track sub-pass relationships
- [ ] **GPU Timing** - Include GPU timestamps where available

### Resource Tracking
- [ ] **Texture Allocation** - Track texture creation/destruction
- [ ] **Buffer Allocation** - Track vertex/index buffer operations
- [ ] **Memory Usage** - Track VRAM and system memory usage
- [ ] **Resource Lifetime** - Track resource creation to destruction

### State Change Events
- [ ] **Manager State Changes** - Emit events when manager state changes
- [ ] **Pipeline Changes** - Track render pipeline switches
- [ ] **Render Target Changes** - Track render target binding
- [ ] **Shader Changes** - Track shader program switches

## Automated Analysis Support

### Parsing Tools
- [ ] **NDJSON Parser** - Robust parser for telemetry logs
- [ ] **Event Filtering** - Filter events by type, task_id, time range
- [ ] **Aggregation Tools** - Calculate statistics from event streams
- [ ] **Validation Tools** - Validate event schema compliance

### Performance Analysis
- [ ] **Frame Profiling** - Per-frame performance breakdown
- [ ] **Pass Profiling** - Per-pass timing analysis
- [ ] **Resource Analysis** - Memory usage patterns
- [ ] **Trend Analysis** - Performance trends over time

### Quality Metrics
- [ ] **Visual Quality** - PSNR, SSIM integration with image comparison
- [ ] **Performance Quality** - Frame time consistency, stutter detection
- [ ] **Resource Quality** - Memory efficiency, cache hit rates
- [ ] **Determinism Quality** - Byte-stability validation events