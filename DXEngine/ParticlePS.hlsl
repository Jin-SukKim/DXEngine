struct PSInput {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 color : COLOR;
	uint primID: SV_PrimitiveID;
};

float4 main(PSInput input) : SV_TARGET
{
	float dist = length(float2(0.5, 0.5) - input.texCoord) * 2;
	float scale = 1 - dist;
	return float4(input.color.rgb * scale, 1.0);
}