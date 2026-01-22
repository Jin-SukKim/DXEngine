struct Vertex {
    float3 position;
    float3 normalModel;
    float2 texcoord;
    float3 tangentModel;
};

SamplerState linearWrapSampler : register(s0);
SamplerState linearClampSampler : register(s1);

StructuredBuffer<Vertex> meshVertex : register(t0);
StructuredBuffer<uint> meshIndices : register(t1);
Texture2D spawnTexture : register(t2);

AppendStructuredBuffer<float3> outputPos : register(u0);

cbuffer BakeConsts : register(b0) {
    uint indexCount;
    float threshold;
    float2 padding;
    float4 channelMask;
}

// 무게 중심 좌표 계산 (Barycentric)
// 점 p가 삼각형(a,b,c) 내부에 있으면 (u,v,w)가 모두 0 이상입니다.
float3 CalculateBarycentric(float2 a, float2 b, float2 c, float2 p)
{
    float2 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;

    // 분모가 0에 가까우면(Degenerate triangle) 무효 처리
    if (abs(denom) < 1e-5) return float3(-1, -1, -1);

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return float3(u, v, w);
}

bool CheckTextureCondition(float2 uv)
{
    float4 color = spawnTexture.SampleLevel(linearClampSampler, uv, 0);
    float value = dot(color, channelMask);
    return value >= threshold;
}

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID) {
    uint triIdx = dtID.x;

    if (triIdx >= indexCount / 3) return;

    uint i0 = meshIndices[triIdx * 3 + 0];
    uint i1 = meshIndices[triIdx * 3 + 1];
    uint i2 = meshIndices[triIdx * 3 + 2];

    Vertex v0 = meshVertex[i0];
    Vertex v1 = meshVertex[i1];
    Vertex v2 = meshVertex[i2];

    // Texture 좌표를 Pixel 좌표로 변환
    uint w, h;
    spawnTexture.GetDimensions(w, h);

    float2 uv0 = v0.texcoord * float2(w, h);
    float2 uv1 = v1.texcoord * float2(w, h);
    float2 uv2 = v2.texcoord * float2(w, h);

    // 삼각형의 2D AABB 계산 (픽셀 단위)
    int minX = max(0, (int)floor(min(min(uv0.x, uv1.x), uv2.x)));
    int maxX = min((int)w - 1, (int)ceil(max(max(uv0.x, uv1.x), uv2.x)));
    int minY = max(0, (int)floor(min(min(uv0.y, uv1.y), uv2.y)));
    int maxY = min((int)h - 1, (int)ceil(max(max(uv0.y, uv1.y), uv2.y)));

    if (minX > maxX || minY > maxY) return;

    // 경계 박스 내부 픽셀 순회
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            // 픽셀 중앙 기준
            float2 pixelCenter = float2(x, y) + 0.5f;
            float3 bc = CalculateBarycentric(uv0, uv1, uv2, pixelCenter);

            // 삼각형 내부 체크
            if (bc.x >= -1e-5f && bc.y >= -1e-5f && bc.z >= -1e-5f) {

                float2 sampleUV = 
                    bc.x * v0.texcoord + 
                    bc.y * v1.texcoord + 
                    bc.z * v2.texcoord;

                if (CheckTextureCondition(sampleUV)) {
                    float3 pos = 
                        bc.x * v0.position +
                        bc.y * v1.position +
                        bc.z * v2.position;
                    outputPos.Append(pos);
                }
            }
        }
    }
}