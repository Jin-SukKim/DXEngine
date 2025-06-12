#include "Common.hlsli"

#define NUM_CONTROL_POINTS 4

// Hull Shader는 Vertex만큼 실행
struct VertexOut {
    float4 pos : POSITION;
};

// Hull Shader는 Vertex Shader로부터 출력을 받아서 2가지를 출력
// 데이터를 출력 - Patch Control Points
struct HullOut { 
    float3 pos : POSITION;
};

// 함수를 출력 (삼각형, 사각형 등을 어떻게 쪼개줄지에 대한 함수) - Patch Constant Data
struct PatchConstOutput { 
    float edges[4] : SV_TessFactor; // 각 Edge에 대해 쪼개기
    float inside[2] : SV_InsideTessFactor; // x, y 두 방향으로 쪼개주기
};

// Patch Constant Data를 반환하는 함수 (Hull Shader의 2번째 출력)
PatchConstOutput CalcHSPatchConstants(InputPatch<VertexOut, NUM_CONTROL_POINTS> patch,
									  uint patchID : SV_PrimitiveID)
{
    // 시점과 거리에 따른 물체의 해상도를 동적으로 변환 (LOD)
    float3 center = (patch[0].pos + patch[1].pos + patch[2].pos + patch[3].pos).xyz * 0.25; // Patch의 중심 좌표
    center = mul(float4(center, 1.0), world).xyz; // 월드 좌표계로 변환
    
    float dist = length(center - eyeWorld); // 시점으로부터 Patch까지의 거리
    float distMin = 0.5; // 고해상도 거리
    float distMax = 2.0; // 저해상도 거리
    
    // Tessellation을 얼마나 해줄지
    float weight = saturate((distMax - dist) / (distMax - distMin)); // 시점과 Patch의 거리에 따른 비율, [0.0, 1.0]으로 범위 고정
    // 1.0이 기본 (0.0)으로 하면 아예 Patch 자체가 사라져 모델이 없어짐
    float tess = 1.0;
    // amxtessfactor를 64로 설정했으므로 최대 64번 분할할 수 있음
    tess = tess + 64.0 * weight;
    
    PatchConstOutput pt;
	
    pt.edges[0] = tess; // 가장 작은 단위(ex: 삼각형, 사각형)의 Edge
    pt.edges[1] = tess;
    pt.edges[2] = tess;
    pt.edges[3] = tess;
    // 가장 작은 단위를 얼마나 작게 나눌지 (ex: 사각형인 경우 가로, 세로로 얼마나 작게 더 나눌지)
    pt.inside[0] = tess; 
    pt.inside[1] = tess;
    
    return pt;
}

// Patch의 Control Points들을 반환하는 함수 (Hull Shader의 1번째 출력)
[domain("quad")] // 사각형
[partitioning("integer")] // edges, inside와 같은 숫자에 대해 어떻게 쪼갤지를 의미
[outputtopology("triangle_cw")] // Tessellation, 분할해 시계 방향 삼각형을 생성
[outputcontrolpoints(4)] // GPU의 Thread 하나당 Control Point를 몇 개를 출력할지 조절
[patchconstantfunc("CalcHSPatchConstants")] // 2번째 출력인 Constant Function의 함수 이름
[maxtessfactor(64.f)] // 최대 사용할 수 있는게 64이고 edge 하나를 최대 64 등분할 수 있다는 의미
HullOut main(InputPatch<VertexOut, NUM_CONTROL_POINTS> p, 
	        uint i : SV_OutputControlPointID, // 각 Control Point의 ID
	        uint patchId : SV_PrimitiveID ) // Control Points가 모여 만들어진 각 Patch의 ID
{
	HullOut hout;

    hout.pos = p[i].pos.xyz;

	return hout;
}
