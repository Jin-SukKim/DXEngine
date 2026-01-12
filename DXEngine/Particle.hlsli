#ifndef __PARTICLE_HLSLI__
#define __PARTICLE_HLSLI__

struct Particle
{
    float3 position;
    float3 velocity;
    float3 color;
    float life;
    float lifeMax;
    float size;
};

#endif // __PARTICLE_HLSLI__