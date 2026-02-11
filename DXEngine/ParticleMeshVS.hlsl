#define PARTICLE_RENDER_STAGE
#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D g_heightTexture : register(t0); // Height Map
StructuredBuffer<SortElement> sortedElements : register(t2);
StructuredBuffer<uint> aliveIndices : register(t26);
StructuredBuffer<uint> emitterWriteOffsets : register(t27);

// [추가] 3D Euler 각도 -> 회전 행렬 변환 함수
float3x3 GetRotationMatrix(float3 rot)
{
    float cX = cos(rot.x), sX = sin(rot.x);
    float cY = cos(rot.y), sY = sin(rot.y);
    float cZ = cos(rot.z), sZ = sin(rot.z);

    // Z * Y * X 순서 회전 행렬 (언리얼 엔진 기준)
    return float3x3(
        cY * cZ, -cY * sZ, sY,
        sX * sY * cZ + cX * sZ, -sX * sY * sZ + cX * cZ, -sX * cY,
        -cX * sY * cZ + sX * sZ, cX * sY * sZ + sX * cZ, cX * cY
    );
}

// [중요] SV_InstanceID를 인자로 받아야 합니다.
PSInput main(VSInput input, uint instanceID : SV_InstanceID)
{
    // O(1) lookup via aliveIndices buffer (batch start offset computed on GPU)
    uint batchStartOffset = emitterWriteOffsets[batchEmitterListOffset];
    uint globalIdx = aliveIndices[batchStartOffset + instanceID];
    Particle p = readParticles[globalIdx];
    uint emitterSlotID = p.ownerID;

    PSInput output;

    // -------------------------------------------------------------------------
    // 1. Scale: 크기 변환
    // -------------------------------------------------------------------------
    float3 scaledPos = input.posModel * p.size;

    // HeightMap 적용 (옵션)
    if (useHeightMap) {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0;

        // 로컬 Normal 방향으로 확장
        scaledPos += input.normalModel * height * heightScale;
    }

    // -------------------------------------------------------------------------
    // 2. Rotate: 회전 변환
    // -------------------------------------------------------------------------
    // 회전 행렬 생성 (p.rotation은 라디안 기준으로 저장)
    float3x3 rotMatrix = GetRotationMatrix(p.rotation);

    // 위치 회전 적용
    float3 rotatedPos = mul(rotMatrix, scaledPos);

    // Normal & Tangent 회전 (파티클 자체회전만 회전)
    float3 rotatedNormal = mul(rotMatrix, input.normalModel);
    float3 rotatedTangent = mul(rotMatrix, input.tangentModel);

    // -------------------------------------------------------------------------
    // 3. Translate: 위치 이동 (파티클 중심점)
    // -------------------------------------------------------------------------
    float4 localPosResult = float4(rotatedPos + p.position, 1.0f);

    SpawnConsts spawn = consts[emitterSlotID].spawn;
    // -------------------------------------------------------------------------
    // 4. World Transformation: 시뮬레이션 공간에 따른 처리
    // -------------------------------------------------------------------------
    if (spawn.simulationSpace == 1) // World Space Simulation
    {
        // 1. Position
        // 파티클 위치(p.position)가 이미 World 좌표이므로 Actor의 World 행렬 적용 불필요
        output.posWorld = localPosResult.xyz;

        // 2. Normal & Tangent
        // 파티클이 Actor에서 분리되었으므로, Actor의 회전(worldIT)을 적용하지 않음
        // 단, 파티클 자체의 회전(rotatedNormal)은 유지됨
        output.normalWorld = normalize(rotatedNormal);
        output.tangentWorld = normalize(rotatedTangent);
    }
    else // Local Space Simulation (기본 모드)
    {
        // 1. Position
        // 파티클이 Actor에 종속이므로 Actor의 World 행렬을 적용
        matrix pWorld = meshConsts[p.systemID].pWorld;
        float4 worldPos = mul(localPosResult, pWorld);
        output.posWorld = worldPos.xyz;

        // 2. Normal & Tangent
        // (파티클 회전) -> (Actor 회전) 순서로 적용
        // Normal은 Inverse Transpose를 사용해야 정확함
        output.normalWorld = normalize(mul(float4(rotatedNormal, 0.f), meshConsts[p.systemID].pWorldIT).xyz);
        output.tangentWorld = normalize(mul(float4(rotatedTangent, 0.f), pWorld).xyz);
    }

    // -------------------------------------------------------------------------
    // 최종: View / Projection 변환
    // -------------------------------------------------------------------------
    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;

    return output;
}
