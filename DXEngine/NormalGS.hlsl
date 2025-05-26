#include "Common.hlsli"

struct NormalGSInput {
    float4 posModel : SV_POSITION;
    float3 normalModel : NORMAL;
};

struct NormalPSInput {
	float4 pos : SV_POSITION;
    float3 color : COLOR;
};

static const float lineScale = 0.02;

[maxvertexcount(2)]
void main(
	point NormalGSInput input[1],
	inout LineStream<NormalPSInput> outputStream
)
{
    NormalPSInput output;
    
    float4 posWorld = mul(input[0].posModel, world);
    float4 normalModel = float4(input[0].normalModel, 0.0);
    float4 normalWorld = mul(normalModel, worldIT);
    normalWorld = float4(normalize(normalWorld.xyz), 0.0);
    
    output.pos = mul(posWorld, viewProj);
    output.color = float3(1.0, 1.0, 0.0);
    outputStream.Append(output);
    
    // Normal Vector 방향으로 lineScale만큼 이동한 위치
    output.pos = mul(posWorld + lineScale * normalWorld, viewProj);
    output.color = float3(1.0, 0.0, 0.0);
    outputStream.Append(output);
}