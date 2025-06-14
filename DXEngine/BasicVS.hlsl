#include "Common.hlsli"

Texture2D g_heightTexture : register(t0); // Height Map

PSInput main(VSInput input) {
    PSInput output;
    float4 pos = float4(input.posModel, 1.0);
    pos = mul(pos, world);
    
    output.posWorld = pos.xyz;
    
    //pos = mul(pos, view);
    //pos = mul(pos, proj);
    pos = mul(pos, viewProj);
    
    output.posProj = pos;
    //output.color = input.color
    output.texcoord = input.texcoord;

    float4 normal = float4(input.normalModel, 0.f); // 점이 아닌 방향
    output.normalWorld = mul(normal, worldIT).xyz;
    output.normalWorld = normalize(output.normalWorld);
    
    if (useHeightMap) {
        // Heightmap은 보통 흑백이라서 마지막에 .r로 float 하나만 사용
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0; // 범위를 [-1.0, 1.0]으로 변환
        output.posWorld += output.normalWorld * height * heightScale;
    }
    
    // Tangent Vector
    float4 tangentWorld = float4(input.tangentModel, 0.0);
    tangentWorld = mul(tangentWorld, world);
    output.tangentWorld = tangentWorld.xyz;
    
    return output;
}