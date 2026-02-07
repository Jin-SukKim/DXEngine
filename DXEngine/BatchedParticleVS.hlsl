#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<BatchEmitterInfo> batchInfo : register(t12);

struct GSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float lifeRatio : TEXCOORD0;
    float size : PSIZE1;
    float rotation : PSIZE2;
    nointerpolation uint emitterID : BLENDINDICES;
};

GSInput main(uint vertexID : SV_VertexID)
{
    // vertexID가 속한 emitter 찾기 (선형 탐색, 보통 <10개)
    uint emitterIdx = 0;
    for (uint i = 0; i < batchEmitterCount; i++)
    {
        BatchEmitterInfo info = batchInfo[batchInfoOffset + i];
        if (vertexID < info.batchVertexStart + info.particleCount)
        {
            emitterIdx = i;
            break;
        }
    }

    BatchEmitterInfo entry = batchInfo[batchInfoOffset + emitterIdx];
    uint localVertexID = vertexID - entry.batchVertexStart;
    uint myEmitterID = entry.globalEmitterID;

    Particle p = readParticles[entry.readParticleOffset + localVertexID];

    GSInput output;

    SpawnConsts spawn = consts[myEmitterID].spawn;
    if (spawn.simulationSpace == 1)
    {
        output.position = float4(p.position.xyz, 1.0);
    }
    else
    {
        output.position = mul(float4(p.position.xyz, 1.0), pWorld);
    }

    output.rotation = p.rotation.x;
    output.color = p.color;
    output.life = p.life;
    output.lifeRatio = 1.0 - saturate(p.life / p.lifeMax);
    output.size = p.size;
    output.emitterID = myEmitterID;

    return output;
}
