# Decisions
- RHI boundary: Core code never calls GL.*
- Scene graph mutation: pure functions + event log.
- Determinism policy: fixed timestep + seeded RNG + strict FP.

# Tradeoffs
- WebGL2 parity via feature flags until WebGPU backend lands.

# Pending
- Vulkan backend spike; WebGPU adapter spec.
