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
    ParticleConsts& consts;
};

// Simulation 단계
struct SimulationContext : ParticleContext {
    float dt;
    float time;
    ConstantBuffer<ParticleConsts>& constBuffer;
    AppendBuffer<Particle>& consumeBuffer;
};

// Render 단계
struct RenderContext : ParticleContext {
    ID3D11ShaderResourceView* particleSRV;
    ID3D11ShaderResourceView* countSRV;
    ID3D11Buffer* indirectArgsBuffer;
};

} // namespace DE