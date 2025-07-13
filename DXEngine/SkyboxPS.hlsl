#include "Common.hlsli"

struct SkyboxPSInput {
    float4 posProj : SV_POSITION;
    float3 posModel : POSITION;
};

float4 main(SkyboxPSInput input) : SV_TARGET {
    
    float3 color = envIBLTex.Sample(linearWrapSampler, input.posModel.xyz).rgb * strengthIBL;
    return float4(color, 1.0);

}