#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"
#include "IndirectArgsBuffer.h"

namespace DE {
	class ParticleSystem;

struct PoolHandle {
	UINT particleOffset = UINT_MAX;
	UINT blockCount = 0;
	UINT emitterID = UINT_MAX;
	UINT emitterCount = 0;
	UINT spawnPosOffset = UINT_MAX;
	UINT spawnPosBlockCount = 0;

	bool IsActive() const {
		return particleOffset != UINT_MAX && emitterID != UINT_MAX;
	}
};


struct ParticleMeshConsts {
	Matrix world;
	Matrix worldIT;
	UINT vertexCount;
	UINT indexCount;
	float padding[2];
};
class ParticleMemoryPool
{
public:
	void Initialize(UINT maxParticles = 1000000, UINT maxEmitters = 100, UINT maxSystems = 100);

	PoolHandle Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount);
	void Free(const PoolHandle& handle);
	void PlanDefragmentation(const std::vector<ParticleSystem*>& activeSystems);

	void SwapBuffer() { m_bufferIndex = 1 - m_bufferIndex; }
	void BindCompute();
	void UnbindCompute();
	void BindRender();
	void UnbindRender();
	void ClearWriteCount();

	void UploadConsts(UINT offset, const std::vector<ParticleConsts>& data);
	void UploadFrameConsts(UINT offset, const std::vector<ParticleFrameConsts>& data);
	void UpdateArgs();
	void UploadSpawnPositions(UINT offset, const std::vector<Vector3>& positions);

	// EmitterID ConstantBuffer 관리
	void UpdateEmitterID(UINT slotIndex, const EmitterID& data);
	void BindEmitterID(UINT slotIndex);

	StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_bufferIndex]; }
	StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetReadCount() { return m_counts[m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetWriteCount() { return m_counts[1 - m_bufferIndex]; }
	StructuredBuffer<ParticleFrameConsts>& GetFrameConsts() { return m_frameConsts; }
	IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
	IndirectArgsBuffer<DrawInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
	StructuredBuffer<Vector3>& GetSpawnPosBuffer() { return m_spawnPositions; }

	// MeshConsts 관리 (System별)
	void UploadMeshConsts(UINT systemIndex, const ParticleMeshConsts& data);
	void BindMeshConsts(UINT systemIndex);

private:
	UINT m_blockSize = 1024;
	UINT m_maxParticles = 0;
	UINT m_maxEmitters = 0;

	StructuredBuffer<Particle> m_particles[2];
	StructuredBuffer<uint32_t> m_counts[2];
	UINT m_bufferIndex = 0;

	StructuredBuffer<ParticleConsts> m_consts;
	StructuredBuffer<ParticleFrameConsts> m_frameConsts;

	IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
	IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;

	StructuredBuffer<Vector3> m_spawnPositions;

	// EmitterID ConstantBuffer Pool
	std::vector<ConstantBuffer<EmitterID>> m_emitterIDBuffers;

	// MeshConsts Pool (System별 - activeSystems 인덱스 사용)
	UINT m_maxSystems = 100;
	std::vector<ConstantBuffer<ParticleMeshConsts>> m_meshConstsBuffers;

	// Block Allocator
	std::vector<bool> m_particleBlockTable;
	std::vector<bool> m_emitterSlotTable;
	std::vector<bool> m_spawnPosBlockTable;
};

}
