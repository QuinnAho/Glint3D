# Glint3D — AI-Driven Feature Workflow (End-to-End)

**Goal:** Ship features with *modular, deterministic, AI-maintainable* code. This workflow turns a plain-language idea into a production-ready change with guardrails, tests, and documentation—driven by cooperating AI agents.

---

## Principles
- **Single front door:** UI/CLI/SDK talk only to `glint::intents`.
- **One-way deps:** core → materials → rhi_iface → rendergraph → renderers → intents → platforms.
- **Determinism:** fixed timestep, seeded RNG, strict FP; golden images; render hash.
- **Machine context:** `/ai-meta/*` and `/.ai.json` define the rules AIs follow.
- **Small PRs:** schema → API → pass wiring → UI/SDK → docs/tests.

---

## Roles (multi-agent)
1. **Intake Agent:** turns user idea into a concrete feature card.
2. **Architect Agent:** proposes design, module impact, ADR.
3. **Requirements Auditor:** checks repo against the *requirements catalog*.
4. **Planner Agent:** plans atomic PRs with acceptance criteria.
5. **Scaffolder Agent:** creates stubs/targets/tests; no heavy logic.
6. **Codegen Agent:** applies minimal diffs inside allowed modules.
7. **Test Author Agent:** unit/integration/golden/determinism/perf/parity.
8. **Perf/Determinism Agent:** verifies budgets, seeds, FP mode, render hash.
9. **Security Agent:** path sandbox, JSON schema hardening.
10. **Docs Agent:** updates docs, SDK manifest, release notes.
11. **Release Agent:** versions, changelog, tags, artifacts.

Each agent has a prompt template in `/agents/`.

---

## Lifecycle (from idea → shipped)

### Stage 0 — Intake & Logging
**Input:** free-form user idea.  
**Output:** `/features/FEAT-XXXX/feature.md` + `ai-meta/changelog.json` entry.
- Assign an ID (e.g., FEAT-0241).
- Summarize intent, scope, success metrics, risks.
- Map to existing or new **intents**.

### Stage 1 — Architecture Brief (ADR)
**Output:** `/features/FEAT-XXXX/adr.md` and `docs/rendering_arch_v1.md` updates.
- Impacted modules/targets.
- Data/contract changes (JSON Ops, MaterialCore, RHI caps).
- Pass graph changes (gbuffer→lighting→ssr_t→post→readback).
- Fallbacks & capability probes.

### Stage 2 — Requirements Audit Gate
Run the **Requirements Auditor** against `/ai-out/requirements_catalog.json`.
- Produce `/ai-out/refactor_audit_results.json` and a Markdown scorecard.
- **Gate:** all `must:true` must be PASS or have explicit remediations queued.

### Stage 3 — Plan & Task Graph
**Output:** `/features/FEAT-XXXX/tasks.yml` (atomic PR plan). Typical sequence:
1) Schema & intent surface
2) Scaffolding (targets, headers, stubs)
3) Pass wiring/RHI usage
4) Tests (golden/determinism/parity/perf)
5) SDK & UI
6) Docs & ADR
7) Cleanup/legacy removal

### Stage 4 — Scaffolding
- Create headers in `engine/include/glint3d/**` only.
- Add CMake targets that mirror structure (no cycles).
- Seed tests under `tests/*` with specs (no assertions yet).
- Update `/ai-meta/intent_map.json`, `constraints.json`, `non_functional.json`.

### Stage 5 — Implementation (guarded)
- Code lives only in the allowed modules; **no GL calls** outside `engine/src/rhi/gl/*`.
- Keep diffs small and reversible. Commit per subtask.

### Stage 6 — Tests & Verification
Run:
- **Unit/Integration** (JSON Ops and intents).
- **Golden** (SSIM ≥ 0.995 by default).
- **Determinism** (same seed ⇒ same pixels ± tolerance).
- **Parity** (NullRHI vs GL where relevant).
- **Perf** (per-pass budgets).

If goldens change intentionally, attach diff/heatmap and open a “golden update” PR for human approval.

### Stage 7 — Docs & SDK
- Update `docs/` (architecture, intent surface).
- Update `sdk/web/manifest.json` and package version (engine minor match).
- Add examples and CLI docs (prefer `--mode` over `--raytrace`).

### Stage 8 — Release & Log
- Bump semver; add release notes.
- Write feature entry in `ai-meta/changelog.json` with constraints checked.
- Tag commit; attach build artifacts.

---

## CI Gates (suggested)
- **architecture.yml**: run `tools/arch_check.py` and fail on layering/name violations.
- **build_matrix.yml**: desktop & web profiles.
- **tests.yml**: unit/integration/golden/determinism/perf.
- **auditor.yml**: run Requirements Auditor; upload scorecard.
- **docs_sdk.yml**: ensure docs & SDK manifest updated when intents change.

---

## Definitions of Ready / Done

**DoR (before code):**
- Feature card + ADR drafted.
- Impact map and task graph agreed.
- Requirements audit has no blocking FAIL.

**DoD (to merge):**
- All `must:true` requirements PASS.
- Tests all green; perf budgets respected.
- Docs & SDK updated; release notes drafted.
- `/ai-meta/changelog.json` entry created.

---

## Traceability
- Every PR references `FEAT-XXXX`.
- Render metadata (`render.json`) includes feature ID, seed, caps, pass timings, render hash.

---

## Where things live
- Feature artifacts: `/features/FEAT-XXXX/*`
- AI metadata: `/ai-meta/*`
- Requirements: `/ai-out/requirements_catalog.json` (and scorecards)
- Tests: `/tests/*`
- Tools: `/tools/*`
- Public API: `engine/include/glint3d/**`

---

## Kickoff command (example)
> “Plan FEAT-0241: *Glass SSR-T in raster preview*. Generate ADR, tasks.yml (7 PRs), scaffolding diffs, tests specs, and an auditor scorecard. Respect /.ai.json layering; update ai-meta files.”

