#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"

namespace DE {
	class ParticleSystem;

struct PoolHandle {
	UINT particleOffset = -1; // Pool내 시작 index (파티클 단위)
	UINT blockCount = 0; // 할당된 block 수
	UINT emitterID = -1; // Emitter Index
	UINT emitterCount = 0; // Emitter 개수

	bool IsActive() const {
		return particleOffset >= 0 && emitterID >= 0;
	}
};

class ParticleMemoryPool
{
public:
	void Initialize(UINT maxParticles = 1000000, UINT maxEmitters = 10000);

	// 파티클 수와 emitter 개수로 할당 요청
	PoolHandle Allocate(UINT reqParticleCount, UINT reqEmitterCount);

	// 반환
	void Free(const PoolHandle& handle);

	// 메모리 정리
	void PlanDefragmentation(const std::vector<ParticleSystem*>& activeSystems);

	void SwapBuffer() { m_bufferIndex = 1 - m_bufferIndex; }
	void BindCompute();
	void UnbindCompute();
	void BindRender();
	void UnbindRender();
	void ClearWriteCount();

	void UploadConsts(UINT offset, const std::vector<ParticleConsts>& data);
	void UploadFrameConsts(UINT offset, const std::vector<ParticleFrameConsts>& data);
	void UploadFrameConsts();

	StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_bufferIndex]; }
	StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetReadCount() { return m_counts[m_bufferIndex]; }
	StructuredBuffer<uint32_t>& GetWriteCount() { return m_counts[1 - m_bufferIndex]; }
	StructuredBuffer<ParticleFrameConsts>& GetFrameConsts() { return m_frameConsts; }
private:
	UINT m_blockSize = 1024; // 1block당 particle 수
	UINT m_maxParticles = 0;
	UINT m_maxEmitters = 0;

	StructuredBuffer<Particle> m_particles[2];
	StructuredBuffer<uint32_t> m_counts[2];
	UINT m_bufferIndex = 0;

	StructuredBuffer<ParticleConsts> m_consts;
	StructuredBuffer<ParticleFrameConsts> m_frameConsts;

	// Block Allocator
	std::vector<bool> m_particleBlockTable; // TODO: Bitmap 방식 사용
	std::vector<bool> m_emitterSlotTable;
};

}
