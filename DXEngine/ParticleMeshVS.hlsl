#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D g_heightTexture : register(t0);
StructuredBuffer<SortElement> sortedElements : register(t2);

float3x3 GetRotationMatrix(float3 rot)
{
    float cX = cos(rot.x), sX = sin(rot.x);
    float cY = cos(rot.y), sY = sin(rot.y);
    float cZ = cos(rot.z), sZ = sin(rot.z);

    return float3x3(
        cY * cZ, -cY * sZ, sY,
        sX * sY * cZ + cX * sZ, -sX * sY * sZ + cX * cZ, -sX * cY,
        -cX * sY * cZ + sX * sZ, cX * sY * sZ + sX * cZ, cX * cY
    );
}

PSInput main(VSInput input, uint instanceID : SV_InstanceID)
{
    Particle p = readParticles[instanceID];
    
    // ¡Ú ownerID´Â ±Û·Î¹ú ½½·Ô ÀÎµ¦½º
    uint globalEmitterSlot = p.ownerID;
    EmitterID eID = emitterIDs[globalEmitterSlot];
    ParticleConsts c = consts[globalEmitterSlot];
    ParticleMeshConsts mesh = meshConsts[eID.systemSlot];

    // Sorting »ç¿ë ½Ã ÀÎµ¦½º º¯°æ
    if (c.render.useSorting)
    {
        uint sortedIdx = sortedElements[instanceID].value;
        p = readParticles[sortedIdx];
    }

    PSInput output;

    // Scale
    float3 scaledPos = input.posModel * p.size;

    if (useHeightMap)
    {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0;
        scaledPos += input.normalModel * height * heightScale;
    }

    // Rotate
    float3x3 rotMatrix = GetRotationMatrix(p.rotation);
    float3 rotatedPos = mul(rotMatrix, scaledPos);
    float3 rotatedNormal = mul(rotMatrix, input.normalModel);
    float3 rotatedTangent = mul(rotMatrix, input.tangentModel);

    // Translate
    float4 localPosResult = float4(rotatedPos + p.position, 1.0f);

    SpawnConsts spawn = c.spawn;

    if (spawn.simulationSpace == 1) // World Space
    {
        output.posWorld = localPosResult.xyz;
        output.normalWorld = normalize(rotatedNormal);
        output.tangentWorld = normalize(rotatedTangent);
    }
    else // Local Space
    {
        float4 worldPos = mul(localPosResult, mesh.pWorld);
        output.posWorld = worldPos.xyz;
        output.normalWorld = normalize(mul(float4(rotatedNormal, 0.f), mesh.pWorldIT).xyz);
        output.tangentWorld = normalize(mul(float4(rotatedTangent, 0.f), mesh.pWorld).xyz);
    }

    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;

    return output;
}