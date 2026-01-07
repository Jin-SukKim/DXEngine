struct GSInput {
	float4 position : SV_POSITION;
	float3 color : COLOR;
	float life : PSIZE0;
	float size : PSIZE1;
};

struct PSInput {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 color : COLOR;
	uint primID: SV_PrimitiveID;
};

[maxvertexcount(4)]
void main(
	point GSInput input[1], uint primID : SV_PrimitiveID,
	inout TriangleStream<PSInput> output
)
{
    float hw = input[0].size; // halfWidth
    float3 up = float3(0, 1, 0);
    float3 right = float3(1, 0, 0);

    PSInput billboard;
    billboard.position.w = 1;
    billboard.color = input[0].color;
    billboard.primID = primID;

    billboard.position.xyz = input[0].position.xyz - hw * right - hw * up;
    billboard.texCoord = float2(0.0, 1.0);

    output.Append(billboard);

    billboard.position.xyz = input[0].position.xyz - hw * right + hw * up;
    billboard.texCoord = float2(0.0, 0.0);

    output.Append(billboard);

    billboard.position.xyz = input[0].position.xyz + hw * right - hw * up;
    billboard.texCoord = float2(1.0, 1.0);

    output.Append(billboard);

    billboard.position.xyz = input[0].position.xyz + hw * right + hw * up;
    billboard.texCoord = float2(1.0, 0.0);

    output.Append(billboard);

    // 주의: GS는 Triangle Strips으로 출력합니다.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-strips
    output.RestartStrip(); // Strip을 다시 시작
}