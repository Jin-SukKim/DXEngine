# CLAUDE.md

Use Korean to communicate with users
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

This is a Visual Studio MSBuild project (platform toolset v142). Open `DXEngine.sln` in Visual Studio 2019/2022.

```bash
# Command-line build (requires VS Build Tools)
msbuild DXEngine.sln /p:Configuration=Debug /p:Platform=x64
msbuild DXEngine.sln /p:Configuration=Release /p:Platform=x64
```

- **Primary config**: Debug|x64 and Release|x64 (x64 is the actively developed platform)
- **C++ standard**: C++20 (`/std:c++latest`)
- **Precompiled header**: `pch.h` (all `.cpp` files must include it; `pch.cpp` creates it)
- **Output**: `Binaries/<Platform>/<Configuration>/DXEngine/`
- **Intermediate**: `Intermediate/<Platform>/<Configuration>/DXEngine/`
- **Post-build**: Copies `assimp-vc142-mt.dll` from `libs/assimp/bin/` to output directory
- **Preprocessor** (x64): `_CRT_SECURE_NO_WARNINGS`, `NOMINMAX`
- **OpenMP**: Enabled in Debug|x64; disabled in Release|x64
- **Release|x64** enables AVX2 (`/arch:AVX2`), whole program optimization, and COMDAT folding

There are no unit tests. Testing is done via the built-in ParticleEditor stress test (`StressTest()` method) and runtime profiling.

## Dependencies

- **DirectX 11**: d3d11.lib, d3dcompiler.lib, dxgi.lib (system)
- **DirectXTK**: Math and graphics utilities (via vcpkg, installed at `C:\Study\vcpkg\installed\x64-windows\` for Win32 configs)
- **Assimp** (v142): 3D model loading, bundled in `libs/assimp/` (headers, lib, DLL)
- **ImGui**: Immediate-mode GUI for the particle editor (via vcpkg)
- **nlohmann/json**: Particle effect JSON serialization (via vcpkg)

## Project Overview

DXEngine is a DirectX 11 graphics engine focused on GPU-accelerated particle systems and real-time visual effects. All source files are flat in `DXEngine/` (no subdirectories for C++/HLSL). All shaders are HLSL Shader Model 5.0.

## Architecture

### Namespace and Conventions

- Namespace: `DE`
- Singletons via macros in `Common.h`: `GENERATE_SINGLE(ClassName)` and access via `GET_SINGLE(ClassName)`
- Inheritance helper: `GENERATE_BODY(ClassName, ParentClass)` defines `using Super = ParentClass;` and constructor/destructor
- Key singletons: `RenderBase`, `InputManager`, `ParticleManager`, `TextureManager`, `ModelManager`

### Object Hierarchy

`Object` (base, has name/ID) -> `Actor` (has components, transform, visibility) -> specialized actors.

Components are looked up by `ComponentType` enum: `Transform`, `Model`, `BoundingVolume`, `ParticleSystem`.

`EffectActor` is the base for all particle effect actors (Firework, MagicEffect, PortalEffect, IceEffect, PhoenixEffect, etc.).

### Rendering Pipeline

`RenderBase` manages the D3D11 device/context and drives the frame loop: `preUpdate -> update -> render -> postRender`.

Pipeline State Objects (PSO) are split into `GraphicsPSO` and `ComputePSO`. Shader/state initialization lives in `GraphicsCommon` (graphics) and `ComputeCommon` (compute). `D3D11Utils.h` has template utilities for buffer creation.

Rendering passes: depth-only -> shadow maps -> opaque objects -> billboards -> particles -> post-processing (bloom down/up sampling, tone mapping, fog, halo, copy/composition).

HDR rendering uses float render targets. Post-processing is handled through `ImageFilter` subclasses (`BloomEffect`, `ToneMappingFilter`, `FogEffect`, `CopyFilter`, `DepthFilter`).

### Particle System (core subsystem)

This is the most complex part of the codebase. The data flow is:

```
ParticleEmitter (modules define behavior)
  -> ParticleSystem (aggregates emitters, manages lifecycle)
    -> ParticleManager (singleton, manages all systems + memory pool)
      -> ParticleMemoryPool (GPU memory allocation, compacting, defragmentation)
        -> Compute Shaders (SpawnCS, ParticleCS, ParticleArgsUpdateCS)
          -> Rendering (billboard via GS, or mesh instancing)
```

**Modules** (composable per-emitter): `SpawnModule`, `ForceModule`, `VisualModule`, `RenderModule`, `OrbitModule`, `VortexModule`, `MaterialModule`. Created via `ParticleModuleFactory`.

**GPU particle pipeline**: Particles are simulated entirely on GPU via compute shaders. `StructuredBuffer<T>` wraps SRV/UAV pairs for double-buffered read/write. `AppendBuffer` handles GPU compacting. `IndirectArgsBuffer` enables GPU-driven dispatch and draw calls (`DrawInstancedArgs` for billboards, `DrawIndexedInstancedArgs` for meshes). `BitonicSort` sorts particles on GPU for transparency.

**Memory**: `ParticleMemoryPool` allocates ranges in a single large GPU buffer. Handles are tracked via `PoolHandle`. Supports defragmentation and GPU compacting with append buffers.

**Particle presets**: JSON files in `Assets/Particles/`, loaded by `ParticleLoader`/`ParticleJsonParser`. `FileWatcher` enables hot-reload during editing.

**Sub-emitters**: `SubEmitterEffectsActor` triggers child particle systems on events (OnStart, OnDurationEnd, OnComplete).

### Shader Files

All `.hlsl` files are in `DXEngine/`. Naming convention: `<Name><ShaderType>.hlsl` (e.g., `ParticleCS.hlsl`, `BasicPS.hlsl`, `BillboardGS.hlsl`). Shared includes use `.hlsli` extension (`Common.hlsli`, `ParticleCommon.hlsli`, `Shadow.hlsli`, `BlinnPhong.hlsli`, `ToneMapping.hlsli`, `OrbitCS.hlsli`, `VortexCS.hlsli`).

### Scene System

`Scene` is the base class. Active scenes: `ParticleEditor` (interactive editor with ImGui, stress testing) and `BasicParticleScene` (demo). Scenes manage categorized actor lists and drive update/render.

### Assets

`Assets/` contains models (FBX/GLTF with PBR textures), particle preset JSON files, textures, and other resources. Model loading uses Assimp via `ModelLoader`.
