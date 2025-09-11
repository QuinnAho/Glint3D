#!/usr/bin/env python3
"""
Glint3D Architecture Validation Tool

Validates that the codebase adheres to architectural constraints defined
in ai-meta/constraints.json. This tool enforces:

1. Layer dependency rules (one-way dependencies only)
2. Forbidden dependencies between layers  
3. Naming conventions
4. File organization patterns
5. Security boundaries

Usage:
    python tools/arch_check.py [--fix] [--verbose]
"""

import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

class ArchitectureValidator:
    def __init__(self, root_path: Path):
        self.root = root_path
        self.constraints = self.load_constraints()
        self.errors = []
        self.warnings = []
        
    def load_constraints(self) -> Dict:
        """Load architectural constraints from ai-meta/constraints.json"""
        constraints_path = self.root / "ai-meta" / "constraints.json"
        if not constraints_path.exists():
            raise FileNotFoundError(f"Constraints file not found: {constraints_path}")
        
        with open(constraints_path) as f:
            return json.load(f)
    
    def find_includes(self, file_path: Path) -> List[str]:
        """Extract #include statements from C++ files"""
        includes = []
        if file_path.suffix in ['.cpp', '.h', '.hpp']:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    for line in f:
                        line = line.strip()
                        if line.startswith('#include'):
                            # Extract include path
                            match = re.search(r'#include\s*[<"](.*?)[>"]', line)
                            if match:
                                includes.append(match.group(1))
            except Exception as e:
                self.warnings.append(f"Could not read {file_path}: {e}")
        return includes
    
    def get_layer_for_path(self, path: Path) -> str:
        """Determine which architectural layer a file belongs to"""
        relative_path = path.relative_to(self.root)
        parts = relative_path.parts
        
        if parts[0] == 'engine':
            return 'engine'
        elif parts[0] == 'platforms':
            return 'platforms'
        elif parts[0] == 'ui':
            return 'ui'
        elif parts[0] == 'sdk':
            return 'sdk'
        elif parts[0] == 'schemas':
            return 'schemas'
        elif parts[0] == 'ai-meta':
            return 'ai-meta'
        else:
            return 'unknown'
    
    def validate_layer_dependencies(self):
        """Validate that layers only depend on allowed layers"""
        layers = self.constraints['architecture']['layers']
        
        for layer_name, layer_info in layers.items():
            layer_files = self.find_files_in_layer(layer_name)
            
            for file_path in layer_files:
                includes = self.find_includes(file_path)
                
                for include_path in includes:
                    # Skip system includes
                    if include_path.startswith('/') or not ('/' in include_path):
                        continue
                    
                    # Determine target layer from include path
                    include_parts = include_path.split('/')
                    target_layer = include_parts[0]
                    
                    # Map directory names to layer names
                    if target_layer in ['engine', 'platforms', 'ui', 'sdk', 'schemas']:
                        target_layer_name = target_layer
                    else:
                        continue  # Skip unknown includes
                    
                    # Check if this dependency is allowed
                    allowed_deps = layer_info.get('dependencies', [])
                    forbidden_deps = layer_info.get('forbidden_dependencies', [])
                    
                    if target_layer_name in forbidden_deps:
                        self.errors.append(
                            f"FORBIDDEN DEPENDENCY: {layer_name} file {file_path} "
                            f"includes {include_path} (forbidden layer: {target_layer_name})"
                        )
                    elif target_layer_name != layer_name and target_layer_name not in allowed_deps:
                        self.warnings.append(
                            f"UNEXPECTED DEPENDENCY: {layer_name} file {file_path} "
                            f"includes {include_path} (layer {target_layer_name} not in allowed dependencies)"
                        )
    
    def find_files_in_layer(self, layer_name: str) -> List[Path]:
        """Find all source files belonging to a specific layer"""
        files = []
        layer_dirs = {
            'engine': ['engine'],
            'platforms': ['platforms'],
            'ui': ['ui'],
            'sdk': ['sdk'],
            'schemas': ['schemas'],
            'ai-meta': ['ai-meta']
        }
        
        dirs_to_search = layer_dirs.get(layer_name, [])
        
        for dir_name in dirs_to_search:
            dir_path = self.root / dir_name
            if dir_path.exists():
                files.extend(dir_path.rglob('*.cpp'))
                files.extend(dir_path.rglob('*.h'))
                files.extend(dir_path.rglob('*.hpp'))
        
        return files
    
    def validate_naming_conventions(self):
        """Validate file and directory naming conventions"""
        conventions = self.constraints.get('naming_conventions', {})
        
        # Check directory naming (snake_case)
        for root, dirs, files in os.walk(self.root):
            for dir_name in dirs:
                if not re.match(r'^[a-z0-9_\-\.]+$', dir_name):
                    if not dir_name.startswith('.'):  # Skip hidden dirs
                        self.warnings.append(
                            f"NAMING: Directory '{dir_name}' should use snake_case"
                        )
        
        # Check file naming
        for file_path in self.root.rglob('*'):
            if file_path.is_file():
                name = file_path.name
                if file_path.suffix in ['.cpp', '.h', '.hpp']:
                    if not re.match(r'^[a-z0-9_]+\.(cpp|h|hpp)$', name):
                        self.warnings.append(
                            f"NAMING: File '{name}' should use snake_case"
                        )
    
    def validate_security_boundaries(self):
        """Validate security boundary enforcement"""
        boundaries = self.constraints.get('security_boundaries', {})
        
        # Check for network access in engine core
        if 'network_access' in boundaries:
            engine_files = self.find_files_in_layer('engine')
            network_patterns = [
                r'curl',
                r'http',
                r'socket',
                r'connect\(',
                r'bind\(',
                r'listen\(',
                r'accept\('
            ]
            
            for file_path in engine_files:
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        for pattern in network_patterns:
                            if re.search(pattern, content, re.IGNORECASE):
                                self.errors.append(
                                    f"SECURITY: Engine file {file_path} contains "
                                    f"potential network access pattern: {pattern}"
                                )
                except Exception:
                    pass
    
    def validate_file_patterns(self):
        """Validate file organization patterns"""
        patterns = self.constraints.get('file_patterns', {})
        
        # Check for proper file extensions in appropriate directories
        for root, dirs, files in os.walk(self.root / 'engine' / 'shaders'):
            for file_name in files:
                if not re.search(r'\.(vert|frag|glsl|vs|fs)$', file_name):
                    self.warnings.append(
                        f"FILE PATTERN: Shader file {file_name} has unexpected extension"
                    )
    
    def detect_circular_dependencies(self) -> List[Tuple[str, str]]:
        """Detect circular dependencies between layers"""
        # This is a simplified check - a full implementation would build a dependency graph
        layers = list(self.constraints['architecture']['layers'].keys())
        circular_deps = []
        
        # For now, just check for obvious violations
        for layer_name, layer_info in self.constraints['architecture']['layers'].items():
            dependencies = layer_info.get('dependencies', [])
            for dep in dependencies:
                if dep in self.constraints['architecture']['layers']:
                    dep_deps = self.constraints['architecture']['layers'][dep].get('dependencies', [])
                    if layer_name in dep_deps:
                        circular_deps.append((layer_name, dep))
        
        return circular_deps
    
    def validate(self) -> bool:
        """Run all validation checks"""
        print("🔍 Validating Glint3D architecture...")
        
        # Run validation checks
        self.validate_layer_dependencies()
        self.validate_naming_conventions()
        self.validate_security_boundaries()
        self.validate_file_patterns()
        
        # Check for circular dependencies
        circular_deps = self.detect_circular_dependencies()
        for dep1, dep2 in circular_deps:
            self.errors.append(f"CIRCULAR DEPENDENCY: {dep1} ↔ {dep2}")
        
        # Report results
        if self.errors:
            print(f"\n❌ {len(self.errors)} architectural violation(s) found:")
            for error in self.errors:
                print(f"  • {error}")
        
        if self.warnings:
            print(f"\n⚠️  {len(self.warnings)} warning(s):")
            for warning in self.warnings:
                print(f"  • {warning}")
        
        if not self.errors and not self.warnings:
            print("✅ All architectural constraints satisfied!")
            return True
        elif not self.errors:
            print("✅ No violations found (warnings only)")
            return True
        else:
            print(f"\n❌ Architecture validation failed with {len(self.errors)} error(s)")
            return False

def main():
    """Main entry point"""
    root_path = Path(__file__).parent.parent
    validator = ArchitectureValidator(root_path)
    
    success = validator.validate()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()