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

		UINT blockCount = (maxParticles + m_blockSize - 1) / m_blockSize;
		m_blockCount = blockCount;
		m_particleBlockTable.assign(blockCount, false);
		m_emitterSlotTable.assign(maxEmitters, false);
		m_spawnPosBlockTable.assign(blockCount, false);
		m_systemSlotTable.assign(maxSystems, false);

		for (UINT i = 0; i < 2; ++i) {
			m_particles[i].Initialize(device, maxParticles);
			m_counts[i].Initialize(device, maxEmitters);

			std::vector<uint32_t> initialCount(maxEmitters, 0);
			m_counts[i].SetData(initialCount);
			m_counts[i].Upload(context);
		}

		m_consts.Initialize(device, maxEmitters);
		m_frameConsts.InitializeDynamicSRV(device, maxEmitters);

		std::vector<DispatchArgs> initialDispatch(m_maxEmitters, { 0, 1, 1 });
		m_dispatchArgs.Initialize(device, initialDispatch, m_maxEmitters, sizeof(DispatchArgs), 3);

		std::vector<DrawInstancedArgs> initialBillboardArgs(m_maxEmitters, { 0, 1, 0, 0 });
		m_billboardArgsBuffer.Initialize(device, initialBillboardArgs, m_maxEmitters, sizeof(DrawInstancedArgs), 4);

		std::vector<DrawIndexedInstancedArgs> initialMeshArgs(m_maxEmitters, { 0, 0, 0, 0, 0 });
		m_meshArgsBuffer.Initialize(device, initialMeshArgs, m_maxEmitters, sizeof(DrawIndexedInstancedArgs), 5);

		m_spawnPositions.Initialize(device, maxParticles);

		// ★ EmitterID와 MeshConsts를 Dynamic StructuredBuffer로 초기화
		m_emitterIDs.InitializeDynamicSRV(device, maxEmitters);
		m_meshConsts.InitializeDynamicSRV(device, maxSystems);
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

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
		PoolHandle handle;

		if (reqEmitterCount >= m_emitterSlotTable.size())
			return handle;

		// 1. System Slot 할당
		handle.systemSlot = AllocateSystemSlot();
		if (handle.systemSlot == UINT_MAX) {
			return handle;
		}

		// 2. Particle Block 할당
		UINT neededBlocks = (reqParticleCount + m_blockSize - 1) / m_blockSize;
		UINT foundBlock = UINT_MAX;
		UINT consecutive = 0;

		for (size_t i = 0; i < m_particleBlockTable.size(); ++i) {
			if (!m_particleBlockTable[i]) {
				if (consecutive == 0) {
					foundBlock = static_cast<UINT>(i);
				}
				if (++consecutive == neededBlocks) {
					break;
				}
			}
			else {
				consecutive = 0;
				foundBlock = UINT_MAX;
			}
		}

		if (consecutive < neededBlocks) {
			foundBlock = UINT_MAX;
		}

		// 3. Emitter Slot 할당
		std::vector<UINT> IDs;

		for (size_t i = 0; i < m_emitterSlotTable.size(); ++i) {
			if (!m_emitterSlotTable[i]) {
				IDs.push_back(i);
				if (IDs.size() == reqEmitterCount) {
					break;
				}
			}
		}

		// 4. SpawnPosition Block 할당 (reqSpawnPosCount > 0인 경우)
		UINT foundSpawnPosBlock = UINT_MAX;
		UINT neededSpawnPosBlocks = 0;
		
		if (reqSpawnPosCount > 0) {
			neededSpawnPosBlocks = (reqSpawnPosCount + m_blockSize - 1) / m_blockSize;
			consecutive = 0;
			UINT spawnStart = UINT_MAX;

			for (size_t i = 0; i < m_spawnPosBlockTable.size(); ++i) {
				if (!m_spawnPosBlockTable[i]) {
					if (consecutive == 0) {
						spawnStart = static_cast<UINT>(i);
					}
					if (++consecutive == neededSpawnPosBlocks) {
						foundSpawnPosBlock = spawnStart;
						break;
					}
				}
				else {
					consecutive = 0;
					spawnStart = UINT_MAX;
				}
			}
		}

		// 할당 성공 여부 확인
		bool particleOk = (foundBlock != UINT_MAX);
		bool emitterOk = (IDs.size() == reqEmitterCount);
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && emitterOk && spawnPosOk) {
			// Particle blocks 마킹
			for (UINT i = 0; i < neededBlocks; ++i)
				m_particleBlockTable[foundBlock + i] = true;
			
			// Emitter slots 마킹
			for (UINT i = 0; i < reqEmitterCount; ++i)
				m_emitterSlotTable[IDs[i]] = true;
			
			// SpawnPos blocks 마킹
			for (UINT i = 0; i < neededSpawnPosBlocks; ++i)
				m_spawnPosBlockTable[foundSpawnPosBlock + i] = true;

			handle.particleOffset = foundBlock * m_blockSize;
			handle.blockCount = neededBlocks;
			handle.emitterIDs = IDs;
			handle.emitterCount = reqEmitterCount;
			
			if (foundSpawnPosBlock != UINT_MAX) {
				handle.spawnPosOffset = foundSpawnPosBlock * m_blockSize;
				handle.spawnPosBlockCount = neededSpawnPosBlocks;
			}
		}
		else {
			// 할당 실패 시 System Slot 해제
			FreeSystemSlot(handle.systemSlot);
			handle.systemSlot = UINT_MAX;
		}

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
		if (!handle.IsActive()) return;

		// System slot 해제
		FreeSystemSlot(handle.systemSlot);

		// Particle blocks 해제
		size_t startBlock = handle.particleOffset / m_blockSize;
		for (size_t i = 0; i < handle.blockCount; ++i) {
			if (startBlock + i < m_particleBlockTable.size()) {
				m_particleBlockTable[startBlock + i] = false;
			}
		}

		// Emitter slots 해제
		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterIDs[i] < m_emitterSlotTable.size())
				m_emitterSlotTable[handle.emitterIDs[i]] = false;
		}

		// SpawnPos blocks 해제
		if (handle.spawnPosOffset != UINT_MAX) {
			startBlock = handle.spawnPosOffset / m_blockSize;
			for (size_t i = 0; i < handle.spawnPosBlockCount; ++i) {
				if (startBlock + i < m_spawnPosBlockTable.size())
					m_spawnPosBlockTable[startBlock + i] = false;
			}
		}
	}

	void ParticleMemoryPool::BindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		ID3D11UnorderedAccessView* uavs[] = { 
			GetWriteBuffer().GetUAV(),
			GetWriteCount().GetUAV()
		};
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		// ★ EmitterIDs와 MeshConsts를 SRV로 바인딩
		ID3D11ShaderResourceView* srvs[] = { 
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV(),
			m_spawnPositions.GetSRV(),
			m_emitterIDs.GetSRV(),
			m_meshConsts.GetSRV()
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

		// ★ EmitterIDs와 MeshConsts 포함
		ID3D11ShaderResourceView* srvs[] = {
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV(),
			m_emitterIDs.GetSRV(),
			m_meshConsts.GetSRV()
		};
		context->CSSetShaderResources(6, 6, srvs);
		context->VSSetShaderResources(6, 6, srvs);
		context->PSSetShaderResources(6, 6, srvs);
	}

	void ParticleMemoryPool::UnbindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 6, srvs);
		context->VSSetShaderResources(6, 6, srvs);
		context->PSSetShaderResources(6, 6, srvs);
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

		for (size_t i = 0; i < emitterIDs.size(); ++i)
		{
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

		for (size_t i = 0; i < emitterIDs.size(); ++i)
		{
			D3D11_BOX box;
			box.left = emitterIDs[i] * sizeof(ParticleFrameConsts);
			box.right = static_cast<UINT>(box.left + sizeof(ParticleFrameConsts));
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

			const void* pSrcData = data.data() + i;
			context->UpdateSubresource(m_frameConsts.GetBuffer(), 0, &box, pSrcData, 0, 0);
		}
	}

	void ParticleMemoryPool::UploadFrameConsts()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext().Get();
		m_frameConsts.Upload(context);
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

	void ParticleMemoryPool::UploadSpawnPositions(UINT offset, const std::vector<Vector3>& positions)
	{
		if (positions.empty()) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext();
		D3D11_BOX box;
		box.left = offset * sizeof(Vector3);
		box.right = static_cast<UINT>((offset + positions.size()) * sizeof(Vector3));
		box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;
		context->UpdateSubresource(m_spawnPositions.GetBuffer(), 0, &box, positions.data(), 0, 0);
	}

	// ★ EmitterID 업데이트 (CPU 버퍼에 저장, dirty 플래그 설정)
	void ParticleMemoryPool::UpdateEmitterID(UINT slotIndex, const EmitterID& data)
	{
		if (slotIndex >= m_maxEmitters) return;
		
		m_emitterIDs.GetCpu()[slotIndex] = data;
		m_emitterIDsDirty = true;
	}

	// ★ 모든 EmitterID를 한번에 GPU로 업로드
	void ParticleMemoryPool::UploadEmitterIDs()
	{
		if (!m_emitterIDsDirty) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext().Get();
		m_emitterIDs.Upload(context);
		m_emitterIDsDirty = false;
	}

	// ★ MeshConsts 업데이트 (CPU 버퍼에 저장, dirty 플래그 설정)
	void ParticleMemoryPool::UploadMeshConsts(UINT systemIndex, const ParticleMeshConsts& data)
	{
		if (systemIndex >= m_maxSystems) return;
		
		m_meshConsts.GetCpu()[systemIndex] = data;
		m_meshConstsDirty = true;
	}

	// ★ 모든 MeshConsts를 한번에 GPU로 업로드
	void ParticleMemoryPool::UploadAllMeshConsts()
	{
		if (!m_meshConstsDirty) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext().Get();
		m_meshConsts.Upload(context);
		m_meshConstsDirty = false;
	}

	std::vector<UINT> ParticleMemoryPool::Defragment(const std::vector<PoolHandle>& activeHandles)
	{
	    std::vector<UINT> newOffsets;
	    newOffsets.reserve(activeHandles.size());
	    
	    std::fill(m_particleBlockTable.begin(), m_particleBlockTable.end(), false);
	    
	    UINT currentBlock = 0;
	    for (const auto& handle : activeHandles) {
	        newOffsets.push_back(currentBlock * m_blockSize);
	        
	        for (UINT i = 0; i < handle.blockCount; ++i) {
	            m_particleBlockTable[currentBlock + i] = true;
	        }
	        currentBlock += handle.blockCount;
	    }
	    
	    return newOffsets;
	}

	void ParticleMemoryPool::UpdateWriteOffset(UINT slotIndex, UINT newWriteOffset)
	{
	    if (slotIndex >= m_maxEmitters) return;
	    
	    m_emitterIDs.GetCpu()[slotIndex].writeParticleOffset = newWriteOffset;
	    m_emitterIDsDirty = true;
	}

	void ParticleMemoryPool::SyncReadOffset(UINT slotIndex)
	{
	    if (slotIndex >= m_maxEmitters) return;
	    
	    auto& eID = m_emitterIDs.GetCpu()[slotIndex];
	    eID.readParticleOffset = eID.writeParticleOffset;
	    m_emitterIDsDirty = true;
	}

	float ParticleMemoryPool::GetFragmentationRatio() const
	{
		if (m_particleBlockTable.empty()) return 0.0f;

		UINT lastUsedBlock = 0;
		UINT totalUsedBlocks = 0;

		for (UINT i = 0; i < static_cast<UINT>(m_particleBlockTable.size()); ++i) {
			if (m_particleBlockTable[i]) {
				lastUsedBlock = i + 1;
				++totalUsedBlocks;
			}
		}

		if (lastUsedBlock == 0 || totalUsedBlocks == 0) return 0.0f;

		UINT gapBlocks = lastUsedBlock - totalUsedBlocks;
		return static_cast<float>(gapBlocks) / lastUsedBlock;
	}
}