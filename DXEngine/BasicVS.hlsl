#include "Common.hlsli"

PSInput main(VSInput input) {
    PSInput output;
    float4 pos = float4(input.posModel, 1.0);
    pos = mul(pos, world);
    
    output.posWorld = pos.xyz;
    
    pos = mul(pos, view);
    pos = mul(pos, proj);
    
    output.posProj = pos;
    //output.color = input.color
    output.texcoord = input.texcoord;

    float4 normal = float4(input.normalModel, 0.f); // ���� �ƴ� ����
    output.normalWorld = mul(normal, worldIT).xyz;
    output.normalWorld = normalize(output.normalWorld);
    
    return output;
}