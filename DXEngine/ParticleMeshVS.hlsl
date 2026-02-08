#include "Common.hlsli"
#include "ParticleCommon.hlsli"

Texture2D g_heightTexture : register(t0); // Height Map
StructuredBuffer<SortElement> sortedElements : register(t2);
StructuredBuffer<uint> visibleIndices : register(t13);

// [�߰�] 3D Euler ���� -> ȸ�� ��� ��ȯ �Լ�
float3x3 GetRotationMatrix(float3 rot)
{
    float cX = cos(rot.x), sX = sin(rot.x);
    float cY = cos(rot.y), sY = sin(rot.y);
    float cZ = cos(rot.z), sZ = sin(rot.z);

    // Z * Y * X ���� ���� ��� (��� ���� ����)
    return float3x3(
        cY * cZ, -cY * sZ, sY,
        sX * sY * cZ + cX * sZ, -sX * sY * sZ + cX * cZ, -sX * cY,
        -cX * sY * cZ + sX * sZ, cX * sY * sZ + sX * cZ, cX * cY
    );
}

// [�߿�] SV_InstanceID�� ���ڷ� �޾ƾ� �մϴ�.
PSInput main(VSInput input, uint instanceID : SV_InstanceID)
{
    // GPU Frustum Culling: Indirection through visibleIndices
    // visibleIndices contains indices of particles that passed frustum test
    uint visibleIdx = visibleIndices[readParticleOffset + instanceID];

    // 1. ���� �׸� ��ƼŬ�� �ε����� �����ɴϴ�.
    uint particleIdx = consts[emitterID].render.useSorting ? sortedElements[visibleIdx].value : visibleIdx;

    Particle p = readParticles[readParticleOffset + particleIdx];

    PSInput output;

    // -------------------------------------------------------------------------
    // 1. Scale: ũ�� ��ȯ
    // -------------------------------------------------------------------------
    float3 scaledPos = input.posModel * p.size;

    // HeightMap ���� (�ɼ�)
    if (useHeightMap) {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0).r;
        height = height * 2.0 - 1.0;

        // ���� Normal �������� Ȯ��
        scaledPos += input.normalModel * height * heightScale;
    }

    // -------------------------------------------------------------------------
    // 2. Rotate: ȸ�� ��ȯ
    // -------------------------------------------------------------------------
    // ȸ�� ��� ���� (p.rotation�� ���� ������� ����)
    float3x3 rotMatrix = GetRotationMatrix(p.rotation);

    // ��ġ ȸ�� ����
    float3 rotatedPos = mul(rotMatrix, scaledPos);

    // Normal & Tangent ȸ�� (���� ���������� ȸ��)
    float3 rotatedNormal = mul(rotMatrix, input.normalModel);
    float3 rotatedTangent = mul(rotMatrix, input.tangentModel);

    // -------------------------------------------------------------------------
    // 3. Translate: ��ġ �̵� (��ƼŬ �߽���)
    // -------------------------------------------------------------------------
    float4 localPosResult = float4(rotatedPos + p.position, 1.0f);

    SpawnConsts spawn = consts[emitterID].spawn;
    // -------------------------------------------------------------------------
    // 4. World Transformation: �ùķ��̼� ������ ���� ó��
    // -------------------------------------------------------------------------
    if (spawn.simulationSpace == 1) // World Space Simulation
    {
        // 1. Position
        // ��ƼŬ ��ġ(p.position)�� �̹� World ��ǥ�̹Ƿ� Actor�� World ��� ���� ����
        output.posWorld = localPosResult.xyz;

        // 2. Normal & Tangent
        // ��ƼŬ�� Actor�� �и��Ǿ����Ƿ�, Actor�� ȸ��(worldIT)�� �������� ����
        // ��, ��ƼŬ ��ü�� ȸ��(rotatedNormal)�� �����
        output.normalWorld = normalize(rotatedNormal);
        output.tangentWorld = normalize(rotatedTangent);
    }
    else // Local Space Simulation (���� ���)
    {
        // 1. Position
        // ��ƼŬ�� Actor�� �������̹Ƿ� Actor�� World ����� ����
        float4 worldPos = mul(localPosResult, pWorld);
        output.posWorld = worldPos.xyz;

        // 2. Normal & Tangent
        // (��ƼŬ ȸ��) -> (Actor ȸ��) ������ ����
        // Normal�� Inverse Transpose�� ����ؾ� ��Ȯ��
        output.normalWorld = normalize(mul(float4(rotatedNormal, 0.f), pWorldIT).xyz);
        output.tangentWorld = normalize(mul(float4(rotatedTangent, 0.f), pWorld).xyz);
    }

    // -------------------------------------------------------------------------
    // ����: View / Projection ��ȯ
    // -------------------------------------------------------------------------
    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;

    return output;
}