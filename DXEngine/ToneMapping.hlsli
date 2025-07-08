cbuffer ImageFilterConstData : register(b4) {
    // Texture의 Pixel 간격 (설정한 해상도에 따라 dx, dy값은 다름)
    float dx;
    float dy;
    float threadhold;
    float strength;
    // option 사용 (이미지 필터링에 사용되는 여러가지 변수들)
    float exposure; // option1 in C++ 
    float gamma; // option2 in C++
    float blur; // option3 in C++
    float option4; // 사용안함
};

// Tone Mapping 함수들
float3 FilmicToneMapping(float3 color) {
    color = max(float3(0, 0, 0), color);
    color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
};

float3 LinearToneMapping(float3 color) {
    float3 invGamma = float3(1, 1, 1) / gamma; // Gamma Correction

    color = clamp(exposure * color, 0., 1.);
    color = pow(color, invGamma);
    return color;
}

float3 Uncharted2ToneMapping(float3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    color = pow(color, float3(1.0, 1.0, 1.0) / gamma);
    return color;
}

float3 lumaBasedReinhardToneMapping(float3 color) {
    float3 invGamma = float3(1, 1, 1) / gamma;
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma / (1. + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, invGamma);
    return color;
}
