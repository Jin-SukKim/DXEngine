#include "pch.h"
#include "ParticleMemoryPool.h"
#include "ParticleSystem.h"
#include "GeometryGenerator.h"
#include "D3D11Utils.h"

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

		// Initialize particle and spawn block maps (empty at start)
		m_particleBlockMap.clear();
		m_spawnPosBlockMap.clear();

		// Initialize free slot queues
		m_freeEmitterSlots = std::queue<UINT>();
		for (UINT i = 0; i < maxEmitters; ++i) {
			m_freeEmitterSlots.push(i);
		}
		m_usedEmitterCount = 0;

		m_freeSystemSlots = std::queue<UINT>();
		for (UINT i = 0; i < maxSystems; ++i) {
			m_freeSystemSlots.push(i);
		}
		m_usedSystemCount = 0;

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

		std::vector<DispatchArgs> initialBatchVec(1, { 0, 1, 1 });
		m_batchDispatchArgs.Initialize(device, initialBatchVec, 1, sizeof(DispatchArgs), 3);

		// Billboard도 DrawIndexedInstancedArgs로 변경 (쿼드 메쉬 인스턴싱)
		std::vector<DrawIndexedInstancedArgs> initialBillboardArgs(m_maxEmitters, { 6, 0, 0, 0, 0 });
		m_billboardArgsBuffer.Initialize(device, initialBillboardArgs, m_maxEmitters, sizeof(DrawIndexedInstancedArgs), 5);

		std::vector<DrawIndexedInstancedArgs> initialMeshArgs(m_maxEmitters, { 0, 0, 0, 0, 0 });
		m_meshArgsBuffer.Initialize(device, initialMeshArgs, m_maxEmitters, sizeof(DrawIndexedInstancedArgs), 5);

		m_spawnPositions.Initialize(device, maxParticles);

		// EmitterID ConstantBuffer Pool
		m_emitterIDs.InitializeDynamicSRV(device, maxEmitters);
		m_emitterIDBuffer.Initialize();

		// MeshConsts Pool (System)
		m_meshConstsCPU.resize(maxSystems);
		m_meshConstsBuffer.Initialize();

		// Quad Mesh for Billboard Instancing (GS 제거용)
		MeshData quadMesh = GeometryGenerator::MakeSquare(1.0f);
		m_quadVertexCount = static_cast<UINT>(quadMesh.vertices.size());
		m_quadIndexCount = static_cast<UINT>(quadMesh.indices.size());

		ComPtr<ID3D11Device> devicePtr;
		device->QueryInterface(devicePtr.GetAddressOf());
		D3D11Utils::CreateVertexBuffer(devicePtr, quadMesh.vertices, m_quadVertexBuffer);
		D3D11Utils::CreateIndexBuffer(devicePtr, quadMesh.indices, m_quadIndexBuffer);

		// Initialize fragmentation cache
		m_cachedLastUsedBlock = 0;
		m_cachedTotalUsedBlocks = 0;
		m_fragmentationDirty = true;
	}

	UINT ParticleMemoryPool::AllocateSystemSlot()
	{
		if (m_freeSystemSlots.empty()) {
			return UINT_MAX;
		}

		UINT slot = m_freeSystemSlots.front();
		m_freeSystemSlots.pop();
		++m_usedSystemCount;
		return slot;
	}

	void ParticleMemoryPool::FreeSystemSlot(UINT slot)
	{
		if (slot < m_maxSystems) {
			m_freeSystemSlots.push(slot);
			if (m_usedSystemCount > 0) {
				--m_usedSystemCount;
			}
		}
	}

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
		PoolHandle handle;

		if (reqEmitterCount >= m_emitterSlotTable.size())
			return handle;

		// 1. System Slot Ҵ
		handle.systemSlot = AllocateSystemSlot();
		if (handle.systemSlot == UINT_MAX) {
			return handle;
		}

		// 2. Particle Block Ҵ
		UINT neededBlocks = (reqParticleCount + m_blockSize - 1) / m_blockSize;
		UINT foundBlock = UINT_MAX;

		// Find consecutive free blocks
		UINT searchStart = 0;
		for (const auto& pair : m_particleBlockMap) {
			UINT allocatedStart = pair.first;
			UINT allocatedCount = pair.second;

			// Check gap before this allocated block
			if (allocatedStart >= searchStart + neededBlocks) {
				foundBlock = searchStart;
				break;
			}

			// Move search start to after this allocated block
			searchStart = allocatedStart + allocatedCount;
		}

		// Check gap after all allocated blocks
		if (foundBlock == UINT_MAX && searchStart + neededBlocks <= m_blockCount) {
			foundBlock = searchStart;
		}

		// 3. Emitter Slot Ҵ
		std::vector<UINT> IDs;
		std::queue<UINT> tempQueue = m_freeEmitterSlots;

		for (UINT i = 0; i < reqEmitterCount && !tempQueue.empty(); ++i) {
			IDs.push_back(tempQueue.front());
			tempQueue.pop();
		}

		// 4. SpawnPosition Block Ҵ (reqSpawnPosCount > 0 )
		UINT foundSpawnPosBlock = UINT_MAX;
		UINT neededSpawnPosBlocks = 0;

		if (reqSpawnPosCount > 0) {
			neededSpawnPosBlocks = (reqSpawnPosCount + m_blockSize - 1) / m_blockSize;

			// Find consecutive free blocks
			UINT searchStart = 0;
			for (const auto& pair : m_spawnPosBlockMap) {
				UINT allocatedStart = pair.first;
				UINT allocatedCount = pair.second;

				// Check gap before this allocated block
				if (allocatedStart >= searchStart + neededSpawnPosBlocks) {
					foundSpawnPosBlock = searchStart;
					break;
				}

				// Move search start to after this allocated block
				searchStart = allocatedStart + allocatedCount;
			}

			// Check gap after all allocated blocks
			if (foundSpawnPosBlock == UINT_MAX && searchStart + neededSpawnPosBlocks <= m_blockCount) {
				foundSpawnPosBlock = searchStart;
			}
		}

		// Ҵ   Ȯ
		bool particleOk = (foundBlock != UINT_MAX);
		bool emitterOk = (IDs.size() == reqEmitterCount);
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && emitterOk && spawnPosOk) {
			// Particle blocks ŷ
			m_particleBlockMap[foundBlock] = neededBlocks;

			m_fragmentationDirty = true;  // Invalidate cache

			// Emitter slots ŷ - remove from queue
			for (UINT i = 0; i < reqEmitterCount; ++i) {
				m_freeEmitterSlots.pop();
			}
			m_usedEmitterCount += reqEmitterCount;

			// SpawnPos blocks ŷ
			if (neededSpawnPosBlocks > 0) {
				m_spawnPosBlockMap[foundSpawnPosBlock] = neededSpawnPosBlocks;
			}

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
			// Ҵ   System Slot 
			FreeSystemSlot(handle.systemSlot);
			handle.systemSlot = UINT_MAX;
		}

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
		if (!handle.IsActive()) return;

		// System slot
		FreeSystemSlot(handle.systemSlot);

		// Particle blocks
		UINT startBlock = handle.particleOffset / m_blockSize;
		auto it = m_particleBlockMap.find(startBlock);
		if (it != m_particleBlockMap.end()) {
			m_particleBlockMap.erase(it);
		}

		m_fragmentationDirty = true;  // Invalidate cache

		// Emitter slots - return to queue
		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterIDs[i] < m_maxEmitters) {
				m_freeEmitterSlots.push(handle.emitterIDs[i]);
			}
		}
		if (m_usedEmitterCount >= handle.emitterCount) {
			m_usedEmitterCount -= handle.emitterCount;
		}

		// SpawnPos blocks
		if (handle.spawnPosOffset != UINT_MAX) {
			startBlock = handle.spawnPosOffset / m_blockSize;
			auto it = m_spawnPosBlockMap.find(startBlock);
			if (it != m_spawnPosBlockMap.end()) {
				m_spawnPosBlockMap.erase(it);
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

		ID3D11ShaderResourceView* srvs[] = { 
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV(),
			m_spawnPositions.GetSRV(),
			m_emitterIDs.GetSRV()
		};
		context->CSSetShaderResources(6, 6, srvs);
	}

	void ParticleMemoryPool::UnbindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		ID3D11UnorderedAccessView* uavs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 6, srvs);
	}
	
	void ParticleMemoryPool::BindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = {
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV()
		};
		context->CSSetShaderResources(6, 4, srvs);
		context->VSSetShaderResources(6, 4, srvs);
		context->PSSetShaderResources(6, 4, srvs);
	}

	void ParticleMemoryPool::UnbindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 4, srvs);
		context->VSSetShaderResources(6, 4, srvs);
		context->PSSetShaderResources(6, 4, srvs);
	}

	void ParticleMemoryPool::ClearWriteCount()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		const UINT clearVal[1] = { 0 };
		context->ClearUnorderedAccessViewUint(GetWriteCount().GetUAV(), clearVal);
		context->CSSetShaderResources(8, 1, m_consts.GetAddressOfSRV());
	}

	void ParticleMemoryPool::ExcuteParticleLogic()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		auto& particleCS = RenderBase::computeCommon.particle.particleCS;
		context->CSSetShader(particleCS.computeShader.Get(), 0, 0);
		context->DispatchIndirect(m_batchDispatchArgs.GetBuffer(), 0);
		context->CSSetShader(nullptr, 0, 0);
	}

	void ParticleMemoryPool::UploadConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleConsts>& data)
	{
		if (emitterIDs.size() != data.size()) return; //  ġ

		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < emitterIDs.size(); ++i)
		{
			D3D11_BOX box;
			// GPU   ġ (񿬼  ID )
			box.left = emitterIDs[i] * sizeof(ParticleConsts);
			box.right = static_cast<UINT>(box.left + sizeof(ParticleConsts));
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

			// ߿: CPU  ҽ ġ iŭ ̵Ѿ 
			const void* pSrcData = data.data() + i;

			context->UpdateSubresource(m_consts.GetBuffer(), 0, &box, pSrcData, 0, 0);
		}
	}

	// FrameConsts ϰ 
	void ParticleMemoryPool::UpdateFrameConsts(const std::vector<UINT>& emitterIDs, const std::vector<ParticleFrameConsts>& data)
	{
		if (emitterIDs.size() != data.size()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < emitterIDs.size(); ++i)
			m_frameConsts.Get(emitterIDs[i]) = data[i];
	}

	void ParticleMemoryPool::UploadFrameConsts()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext().Get();
		m_frameConsts.Upload(context);
	}

	void ParticleMemoryPool::UpdateRenderConst(UINT emitterID, float spawnRatio)
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		m_frameConsts.Get(emitterID).spawnRatio = spawnRatio;
	}

	void ParticleMemoryPool::UpdateArgs()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		auto& argsUpdateCS = RenderBase::computeCommon.particle.argsUpdateCS;
		context->CSSetShader(argsUpdateCS.computeShader.Get(), nullptr, 0);

		// 배치 args 초기화 (InterlockedMax 전에 0으로)
		const UINT clearVal[4] = { 0, 0, 0, 0 };
		context->ClearUnorderedAccessViewUint(m_batchDispatchArgs.GetUAV(), clearVal);

		ID3D11UnorderedAccessView* uavs[] = {
			m_dispatchArgs.GetUAV(),        // u0
			m_batchDispatchArgs.GetUAV()    // u1
		};
		context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

		context->Dispatch((m_maxEmitters + 255) / 256, 1, 1);

		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	}

	void ParticleMemoryPool::UpdateRenderArgs()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		auto& argsUpdateCS = RenderBase::computeCommon.particle.renderArgsUpdateCS;
		context->CSSetShader(argsUpdateCS.computeShader.Get(), nullptr, 0);

		ID3D11UnorderedAccessView* uavs[] = {
			m_billboardArgsBuffer.GetUAV(),
			m_meshArgsBuffer.GetUAV()
		};
		context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

		context->Dispatch((m_maxEmitters + 255) / 256, 1, 1);

		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
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

	void ParticleMemoryPool::UpdateEmitterID(UINT slotIndex, const EmitterID& data)
	{
		if (slotIndex >= m_maxEmitters) return;
		
		m_emitterIDs.Get(slotIndex) = data;
	}

	void ParticleMemoryPool::BindEmitterID(UINT slotIndex)
	{
		if (slotIndex >= m_maxEmitters) return;
		
		auto context = GET_SINGLE(RenderBase)->GetContext();

		m_emitterIDBuffer.SetCpuData(m_emitterIDs.Get(slotIndex));
		m_emitterIDBuffer.Upload();

		context->CSSetConstantBuffers(5, 1, m_emitterIDBuffer.GetAddressOf());
		context->VSSetConstantBuffers(5, 1, m_emitterIDBuffer.GetAddressOf());
		context->PSSetConstantBuffers(5, 1, m_emitterIDBuffer.GetAddressOf());
	}

	void ParticleMemoryPool::UploadEmitterIDs()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		m_emitterIDs.Upload(context.Get());
	}

	void ParticleMemoryPool::UpdateMeshConsts(UINT systemIndex, const ParticleMeshConsts& data)
	{
		if (systemIndex >= m_maxSystems) return;
		
		m_meshConstsCPU[systemIndex] = data;
	}

	void ParticleMemoryPool::BindMeshConsts(UINT systemIndex)
	{
		if (systemIndex >= m_maxSystems) return;
		
		m_meshConstsBuffer.SetCpuData(m_meshConstsCPU[systemIndex]);

		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConstsBuffer.GetAddressOf());
		context->VSSetConstantBuffers(6, 1, m_meshConstsBuffer.GetAddressOf());
	}

	std::vector<UINT> ParticleMemoryPool::Defragment(const std::vector<PoolHandle>& activeHandles)
	{
	    std::vector<UINT> newOffsets;
	    newOffsets.reserve(activeHandles.size());

	    // Clear map
	    m_particleBlockMap.clear();

	    UINT currentBlock = 0;
	    for (const auto& handle : activeHandles) {
	        newOffsets.push_back(currentBlock * m_blockSize);

	        // Update map
	        m_particleBlockMap[currentBlock] = handle.blockCount;
	        currentBlock += handle.blockCount;
	    }

	    m_fragmentationDirty = true;  // Invalidate cache

	    return newOffsets;
	}

	void ParticleMemoryPool::UpdateWriteOffset(UINT slotIndex, UINT newWriteOffset)
	{
	    if (slotIndex >= m_maxEmitters) return;
	    
		m_emitterIDs.Get(slotIndex).writeParticleOffset = newWriteOffset;
	}

	void ParticleMemoryPool::SyncReadOffset(UINT slotIndex)
	{
	    if (slotIndex >= m_maxEmitters) return;
	    
	    EmitterID& eID = m_emitterIDs.Get(slotIndex);
		eID.readParticleOffset = eID.writeParticleOffset;
	}

	float ParticleMemoryPool::GetFragmentationRatio() const
	{
		if (m_particleBlockMap.empty()) return 0.0f;

		// Rebuild cache only when dirty flag is set
		if (m_fragmentationDirty) {
			m_cachedLastUsedBlock = 0;
			m_cachedTotalUsedBlocks = 0;

			// Calculate from map
			for (const auto& pair : m_particleBlockMap) {
				UINT blockEnd = pair.first + pair.second;
				if (blockEnd > m_cachedLastUsedBlock) {
					m_cachedLastUsedBlock = blockEnd;
				}
				m_cachedTotalUsedBlocks += pair.second;
			}
			m_fragmentationDirty = false;
		}

		if (m_cachedLastUsedBlock == 0 || m_cachedTotalUsedBlocks == 0) return 0.0f;

		// Use cached values
		UINT gapBlocks = m_cachedLastUsedBlock - m_cachedTotalUsedBlocks;
		return static_cast<float>(gapBlocks) / m_cachedLastUsedBlock;
	}
}