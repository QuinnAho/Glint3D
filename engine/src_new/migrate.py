#!/usr/bin/env python3
"""
Engine Source Migration Script

This script helps migrate files from the flat engine/src structure
to the new organized structure in engine/src_new.

Usage:
    python migrate.py --dry-run           # Show what would be moved
    python migrate.py --phase 1          # Execute Phase 1 (low-risk moves)
    python migrate.py --phase 2          # Execute Phase 2 (core systems)
    python migrate.py --validate         # Check for broken includes
"""

import os
import shutil
from pathlib import Path

# Migration mapping: old_file -> new_location
MIGRATION_PLAN = {
    # Phase 1: Low-risk utility moves
    'utilities': {
        'path_security.cpp': 'utilities/security/',
        'path_utils.cpp': 'utilities/security/',
        'image_io.cpp': 'utilities/io/',
        'file_dialog.cpp': 'utilities/io/',
        'assimp_loader.cpp': 'utilities/',
        'glad.c': 'utilities/',
    },

    # Phase 1: Graphics utilities
    'graphics': {
        'gizmo.cpp': 'graphics/ui_elements/',
        'grid.cpp': 'graphics/ui_elements/',
        'axisrenderer.cpp': 'graphics/ui_elements/',
        'skybox.cpp': 'graphics/environment/',
        'ibl_system.cpp': 'graphics/environment/',
        'camera_controller.cpp': 'graphics/',
        'shader.cpp': 'graphics/',
        'light.cpp': 'graphics/lighting/',
    },

    # Phase 1: Raytracing subsystem
    'raytracing': {
        'raytracer.cpp': 'raytracing/',
        'raytracer_lighting.cpp': 'raytracing/',
        'bvh_node.cpp': 'raytracing/acceleration/',
        'triangle.cpp': 'raytracing/acceleration/',
        'microfacet_sampling.cpp': 'raytracing/sampling/',
        'ray_utils.cpp': 'raytracing/sampling/',
        'brdf.cpp': 'raytracing/materials/',
        'refraction.cpp': 'raytracing/materials/',
    },

    # Phase 2: Scene management
    'scene': {
        'scene_manager.cpp': 'scene/',
        'material_core.cpp': 'scene/',
        'importer_registry.cpp': 'scene/assets/',
        'objloader.cpp': 'scene/assets/',
        'mesh_loader.cpp': 'scene/assets/',
        'texture.cpp': 'scene/resources/',
        'texture_cache.cpp': 'scene/resources/',
        'importers/assimp_importer.cpp': 'scene/assets/importers/',
        'importers/obj_importer.cpp': 'scene/assets/importers/',
    },

    # Phase 2: Core application
    'core': {
        'application_core.cpp': 'core/',
        'cli_parser.cpp': 'core/',
        'clock.cpp': 'core/',
        'render_settings.cpp': 'core/',
    },

    # Phase 2: Rendering system
    'rendering': {
        'render_system.cpp': 'rendering/',
        'render_pass.cpp': 'rendering/',
        'render_mode_selector.cpp': 'rendering/',
        'render/ssr_pass.cpp': 'rendering/passes/',
        'rhi/rhi.cpp': 'rendering/rhi/',
        'rhi/rhi_gl.cpp': 'rendering/rhi/',
        'rhi/rhi_null.cpp': 'rendering/rhi/',
    },

    # Phase 2: Interface layer
    'interface': {
        'ui_bridge.cpp': 'interface/',
        'json_ops.cpp': 'interface/',
        'schema_validator.cpp': 'interface/',
    },

    # Phase 3: Managers (existing)
    'managers': {
        'managers/camera_manager.cpp': 'managers/',
    }
}

def main():
    print("Engine Source Migration Script")
    print("==============================")
    print()
    print("Current structure:")
    print("  engine/src/        - 45 files in flat structure")
    print("  engine/src_new/    - Organized structure (created)")
    print()
    print("This script is a PLACEHOLDER for the actual migration.")
    print("Manual steps required:")
    print()
    print("1. Review the migration plan in this script")
    print("2. Test builds after each phase")
    print("3. Update include paths as needed")
    print("4. Update CMakeLists.txt for new structure")
    print("5. Run tests to ensure functionality is preserved")
    print()
    print("Phases:")
    print("  Phase 1: Low-risk utilities and well-defined subsystems")
    print("  Phase 2: Core systems with potential include dependencies")
    print("  Phase 3: Manager integration after refactoring")
    print()
    print("Use 'git mv' commands to preserve file history during migration.")

if __name__ == "__main__":
    main()