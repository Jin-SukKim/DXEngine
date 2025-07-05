#include "Common.hlsli"

struct DepthOnlyVSInput {
    float3 posModel : POSITION;
};


float4 main(DepthOnlyVSInput input) : SV_Position {
    // 월드 좌표계로만 변환해서 Depth값을 알아내기
    float4 pos = mul(float4(input.posModel, 1.0), world);
    pos = mul(pos, viewProj);
	return pos;
}