# Render System Task Management

## AI Agent Instructions

### Primary Directive

You are an AI agent working on the Glint3D render system decomposition project. Your role is to execute tasks systematically while maintaining high code quality and proper documentation.

### Task Execution Protocol

1. **Always start by reading the current index**:
   ```
   Read ai/tasks/current_index.json to understand:
   - Which task is currently active
   - Current sub-issue focus
   - Blocking dependencies
   - Critical path status
   ```

2. **Read all relevant task files**:
   ```
   For the active task, read ALL files in ai/tasks/{active_task}/:
   - task.json (specification, inputs, outputs, acceptance criteria)
   - checklist.md (atomic steps to execute)
   - progress.ndjson (current status and history)
   - artifacts/ (any existing outputs or test cases)
   ```

3. **Execute work with best practices**:
   - Follow the atomic steps in checklist.md exactly
   - Create code that is modular, testable, and well-documented
   - Use existing patterns and conventions from the codebase
   - Implement incrementally with validation at each step
   - Add comprehensive error handling and logging
   - Write tests for all new functionality

4. **Update documentation after completion**:
   - Mark completed checklist items with [x]
   - Append progress events to progress.ndjson with timestamp
   - Update current_index.json with new status
   - Generate artifacts in the artifacts/ directory
   - Update task.json status if task is complete
   - Update the main decomposition document if phase changes

### Best Practices Requirements

#### Code Quality
- **Modularity**: Write small, focused functions with single responsibilities
- **Error Handling**: Add comprehensive error checking and graceful failure modes
- **Performance**: Consider performance implications, especially in hot paths
- **Memory Safety**: Ensure proper resource cleanup and no leaks
- **Thread Safety**: Use appropriate synchronization for multi-threaded code

#### Documentation Standards
- **API Documentation**: Document all public interfaces with clear parameter descriptions
- **Implementation Notes**: Explain complex algorithms or design decisions
- **Usage Examples**: Provide examples for new APIs or utilities
- **Migration Guides**: Document any breaking changes or required updates

### Progress Tracking Format

When updating progress.ndjson, use this format:
```json
{"ts": "2024-12-28T12:34:56Z", "task_id": "task_name", "event": "step_completed", "step": "specific_checklist_item", "status": "success", "details": "brief_description", "agent": "agent_id"}
```

### Completion Criteria

A task is complete when:
- All checklist items are marked as completed
- All acceptance criteria from task.json are met
- All artifacts are generated and validated
- Progress is properly documented
- Dependencies for next task are satisfied

---

## Task Module Structure Example

Each task module follows this canonical structure:

```
ai/tasks/{task_name}/
├── task.json              # Complete task specification
├── checklist.md           # Atomic execution steps
├── progress.ndjson        # Event tracking history
├── artifacts/             # Generated outputs
│   ├── README.md          # Description of expected artifacts
│   ├── code/              # Generated code files
│   ├── tests/             # Test files and results
│   ├── documentation/     # Generated documentation
│   └── validation/        # Validation results and reports
└── [optional files]       # Task-specific additional files
```

### task.json Schema
```json
{
  "id": "unique_task_identifier",
  "title": "Human Readable Task Title",
  "owner": "responsible_team",
  "status": "pending|in_progress|completed|blocked",
  "inputs": {
    "files": ["list", "of", "input", "files"],
    "schema_version": "required_schema_version"
  },
  "outputs": {
    "artifacts": ["expected", "output", "files"],
    "events": ["telemetry", "event", "types"]
  },
  "acceptance": [
    "Specific acceptance criterion 1",
    "Specific acceptance criterion 2",
    "Agent-testable requirement 3"
  ],
  "safety": {
    "rate_limits": ["safety", "constraints"],
    "sandbox": true|false
  },
  "dependencies": ["prerequisite", "tasks"],
  "seed_policy": "determinism_requirements"
}
```

### checklist.md Format
```markdown
# Task Name Checklist

## Phase 1: Description
- [ ] **Step Name** - Detailed description of atomic step
- [ ] **Another Step** - What needs to be accomplished
- [x] **Completed Step** - Example of completed item

## Phase 2: Next Phase
- [ ] **Validation Step** - How to verify the work
- [ ] **Documentation Step** - What documentation to update
```

### progress.ndjson Format
```
{"ts": "2024-12-28T00:00:00Z", "task_id": "task_name", "event": "task_started", "phase": "initial", "status": "in_progress"}
{"ts": "2024-12-28T01:30:00Z", "task_id": "task_name", "event": "step_completed", "step": "specific_step", "status": "success"}
{"ts": "2024-12-28T02:45:00Z", "task_id": "task_name", "event": "phase_completed", "phase": "initial", "next_phase": "validation"}
```

---

## Current Task Status

**Active Task**: Check `current_index.json` for the most up-to-date information.

**Quick Status Check**:
```bash
# View current focus
cat ai/tasks/current_index.json | jq '.current_focus'

# View blocking issues
cat ai/tasks/current_index.json | jq '.blockers'

# View critical path
cat ai/tasks/current_index.json | jq '.critical_path'
```

---

## Emergency Procedures

### If a Task is Blocked
1. Update current_index.json with blocker details
2. Add blocker event to progress.ndjson
3. Identify preparatory work that can be done
4. Switch to preparation work or next unblocked task

### If Acceptance Criteria Cannot Be Met
1. Document the issue in progress.ndjson
2. Update task.json with revised acceptance criteria if needed
3. Escalate to project maintainers
4. Do not mark task as complete

### If Critical Path Changes
1. Update current_index.json with new critical path
2. Notify all stakeholders
3. Update main decomposition document
4. Reassess all task dependencies