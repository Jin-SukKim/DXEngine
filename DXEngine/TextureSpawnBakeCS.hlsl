struct Vertex {
    float3 position;
    float3 normalModel;
    float2 texcoord;
    float3 tangentModel;
};

// Sampler는 사용 안 하므로 선언만 유지하거나 제거해도 됨
SamplerState linearWrapSampler : register(s0);
SamplerState linearClampSampler : register(s1);

StructuredBuffer<Vertex> meshVertex : register(t0);
StructuredBuffer<uint> meshIndices : register(t1);
Texture2D spawnTexture : register(t2);

AppendStructuredBuffer<float3> outputPos : register(u0);

// C++과 슬롯 일치 확인 (b0 or b4)
cbuffer BakeConsts : register(b0) {
    uint indexCount;
    float threshold;
    float2 padding;
    float4 channelMask;
};

// 무게 중심 좌표 (수정 없음)
float3 CalculateBarycentric(float2 a, float2 b, float2 c, float2 p)
{
    float2 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;

    if (abs(denom) < 1e-5) return float3(-1, -1, -1);

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return float3(u, v, w);
}

bool CheckTextureCondition(int2 pixelPos, uint w, uint h)
{
    // [핵심] Wrapping 처리 (Tiling 지원)
    // 음수 좌표도 처리하기 위해 넉넉히 더해주고 나머지 연산
    int2 wrappedPos = int2(pixelPos.x % int(w), pixelPos.y % int(h));
    if (wrappedPos.x < 0) wrappedPos.x += int(w);
    if (wrappedPos.y < 0) wrappedPos.y += int(h);

    // MipLevel 0번 로드
    float4 color = spawnTexture.Load(int3(wrappedPos, 0));

    // 마스크 적용 (C++에서 설정한 channelMask 사용)
    float value = dot(color, channelMask);

    return value >= threshold;
}

[numthreads(256, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint triIdx = dtID.x;
    if (triIdx >= indexCount / 3) return;

    uint i0 = meshIndices[triIdx * 3 + 0];
    uint i1 = meshIndices[triIdx * 3 + 1];
    uint i2 = meshIndices[triIdx * 3 + 2];

    Vertex v0 = meshVertex[i0];
    Vertex v1 = meshVertex[i1];
    Vertex v2 = meshVertex[i2];

    uint w, h;
    spawnTexture.GetDimensions(w, h);
    if (w == 0 || h == 0) return;

    // [안전장치 1] UV 래핑으로 인한 거대 삼각형 방지
    // 인접 정점 간 UV 차이가 0.8 이상이면 경계선(Seam)에 걸친 것으로 간주하고 스킵
    float2 uv0_raw = v0.texcoord;
    float2 uv1_raw = v1.texcoord;
    float2 uv2_raw = v2.texcoord;

    bool isSeam =
        (length(uv0_raw - uv1_raw) > 0.8f) ||
        (length(uv1_raw - uv2_raw) > 0.8f) ||
        (length(uv2_raw - uv0_raw) > 0.8f);

    if (isSeam) return;

    // 픽셀 좌표 변환 (Tiling UV 허용)
    float2 uv0 = uv0_raw * float2(w, h);
    float2 uv1 = uv1_raw * float2(w, h);
    float2 uv2 = uv2_raw * float2(w, h);

    int minX = (int)floor(min(min(uv0.x, uv1.x), uv2.x));
    int maxX = (int)ceil(max(max(uv0.x, uv1.x), uv2.x));
    int minY = (int)floor(min(min(uv0.y, uv1.y), uv2.y));
    int maxY = (int)ceil(max(max(uv0.y, uv1.y), uv2.y));

    // [안전장치 2] 픽셀 수 제한 (TDR 방지)
    // 텍스처 전체 크기의 2배를 넘거나 50만 픽셀을 넘으면 스킵
    uint totalPixels = (uint)((maxX - minX) * (maxY - minY));
    uint maxAllowed = max(500000u, (w * h) * 2);

    if (totalPixels > maxAllowed) return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float2 pixelCenter = float2(x, y) + 0.5f;
            float3 bc = CalculateBarycentric(uv0, uv1, uv2, pixelCenter);

            if (bc.x >= -1e-5f && bc.y >= -1e-5f && bc.z >= -1e-5f)
            {
                // 인자 단순화: mask, thresh는 전역변수(CB) 사용
                if (CheckTextureCondition(int2(x, y), w, h))
                {
                    float3 pos = bc.x * v0.position +
                        bc.y * v1.position +
                        bc.z * v2.position;
                    outputPos.Append(pos);
                }
            }
        }
    }
}