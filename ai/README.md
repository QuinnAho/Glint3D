# AI Directory

Centralized location for all AI-related configuration, task management, and outputs for the Glint3D project.

## Directory Structure

```
ai/
├── README.md                   # This documentation
├── ontology/                   # AI ontology and semantic definitions
│   └── ontology.jsonld        # Linked data ontology
├── config/                     # AI configuration and constraints
│   ├── claude.json            # Global Claude Code configuration
│   ├── requirements_catalog.json  # Reusable requirement definitions
│   └── README.md              # Configuration documentation
└── tasks/                      # Task capsule management system
    ├── index.json             # Task registry and dependency graph
    ├── CURRENT                # Active task pointer
    ├── README.md              # Task system documentation
    └── FEAT-*/                # Feature task capsules (growing list)
```

## Purpose

This directory consolidates all AI-related files that were previously scattered across:
- `ai-config/` → `ai/config/` 
- `tasks/` → `ai/tasks/`
- `ontology/` → `ai/ontology/`

**Removed Legacy Files:**
- `ai-meta/` - Superseded by task system and requirements catalog
- `ai-out/` - No longer used with new task-based workflow  
- `.ai.json` - Replaced by modern `ai/config/claude.json`

## Quick Start

### Check Current Active Task
```bash
cat ai/tasks/CURRENT
# Example output: FEAT-0242/PR1
```

### Work with Claude Code
```bash
# Point Claude at the active task
Read ai/tasks/CURRENT, then read ai/tasks/<ID>/spec.yaml and overlay.ai.json.
Only edit files listed in whitelist.txt. Update AI-Editable sections in README.md.
```

### Validate Changes
```bash
# Check architectural constraints
tools/check_architecture.py --config ai/config/claude.json

# Validate task compliance
tools/validate_task_changes.py --task $(cat ai/tasks/CURRENT)
```

## Key Features

### Task Capsule System
- **Machine-readable specifications** for AI constraints
- **File whitelists** to prevent scope drift
- **Quality gates** for systematic validation
- **Dependency management** for proper ordering
- **Progress tracking** with append-only logs

### Configuration Management
- **Global constraints** for all AI assistants
- **Requirements catalog** for consistency
- **Task-specific overlays** for focused work
- **Automation integration** for CI/CD

### Quality Assurance
- **Architectural enforcement** (RHI abstraction, material unification)
- **Performance budgets** (frame time, memory usage)
- **Security validation** (path traversal protection)
- **Cross-platform testing** (desktop/web parity)

## Current Task Overview

The system tracks the v0.4.0 architecture refactoring with these priorities:

### Critical (Foundation)
- **FEAT-0242**: RHI Abstraction Layer - Eliminate direct OpenGL calls

### High Priority (Core Features)  
- **FEAT-0243**: MaterialCore Unified BSDF - Single material system
- **FEAT-0244**: Hybrid Auto Mode - Intelligent pipeline selection
- **FEAT-0246**: Deterministic Rendering - Golden test suite

### Medium Priority (Quality of Life)
- **FEAT-0245**: RenderGraph Pass System - Modular rendering
- **FEAT-0247**: Performance Profiling - Optimization framework

## Integration

### Development Workflow
- **Git**: Branches map to task PRs (e.g., `feat/242-rhi-interface-pr1`)
- **CI/CD**: Automated constraint validation and quality gates
- **Code Review**: Whitelist compliance and architectural alignment

### AI Assistance
- **Scope Control**: File whitelists prevent unrelated changes
- **Progress Tracking**: AI-Editable sections in task READMEs
- **Quality Gates**: Automated validation of acceptance criteria
- **Audit Trail**: Event logging for change tracking

This consolidated structure provides a single entry point for all AI-related project work while maintaining clear separation between configuration, active tasks, metadata, and outputs.