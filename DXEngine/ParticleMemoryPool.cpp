#include "pch.h"
#include "ParticleMemoryPool.h"
#include "ParticleSystem.h"

namespace DE {
	void ParticleMemoryPool::Initialize(UINT maxParticles, UINT maxEmitters, UINT maxSystems)
	{
		ID3D11Device* device = GET_SINGLE(RenderBase)->GetDevice().Get();
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		m_maxParticles = maxParticles;
		m_maxEmitters = maxEmitters;
		m_maxSystems = maxSystems;
		m_totalPages = (maxParticles + PAGE_SIZE - 1) / PAGE_SIZE;

		// 파티클 페이지 테이블 초기화
		m_pageUsed.assign(m_totalPages, false);
		m_freePageList.reserve(m_totalPages);
		for (UINT i = 0; i < m_totalPages; ++i) {
			m_freePageList.push_back(i);
		}

		m_pageTableCPU.resize(m_totalPages);
		for (UINT i = 0; i < m_totalPages; ++i) {
			m_pageTableCPU[i].baseOffset = i * PAGE_SIZE;
			m_pageTableCPU[i].ownerSystem = UINT_MAX;
			m_pageTableCPU[i].padding[0] = 0;
			m_pageTableCPU[i].padding[1] = 0;
		}

		// SpawnPos 페이지 테이블 초기화 (파티클과 동일한 구조)
		UINT spawnPosPages = m_totalPages;
		m_spawnPosPageUsed.assign(spawnPosPages, false);
		m_freeSpawnPosList.reserve(spawnPosPages);
		for (UINT i = 0; i < spawnPosPages; ++i) {
			m_freeSpawnPosList.push_back(i);
		}

		m_spawnPosPageTableCPU.resize(spawnPosPages);
		for (UINT i = 0; i < spawnPosPages; ++i) {
			m_spawnPosPageTableCPU[i].baseOffset = i * PAGE_SIZE;
			m_spawnPosPageTableCPU[i].ownerSystem = UINT_MAX;
			m_spawnPosPageTableCPU[i].padding[0] = 0;
			m_spawnPosPageTableCPU[i].padding[1] = 0;
		}

		m_emitterSlotTable.assign(maxEmitters, false);
		m_systemSlotTable.assign(maxSystems, false);

		// GPU 버퍼 초기화
		for (UINT i = 0; i < 2; ++i) {
			m_particles[i].Initialize(device, maxParticles);
			m_counts[i].Initialize(device, maxEmitters);

			std::vector<uint32_t> initialCount(maxEmitters, 0);
			m_counts[i].SetData(initialCount);
			m_counts[i].Upload(context);
		}

		m_consts.Initialize(device, maxEmitters);
		m_frameConsts.Initialize(device, maxEmitters);
		m_pageTable.Initialize(device, m_totalPages);
		m_spawnPosPageTable.Initialize(device, spawnPosPages);

		std::vector<DispatchArgs> initialDispatch(m_maxEmitters, { 0, 1, 1 });
		m_dispatchArgs.Initialize(device, initialDispatch, m_maxEmitters, sizeof(DispatchArgs), 3);

		std::vector<DrawInstancedArgs> initialBillboardArgs(m_maxEmitters, { 0, 1, 0, 0 });
		m_billboardArgsBuffer.Initialize(device, initialBillboardArgs, m_maxEmitters, sizeof(DrawInstancedArgs), 4);

		std::vector<DrawIndexedInstancedArgs> initialMeshArgs(m_maxEmitters, { 0, 0, 0, 0, 0 });
		m_meshArgsBuffer.Initialize(device, initialMeshArgs, m_maxEmitters, sizeof(DrawIndexedInstancedArgs), 5);

		m_spawnPositions.Initialize(device, maxParticles);

		m_emitterIDBuffers.resize(maxEmitters);
		for (UINT i = 0; i < maxEmitters; ++i) {
			m_emitterIDBuffers[i].Initialize();
		}

		m_meshConstsBuffers.resize(maxSystems);
		for (UINT i = 0; i < maxSystems; ++i) {
			m_meshConstsBuffers[i].Initialize();
		}

		// 초기 페이지 테이블 업로드
		m_pageTable.SetData(m_pageTableCPU);
		m_pageTable.Upload(context);
		
		m_spawnPosPageTable.SetData(m_spawnPosPageTableCPU);
		m_spawnPosPageTable.Upload(context);
	}

	std::vector<UINT> ParticleMemoryPool::AllocatePages(UINT requestedPages)
	{
		std::vector<UINT> allocated;
		
		if (m_freePageList.size() < requestedPages) {
			return allocated;
		}

		allocated.reserve(requestedPages);
		
		for (UINT i = 0; i < requestedPages; ++i) {
			UINT pageIdx = m_freePageList.back();
			m_freePageList.pop_back();
			m_pageUsed[pageIdx] = true;
			allocated.push_back(pageIdx);
		}

		m_pageTableDirty = true;
		return allocated;
	}

	void ParticleMemoryPool::FreePages(const std::vector<UINT>& pages)
	{
		for (UINT pageIdx : pages) {
			if (pageIdx < m_totalPages && m_pageUsed[pageIdx]) {
				m_pageUsed[pageIdx] = false;
				m_freePageList.push_back(pageIdx);
				m_pageTableCPU[pageIdx].ownerSystem = UINT_MAX;
			}
		}
		m_pageTableDirty = true;
	}

	std::vector<UINT> ParticleMemoryPool::AllocateSpawnPosPages(UINT requestedPages)
	{
		std::vector<UINT> allocated;
		
		if (m_freeSpawnPosList.size() < requestedPages) {
			return allocated;
		}

		allocated.reserve(requestedPages);
		
		for (UINT i = 0; i < requestedPages; ++i) {
			UINT pageIdx = m_freeSpawnPosList.back();
			m_freeSpawnPosList.pop_back();
			m_spawnPosPageUsed[pageIdx] = true;
			allocated.push_back(pageIdx);
		}

		m_spawnPosPageTableDirty = true;
		return allocated;
	}

	void ParticleMemoryPool::FreeSpawnPosPages(const std::vector<UINT>& pages)
	{
		for (UINT pageIdx : pages) {
			if (pageIdx < m_spawnPosPageUsed.size() && m_spawnPosPageUsed[pageIdx]) {
				m_spawnPosPageUsed[pageIdx] = false;
				m_freeSpawnPosList.push_back(pageIdx);
				m_spawnPosPageTableCPU[pageIdx].ownerSystem = UINT_MAX;
			}
		}
		m_spawnPosPageTableDirty = true;
	}

	UINT ParticleMemoryPool::AllocateSystemSlot()
	{
		for (UINT i = 0; i < m_maxSystems; ++i) {
			if (!m_systemSlotTable[i]) {
				m_systemSlotTable[i] = true;
				return i;
			}
		}
		return UINT_MAX;
	}

	void ParticleMemoryPool::FreeSystemSlot(UINT slot)
	{
		if (slot < m_maxSystems) {
			m_systemSlotTable[slot] = false;
		}
	}

	PageHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
		PageHandle handle;

		// 1. System Slot 할당
		handle.systemSlot = AllocateSystemSlot();
		if (handle.systemSlot == UINT_MAX) {
			return handle;
		}

		// 2. 파티클 페이지 할당 (비연속 가능)
		UINT neededPages = (reqParticleCount + PAGE_SIZE - 1) / PAGE_SIZE;
		handle.pageIndices = AllocatePages(neededPages);
		
		if (handle.pageIndices.empty() && neededPages > 0) {
			FreeSystemSlot(handle.systemSlot);
			handle.systemSlot = UINT_MAX;
			return handle;
		}

		handle.pageCount = neededPages;
		handle.totalCapacity = neededPages * PAGE_SIZE;

		// 3. Emitter 슬롯 할당
		for (UINT i = 0; i < m_emitterSlotTable.size() && handle.emitterIDs.size() < reqEmitterCount; ++i) {
			if (!m_emitterSlotTable[i]) {
				m_emitterSlotTable[i] = true;
				handle.emitterIDs.push_back(i);
			}
		}

		if (handle.emitterIDs.size() < reqEmitterCount) {
			// 롤백
			FreePages(handle.pageIndices);
			for (UINT id : handle.emitterIDs) {
				m_emitterSlotTable[id] = false;
			}
			FreeSystemSlot(handle.systemSlot);
			handle = PageHandle{};
			return handle;
		}
		handle.emitterCount = reqEmitterCount;

		// 4. SpawnPos 페이지 할당 (비연속 가능)
		if (reqSpawnPosCount > 0) {
			UINT spawnPages = (reqSpawnPosCount + PAGE_SIZE - 1) / PAGE_SIZE;
			handle.spawnPosPageIndices = AllocateSpawnPosPages(spawnPages);
			handle.spawnPosPageCount = static_cast<UINT>(handle.spawnPosPageIndices.size());
			
			// SpawnPos 페이지 테이블에 owner 설정
			for (UINT pageIdx : handle.spawnPosPageIndices) {
				m_spawnPosPageTableCPU[pageIdx].ownerSystem = handle.systemSlot;
			}
		}

		// 파티클 페이지 테이블에 owner 설정
		for (UINT pageIdx : handle.pageIndices) {
			m_pageTableCPU[pageIdx].ownerSystem = handle.systemSlot;
		}

		return handle;
	}

	void ParticleMemoryPool::Free(const PageHandle& handle)
	{
		if (!handle.IsActive()) return;

		FreeSystemSlot(handle.systemSlot);
		FreePages(handle.pageIndices);

		for (UINT id : handle.emitterIDs) {
			if (id < m_emitterSlotTable.size()) {
				m_emitterSlotTable[id] = false;
			}
		}

		// SpawnPos 페이지 해제 (비연속 인덱스 목록 사용)
		FreeSpawnPosPages(handle.spawnPosPageIndices);
	}

	void ParticleMemoryPool::UpdatePageTable()
	{
		if (!m_pageTableDirty) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();
		m_pageTable.SetData(m_pageTableCPU);
		m_pageTable.Upload(context.Get());
		m_pageTableDirty = false;
	}

	void ParticleMemoryPool::UpdateSpawnPosPageTable()
	{
		if (!m_spawnPosPageTableDirty) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();
		m_spawnPosPageTable.SetData(m_spawnPosPageTableCPU);
		m_spawnPosPageTable.Upload(context.Get());
		m_spawnPosPageTableDirty = false;
	}

	UINT ParticleMemoryPool::GetFreePageCount() const
	{
		return static_cast<UINT>(m_freePageList.size());
	}

	UINT ParticleMemoryPool::GetFreeSpawnPosPageCount() const
	{
		return static_cast<UINT>(m_freeSpawnPosList.size());
	}

	void ParticleMemoryPool::BindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		// 페이지 테이블 업데이트
		UpdatePageTable();
		UpdateSpawnPosPageTable();

		ID3D11UnorderedAccessView* uavs[] = { 
			GetWriteBuffer().GetUAV(),
			GetWriteCount().GetUAV()
		};
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		ID3D11ShaderResourceView* srvs[] = { 
			GetReadBuffer().GetSRV(),           // t6
			GetReadCount().GetSRV(),            // t7
			m_frameConsts.GetSRV(),             // t8
			m_consts.GetSRV(),                  // t9
			m_spawnPositions.GetSRV(),          // t10
			m_pageTable.GetSRV(),               // t11
			m_spawnPosPageTable.GetSRV()        // t12
		};
		context->CSSetShaderResources(6, 7, srvs);
	}

	void ParticleMemoryPool::UnbindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		ID3D11UnorderedAccessView* uavs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 7, srvs);
	}
	
	void ParticleMemoryPool::BindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = {
			GetReadBuffer().GetSRV(),           // t6
			GetReadCount().GetSRV(),            // t7
			m_frameConsts.GetSRV(),             // t8
			m_consts.GetSRV(),                  // t9
			m_spawnPositions.GetSRV(),          // t10
			m_pageTable.GetSRV(),               // t11
			m_spawnPosPageTable.GetSRV()        // t12
		};
		context->CSSetShaderResources(6, 7, srvs);
		context->VSSetShaderResources(6, 7, srvs);
		context->PSSetShaderResources(6, 7, srvs);
	}

	void ParticleMemoryPool::UnbindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 7, srvs);
		context->VSSetShaderResources(6, 7, srvs);
		context->PSSetShaderResources(6, 7, srvs);
	}

	void ParticleMemoryPool::ClearWriteCount()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		const UINT clearVal[1] = { 0 };
		context->ClearUnorderedAccessViewUint(GetWriteCount().GetUAV(), clearVal);
		context->CSSetShaderResources(8, 1, m_consts.GetAddressOfSRV());
	}

	void ParticleMemoryPool::UploadConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleConsts>& data)
	{
		if (emitterIDs.size() != data.size()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < emitterIDs.size(); ++i) {
			D3D11_BOX box;
			box.left = emitterIDs[i] * sizeof(ParticleConsts);
			box.right = static_cast<UINT>(box.left + sizeof(ParticleConsts));
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

			const void* pSrcData = data.data() + i;
			context->UpdateSubresource(m_consts.GetBuffer(), 0, &box, pSrcData, 0, 0);
		}
	}

	void ParticleMemoryPool::UploadFrameConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleFrameConsts>& data)
	{
		if (emitterIDs.size() != data.size()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < emitterIDs.size(); ++i) {
			D3D11_BOX box;
			box.left = emitterIDs[i] * sizeof(ParticleFrameConsts);
			box.right = static_cast<UINT>(box.left + sizeof(ParticleFrameConsts));
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

			const void* pSrcData = data.data() + i;
			context->UpdateSubresource(m_frameConsts.GetBuffer(), 0, &box, pSrcData, 0, 0);
		}
	}

	void ParticleMemoryPool::UpdateArgs()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		auto& argsUpdateCS = RenderBase::computeCommon.particle.argsUpdateCS;
		context->CSSetShader(argsUpdateCS.computeShader.Get(), nullptr, 0);

		ID3D11UnorderedAccessView* uavs[] = { 
			m_dispatchArgs.GetUAV(), 
			m_billboardArgsBuffer.GetUAV(),
			m_meshArgsBuffer.GetUAV() 
		};
		context->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);

		context->Dispatch((m_maxEmitters + 255) / 256, 1, 1);

		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 3, nullUAVs, nullptr);
	}

	void ParticleMemoryPool::UploadSpawnPositions(const std::vector<UINT>& pageIndices, const std::vector<Vector3>& positions)
	{
		if (positions.empty() || pageIndices.empty()) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext();
		
		size_t posIdx = 0;
		for (UINT pageIdx : pageIndices) {
			if (posIdx >= positions.size()) break;
			
			UINT offset = pageIdx * PAGE_SIZE;
			UINT countThisPage = std::min(
				static_cast<UINT>(PAGE_SIZE), 
				static_cast<UINT>(positions.size() - posIdx)
			);
			
			D3D11_BOX box;
			box.left = offset * sizeof(Vector3);
			box.right = (offset + countThisPage) * sizeof(Vector3);
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;
			
			context->UpdateSubresource(
				m_spawnPositions.GetBuffer(), 0, &box, 
				positions.data() + posIdx, 0, 0
			);
			
			posIdx += countThisPage;
		}
	}

	void ParticleMemoryPool::UpdateEmitterID(UINT slotIndex, const EmitterIDPaged& data)
	{
		if (slotIndex >= m_maxEmitters) return;
		
		m_emitterIDBuffers[slotIndex].SetCpuData(data);
		m_emitterIDBuffers[slotIndex].Upload();
	}

	void ParticleMemoryPool::BindEmitterID(UINT slotIndex)
	{
		if (slotIndex >= m_maxEmitters) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(5, 1, m_emitterIDBuffers[slotIndex].GetAddressOf());
		context->VSSetConstantBuffers(5, 1, m_emitterIDBuffers[slotIndex].GetAddressOf());
		context->PSSetConstantBuffers(5, 1, m_emitterIDBuffers[slotIndex].GetAddressOf());
	}

	void ParticleMemoryPool::UploadMeshConsts(UINT systemIndex, const ParticleMeshConsts& data)
	{
		if (systemIndex >= m_maxSystems) return;
		
		m_meshConstsBuffers[systemIndex].SetCpuData(data);
		m_meshConstsBuffers[systemIndex].Upload();
	}

	void ParticleMemoryPool::BindMeshConsts(UINT systemIndex)
	{
		if (systemIndex >= m_maxSystems) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConstsBuffers[systemIndex].GetAddressOf());
		context->VSSetConstantBuffers(6, 1, m_meshConstsBuffers[systemIndex].GetAddressOf());
	}
}