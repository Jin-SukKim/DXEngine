#include "ParticleCommon.hlsli"

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    // 유효 범위를 벗어나면 리턴
    if (dtID.x >= readCount[readEmitterID])
        return;
    
    Particle p = readParticles[readParticleOffset + dtID.x];
    
    float dt = frameConsts[readEmitterID].dt;
    if (p.life - dt > 0.f) {
        p.life -= dt;
        
        ForceConsts force = consts[readEmitterID].force;
        // 1. 물리 연산 (Physics)

        // 중력 적용
        p.velocity += force.gravity * dt;

        // 공기 저항 (Drag) 적용
        // drag 값이 클수록 속도가 0에 빠르게 수렴
        p.velocity *= 1.0f / (1.0f + force.drag * dt);

        // 위치 갱신
        p.position += p.velocity * dt;

        // 2. 시각 효과 (Visuals)
        VisualConsts visual = consts[readEmitterID].visual;
        float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));
        // 크기 보간 (Start -> End)
        // sizeRange.x = Start Size, sizeRange.y = End Size
        p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, ageRatio);

        // 색상 보간 (Start -> End)
        p.color = lerp(visual.startColor, visual.endColor, ageRatio);

        //p.rotation += p.rotSpeed * dt;
        p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

        // 결과 저장
        uint index;
        InterlockedAdd(writeCount[writeEmitterID], 1, index);
        writeParticles[writeParticleOffset + index] = p;
    }
}