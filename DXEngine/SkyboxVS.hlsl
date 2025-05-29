#include "Common.hlsli"

struct SkyboxPSInput {
    float4 posProj : SV_POSITION;
    float3 posModel : POSITION;
};

SkyboxPSInput main(VSInput input) {
    SkyboxPSInput output;
    output.posModel = input.posModel;
    // 모델을 이동, 회전시키면 세상은 움직이면 안되지만 뷰가 변환하면 세상도 같이 움직여야 함
    // 이때 회전 변환만 적용하기 위해서 4번째 값을 1.0이 아닌 0.0으로 변환 적용
    output.posProj = mul(float4(input.posModel, 0.0), view); 
    // 이후 제대로 proj 적용
    output.posProj = mul(float4(output.posProj.xyz, 1.0), proj);
    return output;
}