#include "Common.hlsli"

cbuffer BillboardConsts : register(b3) {
    float widthWorld; // world width
    float3 dummy4;
    uint arraySize;
    float3 dummy5;
};

struct GSInput {
    float4 pos : SV_POSITION; // ScreenPoint;
};

struct BillboardPSInput
{
    float4 pos : SV_POSITION;
    float4 posWorld : POSITION0;
    float4 center : POSITION1;
    float2 texcoord : TEXCOORD;
    uint primID : SV_PrimitiveID;
};

// billboard는 vertex를 4개로 만들어 사각형을 생성
[maxvertexcount(4)]
void main(point GSInput input[1] : SV_POSITION, uint primID : SV_PrimitiveID,
	inout TriangleStream<BillboardPSInput> outputStream)
{
    float hw = 0.5 * widthWorld;
	
    // 월드 좌표계의 up 축
    float4 up = float4(0.0, 1.0, 0.0, 0.0);
    
    // (0, 1, 0, 0)가 뷰 좌표계의 Up 축의 값으로 생각하고 invView로 월드 좌표계로 역변환해 월드 좌표계에서의 View 좌표계의 Up Vector를 계산
    //float4 up = mul(float4(0, 1, 0, 0), invView); // <- 뷰의 업벡터를 월드로 변환 (파이어볼을 위에서 보는 경우)
    //up.xyz = normalize(up.xyz);
    float4 front = float4(eyeWorld, 1.0) - input[0].pos;
    front.w = 0.0; // 벡터
    
    // Billboard가 시점을 바라보는 방향 기준으로 오른쪽
    // 시점에서 Billboard를 바라보는 방향헤서는 왼쪽 (Texture 좌표 주의)
    float4 right = normalize(float4(cross(up.xyz, normalize(front.xyz)), 0.0));
	
    BillboardPSInput output;
	
    output.center = input[0].pos; // 빌보드의 중심
	
    // 왼쪽 아래 Point
    output.posWorld = input[0].pos - hw * right - hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(1.0, 1.0);
    output.primID = primID;
    
    outputStream.Append(output);
	
    // 왼쪽 위 Point
    output.posWorld = input[0].pos - hw * right + hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(1.0, 0.0);
    output.primID = primID;
	
    outputStream.Append(output);
    
    // 오른쪽 아래 Point
    output.posWorld = input[0].pos + hw * right - hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(0.0, 1.0);
    output.primID = primID;
    
    outputStream.Append(output);
	
    // 오른쪽 위 Point
    output.posWorld = input[0].pos + hw * right + hw * up;
    output.pos = output.posWorld;
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.texcoord = float2(0.0, 0.0);
    output.primID = primID;
    
    outputStream.Append(output);
    
    // GS는 Triangle Strips으로 출력
    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-strips
    outputStream.RestartStrip(); // Strip을 다시 시작

}