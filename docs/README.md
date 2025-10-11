# Glint3D Documentation Index

Welcome to the Glint3D documentation! This index will guide you to the right resource based on your needs.

---

## Getting Started

### New Developer? Start Here:
1. **[ONBOARDING.md](ONBOARDING.md)** - Comprehensive guide for developers with C++/game dev background
   - Quick start (building, running examples)
   - Architecture deep dive (RHI, MaterialCore, RenderSystem, etc.)
   - Key concepts to research (OpenGL, PBR, ray tracing, render graphs)
   - Learning roadmap (Week 1 → Month 2+)
   - Agent-oriented design principles

### Want to Understand the Vision?
2. **[PRODUCT_VISION.md](PRODUCT_VISION.md)** - Market positioning and product strategy
   - The problem we solve (vs. Blender, Maya, 3ds Max)
   - Target audiences (businesses, creative teams, AI/ML labs, DevOps)
   - Platform roadmap (VizPack, DataGen, FinViz, verticals)
   - "Living Renderer" concept (self-improving, agent-friendly)

### Curious About Technical Decisions?
3. **[ARCHITECTURE_NOTES.md](ARCHITECTURE_NOTES.md)** - Deep dives on specific choices
   - Why raytracer isn't fully RHI-integrated (CPU vs GPU trade-offs)
   - Why MaterialCore instead of dual material systems
   - Why RenderGraph instead of monolithic rendering
   - Future direction: GPU ray tracing, compute shaders

---

## 📚 Core Documentation

### Development Workflow
- **[../CLAUDE.md](../CLAUDE.md)** - Primary reference for Claude Code assistant
  - Build commands (CMake, web, Tauri)
  - Running examples (desktop, headless, web)
  - Architecture overview (v0.3.0 highlights)
  - JSON-Ops bridge details
  - Security features (--asset-root)
  - Task workflow integration

### Task System
- **[../ai/tasks/README.md](../ai/tasks/README.md)** - AI agent instructions for task execution
  - Task execution protocol
  - Best practices (code quality, documentation)
  - Progress tracking format (NDJSON)
  - Task module structure
- **[../ai/tasks/current_index.json](../ai/tasks/current_index.json)** - Current active task and status
  - Check this FIRST before starting work
  - See critical path and blockers

---

## 🎯 By Role / Use Case

### I'm a Graphics Programmer
**Start with**: [ONBOARDING.md](ONBOARDING.md) → Section "Architecture Deep Dive"
**Focus on**:
- RHI abstraction layer (`engine/include/glint3d/rhi.h`)
- Pass-based rendering (`engine/include/render_pass.h`)
- PBR shader implementation (`engine/shaders/pbr.frag`)

**Next**: Explore active task in `ai/tasks/current_index.json` (currently: `pass_bridging`)

### I'm a Product Manager / Business Person
**Start with**: [PRODUCT_VISION.md](PRODUCT_VISION.md)
**Focus on**:
- Market fit & audience (Section: "Market Fit & Audience")
- Competitive differentiation (table comparing Glint3D vs. Blender/Maya)
- Platform roadmap (VizPack → DataGen → FinViz → Verticals)

**Next**: Review use cases and examples for your target market

### I'm an AI/ML Engineer (Building Agents)
**Start with**: [ONBOARDING.md](ONBOARDING.md) → Section "Agent-Oriented Design Philosophy"
**Focus on**:
- JSON-Ops API (`engine/include/json_ops.h`, `schemas/json_ops_v1.json`)
- Deterministic execution (seeded PRNG, fixed time stepping)
- Structured telemetry (NDJSON event format)

**Next**: Read `ai/tasks/ndjson_telemetry/checklist.md` and `ai/tasks/determinism_hooks/checklist.md`

### I'm a DevOps / Infrastructure Engineer
**Start with**: [PRODUCT_VISION.md](PRODUCT_VISION.md) → Section "For AI & Automation"
**Focus on**:
- Headless CLI (`glint --ops batch.json --render out.png`)
- Deterministic rendering (byte-stable outputs)
- Docker/cloud deployment (CPU raytracer fallback)

**Next**: Review `examples/json-ops/*.json` for scripting patterns

### I'm Contributing to Active Tasks
**Start with**: [../ai/tasks/current_index.json](../ai/tasks/current_index.json)
**Read**:
1. Current task's `README.md` (if exists)
2. Current task's `checklist.md` (atomic steps)
3. Current task's `progress.ndjson` (current status)

**Follow**: [../ai/tasks/README.md](../ai/tasks/README.md) for task execution protocol

---

## 🗂️ File Reference

### Documentation Files
```
docs/
├── README.md                    # This file (index)
├── ONBOARDING.md                # New developer comprehensive guide
├── PRODUCT_VISION.md            # Market positioning and roadmap
├── ARCHITECTURE_NOTES.md        # Technical decisions and trade-offs
└── release.md                   # Release notes (historical)
```

### Code Documentation
```
engine/
├── include/glint3d/            # Public API headers (well-documented)
│   ├── rhi.h                   # Render Hardware Interface
│   ├── rhi_types.h             # RHI type definitions
│   ├── material_core.h         # Unified material system
│   └── uniform_blocks.h        # UBO structures
├── include/                    # Internal headers
│   ├── render_system.h         # Main rendering orchestrator
│   ├── render_pass.h           # Pass-based architecture
│   └── raytracer.h             # CPU path tracer
└── shaders/                    # GLSL shaders
    ├── pbr.{vert,frag}         # PBR rendering
    ├── gbuffer.{vert,frag}     # Deferred rendering
    └── deferred.{vert,frag}    # Deferred lighting
```

### Task System
```
ai/tasks/
├── current_index.json          # Active task and status
├── README.md                   # Task execution protocol
├── pass_bridging/              # Current active task
├── state_accessibility/        # Next task (blocked by pass_bridging)
├── ndjson_telemetry/           # Future task
├── determinism_hooks/          # Future task
├── resource_model/             # Future task
└── json_ops_runner/            # End goal task
```

---

## 🔍 Quick Links by Topic

### Rendering
- **RHI Abstraction**: [ARCHITECTURE_NOTES.md](ARCHITECTURE_NOTES.md) + `engine/include/glint3d/rhi.h`
- **Pass-Based Architecture**: [ONBOARDING.md](ONBOARDING.md) Section 4 + `engine/include/render_pass.h`
- **Ray Tracing**: [ARCHITECTURE_NOTES.md](ARCHITECTURE_NOTES.md) Section "Raster vs Ray" + `engine/include/raytracer.h`

### Materials & Shading
- **MaterialCore**: [ONBOARDING.md](ONBOARDING.md) Section 2 + `engine/include/glint3d/material_core.h`
- **PBR Shaders**: `engine/shaders/pbr.frag` (inline documentation)

### Automation & Agents
- **JSON-Ops API**: [ONBOARDING.md](ONBOARDING.md) Section 6 + `engine/include/json_ops.h`
- **Determinism**: [ONBOARDING.md](ONBOARDING.md) Section "Deterministic Systems"
- **Telemetry**: `ai/tasks/ndjson_telemetry/checklist.md`

### Workflows
- **Building**: [../CLAUDE.md](../CLAUDE.md) Section "Common Development Commands"
- **Task System**: [../ai/tasks/README.md](../ai/tasks/README.md)
- **Contributing**: [ONBOARDING.md](ONBOARDING.md) Section "Contributing"

---

## 📖 Recommended Reading Order

### Week 1: Foundation
1. [PRODUCT_VISION.md](PRODUCT_VISION.md) - Understand the "why"
2. [ONBOARDING.md](ONBOARDING.md) Sections 1-2 - Project overview, quick start
3. [LearnOpenGL.com](https://learnopengl.com/) - External: OpenGL fundamentals

### Week 2-3: Deep Dive
1. [ONBOARDING.md](ONBOARDING.md) Section 3 - Architecture deep dive
2. `engine/include/glint3d/rhi.h` - Read RHI interface (300 lines, well-commented)
3. [ARCHITECTURE_NOTES.md](ARCHITECTURE_NOTES.md) - Understand technical choices

### Week 4: Contribute
1. [../ai/tasks/current_index.json](../ai/tasks/current_index.json) - Check active task
2. [../ai/tasks/README.md](../ai/tasks/README.md) - Learn task workflow
3. [ONBOARDING.md](ONBOARDING.md) Section "Next Steps" - Pick first contribution

---

## 🆘 Getting Help

### Documentation Questions
- **Missing info?** File an issue: [github.com/glint3d/glint3d/issues](https://github.com/glint3d/glint3d/issues) (placeholder)
- **Unclear section?** Suggest improvements via PR

### Code Questions
- **Architecture**: Read [ARCHITECTURE_NOTES.md](ARCHITECTURE_NOTES.md) first
- **API usage**: Check header files (`engine/include/`) - well-documented
- **Workflow**: See [../CLAUDE.md](../CLAUDE.md)

### Community
- **Discord**: [discord.gg/glint3d](https://discord.gg/glint3d) (placeholder)
- **GitHub Discussions**: [github.com/glint3d/glint3d/discussions](https://github.com/glint3d/glint3d/discussions) (placeholder)

---

## 🎯 Document Status

| Document | Status | Last Updated | Audience |
|----------|--------|--------------|----------|
| ONBOARDING.md | ✅ Complete | 2025-10-08 | New developers |
| PRODUCT_VISION.md | ✅ Complete | 2025-10-08 | PM, investors, business |
| ARCHITECTURE_NOTES.md | ✅ Complete | 2025-10-08 | Engineers |
| CLAUDE.md | ✅ Maintained | Ongoing | AI assistant, devs |
| ai/tasks/README.md | ✅ Maintained | Ongoing | AI agents, contributors |

---

**Welcome to Glint3D!** 🚀

Whether you're here to build features, understand the market, or contribute to the vision—we're glad to have you. Start with the document that matches your role above, and feel free to explore.

*Last updated: 2025-10-08*
