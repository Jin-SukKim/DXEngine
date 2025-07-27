#include "Common.hlsli"
#include "Shadow.hlsli"
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D aoTex : register(t2);
Texture2D metallicRoughnessTex : register(t3);
Texture2D emissiveTex : register(t4);

// 물체의 재질에 따라 F0을 결정하는데 metallic 값에 따라 Fdielectric와 albedo의 lerp 범위로 조정
static const float3 Fdielectric = 0.04; // 비금속 재질의 F0이 최소가 0이 아닌 0.04

float3 GetNormal(PSInput input) {
    float3 normalWorld = input.normalWorld;
    
    if (useNormalMap) {
        // Texture 좌표계의 Noraml Vector
        float3 normal = normalTex.SampleLevel(linearWrapSampler, input.texcoord, 0).rgb;
        normal = 2.0 * normal - 1.0; // 모든 방향에 대응하기 위해 [-1.0, 1.0]으로 변환
    
        // OpenGL 용 Normal Map일 경우에는 y 방향을 뒤집어주기
        normal.y = invertNormalMapY ? -normal.y : normal.y;
        
        // Texture 좌표계에 정의된 Normal Vector를 World 좌표계로 변환
        float3 N = normalWorld;
        float3 T = normalize(input.tangentWorld - dot(input.tangentWorld, N) * N); // Tangent
        float3 B = cross(N, T); // Bitangent
    
        // Texture 좌표계의 좌표축 방향을 World 좌표계로 변환한 3개의 벡털르 가지고 변환행렬 생성
        float3x3 TBN = float3x3(T, B, N);
        // Texture 좌표계의 Normal Vector를 World 좌표계로 변환행렬로 변환
        normalWorld = normalize(mul(normal, TBN));
    }
    
    return normalWorld;
}

float3 SchlickFresnel(float3 F0, float NdotH) {
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * NdotH - 6.98316) * NdotH);
    
    // 요즘은 GPU 속도가 빠르기 때문에 5제곱 해줘도 문제가  없음
    //return F0 + (1.0 - F0) * po   w(1.0 - cosTheta, 5.0);
}

float3 DiffuseIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic) {
    // metallic을 이용해 Diffuse 색을 결정 (diffuse에서 metallic 성분이 늘어나면 Specular에서 줄어드는 에너지 보존 법칙)
    float3 F0 = lerp(Fdielectric, albedo, metallic); // 물체의 재질에 따라 F0을 결점
    float3 F = SchlickFresnel(F0, max(0.0, dot(normalWorld, pixelToEye)));
    // metallic이 1.0에 가까워질수록 kd 값은 0에 가까워져 metal이면 diffuse가 줄어들도록 구현
    float3 kd = lerp(1.0 - F, 0.0, metallic);
    
    float3 irradiance = irradianceIBLTex.SampleLevel(linearWrapSampler, normalWorld, 0.0).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic, float roughness) {
    // Environement BRDF (Clamp Sampler 사용) - r = cos(theta), g = roughness
    // (IBLBaker를 사용해 만든 LUT의 경우 roughness를 그대로 사용하면 안되고 1 - roughness로 넣어줘야 함)
    float2 specularBRDF = brdfTex.SampleLevel(linearClampSampler, float2(dot(normalWorld, pixelToEye), 1.0 - roughness), 0.0f).rg;

    // Roughness가 올라가면 낮은 해상도의 Mipmap을 사용해 Non-Metal 느낌 표현 
    // (lod level로 들어가는 값은 roughness에 mipmap의 최고 레벨이나 임의의 최고 레벨을 곱해주기)
    float3 specularIrradiance = specularIBLTex.SampleLevel(linearWrapSampler, reflect(-pixelToEye, normalWorld), roughness * 5.0f).rgb;
    // 물체의 재질에 따라 F0을 결점
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    
    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

}

// 간접광 (Ambient Light) - 주변 환경으로부터 받는 빛
float3 AmbientLightingByIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float ao, float metallic, float roughness) {
    // metallic을 이용해 diffuse 색을 결정
    float3 diffuseIBL = DiffuseIBL(albedo, normalWorld, pixelToEye, metallic);
    // 에너지 보존 법칙으로 인해 diffuse가 늘어나면 specular가 줄어들고 그 반대 상황도 가능
    float3 specularIBL = SpecularIBL(albedo, normalWorld, pixelToEye, metallic, roughness);
    
    // ao는 물체의 빛이 제대로 들어가지 않는 부분
    return (diffuseIBL + specularIBL) * ao;
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NdfGGX(float NdotH, float roughness, float alphaPrime) {
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;

    return alphaPrime * alphaPrime / (3.141592 * denom * denom);
}

// Single term for separable Schlick-GGX below.
float SchlickG1(float NdotV, float k) {
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float SchlickGGX(float NdotI, float NdotO, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);
}

// Light Type별로 빛의 세기 (조명 자체의 강도만 계산)
float3 LightRadiance(Light light, float3 representativePoint, float3 posWorld, float3 normalWorld) {
    // Directional Light
    float3 lightVec = light.type & LIGHT_DIRECTIONAL ?
                        // Directional Light라면 빛의 방향을 그대로 사용 (ex: 태양)
                        -light.direction :
                        representativePoint - posWorld; // (Area Light를 사용 안한다면 : light.position - posWorld;)
    
    float lightDist = length(lightVec); // Dirctional Light은 1.0이 되어야함
    lightVec /= lightDist; // Normalize
    
    // Spot Light
    float spotFactor = light.type & LIGHT_SPOT ?
                        // 빛이 향하는 방향으로 빛을 모아주는 강도
                        pow(max(-dot(lightVec, light.direction), 0.0), light.spotPower) :
                        1.0; // Directional Light이나 Point Light인 경우

    // Distance attenuation (거리에 따른 빛의 강도 가중치)
    float att = saturate((light.fallOffEnd - lightDist) / (light.fallOffEnd - light.fallOffStart));

    // Shadow Map
    float shadowFactor = 1.0;
    
    // 빛의 강도
    float3 radiance = light.radiance * spotFactor * att * shadowFactor;
    
    return radiance;
}

// Shadow Map까지 고려한 조명의 밝기
float3 LightRadiance(Light light, float3 representativePoint, float3 posWorld, float3 normalWorld, Texture2D shadowMap) {
    // Directional Light
    float3 lightVec = light.type & LIGHT_DIRECTIONAL ?
                        // Directional Light라면 빛의 방향을 그대로 사용 (ex: 태양)
                        -light.direction :
                        representativePoint - posWorld; // (Area Light를 사용 안한다면 : light.position - posWorld;)
    
    float lightDist = length(lightVec); // Dirctional Light은 1.0이 되어야함
    lightVec /= lightDist; // Normalize
    
    // Spot Light
    float spotFactor = light.type & LIGHT_SPOT ?
                        // 빛이 향하는 방향으로 빛을 모아주는 강도
                        pow(max(-dot(lightVec, light.direction), 0.0), light.spotPower) :
                        1.0; // Directional Light이나 Point Light인 경우

    // Distance attenuation (거리에 따른 빛의 강도 가중치)
    float att = saturate((light.fallOffEnd - lightDist) / (light.fallOffEnd - light.fallOffStart));

    // Shadow Map
    float shadowFactor = 1.0; // 그림자가 없다면 ShadowFactor가 1.0
    
    if (light.type & LIGHT_SHADOW) {
        // Project posWorld to light screen
        // 조명을 시점처럼 생각해서 Projection
        float4 lightScreen = mul(float4(posWorld, 1.0), light.viewProj);
        // NDC 좌표계
        lightScreen.xyz /= lightScreen.w; // Homogenization
        
        // 광원에서 볼 때의 Texture 좌표 계산 (Shadow Map은 Texture이기 때문에 변환)
        // [-1.0, 1.0] x [-1.0, 1.0] -> [0.0, 1.0] x [0.0, 1.0]
        // 주의: Texture 좌표와 NDC는 y가 반대 (중요)
        float2 lightTexcoord = float2(lightScreen.x, -lightScreen.y);
        lightTexcoord = (lightTexcoord + 1.0) * 0.5;
        
        // 1번 방식 : 가장 간단한 방법
        //// Shadow Map에서 값 가져오기 (광원으로부터 가장 가까이 있는 물체의 거리)
        //float depth = shadowMap.Sample(shadowPointSampler, lightTexcoord).r; // Depth Only Buffer는 R32를 사용중
        
        //// 가려져 있다면 그림자로 표시
        //// (작은 bias = 0.001 정도가 필요, 수치오류를 방지하기 위함이고 bias가 없으면 그림자에 noise가 발생)
        //if (depth + 0.001 < lightScreen.z) // lightScreen의 z는 제일 먼 1.0일텐데 더 가까운게 있다는 의미
        //    shadowFactor = 0.0; // 0.0은 그림자가 있다는 의미로 빛의 강도를 0으로 만들어버림
        
        // 2번 방식 : ComparePointSampler를 사용해 주변의 값을 비교하고 결과를 만들어내기 때문에 더 부드러운 그림자 생성 (PCF 4 Sample 방식으로도 볼 수 있음)
        //shadowFactor = shadowMap.SampleCmpLevelZero(shadowCompare, lightTexcoord.xy, lightScreen.z - 0.001).r; // bias를 depth + bias가 아닌 lightScreen.z - bias하는 방식
        
        // 3번 방식 : Sampling을 여러번 해주면 더 부드럽게 그림자 렌더링을 할 수 있음
        //uint width, height, numMips;
        //// Texture의 정보를 가져오기 (Image Filter에서는 dx, dy 값을 계산해서 Constant Buffer로 넘겨줬지만 여기선 또 다른 방법인 GetDimensions()를 사용)
        //shadowMap.GetDimensions(0, width, height, numMips); // 해상도와 Mipmap의 개수 가져오기
        //// 그림자에 결이 생기는 걸 줄이고 쉽다면 가장 쉬운 방법은 dx값을 줄여주는 것 (ex: dx = 2.0 / (float)width;)
        //float dx = 5.0 / (float) width; // dx는 radius라고 생각하면 됨
        //// PCF 방식은 Depth Map을 Sampling했을떄 더 가까이에 있는 비율을 계산
        //float percentLit = 0.0; // 각 픽셀의 PCF 합
        //// 더 부드러운 결과를 위해 9번 Sampling
        //const float2 offsets[9] = {
        //    float2(-1, -1), float2(0, -1), float2(1, -1),
        //    float2(-1, 0), float2(0, 0), float2(1, 0),
        //    float2(-1, +1), float2(0, +1), float2(1, +1)
        //};
        
        //// Sampling한 수 중 몇개가 더 멀고 몇개가 더 가까운지 계산 (Sample 수가 늘어날수록 퀄리티가 올라가지만 연산량 증가)
        //[unroll]
        //for (int i = 0; i < 9; ++i) 
        //    percentLit += shadowMap.SampleCmpLevelZero(shadowCompare, lightTexcoord.xy + offsets[i] * dx, lightScreen.z - 0.001).r;

        //shadowFactor = percentLit / 9.0; // 평균
        
        // 4번 방식 PCSS
        uint width, height, numMips;
        shadowMap.GetDimensions(0, width, height, numMips);
        
        // Texel Size
        //float dx = 5.0 / (float) width;
        //shadowFactor = PCF_Filter(lightTexcoord.xy, lightScreen.z - 0.001, dx, shadowMap);
        shadowFactor = PCSS(lightTexcoord, lightScreen.z - 0.01, shadowMap, light.invProj, light.radius, light.nearPlane, light.frustumWidth);
    }
    
    // 빛의 강도
    float3 radiance = light.radiance * spotFactor * att * shadowFactor;
    
    return radiance;
}

// 직접광 (point light, spot light 등으로 직접 빛을 비추는 광원)
float3 DirectLighting(Light light, float3 posWorld, float3 pixelToEye, float3 normalWorld, float3 albedo, float metallic, float roughness, Texture2D shadowMap) {
    // Sphere Light 구현
    float3 L = light.position - posWorld;
    float3 r = normalize(reflect(-pixelToEye, normalWorld));
    float3 centerToRay = dot(L, r) * r - L; // Sphere Light 중심에서 r까지 수직인 벡터
    // RepresentativePoint는 World 좌표계에서 Shading할 Pixel 지점이 원점으로 계산해서 찾음
    float3 representativePoint = L + centerToRay * saturate(light.radius / length(centerToRay)); // Area Light를 대표하는 조명 지점
    // posWorld를 더해서 World 좌표계의 (0, 0, 0)이 원점이 되도록 변환
    representativePoint += posWorld;
    
    float3 lightVec = representativePoint - posWorld; // Area Light 사용
    //float3 lightVec = light.position - posWorld; // 일반 Light를 사용한 경우
    
    float lightDist = length(lightVec);
    lightVec /= lightDist;
    float3 halfway = normalize(pixelToEye + lightVec);
        
    float NdotI = max(0.0, dot(normalWorld, lightVec));
    float NdotH = max(0.0, dot(normalWorld, halfway));
    float NdotO = max(0.0, dot(normalWorld, pixelToEye));
    
    float3 F0 = lerp(Fdielectric, albedo, metallic); // 물체의 재질에 따라 F0을 결정
    float3 F = SchlickFresnel(F0, max(0.0, dot(halfway, pixelToEye)));
    
    // 에너지 보존 법칙에 따라 금속인지 비금속인지에 따라 Specular와 Diffuse의 비율을 계산
    // 완전 금속이면 kd = 0으로 diffuse 없이 Specular만 계산, 완전 비금속이면 kd = 1로 diffuse만 계산
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
      
    float3 diffuseBRDF = kd * albedo;
        
    // Sphere Normalization (Sphere Light) - Area Light는 Specular 범위가 넓어지므로 SphereNormalization으로 보정
    float alpha = roughness * roughness;
    float alphaPrime = saturate(alpha + light.radius / (2.0 * lightDist));
    
    float D = NdfGGX(NdotH, roughness, alphaPrime);
    float3 G = SchlickGGX(NdotI, NdotO, roughness);
    
    // 0으로 나누기 방지
    float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * NdotI * NdotO);
    
    // 빛의 강도
    float3 radiance = float3(0.0, 0.0, 0.0);
    radiance = LightRadiance(light, representativePoint, posWorld, normalWorld, shadowMap);
    
    // 마지막으로 빛의 강도와 조명을 향하는 방향과 시점 방향의 각도를 곱해 표면과 수평하면 빛이 안들어오도록 계산
    return (diffuseBRDF + specularBRDF) * radiance * NdotI;
}

PSOutput main(PSInput input)
{
    float3 pixelToEye = normalize(eyeWorld - input.posWorld);
    float3 normalWorld = GetNormal(input);
    
    float maxDist = 7.0f;
    float minDist = 3.0;
    
    float dist = length(eyeWorld - input.posWorld);
    //float lod = 10.0 * saturate((dist - minDist) / (maxDist - minDist));
    float lod = 0.0;
    
    float4 albedo = useAlbedoMap ?
        albedoTex.SampleLevel(linearWrapSampler, input.texcoord, lod) * float4(albedoFactor, 1.0)
        : float4(albedoFactor, 1.0);
    
    float ao = useAOMap ? aoTex.SampleLevel(linearWrapSampler, input.texcoord, lod).r : 1.0;
    
    // Metal Texture와 Roughness Texture는 한 Texture로 통합해서 각각 b와 g값을 가져와 사용
    float metallic = useMetallicMap ? metallicRoughnessTex.SampleLevel(linearWrapSampler, input.texcoord, lod).b * metallicFactor
                                    : metallicFactor;
    float roughness = useRoughnessMap ? metallicRoughnessTex.SampleLevel(linearWrapSampler, input.texcoord, lod).g * roughnessFactor
                                      : roughnessFactor;
    //float metallic = metallicFactor;
    //float roughness = roughnessFactor;
    float3 emission = useEmissiveMap ? emissiveTex.SampleLevel(linearWrapSampler, input.texcoord, lod).rgb
                                     : emissionFactor;

    // 간접광 (환경맵으로부터 받는 빛)
    float3 ambientLighting = AmbientLightingByIBL(albedo.rgb, normalWorld, pixelToEye, ao, metallic, roughness) * strengthIBL;
    
    // 직접광 (Direct Light) - Directional, Point, Spot Light, Sphere Light 등
    float3 directLighting = float3(0.0, 0.0, 0.0);
    
    // 임시로 unroll 사용
    [unroll] // warning X3557: loop only executes for 1 iteration(s), forcing loop to unroll
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].type) {
            float3 radiance = DirectLighting(lights[i], input.posWorld, pixelToEye, normalWorld, albedo.rgb, metallic, roughness, shadowMaps[i]);
            // TODO: radiance가 (0, 0, 0)인 경우 DirectLighting += ... 인데도 direfctLight이 (0, 0, 0)이 되어 버리는 오류 임시 수정
            if (abs(dot(float3(1, 1, 1), radiance)) > 1e-5) // radiance가 (0, 0, 0)일 경우 더하지 않음
                // DirectX 11에선 배열의 Indexing을 마음대로 할 수 없음
                // 그래서 Loop에 임시로 unroll을 사용함 (내부적으로 For loop를 풀어서 3번 반복하는 것으로 만들어줌)
                // TODO: Texture2D가 아닌 Texture2DArray로 만들어서 구현하는 방법이 제일 깔끔
                directLighting += radiance;
        }
    }
    
    PSOutput output;
    // emission은 물체 자체가 빛을 내는 부분이라 생각할 수 있는데 덧셈, 곱셈등으로 변화를 줄 수 있음
    output.pixelColor = float4(ambientLighting + directLighting + emission, 1.0);
    output.pixelColor = clamp(output.pixelColor, 0.0, 1000.0);
    output.indexColor = HashIdToColor(hashID);
    return output;
}