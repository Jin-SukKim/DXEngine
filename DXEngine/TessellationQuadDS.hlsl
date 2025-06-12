#include "Common.hlsli"
#define NUM_CONTROL_POINTS 4

// Hull Shader의 1번째 출력 (Patch Control Points)
struct HullOut {
    float3 pos : POSITION;
};

// Hull Shader의 2번째 출력 (Patch Constant Data)
struct PatchConstOutput {
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

// Domain Shader의 출력
struct DomainOut
{
    float4 pos : SV_Position;
};

// Hull Shader는 Vertex만큼 실행되고 Domain Shader는 쪼개진 좌표의 개수만큼 실행
// Tesselation을 통해 쪼개진 Texture의 좌표값을 Domain Shader가 받아서 사용하는데
// 이 쪼개진 Texture의 좌표 수만큼 실행됨
[domain("quad")]
DomainOut main(
    // Tessellation Stage의 출력인 쪼개진 위치의 Texture 좌표
    // (쪼개진 안쪽에 있는 Vertex들의 Texture 좌표)
	PatchConstOutput patchConst,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, NUM_CONTROL_POINTS> patch) {
	
    // 쪼개진 후의 Vertex 좌표들을 만들어서 출력
    DomainOut dout;

    // Patch의 Control Point와 Tessellation Stage를 통해 쪼개진 위치의 Texture 좌표를 이용해
    // 쪼개진 위치의 Vertex들을 계산
    
    // Bilinear Interpolation (가장 간단한 방법)
    float3 v1 = lerp(patch[0].pos, patch[1].pos, uv.x); // 쪼개진 위치의 uv값으로 vertex pos 계산
    float3 v2 = lerp(patch[2].pos, patch[3].pos, uv.x);
    float3 p = lerp(v1, v2, uv.y);
    
    dout.pos = float4(p, 1.0); // 쪼개진 Vertex의 위치
    dout.pos = mul(dout.pos, view);
    dout.pos = mul(dout.pos, proj);

    return dout;
}
