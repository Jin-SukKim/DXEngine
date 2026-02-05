#include "Common.hlsli"
#include "ParticleCommon.hlsli"

struct GSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float life : PSIZE0;
    float lifeRatio : TEXCOORD0;
    float size : PSIZE1;
    float rotation : PSIZE2;
    uint emitterID : PSIZE3;
};

GSInput main(uint vertexID : SV_VertexID)
{
    uint particleIdx = vertexID;

    Particle p = readParticles[particleIdx];

    GSInput output;

    // ¡Ú ownerID´Â ±Û·Î¹ú ½½·Ô ÀÎµ¦½º
    uint globalEmitterSlot = p.ownerID;
    EmitterID id = emitterIDs[globalEmitterSlot];
    output.emitterID = globalEmitterSlot; // ¡Ú ±Û·Î¹ú ½½·Ô ÀÎµ¦½º Àü´Þ

    // ¡Ú constsµµ ±Û·Î¹ú ½½·Ô ÀÎµ¦½º·Î Á¢±Ù
    SpawnConsts spawn = consts[globalEmitterSlot].spawn;
    
    // ¡Ú MeshConsts¿¡¼­ world Çà·Ä °¡Á®¿À±â (systemSlotÀ¸·Î ÀÎµ¦½Ì)
    ParticleMeshConsts mesh = meshConsts[id.systemSlot];
    
    if (spawn.simulationSpace == 1) // World Space
    {
        output.position = float4(p.position.xyz, 1.0);
    }
    else // Local Space
    {
        output.position = mul(float4(p.position.xyz, 1.0), mesh.pWorld);
    }

    output.rotation = p.rotation.x;
    output.color = p.color;
    output.life = p.life;
    output.lifeRatio = 1.0 - saturate(p.life / p.lifeMax);
    output.size = p.size;

    return output;
}