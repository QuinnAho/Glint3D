# Task Module System - Machine Specification

## Primary Directive

You are an AI agent executing tasks in the Glint3D project task management system. Follow this specification exactly to ensure consistent, high-quality task execution.

## Execution Protocol

### Step 1: Read Current Index

**ALWAYS** start by reading the current index file:

```
FILE: ai/tasks/current_index.json
```

**Extract:**
- `current_focus.active_task` - Which task to execute
- `current_focus.phase` - Current execution phase
- `current_focus.sub_issue` - Specific focus area
- `task_status[active_task].status` - Task state
- `task_status[active_task].blocked_by` - Blocking dependencies
- `critical_path` - Sequence of dependent tasks

**Decision Logic:**
```
IF current_focus.active_task == null THEN
  REPORT "No active task" and WAIT for instruction
ELSE IF task_status[active_task].status == "blocked" THEN
  REPORT blocker details and WAIT for instruction
ELSE IF task_status[active_task].status == "completed" THEN
  REPORT "Task already complete" and ASK for next task
ELSE
  PROCEED to Step 2 with active_task
END IF
```

### Step 2: Load Task Context

**Read ALL files for the active task** in this order:

1. **task.json** - Task specification
   ```
   FILE: ai/tasks/{active_task}/task.json
   ```
   Extract:
   - `id`, `title`, `owner`, `status`
   - `inputs.files` - Files to modify
   - `outputs.artifacts` - Expected deliverables
   - `acceptance` - Completion criteria
   - `dependencies` - Prerequisite tasks
   - `safety.rate_limits` - Constraints

2. **checklist.md** - Execution steps
   ```
   FILE: ai/tasks/{active_task}/checklist.md
   ```
   Parse:
   - All `[ ]` (pending) and `[x]` (completed) items
   - Phase headers (`## Phase N: ...`)
   - Step descriptions with **bold names**

3. **progress.ndjson** - Event history
   ```
   FILE: ai/tasks/{active_task}/progress.ndjson
   ```
   Parse each line as separate JSON object:
   - Identify latest event by `ts` field
   - Find last completed step
   - Check for blockers or failures

4. **artifacts/README.md** - Expected outputs
   ```
   FILE: ai/tasks/{active_task}/artifacts/README.md
   ```
   Note required artifacts and validation criteria

5. **Optional task-specific files**:
   - `coverage.md` - Detailed requirements
   - `mapping.md` - Code location reference
   - `README.md` - Task overview
   - Implementation plans in `artifacts/`

### Step 3: Determine Next Action

**Find next incomplete step:**

```python
# Pseudo-code for step selection
pending_steps = [step for step in checklist if step.status == "[ ]"]

if len(pending_steps) == 0:
  action = "validate_completion"
else:
  next_step = pending_steps[0]
  action = "execute_step"
  step_name = next_step.name
  step_description = next_step.description
```

**If all steps complete:**
- PROCEED to Step 5 (Validate Completion)

**If steps remain:**
- PROCEED to Step 4 (Execute Step)

### Step 4: Execute Step

**For each checklist step:**

1. **Announce intention:**
   ```
   "Working on: {step_name}"
   "Description: {step_description}"
   ```

2. **Perform the work:**
   - Read files specified in step
   - Make required code changes
   - Run commands (build, test, etc.)
   - Generate artifacts

3. **Validate the work:**
   - Verify no compilation errors
   - Check test results
   - Confirm expected outputs exist

4. **Update checklist:**
   ```
   ACTION: Edit ai/tasks/{active_task}/checklist.md
   CHANGE: "- [ ] **{step_name}**" → "- [x] **{step_name}**"
   ```

5. **Log progress:**
   ```
   ACTION: Append to ai/tasks/{active_task}/progress.ndjson
   FORMAT: Single-line JSON object (no pretty printing)
   CONTENT:
   {
     "ts": "{ISO_8601_timestamp}",
     "task_id": "{active_task}",
     "event": "step_completed",
     "step": "{step_name}",
     "status": "success|failure",
     "details": "{brief_description}",
     "agent": "{agent_identifier}"
   }
   ```

6. **Handle failures:**
   ```
   IF step fails THEN
     LOG event with "status": "failure" and "error": "{error_message}"
     DO NOT mark checklist item as complete
     DO NOT proceed to next step
     REPORT failure and WAIT for instruction
   END IF
   ```

7. **Repeat** for next incomplete step

### Step 5: Validate Completion

**Check all acceptance criteria** from task.json:

```python
# For each criterion in task.json["acceptance"]
for criterion in acceptance_criteria:
  result = validate(criterion)
  if result == False:
    LOG: {"event": "acceptance_failed", "criterion": criterion}
    REPORT: "Task not complete: {criterion} not met"
    RETURN to Step 4
```

**All criteria must be met:**
- Build succeeds without errors
- All tests pass
- Performance meets budgets
- All artifacts generated
- Documentation updated
- No regressions introduced

**If any criterion fails:**
- DO NOT mark task complete
- LOG failure event
- Return to execution or report blocker

**If all criteria pass:**
- PROCEED to Step 6

### Step 6: Mark Task Complete

**Update task.json:**
```
ACTION: Edit ai/tasks/{active_task}/task.json
CHANGE: "status": "in_progress" → "status": "completed"
```

**Log completion:**
```
ACTION: Append to ai/tasks/{active_task}/progress.ndjson
CONTENT:
{
  "ts": "{ISO_8601_timestamp}",
  "task_id": "{active_task}",
  "event": "task_completed",
  "status": "success",
  "artifacts": ["{list}", "{of}", "{generated}", "{artifacts}"],
  "agent": "{agent_identifier}"
}
```

**Update current_index.json:**
```
ACTION: Edit ai/tasks/current_index.json
UPDATE: task_status[active_task].status = "completed"
UPDATE: task_status[active_task].completion_date = "{date}"
UPDATE: current_focus.active_task = "{next_task_in_critical_path}"
```

**Report completion:**
```
"Task {active_task} completed successfully.
All acceptance criteria met.
Artifacts generated in ai/tasks/{active_task}/artifacts/
Next task: {next_task}"
```

## File Format Specifications

### task.json Schema

```json
{
  "id": "string (required, matches directory name)",
  "title": "string (required, human-readable)",
  "owner": "string (required)",
  "status": "enum (required): pending|in_progress|completed|blocked",
  "inputs": {
    "files": ["array of strings (required, file paths or paths:lines)"],
    "schema_version": "string (required)"
  },
  "outputs": {
    "artifacts": ["array of strings (required, expected output files)"],
    "events": ["array of strings (required, telemetry event types)"]
  },
  "acceptance": [
    "array of strings (required, testable completion criteria)"
  ],
  "safety": {
    "rate_limits": ["array of strings (optional)"],
    "sandbox": "boolean (optional)"
  },
  "dependencies": ["array of strings (required, task IDs)"],
  "seed_policy": "string (optional): deterministic|non_deterministic"
}
```

### checklist.md Format

```markdown
# {Task Name} Checklist

## Phase N: {Phase Name}
- [ ] **{Step Name}** - {Step Description}
- [x] **{Completed Step}** - {Description}
- [ ] **{Another Step}** - {Description with specific commands or files}

## Phase M: {Next Phase Name}
- [ ] **{Step Name}** - {Description}
```

**Parsing Rules:**
- Phase headers: Lines starting with `## Phase`
- Pending steps: Lines starting with `- [ ] **`
- Completed steps: Lines starting with `- [x] **`
- Step name: Text between `**` markers
- Description: Text after `**{name}** - `

### progress.ndjson Format

**Each line is a complete JSON object** (newline-delimited JSON):

```json
{"ts":"2024-12-28T10:00:00Z","task_id":"task_name","event":"task_started","status":"in_progress","agent":"human"}
{"ts":"2024-12-28T11:30:00Z","task_id":"task_name","event":"step_completed","step":"Read existing code","status":"success","details":"Reviewed 250 lines","agent":"claude"}
```

**Standard Event Types:**

| Event Type | Required Fields | Optional Fields | When to Use |
|------------|----------------|-----------------|-------------|
| `task_started` | ts, task_id, event, agent | phase | Task begins execution |
| `task_completed` | ts, task_id, event, status, agent | artifacts | All acceptance criteria met |
| `step_completed` | ts, task_id, event, step, status, agent | details | Checklist item finished |
| `step_failed` | ts, task_id, event, step, status, error, agent | details | Checklist item failed |
| `phase_completed` | ts, task_id, event, phase, agent | next_phase | Major section done |
| `build_succeeded` | ts, task_id, event, status, agent | duration_ms | Compilation successful |
| `build_failed` | ts, task_id, event, status, error, agent | - | Compilation failed |
| `test_passed` | ts, task_id, event, test_name, status, agent | duration_ms | Test successful |
| `test_failed` | ts, task_id, event, test_name, status, error, agent | - | Test failed |
| `blocker_encountered` | ts, task_id, event, blocker, agent | blocked_by, action | Cannot proceed |
| `blocker_resolved` | ts, task_id, event, blocker, agent | resolution | Can now proceed |

**Field Specifications:**

- `ts`: ISO 8601 timestamp with timezone (e.g., `2024-12-28T10:00:00Z`)
- `task_id`: Must match task directory name
- `event`: One of the standard event types above
- `status`: `success|failure|warning|in_progress`
- `agent`: Identifier (e.g., `claude_code_assistant`, `human`, `github_ci`)
- `step`: Must match checklist step name (text between `**` markers)
- `phase`: Must match phase header from checklist
- `error`: Human-readable error message
- `details`: Additional context (optional but recommended)

### current_index.json Schema

```json
{
  "current_focus": {
    "active_task": "string|null (task ID currently being worked on)",
    "phase": "string (current execution phase)",
    "sub_issue": "string (specific focus area)",
    "assigned_agent": "string|null (agent working on this)",
    "started_at": "ISO 8601 timestamp",
    "estimated_completion": "ISO 8601 timestamp",
    "notes": "string (current status summary)"
  },
  "task_status": {
    "{task_id}": {
      "status": "enum: pending|ready|in_progress|completed|blocked",
      "completion_percentage": "number (0-100, optional)",
      "completion_date": "ISO 8601 date (if completed)",
      "blocked_by": ["array of task IDs"],
      "ready_when": "string (condition for unblocking)",
      "priority": "enum: low|medium|high|critical",
      "estimated_effort": "string (time estimate)",
      "notes": "string (additional context)"
    }
  },
  "critical_path": [
    "array of task IDs in execution order"
  ],
  "blockers": {
    "immediate": ["array of current blockers"],
    "upcoming": ["array of anticipated blockers"]
  },
  "last_updated": "ISO 8601 timestamp",
  "updated_by": "string (agent or human identifier)",
  "notes": "string (global status notes)"
}
```

## Code Quality Requirements

### Modularity
- Functions should have single responsibility
- Maximum 50 lines per function (guideline)
- Clear separation of concerns
- Minimal coupling between modules

### Error Handling
- Check all return values
- Provide meaningful error messages
- Log errors before propagating
- Use exceptions appropriately (C++ projects)

### Performance
- Consider hot path performance
- Avoid unnecessary allocations
- Profile before optimizing
- Document performance requirements

### Memory Safety
- No memory leaks (use RAII in C++)
- Check for null pointers
- Validate array bounds
- Clean up resources in destructors

### Documentation
- Document all public APIs
- Explain complex algorithms
- Provide usage examples
- Update documentation when code changes

## Safety Constraints

### Rate Limits

**Respect rate_limits from task.json:**

```python
if "max_build_time_10min" in task.safety.rate_limits:
  # Abort build if it exceeds 10 minutes
  timeout = 600  # seconds

if "max_file_size_1mb" in task.safety.rate_limits:
  # Don't create files larger than 1MB
  max_size = 1048576  # bytes
```

**Standard Rate Limits:**
- `max_build_time_{N}s` / `max_build_time_{N}min` - Compilation timeout
- `max_file_size_{N}kb` / `max_file_size_{N}mb` - Output file size limit
- `max_memory_usage_{N}mb` / `max_memory_usage_{N}gb` - Memory consumption limit
- `max_render_time_{N}s` - Rendering operation timeout

### Sandbox Mode

```python
if task.safety.sandbox == True:
  # Restrict operations:
  # - No network access
  # - No file system access outside ai/tasks/{task_id}/
  # - No system calls
  # - No subprocess spawning
```

## Failure Handling

### Build Failures

```
IF build fails THEN
  LOG: {
    "event": "build_failed",
    "status": "failure",
    "error": "{compiler_error_message}"
  }
  ANALYZE error message
  ATTEMPT to fix common issues (missing includes, typos, etc.)
  RETRY build once
  IF still fails THEN
    REPORT failure and WAIT for instruction
  END IF
END IF
```

### Test Failures

```
IF tests fail THEN
  LOG: {
    "event": "test_failed",
    "test_name": "{test_that_failed}",
    "error": "{failure_message}"
  }
  REVIEW test expectations
  CHECK if implementation matches specification
  FIX implementation or test as appropriate
  RERUN tests
  IF still fails THEN
    REPORT failure and WAIT for instruction
  END IF
END IF
```

### Blocker Encountered

```
IF blocker encountered THEN
  LOG: {
    "event": "blocker_encountered",
    "blocker": "{description}",
    "blocked_by": ["{dependency_task}"],
    "action": "wait|switch_task|preparatory_work"
  }

  UPDATE: ai/tasks/current_index.json
  SET: task_status[active_task].status = "blocked"
  SET: task_status[active_task].blocked_by = ["{dependency}"]

  REPORT: "Task {active_task} is blocked by {blocker}"

  IF preparatory work available THEN
    SUGGEST: "Can work on {preparatory_items} while blocked"
  ELSE
    WAIT for instruction
  END IF
END IF
```

### Acceptance Criteria Not Met

```
IF acceptance criteria fail THEN
  LOG: {
    "event": "acceptance_failed",
    "criterion": "{failed_criterion}",
    "actual": "{actual_result}",
    "expected": "{expected_result}"
  }

  DO NOT mark task as complete
  DO NOT update status to "completed"

  ANALYZE gap between actual and expected
  IDENTIFY what needs to change

  IF can fix THEN
    ADD remediation step to checklist
    RETURN to execution
  ELSE
    REPORT failure and WAIT for instruction
  END IF
END IF
```

## Artifact Generation

### Required Artifacts

**Generate ALL artifacts** listed in task.json["outputs"]["artifacts"]:

```python
for artifact_path in task.outputs.artifacts:
  if not file_exists(artifact_path):
    CREATE file at artifact_path
    POPULATE with appropriate content
    VALIDATE format and completeness
```

### Artifact Types

**Code Artifacts:**
- Save to `artifacts/code/`
- Use appropriate file extension (.cpp, .h, .py, etc.)
- Include copyright/license header
- Follow project code style

**Test Artifacts:**
- Save to `artifacts/tests/`
- Include test results (pass/fail counts)
- Include test output logs
- Include coverage reports if available

**Documentation Artifacts:**
- Save to `artifacts/documentation/`
- Use Markdown format
- Include API documentation
- Include usage examples
- Include migration guides if applicable

**Validation Artifacts:**
- Save to `artifacts/validation/`
- Include performance benchmarks
- Include profiling results
- Include golden image comparisons
- Include any validation reports

### Artifact Validation

```python
for artifact in generated_artifacts:
  VERIFY file exists
  VERIFY file size > 0
  VERIFY file format is valid
  VERIFY content matches specification

  if validation fails:
    LOG error
    DELETE invalid artifact
    REGENERATE artifact
```

## Best Practices Checklist

Before marking any step complete, verify:

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] No performance regressions
- [ ] Memory leaks checked (if applicable)
- [ ] Documentation updated
- [ ] Checklist item marked `[x]`
- [ ] Progress logged to ndjson
- [ ] Artifacts generated
- [ ] Changes committed (if applicable)

Before marking task complete, verify:

- [ ] All checklist items marked `[x]`
- [ ] All acceptance criteria met
- [ ] All required artifacts generated
- [ ] All tests passing
- [ ] Performance budgets met
- [ ] Documentation complete
- [ ] No known issues or blockers
- [ ] Task status updated to "completed"
- [ ] Progress log shows "task_completed" event
- [ ] current_index.json updated

## Example Execution Trace

```
[1] Read ai/tasks/current_index.json
    → active_task = "pass_bridging"
    → status = "ready"
    → blockers = []

[2] Read ai/tasks/pass_bridging/task.json
    → Loaded specification
    → acceptance criteria: 5 items
    → dependencies: ["opengl_migration"] (met)

[3] Read ai/tasks/pass_bridging/checklist.md
    → Found 3 phases, 12 steps
    → 0 steps completed, 12 pending

[4] Read ai/tasks/pass_bridging/progress.ndjson
    → 1 event: task_created
    → No steps completed yet

[5] Execute first step: "Read existing implementation"
    → Read engine/src/render_system.cpp:2360-2628
    → Analyzed RenderGraph structure
    → Duration: 2 minutes

[6] Update checklist: mark "Read existing implementation" as [x]

[7] Log to progress.ndjson:
    {"ts":"2024-12-28T10:02:00Z","task_id":"pass_bridging","event":"step_completed","step":"Read existing implementation","status":"success","details":"Analyzed 268 lines","agent":"claude_code"}

[8] Execute next step: "Implement passGBuffer"
    → Modified engine/src/render_pass.cpp
    → Added passGBuffer function
    → Duration: 15 minutes

[9] Build verification:
    → cmake --build builds/desktop/cmake
    → Build succeeded in 45 seconds

[10] Update checklist: mark "Implement passGBuffer" as [x]

[11] Log to progress.ndjson:
     {"ts":"2024-12-28T10:17:00Z","task_id":"pass_bridging","event":"step_completed","step":"Implement passGBuffer","status":"success","agent":"claude_code"}
     {"ts":"2024-12-28T10:18:00Z","task_id":"pass_bridging","event":"build_succeeded","status":"success","duration_ms":45000,"agent":"claude_code"}

[...continue for all steps...]

[N] All steps complete, validate acceptance criteria:
    → "Raster graph produces lit frames" - PASS
    → "Ray graph produces non-black output" - PASS
    → "Readback functionality works" - PASS
    → "Screen-quad utility enables composition" - PASS
    → "Pass timing validation" - PASS

[N+1] Mark task complete:
      → Updated task.json status = "completed"
      → Logged task_completed event
      → Updated current_index.json
      → Moved to next task: "state_accessibility"

[N+2] Report:
      "Task pass_bridging completed successfully.
       All 5 acceptance criteria met.
       Generated 5 artifacts.
       Next task: state_accessibility"
```

## Summary

This specification defines the complete protocol for task execution. To execute a task:

1. **Always** read current_index.json first
2. Load all task context files
3. Execute checklist steps sequentially
4. Log progress after every action
5. Generate all required artifacts
6. Validate acceptance criteria
7. Mark task complete only when all criteria met
8. Update current_index.json with completion status

**DO NOT:**
- Skip steps without explicit instruction
- Mark steps complete if they fail
- Proceed when blocked
- Ignore acceptance criteria
- Leave artifacts ungenerated
- Make assumptions about task status

**ALWAYS:**
- Follow this specification exactly
- Log all events to progress.ndjson
- Validate work at each step
- Report failures immediately
- Wait for instruction when blocked
- Generate all required artifacts
- Verify acceptance criteria before completing

---

**Version:** 1.0
**Last Updated:** 2025-10-09
**Specification Status:** Stable
