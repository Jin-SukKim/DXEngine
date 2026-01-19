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

    // 2. 모델 로컬 변환 (SRT 순서: Scale -> Rotate -> Translate)

    // (1) Scale: 파티클의 크기 적용 (기본 1.0이라고 가정, p.scale이 있다면 곱해줌)
    // float3 scaledPos = input.posModel * p.scale.xyz; 
    float3 scaledPos = input.posModel * p.size.x; // 예시: size가 float3 혹은 float라면 맞춰서 사용

    // (2) HeightMap 적용 (옵션)
    // HeightMap은 로컬 공간에서 정점을 밀어낸 후 회전/이동하는 것이 자연스럽습니다.
    if (useHeightMap) {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0;

        // 로컬 Normal 방향으로 확장
        scaledPos += input.normalModel * height * heightScale;
    }

    // (3) Rotate (파티클에 회전이 있다면 여기서 쿼터니언 회전 적용)
    // float3 rotatedPos = RotateVector(scaledPos, p.rotation); // 쿼터니언 함수 필요
    float3 rotatedPos = scaledPos; // 회전 없으면 그대로

    // (4) Translate: 파티클의 월드 위치 더하기 (질문하신 부분!)
    float4 worldPos = float4(rotatedPos + p.position, 1.0f);


    // 3. 월드 변환 적용 (파티클 시스템 자체가 월드 행렬을 가질 경우)
    // 보통 파티클 시뮬레이션은 월드 좌표계에서 이루어지므로 world 행렬은 Identity일 수 있습니다.
    worldPos = mul(worldPos, world);
    output.posWorld = worldPos.xyz;

    // 4. 뷰/프로젝션 변환
    output.posProj = mul(worldPos, viewProj);

    output.texcoord = input.texcoord;

    // 5. Normal 변환
    // 파티클이 회전했다면 Normal도 같이 회전시켜야 함
    float3 normal = input.normalModel;
    // normal = RotateVector(normal, p.rotation); // 파티클 회전 적용
    output.normalWorld = normalize(mul(float4(normal, 0.f), worldIT).xyz);

    // 6. Tangent 변환
    float3 tangent = input.tangentModel;
    // tangent = RotateVector(tangent, p.rotation); // 파티클 회전 적용
    output.tangentWorld = normalize(mul(float4(tangent, 0.f), world).xyz);

    return output;
}