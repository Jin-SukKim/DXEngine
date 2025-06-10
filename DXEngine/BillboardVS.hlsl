#include "Common.hlsli"

struct BillboardVSInput {
    float3 pos : POSITIONT; // ¸ðµ¨ ÁÂÇ¥°èÀÇ À§Ä¡
};

struct GSInput {
    float4 pos : SV_Position; // ScreenPoint;
};

GSInput main(BillboardVSInput input) {
    GSInput output;
    output.pos = mul(float4(input.pos, 1.f), world);
	return output;
}