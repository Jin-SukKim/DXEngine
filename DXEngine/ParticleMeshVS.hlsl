#include "Common.hlsli"
#include "ParticleCommon.hlsli"

StructuredBuffer<Particle> particles : register(t0);
StructuredBuffer<SortElement> sortedElements : register(t1);

Texture2D g_heightTexture : register(t2); // Height Map

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

    // Translate: 파티클의 월드 위치 더하기 
    float4 worldPos = float4(rotatedPos + p.position, 1.0f);
    
    // World 좌표계 변환
    worldPos = mul(worldPos, world);
    output.posWorld = worldPos.xyz;

    // 뷰/프로젝션 변환
    output.posProj = mul(worldPos, viewProj);

    output.texcoord = input.texcoord;

    // Normal 변환
    // 파티클이 회전했다면 Normal도 같이 회전시켜야 함
    float3 normal = input.normalModel;
    // normal = RotateVector(normal, p.rotation); // 파티클 회전 적용
    output.normalWorld = normalize(mul(float4(normal, 0.f), worldIT).xyz);

    // Tangent 변환
    float3 tangent = input.tangentModel;
    // tangent = RotateVector(tangent, p.rotation); // 파티클 회전 적용
    output.tangentWorld = normalize(mul(float4(tangent, 0.f), world).xyz);

    return output;
}