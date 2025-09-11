# Glint3D Architectural Reasoning

## Design Philosophy

Glint3D adopts a **"boxes with one-way arrows"** architecture to maximize AI maintainability while preserving performance and flexibility.

### Core Principles

1. **Explicit Boundaries**: Each layer has clear responsibilities and cannot access internals of other layers
2. **Intent-Based Interfaces**: AI interacts through high-level intents, not low-level implementation details
3. **Immutable Contracts**: Schemas and interfaces are versioned and backward-compatible
4. **Machine-Readable Constraints**: Architecture rules enforced by tooling, not documentation

## Layer Justification

### Engine Core
**Why separate?** The rendering engine contains complex graphics algorithms that rarely need AI modification. By isolating this layer behind stable interfaces, we protect performance-critical code while providing clear entry points for functional changes.

**Dependencies**: Only external libraries (GLM, stb, etc.)
**Forbidden**: Direct platform code (GLFW, Emscripten), UI frameworks (ImGui, React)

### Platform Layer  
**Why separate?** Desktop and web platforms have fundamentally different capabilities and constraints. Separating platform code prevents cross-contamination and enables targeted optimizations.

**Dependencies**: Engine interfaces only
**Forbidden**: Direct UI code, cross-platform assumptions

### SDK Layer
**Why separate?** The SDK provides a stable, versioned API surface for AI interactions. This prevents AI from directly manipulating engine internals and ensures changes are backward-compatible.

**Dependencies**: Only schemas for validation
**Forbidden**: Direct engine access, platform-specific code

### UI Layer
**Why separate?** UI frameworks change frequently and have different paradigms (immediate mode ImGui vs reactive React). Isolating UI code prevents framework churn from affecting core systems.

**Dependencies**: SDK for engine communication, platform for windowing
**Forbidden**: Direct engine manipulation

## Intent System Rationale

### Problem
Traditional 3D engines expose hundreds of low-level operations (setVertexBuffer, bindTexture, etc.) that require deep graphics knowledge to use correctly.

### Solution  
Glint3D exposes ~15 high-level intents (loadModel, addLight, render) that map to common 3D workflows. AI can accomplish most tasks by combining these intents.

### Benefits
- **Reduced Cognitive Load**: AI doesn't need to understand OpenGL state machines
- **Safety**: Invalid operations are caught at the intent level
- **Testability**: Each intent has clear inputs, outputs, and test coverage
- **Evolvability**: Intent implementations can change without breaking AI code

## Dependency Management

### One-Way Dependencies
```
Schemas ← SDK ← UI
         ↑     ↑
       Engine ← Platform
         ↑
      External
```

### Why One-Way?
- **Predictable Build Order**: Lower layers build first, higher layers depend on them
- **Change Isolation**: Changes in higher layers don't affect lower layers
- **Testing Strategy**: Lower layers can be tested in isolation
- **Deployment Flexibility**: Different platforms can bundle different layer combinations

## Security Boundaries

### Asset Root Restriction
All file operations are bounded by a configured asset root directory to prevent path traversal attacks. This is enforced at the engine level and propagated through all interfaces.

### Shader Validation
Shaders are validated at build time to prevent malicious code injection. Only vetted shader sources are compiled into the final binary.

### Network Isolation
The engine core has no network dependencies, preventing data exfiltration or remote code execution through graphics operations.

## Performance Architecture

### Dual Rendering Pipeline
- **Rasterization**: Real-time preview with 16ms frame budget
- **Ray Tracing**: Offline quality with unlimited time budget
- **Auto Mode**: Automatically selects appropriate pipeline based on material requirements

### Memory Management
- **Desktop**: 2GB budget for complex scenes and high-resolution textures
- **Web**: 512MB budget with asset streaming and texture compression
- **Shared**: Common asset cache to minimize memory duplication

### Binary Size Optimization
- **Desktop**: Full feature set in ~100MB executable
- **Web**: Core features only in ~5MB WASM bundle
- **Modular**: Optional features can be excluded to reduce size

## Testing Strategy

### Layer Testing
- **Unit Tests**: Each layer tested in isolation with mocked dependencies
- **Integration Tests**: Cross-layer workflows validated end-to-end
- **Golden Tests**: Visual regression detection via SSIM comparison
- **Security Tests**: Path traversal and injection protection validated

### AI Development Support
- **Deterministic Tests**: Identical outputs across runs for reliable CI
- **Performance Tests**: Frame time and memory usage regression detection  
- **Parity Tests**: Desktop and web platforms produce identical results
- **Schema Validation**: All operations validated against JSON schemas

## Evolution Strategy

### Version Management
- **Schemas**: Semantic versioning with backward compatibility guarantees
- **SDK**: Major version bumps for breaking API changes
- **Engine**: Internal versioning, stable external interfaces
- **Intents**: Additive changes only, deprecated intents marked but preserved

### Extension Points
- **New Intents**: Add to SDK → implement in engine → update schemas
- **New Platforms**: Implement platform interface → integrate with build system
- **New Renderers**: Implement RHI backend → integrate with pipeline
- **New Materials**: Extend material schema → update shader system

This architecture enables AI to effectively maintain and extend Glint3D while preserving the performance and reliability expected from a production graphics engine.