cbuffer MeshConstBuffer : register(b0) {
    matrix world;
    matrix view;
    matrix proj;
}

struct VSInput {
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
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