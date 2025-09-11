# Glint3D Architecture Overhaul v1.0

## Overview

This document describes the completed architectural overhaul of Glint3D to create a cleaner, more AI-maintainable structure with explicit dependencies and machine-readable constraints.

## Implemented Changes

### 1. Platform Separation
- **Created `platforms/` directory** with desktop and web specific code
- **Moved desktop code**: `engine/src/main.cpp` → `platforms/desktop/main.cpp`
- **Moved UI panels**: `engine/src/panels/` → `platforms/desktop/panels/`
- **Moved ImGui integration**: `engine/imgui*` → `platforms/desktop/imgui/`
- **Moved web code**: `web/` → `platforms/web/`

### 2. SDK Creation
- **Created `sdk/web/` directory** with versioned TypeScript interface
- **Intent-based API**: High-level operations instead of low-level engine calls
- **Type-safe interface**: Full TypeScript definitions for all operations
- **Version management**: Semantic versioning with backward compatibility

### 3. Schema Organization  
- **Existing schemas**: Preserved `schemas/json_ops_v1.json`
- **Added JSON Ops v2**: Intent-based operations schema
- **Added config schema**: Engine configuration validation
- **Machine-readable contracts**: All operations defined by schemas

### 4. AI Metadata System
- **Created `ai-meta/` directory** with machine-readable project information
- **Architectural constraints**: `constraints.json` with layer rules and dependencies  
- **Intent mapping**: `intent_map.json` showing AI entry points and workflows
- **Design reasoning**: `reasoning.md` explaining architectural decisions
- **Change tracking**: `changelog.json` for evolution management
- **Requirements**: `non_functional.json` with performance and quality targets

### 5. Architectural Tooling
- **Dependency validator**: `tools/arch_check.py` enforces layer boundaries
- **Code generator**: `tools/codegen/intent_generator.py` for new intents
- **Automated enforcement**: CI-ready validation tools

### 6. Build System Updates
- **Platform CMake**: `platforms/desktop/CMakeLists.txt` for desktop builds
- **Web dependencies**: Updated `platforms/web/package.json` to use SDK
- **Dependency management**: Clear separation between platform and core

## New Directory Structure

```
├── engine/                    # Core C++ engine (unchanged structure)
├── platforms/
│   ├── desktop/              # Desktop-specific code (GLFW, ImGui)
│   └── web/                  # Web UI (React, Tauri)
├── sdk/
│   └── web/                  # TypeScript SDK with intent-based API
├── schemas/                  # JSON schemas and contracts
├── ai-meta/                  # Machine-readable project metadata
├── tools/                    # Architecture validation and code generation
├── tests/                    # Test suites (existing)
├── docs/                     # Documentation (existing)
└── .ai.json                  # AI interaction configuration
```

## Layer Dependencies (One-Way Only)

```
Schemas ← SDK ← UI (platforms/web)
         ↑     ↑  
       Engine ← Platform (platforms/desktop)
         ↑
      External
```

## AI Interaction Points

### Primary Entry Points
- `sdk/web/glint3d.ts` - Intent-based API for scene manipulation
- `schemas/json_ops_v2.json` - Operation definitions and validation
- `ai-meta/intent_map.json` - Workflow patterns and examples

### Secondary Entry Points  
- `engine/src/json_ops.cpp` - Intent implementation handlers
- `platforms/desktop/main.cpp` - Desktop application entry point
- `platforms/web/src/App.tsx` - Web UI main component

## Key Benefits

### 1. AI Maintainability
- **Clear boundaries**: Each layer has explicit responsibilities  
- **Intent system**: High-level operations instead of low-level calls
- **Machine-readable**: Constraints and workflows defined in JSON/schema
- **Validation tools**: Automated checking of architectural rules

### 2. Platform Isolation
- **No cross-contamination**: Desktop and web code completely separated
- **Targeted optimization**: Platform-specific features without affecting others
- **Clear interfaces**: Engine provides same API to all platforms

### 3. Stable Contracts
- **Versioned schemas**: Backward-compatible evolution
- **SDK abstraction**: Internal changes don't break external users  
- **Testing boundaries**: Each layer can be tested independently

### 4. Development Efficiency
- **Code generation**: Automated boilerplate for new intents
- **Architecture validation**: Prevent dependency violations automatically
- **Clear workflows**: Standard patterns for common tasks

## Usage Examples

### Adding New Intent
```bash
# Generate boilerplate code
python tools/codegen/intent_generator.py --intent add_material

# Files created:
# - generated/add_material/add_material_sdk.ts
# - generated/add_material/add_material_handler.cpp  
# - generated/add_material/add_material_schema.json
```

### Validating Architecture
```bash
# Check all architectural constraints
python tools/arch_check.py

# Example output:
# 🔍 Validating Glint3D architecture...
# ✅ All architectural constraints satisfied!
```

### Using SDK (AI-Friendly)
```typescript
import Glint3D from '@glint3d/web-sdk';

const engine = new Glint3D({ canvasId: 'glint-canvas' });
await engine.initialize();

// Intent-based operations
const objectId = await engine.loadModel('/models/sphere.obj');
await engine.setObjectMaterial(objectId, { 
  ior: 1.5, 
  transmission: 0.9 
});

const lightId = await engine.addLight({
  type: 'directional',
  direction: [0, -1, 0],
  intensity: 3.0,
  enabled: true
});

await engine.render({ width: 1024, height: 1024 });
```

## Migration Impact

### Build System
- Desktop builds now reference `platforms/desktop/`
- Web builds use SDK dependency from `sdk/web/`
- CMake configuration updated for platform separation

### Import Paths
- Web UI imports change from `web/` to `platforms/web/`  
- Desktop main entry point moved to `platforms/desktop/main.cpp`
- SDK provides versioned API surface

### Development Workflow
1. Check `ai-meta/intent_map.json` for entry points
2. Use SDK intents when possible
3. Validate architecture with `tools/arch_check.py`
4. Update schemas for new operations

## Validation Commands

```bash
# Architecture validation
python tools/arch_check.py

# Build desktop platform
cmake -S . -B builds/desktop/cmake
cmake --build builds/desktop/cmake

# Build web platform  
cd platforms/web && npm install && npm run build

# Build SDK
cd sdk/web && npm run build
```

## Next Steps

1. **Test integration**: Verify all build targets work with new structure
2. **Update CI/CD**: Modify build scripts for new platform paths  
3. **Documentation**: Update README and development guides
4. **Migration guide**: Create detailed migration instructions for contributors

This architectural overhaul creates a solid foundation for AI-maintainable development while preserving all existing functionality and performance characteristics.