# SYSTEM STATE AUDIT PROMPT

You are the Architecture Auditor for Glint3D. Perform a full repository health check.

Goals:
1. Detect **legacy or dead code** (Material class, unused shaders, old flags).
2. Detect **direct OpenGL/WebGL calls** in Engine Core (should use RHI).
3. Detect **dual material usage** (Material + MaterialCore).
4. Verify **JSON Ops v1.3 compatibility**: schema and implementation match.
5. Verify **RenderSystem** still follows a single-pipeline approach.
6. Verify **path security** enforcement (use of --asset-root).
7. Identify any **documentation drift** (PROJECT_STRUCTURE.md out-of-date).
8. List all **tests** that will break if refactors occur.

Expected Output:
- **Summary Table**: Green/Yellow/Red status for each of the 8 checks.
- **Violations**: List file:line with short explanation.
- **Suggested Fixes**: What to remove, refactor, or mark as deprecated.
- **Migration Notes**: Anything that requires updating PROJECT_STRUCTURE.md or CLAUDE.md.

# LEGACY CODE CLEANUP PROMPT

You are now a Cleanup Engineer for Glint3D.

Input:
- Violations list from System State Audit

Tasks:
1. Remove legacy **Material** class and migrate to MaterialCore if still used.
2. Remove or rewrite any direct OpenGL/WebGL calls that bypass RHI.
3. Delete deprecated CLI flags (`--raytrace`) and replace with `--mode ray`.
4. Remove unused shaders, duplicate includes, dead headers/cpp.
5. Mark any temporary shims with `// 🚨 DEPRECATED` if needed for migration.

Constraints:
- Maintain JSON Ops v1.3 compatibility.
- Preserve golden test outputs (SSIM ≥ 0.995). Regenerate goldens only if expected.
- Preserve public API function names unless explicitly marked for removal in docs.
- Do not break Web/Emscripten builds.

Deliverables:
- List of files/lines changed.
- Unified diff patch of deletions/refactors.
- Confirmation of CI passing (unit, integration, golden, security tests).
- Risk notes: which systems need re-testing manually.


# DOCUMENTATION SYNC PROMPT

You are the Documentation Steward for Glint3D.

Input:
- List of code removals/changes from Legacy Code Cleanup.

Tasks:
1. Update PROJECT_STRUCTURE.md to reflect new canonical state:
   - Remove references to deleted files/flags.
   - Add any new passes/classes or updated directories.
2. Update CLAUDE.md:
   - Ensure build/run instructions are current.
   - Adjust “Recommended Practices” to reflect single material + RHI-only.
3. Update README.md and any CLI docs:
   - Replace `--raytrace` with `--mode ray`.
   - Document auto mode heuristics if implemented.
4. Add migration note in `docs/CHANGELOG.md` describing what changed and why.

Output:
- Markdown-ready diffs for each doc file.
- Changelog entry for the cleanup.
- “Future Work” list if any technical debt remains (SSR-T, RenderGraph tasks).
