#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<BatchEmitterInfo> batchInfo : register(t12);
Texture2D g_heightTexture : register(t0);

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
    // instanceID가 속한 emitter 찾기
    uint emitterIdx = 0;
    for (uint i = 0; i < batchEmitterCount; i++)
    {
        BatchEmitterInfo info = batchInfo[batchInfoOffset + i];
        if (instanceID < info.batchVertexStart + info.particleCount)
        {
            emitterIdx = i;
            break;
        }
    }

    BatchEmitterInfo entry = batchInfo[batchInfoOffset + emitterIdx];
    uint localInstanceID = instanceID - entry.batchVertexStart;
    uint myEmitterID = entry.globalEmitterID;

    Particle p = readParticles[entry.readParticleOffset + localInstanceID];

    PSInput output;

    // 1. Scale
    float3 scaledPos = input.posModel * p.size;

    if (useHeightMap)
    {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0;
        scaledPos += input.normalModel * height * heightScale;
    }

    // 2. Rotate
    float3x3 rotMatrix = GetRotationMatrix(p.rotation);
    float3 rotatedPos = mul(rotMatrix, scaledPos);
    float3 rotatedNormal = mul(rotMatrix, input.normalModel);
    float3 rotatedTangent = mul(rotMatrix, input.tangentModel);

    // 3. Translate
    float4 localPosResult = float4(rotatedPos + p.position, 1.0f);

    SpawnConsts spawn = consts[myEmitterID].spawn;

    // 4. World Transformation
    if (spawn.simulationSpace == 1)
    {
        output.posWorld = localPosResult.xyz;
        output.normalWorld = normalize(rotatedNormal);
        output.tangentWorld = normalize(rotatedTangent);
    }
    else
    {
        float4 worldPos = mul(localPosResult, pWorld);
        output.posWorld = worldPos.xyz;
        output.normalWorld = normalize(mul(float4(rotatedNormal, 0.f), pWorldIT).xyz);
        output.tangentWorld = normalize(mul(float4(rotatedTangent, 0.f), pWorld).xyz);
    }

    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;

    return output;
}
