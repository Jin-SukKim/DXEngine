#include "Common.hlsli"

Texture2D lowResTexture : register(t0);      // Low-res particle color (RGB) + Alpha (A)
Texture2D fullResDepth  : register(t1);      // Full-res Scene Depth (High Res)
Texture2D lowResSceneDepth : register(t2);   // Downsampled Scene Depth (Low Res) - �߿�!

struct SamplingPSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPSInput input) : SV_TARGET
{
    // 1. ���� �ȼ��� ���ػ� ���� (���ذ�)
    float fullDepth = fullResDepth.SampleLevel(pointClampSampler, input.texcoord, 0).r;

    // 2. ���ػ� �ؽ�ó ����
    float2 lowResSize;
    lowResTexture.GetDimensions(lowResSize.x, lowResSize.y);
    float2 texelSize = 1.0 / lowResSize;

    // 3. ���� UV�� ���ػ� �׸��忡�� ��� ��ġ�ϴ��� ���
    // ��: 10.4 -> 10�� �ȼ��� 11�� �ȼ� ����, 10������ 0.4��ŭ ������
    float2 lowResCoord = input.texcoord * lowResSize;
    float2 centerPos = floor(lowResCoord - 0.5) + 0.5; // 2x2 �׸����� �»�� �ȼ� �߽���

    // 4. ���� �ȼ��� 2x2 �׸��� ������ ��� ġ������ �ִ��� (0.0 ~ 1.0)
    // �̰��� �ٷ� Bilinear Interpolation�� �ٽ� ����ġ�Դϴ�.
    float2 t = lowResCoord - centerPos;

    float4 totalColor = 0;
    float totalWeight = 0.0;
    const float depthThreshold = 0.02; // ���� �ΰ��� (��Ȳ�� ���� ����)

    // 2x2 ���� ���ø� ����
    [unroll]
    for (int x = 0; x <= 1; x++)
    {
        [unroll]
        for (int y = 0; y <= 1; y++)
        {
            float2 offset = float2(x, y);

            // ���ø��� UV ��ǥ (��Ȯ�� �ؼ��� �߽��� ���� ��)
            float2 sampleUV = (centerPos + offset) * texelSize;

            // [�߿�] ���� ������ �ϹǷ� ���⼭�� Point Sampler�� ��� ��Ȯ�մϴ�.
            float lowDepth = lowResSceneDepth.SampleLevel(pointClampSampler, sampleUV, 0).r;
            float4 sampleColor = lowResTexture.SampleLevel(pointClampSampler, sampleUV, 0);

            // (A) Depth Weight: ���� ���̿� ���� ����ġ
            float depthDiff = abs(fullDepth - lowDepth);
            float depthWeight = 1.0 / (1.0 + depthDiff / depthThreshold); // �������� ����ġ �϶�

            // (B) Spatial Weight: �Ÿ��� ���� ����ġ (�̰� �߰���!)
            // x=0�� ���� (1-t.x), x=1�� ���� t.x�� ���
            float spatialWeightX = (x == 0) ? (1.0 - t.x) : t.x;
            float spatialWeightY = (y == 0) ? (1.0 - t.y) : t.y;
            float spatialWeight = spatialWeightX * spatialWeightY;

            // (C) Alpha Weight removed: alpha=0 clear pixels must contribute to interpolation.
            // Excluding empty pixels (alpha=0) from spatial blending causes hard pixelated edges.
            // With alpha=0 clear, (0,0,0,0) * weight blends smoothly -> soft anti-aliased edge.

            // ���� ����ġ ����
            float weight = depthWeight * spatialWeight;

            totalColor += sampleColor * weight;
            totalWeight += weight;
        }
    }

    // ��� ����ȭ
    float4 finalColor;
    if (totalWeight > 0.0001)
    {
        finalColor = totalColor / totalWeight;
    }
    else
    {
        // Fallback: ���� ���̰� �ʹ� Ŀ�� ��Ī�Ǵ°� ���ų� ���İ� �� 0�� ���
        // �׳� Bilinear (�ε巴�� ������)
        finalColor = lowResTexture.SampleLevel(linearClampSampler, input.texcoord, 0);
    }

    return finalColor;
}