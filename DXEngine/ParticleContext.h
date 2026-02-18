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
struct ArgsParam;

//=============================================================================
// ParticleInitContext - �ʱ�ȭ �� ���
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
};

// SimulationContext - SpawnPos ���� ����
struct SimulationContext : ParticleContext {
    float dt;
    ArgsParam* dispatchArgs;
    ParticleFrameConsts* fsConsts = nullptr;
};

} // namespace DE