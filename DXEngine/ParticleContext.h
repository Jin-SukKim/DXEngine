#pragma once
#include "pch.h"
#include "Particle.h"
#include "AppendBuffer.h"
#include "IndirectArgsBuffer.h"
#include "MeshData.h"

namespace DE {

class RenderModule;
class MaterialModule;
class ParticleSystem;

//=============================================================================
// ParticleInitContext - 초기화 시 사용
//=============================================================================
struct ParticleInitContext {
    ID3D11Device* device;
    ParticleConsts& consts;
    ParticleFrameConsts& frameConsts;
    DrawIndexedInstancedArgs& meshArgs;
    RenderModule* renderModule;
    UINT emitterID;

    // Custom Spawn Position
    std::vector<Vector3>& customPositions;
    bool& usingCustomPositions;
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
    ID3D11Buffer* dispatchArgs;
    UINT argsOffset;
    StructuredBuffer<Vector3>& bakedSpawnPos;
    StructuredBuffer<Vector3>& customPosBuffer;
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