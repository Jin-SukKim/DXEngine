#include "ParticleCommon.hlsli"

// 입력 리소스
StructuredBuffer<Vertex> meshVertex : register(t0);
StructuredBuffer<uint> meshIndices : register(t1);
Texture2D emissiveMap : register(t2);

// 결과 저장용 (AppendBuffer)
AppendStructuredBuffer<float3> outputPoints : register(u0);

cbuffer BakeConsts : register(b0)
{
    uint indexCount;
    float threshold;
    float2 padding;
};

// 무게 중심 좌표 계산 (Barycentric)
float3 CalculateBarycentric(float2 a, float2 b, float2 c, float2 p)
{
    float2 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0), d01 = dot(v0, v1), d11 = dot(v1, v1);
    float d20 = dot(v2, v0), d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    if (abs(denom) < 1e-5) return float3(-1, -1, -1);
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    return float3(1.0f - v - w, v, w);
}

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint triIdx = dtID.x;
    if (triIdx >= indexCount / 3) return;

    // 1. 삼각형 정보 가져오기
    uint i0 = meshIndices[triIdx * 3 + 0];
    uint i1 = meshIndices[triIdx * 3 + 1];
    uint i2 = meshIndices[triIdx * 3 + 2];

    Vertex v0 = meshVertex[i0];
    Vertex v1 = meshVertex[i1];
    Vertex v2 = meshVertex[i2];

    // 2. 텍스처 좌표 변환
    uint w, h;
    emissiveMap.GetDimensions(w, h);
    float2 uv0 = v0.texcoord * float2(w, h);
    float2 uv1 = v1.texcoord * float2(w, h);
    float2 uv2 = v2.texcoord * float2(w, h);

    // 3. AABB(경계 박스) 계산
    int minX = max(0, floor(min(min(uv0.x, uv1.x), uv2.x)));
    int maxX = min(w - 1, ceil(max(max(uv0.x, uv1.x), uv2.x)));
    int minY = max(0, floor(min(min(uv0.y, uv1.y), uv2.y)));
    int maxY = min(h - 1, ceil(max(max(uv0.y, uv1.y), uv2.y)));

    // 4. 픽셀 순회
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            // 삼각형 내부 검사
            float3 bc = CalculateBarycentric(uv0, uv1, uv2, float2(x, y) + 0.5f);
            if (bc.x >= 0 && bc.y >= 0 && bc.z >= 0)
            {
                // 밝기 검사
                float4 color = emissiveMap.Load(int3(x, y, 0));
                float brightness = dot(color.rgb, float3(0.299, 0.587, 0.114));

                if (brightness > threshold)
                {
                    // 월드 좌표 보간 후 저장
                    float3 pos = bc.x * v0.position + bc.y * v1.position + bc.z * v2.position;
                    outputPoints.Append(pos);
                }
            }
        }
    }
}