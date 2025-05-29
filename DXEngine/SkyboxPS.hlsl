#include "Common.hlsli"

struct SkyboxPSInput {
    float4 posProj : SV_POSITION;
    float3 posModel : POSITION;
};

float4 main(SkyboxPSInput input) : SV_TARGET {
    
    return envIBLTex.Sample(linearWrapSampler, input.posModel.xyz);
}