# AI Configuration

Centralized configuration for Claude Code and other AI assistants working on Glint3D.

## File Structure

- **`claude.json`** - Global Claude Code configuration, constraints, and automation
- **`requirements_catalog.json`** - Reusable requirement definitions referenced by tasks
- **`README.md`** - This documentation

## Usage with Claude Code

### For Task-Based Development

```bash
# Point Claude at the current active task
cat ai/tasks/CURRENT  # -> FEAT-0241/PR1

# Claude reads this task capsule structure:
ai/tasks/FEAT-0241/
├── spec.yaml              # Machine-readable task specification
├── README.md              # Human-readable status and notes
├── passes.yaml            # Quality gate status
├── whitelist.txt          # Files this task is allowed to modify
├── overlay.ai.json        # Task-specific AI constraints
└── progress.ndjson        # Append-only event log
```

### Prompting Claude for Task Work

```
Read ai/tasks/CURRENT, then read ai/tasks/<ID>/spec.yaml and overlay.ai.json.
Only edit files listed in whitelist.txt. Update the AI-Editable sections
in README.md with your progress. Satisfy acceptance_criteria and 
must_requirements from the spec.
```

### Quality Gate Integration

Before making changes, Claude should check:
1. Files are within the task's `whitelist.txt`
2. Changes satisfy `must_requirements` from `ai/config/requirements_catalog.json`
3. Acceptance criteria from `spec.yaml` are addressed
4. Task-specific constraints from `overlay.ai.json` are respected

## Automation Integration

### Pre-commit Hooks
```bash
# Install hooks that enforce AI constraints
tools/setup_git_hooks.sh

# Hooks will check:
# - File whitelist compliance for active task
# - Architecture constraint validation
# - Security pattern scanning
```

### CI Integration
```yaml
# Example CI job using these configs
- name: Validate Architecture
  run: tools/check_architecture.py --config ai/config/claude.json
  
- name: Check Task Constraints  
  run: tools/validate_task_changes.py --task $(cat ai/tasks/CURRENT)
```

## Requirement System

Requirements are defined once in `requirements_catalog.json` and referenced by ID in task specs:

```yaml
# In ai/tasks/FEAT-XXXX/spec.yaml
must_requirements:
  - ARCH.NO_GL_OUTSIDE_RHI    # No direct OpenGL outside RHI layer
  - TEST.GOLDEN_SSIM          # Visual regression testing
  - PERF.FRAME_BUDGET         # 60fps performance target
```

This enables:
- **Consistency**: Same requirement definition across all tasks
- **Traceability**: Clear mapping from requirements to validation
- **Maintenance**: Update requirement once, applies to all tasks
- **Automation**: Machine-readable validation criteria

## Adding New Requirements

1. Add to appropriate category in `requirements_catalog.json`
2. Include validation criteria and rationale
3. Update automation scripts to check the requirement
4. Reference by ID in relevant task specs

## Task Lifecycle

1. **Create Task**: Add to `ai/tasks/index.json`, create task directory with spec.yaml
2. **Set Active**: Update `ai/tasks/CURRENT` to point to the task  
3. **Work**: Claude modifies files within whitelist, updates README.md progress
4. **Quality Gates**: Passes.yaml tracks completion of docs/tests/qa/perf gates
5. **Complete**: Mark task complete, update to next task or clear CURRENT

## Architecture Enforcement

The configuration enforces key architectural principles:

- **RHI Abstraction**: No direct OpenGL calls outside the RHI layer
- **Engine Isolation**: Core engine independent of UI frameworks  
- **Material Unification**: Single MaterialCore struct, no dual storage
- **Path Security**: File operations must validate against asset root
- **Performance Budgets**: Frame time and memory constraints
- **Cross-Platform**: Desktop/web feature parity

## Security Features

- **Path traversal protection**: File operations confined to asset root
- **Secret scanning**: Pre-commit hooks detect API keys, passwords
- **Whitelist enforcement**: Tasks can only modify approved files
- **Audit trail**: Progress logs provide change tracking

This configuration system enables reliable, constrained AI assistance while maintaining code quality and architectural integrity.