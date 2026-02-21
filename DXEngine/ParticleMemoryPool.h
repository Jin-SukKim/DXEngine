#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"
#include "IndirectArgsBuffer.h"
#include "ConstantData.h"
#include "AppendBuffer.h"
#include "BitonicSort.h"
#include <map>

namespace DE {
	class ParticleSystem;

struct PoolHandle {
	UINT particleOffset = UINT_MAX;
	UINT blockCount = 0;
	std::vector<UINT> emitterIDs;
	UINT emitterCount = 0;
	UINT spawnPosOffset = UINT_MAX;
	UINT spawnPosBlockCount = 0;
	UINT systemSlot = UINT_MAX;
	UINT meshVertexOffset = UINT_MAX;
	UINT meshVertexCount = 0;
	UINT meshIndexOffset = UINT_MAX;
	UINT meshIndexCount = 0;
	int modelIdx = -1;  // Track which model this handle uses (-1 = custom mesh)
	std::wstring bakedPosKey;  // Track which spawn pos cache entry this handle uses

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
	UINT vertexOffset;
	UINT indexOffset;
};

struct SpawnPosAllocation {
	UINT offset;
	UINT count;
	UINT blockCount;
	UINT refCount;

	SpawnPosAllocation()
		: offset(UINT_MAX), count(0), blockCount(0), refCount(0) {}

	SpawnPosAllocation(UINT off, UINT cnt, UINT blocks)
		: offset(off), count(cnt), blockCount(blocks), refCount(1) {}
};

struct MeshAllocation {
	UINT vertexOffset;
	UINT indexOffset;
	UINT vertexCount;
	UINT indexCount;
	UINT refCount;

	MeshAllocation()
		: vertexOffset(UINT_MAX)
		, indexOffset(UINT_MAX)
		, vertexCount(0)
		, indexCount(0)
		, refCount(0) {}

	MeshAllocation(UINT vOffset, UINT iOffset, UINT vCount, UINT iCount)
		: vertexOffset(vOffset)
		, indexOffset(iOffset)
		, vertexCount(vCount)
		, indexCount(iCount)
		, refCount(1) {}
};

class ParticleMemoryPool
{
public:
	void Initialize(UINT maxParticles = 1000000, UINT maxEmitters = 100, UINT maxSystems = 100);

	PoolHandle Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount);
	void Free(const PoolHandle& handle);
	
	void SwapAliveIndices() { m_aliveBufferIndex = 1 - m_aliveBufferIndex; }
	void BindSpawnCompute();
	void UnbindSpawnCompute();
	void BindSimulationCompute();
	void UnbindSimulationCompute();
	void BindRender();
	void BindBatchAliveIndices();
	void UnbindRender();
	void ClearWriteAliveCount();
	void ExcuteParticleLogic();

	void UploadConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleConsts>& data);
	void UpdateFrameConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleFrameConsts>& data);
	void UploadFrameConsts();
	void UpdateRenderConst(UINT emitterID, float spawnRatio);
	void UpdateArgs();
	void UpdateRenderArgs();
	void UploadSpawnPositions(UINT offset, const std::vector<Vector3>& positions);
	void UploadMeshVertices(UINT offset, const std::vector<Vector3>& vertices);
	void UploadMeshIndices(UINT offset, const std::vector<uint32_t>& indices);
	UINT AllocateMeshVertices(UINT count);
	UINT AllocateMeshIndices(UINT count);
	bool AllocateMeshForModel(int modelIdx, UINT vertexCount, UINT indexCount,
	                          UINT& outVertexOffset, UINT& outIndexOffset);
	bool AllocateSpawnPosForBakedPath(const std::wstring& bakedPath, UINT posCount, UINT& outOffset);

	// EmitterID ConstantBuffer
	void UpdateEmitterID(UINT slotIndex, const EmitterID& data);
	void BindEmitterID(UINT slotIndex);
	void UploadEmitterIDs();

	// Batch Rendering
	void UploadBatchData(const std::vector<UINT>& emitterList, const std::vector<BatchDescriptor>& descriptors);
	void BindBatchInfo(UINT emitterCount, UINT listOffset, UINT instanceOffset);
	void BindDefaultParticleMaterial();
	
	// MeshConsts 
	void UpdateMeshConsts(UINT systemSlot, const ParticleMeshConsts& data);
	void UploadMeshConsts();

	StructuredBuffer<Particle>& GetParticleBuffer() { return m_particles; }
	StructuredBuffer<uint32_t>& GetReadAliveIndices() { return m_aliveIndices[m_aliveBufferIndex]; }
	StructuredBuffer<uint32_t>& GetWriteAliveIndices() { return m_aliveIndices[1 - m_aliveBufferIndex]; }
	StructuredBuffer<uint32_t>& GetReadAliveCount() { return m_aliveCounts[m_aliveBufferIndex]; }
	StructuredBuffer<uint32_t>& GetWriteAliveCount() { return m_aliveCounts[1 - m_aliveBufferIndex]; }
	AppendBuffer<uint32_t>& GetDeadIndices() { return m_deadIndices; }
	StructuredBuffer<ParticleFrameConsts>& GetFrameConsts() { return m_frameConsts; }
	StructuredBuffer<ParticleConsts>& GetConsts() { return m_consts; }
	IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
	StructuredBuffer<Vector3>& GetSpawnPosBuffer() { return m_spawnPositions; }
	StructuredBuffer<Vector3>& GetMeshVertexPool() { return m_meshVertexPool; }
	StructuredBuffer<uint32_t>& GetMeshIndexPool() { return m_meshIndexPool; }
	StructuredBuffer<EmitterID>& GetEmitterIDs() { return m_emitterIDs; }

	// Batch buffers getters
	StructuredBuffer<UINT>& GetBatchEmitterList() { return m_batchEmitterList; }
	StructuredBuffer<BatchDescriptor>& GetBatchDescriptors() { return m_batchDescriptors; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetBatchBillboardArgs() { return m_batchBillboardArgs; }
	StructuredBuffer<UINT>& GetEmitterWriteOffsets() { return m_emitterWriteOffsets; }
	StructuredBuffer<UINT>& GetBatchAliveIndices() { return m_batchAliveIndices; }
	StructuredBuffer<BatchSortParam>& GetBatchSortParams() { return m_batchSortParams; }

	// Quad Mesh Getters
	ID3D11Buffer* GetQuadVertexBuffer() { return m_quadVertexBuffer.Get(); }
	ID3D11Buffer* GetQuadIndexBuffer() { return m_quadIndexBuffer.Get(); }
	UINT GetQuadVertexCount() const { return m_quadVertexCount; }
	UINT GetQuadIndexCount() const { return m_quadIndexCount; }

	// BitonicSort Getters
	StructuredBuffer<BitonicSort::Element>& GetSortElements() { return m_sortElements; }
	BitonicSort& GetBitonicSort() { return m_bitonicSort; }

	UINT GetBlockSize() { return m_blockSize; }
	UINT GetBlockCount() const { return m_blockCount; }

	UINT GetTotalBlockCount() const { return m_blockCount; }
	UINT GetBlockSize() const { return m_blockSize; }

	UINT GetUsedBlockCount() const {
		return m_cachedUsedBlocks;  // O(1) cached value
	}

	UINT GetTotalEmitterSlots() const { return m_maxEmitters; }
	UINT GetUsedEmitterSlots() const {
		return m_maxEmitters - (UINT)m_freeEmitterSlots.size();
	}

	UINT GetTotalSystemSlots() const { return m_maxSystems; }
	UINT GetUsedSystemSlots() const {
		return m_maxSystems - (UINT)m_freeSystemSlots.size();
	}

	const std::vector<bool>& GetParticleBlockTable() const {
		if (m_visualizationDirty) {
			RebuildVisualizationCache();
			m_visualizationDirty = false;
		}
		return m_particleBlockTableCache;
	}

	const std::vector<bool>& GetSpawnPosBlockTable() const {
		if (m_visualizationDirty) {
			RebuildVisualizationCache();
			m_visualizationDirty = false;
		}
		return m_spawnPosBlockTableCache;
	}

	std::vector<UINT> Defragment(const std::vector<PoolHandle>& activeHandles);
	void UpdateWriteOffset(UINT slotIndex, UINT newWriteOffset);

	void SyncReadOffset(UINT slotIndex);

	float GetFragmentationRatio() const;

#ifdef _DEBUG
	// Performance benchmarking getters
	UINT GetAllocateCallCount() const { return m_allocateCallCount; }
	UINT GetFreeCallCount() const { return m_freeCallCount; }
	double GetAvgAllocateTime() const {
		return m_allocateCallCount > 0 ? m_totalAllocateTime / m_allocateCallCount : 0.0;
	}
	double GetAvgFreeTime() const {
		return m_freeCallCount > 0 ? m_totalFreeTime / m_freeCallCount : 0.0;
	}
#endif

private:
	UINT m_blockSize = 1024;
	UINT m_maxParticles = 0;
	UINT m_maxEmitters = 0;
	UINT m_maxSystems = 100;
	UINT m_blockCount = 0;

	StructuredBuffer<Particle> m_particles;                // Single particle buffer (in-place update)
	StructuredBuffer<uint32_t> m_aliveIndices[2];          // Ping-pong alive indices (per-emitter)
	StructuredBuffer<uint32_t> m_aliveCounts[2];           // Ping-pong alive counts (per-emitter)
	AppendBuffer<uint32_t> m_deadIndices;
	UINT m_aliveBufferIndex = 0;

	StructuredBuffer<ParticleConsts> m_consts;
	StructuredBuffer<ParticleFrameConsts> m_frameConsts;

	IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
	IndirectArgsBuffer<DispatchArgs> m_batchDispatchArgs;  // 배치 dispatch용
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_billboardArgsBuffer; // 쿼드 메쉬 인스턴싱
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;

	StructuredBuffer<Vector3> m_spawnPositions;
	StructuredBuffer<Vector3> m_meshVertexPool;
	StructuredBuffer<uint32_t> m_meshIndexPool;

	// Quad Mesh for Billboard Instancing (GS 제거용)
	ComPtr<ID3D11Buffer> m_quadVertexBuffer;
	ComPtr<ID3D11Buffer> m_quadIndexBuffer;
	UINT m_quadVertexCount = 4;
	UINT m_quadIndexCount = 6;

	// EmitterID ConstantBuffer Pool
	StructuredBuffer<EmitterID> m_emitterIDs;
	ConstantBuffer<EmitterID> m_emitterIDBuffer;

	// MeshConsts Pool (System)
	StructuredBuffer<ParticleMeshConsts> m_meshConsts;

	// Batch Rendering Buffers
	StructuredBuffer<UINT> m_batchEmitterList;                      // Flat emitter ID list
	StructuredBuffer<BatchDescriptor> m_batchDescriptors;           // Per-batch metadata
	IndirectArgsBuffer<DrawIndexedInstancedArgs> m_batchBillboardArgs;  // Merged args
	StructuredBuffer<UINT> m_emitterWriteOffsets;                    // Pass1 -> Pass2
	StructuredBuffer<UINT> m_batchAliveIndices;                           // Pass2 -> VS (Batch Rendering)
	StructuredBuffer<BatchSortParam> m_batchSortParams;                   // Per-batch sort params (GPU-only)
	ConstantBuffer<BatchInfo> m_batchInfoBuffer;                    // CB5 for rendering
	ConstantBuffer<MaterialConstants> m_defaultParticleMaterialCB;  // For circle rendering (no texture)

	// BitonicSort Buffers (for AlphaBlend particle sorting)
	StructuredBuffer<BitonicSort::Element> m_sortElements;          // Per-emitter sort keys
	BitonicSort m_bitonicSort;                                      // Sorter instance

	UINT m_meshVertexPoolCapacity = 10000000;
	UINT m_meshIndexPoolCapacity = 30000000;
	UINT m_meshVertexNextOffset = 0;
	UINT m_meshIndexNextOffset = 0;

	// Model-based mesh caching (modelIdx -> allocation info)
	std::map<int, MeshAllocation> m_meshCache;

	// Spawn position caching (bakedPath or custom key -> allocation info)
	std::map<std::wstring, SpawnPosAllocation> m_spawnPosCache;

	// Block Allocator - Map-based (startBlock -> blockCount)
	std::map<UINT, UINT> m_particleBlockMap;
	std::map<UINT, UINT> m_spawnPosBlockMap;

	std::queue<UINT> m_freeEmitterSlots;
	std::queue<UINT> m_freeSystemSlots;

	// Cached fragmentation metrics (dirty flag pattern)
	mutable UINT m_cachedLastUsedBlock = 0;
	mutable UINT m_cachedTotalUsedBlocks = 0;
	mutable bool m_fragmentationDirty = true;

	// O(1) GetUsedBlockCount cached value
	mutable UINT m_cachedUsedBlocks = 0;

	// GUI visualization cache (lazy generation)
	mutable std::vector<bool> m_particleBlockTableCache;
	mutable std::vector<bool> m_spawnPosBlockTableCache;
	mutable bool m_visualizationDirty = true;

	// Helper method for visualization cache
	void RebuildVisualizationCache() const;

#ifdef _DEBUG
	// Performance benchmarking
	mutable UINT m_allocateCallCount = 0;
	mutable UINT m_freeCallCount = 0;
	mutable double m_totalAllocateTime = 0.0;  // microseconds
	mutable double m_totalFreeTime = 0.0;      // microseconds
#endif
};

}
