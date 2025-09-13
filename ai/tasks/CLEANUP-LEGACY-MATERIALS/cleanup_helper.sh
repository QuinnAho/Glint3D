#!/bin/bash
# Legacy Material System Cleanup Helper Script
# Run this script to identify all locations that need cleanup

echo "=== Legacy Material System Cleanup Helper ==="
echo "This script identifies code locations that need cleanup when removing the legacy Material system."
echo

echo "🚨 DEPRECATED markers in codebase:"
echo "======================================"
grep -r "🚨 DEPRECATED" engine/src/ engine/include/ --include="*.cpp" --include="*.h" -n

echo
echo "Legacy Material field usage:"
echo "============================"
grep -r "\.material[^C]" engine/src/ --include="*.cpp" -n

echo
echo "Legacy conversion calls:"
echo "========================"
grep -r "toLegacyMaterial" engine/src/ --include="*.cpp" -n

echo
echo "Material struct references:"
echo "==========================="
grep -r "Material " engine/include/ --include="*.h" -n | grep -v MaterialCore

echo
echo "Raytracer API calls needing update:"
echo "==================================="
grep -r "loadModel.*Material" engine/src/ --include="*.cpp" -n

echo
echo "Serialization using legacy material:"
echo "===================================="
grep -r "obj\.material\." engine/src/ --include="*.cpp" -n | head -10

echo
echo "=== Cleanup Progress Verification ==="
echo "Run this after cleanup to verify completion:"
echo
echo "# Should return 0 results after cleanup:"
echo "grep -r 'obj\.material[^C]' engine/src/ --include='*.cpp'"
echo "grep -r 'toLegacyMaterial' engine/src/ --include='*.cpp'"
echo "grep -r '🚨 DEPRECATED' engine/src/ --include='*.cpp'"
echo
echo "=== Next Steps ==="
echo "1. Follow the implementation order in CLEANUP-LEGACY-MATERIALS/spec.yaml"
echo "2. Use the markers (🚨 DEPRECATED) to find code locations"
echo "3. Test each change incrementally"
echo "4. Run verification commands after completion"