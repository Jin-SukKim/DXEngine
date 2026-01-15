#pragma once
#include "pch.h"
#include "Particle.h"
#include "AppendBuffer.h"

namespace DE {

struct ParticleInitContext {
    ID3D11Device* device;
    ParticleConsts& consts;
};

// Base Context (공통 멤버)
struct ParticleContext {
    ID3D11DeviceContext* context;
    ConstantBuffer<ParticleConsts>& constBuffer;
};

// Simulation 단계
struct SimulationContext : ParticleContext {
    float dt;
    float time;
    AppendBuffer<Particle>& consumeBuffer;
    ID3D11ShaderResourceView* particleSRV;
    ID3D11ShaderResourceView* countSRV;
};

// Render 단계
struct RenderContext : ParticleContext {
    ID3D11ShaderResourceView* particleSRV;
    ID3D11Buffer* indirectArgsBuffer;
};

} // namespace DE