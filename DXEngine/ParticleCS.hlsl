#include "ParticleCommon.hlsli"
#include "OrbitCS.hlsli"
#include "VortexCS.hlsli"

AppendStructuredBuffer<uint> deadIndices : register(u4);

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    uint aliveIdx = dtID.x;
    uint particleIdx = readAliveIndices[aliveIdx];
    
    Particle p = particles[particleIdx];
    
    EmitterID eID = emitterIDs[p.ownerID];

    uint emitterAliveCount = readAliveCount[p.ownerID];
    uint localIdx = aliveIdx - eID.readParticleOffset;
    if (localIdx >= emitterAliveCount)
        return;
    
    float dt = frameConsts[p.ownerID].dt;
    p.life -= dt;

    if (p.life <= 0.f) {
        // Return to dead list
        deadIndices.Append(particleIdx);
        return;
    }

    ForceConsts force = consts[p.ownerID].force;
    VisualConsts visual = consts[p.ownerID].visual;
    float ageRatio = 1.0f - (p.life / max(p.lifeMax, 0.0001f));

    // Curve sampling
    uint curveSlice = emitterIDs[p.ownerID].curveLUTSlice;
    float sizeCurve = SampleCurve(CURVE_SIZE, ageRatio, curveSlice);
    float colorCurve = SampleCurve(CURVE_COLOR, ageRatio, curveSlice);
    float alphaCurve = SampleCurve(CURVE_ALPHA, ageRatio, curveSlice);
    float dragCurve = SampleCurve(CURVE_DRAG, ageRatio, curveSlice);
    float gravCurve = SampleCurve(CURVE_GRAVITY, ageRatio, curveSlice);
    float velCurve = SampleCurve(CURVE_VELOCITY, ageRatio, curveSlice);

    // Force (Physics)
    // 
    // Gravity (curve multiplier, default 1.0 = unchanged)
    p.velocity += force.gravity * gravCurve * dt;

    // Drag (curve multiplier, default 1.0 = unchanged)
    p.velocity *= 1.0f / (1.0f + force.drag * dragCurve * dt);

    // Curl Noise Force
    if (force.curlNoiseEnabled) {
        float noiseCurve = SampleCurve(CURVE_NOISE_STRENGTH, ageRatio, curveSlice);
        float3 curlForce = SampleCurlNoiseTiling(p.position, force.curlNoiseFrequency, force.curlNoiseScrollSpeed, frameConsts[p.ownerID].time);
        p.velocity += curlForce * force.curlNoiseStrength * noiseCurve * dt;
    }

    // Vortex
    VortexConsts vortex = consts[p.ownerID].vortex;
    if (vortex.active)
    {
        CalculateVortex(p, vortex, dt, ageRatio);
    }

    // Orbit
    OrbitConsts orbit = consts[p.ownerID].orbit;
    if (orbit.active)
    {
        CalculateOrbit(p, orbit, dt, ageRatio);
    }

    // 위치 이동
    // Position update (velocity curve multiplier, default 1.0 = unchanged)
    p.position += p.velocity * velCurve * dt;

    // Visuals
    // Size: remap via curve (default linear 0->1 = existing behavior)
    p.size = lerp(visual.sizeRange.x, visual.sizeRange.y, sizeCurve);

    // Color: RGB remap + Alpha separate (default linear 0->1 = existing behavior)
    p.color.rgb = lerp(visual.startColor.rgb, visual.endColor.rgb, colorCurve);
    p.color.a = lerp(visual.startColor.a, visual.endColor.a, alphaCurve);

    //p.rotation += p.rotSpeed * dt;
    p.rotation = fmod(p.rotation + p.rotSpeed * dt, 6.28318530718f);

    // Write particle in-place
    particles[particleIdx] = p;

    // Append to write alive indices (compacted)
    uint writeSlot;
    InterlockedAdd(writeAliveCount[p.ownerID], 1, writeSlot);
    writeAliveIndices[eID.readParticleOffset + writeSlot] = particleIdx;
}