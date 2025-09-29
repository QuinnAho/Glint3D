#!/bin/bash

# Task Completion Verification Script
# Usage: ./verify_task_completion.sh <task_name>

TASK_NAME=$1
TASK_DIR="ai/tasks/$TASK_NAME"

if [ -z "$TASK_NAME" ]; then
    echo "Usage: $0 <task_name>"
    echo "Available tasks:"
    ls ai/tasks/ | grep -v "\.md\|\.json\|\.sh"
    exit 1
fi

if [ ! -d "$TASK_DIR" ]; then
    echo "Error: Task directory $TASK_DIR does not exist"
    exit 1
fi

echo "=== Task Completion Verification: $TASK_NAME ==="
echo

# Check required files exist
echo "📁 File Structure Check:"
REQUIRED_FILES=("task.json" "checklist.md" "progress.ndjson")
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$TASK_DIR/$file" ]; then
        echo "  ✅ $file exists"
    else
        echo "  ❌ $file missing"
    fi
done

if [ -d "$TASK_DIR/artifacts" ]; then
    echo "  ✅ artifacts/ directory exists"
else
    echo "  ❌ artifacts/ directory missing"
fi

echo

# Check task status in JSON
echo "📋 Task Status Check:"
if [ -f "$TASK_DIR/task.json" ]; then
    STATUS=$(jq -r '.status' "$TASK_DIR/task.json")
    echo "  Status: $STATUS"

    if [ "$STATUS" = "completed" ]; then
        echo "  ✅ Task marked as completed"
    else
        echo "  ⏳ Task not yet completed"
    fi
else
    echo "  ❌ Cannot check status - task.json missing"
fi

echo

# Check checklist completion
echo "✅ Checklist Progress:"
if [ -f "$TASK_DIR/checklist.md" ]; then
    TOTAL_ITEMS=$(grep -c "^- \[" "$TASK_DIR/checklist.md")
    COMPLETED_ITEMS=$(grep -c "^- \[x\]" "$TASK_DIR/checklist.md")
    PENDING_ITEMS=$(grep -c "^- \[ \]" "$TASK_DIR/checklist.md")

    echo "  Total items: $TOTAL_ITEMS"
    echo "  Completed: $COMPLETED_ITEMS"
    echo "  Pending: $PENDING_ITEMS"

    if [ $TOTAL_ITEMS -gt 0 ]; then
        COMPLETION_PERCENT=$((COMPLETED_ITEMS * 100 / TOTAL_ITEMS))
        echo "  Progress: $COMPLETION_PERCENT%"

        if [ $COMPLETION_PERCENT -eq 100 ]; then
            echo "  ✅ All checklist items completed"
        else
            echo "  ⏳ Checklist not fully completed"
        fi
    fi
else
    echo "  ❌ Cannot check progress - checklist.md missing"
fi

echo

# Check progress events
echo "📊 Progress Events:"
if [ -f "$TASK_DIR/progress.ndjson" ]; then
    EVENT_COUNT=$(wc -l < "$TASK_DIR/progress.ndjson")
    echo "  Total events: $EVENT_COUNT"

    if [ $EVENT_COUNT -gt 0 ]; then
        LAST_EVENT=$(tail -n 1 "$TASK_DIR/progress.ndjson" | jq -r '.event')
        LAST_STATUS=$(tail -n 1 "$TASK_DIR/progress.ndjson" | jq -r '.status')
        echo "  Last event: $LAST_EVENT ($LAST_STATUS)"
    fi
else
    echo "  ❌ No progress events found"
fi

echo

# Check artifacts
echo "📦 Artifacts Check:"
if [ -d "$TASK_DIR/artifacts" ]; then
    ARTIFACT_COUNT=$(find "$TASK_DIR/artifacts" -type f | wc -l)
    echo "  Artifact files: $ARTIFACT_COUNT"

    if [ $ARTIFACT_COUNT -gt 0 ]; then
        echo "  Generated artifacts:"
        find "$TASK_DIR/artifacts" -type f | sed 's|.*/||' | sort | sed 's/^/    /'
    fi
else
    echo "  ❌ No artifacts directory"
fi

echo

# Check dependencies
echo "🔗 Dependencies Check:"
if [ -f "$TASK_DIR/task.json" ]; then
    DEPENDENCIES=$(jq -r '.dependencies[]?' "$TASK_DIR/task.json")
    if [ -n "$DEPENDENCIES" ]; then
        echo "  Dependencies:"
        for dep in $DEPENDENCIES; do
            if [ -f "ai/tasks/$dep/task.json" ]; then
                DEP_STATUS=$(jq -r '.status' "ai/tasks/$dep/task.json")
                if [ "$DEP_STATUS" = "completed" ]; then
                    echo "    $dep: $DEP_STATUS"
                else
                    echo "    $dep: $DEP_STATUS"
                fi
            else
                echo "    $dep: not found"
            fi
        done
    else
        echo "  No dependencies"
    fi
fi

echo

# Overall assessment
echo "Overall Assessment:"
if [ -f "$TASK_DIR/task.json" ] && [ -f "$TASK_DIR/checklist.md" ]; then
    STATUS=$(jq -r '.status' "$TASK_DIR/task.json")
    COMPLETED_ITEMS=$(grep -c "^- \[x\]" "$TASK_DIR/checklist.md" 2>/dev/null || echo "0")
    TOTAL_ITEMS=$(grep -c "^- \[" "$TASK_DIR/checklist.md" 2>/dev/null || echo "1")

    if [ "$STATUS" = "completed" ] && [ $COMPLETED_ITEMS -eq $TOTAL_ITEMS ]; then
        echo "  Task appears to be fully completed"
    elif [ "$STATUS" = "completed" ]; then
        echo "  Task marked completed but checklist not fully done"
    elif [ $COMPLETED_ITEMS -eq $TOTAL_ITEMS ]; then
        echo "  Checklist complete but task not marked as completed"
    else
        echo "  Task is in progress"
    fi
else
    echo "  Cannot assess - missing required files"
fi

echo
echo "=== Verification Complete ==="