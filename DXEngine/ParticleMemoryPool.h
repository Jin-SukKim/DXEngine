#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"
#include "IndirectArgsBuffer.h"

namespace DE {
	class ParticleSystem;

// Spawn Position 타입 (Baked/Custom 통합)
enum class SpawnPosType : uint8_t {
	None = 0,
	Baked,      // Texture에서 Bake된 위치
	Custom      // 코드에서 지정한 위치
};

struct PoolHandle {
	UINT particleOffset = UINT_MAX;
	UINT blockCount = 0;
	UINT emitterID = UINT_MAX;
	UINT emitterCount = 0;
	
	// Baked/Custom 통합 → spawnPosOffset
	UINT spawnPosOffset = UINT_MAX;
	UINT spawnPosBlockCount = 0;

	bool IsActive() const {
		return particleOffset != UINT_MAX && emitterID != UINT_MAX;
	}
};

class ParticleMemoryPool
{
public:
	void Initialize(UINT maxParticles = 1000000, UINT maxEmitters = 10000);

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
	
	// 통합된 SpawnPositions 업로드 (Baked/Custom 공용)
	void UploadSpawnPositions(UINT offset, const std::vector<Vector3>& positions);

	StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_bufferIndex]; }
	StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetReadCount() { return m_counts[m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetWriteCount() { return m_counts[1 - m_bufferIndex]; }
	StructuredBuffer<ParticleFrameConsts>& GetFrameConsts() { return m_frameConsts; }
	IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
	IndirectArgsBuffer<DrawInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
	IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
	StructuredBuffer<Vector3>& GetSpawnPosBuffer() { return m_spawnPositions; }

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

	// Baked/Custom 통합 버퍼
	StructuredBuffer<Vector3> m_spawnPositions;

	// Block Allocator
	std::vector<bool> m_particleBlockTable;
	std::vector<bool> m_emitterSlotTable;
	std::vector<bool> m_spawnPosBlockTable;  // 통합된 테이블
};

}
