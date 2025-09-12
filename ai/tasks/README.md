# Task Management System

A lean, standardized "task capsule" system that provides both human-readable context and machine-readable constraints for AI-assisted development.

## Overview

This system organizes complex features into self-contained task capsules, each with:
- **Machine-readable specifications** (YAML/JSON) for AI constraints
- **Human-readable documentation** with progress tracking
- **File whitelists** to prevent scope drift
- **Quality gates** for systematic validation
- **Dependency management** for proper task ordering

## Directory Structure

```
ai/tasks/
├── README.md                   # This documentation
├── index.json                  # Task registry and requirements catalog
├── CURRENT                     # Active task pointer (e.g., "FEAT-0242/PR1")
├── FEAT-0241/                  # Example: SSR-T refraction feature
│   ├── spec.yaml               # Machine-readable task specification
│   ├── README.md               # Human status, notes, and progress
│   ├── passes.yaml             # Quality gate status (docs/tests/qa/perf)
│   ├── whitelist.txt           # Exact files this task may modify
│   ├── overlay.ai.json         # Task-specific AI constraints
│   ├── progress.ndjson         # Append-only event log
│   ├── patches/                # Proposed diffs (optional)
│   └── notes/                  # Development scratchpads (optional)
├── FEAT-0242/                  # RHI Abstraction Layer
├── FEAT-0243/                  # MaterialCore Unified BSDF
└── ...                         # Additional feature tasks
```

## Getting Started

### 1. Check Current Task Status
```bash
# See what's currently active
cat ai/tasks/CURRENT
# Output: FEAT-0242/PR1

# View task overview
cat ai/tasks/FEAT-0242/spec.yaml | head -20

# Check progress and blockers
cat ai/tasks/FEAT-0242/README.md
```

### 2. Working with AI Assistants

**For Claude Code / Codex:**
```
Read ai/tasks/CURRENT, then read the spec.yaml and overlay.ai.json for that task.
Only edit files listed in whitelist.txt. Update the AI-Editable sections 
in README.md with your progress. Ensure all acceptance_criteria and 
must_requirements are satisfied.
```

**The AI will:**
- Stay within the file whitelist boundaries
- Follow architectural constraints from overlay.ai.json
- Update progress in the README.md AI-Editable sections
- Log major events to progress.ndjson

### 3. Quality Gates System

Each task has standardized quality gates in `passes.yaml`:

- **docs**: Architecture documentation, API references updated
- **tests**: Unit tests, integration tests, golden image tests
- **qa**: Manual verification, cross-platform testing
- **perf**: Performance benchmarks, memory usage validation
- **determinism**: Consistent output across runs and platforms
- **parity**: Desktop/web feature equivalence
- **security**: Path validation, secret scanning

**Checking Quality Gates:**
```bash
# View current gate status
cat ai/tasks/FEAT-0242/passes.yaml

# Run automated checks
tests/scripts/run_unit_tests.sh
tests/scripts/run_golden_tests.sh
tools/check_architecture.py --task FEAT-0242
```

### 4. Task Lifecycle Management

**Creating New Tasks:**
1. Add entry to `ai/tasks/index.json` with dependencies
2. Create task directory with required files
3. Set priority and owner
4. Define acceptance criteria and file whitelist

**Working on Tasks:**
1. Update `ai/tasks/CURRENT` to point to active task
2. Work within the file whitelist boundaries
3. Update progress in README.md AI-Editable sections
4. Mark quality gates as pass/fail in passes.yaml

**Completing Tasks:**
1. Ensure all quality gates pass
2. Update task status to "completed" in index.json
3. Move to next task or clear CURRENT

## Task Dependencies

The system enforces proper dependency ordering:

```
FEAT-0242 (RHI Abstraction)
├── blocks: FEAT-0243, FEAT-0245, FEAT-0247
└── blocked_by: []

FEAT-0243 (MaterialCore) 
├── blocks: FEAT-0241, FEAT-0244, FEAT-0246
└── blocked_by: FEAT-0242

FEAT-0241 (SSR-T Refraction)
├── blocks: []
└── blocked_by: FEAT-0242, FEAT-0243
```

**Dependency Rules:**
- Work on tasks in dependency order (foundation first)
- Blocked tasks cannot start until dependencies complete
- CI validates dependency constraints

## Current Task Priorities

### Critical (Foundation)
- **FEAT-0242**: RHI Abstraction Layer - Enables all other architectural work

### High (Core Features)
- **FEAT-0243**: MaterialCore Unified BSDF - Eliminates dual material storage
- **FEAT-0244**: Hybrid Auto Mode - Intelligent pipeline selection  
- **FEAT-0246**: Deterministic Rendering - Golden test suite

### Medium (Quality of Life)
- **FEAT-0245**: RenderGraph Pass System - Modular rendering architecture
- **FEAT-0247**: Performance Profiling - Optimization and regression detection

## Architectural Issues Being Addressed

The tasks address key architectural problems identified in the codebase:

1. **Dual Material Storage** → FEAT-0243 (MaterialCore)
   - Problem: SceneObject has both PBR and legacy Material fields
   - Impact: Conversion drift between raster/ray pipelines

2. **Pipeline Fragmentation** → FEAT-0241, FEAT-0244 (SSR-T + Auto Mode)
   - Problem: Users must know when to use --raytrace
   - Impact: Poor UX, missed glass effects

3. **Backend Lock-in** → FEAT-0242 (RHI Abstraction)
   - Problem: Direct OpenGL calls scattered throughout engine
   - Impact: Cannot add Vulkan/WebGPU without major refactoring

4. **No Pass System** → FEAT-0245 (RenderGraph)
   - Problem: Interleaved rendering logic, hard to extend
   - Impact: Adding new techniques requires major surgery

5. **Testing Gaps** → FEAT-0246 (Golden Tests)
   - Problem: Limited regression testing, floating-point CI failures
   - Impact: Visual regressions not caught

6. **Performance Blindness** → FEAT-0247 (Profiling)
   - Problem: No performance monitoring or budget enforcement
   - Impact: Bottlenecks unknown, regressions detected by users

## File Organization Rules

### Whitelist Enforcement
Each task has a strict file whitelist that defines exactly which files it can modify:

```txt
# Example whitelist for RHI task
engine/include/glint3d/rhi*.h
engine/src/rhi/**
engine/src/render_system.cpp
tests/unit/rhi_test.cpp
docs/rhi_architecture.md
ai/tasks/FEAT-0242/**
```

**Benefits:**
- Prevents scope creep and unrelated changes
- Enables parallel development on different tasks
- Simplifies code review and conflict resolution
- Provides clear boundaries for AI assistance

### Documentation Standards
Each task maintains documentation at multiple levels:

- **spec.yaml**: Machine-readable source of truth
- **README.md**: Human-readable status and notes
- **Code Documentation**: Updated API references and architecture docs
- **Test Documentation**: Test plans and validation criteria

## Integration with Development Workflow

### Git Integration
- **Branches**: Each PR maps to a plan item (e.g., `feat/242-rhi-interface-pr1`)
- **Commits**: Reference task IDs and PR numbers
- **Reviews**: Whitelist violations automatically flagged

### CI/CD Integration
Quality gates are enforced through automated checks:

```yaml
# Example CI job
- name: Validate Task Constraints
  run: |
    TASK=$(cat ai/tasks/CURRENT)
    tools/validate_task_changes.py --task $TASK
    tools/check_architecture.py --config ai-config/claude.json
```

### Performance Monitoring
Tasks include performance budgets that are automatically validated:

- **Frame Time**: < 16ms at 1080p for interactive features
- **Memory Usage**: Defined per-task overhead limits  
- **Build Time**: Shader compilation and link time budgets

## Best Practices

### For Human Developers

1. **Start with Dependencies**: Work on blocking tasks first
2. **Stay in Scope**: Only modify files in the whitelist
3. **Update Progress**: Keep README.md current with status
4. **Validate Gates**: Ensure quality gates pass before completion
5. **Document Decisions**: Log architectural choices in progress.ndjson

### For AI Assistance

1. **Read Current Task**: Always check `ai/tasks/CURRENT` first
2. **Follow Constraints**: Respect whitelist and overlay.ai.json rules
3. **Update Status**: Modify AI-Editable sections in README.md
4. **Satisfy Requirements**: Address all must_requirements from spec.yaml
5. **Log Progress**: Append events to progress.ndjson for traceability

### For Code Review

1. **Check Whitelist**: Verify changes are within approved files
2. **Validate Requirements**: Ensure must_requirements are met
3. **Test Quality Gates**: All passes.yaml criteria should be satisfied
4. **Review Architecture**: Changes align with target_architecture
5. **Update Documentation**: Architecture docs reflect changes

## Advanced Features

### Event Logging
The progress.ndjson file provides an append-only audit trail:

```json
{"ts":"2025-09-11T14:03:12Z","who":"human","kind":"task_created","task":"FEAT-0242","msg":"RHI abstraction layer task created"}
{"ts":"2025-09-11T14:15:23Z","who":"ai","kind":"pr_started","task":"FEAT-0242","pr":"PR1","msg":"Started RHI interface definition"}
{"ts":"2025-09-11T15:42:18Z","who":"ai","kind":"blocker","task":"FEAT-0242","msg":"WebGL2 extension compatibility issue"}
```

### Requirements Catalog
The `ai/tasks/index.json` includes a centralized requirements catalog that tasks reference by ID:

```yaml
# In task spec.yaml
must_requirements:
  - ARCH.NO_GL_OUTSIDE_RHI    # No direct OpenGL outside RHI layer
  - TEST.GOLDEN_SSIM          # Visual regression testing
  - PERF.FRAME_BUDGET         # 60fps performance target
```

This enables:
- **Consistency**: Same requirement across all tasks
- **Traceability**: Clear validation criteria
- **Automation**: Machine-readable constraints for CI

### Cross-Platform Considerations
Tasks account for platform differences:

- **Desktop**: Full OpenGL feature set, native file system
- **Web**: WebGL2 limitations, Emscripten constraints
- **Mobile**: Future consideration for touch interfaces

The quality gates include cross-platform validation to ensure feature parity where possible and graceful degradation where necessary.

## Troubleshooting

### Common Issues

**"Task blocked by dependencies"**
- Check `ai/tasks/index.json` for dependency graph
- Complete blocking tasks first
- Verify dependency task status is "completed"

**"File not in whitelist"** 
- Review `ai/tasks/TASK-ID/whitelist.txt`
- Request whitelist expansion if legitimately needed
- Consider if change belongs in different task

**"Quality gate failing"**
- Check specific criteria in `passes.yaml`
- Run automated validation scripts
- Update documentation or tests as needed

**"AI constraints violated"**
- Review `overlay.ai.json` for task-specific rules
- Check global constraints in `ai-config/claude.json`
- Ensure architectural principles are followed

### Getting Help

1. **Documentation**: Check task README.md and spec.yaml
2. **Architecture**: Review CLAUDE.md for system context
3. **Requirements**: See `ai-config/requirements_catalog.json`
4. **Issues**: Check `ai/tasks/index.json` architectural_issues section

This task management system enables systematic, AI-assisted development while maintaining code quality, architectural integrity, and clear progress tracking.