# Glint3D: The Complete Guide

**The Living Renderer - A Comprehensive Technical & Product Overview**

**Version**: 1.0
**Last Updated**: 2025-10-08
**Target Audience**: Developers, Product Managers, Investors, Contributors

---

## Table of Contents

### Part I: Product Vision & Market
1. [Executive Summary](#executive-summary)
2. [The Problem We Solve](#the-problem-we-solve)
3. [The Glint3D Solution](#the-glint3d-solution)
4. [Market Fit & Target Audiences](#market-fit--target-audiences)
5. [Competitive Positioning](#competitive-positioning)
6. [Platform Roadmap](#platform-roadmap)

### Part II: Getting Started
7. [Quick Start Guide](#quick-start-guide)
8. [Building the Project](#building-the-project)
9. [Running Examples](#running-examples)
10. [Development Environment](#development-environment)

### Part III: Architecture Deep Dive
11. [System Architecture Overview](#system-architecture-overview)
12. [Core Systems](#core-systems)
13. [Render Hardware Interface (RHI)](#render-hardware-interface-rhi)
14. [MaterialCore - Unified Materials](#materialcore---unified-materials)
15. [RenderSystem & Pass-Based Rendering](#rendersystem--pass-based-rendering)
16. [Raytracer Architecture](#raytracer-architecture)
17. [JSON-Ops API](#json-ops-api)

### Part IV: Technical Deep Dives
18. [Architectural Decisions & Trade-offs](#architectural-decisions--trade-offs)
19. [Raster vs Ray Pipeline](#raster-vs-ray-pipeline)
20. [Deterministic Systems](#deterministic-systems)
21. [Agent-Oriented Design](#agent-oriented-design)

### Part V: Development Guide
22. [Key Concepts to Learn](#key-concepts-to-learn)
23. [Code Navigation Guide](#code-navigation-guide)
24. [Current Development Status](#current-development-status)
25. [Contributing Guidelines](#contributing-guidelines)
26. [Learning Roadmap](#learning-roadmap)

---

# Part I: Product Vision & Market

## Executive Summary

**Glint3D is "The Living Renderer"**—a controllable, scriptable, self-improving 3D rendering platform designed for both human creativity and automated pipelines.

### The Core Innovation

Unlike traditional 3D software (Maya, Blender, 3ds Max) that requires expensive licenses, steep learning curves, and manual workflows, Glint3D is built **automation-first**—every parameter is machine-readable, every render is deterministic, and every operation can run headless or interactively.

**Key Differentiators**:
- **Agent-Friendly**: AI can generate scenes, execute renders, validate results—no human required
- **Deterministic**: Same inputs → identical outputs (byte-stable across platforms)
- **Web-Native**: Desktop power + browser previews (no software installation)
- **Self-Improving**: Structured telemetry learns and optimizes over time
- **Cost-Effective**: Open-source, zero licensing fees, runs on modest hardware

### Vision Statement

> "Glint3D becomes the operating system for 3D visualization—where humans, teams, and automation systems co-create, validate, and share 3D content at scale."

---

## The Problem We Solve

### Traditional 3D Software Pain Points

#### 1. High Cost & Complexity

**The Problem**:
- **Maya/3ds Max**: $4,000+ per seat + powerful workstations required
- **Blender**: Free but steep learning curve (weeks of training)
- **Result**: Small businesses and startups priced out of product visualization

**Real-World Example**:
> A Detroit automotive startup with 200 product SKUs can't justify $4,000/seat × 5 designers + weeks of training just to render catalog images. At $50/image outsourced, that's $10,000 for 200 renders. They need automation.

#### 2. Clunky Collaboration

**The Problem**:
- Every collaborator needs heavyweight software installed
- File compatibility issues across versions
- No easy way to share interactive previews
- **Result**: Projects stall waiting for client feedback

**Real-World Example**:
> An ad agency sends a 500MB .blend file to a client who doesn't have Blender installed. The client can't review until they download 2GB of software and figure out basic navigation. Approval cycle: 3 days instead of 3 hours.

#### 3. No Automation Support

**The Problem**:
- Legacy tools built for manual, interactive workflows
- Difficult to script (MEL, Python APIs are complex)
- Can't integrate into CI/CD pipelines
- No deterministic rendering (same inputs ≠ same outputs)
- **Result**: Teams spend months building custom pipelines

**Real-World Example**:
> A robotics team needs 100,000 synthetic training images with varied lighting/camera angles. Blender Python scripting takes 3 months of engineering time. Non-deterministic outputs force manual validation. Budget: $50K+ in engineering labor.

### The Market Gap

**Who's Underserved?**
- **Businesses** needing automated product visualization (e-commerce, manufacturing)
- **AI/ML labs** requiring massive synthetic datasets (autonomous vehicles, robotics)
- **Creative teams** wanting faster iteration (advertising, design)
- **DevOps engineers** needing visual regression testing (game studios, SaaS)
- **Educators** seeking accessible 3D tools (STEM, design schools)

**Market Size**: $5B+ opportunity
- 3D software market: $2.5B (CAD, DCC tools)
- Render farm services: $1.5B (cloud rendering)
- Synthetic data generation: $1B+ (growing 40% YoY)

---

## The Glint3D Solution

### Dual-Mode Design: Humans + Machines

Glint3D works **equally well** for interactive creative work and headless automation:

#### For People (Interactive Mode)

**Drag & Drop Simplicity**
- Load OBJ/glTF/FBX models in seconds
- No complex project setup or scene graph management
- Desktop app (native) or web app (browser-based)

**Effortless Staging**
- Position cameras with preset angles (front, side, 3/4 view, turntable)
- Tweak lights (point, directional, spot) with real-time preview
- Adjust materials (PBR sliders: metallic, roughness, transmission, IOR)

**Instant Sharing**
- Export `.glintscene` file (portable, Git-friendly)
- Generate web link for interactive 3D preview (no software required)
- Share renders as PNG/EXR with embedded metadata

**Example Workflow**:
```
Designer loads product.glb
  → Positions camera (3/4 view preset)
  → Adds directional light + fill light
  → Tweaks material (increase metallic to 0.9)
  → Exports web preview URL
  → Client opens in browser, approves
  → Designer triggers final 4K render
Total time: 15 minutes
```

#### For AI & Automation (Headless Mode) 🤖

**Headless & Hands-Free**
```bash
# Run completely without UI
glint --ops batch.json --render output.png --w 2048 --h 2048

# Perfect for CI/CD, Docker, cloud rendering
docker run glint3d:latest glint --ops /data/scene.json --render /output/result.png
```

**Consistent, Reproducible Results**
```bash
# Same inputs → identical outputs
glint --ops scene.json --seed 42 --render out1.png
glint --ops scene.json --seed 42 --render out2.png
diff out1.png out2.png  # Byte-level identical!
```

**Massive Scale with Feedback Loops**
```json
{
  "version": "0.1",
  "task_id": "product_viz_batch_1000",
  "operations": [
    {"op": "load", "path": "product.glb"},
    {"op": "parameter_sweep", "parameter": "camera.angle", "values": [0, 45, 90, 135, 180, 225, 270, 315]},
    {"op": "parameter_sweep", "parameter": "light.intensity", "values": [1.0, 1.5, 2.0]},
    {"op": "batch_render", "output_pattern": "frame_{camera_angle}_{light_intensity}.png"}
  ]
}
```
**Result**: 8 camera angles × 3 light intensities = 24 images, fully automated

**A Living Renderer** 🌱
```ndjson
{"ts": "2025-10-08T12:00:00Z", "event": "pass_end", "pass": "RasterPass", "time_ms": 12.3}
{"ts": "2025-10-08T12:00:01Z", "event": "quality_metric", "metric": "SSIM", "value": 0.98}
{"ts": "2025-10-08T12:00:02Z", "event": "resource_alloc", "type": "texture", "size_mb": 45}
```
- Each render generates structured logs (timing, memory, quality)
- Analytics layer identifies bottlenecks: "RasterPass taking 80% of frame time"
- Future: ML-driven parameter tuning ("Auto-expose reduced by 0.3 EV for optimal histogram")

---

### What Sets Glint3D Apart

#### 1. Automation-First Design

**Every parameter is scriptable and machine-readable**:

```json
{
  "version": "0.1",
  "operations": [
    {"op": "load", "path": "product.glb"},
    {"op": "set_camera", "preset": "3/4_view", "fov": 45},
    {"op": "add_light", "type": "directional", "direction": [0, -1, 0], "intensity": 1.5},
    {"op": "set_material", "metallic": 0.9, "roughness": 0.2},
    {"op": "render", "output": "catalog_image.png", "width": 2048, "height": 2048}
  ]
}
```

**No GUI required**—this JSON runs identically on:
- Desktop (native OpenGL)
- Web (WebGL2/WASM)
- CI server (headless Docker)
- Cloud VM (AWS, GCP, Azure)

#### 2. Task-Based Workflow

Work is broken into **small, auditable tasks** with progress tracking:

```
ai/tasks/product_viz_batch_42/
├── task.json              # Specification (inputs, outputs, acceptance criteria)
├── checklist.md           # Atomic execution steps
├── progress.ndjson        # Event-driven progress log
└── artifacts/
    ├── renders/           # Generated images
    ├── telemetry.ndjson   # Performance logs
    └── validation.json    # Quality metrics
```

**Benefits**:
- **Automated oversight**: Agents can read progress, validate outputs, retry failures
- **Human-readable**: Designers can see "Step 5/10: Rendering lighting pass"
- **Machine-parseable**: CI can fail builds if quality metrics drop

#### 3. Sandboxed Experimentation

**Isolated rendering tasks** prevent breakage:
```bash
# Test new lighting without affecting production scenes
glint --ops experimental_lighting.json --sandbox --timeout 60s

# A/B test material settings
glint --ops glossy_metal.json --output variant_a.png
glint --ops matte_metal.json --output variant_b.png
```

Safety features:
- Resource limits (max texture size, polygon count)
- Timeout protection (kill hung renders)
- Path restrictions (--asset-root for file access control)

#### 4. Autonomous Rendering

**Generate thousands of variations hands-free**:

```bash
# Parameter sweep: 10 camera angles × 5 lighting setups = 50 images
glint --ops turntable.json --param camera.angle:0:360:10 --param light.intensity:1.0:3.0:5

# Animation: 300-frame turntable with one command
glint --ops scene.json --frames 0:300 --output frame_{frame}.png

# Dataset generation: 100,000 synthetic images for ML training
for i in {1..100000}; do
  glint --ops scene.json --seed $i --param randomize:all --output train_$i.png
done
```

#### 5. Intelligent Variation

**Automatic scene variation** (seeded for reproducibility):
- Randomized light placement within constraints
- Camera jitter for anti-aliasing or augmentation
- Material parameter sampling (metallic 0.0-1.0, roughness 0.0-1.0)
- Background environment rotation

**Example**: Synthetic dataset with domain randomization
```json
{
  "op": "randomize",
  "seed": 12345,
  "parameters": {
    "camera.position": {"type": "sphere", "radius": [3.0, 5.0]},
    "light.direction": {"type": "hemisphere", "up": [0, 1, 0]},
    "material.roughness": {"type": "uniform", "range": [0.1, 0.9]},
    "background.hdri": {"type": "choice", "options": ["hdri_001.exr", "hdri_002.exr", "hdri_003.exr"]}
  }
}
```

#### 6. Self-Improving Engine

**Benchmarks itself and optimizes over time**:

**Telemetry Collection**:
```ndjson
{"ts": "2025-10-08T12:00:00Z", "event": "pass_start", "pass": "GBufferPass", "frame": 0}
{"ts": "2025-10-08T12:00:15Z", "event": "pass_end", "pass": "GBufferPass", "time_ms": 15.2, "draws": 120}
{"ts": "2025-10-08T12:00:15Z", "event": "pass_start", "pass": "DeferredLightingPass", "frame": 0}
{"ts": "2025-10-08T12:00:27Z", "event": "pass_end", "pass": "DeferredLightingPass", "time_ms": 12.1}
```

**Analytics & Optimization**:
- Identify slow passes: "GBufferPass taking 55% of frame time"
- Memory leak detection: "Texture allocations growing over time"
- Quality vs performance trade-offs: "SSAO adds 8ms for 2% quality gain"
- Future: "Switch to deferred rendering for >10 lights (detected 15 lights)"

---

### Hybrid Desktop → Web Workflow

Glint3D keeps projects **alive and shareable** at every stage:

#### The Collaborative Feedback Loop

```
┌─────────────────────────────────────────────────────────────────┐
│ 1. Designer (Desktop App)                                       │
│    - Loads product.glb                                          │
│    - Stages camera + lighting                                   │
│    - Tweaks materials (PBR sliders)                             │
│    - Exports web preview                                        │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼ (Generates web link)
┌─────────────────────────────────────────────────────────────────┐
│ 2. Client (Web Browser)                                         │
│    - Opens URL (no software install)                            │
│    - Views interactive 3D (WebGL2)                              │
│    - Rotates, zooms, inspects                                   │
│    - Clicks "Approve" or "Request Changes"                      │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼ (Approval triggers)
┌─────────────────────────────────────────────────────────────────┐
│ 3. Cloud Render Farm                                            │
│    - Receives job queue                                         │
│    - Renders final 4K/8K image                                  │
│    - Uploads to S3/CDN                                          │
│    - Notifies team via webhook                                  │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼ (Results available)
┌─────────────────────────────────────────────────────────────────┐
│ 4. Team (Collaboration Platform)                                │
│    - Downloads final assets                                     │
│    - Reviews telemetry (render time, quality metrics)           │
│    - Archives scene files (Git repo)                            │
│    - Iterates for next version                                  │
└─────────────────────────────────────────────────────────────────┘
```

**Key Benefits**:
- **Always accessible**: Assets viewable even while background renders run
- **Zero friction**: Client doesn't need Blender/Maya/3ds Max installed
- **Version controlled**: `.glintscene` files are Git-friendly JSON
- **Reproducible**: Same scene file → identical render across machines

## Market Fit & Target Audiences

### Primary Audiences

#### 1. Businesses & Startups (Product Visualization) 🏢

**Pain Points**:
- Can't afford $4K/seat Maya licenses or weeks of designer training
- Manual rendering doesn't scale (200+ SKUs = months of work)
- Outsourcing costs $50-200/image (adds up fast)

**Use Case**: Automate catalog images for e-commerce

**Glint3D Solution**:
- Upload 3D models (from CAD or modeling service)
- Define lighting/camera templates via JSON-Ops
- Batch render all products overnight
- **Cost**: $0.04/image (cloud compute) vs $50/image (manual)

**ROI Example**:
```
Traditional approach:
- 500 SKUs × $50/image = $25,000
- 3-4 weeks turnaround

Glint3D approach:
- 500 SKUs × $0.04/image = $20
- 1 night render time
- Savings: $24,980 + 3 weeks of time
```

**Target Industries**:
- E-commerce (Amazon sellers, Shopify stores)
- Manufacturing (CAD → catalog pipeline)
- Furniture (IKEA-style configurators)
- Automotive (parts catalogs, customization)

#### 2. Creative Teams (Advertising, Design) 🎨

**Pain Points**:
- Iteration is slow (manual re-renders take minutes/hours)
- File sharing friction (500MB Blender files, compatibility issues)
- Client approval bottlenecks (days waiting for feedback)

**Use Case**: Client approval workflows, A/B testing material variations

**Glint3D Solution**:
- Designer tweaks materials in desktop app
- Exports web preview (generates URL instantly)
- Client reviews in browser, approves
- Final 4K renders trigger automatically

**ROI Example**:
```
Traditional approach:
- Email 500MB .blend file
- Client downloads Blender (2GB)
- Struggles with navigation
- 3-day approval cycle

Glint3D approach:
- Share web link
- Client clicks, sees 3D preview
- Approves in browser
- 3-hour approval cycle
- 8x faster iteration
```

**Target Industries**:
- Advertising agencies
- Product design studios
- Architectural visualization
- Marketing departments

#### 3. AI/ML Labs & Robotics (Dataset Generation) 🤖

**Pain Points**:
- Need millions of labeled synthetic images
- Blender Python scripting takes months to build pipeline
- Non-deterministic outputs force manual validation
- Can't scale to 100K+ images without custom infrastructure

**Use Case**: Generate training datasets with domain randomization

**Glint3D Solution**:
- Write JSON-Ops for random parameter sampling
- Run on AWS spot instances (10,000 parallel renders)
- Telemetry confirms quality metrics (no black frames, correct labels)
- Deterministic seeds enable reproducible experiments

**ROI Example**:
```
Traditional approach:
- 100K images needed
- 3 months engineering time (Blender pipeline)
- $50K engineering cost
- Manual QA on 1,000 samples

Glint3D approach:
- 1 week setup time
- $200 cloud compute (100K renders @ $0.002/image)
- Automated QA via telemetry
- Savings: $49,800 + 11 weeks
```

**Use Cases**:
- Self-driving cars (street scenes with weather variation)
- Robotics (object detection, grasping datasets)
- Retail AI (product recognition, SKU identification)
- Security (person detection, pose estimation)

#### 4. DevOps & Automation Engineers (Visual Regression Testing) 🔧

**Pain Points**:
- Rendering isn't "infrastructure as code"
- Can't integrate into CI/CD pipelines easily
- Visual bugs slip into production (lighting changes, material errors)
- Manual QA is slow and expensive

**Use Case**: CI/CD integration, golden image comparison

**Glint3D Solution**:
- Every commit triggers renders of 50 test scenes
- Compare against golden images (byte-level diff or SSIM)
- Catch rendering bugs before merge
- Fail builds if visual quality degrades

**ROI Example**:
```
Traditional approach:
- Manual QA reviews screenshots
- 2 hours per release
- Visual bugs slip through
- Hotfix costs $10K in emergency work

Glint3D approach:
- Automated visual regression tests
- Runs in 10 minutes on CI
- Catches bugs before merge
- Zero hotfixes for rendering issues
```

**Target Industries**:
- Game studios (visual regression testing)
- SaaS with 3D features (CAD, design tools)
- Real-time graphics engines (Unity, Unreal plugins)

#### 5. Educators & Students (Learning 3D) 🎓

**Pain Points**:
- Blender's complexity overwhelms beginners
- IT departments won't approve software installations
- Students have low-end hardware (can't run Maya)

**Use Case**: Interactive 3D lessons in browser, no software installation

**Glint3D Solution**:
- Teacher shares Glint3D web link
- Students explore 3D models in browser (any device)
- Adjust lighting, see instant results
- No IT approval, no downloads

**ROI Example**:
```
Traditional approach:
- Lab with 30 PCs running Blender
- $20K in hardware upgrades
- 8 hours training per student
- 40% drop out due to complexity

Glint3D approach:
- Students use Chromebooks/tablets
- Zero hardware cost
- 1 hour to productive work
- 90% completion rate
```

**Target Institutions**:
- High schools (STEM programs)
- Community colleges (design courses)
- Online learning platforms (Coursera, Udemy)
- Maker spaces, coding bootcamps

---

## Competitive Positioning

### Comparison Matrix

| Feature | Glint3D | Blender | Maya/3ds Max | Unity/Unreal |
|---------|---------|---------|--------------|--------------|
| **Cost** | Free (open-source) | Free | $4,000+/seat/year | Free (5% royalty) |
| **Learning Curve** | Low (JSON-Ops) | Steep (months) | Very Steep (years) | Medium (weeks) |
| **Automation** | ✅ Native (headless CLI) | ⚠️ Possible (Python) | ⚠️ Complex (MEL) | ⚠️ Game-focused |
| **Deterministic** | ✅ Byte-stable | ❌ Non-deterministic | ❌ Non-deterministic | ❌ Non-deterministic |
| **Web Previews** | ✅ Built-in | ⚠️ Manual export | ❌ No | ⚠️ WebGL export |
| **Telemetry** | ✅ Structured (NDJSON) | ❌ None | ❌ None | ⚠️ Analytics (games) |
| **CI/CD Integration** | ✅ Docker, headless | ⚠️ Hacky | ❌ Poor | ⚠️ Game-focused |
| **Target Use** | Automation + Creative | Creative | Creative | Real-time games |
| **Platform** | Desktop, Web, Cloud | Desktop only | Desktop only | Desktop, Mobile |
| **Scripting** | JSON (declarative) | Python (imperative) | MEL, Python | C#, Blueprints |

### Key Differentiators

#### 1. Automation-First Design
**Glint3D**: Built from day one for headless, scriptable workflows
**Others**: Retrofitted scripting onto interactive tools (awkward APIs, limited support)

#### 2. Deterministic Rendering
**Glint3D**: Same inputs → identical outputs (byte-stable)
**Others**: Random seeds, wall-clock time, GPU race conditions → non-reproducible

#### 3. Web-Native Workflows
**Glint3D**: Desktop ↔ Web seamless (same codebase via Emscripten)
**Others**: Manual export, separate web viewers, poor integration

#### 4. Structured Telemetry
**Glint3D**: NDJSON logs, performance metrics, quality scores (agent-parseable)
**Others**: Console logs, no structured data, manual profiling

#### 5. Agent-Friendly Architecture
**Glint3D**: Designed for AI/agent workflows (JSON-Ops, task system, telemetry)
**Others**: Designed for humans (GUI-first, manual workflows)

### Positioning Statement

> "For businesses and developers who need automated 3D rendering at scale, Glint3D is a deterministic, scriptable rendering platform that treats visualization like code—unlike legacy tools (Blender, Maya) that require manual workflows and expensive licenses."

---

## Platform Roadmap

**Glint3D isn't just a renderer—it's a platform for 3D automation.**

### EXAMPLE:VizPack (Product Visualization) 🎯 [2025-2026]

**Target Market**: E-commerce, manufacturing, catalog production

**Core Features**:
- Product staging templates (turntables, hero shots, detail close-ups)
- Material presets (brushed aluminum, matte plastic, polished chrome)
- Batch rendering with parameter sweeps (in progress)
- Web previews for client approval (in progress)
- Cloud render farm integration (planned)
- Asset marketplace (planned)

**EXAMPLE Business Model**: SaaS subscription
- **Solo**: $49/mo (100 renders/month, web previews, templates)
- **Team**: $199/mo (1000 renders/month, collaboration, priority support)
- **Enterprise**: Custom pricing (unlimited renders, on-premise deployment)

**Go-to-Market**:
1. Open-source community building (GitHub, Discord)
2. Design partner program (10 early adopters)
3. Product Hunt launch + content marketing
4. Sales outreach to e-commerce agencies

**Success Metrics**:
- 1,000 active users by Q4 2025
- 50 paying customers by Q1 2026
- $10K MRR by Q2 2026

---

### EXAMPLE PRODUCT: DataGen (Synthetic Dataset Generation) [2026-2027]

**Target Market**: AI/ML labs, robotics, autonomous vehicles

**Core Features**:
- Domain randomization (lighting, textures, camera jitter)
- Automatic labeling (bounding boxes, segmentation masks, depth maps)
- Quality validation (detect black frames, mislabeled objects)
- Cloud-scale rendering (100K+ images/day on spot instances)
- Dataset export formats (COCO, YOLO, Pascal VOC, custom)

**Business Model**: Pay-per-render + Enterprise licensing
- **Pay-per-Render**: $0.01-0.10/image (based on complexity)
- **Enterprise**: $50K-500K/year (unlimited renders, custom features)

**Go-to-Market**:
1. Research partnerships (universities, labs)
2. Conference presence (CVPR, ICCV, NeurIPS)
3. Case studies (show dataset quality improvements)
4. Direct sales to robotics/AV companies

**Success Metrics**:
- 10 enterprise customers by Q4 2026
- 100M images rendered by Q2 2027
- $500K ARR by Q4 2027

---

### Phase 3: FinViz (Financial Data Visualization) 📊 [2027-2028]

**Target Market**: Hedge funds, analytics firms, business intelligence

**Core Features**:
- Cinematic data storytelling (animated charts, 3D graphs)
- Scheduled renders (daily market reports, quarterly earnings)
- Automated annotations (trend lines, callouts, highlights)
- Integration with data pipelines (Tableau, Power BI, SQL, APIs)
- White-label branding (custom colors, logos, styles)

**Business Model**: Enterprise licensing
- **SMB**: $10K/year (50 reports/month, standard templates)
- **Enterprise**: $100K+/year (unlimited reports, custom development)

**Go-to-Market**:
1. Vertical-specific partnerships (Bloomberg Terminal integration)
2. Whale hunting (10 large financial institutions)
3. Content marketing (LinkedIn, finance blogs)

**Success Metrics**:
- 5 enterprise customers by Q2 2028
- $1M ARR by Q4 2028

---

### Phase 4+: Vertical Platforms [2028+]

#### EduViz (Education) 🎓
**Target**: K-12 schools, universities, online learning

**Features**:
- Interactive STEM lessons (physics simulations, chemistry models)
- Lab simulations (dissections, experiments, virtual labs)
- No-code scene builder for teachers
- Student portfolio hosting (showcase 3D work)

**Business Model**: Per-seat SaaS ($5/student/year, $500/teacher/year)

---

#### GameViz (Game Development) 🎮
**Target**: Game studios, indie developers

**Features**:
- Automated scene validation (lighting checks, occlusion testing)
- Playthrough rendering (capture gameplay as cinematic video)
- Asset previews (test materials before import into engine)
- CI/CD integration (visual regression testing)

**Business Model**: Per-project licensing ($500-5K/project)

---

#### MedViz (Medical Training) 🏥
**Target**: Medical schools, hospitals, device manufacturers

**Features**:
- High-fidelity anatomy datasets (surgical simulations)
- AR/VR training modules (procedural practice)
- Patient-specific modeling (from CT/MRI scans)
- HIPAA-compliant rendering (secure cloud, encrypted data)

**Business Model**: Enterprise licensing ($50K-500K/year)

---

#### ArchViz/XR (Architecture & Real Estate) 🏗️
**Target**: Architecture firms, real estate developers

**Features**:
- Instant virtual walkthroughs from CAD files
- VR tours triggered from design updates
- Client collaboration in browser (annotations, feedback)
- Photorealistic rendering (GI, caustics, volumetrics)

**Business Model**: Per-project licensing ($1K-10K/project)

---

### Long-Term Vision (2030+)

**Glint3D becomes the operating system for 3D visualization**:

1. **Marketplace Economy**:
   - Template marketplace (pre-made scenes: $10-100)
   - Material marketplace (PBR textures: $5-50)
   - Model marketplace (3D assets: $10-1000)
   - Revenue share: 70% creator, 30% platform

2. **AI Cinematographer**:
   - Upload product model → AI suggests 10 best camera angles
   - "Make it look premium" → AI adjusts lighting/materials
   - Learn from user feedback (thumbs up/down on renders)
   - Personalized recommendations based on industry/style

3. **Autonomous Creative Studio**:
   - Input: Brand guidelines + product specs
   - Output: Complete marketing campaign (images, videos, AR previews)
   - Human approval at key milestones, AI handles execution
   - Example: "Generate 50 product shots in brand style" → done in 1 hour

4. **Distributed Rendering Network**:
   - Peer-to-peer render farm (users rent spare GPU cycles)
   - Tokenized economy (GLINT tokens for compute)
   - Carbon-neutral rendering (renewable energy datacenters)

---

# Part II: Getting Started

## Quick Start Guide

### Prerequisites

- **C++ Compiler**: C++17 support (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake**: 3.15 or higher
- **Graphics**: OpenGL 3.3+ compatible GPU (or WebGL2 for web)

### Platform-Specific Requirements

#### Windows
- Visual Studio 2022 (Community Edition or higher)
- vcpkg (recommended for dependency management)

#### Linux
- GCC 9+ or Clang 10+
- OpenGL development libraries: `libgl1-mesa-dev libglu1-mesa-dev`

#### macOS
- Xcode Command Line Tools
- Homebrew (for dependencies)

---

## Building the Project

### Desktop (Windows)

#### Option 1: Using vcpkg (Recommended)

```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install glfw3:x64-windows assimp:x64-windows openimagedenoise:x64-windows

# Build Glint3D
cd path/to/Glint3D
cmake -S . -B builds/desktop/cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake --config Release -j

# Run
./builds/desktop/cmake/Release/glint.exe
```

#### Option 2: Using Convenience Script

```bash
# Debug build + launch UI
./build-and-run.bat

# Release build + launch UI
./build-and-run.bat release
```

### Desktop (Linux/macOS)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libglfw3-dev libassimp-dev

# Build
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# Run
./builds/desktop/cmake/Release/glint
```

### Web (Emscripten)

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build Glint3D for web
cd path/to/Glint3D
npm run web:build

# Or manually:
emcmake cmake -S . -B builds/web/emscripten -DCMAKE_BUILD_TYPE=Release
cmake --build builds/web/emscripten -j

# Serve locally
cd platforms/web
npm install
npm run dev
# Open http://localhost:3000
```

---

## Running Examples

### Interactive Mode (Desktop)

```bash
# Launch GUI
./builds/desktop/cmake/Release/glint.exe

# Load a model
File → Open → select assets/models/sphere.obj

# Experiment with:
- Camera presets (View menu)
- Lighting controls (Lights panel)
- Material editor (Inspector panel)
```

### Headless Rendering (CLI)

```bash
# Basic render from JSON-Ops
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output.png --w 512 --h 512

# With AI denoising (requires Intel Open Image Denoise)
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/directional-light-test.json --render output.png --denoise

# Force rendering mode
./builds/desktop/cmake/Release/glint.exe --ops scene.json --mode raster --render raster_output.png
./builds/desktop/cmake/Release/glint.exe --ops scene.json --mode ray --render ray_output.png

# Deterministic rendering (same seed → same output)
./builds/desktop/cmake/Release/glint.exe --ops scene.json --seed 42 --render deterministic.png
```

### Example JSON-Ops

**Simple sphere with directional light**:
```json
{
  "version": "0.1",
  "operations": [
    {
      "op": "load",
      "path": "assets/models/sphere.obj"
    },
    {
      "op": "add_light",
      "type": "directional",
      "direction": [0, -1, 0],
      "intensity": 1.5,
      "color": [1.0, 1.0, 1.0]
    },
    {
      "op": "set_camera",
      "position": [5, 5, 5],
      "look_at": [0, 0, 0],
      "fov": 45
    },
    {
      "op": "render",
      "output": "sphere_render.png",
      "width": 1920,
      "height": 1080
    }
  ]
}
```

Run with:
```bash
./glint --ops my_scene.json
```

---

## Development Environment

### Recommended IDE Setup

#### Visual Studio Code
```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-vscode.cmake-tools",
    "twxs.cmake",
    "llvm-vs-code-extensions.vscode-clangd"
  ]
}
```

#### Visual Studio 2022
- Open `builds/desktop/cmake/glint.sln` (generated by CMake)
- Set startup project to `glint`
- Configuration: Release (for performance) or Debug (for debugging)

### Debugging Tips

```bash
# Debug build with verbose output
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build builds/desktop/cmake --config Debug -j

# Run with debugger
gdb ./builds/desktop/cmake/Debug/glint
# or
lldb ./builds/desktop/cmake/Debug/glint
```

### Environment Setup (Optional)

To use `glint` as a global command:

**Windows**:
```bash
setup_glint_env.bat
```

**Linux/macOS**:
```bash
source setup_glint_env.sh
```

After setup:
```bash
# Instead of ./builds/desktop/cmake/Release/glint.exe ...
glint --ops scene.json --render output.png
```

---

# Part III: Architecture Deep Dive

## System Architecture Overview

Glint3D follows a **layered architecture** with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────────┐
│                     Application Layer                           │
│  (Desktop: ImGui UI, Web: React/Tailwind, CLI: Headless)        │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   JSON-Ops Bridge (v1)                          │
│      (Cross-platform scripting: load, transform, render)        │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Engine Core                                │
│  ┌─────────────────────────────────────────────────────┐       │
│  │            RenderSystem (Unified)                   │       │
│  │   - renderUnified() - main entry point              │       │
│  │   - Manager systems (camera, light, material)       │       │
│  │   - Pass-based architecture (RenderGraph)           │       │
│  └─────────────────────────────────────────────────────┘       │
│                         │                                       │
│        ┌────────────────┴────────────────┐                     │
│        ▼                                  ▼                     │
│  ┌──────────┐                      ┌──────────┐               │
│  │  Raster  │                      │   Ray    │               │
│  │ Pipeline │                      │ Pipeline │               │
│  └──────────┘                      └──────────┘               │
│  (RasterGraph)                     (RayGraph)                 │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│       RHI (Render Hardware Interface) Abstraction               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                     │
│  │  OpenGL  │  │  WebGL2  │  │  Future  │                     │
│  │ Backend  │  │ Backend  │  │  Vulkan  │                     │
│  └──────────┘  └──────────┘  └──────────┘                     │
└─────────────────────────────────────────────────────────────────┘
```

### Key Design Principles

1. **Separation of Concerns**: Each layer has a single, well-defined responsibility
2. **Dependency Inversion**: Engine core depends on abstractions (RHI), not concrete implementations
3. **Platform Agnostic**: Same engine code runs on desktop, web, and headless
4. **Data-Oriented**: Favor declarative JSON specs over imperative code
5. **Testability**: Every component can be tested in isolation

---

## Core Systems

### 1. Render Hardware Interface (RHI)

**Location**: `engine/include/glint3d/rhi.h`, `engine/src/rhi/rhi_gl.cpp`

The RHI is a **thin abstraction layer** over graphics APIs, similar to what you'd find in commercial engines (Unreal's RHI, Unity's Graphics API abstraction).

#### Design Goals

- **WebGPU-shaped API**: Modern design with CommandEncoder, RenderPassEncoder, Queue
- **Opaque handles**: `TextureHandle`, `BufferHandle`, `PipelineHandle` (type-safe, no raw pointers)
- **RAII patterns**: Resources auto-cleanup via destructors
- **<5% overhead**: Minimal performance cost (measured in production)

#### Core API

```cpp
// Resource creation
TextureHandle tex = rhi->createTexture(TextureDesc{
    .width = 1024,
    .height = 1024,
    .format = TextureFormat::RGBA8,
    .mipLevels = 1,
    .samples = 1
});

BufferHandle vbo = rhi->createBuffer(BufferDesc{
    .type = BufferType::Vertex,
    .size = vertexData.size() * sizeof(Vertex),
    .usage = BufferUsage::Static
});

ShaderHandle shader = rhi->createShader(ShaderDesc{
    .vertexSource = vertShaderCode,
    .fragmentSource = fragShaderCode
});

// Drawing
rhi->bindPipeline(pipeline);
rhi->bindTexture(tex, 0);  // Texture unit 0
rhi->bindUniformBuffer(ubo, 1);  // UBO binding point 1
rhi->draw(DrawDesc{
    .vertexCount = 36,
    .instanceCount = 1
});

// Cleanup (automatic via unique_ptr, or manual)
rhi->destroyTexture(tex);
rhi->destroyBuffer(vbo);
```

#### Why RHI Matters

**Without RHI** (direct OpenGL):
```cpp
// Tightly coupled to OpenGL
GLuint tex;
glGenTextures(1, &tex);
glBindTexture(GL_TEXTURE_2D, tex);
glTexImage2D(GL_TEXTURE_2D, ...);
// Hard to port to Vulkan/Metal!
```

**With RHI**:
```cpp
// Platform-agnostic
TextureHandle tex = rhi->createTexture(...);
// Same code works on OpenGL, WebGL2, future Vulkan
```

**Result**: Porting to Vulkan means implementing `rhi_vk.cpp`, not rewriting the entire engine.

#### Current Backends

- **OpenGL 3.3+** (`rhi_gl.cpp`): Desktop Windows/Linux/macOS
- **WebGL2** (`rhi_gl.cpp` with preprocessor flags): Web browsers
- **Null Backend** (`rhi_null.cpp`): Testing, CI without GPU

#### Future Backends (Planned)

- **Vulkan**: Desktop/mobile high-performance
- **Metal**: macOS/iOS native
- **WebGPU**: Next-gen web graphics

---

### 2. MaterialCore - Unified Materials

**Location**: `engine/include/glint3d/material_core.h`

This is the **single source of truth** for material properties. Both rasterization and ray tracing use the same struct—no more dual material systems!

#### The Old Problem (Pre-v0.3.0)

```cpp
// Rasterization had its own material struct
struct Material {
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

// Ray tracing had a separate struct
struct RaytraceMaterial {
    glm::vec3 baseColor;
    float metallic;
    float roughness;
    float ior;
};

// Conversion functions were lossy and error-prone
RaytraceMaterial convertToRayMaterial(const Material& mat);
```

**Problems**:
- Materials drift out of sync
- Conversion is lossy (shininess → roughness mapping is approximate)
- Duplicate storage in memory
- Confusing for users ("Which material is the source of truth?")

#### The Solution: MaterialCore

```cpp
struct MaterialCore {
    // Core PBR properties
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};    // RGBA (sRGB)
    float metallic{0.0f};                             // 0=dielectric, 1=metal
    float roughness{0.5f};                            // 0=mirror, 1=rough
    float normalStrength{1.0f};                       // Normal map intensity
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};            // Self-emission (HDR)

    // Transparency and refraction (for glass, liquids)
    float ior{1.5f};                                  // Index of refraction
    float transmission{0.0f};                         // Transparency [0,1]
    float thickness{0.001f};                          // Volume thickness
    float attenuationDistance{1.0f};                  // Beer-Lambert falloff

    // Advanced properties (clearcoat, subsurface, anisotropy)
    float clearcoat{0.0f};
    float clearcoatRoughness{0.03f};
    float subsurface{0.0f};
    glm::vec3 subsurfaceColor{1.0f, 1.0f, 1.0f};
    float anisotropy{0.0f};

    // Texture maps
    std::string baseColorTex;
    std::string normalTex;
    std::string metallicRoughnessTex;
    std::string emissiveTex;
    // ... more texture paths

    // Metadata
    std::string name;
    uint32_t id{0};
};
```

#### Factory Functions

```cpp
// Create common material types
auto aluminum = MaterialCore::createMetal(
    glm::vec3(0.91f, 0.92f, 0.92f),  // Color
    0.1f                              // Roughness (polished)
);

auto plastic = MaterialCore::createDielectric(
    glm::vec3(0.8f, 0.1f, 0.1f),     // Red plastic
    0.6f                              // Roughness (matte)
);

auto glass = MaterialCore::createGlass(
    glm::vec3(1.0f, 1.0f, 1.0f),     // Clear glass
    1.5f,                             // IOR (crown glass)
    0.95f                             // Transmission (5% reflection)
);

auto light = MaterialCore::createEmissive(
    glm::vec3(1.0f, 0.9f, 0.8f),     // Warm white
    10.0f                             // Intensity
);
```

#### Benefits

1. **Single source of truth**: One struct for all pipelines
2. **No conversion needed**: Rasterizer and raytracer read same data
3. **No drift**: Materials can't get out of sync
4. **Memory efficient**: No duplicate storage
5. **Extensible**: Easy to add new properties (subsurface, clearcoat)

#### Usage in Shaders

**Vertex Shader** (`pbr.vert`):
```glsl
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

**Fragment Shader** (`pbr.frag`):
```glsl
// Material properties (from MaterialCore)
uniform vec4 u_baseColor;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_normalStrength;
uniform vec3 u_emissive;
uniform float u_transmission;
uniform float u_ior;

uniform sampler2D u_baseColorTex;
uniform sampler2D u_normalTex;
uniform sampler2D u_metallicRoughnessTex;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

void main() {
    // Sample textures
    vec4 baseColor = u_baseColor * texture(u_baseColorTex, TexCoord);
    vec3 normal = computeNormal(TexCoord, u_normalTex, u_normalStrength);
    vec2 metallicRoughness = texture(u_metallicRoughnessTex, TexCoord).gb;

    // PBR lighting calculation
    vec3 color = computePBR(
        baseColor.rgb,
        u_metallic * metallicRoughness.g,
        u_roughness * metallicRoughness.r,
        normal,
        FragPos,
        viewPos
    );

    // Add emissive
    color += u_emissive;

    FragColor = vec4(color, baseColor.a);
}
```

---

### 3. RenderSystem & Pass-Based Rendering

**Location**: `engine/include/render_system.h`, `engine/src/render_system.cpp`

The RenderSystem is the **main rendering orchestrator**. It coordinates all subsystems and executes render passes.

#### Manager Systems

RenderSystem delegates responsibilities to specialized managers:

```cpp
class RenderSystem {
private:
    // Core managers
    CameraManager m_cameraManager;           // View/projection matrices
    LightingManager m_lightingManager;       // Lights, shadows
    MaterialManager m_materialManager;       // Materials, textures
    PipelineManager m_pipelineManager;       // Shader pipelines
    TransformManager m_transformManager;     // Object transforms
    RenderingManager m_renderingManager;     // Exposure, gamma, tone mapping

    // Render graphs
    std::unique_ptr<RenderGraph> m_rasterGraph;  // Rasterization pipeline
    std::unique_ptr<RenderGraph> m_rayGraph;     // Ray tracing pipeline

    // Mode selection
    std::unique_ptr<RenderPipelineModeSelector> m_pipelineSelector;
    RenderPipelineMode m_activePipelineMode;
};
```

#### Main Entry Point

```cpp
void RenderSystem::renderUnified(const SceneManager& scene, const Light& lights) {
    // 1. Analyze scene and select pipeline mode (raster vs ray)
    RenderPipelineMode mode = m_pipelineSelector->selectMode(scene);

    // 2. Get the appropriate render graph
    RenderGraph* graph = (mode == RenderPipelineMode::Raster) ? m_rasterGraph.get() : m_rayGraph.get();

    // 3. Build pass context with all necessary state
    PassContext ctx{};
    ctx.rhi = m_rhi.get();
    ctx.scene = &scene;
    ctx.lights = &lights;
    ctx.renderer = this;
    ctx.viewMatrix = m_cameraManager.viewMatrix();
    ctx.projMatrix = m_cameraManager.projectionMatrix();
    ctx.viewportWidth = m_fbWidth;
    ctx.viewportHeight = m_fbHeight;
    ctx.frameIndex = ++m_frameCounter;

    // 4. Execute the render graph
    m_rhi->beginFrame();
    graph->execute(ctx);
    m_rhi->endFrame();
}
```

#### Render Pipeline Mode Selection

**Automatic mode selection** based on scene analysis:

```cpp
class RenderPipelineModeSelector {
public:
    RenderPipelineMode selectMode(const SceneManager& scene) {
        // Analyze materials
        for (const auto& obj : scene.getObjects()) {
            const auto& mat = obj.materialCore;

            // If transmission + refraction → needs ray tracing
            if (mat.transmission > 0.01f && mat.ior > 1.05f) {
                return RenderPipelineMode::Ray;
            }

            // If complex volumetrics → needs ray tracing
            if (mat.subsurface > 0.01f) {
                return RenderPipelineMode::Ray;
            }
        }

        // Otherwise, use fast rasterization
        return RenderPipelineMode::Raster;
    }
};
```

**Manual override**:
```bash
# Force raster (fast, SSR approximation for reflections)
glint --ops scene.json --mode raster --render output.png

# Force ray (slow, physically accurate refraction)
glint --ops scene.json --mode ray --render output.png

# Auto-select (default, analyzes scene)
glint --ops scene.json --mode auto --render output.png
```

#### Render Graphs

**RasterGraph** (fast, OpenGL rasterization):
```
FrameSetupPass
    ↓
RasterPass          (render all objects with PBR shaders)
    ↓
OverlayPass         (debug gizmos, grid, axes)
    ↓
ResolvePass         (MSAA resolve, tonemapping)
    ↓
PresentPass         (display to screen)
```

**RayGraph** (slow, physically accurate):
```
FrameSetupPass
    ↓
RaytracePass        (CPU path tracing)
    ↓
RayDenoisePass      (AI denoising via Intel OIDN)
    ↓
ResolvePass         (tonemapping)
    ↓
PresentPass         (display to screen)
```

#### Pass Context

The `PassContext` struct carries all state needed by passes:

```cpp
struct PassContext {
    // Core pointers
    RHI* rhi;
    const SceneManager* scene;
    const Light* lights;
    RenderSystem* renderer;

    // Render targets
    RenderTargetHandle renderTarget;
    TextureHandle outputTexture;

    // Camera state
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    int viewportWidth;
    int viewportHeight;

    // Frame state
    uint32_t frameIndex;
    float deltaTime;

    // Control flags
    bool interactive;          // Interactive vs headless
    bool enableRaster;         // Raster pass enabled
    bool enableRay;            // Ray pass enabled
    bool enableOverlays;       // Debug overlays enabled
    bool resolveMsaa;          // MSAA resolve enabled
    bool finalizeFrame;        // Present to screen

    // Timing
    bool enableTiming;
    std::vector<PassTiming>* passTimings;
};
```

#### Pass Implementation Example

```cpp
class RasterPass : public RenderPass {
public:
    bool setup(const PassContext& ctx) override {
        // Validate requirements
        if (!ctx.rhi || !ctx.scene) return false;
        return true;
    }

    void execute(const PassContext& ctx) override {
        // Skip if disabled
        if (!ctx.enableRaster) return;

        // Render all scene objects
        ctx.renderer->passRaster(ctx);
    }

    void teardown(const PassContext& ctx) override {
        // Cleanup if needed
    }

    const char* getName() const override {
        return "RasterPass";
    }
};
```

---

### 4. Raytracer Architecture

**Location**: `engine/include/raytracer.h`, `engine/src/raytracer.cpp`

The raytracer is a **CPU-based path tracer** with BVH acceleration and physically accurate shading.

#### Why CPU Instead of GPU?

**Current Architecture** (CPU):
```cpp
class Raytracer {
    std::vector<Triangle> triangles;  // CPU memory
    BVHNode* bvhRoot;                 // CPU BVH tree

    void renderImage(std::vector<glm::vec3>& output, ...);  // CPU path tracing
};
```

**Reasons for CPU Approach**:

1. **Simplicity**: Easier to implement and debug than GPU compute
2. **Portability**: Works on systems without compute shader support (WebGL2, older hardware)
3. **Flexibility**: Easy to add complex features (nested refraction, subsurface scattering)
4. **Educational**: Code is readable C++, not cryptic GLSL compute
5. **Correctness**: Physically accurate reference implementation

**Trade-offs**:

| Aspect | CPU Raytracer | GPU Raytracer (Future) |
|--------|---------------|------------------------|
| **Performance** | Slow (seconds per frame) | Fast (milliseconds per frame) |
| **Memory** | Duplicated (CPU + GPU) | Shared (GPU only) |
| **Complexity** | Simple (C++ code) | Complex (compute shaders) |
| **Portability** | High (any platform) | Medium (needs compute support) |
| **Accuracy** | 100% (reference) | 100% (with careful implementation) |

#### Current Integration with RHI

The raytracer outputs to CPU memory, then bridges to RHI:

```cpp
void RenderSystem::passRayIntegrator(const PassContext& ctx,
                                     TextureHandle outputTexture, ...) {
    // 1. Load scene into CPU raytracer
    m_raytracer = std::make_unique<Raytracer>();
    m_raytracer->setSeed(m_seed);  // Deterministic

    for (const auto& obj : ctx.scene->getObjects()) {
        m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, obj.materialCore);
    }

    // 2. Ray trace to CPU buffer
    std::vector<glm::vec3> raytraceBuffer(ctx.viewportWidth * ctx.viewportHeight);
    m_raytracer->renderImage(raytraceBuffer, ctx.viewportWidth, ctx.viewportHeight,
                            camera.position, camera.front, camera.up, camera.fov, *ctx.lights);

    // 3. Upload to RHI texture (bridge CPU → GPU)
    m_rhi->updateTexture(outputTexture, raytraceBuffer.data(),
                        ctx.viewportWidth, ctx.viewportHeight, TextureFormat::RGB32F);
}
```

**Key Insight**: The raytracer is **intentionally** not fully RHI-integrated. It's a pragmatic design choice, not an oversight.

#### Future: GPU Ray Tracing

**Roadmap for GPU integration**:

**Phase 1 (Short Term)**: Keep CPU raytracer, optimize bridge
- ✅ Current state: CPU raytracer outputs to `std::vector<glm::vec3>`
- ✅ Bridge via `rhi->updateTexture()` (copy CPU → GPU)
- **No changes needed** for v0.3.x

**Phase 2 (Medium Term)**: Add optional GPU raytracer
- Implement compute shader raytracer (`ray_integrator.comp`)
- Use RHI compute pipeline: `rhi->dispatchCompute()`
- Share geometry buffers between raster and ray pipelines
- Keep CPU raytracer as fallback for WebGL2

**Phase 3 (Long Term)**: Hardware ray tracing
- Use DXR/Vulkan ray tracing extensions (RTX cores)
- Build acceleration structures on GPU (BLAS/TLAS)
- Hybrid rendering: raster primary rays, ray trace secondary rays

**Example Future Code**:
```cpp
// GPU compute shader raytracer (future)
void RenderSystem::passRayIntegratorGPU(const PassContext& ctx,
                                        TextureHandle outputTexture) {
    // Shared geometry (already on GPU from raster pass)
    BufferHandle vbo = ctx.scene->getVertexBuffer();
    BufferHandle bvh = ctx.scene->getBVHBuffer();

    // Bind resources
    m_rhi->bindStorageBuffer(vbo, 0);  // SSBO binding 0
    m_rhi->bindStorageBuffer(bvh, 1);  // SSBO binding 1
    m_rhi->bindImage(outputTexture, 2);  // Image binding 2

    // Dispatch compute shader
    m_rhi->bindPipeline(m_rayIntegratorPipeline);
    m_rhi->dispatchCompute(width/8, height/8, 1);  // 8×8 workgroups
}
```

**Benefit**: 10-100x faster ray tracing, real-time hybrid rendering possible.

---

### 5. JSON-Ops API

**Location**: `engine/include/json_ops.h`, `engine/src/json_ops.cpp`

JSON-Ops is a **declarative API** for scene manipulation. It's the bridge between UIs (desktop, web) and the engine.

#### Design Philosophy

**Declarative, not imperative**:
```json
// ✅ Good: Declarative (describe what, not how)
{
  "op": "set_camera",
  "position": [5, 5, 5],
  "look_at": [0, 0, 0],
  "fov": 45
}

// ❌ Bad: Imperative (step-by-step instructions)
{
  "op": "camera_set_position", "x": 5, "y": 5, "z": 5
},
{
  "op": "camera_look_at", "target": [0, 0, 0]
},
{
  "op": "camera_set_fov", "fov": 45
}
```

**Version-controlled and reproducible**:
```json
{
  "version": "0.1",
  "task_id": "product_render_v3",
  "seed": 42,
  "operations": [...]
}
```

#### Core Operations

**Scene Operations**:
```json
{"op": "load", "path": "assets/models/product.glb"}
{"op": "duplicate", "target": "object_1", "offset": [2, 0, 0]}
{"op": "transform", "target": "object_1", "translate": [1, 0, 0], "rotate": [0, 45, 0], "scale": [1.5, 1.5, 1.5]}
```

**Camera Operations**:
```json
{"op": "set_camera", "position": [5, 5, 5], "look_at": [0, 0, 0], "fov": 45}
{"op": "set_camera_preset", "preset": "3/4_view"}
{"op": "set_camera_preset", "preset": "front"}
```

**Lighting Operations**:
```json
{"op": "add_light", "type": "directional", "direction": [0, -1, 0], "intensity": 1.5}
{"op": "add_light", "type": "point", "position": [2, 3, 1], "color": [1.0, 0.9, 0.8], "intensity": 10.0}
{"op": "add_light", "type": "spot", "position": [0, 5, 0], "direction": [0, -1, 0], "inner_deg": 15, "outer_deg": 30}
```

**Material Operations**:
```json
{"op": "set_material", "target": "object_1", "baseColor": [0.8, 0.1, 0.1], "metallic": 0.9, "roughness": 0.2}
{"op": "set_material", "target": "object_2", "transmission": 0.95, "ior": 1.5}
```

**Rendering Operations**:
```json
{"op": "render", "output": "final_render.png", "width": 3840, "height": 2160}
{"op": "render", "output": "preview.png", "width": 512, "height": 512, "mode": "raster"}
```

#### Schema Validation

**JSON Schema** (`schemas/json_ops_v1.json`):
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Glint3D JSON Operations v0.1",
  "type": "object",
  "required": ["version", "operations"],
  "properties": {
    "version": {"type": "string", "enum": ["0.1"]},
    "task_id": {"type": "string"},
    "seed": {"type": "integer"},
    "operations": {
      "type": "array",
      "items": {
        "oneOf": [
          {"$ref": "#/definitions/LoadOp"},
          {"$ref": "#/definitions/CameraOp"},
          {"$ref": "#/definitions/LightOp"},
          {"$ref": "#/definitions/RenderOp"}
        ]
      }
    }
  },
  "definitions": {
    "LoadOp": {
      "type": "object",
      "required": ["op", "path"],
      "properties": {
        "op": {"const": "load"},
        "path": {"type": "string"}
      }
    },
    "CameraOp": {
      "type": "object",
      "required": ["op", "position", "look_at"],
      "properties": {
        "op": {"const": "set_camera"},
        "position": {"type": "array", "items": {"type": "number"}, "minItems": 3, "maxItems": 3},
        "look_at": {"type": "array", "items": {"type": "number"}, "minItems": 3, "maxItems": 3},
        "fov": {"type": "number", "minimum": 1, "maximum": 179}
      }
    }
  }
}
```

**Benefits**:
- Catch errors before execution
- Clear error messages ("fov must be between 1 and 179")
- Auto-completion in IDEs (with JSON schema support)

#### Usage Across Platforms

**Desktop (ImGui UI)**:
```cpp
// User clicks "Add Light" button
void onAddLightClicked() {
    nlohmann::json op = {
        {"op", "add_light"},
        {"type", "directional"},
        {"direction", {0, -1, 0}},
        {"intensity", 1.5}
    };
    applyOperation(op);
}
```

**Web (React UI)**:
```typescript
// User adjusts slider
function onMetallicChange(value: number) {
  const op = {
    op: "set_material",
    target: selectedObject,
    metallic: value
  };
  applyOp(JSON.stringify(op));
}
```

**CLI (Headless)**:
```bash
# Execute batch of operations
glint --ops my_scene.json

# Where my_scene.json contains:
# {
#   "version": "0.1",
#   "operations": [...]
# }
```

**Result**: Same JSON-Ops work everywhere—desktop, web, CI/CD, cloud.

---

# Part IV: Technical Deep Dives

## Architectural Decisions & Trade-offs

### Decision 1: Why MaterialCore Instead of Dual Materials?

**Old Approach** (pre-v0.3.0):
- Separate `Material` (raster) and `RaytraceMaterial` (ray) structs
- Conversion functions between the two
- Duplicate storage in scene objects

**Problems**:
- Materials drift out of sync (designer changes raster, forgets ray)
- Conversion is lossy (Phong shininess → GGX roughness)
- Confusing for users ("Which is the source of truth?")

**Solution: MaterialCore**:
- Single unified struct used by both pipelines
- PBR parameters (metallic, roughness, transmission, IOR)
- No conversion, no drift, single source of truth

**Trade-off**: Can't use legacy Phong lighting (but PBR is industry standard now).

---

### Decision 2: Why RenderGraph Instead of Monolithic Render?

**Old Approach** (pre-v0.3.0):
```cpp
void RenderSystem::render(const SceneManager& scene, const Light& lights) {
    // 500+ lines of rendering code in one function
    if (mode == RenderMode::Raster) {
        // ... 200 lines of raster code
    } else if (mode == RenderMode::Raytrace) {
        // ... 300 lines of ray code
    }

    // Hard to add new effects (where do I insert SSAO?)
    // Hard to profile (which part is slow?)
}
```

**Problems**:
- Monolithic code is hard to understand/modify
- Can't easily add new effects (SSAO, bloom, DOF)
- Can't profile individual steps
- Can't reorder or disable passes dynamically

**Solution: RenderGraph**:
```cpp
// Each pass is independent, composable
m_rasterGraph->addPass(std::make_unique<FrameSetupPass>());
m_rasterGraph->addPass(std::make_unique<RasterPass>());
m_rasterGraph->addPass(std::make_unique<OverlayPass>());
m_rasterGraph->addPass(std::make_unique<ResolvePass>());
m_rasterGraph->addPass(std::make_unique<PresentPass>());

// Easy to add new passes
m_rasterGraph->addPass(std::make_unique<SSAOPass>());

// Easy to profile
for (auto& pass : passes) {
    auto start = std::chrono::high_resolution_clock::now();
    pass->execute(ctx);
    auto end = std::chrono::high_resolution_clock::now();
    logTelemetry("pass_end", pass->getName(), duration_ms(end - start));
}
```

**Benefits**:
- Modular: Each pass has single responsibility
- Testable: Can test passes in isolation
- Extensible: Add new passes without touching existing code
- Profilable: Measure performance per-pass
- Reorderable: Change pipeline order via configuration

**Trade-off**: Slight overhead from function calls (but <1% measured).

---

### Decision 3: Why JSON-Ops API?

**Problem**: How to script the engine across platforms?
- Desktop: Could use C++ API directly
- Web: JavaScript needs to communicate with WASM
- Headless CI: Need reproducible test cases
- Agents: Need machine-readable format

**Alternative Approaches**:

**Option A: Python API**:
```python
scene = glint.Scene()
scene.load("model.obj")
scene.add_light(type="directional", direction=[0, -1, 0])
scene.render("output.png")
```
- ❌ Doesn't work in web (Python in browser?)
- ❌ Not declarative (procedural code)
- ❌ Hard to version control (code diffs are noisy)

**Option B: Custom DSL**:
```glint
load "model.obj"
light directional direction(0, -1, 0)
render "output.png"
```
- ❌ Custom parser needed (complexity)
- ❌ No editor support (syntax highlighting, autocomplete)
- ❌ Not standard (JSON is ubiquitous)

**Option C: JSON-Ops** (chosen):
```json
{
  "version": "0.1",
  "operations": [
    {"op": "load", "path": "model.obj"},
    {"op": "add_light", "type": "directional", "direction": [0, -1, 0]},
    {"op": "render", "output": "output.png"}
  ]
}
```
- ✅ Works everywhere (desktop, web, CLI)
- ✅ Declarative (describe what, not how)
- ✅ Version-controlled (Git diffs are clear)
- ✅ Editor support (JSON schema → autocomplete)
- ✅ Standard format (every language has JSON parser)

**Trade-off**: More verbose than DSL, but benefits outweigh cost.

---

## Raster vs Ray Pipeline

### Current Architecture

```
Application Code
      │
      ▼
RenderSystem::renderUnified()
      │
      ├─────────────────┬─────────────────┐
      ▼                 ▼                 ▼
 RasterGraph       RayGraph        Auto-Select
 (GPU/OpenGL)     (CPU/C++)      (Analyze scene)
      │                 │
      ▼                 ▼
  RHI Layer       std::vector<glm::vec3>
  (GPU buffers)   (CPU memory)
      │                 │
      └────────┬────────┘
               ▼
          rhi->updateTexture()
        (Bridge CPU → GPU)
               │
               ▼
          Final Image
```

### Rasterization Pipeline (GPU)

**Fully integrated with RHI**:
```cpp
// Geometry in GPU buffers
BufferHandle vbo = rhi->createBuffer(BufferDesc{...});
rhi->updateBuffer(vbo, vertices.data(), vertices.size() * sizeof(Vertex));

// Shaders compiled to GPU programs
ShaderHandle shader = rhi->createShader(ShaderDesc{
    .vertexSource = pbr_vert,
    .fragmentSource = pbr_frag
});

// Draw calls via RHI
rhi->bindPipeline(pipeline);
rhi->bindBuffer(vbo);
rhi->draw(DrawDesc{.vertexCount = vertexCount});
```

**Characteristics**:
- ✅ Fast (16ms per frame @ 1080p)
- ✅ Fully hardware-accelerated
- ✅ Real-time (60 FPS)
- ❌ Approximate reflections (SSR)
- ❌ No refraction (can't do glass properly)

### Ray Tracing Pipeline (CPU)

**Operates on CPU memory**:
```cpp
// Geometry in CPU memory
std::vector<Triangle> triangles;
for (const auto& vertex : obj.vertices) {
    triangles.push_back(Triangle{v0, v1, v2, normal, material});
}

// BVH acceleration structure (CPU)
BVHNode* bvhRoot = buildBVH(triangles);

// Ray-triangle intersection (CPU)
for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
        Ray ray = generateRay(x, y, camera);
        glm::vec3 color = traceRay(ray, bvhRoot, lights);
        output[y * width + x] = color;
    }
}

// Upload to GPU for display
rhi->updateTexture(outputTexture, output.data(), width, height, TextureFormat::RGB32F);
```

**Characteristics**:
- ✅ Physically accurate (refraction, caustics)
- ✅ Correct glass materials (Fresnel, IOR)
- ✅ Global illumination (bounce lighting)
- ❌ Slow (seconds per frame)
- ❌ Not real-time (<1 FPS)

### Why This Hybrid Design?

**Pragmatic trade-offs**:

| Aspect | CPU Raytracer | GPU Raytracer (Future) |
|--------|---------------|------------------------|
| **Implementation** | ✅ Simple (250 lines C++) | ❌ Complex (compute shaders, BVH on GPU) |
| **Portability** | ✅ Works everywhere (WebGL2, old GPUs) | ❌ Needs compute shaders (not WebGL2) |
| **Performance** | ❌ Slow (seconds) | ✅ Fast (milliseconds) |
| **Memory** | ❌ Duplicated (CPU+GPU) | ✅ Shared (GPU only) |
| **Development** | ✅ Done (v0.3.0) | ❌ 3-6 months work |

**Decision**: Ship CPU raytracer now (correct + simple), add GPU raytracer later (fast + complex).

### Future: Unified GPU Pipeline

**Goal**: Single geometry buffer, both raster and ray use GPU.

**Phase 1** (current):
```
Scene Geometry
      │
      ├──────────┬──────────┐
      ▼          ▼          ▼
  CPU Copy    GPU VBO    GPU VBO
  (raytracer) (raster)   (duplicate!)
```

**Phase 2** (future):
```
Scene Geometry
      │
      ▼
   GPU SSBO (single copy)
      │
      ├──────────┬──────────┐
      ▼          ▼          ▼
  Raster      Ray       Ray
  (vertex    (compute   (hardware
   shader)    shader)   accel)
```

**Benefits**:
- Zero duplication (geometry uploaded once)
- Faster uploads (no CPU copy)
- Real-time hybrid rendering (raster primary + ray secondary)

---

## Deterministic Systems

### Why Determinism Matters

**Non-Deterministic Rendering** (most engines):
```cpp
// ❌ Different outputs every run
float getRandom() {
    return rand() / RAND_MAX;  // Uses global state, system time
}

void update(float deltaTime) {
    // deltaTime varies based on wall clock
    rotation += speed * deltaTime;
}
```

**Problems**:
- Can't reproduce bugs ("Worked on my machine!")
- Visual regression testing breaks (images differ every time)
- Dataset generation has random variations (can't validate results)
- CI/CD tests are flaky

**Deterministic Rendering** (Glint3D):
```cpp
// ✅ Same output every run
float getRandom(SeededRng& rng) {
    return rng.uniform01();  // Explicit seed, no global state
}

void update(int frameIndex) {
    // Fixed time step based on frame counter
    float time = frameIndex * (1.0f / 60.0f);
    rotation = speed * time;
}
```

**Benefits**:
- Reproducible: Same inputs → identical outputs
- Testable: Golden images work (byte-level comparison)
- Debuggable: Can replay exact conditions
- Validatable: Agents can verify results

### Components of Determinism

#### 1. Seeded PRNG (Random Number Generation)

**Existing Implementation**:
```cpp
class SeededRng {
    uint32_t m_state;

public:
    explicit SeededRng(uint32_t seed) : m_state(seed) {}

    // Xorshift32 (deterministic, fast)
    uint32_t next() {
        m_state ^= m_state << 13;
        m_state ^= m_state >> 17;
        m_state ^= m_state << 5;
        return m_state;
    }

    float uniform01() {
        return next() / float(UINT32_MAX);
    }
};
```

**Usage**:
```cpp
// Deterministic ray tracing
SeededRng rng(m_seed);  // m_seed from --seed CLI arg
for (int sample = 0; sample < samplesPerPixel; ++sample) {
    Ray ray = generateRay(x, y, camera, rng);  // Use seeded RNG
    color += traceRay(ray, scene, lights, rng);
}
color /= samplesPerPixel;
```

#### 2. Fixed Time Stepping

**Non-Deterministic** (wall clock):
```cpp
auto now = std::chrono::high_resolution_clock::now();
float deltaTime = (now - lastTime).count();
rotation += speed * deltaTime;  // Varies with CPU load!
```

**Deterministic** (frame counter):
```cpp
float time = frameIndex * (1.0f / 60.0f);  // Fixed 60 FPS
rotation = speed * time;  // Always same at frame N
```

#### 3. Byte-Stable Floating Point

**Challenge**: FP arithmetic can vary across platforms (SSE, AVX, ARM NEON).

**Mitigation**:
- Use deterministic math libraries (GLM with consistent settings)
- Avoid `fast-math` compiler flags (breaks IEEE 754 compliance)
- Test across platforms (Windows, Linux, macOS)

**Acceptance**: Some FP differences are acceptable (±1 ULP). Validate with SSIM (perceptual similarity) instead of byte-level diff.

### Roadmap: Full Determinism (Future Tasks)

#### Task: `determinism_hooks`

**Goal**: Achieve byte-stable outputs across all platforms.

**Checklist**:
1. ✅ Seeded PRNG (done in v0.3.0)
2. 📋 Global seed propagation (task_id → frame → pass seeds)
3. 📋 Eliminate wall-clock time (use frame counter everywhere)
4. 📋 Fixed time stepping (1/60 FPS delta)
5. 📋 Byte-stability tests (render on Windows/Linux/macOS, compare)

**Acceptance Criteria**:
```bash
# Render on Windows
glint --ops scene.json --seed 42 --render win.png

# Render on Linux
glint --ops scene.json --seed 42 --render linux.png

# Compare
diff win.png linux.png  # Should be byte-identical
```

---

## Agent-Oriented Design

### Why Design for Agents?

**Traditional 3D tools** (Blender, Maya):
- Designed for humans (GUI-first)
- Manual workflows (click, drag, type)
- Difficult to automate (Python APIs are complex)

**Glint3D**:
- Designed for humans **AND** agents
- Declarative workflows (JSON-Ops)
- Easy to automate (machine-readable formats)

**Use Cases for Agents**:
1. **Content generation**: AI generates 1000 product variations
2. **Parameter optimization**: Agent tunes lighting for best quality
3. **Quality assurance**: Agent validates renders (no black frames, correct labels)
4. **Continuous learning**: Agent improves rendering parameters over time

### Design Principles

#### 1. Declarative over Imperative

**Imperative** (bad for agents):
```cpp
scene.beginTransaction();
scene.selectObject("obj_1");
scene.setPosition(1, 0, 0);
scene.setRotation(0, 45, 0);
scene.commitTransaction();
```
- ❌ Stateful (order matters)
- ❌ Error-prone (forget commit → corrupted state)
- ❌ Hard to parallelize

**Declarative** (good for agents):
```json
{
  "op": "transform",
  "target": "obj_1",
  "translate": [1, 0, 0],
  "rotate": [0, 45, 0]
}
```
- ✅ Stateless (order doesn't matter for independent ops)
- ✅ Atomic (all-or-nothing)
- ✅ Parallelizable (agents can run 1000 ops in parallel)

#### 2. Pure Functions over Side Effects

**Impure** (bad for agents):
```cpp
float getRandomColor() {
    return rand() / RAND_MAX;  // ❌ Hidden global state
}

Texture* loadTexture(const char* path) {
    return g_textureCache.load(path);  // ❌ Hidden global cache
}
```

**Pure** (good for agents):
```cpp
float getRandomColor(SeededRng& rng) {
    return rng.uniform01();  // ✅ Explicit dependency
}

Texture* loadTexture(const char* path, TextureCache& cache) {
    return cache.load(path);  // ✅ Explicit dependency
}
```

**Benefits**:
- Reproducible (same inputs → same outputs)
- Testable (can mock dependencies)
- Debuggable (no hidden state)

#### 3. Structured Data over Unstructured Logs

**Unstructured** (bad for agents):
```cpp
std::cout << "Rendered 100 triangles in 2.5ms" << std::endl;
```
- ❌ Hard to parse (regex needed)
- ❌ No schema (format can change)
- ❌ Not machine-readable

**Structured** (good for agents):
```ndjson
{"ts": "2025-10-08T12:00:00Z", "event": "pass_end", "pass": "RasterPass", "triangles": 100, "time_ms": 2.5}
```
- ✅ Easy to parse (JSON parser)
- ✅ Schema-validated (enforce structure)
- ✅ Machine-readable (agents can analyze)

**Benefits**:
- Dashboards: Parse logs into Grafana/Prometheus
- Alerts: Trigger if `time_ms > 50`
- Analysis: ML models predict performance

#### 4. Content-Addressed over File Paths

**Path-Dependent** (bad for agents):
```cpp
Texture* tex = loadTexture("assets/models/car/diffuse.png");
```
- ❌ Breaks if file moves
- ❌ Hard to cache (path != content)
- ❌ No versioning

**Content-Addressed** (good for agents):
```cpp
ResourceID id = hashAsset("diffuse.png", compressionParams);
Texture* tex = cache.get(id);  // ✅ Location-independent
```
- ✅ Survives file reorganization
- ✅ Deduplication (same content = same hash)
- ✅ Version control (different content = different hash)

**Benefits**:
- Reproducible builds (hash identifies exact asset version)
- Instant reuse (cache hit across projects)
- Distributed rendering (content-addressed storage)

### How This Benefits Developers

**Easier Testing**:
```cpp
// Pure function → easy to unit test
TEST(Material, CreateMetal) {
    auto mat = MaterialCore::createMetal(glm::vec3(0.9f), 0.1f);
    EXPECT_EQ(mat.metallic, 1.0f);
    EXPECT_EQ(mat.roughness, 0.1f);
}
```

**Better Debugging**:
```bash
# Structured logs → filter by pass
cat telemetry.ndjson | jq 'select(.pass == "RasterPass")'

# Find slow passes
cat telemetry.ndjson | jq 'select(.time_ms > 20) | .pass'
```

**Safer Refactoring**:
```cpp
// Pure function → can't break distant code
float computePBR(MaterialCore mat, Light light, vec3 normal) {
    // No side effects, no global state
    // Refactor freely!
}
```

**Future-Proof Code**:
- Agent-friendly code is also human-friendly
- JSON-Ops enable new UIs without engine changes
- Telemetry enables new analytics without engine changes

---

# Part V: Development Guide

## Key Concepts to Learn

### 1. Graphics APIs (Critical - Start Here)

#### OpenGL (Core Profile 3.3+)
**Priority**: **HIGH** - This is what Glint3D uses today.

**What to Learn**:
- **Vertex Pipeline**: VBO (Vertex Buffer Objects), IBO (Index Buffer Objects), VAO (Vertex Array Objects)
- **Shaders**: GLSL vertex/fragment shaders, uniforms, attributes
- **Textures**: 2D textures, cubemaps, mipmaps, texture filtering
- **Framebuffers**: Render-to-texture, MSAA (multisampling)
- **Uniforms**: Single values vs Uniform Buffer Objects (UBO)

**Resources**:
- **[LearnOpenGL.com](https://learnopengl.com/)** - **START HERE!** Excellent, comprehensive tutorials
  - Read: Getting Started → Lighting → Model Loading (minimum)
  - Optional: Advanced OpenGL, PBR sections
- [OpenGL 3.3 Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/) - Official docs

**Key Concept**: OpenGL is a state machine. You:
1. Bind resources (buffers, textures, shaders)
2. Set state (depth test, blending, viewport)
3. Issue draw calls

**Time Investment**: 2-3 weeks (30-40 hours)

#### RHI Abstraction Layer
**Priority**: **HIGH** - Understand how Glint3D wraps OpenGL.

**What to Learn**:
- Why abstract graphics APIs? (portability, cleaner code)
- Handle-based resource management (vs raw pointers/IDs)
- Command buffers and encoders (WebGPU/Vulkan style)
- Backend swapping (OpenGL today, Vulkan tomorrow)

**Resources**:
- Read `engine/include/glint3d/rhi.h` - Well-documented interface (400 lines)
- Read `engine/src/rhi/rhi_gl.cpp` - OpenGL implementation (1000 lines)
- [WebGPU Spec](https://www.w3.org/TR/webgpu/) - Modern API design (we mimic this)

**Key Concept**: RHI provides **platform-agnostic** API. Code uses `rhi->createTexture()` instead of `glGenTextures()`.

**Time Investment**: 1 week (10 hours)

---

### 2. Physically Based Rendering (PBR) (High Priority)

**What to Learn**:
- **Microfacet theory**: Cook-Torrance BRDF, GGX distribution
- **Metallic/roughness workflow**: Dielectric vs metallic surfaces
- **Image-based lighting (IBL)**: Environment maps, prefiltered reflections
- **Tone mapping**: HDR → LDR (ACES, Reinhard, Filmic)
- **Color spaces**: sRGB vs linear, gamma correction

**Resources**:
- **[Physically Based Rendering Book](https://pbr-book.org/)** - The bible (reference, not tutorial)
- **[Filament PBR Guide](https://google.github.io/filament/Filament.md.html)** - **BEST RESOURCE!** Practical, excellent diagrams
- [LearnOpenGL PBR](https://learnopengl.com/PBR/Theory) - Good intro, hands-on

**Key Files to Read**:
- `engine/shaders/pbr.frag` - Our PBR shader implementation (300 lines)
- `engine/include/glint3d/material_core.h` - Material parameters

**Key Concept**: PBR uses physically accurate equations (energy conservation, Fresnel). This makes materials look realistic under any lighting, unlike ad-hoc Phong/Blinn-Phong.

**Time Investment**: 2-3 weeks (20-30 hours)

---

### 3. Ray Tracing (Medium Priority)

**What to Learn**:
- **Ray-primitive intersection**: Ray-triangle, ray-sphere, ray-plane
- **Acceleration structures**: BVH (Bounding Volume Hierarchy), kd-trees
- **Path tracing**: Monte Carlo integration, importance sampling
- **BRDF sampling**: Sampling microfacet lobes
- **Russian roulette**: Unbiased early ray termination

**Resources**:
- **[Ray Tracing in One Weekend](https://raytracing.github.io/)** - **START HERE!** Quick, practical series (3 books)
- [PBRT Book](https://pbr-book.org/) - In-depth theory (Chapters 7-14 relevant)
- [Scratchapixel](https://www.scratchapixel.com/) - Great fundamentals

**Key Files to Read**:
- `engine/src/raytracer.cpp` - Our path tracer (500 lines)
- `engine/include/bvh_node.h` - BVH acceleration structure
- `engine/src/microfacet_sampling.cpp` - BRDF importance sampling

**Key Concept**: Ray tracing simulates light transport by shooting rays from the camera. Slow but physically accurate (caustics, refraction, global illumination).

**Time Investment**: 2-3 weeks (20-30 hours)

---

### 4. Render Graphs and Pass-Based Rendering (Medium Priority)

**What to Learn**:
- **Why passes?** Modularity, profiling, flexibility
- **Dependency graphs**: Pass A outputs texture, Pass B inputs it
- **Deferred rendering**: G-buffer passes (geometry → lighting)
- **Post-processing chains**: Bloom → DOF → Tone mapping

**Resources**:
- **[FrameGraph by Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in)** - Industry standard (GDC talk, 60 min)
- [Render Graphs - Yuriy O'Donnell](https://ourmachinery.com/post/high-level-rendering-using-render-graphs/) - Good blog post

**Key Files to Read**:
- `engine/include/render_pass.h` - Pass interface and built-in passes (300 lines)
- `engine/src/render_system.cpp:renderUnified()` - How passes are executed

**Key Concept**: Instead of one big render function, break rendering into **composable passes** (geometry → lighting → post-processing).

**Time Investment**: 1-2 weeks (10-20 hours)

---

### 5. WebGL2 and Emscripten (Low Priority - Web Platform)

**What to Learn** (only if working on web platform):
- **WebGL2 limitations**: No geometry shaders, no compute, limited texture formats
- **Emscripten**: C++ → WebAssembly compiler
- **JavaScript ↔ WASM interop**: Function calls, memory sharing

**Resources**:
- [WebGL2 Fundamentals](https://webgl2fundamentals.org/) - Good intro
- [Emscripten Docs](https://emscripten.org/docs/getting_started/index.html) - Official docs

**Key Concept**: WebGL2 is OpenGL ES 3.0. More restricted than desktop OpenGL but runs in browsers.

**Time Investment**: 1 week (10 hours)

---

### 6. Deterministic Systems and Telemetry (Medium Priority - Future Focus)

**What to Learn**:
- **Deterministic execution**: Fixed time stepping, seeded RNG
- **Content-addressed caching**: Hash-based resource lookup
- **Structured logging**: NDJSON event streams
- **Byte-level reproducibility**: Floating point determinism, platform differences

**Resources**:
- [Content-Addressed Storage](https://en.wikipedia.org/wiki/Content-addressable_storage) - Hash-based lookup
- [Deterministic Simulation](https://gafferongames.com/post/deterministic_lockstep/) - Networking/replay systems
- [NDJSON Format](http://ndjson.org/) - Newline-Delimited JSON spec
- [Reproducible Builds](https://reproducible-builds.org/) - Build determinism principles

**Key Concept**: The system is moving toward **byte-stable outputs**—same inputs (JSON-Ops + seed) produce identical results across platforms. This enables automated testing, visual regression detection, and agent-driven workflows.

**Why It Matters**: As you write new features, consider:
- "Does this use random numbers?" → Use `SeededRng` with task-scoped seed
- "Does this use wall-clock time?" → Use frame counter or deterministic delta
- "Can this be described as JSON-Ops?" → Design for declarative specification

**Time Investment**: 1-2 weeks (reading + experimentation)

---

### 7. Advanced Topics (Optional)

#### Gaussian Splatting (Experimental)
**Status**: Not yet implemented in Glint3D, but planned.

**What It Is**: Novel technique for rendering 3D scenes using 3D Gaussians instead of triangles. Allows real-time rendering of photogrammetry/NeRF-like scenes.

**Resources**:
- [3D Gaussian Splatting Paper](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/) - Original paper + code
- [Gaussian Splatting Explained](https://medium.com/@AriaLeeNotAriel/numbynum-3d-gaussian-splatting-for-real-time-radiance-field-rendering-kerbl-et-al-60c0b25e5544) - Accessible explanation

**Why It Matters**: Future direction for real-time photorealistic rendering. Different paradigm from triangle rasterization.

**Time Investment**: 2-3 days (optional exploration)

#### Image-Based Lighting (IBL)
**Status**: Partially implemented (see `engine/include/ibl_system.h`)

**What to Learn**:
- HDR environment maps (equirectangular format)
- Prefiltered cubemaps (diffuse irradiance, specular prefilter)
- Split-sum approximation (BRDF LUT)

**Resources**:
- [LearnOpenGL IBL](https://learnopengl.com/PBR/IBL/Diffuse-irradiance) - Good tutorial series
- [Unreal Engine IBL](https://docs.unrealengine.com/5.0/en-US/image-based-lighting-in-unreal-engine/) - Production approach

**Time Investment**: 1 week (10 hours)

#### AI Denoising
**Status**: Integrated (Intel Open Image Denoise)

**What It Is**: Use neural networks to remove noise from ray-traced images with fewer samples. Trades GPU time for quality.

**Resources**:
- [OIDN Docs](https://www.openimagedenoise.org/) - Library we use
- [NVIDIA OptiX Denoiser](https://developer.nvidia.com/optix-denoiser) - Alternative approach

**Time Investment**: 2-3 days (integration, not theory)

---

## Code Navigation Guide

### Where to Find Things

#### Rendering Code
| What | File Path |
|------|-----------|
| **Main loop** | `engine/src/application_core.cpp:mainLoop()` |
| **Unified render entry** | `engine/src/render_system.cpp:renderUnified()` |
| **Raster pipeline** | `engine/src/render_system.cpp:passRaster()` |
| **Ray pipeline** | `engine/src/raytracer.cpp:renderImage()` |
| **PBR shader** | `engine/shaders/pbr.frag` |
| **RHI interface** | `engine/include/glint3d/rhi.h` |
| **RHI OpenGL impl** | `engine/src/rhi/rhi_gl.cpp` |

#### Asset Loading
| What | File Path |
|------|-----------|
| **Model import** | `engine/src/importers/` (OBJ, Assimp plugins) |
| **Texture loading** | `engine/src/texture_cache.cpp` |
| **Scene management** | `engine/include/managers/scene_manager.h` |
| **Material system** | `engine/include/glint3d/material_core.h` |

#### Platform-Specific Code
| What | File Path |
|------|-----------|
| **Desktop UI** | `engine/src/ui/` (ImGui panels) |
| **Web UI** | `platforms/web/src/` (React components) |
| **CLI** | `engine/src/application_core.cpp` (headless mode) |

#### JSON Operations
| What | File Path |
|------|-----------|
| **API definition** | `engine/include/json_ops.h` |
| **Implementation** | `engine/src/json_ops.cpp` |
| **Schema** | `schemas/json_ops_v1.json` |
| **Examples** | `examples/json-ops/*.json` |

#### Tests
| What | File Path |
|------|-----------|
| **Unit tests** | `tests/unit/` |
| **Golden images** | `tests/golden/` (visual regression) |
| **Security tests** | `tests/security/` (path traversal, etc.) |

---

### How to Add a New Feature

#### Example: Adding a New Material Property

Let's say you want to add **anisotropy** (brushed metal effect) to materials.

**Step 1: Update MaterialCore**
```cpp
// engine/include/glint3d/material_core.h
struct MaterialCore {
    // ... existing fields
    float anisotropy{0.0f};  // ✅ Add your property [-1, 1]
    float anisotropyRotation{0.0f};  // Rotation angle [0, 360]
};
```

**Step 2: Update PBR Shader**
```glsl
// engine/shaders/pbr.frag
uniform float u_anisotropy;
uniform float u_anisotropyRotation;

// In computePBR() function:
vec3 computePBR(...) {
    // ... existing code

    // Add anisotropic roughness
    vec3 tangent = computeTangent(normal, anisotropyRotation);
    float roughnessT = mix(roughness, 1.0, abs(anisotropy));
    float roughnessB = roughness;

    // Use anisotropic GGX
    float D = GGX_anisotropic(roughnessT, roughnessB, ...);

    // ... rest of PBR calculation
}
```

**Step 3: Update RenderSystem to Bind Uniform**
```cpp
// engine/src/render_system.cpp
void RenderSystem::renderObject(const SceneObject& obj, ...) {
    // ... existing uniforms
    m_rhi->setUniformFloat("u_anisotropy", obj.materialCore.anisotropy);
    m_rhi->setUniformFloat("u_anisotropyRotation", obj.materialCore.anisotropyRotation);
}
```

**Step 4: Update JSON Schema**
```json
// schemas/json_ops_v1.json
{
  "definitions": {
    "MaterialOp": {
      "properties": {
        "anisotropy": {
          "type": "number",
          "minimum": -1.0,
          "maximum": 1.0,
          "description": "Anisotropic roughness (-1=tangent, 0=isotropic, 1=bitangent)"
        },
        "anisotropyRotation": {
          "type": "number",
          "minimum": 0,
          "maximum": 360,
          "description": "Anisotropy direction in degrees"
        }
      }
    }
  }
}
```

**Step 5: Update JSON-Ops Handler**
```cpp
// engine/src/json_ops.cpp
void applySetMaterialOp(const nlohmann::json& op, SceneManager& scene) {
    // ... existing properties
    if (op.contains("anisotropy")) {
        obj.materialCore.anisotropy = op["anisotropy"].get<float>();
    }
    if (op.contains("anisotropyRotation")) {
        obj.materialCore.anisotropyRotation = op["anisotropyRotation"].get<float>();
    }
}
```

**Step 6: Add Test Case**
```json
// tests/integration/anisotropy_test.json
{
  "version": "0.1",
  "operations": [
    {"op": "load", "path": "assets/models/sphere.obj"},
    {"op": "set_material", "baseColor": [0.8, 0.8, 0.8], "metallic": 1.0, "roughness": 0.3, "anisotropy": 0.9},
    {"op": "render", "output": "anisotropic_sphere.png", "width": 512, "height": 512}
  ]
}
```

**Step 7: Test**
```bash
# Build
cmake --build builds/desktop/cmake -j

# Test
./builds/desktop/cmake/Release/glint.exe --ops tests/integration/anisotropy_test.json

# Verify output
open anisotropic_sphere.png  # Should show brushed metal effect
```

**Done!** You've added a new material property end-to-end.

---

## Current Development Status

### Active Task: `pass_bridging`

**Status**: Ready to start (as of 2025-10-08)

**Goal**: Implement critical render passes using RHI-only calls

**Key Passes**:
1. **passGBuffer**: Render scene to G-buffer (base color, normal, position, material)
2. **passDeferredLighting**: Full-screen pass to compute lighting from G-buffer
3. **passRayIntegrator**: CPU raytracer outputs to RHI texture (no GL calls)
4. **passReadback**: Extract texture data for headless rendering

**Checklist**: See `ai/tasks/pass_bridging/checklist.md`

**Why It Matters**: Completes the pass-based architecture, enables deferred rendering and advanced effects.

---

### Critical Path Roadmap

```
✅ opengl_migration (94% complete)
    │
    ├─ All core systems now RHI-only
    ├─ 426+ GL calls eliminated
    └─ Only legacy Shader class remains (24 calls)

    ↓

🔄 pass_bridging (ready to start)
    │
    ├─ Implement passGBuffer (deferred rendering)
    ├─ Implement passDeferredLighting
    ├─ Implement passRayIntegrator (RHI texture output)
    └─ Implement passReadback (headless support)

    ↓

📋 state_accessibility (blocked by pass_bridging)
    │
    ├─ CameraManager::getState() / applyState()
    ├─ LightingManager::getState() / applyState()
    ├─ MaterialManager::getState() / applyState()
    └─ JSON serialization for all manager states

    ↓

📋 ndjson_telemetry (blocked by state_accessibility)
    │
    ├─ Structured event logging (pass timing, resource allocation)
    ├─ Buffered writer system (ring buffer, background flush)
    ├─ Integration with PassTiming
    └─ Quality metrics (SSIM, PSNR)

    ↓

📋 determinism_hooks (blocked by ndjson_telemetry)
    │
    ├─ Global seed propagation (task_id → frame → pass)
    ├─ Eliminate wall-clock time (use frame counter)
    ├─ Byte-stability validation (cross-platform tests)
    └─ Floating point precision handling

    ↓

📋 resource_model (blocked by determinism_hooks)
    │
    ├─ Handle table interface (stable resource IDs)
    ├─ Content-addressed cache (hash-based lookup)
    ├─ Provenance tracking (asset origin, processing chain)
    └─ RHI capability integration

    ↓

📋 json_ops_runner (blocked by resource_model)
    │
    ├─ Pure, idempotent operation executor
    ├─ CLI: glint --task-id --ops-file --telemetry-file
    ├─ Sandboxed execution (resource limits, timeouts)
    └─ Agent testing framework

    ↓

🎯 Agent-Driven Workflows (end goal)
    │
    └─ AI generates JSON-Ops → executes deterministically → validates via telemetry
```

**Timeline**: 6-12 months to complete all tasks

**Current Priority**: Focus on `pass_bridging` (critical path)

---

## Contributing Guidelines

### Development Workflow

1. **Check active task**: `cat ai/tasks/current_index.json`
2. **Read task specification**:
   - `ai/tasks/{task_name}/task.json` - Inputs, outputs, acceptance criteria
   - `ai/tasks/{task_name}/checklist.md` - Atomic execution steps
   - `ai/tasks/{task_name}/progress.ndjson` - Current status
3. **Make surgical changes**: Keep edits focused and task-aligned
4. **Build and test**:
   ```bash
   # Desktop build
   cmake --build builds/desktop/cmake -j

   # Run tests
   ./builds/desktop/cmake/Release/glint.exe --ops tests/integration/basic_test.json

   # Web build
   npm run web:build
   ```
5. **Update documentation**:
   - Mark completed checklist items: `[x]`
   - Append progress event to `progress.ndjson`:
     ```json
     {"ts": "2025-10-08T14:30:00Z", "task_id": "pass_bridging", "event": "step_completed", "step": "passGBuffer implementation", "status": "success"}
     ```
   - Update `current_index.json` if task status changes

---

### Code Style

- **C++17**: Use modern features (`auto`, range-for, `std::optional`, structured bindings)
- **RAII**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- **Const correctness**: Mark methods `const` when they don't modify state
- **Naming Conventions**:
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Member variables: `m_camelCase`
  - Constants: `UPPER_SNAKE_CASE`

**Example**:
```cpp
class RenderSystem {
public:
    void renderUnified(const SceneManager& scene, const Light& lights);  // camelCase

private:
    std::unique_ptr<RHI> m_rhi;                  // m_camelCase
    CameraManager m_cameraManager;               // m_camelCase
    static constexpr float DEFAULT_FOV = 45.0f;  // UPPER_SNAKE_CASE
};
```

---

### Performance Budgets

Track these metrics via `RenderStats`:

| Metric | Budget | How to Check |
|--------|--------|--------------|
| **Draw calls** | < 5,000 per frame | `m_stats.drawCalls` |
| **Triangles** | < 15 million per frame | `m_stats.totalTriangles` |
| **Frame time** | < 16ms @ 1080p | PassTiming logs |
| **VRAM** | Track growth | `m_stats.vramMB` |

**If you exceed budget**:
- Implement instancing (reduce draw calls)
- LOD/decimation (reduce triangles)
- Merge static geometry (reduce draw calls)
- Profile and optimize hot paths

---

### Testing

#### Visual Regression Testing
```bash
# Render golden scene
./builds/desktop/cmake/Release/glint.exe --ops tests/golden/scenes/raster-pipeline.json --render output.png

# Compare against reference
compare -metric SSIM tests/golden/references/raster-pipeline.png output.png diff.png

# If SSIM > 0.99 → pass
# If SSIM < 0.95 → fail (visual regression!)
```

#### Security Testing
```bash
# Test path traversal protection
bash tests/security/test_path_security.sh

# Should block:
# - ../../../etc/passwd
# - ..\\..\\system32\\config
# - URL-encoded traversals
```

#### Unit Testing
```bash
# C++ unit tests (future)
./builds/desktop/cmake/Release/glint_tests

# JavaScript unit tests (web platform)
cd platforms/web
npm test
```

---

## Learning Roadmap

### Week 1: Environment Setup & Exploration
1. **Build the project** - Get it running on your machine (desktop + web)
2. **Run examples** - Try `examples/json-ops/*.json` to see features
3. **Read active task** - `cat ai/tasks/current_index.json` to see current focus
4. **Read LearnOpenGL** - Start with "Getting Started" section (fundamentals)

### Week 2-3: Graphics Fundamentals
1. **Complete LearnOpenGL** - Finish through "Lighting" and "Model Loading"
2. **Read RHI code** - `engine/include/glint3d/rhi.h` with OpenGL knowledge
3. **Modify a shader** - Tweak `engine/shaders/pbr.frag`, add a tint parameter
4. **Trace a render** - Set breakpoints, follow a frame through `renderUnified()`

### Week 4: First Contribution
Pick **ONE** based on active task and interest:

#### Beginner Tasks (Standalone, low risk)
- **Add material preset**: Implement `MaterialCore::createCopper()`, `createPlastic()`
- **Improve PBR shader**: Add anisotropy or clearcoat support
- **Optimize BVH**: Implement SAH (Surface Area Heuristic) splitting
- **Add camera preset**: Implement "isometric" or "orthographic" preset

#### Intermediate Tasks (Touches multiple systems)
- **Implement new render pass**: `SSAOPass`, `BloomPass`, or `DOFPass`
- **Add JSON-Ops validation**: JSON schema enforcement with clear error messages
- **Performance profiling**: Identify and fix bottlenecks in hot paths
- **Texture compression**: Add KTX2/Basis Universal support

#### Advanced Tasks (Requires understanding of task system)
- **Help with `pass_bridging`**: Implement `passGBuffer` or `passDeferredLighting`
- **State accessibility prep**: Design `CameraState` structure and JSON schema
- **NDJSON telemetry prep**: Design event schema for performance tracking
- **Resource cache**: Implement content-addressed texture cache

### Month 2+: Task System Contributions

Once comfortable with codebase:
1. **Pick an active task** - Check `ai/tasks/current_index.json`
2. **Read task files** - `task.json`, `checklist.md`, `progress.ndjson`
3. **Follow checklist** - Complete items systematically (one at a time)
4. **Update documentation** - Mark completed items, emit progress events:
   ```json
   {"ts": "2025-10-08T15:00:00Z", "task_id": "pass_bridging", "event": "step_completed", "step": "passGBuffer validation", "status": "success"}
   ```
5. **Test thoroughly** - Ensure all acceptance criteria pass:
   ```bash
   # Example: passGBuffer acceptance test
   ./glint --ops tests/pass_bridging/gbuffer_test.json --render gbuffer_output.png
   compare -metric SSIM gbuffer_output.png tests/pass_bridging/gbuffer_reference.png diff.png
   # SSIM should be > 0.99
   ```

---

### Learning Resources by Phase

#### Phase 1 (Foundations - Week 1-3)
- **[LearnOpenGL](https://learnopengl.com/)** - Essential reading (30-40 hours)
- **Glint3D codebase headers** - Well-documented, read them! (10 hours)
- **`CLAUDE.md`** - Development workflows and commands (1 hour)

#### Phase 2 (Advanced Graphics - Week 4+)
- **[Filament PBR Guide](https://google.github.io/filament/Filament.md.html)** - PBR theory (10-15 hours)
- **[Ray Tracing in One Weekend](https://raytracing.github.io/)** - Ray tracing basics (10-15 hours)
- **`docs/ARCHITECTURE_NOTES.md`** - Project-specific decisions (2 hours)

#### Phase 3 (System Architecture - Month 2+)
- **[FrameGraph GDC Talk](https://www.gdcvault.com/play/1024612/)** - Render graphs (1 hour)
- **[Deterministic Lockstep](https://gafferongames.com/post/deterministic_lockstep/)** - Determinism (2 hours)
- **[NDJSON Format](http://ndjson.org/)** - Structured logging (30 min)
- **Task system docs**: `ai/tasks/README.md` (1 hour)

---

## Related Documents

- **`CLAUDE.md`** - Development workflows and commands
- **`PRODUCT_VISION.md`** - Market positioning, use cases, roadmap (archived, see this doc)
- **`ARCHITECTURE_NOTES.md`** - Technical decisions (archived, see this doc)
- **`ai/tasks/README.md`** - Task system instructions
- **`ai/tasks/current_index.json`** - Current active task

---

## Quick Reference

### Key Commands

```bash
# Build desktop (Release)
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# Build desktop (Debug)
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build builds/desktop/cmake --config Debug -j

# Build web
npm run web:build

# Run interactive
./builds/desktop/cmake/Release/glint.exe

# Run headless
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output.png

# With denoising
./builds/desktop/cmake/Release/glint.exe --ops scene.json --denoise --render denoised.png

# Deterministic render
./builds/desktop/cmake/Release/glint.exe --ops scene.json --seed 42 --render deterministic.png
```

### Architecture Mantras

- **Use RHI, never raw OpenGL** - All GPU work goes through RHI abstraction
- **MaterialCore only** - Single material struct for all pipelines
- **Pass-based design** - Add passes to RenderGraph, don't create new pipelines
- **JSON Ops first** - Test features via JSON ops before adding UI
- **Determinism matters** - Use SeededRng, no wall-clock time, byte-stable outputs
- **Agent-friendly** - Declarative, pure, structured, content-addressed

---

**Welcome to Glint3D! Let's build the future of 3D visualization together.** 🚀

---

*Last Updated: 2025-10-08 | Version 1.0*
*For the latest updates, check `ai/tasks/current_index.json`*
