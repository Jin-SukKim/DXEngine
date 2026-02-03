#pragma once
#include "Particle.h"
#include "StructuredBuffer.h"
#include "IndirectArgsBuffer.h"

namespace DE {
	class ParticleSystem;

	constexpr UINT PAGE_SIZE = 1024;  // 페이지당 파티클 수
	constexpr UINT INVALID_PAGE = UINT_MAX;

	// 페이지 핸들 - 비연속 페이지들의 집합
	struct PageHandle {
		std::vector<UINT> pageIndices;      // 파티클 페이지 인덱스들 (비연속 가능)
		UINT pageCount = 0;
		UINT totalCapacity = 0;             // pageCount * PAGE_SIZE

		std::vector<UINT> emitterIDs;
		UINT emitterCount = 0;
		
		std::vector<UINT> spawnPosPageIndices;  // SpawnPos 페이지 인덱스들 (비연속 가능)
		UINT spawnPosPageCount = 0;
		
		UINT systemSlot = UINT_MAX;

		bool IsActive() const {
			return !pageIndices.empty() && !emitterIDs.empty() && systemSlot != UINT_MAX;
		}
		
		// SpawnPos 페이지 테이블 시작 인덱스 (첫 번째 할당된 페이지)
		UINT GetSpawnPosPageTableStart() const {
			return spawnPosPageIndices.empty() ? INVALID_PAGE : spawnPosPageIndices[0];
		}
	};

	// GPU에서 사용할 페이지 테이블 엔트리
	struct PageTableEntry {
		UINT baseOffset;    // 실제 버퍼 내 오프셋 (pageIndex * PAGE_SIZE)
		UINT ownerSystem;   // 소유 시스템 ID (디버깅용)
		UINT padding[2];
	};

	// 새로운 EmitterID (페이징 지원) - GPU용
	struct EmitterIDPaged {
		UINT emitterID;
		UINT pageTableStart;        // 파티클 페이지 테이블 시작 인덱스
		UINT pageCount;             // 파티클 페이지 수
		UINT localParticleMax;      // 이 Emitter의 최대 파티클 수
		UINT spawnPosPageTableStart;// SpawnPos 페이지 테이블 시작 인덱스
		UINT spawnPosPageCount;     // SpawnPos 페이지 수
		UINT padding[2];
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

		// 페이징 기반 할당/해제
		PageHandle Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount);
		void Free(const PageHandle& handle);

		// 페이지 테이블 업로드
		void UpdatePageTable();
		void UpdateSpawnPosPageTable();

		void SwapBuffer() { m_bufferIndex = 1 - m_bufferIndex; }
		void BindCompute();
		void UnbindCompute();
		void BindRender();
		void UnbindRender();
		void ClearWriteCount();

		void UploadConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleConsts>& data);
		void UploadFrameConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleFrameConsts>& data);
		void UpdateArgs();
		
		// SpawnPos 비연속 페이지 업로드
		void UploadSpawnPositions(const std::vector<UINT>& pageIndices, const std::vector<Vector3>& positions);

		void UpdateEmitterID(UINT slotIndex, const EmitterIDPaged& data);
		void BindEmitterID(UINT slotIndex);

		void UploadMeshConsts(UINT systemSlot, const ParticleMeshConsts& data);
		void BindMeshConsts(UINT systemSlot);

		// Getter
		StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_bufferIndex]; }
		StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_bufferIndex]; }
		StructuredBuffer<uint32_t>& GetReadCount() { return m_counts[m_bufferIndex]; }
		StructuredBuffer<uint32_t>& GetWriteCount() { return m_counts[1 - m_bufferIndex]; }
		StructuredBuffer<ParticleFrameConsts>& GetFrameConsts() { return m_frameConsts; }
		StructuredBuffer<ParticleConsts>& GetConsts() { return m_consts; }
		StructuredBuffer<PageTableEntry>& GetPageTable() { return m_pageTable; }
		StructuredBuffer<PageTableEntry>& GetSpawnPosPageTable() { return m_spawnPosPageTable; }
		IndirectArgsBuffer<DispatchArgs>& GetDispatchArgs() { return m_dispatchArgs; }
		IndirectArgsBuffer<DrawInstancedArgs>& GetBillboardArgs() { return m_billboardArgsBuffer; }
		IndirectArgsBuffer<DrawIndexedInstancedArgs>& GetMeshArgs() { return m_meshArgsBuffer; }
		
		UINT GetPageSize() const { return PAGE_SIZE; }
		UINT GetFreePageCount() const;
		UINT GetFreeSpawnPosPageCount() const;

	private:
		UINT AllocateSystemSlot();
		void FreeSystemSlot(UINT slot);
		
		// 파티클 페이지 할당 (비연속 허용)
		std::vector<UINT> AllocatePages(UINT requestedPages);
		void FreePages(const std::vector<UINT>& pages);
		
		// SpawnPos 페이지 할당 (비연속 허용)
		std::vector<UINT> AllocateSpawnPosPages(UINT requestedPages);
		void FreeSpawnPosPages(const std::vector<UINT>& pages);

	private:
		UINT m_maxParticles = 0;
		UINT m_maxEmitters = 0;
		UINT m_maxSystems = 100;
		UINT m_totalPages = 0;

		StructuredBuffer<Particle> m_particles[2];
		StructuredBuffer<uint32_t> m_counts[2];
		UINT m_bufferIndex = 0;

		StructuredBuffer<ParticleConsts> m_consts;
		StructuredBuffer<ParticleFrameConsts> m_frameConsts;

		// 파티클 페이지 테이블 (GPU에서 접근)
		StructuredBuffer<PageTableEntry> m_pageTable;
		std::vector<PageTableEntry> m_pageTableCPU;
		bool m_pageTableDirty = false;

		// SpawnPos 페이지 테이블 (GPU에서 접근)
		StructuredBuffer<PageTableEntry> m_spawnPosPageTable;
		std::vector<PageTableEntry> m_spawnPosPageTableCPU;
		bool m_spawnPosPageTableDirty = false;

		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
		IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;

		StructuredBuffer<Vector3> m_spawnPositions;

		std::vector<ConstantBuffer<EmitterIDPaged>> m_emitterIDBuffers;
		std::vector<ConstantBuffer<ParticleMeshConsts>> m_meshConstsBuffers;

		// 파티클 페이지 단위 Free List
		std::vector<UINT> m_freePageList;
		std::vector<bool> m_pageUsed;
		
		// SpawnPos 페이지 단위 Free List
		std::vector<UINT> m_freeSpawnPosList;
		std::vector<bool> m_spawnPosPageUsed;
		
		// Emitter/System 슬롯
		std::vector<bool> m_emitterSlotTable;
		std::vector<bool> m_systemSlotTable;
	};
}
