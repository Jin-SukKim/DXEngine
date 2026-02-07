#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"
#include "IndirectArgsBuffer.h"

namespace DE {
	class ParticleSystem;

	// 배치 렌더링에 필요한 최대 수
	static constexpr UINT MAX_BATCH_GROUPS = 32;
	static constexpr UINT MAX_BATCH_EMITTERS = 256;

struct PoolHandle {
	UINT particleOffset = UINT_MAX;
	UINT blockCount = 0;
	std::vector<UINT> emitterIDs;
	UINT emitterCount = 0;
	UINT spawnPosOffset = UINT_MAX;
	UINT spawnPosBlockCount = 0;
	UINT systemSlot = UINT_MAX;

	bool IsActive() const {
		return particleOffset != UINT_MAX && !emitterIDs.empty() && systemSlot != UINT_MAX;
	}

	bool operator==(const PoolHandle& other) {
		return particleOffset == other.particleOffset
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

	PoolHandle Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount);
	void Free(const PoolHandle& handle);
	
	void SwapBuffer() { m_bufferIndex = 1 - m_bufferIndex; }
	void BindCompute();
	void UnbindCompute();
	void BindRender();
	void UnbindRender();
	void ClearWriteCount();
	void ExcuteParticleLogic();

	void UploadConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleConsts>& data);
	void UpdateFrameConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleFrameConsts>& data);
	void UploadFrameConsts();
	void UpdateArgs();
	void UpdateRenderArgs();
	void UploadSpawnPositions(UINT offset, const std::vector<Vector3>& positions);

	// EmitterID ConstantBuffer ����
	void UpdateEmitterID(UINT slotIndex, const EmitterID& data);
	void BindEmitterID(UINT slotIndex);
	void UploadEmitterIDs();
	
	// MeshConsts ����
	void UpdateMeshConsts(UINT systemSlot, const ParticleMeshConsts& data);
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
	StructuredBuffer<EmitterID>& GetEmitterIDs() { return m_emitterIDs; }
	std::vector<ParticleMeshConsts>& GetMeshConsts() { return m_meshConstsCPU; }

	// ========== 배치 렌더링 ==========
	void UploadBatchData(const std::vector<BatchGroup>& batches);
	void UpdateBatchArgs(UINT batchCount);
	void BindBatchRender(UINT batchIndex);
	void UnbindBatchRender();

	IndirectArgsBuffer<DrawInstancedArgs>& GetBatchBillboardArgs() { return m_batchBillboardArgs; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetBatchMeshArgs() { return m_batchMeshArgs; }

	UINT GetBatchBillboardArgsOffset(UINT batchIdx) { return batchIdx * sizeof(DrawInstancedArgs); }
	UINT GetBatchMeshArgsOffset(UINT batchIdx) { return batchIdx * sizeof(DrawIndexedInstancedArgs); }

	UINT GetBlockSize() { return m_blockSize; }
	UINT GetBlockCount() const { return m_blockCount; }

	// Debug
	// ������ Getter �߰�
	UINT GetTotalBlockCount() const { return m_blockCount; }
	UINT GetBlockSize() const { return m_blockSize; }

	// ��� ���� ���� ���� ī��Ʈ
	UINT GetUsedBlockCount() const {
		return (UINT)std::count(m_particleBlockTable.begin(), m_particleBlockTable.end(), true);
	}

	// ��ü/����� Emitter ����
	UINT GetTotalEmitterSlots() const { return m_maxEmitters; }
	UINT GetUsedEmitterSlots() const {
		return (UINT)std::count(m_emitterSlotTable.begin(), m_emitterSlotTable.end(), true);
	}

	// ��ü/����� System ����
	UINT GetTotalSystemSlots() const { return m_maxSystems; }
	UINT GetUsedSystemSlots() const {
		return (UINT)std::count(m_systemSlotTable.begin(), m_systemSlotTable.end(), true);
	}

	// �ð�ȭ�� ���� ���̺� ��ü�� ���� ���� ��ȯ (const)
	const std::vector<bool>& GetParticleBlockTable() const { return m_particleBlockTable; }
	const std::vector<bool>& GetSpawnPosBlockTable() const { return m_spawnPosBlockTable; }

	// ParticleMemoryPool.h�� �߰�
	std::vector<UINT> CalculateDefragmentedOffsets(const std::vector<PoolHandle>& activeHandles);
	void UpdateBlockTable(const std::vector<PoolHandle>& activeHandles);
	std::vector<UINT> Defragment(const std::vector<PoolHandle>& activeHandles);
	void UpdateWriteOffset(UINT slotIndex, UINT newWriteOffset);

	void SyncReadOffset(UINT slotIndex);

	// public ��� �Լ��� �߰�
	float GetFragmentationRatio() const;

private:
	// System Slot ����
	UINT AllocateSystemSlot();
	void FreeSystemSlot(UINT slot);

private:
	UINT m_blockSize = 1024;
	UINT m_maxParticles = 0;
	UINT m_maxEmitters = 0;
	UINT m_maxSystems = 100;
	UINT m_blockCount = 0;

	StructuredBuffer<Particle> m_particles[2];
	StructuredBuffer<uint32_t> m_counts[2];
	UINT m_bufferIndex = 0;

	StructuredBuffer<ParticleConsts> m_consts;
	StructuredBuffer<ParticleFrameConsts> m_frameConsts;

	IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
	IndirectArgsBuffer<DispatchArgs> m_batchDispatchArgs;  // 배치 dispatch용
	IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;

	StructuredBuffer<Vector3> m_spawnPositions;

	// EmitterID ConstantBuffer Pool
	StructuredBuffer<EmitterID> m_emitterIDs;
	ConstantBuffer<EmitterID> m_emitterIDBuffer;

	// MeshConsts Pool (System��)
	std::vector<ParticleMeshConsts> m_meshConstsCPU;
	ConstantBuffer<ParticleMeshConsts> m_meshConstsBuffer;

	// Block Allocator
	std::vector<bool> m_particleBlockTable;
	std::vector<bool> m_emitterSlotTable;
	std::vector<bool> m_spawnPosBlockTable;
	std::vector<bool> m_systemSlotTable;

	// ========== 배치 렌더링 버퍼 ==========
	StructuredBuffer<BatchEmitterInfo> m_batchEmitterInfo;       // GPU 배치 매핑 (입력+출력)
	StructuredBuffer<UINT> m_batchOffsets;                       // 각 배치의 시작 인덱스
	StructuredBuffer<UINT> m_batchCounts;                        // 각 배치의 emitter 수
	IndirectArgsBuffer<DrawInstancedArgs> m_batchBillboardArgs;  // 배치별 billboard draw args
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_batchMeshArgs; // 배치별 mesh draw args
	ConstantBuffer<BatchConsts> m_batchConstsBuffer;             // 배치별 상수
};

}
