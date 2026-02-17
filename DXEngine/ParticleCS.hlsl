#include "ParticleCommon.hlsli"
#include "OrbitCS.hlsli"
#include "VortexCS.hlsli"

AppendStructuredBuffer<uint> deadIndices : register(u4);

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    uint aliveIdx = dtID.x;
    uint particleIdx = readAliveIndices[aliveIdx];
    
    Particle p = particles[particleIdx];
    
    EmitterID eID = emitterIDs[p.ownerID];

    uint emitterAliveCount = readAliveCount[p.ownerID];
    uint localIdx = aliveIdx - eID.readParticleOffset;
    if (localIdx >= emitterAliveCount)
        return;
    
    float dt = frameConsts[p.ownerID].dt;
    p.life -= dt;

    if (p.life <= 0.f) {
        // Return to dead list
        deadIndices.Append(particleIdx);
        return;
    }

    ForceConsts force = consts[p.ownerID].force;
    // 1. ���� ���� (Physics)

    // �߷� ����
    p.velocity += force.gravity * dt;

    // ���� ���� (Drag) ����
    // drag ���� Ŭ���� �ӵ��� 0�� ������ ����
    p.velocity *= 1.0f / (1.0f + force.drag * dt);

    // Curl Noise Force
    if (force.curlNoiseEnabled) {
        float3 curlForce = SampleCurlNoiseTiling(p.position, force.curlNoiseFrequency);
        p.velocity += curlForce * force.curlNoiseStrength * dt;
    }

    // Vortex
    VortexConsts vortex = consts[p.ownerID].vortex;
    if (vortex.active)
    {
        CalculateVortex(p, vortex, dt);
    }

    // Orbit
    OrbitConsts orbit = consts[p.ownerID].orbit;
    if (orbit.active)
    {
        CalculateOrbit(p, orbit, dt);
    }

    // ��ġ ����
    p.position += p.velocity * dt;

    // 2. �ð� ȿ�� (Visuals)
    VisualConsts visual = consts[p.ownerID].visual;
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));
    // ũ�� ���� (Start -> End)
    // sizeRange.x = Start Size, sizeRange.y = End Size
    p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);

    // ���� ���� (Start -> End)
    p.color = lerp(visual.startColor, visual.endColor, ageRatio);

    //p.rotation += p.rotSpeed * dt;
    p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

    // Write particle in-place
    particles[particleIdx] = p;

    // Append to write alive indices (compacted)
    uint writeSlot;
    InterlockedAdd(writeAliveCount[p.ownerID], 1, writeSlot);
    writeAliveIndices[eID.readParticleOffset + writeSlot] = particleIdx;
}