# Engine Source Reorganization Plan

This directory contains the proposed new structure for `engine/src/` to improve maintainability and scalability.

## Directory Structure Overview

```
engine/src_new/
├── core/                       # Core application framework
├── rendering/                  # Core rendering engine
│   ├── passes/                # Render pass implementations
│   └── rhi/                   # Hardware abstraction layer
├── scene/                     # Scene graph and asset management
│   ├── assets/                # Asset loading
│   │   └── importers/         # Format-specific importers
│   └── resources/             # Resource management
├── graphics/                  # Graphics utilities and components
│   ├── lighting/              # Lighting system
│   ├── ui_elements/           # UI rendering components
│   └── environment/           # Environment rendering
├── raytracing/                # Ray tracing implementation
│   ├── acceleration/          # Acceleration structures
│   ├── sampling/              # Monte Carlo sampling
│   └── materials/             # RT material models
├── interface/                 # UI and external interfaces
├── managers/                  # System managers (expanding)
└── utilities/                 # Cross-cutting utilities
    ├── security/              # Security utilities
    └── io/                    # I/O operations
```

## Migration Strategy

### Phase 1: Low-Risk Structural Moves
1. Move utilities and smaller files first
2. Move well-defined subsystems (raytracing, graphics utilities)
3. Update include paths incrementally

### Phase 2: Core System Moves
1. Move rendering components
2. Move scene management
3. Move interface layer
4. Update build files

### Phase 3: Manager Integration
1. Extract managers from monolithic files
2. Place new managers in managers/ directory
3. Update core systems to use managers

### Phase 4: Validation & Cleanup
1. Verify all includes are correct
2. Run full build and test suite
3. Update documentation
4. Remove unused files

## Files Requiring Refactoring

Large files that should be broken up during migration:

- `render_system.cpp` (2,298 lines) - Extract managers
- `rhi_gl.cpp` (1,356 lines) - Split into specialized managers
- `ui_bridge.cpp` (1,190 lines) - Use command handler pattern
- `json_ops.cpp` (790 lines) - Modularize into parser/validator/executor
- `ibl_system.cpp` (738 lines) - Consider splitting environment features
- `scene_manager.cpp` (680 lines) - May split scene graph from asset management

## Benefits

- **Maintainability**: Clear responsibility boundaries, easier debugging
- **Scalability**: Room for growth in each category
- **Team Development**: Clear ownership, parallel development
- **Testing**: Tests can mirror source structure
- **Onboarding**: New developers can navigate easily

## Next Steps

1. Review this structure with the team
2. Begin Phase 1 migration with low-risk moves
3. Update CMakeLists.txt to reflect new structure
4. Update documentation and examples