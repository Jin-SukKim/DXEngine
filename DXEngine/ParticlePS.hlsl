struct PSInput {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float3 color : COLOR;
	uint primID: SV_PrimitiveID;
};

float4 main(PSInput input) : SV_TARGET
{
    // 1. 중심(0.5, 0.5)으로부터의 거리 계산
    float dist = length(float2(0.5, 0.5) - input.texCoord) * 2;

	// 2. 음수가 되지 않도록 saturate(0~1 사이로 제한) 사용
	// 원의 중심은 1.0, 원 밖은 0.0이 됨
	float scale = saturate(1.0 - dist);

	// 3. 부드러운 감쇠 효과를 위해 제곱(pow)을 사용하면 더 예쁩니다 (선택 사항)
	// scale = pow(scale, 2.0); 

	// 4. 결과 출력: RGB뿐만 아니라 Alpha에도 scale을 적용
	// 이렇게 해야 블렌드 스테이트의 SRC_ALPHA가 원 밖의 영역을 0으로 처리합니다.
	return float4(input.color.rgb * scale, 1.0);
}