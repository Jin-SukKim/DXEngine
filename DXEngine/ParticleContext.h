#pragma once
#include "pch.h"
#include "Particle.h"
#include "AppendBuffer.h"
#include "IndirectArgsBuffer.h"
#include "MeshData.h"

namespace DE {

    class RenderModule;
    class MaterialModule;

struct ParticleInitContext {
    ID3D11Device* device;
    ParticleConsts& consts;
    ParticleFrameConsts& frameConsts;
    DrawIndexedInstancedArgs& meshArgs;
    UINT emitterID;
};

struct ParticleContext {
    ID3D11DeviceContext* context;
    ParticleConsts& constsData;
    ParticleFrameConsts& frameConstData;
    StructuredBuffer<Particle>& readParticles;
    StructuredBuffer<Particle>& writeParticles;
    StructuredBuffer<uint32_t>& readCount;
    StructuredBuffer<uint32_t>& writeCount;
};

struct SimulationContext : ParticleContext {
    float dt;
    float time;
    ID3D11Buffer* dispatchArgs;
    UINT argsOffset;
    ID3D11Device* device;
    RenderModule* renderModule;
    
    StructuredBuffer<Vector3>& bakedSpawnPos;
    StructuredBuffer<Vector3>* customPositions = nullptr;
    UINT bakedCount = 0;
    
    IndirectArgsBuffer<DrawIndexedInstancedArgs>& meshArgsBuffer;
};

struct RenderContext : ParticleContext {
    MaterialModule* materialModule;
    const UINT& emitterID;

    ID3D11Buffer* billboardArgs;
    UINT billbaordArgsOffset;
    IndirectArgsBuffer<DrawIndexedInstancedArgs>& meshArgsBuffer;
    UINT meshArgsOffset;
};

} // namespace DE