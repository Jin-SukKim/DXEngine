
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
	//uint activeCount;
};

//RWStructuredBuffer<Particle> particles : register(u0);
ConsumeStructuredBuffer<Particle> inputParticles : register(u0);
AppendStructuredBuffer<Particle> outputParticles : register(u1);


[numthreads(1024, 1, 1)]
void main(uint3 gID : SV_GroupID, int3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
	//if (dtID.x >= activeCount) return;

	Particle p = inputParticles.Consume();

	//p.position += p.velocity * dt * 0.5f;

	outputParticles.Append(p);
}