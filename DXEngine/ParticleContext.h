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
};

// Base Context (공통 정보)
struct ParticleContext {
    ID3D11DeviceContext* context;
    ConstantBuffer<ParticleConsts>& constBuffer;
    ConstantBuffer<ParticleFrameConsts>& frameConstBuffer;
};

// Simulation 단계
struct SimulationContext : ParticleContext {
    float dt;
    float time;
    AppendBuffer<Particle>& consumeBuffer;
    AppendBuffer<Particle>& appendBuffer;
    ID3D11ShaderResourceView* countSRV;
    ID3D11Buffer* dispatchArgs;
    ID3D11Device* device;
    RenderModule* renderModule;
    
    // Mesh 데이터 (Vertex/Surface Spawn용)
    StructuredBuffer<Vertex>* meshVertex = nullptr;
    StructuredBuffer<uint32_t>* meshIndices = nullptr;
    UINT vertexCount = 0;
    UINT indexCount = 0;
    
    // Baked Spawn Position 데이터 (Texture Spawn용)
    StructuredBuffer<Vector3>* bakedSpawnPos = nullptr;
    UINT bakedCount = 0;
};

// Render 단계
struct RenderContext : ParticleContext {
    ID3D11ShaderResourceView* particleSRV;
    MaterialModule* materialModule;
};

} // namespace DE