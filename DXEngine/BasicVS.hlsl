#include "Common.hlsli"

cbuffer MeshConstBuffer : register(b1) {
    matrix world;
};

struct VSInput {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PSOutput {
    float4 pos : SV_Position;
    float3 color : COLOR;
};

PSOutput main(VSInput input) {
    PSOutput output;
    float4 pos = float4(input.pos, 1.0);
    pos = mul(pos, world);
    pos = mul(pos, view);
    pos = mul(pos, proj);
    
    output.pos = pos;
    output.color = float3(1.0, 1.0, 1.0);

    return output;
}