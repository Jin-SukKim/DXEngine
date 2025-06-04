#include "Common.hlsli"

struct SamplingVSInput {
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

SamplingPSInput main(SamplingVSInput input)
{
    SamplingPSInput output;
    
    output.position = float4(input.position, 1.0);
    output.texcoord = input.texcoord;
    
    return output;
}