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
	UINT systemSlot = UINT_MAX;

	bool IsActive() const {
		return particleOffset != UINT_MAX && emitterID != UINT_MAX && systemSlot != UINT_MAX;
	}

	bool operator==(const PoolHandle& other) {
		return particleOffset == other.particleOffset
			&& blockCount == other.blockCount
			&& emitterID == other.emitterID
			&& emitterCount == other.emitterCount
			&& spawnPosOffset == other.spawnPosOffset
			&& spawnPosBlockCount == other.spawnPosBlockCount
			&& systemSlot == other.systemSlot;
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

	// EmitterID ConstantBuffer 包府
	void UpdateEmitterID(UINT slotIndex, const EmitterID& data);
	void UpdateEmitterID(UINT slotIndex, const PoolHandle& next, const UINT& emitterID);
	void BindEmitterID(UINT slotIndex);

	// MeshConsts 包府
	void UploadMeshConsts(UINT systemSlot, const ParticleMeshConsts& data);
	void BindMeshConsts(UINT systemSlot);

	StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_bufferIndex]; }
	StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetReadCount() { return m_counts[m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetWriteCount() { return m_counts[1 - m_bufferIndex]; }
	StructuredBuffer<ParticleFrameConsts>& GetFrameConsts() { return m_frameConsts; }
	StructuredBuffer<ParticleConsts>& GetConsts() { return m_consts; }
	IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
	IndirectArgsBuffer<DrawInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
	StructuredBuffer<Vector3>& GetSpawnPosBuffer() { return m_spawnPositions; }
	std::vector<ConstantBuffer<EmitterID>>& GetEmitterIDs() { return m_emitterIDBuffers; }
	std::vector<ConstantBuffer<ParticleMeshConsts>>& GetMeshConsts() { return m_meshConstsBuffers; }

	bool IsDefragStarted() const { return m_startDefrag; }
	void FinishDefrag() { m_startDefrag = false; }
	UINT GetBlockSize() { return m_blockSize; }
private:
	// System Slot 包府
	UINT AllocateSystemSlot();
	void FreeSystemSlot(UINT slot);

private:
	UINT m_blockSize = 1024;
	UINT m_maxParticles = 0;
	UINT m_maxEmitters = 0;
	UINT m_maxSystems = 100;

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

	// MeshConsts Pool (System喊)
	std::vector<ConstantBuffer<ParticleMeshConsts>> m_meshConstsBuffers;

	// Block Allocator
	std::vector<bool> m_particleBlockTable;
	std::vector<bool> m_emitterSlotTable;
	std::vector<bool> m_spawnPosBlockTable;
	std::vector<bool> m_systemSlotTable;

	bool m_startDefrag = false;
};

}
