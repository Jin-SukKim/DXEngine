#include "Common.hlsli"

struct PSInput {
    float4 pos : SV_Position;
    float3 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);
}