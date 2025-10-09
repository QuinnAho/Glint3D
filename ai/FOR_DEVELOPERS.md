# Task Module System - Developer Guide

## What is a Task Module?

A **task module** is a structured directory containing all the information needed to plan, execute, track, and validate a specific engineering task. Task modules are designed to facilitate collaboration between human developers and AI assistants by providing a shared, machine-readable representation of work to be done.

Think of a task module as a "work capsule" that contains:
- **Specification**: What needs to be done and why
- **Execution plan**: Step-by-step instructions
- **Progress tracking**: Real-time status updates
- **Validation criteria**: How to know when it's done
- **Artifacts**: Generated outputs and documentation

## Why Use Task Modules?

### Benefits for Developers
- **Clear scope**: Every task has explicit inputs, outputs, and acceptance criteria
- **Progress visibility**: Real-time tracking via NDJSON event logs
- **Context preservation**: All task-related information in one place
- **Handoff ready**: Easy to pause, resume, or transfer work between team members or AI assistants
- **Audit trail**: Complete history of what was done and when

### Benefits for AI Collaboration
- **Deterministic execution**: AI assistants know exactly what to do and in what order
- **Quality gates**: Explicit acceptance criteria prevent incomplete work
- **Context loading**: AI can pick up where previous work left off
- **Minimal hallucination**: Structured format reduces ambiguity

## Directory Structure

Each task module follows this canonical structure:

```
ai/tasks/{task_name}/
├── task.json              # Complete task specification (REQUIRED)
├── checklist.md           # Atomic execution steps (REQUIRED)
├── progress.ndjson        # Event tracking history (REQUIRED)
├── artifacts/             # Generated outputs (REQUIRED)
│   ├── README.md          # Description of expected artifacts
│   ├── code/              # Generated code files
│   ├── tests/             # Test files and results
│   ├── documentation/     # Generated documentation
│   └── validation/        # Validation results and reports
└── [optional files]       # Task-specific files (coverage.md, mapping.md, etc.)
```

## Core Files Explained

### 1. task.json - Task Specification

The `task.json` file is the single source of truth for what needs to be accomplished.

**Schema:**
```json
{
  "id": "unique_task_identifier",
  "title": "Human Readable Task Title",
  "owner": "responsible_team",
  "status": "pending|in_progress|completed|blocked",
  "inputs": {
    "files": ["list/of/input/files.cpp", "with/line/ranges.h:100-200"],
    "schema_version": "required_schema_version"
  },
  "outputs": {
    "artifacts": ["expected/output/files", "documentation.md"],
    "events": ["telemetry_event_types", "logged_to_progress_ndjson"]
  },
  "acceptance": [
    "Specific acceptance criterion 1",
    "Specific acceptance criterion 2",
    "Agent-testable requirement 3"
  ],
  "safety": {
    "rate_limits": ["max_build_time_10min", "max_file_size_1mb"],
    "sandbox": true
  },
  "dependencies": ["prerequisite_task_1", "prerequisite_task_2"],
  "seed_policy": "deterministic|non_deterministic"
}
```

**Key Fields:**
- **id**: Unique identifier matching the directory name
- **status**: Current state (pending, in_progress, completed, blocked)
- **inputs.files**: Specific files and line ranges that will be modified
- **outputs.artifacts**: Expected deliverables
- **acceptance**: Criteria that must be met for the task to be considered complete
- **dependencies**: Other tasks that must be completed first

### 2. checklist.md - Execution Plan

The checklist breaks down the task into atomic, actionable steps. Each step should be small enough to complete in one sitting (15-60 minutes).

**Format:**
```markdown
# Task Name Checklist

## Phase 1: Setup & Analysis
- [ ] **Read existing implementation** - Review current code in engine/src/foo.cpp
- [ ] **Identify dependencies** - Document what systems are affected
- [x] **Create backup branch** - git checkout -b task/feature-name

## Phase 2: Implementation
- [ ] **Migrate function X** - Convert foo() to use RHI API
- [ ] **Add unit tests** - Test foo() with valid/invalid inputs
- [ ] **Update documentation** - Document new API in header comments

## Phase 3: Validation
- [ ] **Build verification** - cmake --build builds/desktop/cmake -j
- [ ] **Run tests** - ./run_tests.sh
- [ ] **Visual regression** - Compare golden images
```

**Best Practices:**
- Use `[x]` to mark completed items
- Include **bold task names** for easy scanning
- Provide specific commands or file paths where relevant
- Group related steps into logical phases
- Keep steps atomic (one clear action per checkbox)

### 3. progress.ndjson - Event Log

The progress file is a **newline-delimited JSON** log that tracks every significant event during task execution. Each line is a complete JSON object.

**Format:**
```json
{"ts": "2024-12-28T00:00:00Z", "task_id": "task_name", "event": "task_started", "phase": "initial", "status": "in_progress", "agent": "human"}
{"ts": "2024-12-28T01:30:00Z", "task_id": "task_name", "event": "step_completed", "step": "Read existing implementation", "status": "success", "details": "Reviewed 250 lines", "agent": "claude"}
{"ts": "2024-12-28T02:15:00Z", "task_id": "task_name", "event": "build_failed", "error": "undefined reference to RHI::clear()", "agent": "claude"}
{"ts": "2024-12-28T02:45:00Z", "task_id": "task_name", "event": "phase_completed", "phase": "implementation", "next_phase": "validation", "agent": "claude"}
```

**Standard Event Types:**
- `task_started` - Task begins
- `step_completed` - Checklist item finished
- `phase_completed` - Major section done
- `build_succeeded` / `build_failed` - Compilation results
- `test_passed` / `test_failed` - Test results
- `blocker_encountered` - Something prevents progress
- `task_completed` - All acceptance criteria met

**Required Fields:**
- `ts`: ISO 8601 timestamp
- `task_id`: Matches directory name
- `event`: Event type from standard set
- `agent`: Who/what performed the action (human, claude, github_ci, etc.)

**Optional Fields:**
- `status`: success, failure, warning
- `details`: Human-readable description
- `step`: References specific checklist item
- `phase`: Current phase name
- `error`: Error message if applicable

### 4. artifacts/ - Generated Outputs

The artifacts directory contains all deliverables produced during task execution.

**Standard Subdirectories:**
- `code/` - Generated or modified code files
- `tests/` - Test files and test results
- `documentation/` - Generated docs, API references, migration guides
- `validation/` - Performance reports, coverage reports, benchmark results

**artifacts/README.md** should list:
- Expected artifacts and their purposes
- Naming conventions
- Validation requirements

## How to Prompt AI to Use Task Modules

### Starting a New Task

```
I want to work on the "pass_bridging" task. Please:
1. Read ai/tasks/current_index.json to confirm it's the active task
2. Read all files in ai/tasks/pass_bridging/
3. Execute the checklist step by step
4. Update progress.ndjson after each step
5. Generate artifacts in the artifacts/ directory
```

### Resuming Work on an Existing Task

```
Continue working on the current task. Please:
1. Read ai/tasks/current_index.json to see what's active
2. Check progress.ndjson to see what's been completed
3. Review checklist.md to see what's left
4. Continue from the next incomplete step
```

### Checking Task Status

```
What's the status of the render system tasks?
Please read ai/tasks/current_index.json and summarize:
- What task is currently active
- What phase we're in
- What's blocking progress (if anything)
- What's next on the critical path
```

### Switching Tasks

```
I need to switch to a different task because of a blocker.
Please:
1. Update ai/tasks/current_index.json with the blocker details
2. Log the blocker event to the current task's progress.ndjson
3. Switch active_task to "new_task_name"
4. Begin the new task following the standard protocol
```

## How to Create a New Task Module

### Step 1: Create Directory Structure

```bash
cd ai/tasks
mkdir my_new_task
cd my_new_task
mkdir -p artifacts/{code,tests,documentation,validation}
```

### Step 2: Create task.json

```json
{
  "id": "my_new_task",
  "title": "Implement Feature X",
  "owner": "your_team",
  "status": "pending",
  "inputs": {
    "files": ["engine/src/feature.cpp", "engine/include/feature.h"],
    "schema_version": "v1"
  },
  "outputs": {
    "artifacts": ["artifacts/code/feature_impl.cpp", "artifacts/tests/feature_test.cpp"],
    "events": ["feature_integrated", "tests_passed"]
  },
  "acceptance": [
    "Feature compiles without warnings",
    "All unit tests pass",
    "Performance meets budget (<5ms per operation)",
    "Documentation updated in header file"
  ],
  "safety": {
    "rate_limits": ["max_memory_usage_500mb"],
    "sandbox": false
  },
  "dependencies": ["prerequisite_task"],
  "seed_policy": "deterministic"
}
```

### Step 3: Create checklist.md

Break down the task into phases and atomic steps:

```markdown
# My New Task Checklist

## Phase 1: Research & Design
- [ ] **Review existing code** - Read engine/src/feature.cpp
- [ ] **Identify dependencies** - List affected systems
- [ ] **Design API** - Sketch function signatures

## Phase 2: Implementation
- [ ] **Implement core logic** - Write Feature::doWork()
- [ ] **Add error handling** - Handle edge cases
- [ ] **Write unit tests** - Cover all code paths

## Phase 3: Integration & Validation
- [ ] **Build verification** - cmake --build builds/desktop/cmake
- [ ] **Run tests** - ctest --test-dir builds/desktop/cmake
- [ ] **Update documentation** - Add API docs to header
```

### Step 4: Initialize progress.ndjson

Create an empty file or add the initial event:

```bash
echo '{"ts":"'$(date -u +%Y-%m-%dT%H:%M:%SZ)'","task_id":"my_new_task","event":"task_created","status":"pending","agent":"human"}' > progress.ndjson
```

### Step 5: Create artifacts/README.md

```markdown
# My New Task Artifacts

## Expected Outputs

### Code
- `code/feature_impl.cpp` - Core implementation
- `code/feature.h` - Public API header

### Tests
- `tests/feature_test.cpp` - Unit tests
- `tests/test_results.txt` - Test execution log

### Documentation
- `documentation/api_reference.md` - API documentation
- `documentation/migration_guide.md` - How to use the new feature

### Validation
- `validation/performance_report.txt` - Benchmark results
- `validation/coverage_report.txt` - Test coverage metrics
```

### Step 6: Register in current_index.json

Add your task to the task_status section:

```json
{
  "task_status": {
    "my_new_task": {
      "status": "pending",
      "blocked_by": ["prerequisite_task"],
      "ready_when": "prerequisite_task completes"
    }
  }
}
```

## How Task Modules Work

### Task Lifecycle

```
pending → ready → in_progress → completed
                       ↓
                    blocked (if issues arise)
                       ↓
                  in_progress (once unblocked)
```

### Execution Flow

1. **Task Selection**: AI reads `current_index.json` to identify the active task
2. **Context Loading**: AI reads `task.json`, `checklist.md`, and `progress.ndjson`
3. **Step Execution**: AI works through checklist items sequentially
4. **Progress Logging**: After each step, AI appends an event to `progress.ndjson`
5. **Artifact Generation**: AI creates outputs in `artifacts/` directory
6. **Validation**: AI checks acceptance criteria from `task.json`
7. **Completion**: AI marks task as complete and updates `current_index.json`

### Dependency Management

Tasks can depend on other tasks. The system automatically:
- Blocks tasks until dependencies are met
- Updates `ready_when` conditions in `current_index.json`
- Maintains a `critical_path` array showing the sequence of tasks

Example:
```json
{
  "critical_path": [
    "opengl_migration",
    "pass_bridging",
    "state_accessibility",
    "ndjson_telemetry"
  ]
}
```

## How to Read Task Modules

### Quick Status Check

```bash
# See what's currently active
cat ai/tasks/current_index.json | jq '.current_focus'

# Check task completion percentage
cat ai/tasks/opengl_migration/task.json | jq '.status'

# View recent progress events
tail -5 ai/tasks/pass_bridging/progress.ndjson | jq '.'
```

### Deep Dive into a Task

1. **Start with task.json** - Understand the goal and acceptance criteria
2. **Review checklist.md** - See what steps are planned
3. **Read progress.ndjson** - See what's been done and current status
4. **Check artifacts/** - Review generated outputs
5. **Read optional files** - coverage.md, mapping.md, implementation plans

### Understanding Progress

The progress.ndjson file is the real-time log. To find specific information:

```bash
# Find all build failures
grep 'build_failed' ai/tasks/my_task/progress.ndjson | jq '.'

# Find when a specific step was completed
grep 'Migrate function X' ai/tasks/my_task/progress.ndjson | jq '.'

# Count completed steps
grep 'step_completed' ai/tasks/my_task/progress.ndjson | wc -l

# See latest status
tail -1 ai/tasks/my_task/progress.ndjson | jq '.'
```

## Best Practices

### For Developers

1. **Keep checklists atomic**: Each step should be completable in 15-60 minutes
2. **Be specific**: "Fix bug" is bad; "Fix null pointer in RHI::clear()" is good
3. **Update progress frequently**: Log after every significant action
4. **Document blockers immediately**: Don't wait until standup to report issues
5. **Review acceptance criteria regularly**: Make sure you're on track
6. **Generate artifacts continuously**: Don't wait until the end to create outputs

### For AI Assistants

1. **Always read current_index.json first**: Know what's active before starting work
2. **Follow checklist order**: Don't skip ahead unless explicitly told to
3. **Log every step**: Update progress.ndjson after each action
4. **Check acceptance criteria**: Validate work against requirements
5. **Generate artifacts**: Create all expected outputs in artifacts/ directory
6. **Don't mark tasks complete prematurely**: All criteria must be met

### For Teams

1. **One task active at a time**: Focus reduces context switching
2. **Regular index updates**: Keep current_index.json synchronized
3. **Consistent event naming**: Use standard event types across tasks
4. **Artifact validation**: Review generated outputs before marking complete
5. **Dependency management**: Update blocked_by relationships promptly

## Common Patterns

### Pattern: Phase-Based Execution

Break large tasks into phases with clear milestones:

```markdown
## Phase 1: Research (No code changes)
- [ ] Read documentation
- [ ] Analyze existing code
- [ ] Design approach

## Phase 2: Implementation (Write code)
- [ ] Implement feature X
- [ ] Implement feature Y

## Phase 3: Validation (Verify quality)
- [ ] Run tests
- [ ] Check performance
- [ ] Update docs
```

### Pattern: Incremental Migration

For large refactors, migrate system by system:

```json
{
  "outputs": {
    "artifacts": [
      "artifacts/migration/system_a_migrated.cpp",
      "artifacts/migration/system_b_migrated.cpp",
      "artifacts/migration/system_c_migrated.cpp"
    ]
  }
}
```

### Pattern: Test-Driven Development

Write tests before implementation:

```markdown
- [ ] **Write test for feature X** - Define expected behavior
- [ ] **Implement feature X** - Make the test pass
- [ ] **Refactor** - Improve code quality
```

### Pattern: Blocked Task Handling

When a task is blocked:

```json
{
  "ts": "2024-12-28T10:00:00Z",
  "task_id": "my_task",
  "event": "blocker_encountered",
  "blocker": "Missing RHI API for cubemap generation",
  "blocked_by": "opengl_migration",
  "action": "Switch to preparatory work",
  "agent": "claude"
}
```

Update current_index.json:
```json
{
  "task_status": {
    "my_task": {
      "status": "blocked",
      "blocked_by": ["opengl_migration"],
      "preparation_work": ["design_api", "write_tests"]
    }
  }
}
```

## Troubleshooting

### Problem: AI isn't following the checklist

**Solution**: Be explicit in your prompt:
```
Please follow the checklist in ai/tasks/my_task/checklist.md exactly.
Work through each item in order and update progress.ndjson after each step.
```

### Problem: Progress isn't being logged

**Solution**: Check that progress.ndjson is being updated:
```bash
# Watch progress in real-time
tail -f ai/tasks/my_task/progress.ndjson
```

Remind the AI:
```
After completing each step, append an event to progress.ndjson with:
- Current timestamp
- task_id: "my_task"
- event: "step_completed"
- step: Name of the checklist item
```

### Problem: Task marked complete but acceptance criteria not met

**Solution**: Review acceptance criteria explicitly:
```
Before marking this task complete, verify that all acceptance criteria
from task.json are met:
1. [Check criterion 1]
2. [Check criterion 2]
3. [Check criterion 3]
```

### Problem: Dependencies not clear

**Solution**: Update current_index.json with clear dependency information:
```json
{
  "task_status": {
    "task_b": {
      "status": "blocked",
      "blocked_by": ["task_a"],
      "ready_when": "task_a provides RHI API extensions"
    }
  }
}
```

## Advanced Topics

### Custom Task Types

You can extend the base task module structure for specific needs:

**Performance Optimization Task:**
```
ai/tasks/optimize_raytracer/
├── task.json
├── checklist.md
├── progress.ndjson
├── artifacts/
│   ├── profiling/          # Profiler outputs
│   ├── benchmarks/         # Before/after metrics
│   └── optimization_log.md # What was changed and why
└── baseline_metrics.json   # Performance before optimization
```

**Bug Fix Task:**
```
ai/tasks/fix_memory_leak/
├── task.json
├── checklist.md
├── progress.ndjson
├── artifacts/
│   ├── valgrind_before.txt
│   ├── valgrind_after.txt
│   └── root_cause_analysis.md
└── reproduction_steps.md
```

### Integration with CI/CD

Task modules can integrate with continuous integration:

**In .github/workflows/task-validation.yml:**
```yaml
name: Task Validation

on:
  push:
    paths:
      - 'ai/tasks/*/progress.ndjson'

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - name: Check acceptance criteria
        run: |
          python scripts/validate_task_completion.py
```

### Telemetry and Analytics

Analyze progress.ndjson files to gain insights:

```python
import json
import pandas as pd

# Load all events
events = []
with open('progress.ndjson', 'r') as f:
    for line in f:
        events.append(json.loads(line))

df = pd.DataFrame(events)

# Calculate task duration
start = df[df['event'] == 'task_started']['ts'].iloc[0]
end = df[df['event'] == 'task_completed']['ts'].iloc[0]
duration = pd.to_datetime(end) - pd.to_datetime(start)
print(f"Task took: {duration}")

# Count step completions per day
df['date'] = pd.to_datetime(df['ts']).dt.date
daily_progress = df[df['event'] == 'step_completed'].groupby('date').size()
print(daily_progress)
```

## Summary

Task modules provide a structured, machine-readable way to plan, execute, and track engineering work. By following the conventions in this guide, you can:

- Collaborate effectively with AI assistants
- Maintain clear progress visibility
- Ensure work is completed to specification
- Create a detailed audit trail
- Reduce context switching and ambiguity

The key to success is **consistency**: use the same structure, follow the same patterns, and maintain the same level of detail across all tasks.

---

**Next Steps:**
- Read `ai/FOR_MACHINES.md` for the machine-readable specification
- Review existing task modules in `ai/tasks/` for examples
- Create your first task module using the templates in this guide
- Start using task modules in your daily workflow
