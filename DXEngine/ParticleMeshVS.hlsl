#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D g_heightTexture : register(t0); // Height Map
StructuredBuffer<Particle> particles : register(t1);
StructuredBuffer<SortElement> sortedElements : register(t2);


// [중요] SV_InstanceID를 인자로 받아야 합니다.
PSInput main(VSInput input, uint instanceID : SV_InstanceID)
{
    // 1. 현재 그릴 파티클의 인덱스를 가져옵니다.
    // (만약 정렬을 안 쓴다면 uint particleIdx = instanceID;)
    uint particleIdx = sortedElements[instanceID].value;
    Particle p = particles[particleIdx];

    PSInput output;

    // 모델 로컬 변환 (SRT 순서: Scale -> Rotate -> Translate)

    // float3 scaledPos = input.posModel * p.scale.xyz; 
    float3 scaledPos = input.posModel * p.size; 

    // HeightMap 적용 (옵션)
    if (useHeightMap) {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0;

        // 로컬 Normal 방향으로 확장
        scaledPos += input.normalModel * height * heightScale;
    }

    // Rotate (파티클에 회전이 있다면 여기서 쿼터니언 회전 적용)
    // float3 rotatedPos = RotateVector(scaledPos, p.rotation); // 쿼터니언 함수 필요
    float3 rotatedPos = scaledPos; // 회전 없으면 그대로

    // Translate: 파티클의 위치 더하기 
    float4 localPosResult = float4(rotatedPos + p.position, 1.0f);
    
    if (spawn.simulationSpace == 1) // World Space Simulation
    {
        // 1. Position
        // 파티클 위치(p.position)가 이미 World 좌표이므로 Actor의 World 행렬 곱을 생략합니다.
        output.posWorld = localPosResult.xyz;

        // 2. Normal & Tangent
        // 파티클이 Actor와 분리되었으므로, Actor의 회전(worldIT)을 적용하지 않습니다.
        // (단, 파티클 자체의 회전인 p.rotation은 위에서 이미 적용되어 있어야 합니다)
        output.normalWorld = normalize(input.normalModel);
        output.tangentWorld = normalize(input.tangentModel);

        // 만약 추후 p.rotation을 적용한다면:
        // output.normalWorld = normalize(RotateVector(input.normalModel, p.rotation)); 
    }
    else // Local Space Simulation (기존 방식)
    {
        // 1. Position
        // 파티클이 Actor에 종속적이므로 Actor의 World 행렬을 곱합니다.
        float4 worldPos = mul(localPosResult, world);
        output.posWorld = worldPos.xyz;

        // 2. Normal & Tangent
        // Actor가 회전하면 파티클 메쉬의 Normal도 같이 회전해야 합니다.
        // Normal은 Inverse Transpose를 사용해야 정확합니다.
        output.normalWorld = normalize(mul(float4(input.normalModel, 0.f), worldIT).xyz);
        output.tangentWorld = normalize(mul(float4(input.tangentModel, 0.f), world).xyz);
    }

    // -------------------------------------------------------------------------
    // 공통: View / Projection 변환
    // -------------------------------------------------------------------------
    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;

    return output;
}