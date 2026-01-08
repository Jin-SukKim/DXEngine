Buffer<uint> particleCountBuffer : register(t0);

RWBuffer<uint> indirectArgs : register(u0);
RWBuffer<uint> drawArgs : register(u1);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint particleCount = particleCountBuffer[0];

	indirectArgs[0] = (particleCount + 256) / 256;
	indirectArgs[1] = 1;
	indirectArgs[2] = 1;

	drawArgs[0] = particleCount;
	drawArgs[1] = 1;
	drawArgs[2] = 0;
	drawArgs[3] = 0;
}