
struct Particle {
	float3 position;
	float3 velocity;
	float3 color;
	float life;
	float size;
};

cbuffer ParticleConsts : register(b0)
{
	float dt;
	float3 dummy;
};

RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
	Particle p = particles[dtID.x];

	p.position += p.velocity * dt * 0.5f;

	particles[dtID.x].position = p.position;
}