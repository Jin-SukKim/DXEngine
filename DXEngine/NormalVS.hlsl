#include "Common.hlsli"

struct NormalGSInput {
    float4 posModel : SV_POSITION;
    float3 normalModel : NORMAL;
};

NormalGSInput main(VSInput input)
{
    NormalGSInput output;
    
    output.posModel = float4(input.posModel, 1.0);
    output.normalModel = input.normalModel;
   
	return output;
}