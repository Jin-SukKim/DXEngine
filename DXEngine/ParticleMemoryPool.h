#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"
#include "IndirectArgsBuffer.h"

namespace DE {
	class ParticleSystem;

struct PoolHandle {
	std::vector<UINT> particleIndices;
	UINT blockCount = 0;
	std::vector<UINT> emitterIDs;
	UINT emitterCount = 0;
	UINT spawnPosOffset = UINT_MAX;
	UINT spawnPosBlockCount = 0;
	UINT systemSlot = UINT_MAX;
	UINT pageTableOffset = UINT_MAX;  // 추가: PageTable 내 오프셋

	bool IsActive() const {
		return !particleIndices.empty() && !emitterIDs.empty() && systemSlot != UINT_MAX;
	}

	bool operator==(const PoolHandle& other) {
		return particleIndices == other.particleIndices
			&& blockCount == other.blockCount
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

	PoolHandle Allocate(UINT reqParticleBlockCount, UINT reqEmitterCount, UINT reqSpawnPosCount);
	void Free(const PoolHandle& handle);
	
	// PageTable 관리 (간소화)
	UINT AppendToPageTable(const std::vector<UINT>& blockIndices);  // 끝에 추가, 오프셋 반환
	void RebuildPageTable(const std::vector<ParticleSystem*>& activeSystems);
	float GetFragmentationRatio() const;
	
	void SwapBuffer() { m_bufferIndex = 1 - m_bufferIndex; }
	void BindCompute();
	void UnbindCompute();
	void BindRender();
	void UnbindRender();
	void ClearWriteCount();

	void UploadConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleConsts>& data);
	void UploadFrameConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleFrameConsts>& data);
	void UpdateArgs();
	void UploadSpawnPositions(UINT offset, const std::vector<Vector3>& positions);

	void UpdateEmitterID(UINT slotIndex, const EmitterID& data);
	void BindEmitterID(UINT slotIndex);

	void UploadMeshConsts(UINT systemSlot, const ParticleMeshConsts& data);
	 void BindMeshConsts(UINT systemSlot);

	// Getters (기존과 동일)
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

	UINT GetBlockSize() { return m_blockSize; }
	UINT GetBlockCount() const { return m_blockCount; }
	UINT GetTotalBlockCount() const { return m_blockCount; }
	UINT GetBlockSize() const { return m_blockSize; }
	UINT GetUsedBlockCount() const {
		return (UINT)std::count(m_particleBlockTable.begin(), m_particleBlockTable.end(), true);
	}
	UINT GetTotalEmitterSlots() const { return m_maxEmitters; }
	UINT GetUsedEmitterSlots() const {
		return (UINT)std::count(m_emitterSlotTable.begin(), m_emitterSlotTable.end(), true);
	}
	UINT GetTotalSystemSlots() const { return m_maxSystems; }
	UINT GetUsedSystemSlots() const {
		return (UINT)std::count(m_systemSlotTable.begin(), m_systemSlotTable.end(), true);
	}
	const std::vector<bool>& GetParticleBlockTable() const { return m_particleBlockTable; }
	const std::vector<bool>& GetSpawnPosBlockTable() const { return m_spawnPosBlockTable; }

	UINT GetPageTableUsedSize() const { return m_pageTableUsedSize; }
private:
	UINT AllocateSystemSlot();
	void FreeSystemSlot(UINT slot);

private:
	UINT m_blockSize = 1024;
	UINT m_maxParticles = 0;
	UINT m_maxEmitters = 0;
	UINT m_maxSystems = 100;
	UINT m_blockCount = 0;

	StructuredBuffer<uint32_t> m_pageTable;
	StructuredBuffer<Particle> m_particles[2];
	StructuredBuffer<uint32_t> m_counts[2];
	UINT m_bufferIndex = 0;

	StructuredBuffer<ParticleConsts> m_consts;
	StructuredBuffer<ParticleFrameConsts> m_frameConsts;

	IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
	IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;

	StructuredBuffer<Vector3> m_spawnPositions;

	std::vector<ConstantBuffer<EmitterID>> m_emitterIDBuffers;
	std::vector<ConstantBuffer<ParticleMeshConsts>> m_meshConstsBuffers;

	std::vector<bool> m_particleBlockTable;
	std::vector<bool> m_emitterSlotTable;
	std::vector<bool> m_spawnPosBlockTable;
	std::vector<bool> m_systemSlotTable;

	// PageTable 관리 (간소화)
	std::vector<UINT> m_pageTableCPU;
	UINT m_pageTableUsedSize = 0;

	// private 멤버 추가
	UINT m_activeBlockCount = 0;  // 캐싱된 활성 블록 수
};

}
