
struct Particle {
	float3 position;
	float3 velocity;
	float3 color;
	float life;
	float size;
};

StructuredBuffer<Particle> particles : register(t0);

struct PSInput {
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

PSInput main(uint vertexID : SV_VertexID)
{
	Particle p = particles[vertexID];

	PSInput output;
	output.position = float4(p.position.xyz, 1.0);
	output.color = p.color;

	return output;
}