#include "pch.h"
#include "ParticleMemoryPool.h"
#include "ParticleSystem.h"
#include "GeometryGenerator.h"
#include "D3D11Utils.h"
#include <chrono>

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

		// Initialize map-based allocators
		m_particleBlockMap.clear();
		m_spawnPosBlockMap.clear();

		// Initialize caches
		m_cachedUsedBlocks = 0;
		m_cachedLastUsedBlock = 0;
		m_cachedTotalUsedBlocks = 0;
		m_fragmentationDirty = true;
		m_visualizationDirty = true;

		// Lazy visualization caches (only allocated when GUI needs them)
		m_particleBlockTableCache.clear();
		m_spawnPosBlockTableCache.clear();
		
		std::queue<UINT> emptyQueue;
		std::swap(m_freeEmitterSlots, emptyQueue);
		std::swap(m_freeSystemSlots, emptyQueue);
		for (UINT i = 0; i < maxEmitters; ++i) m_freeEmitterSlots.push(i);
		for (UINT i = 0; i < maxSystems; ++i) m_freeSystemSlots.push(i);

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
		m_meshConsts.InitializeDynamicSRV(device, maxSystems);

		// Quad Mesh for Billboard Instancing (GS 제거용)
		MeshData quadMesh = GeometryGenerator::MakeSquare(1.0f);
		m_quadVertexCount = static_cast<UINT>(quadMesh.vertices.size());
		m_quadIndexCount = static_cast<UINT>(quadMesh.indices.size());

		ComPtr<ID3D11Device> devicePtr;
		device->QueryInterface(devicePtr.GetAddressOf());
		D3D11Utils::CreateVertexBuffer(devicePtr, quadMesh.vertices, m_quadVertexBuffer);
		D3D11Utils::CreateIndexBuffer(devicePtr, quadMesh.indices, m_quadIndexBuffer);

#ifdef _DEBUG
		// Initialize performance counters
		m_allocateCallCount = 0;
		m_freeCallCount = 0;
		m_totalAllocateTime = 0.0;
		m_totalFreeTime = 0.0;
#endif
	}

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
#ifdef _DEBUG
		auto startTime = std::chrono::high_resolution_clock::now();
#endif

		PoolHandle handle;

		if (m_freeSystemSlots.empty())
			return handle;

		if (reqEmitterCount > m_freeEmitterSlots.size())
			return handle;

		handle.systemSlot = m_freeSystemSlots.front();
		m_freeSystemSlots.pop();

		// 2. Particle Block Allocation - Map-based gap finding O(m log m)
		UINT neededBlocks = (reqParticleCount + m_blockSize - 1) / m_blockSize;
		UINT foundBlock = UINT_MAX;

		// Strategy: Iterate through sorted allocated ranges to find gaps
		UINT searchStart = 0;
		for (const auto& [allocatedStart, allocatedCount] : m_particleBlockMap) {
			// Check gap BEFORE this allocated range
			if (allocatedStart >= searchStart + neededBlocks) {
				foundBlock = searchStart;
				break;
			}

			// Move search start to AFTER this allocated range
			searchStart = allocatedStart + allocatedCount;
		}

		// Check gap AFTER all allocated blocks
		if (foundBlock == UINT_MAX && searchStart + neededBlocks <= m_blockCount) {
			foundBlock = searchStart;
		}

		// 3. Emitter Slot Allocation
		std::vector<UINT> IDs;
		for (UINT i = 0; i < reqEmitterCount; ++i) {
			IDs.push_back(m_freeEmitterSlots.front());
			m_freeEmitterSlots.pop();
		}

		// 4. SpawnPosition Block Allocation (if reqSpawnPosCount > 0)
		UINT foundSpawnPosBlock = UINT_MAX;
		UINT neededSpawnPosBlocks = 0;

		if (reqSpawnPosCount > 0) {
			neededSpawnPosBlocks = (reqSpawnPosCount + m_blockSize - 1) / m_blockSize;

			// Same map-based gap finding for spawn positions
			UINT spawnSearchStart = 0;
			for (const auto& [allocatedStart, allocatedCount] : m_spawnPosBlockMap) {
				// Check gap BEFORE this allocated range
				if (allocatedStart >= spawnSearchStart + neededSpawnPosBlocks) {
					foundSpawnPosBlock = spawnSearchStart;
					break;
				}

				// Move search start to AFTER this allocated range
				spawnSearchStart = allocatedStart + allocatedCount;
			}

			// Check gap AFTER all allocated blocks
			if (foundSpawnPosBlock == UINT_MAX && spawnSearchStart + neededSpawnPosBlocks <= m_blockCount) {
				foundSpawnPosBlock = spawnSearchStart;
			}
		}

		// Allocation success check
		bool particleOk = (foundBlock != UINT_MAX);
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && spawnPosOk) {
			// Mark particle blocks - Single O(log m) insert
			m_particleBlockMap[foundBlock] = neededBlocks;
			m_cachedUsedBlocks += neededBlocks;

			// Mark spawn position blocks
			if (neededSpawnPosBlocks > 0) {
				m_spawnPosBlockMap[foundSpawnPosBlock] = neededSpawnPosBlocks;
			}

			// Invalidate caches
			m_fragmentationDirty = true;
			m_visualizationDirty = true;

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
			// System Slot rollback
			m_freeSystemSlots.push(handle.systemSlot);
			handle.systemSlot = UINT_MAX;

			// Emitter Slot rollback
			for (UINT id : IDs) {
				m_freeEmitterSlots.push(id);
			}
		}

#ifdef _DEBUG
		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
		m_totalAllocateTime += duration.count();
		m_allocateCallCount++;
#endif

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
#ifdef _DEBUG
		auto startTime = std::chrono::high_resolution_clock::now();
#endif

		if (!handle.IsActive()) return;

		// System slot return
		m_freeSystemSlots.push(handle.systemSlot);

		// Particle blocks - Map-based O(log m) erase
		UINT startBlock = handle.particleOffset / m_blockSize;
		auto it = m_particleBlockMap.find(startBlock);
		if (it != m_particleBlockMap.end()) {
			m_cachedUsedBlocks -= it->second;  // Update cache
			m_particleBlockMap.erase(it);       // O(log m) erase
		}

		// Emitter slots return
		for (UINT id : handle.emitterIDs)
			m_freeEmitterSlots.push(id);

		// SpawnPos blocks - Map-based O(log m) erase
		if (handle.spawnPosOffset != UINT_MAX) {
			startBlock = handle.spawnPosOffset / m_blockSize;
			auto spawnIt = m_spawnPosBlockMap.find(startBlock);
			if (spawnIt != m_spawnPosBlockMap.end()) {
				m_spawnPosBlockMap.erase(spawnIt);
			}
		}

		// Invalidate caches
		m_fragmentationDirty = true;
		m_visualizationDirty = true;

#ifdef _DEBUG
		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
		m_totalFreeTime += duration.count();
		m_freeCallCount++;
#endif
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
			m_emitterIDs.GetSRV(),
			m_meshConsts.GetSRV()
		};
		context->CSSetShaderResources(16, 7, srvs);
	}

	void ParticleMemoryPool::UnbindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		ID3D11UnorderedAccessView* uavs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(16, 7, srvs);
	}
	
	void ParticleMemoryPool::BindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = {
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV(),
			m_spawnPositions.GetSRV(),
			m_emitterIDs.GetSRV(),
			m_meshConsts.GetSRV()
		};
		context->CSSetShaderResources(16, 7, srvs);
		context->VSSetShaderResources(16, 7, srvs);
		context->PSSetShaderResources(16, 7, srvs);
	}

	void ParticleMemoryPool::UnbindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(16, 7, srvs);
		context->VSSetShaderResources(16, 7, srvs);
		context->PSSetShaderResources(16, 7, srvs);
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
		
		m_meshConsts.Get(systemIndex) = data;
	}

	void ParticleMemoryPool::UploadMeshConsts()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		m_meshConsts.Upload(context.Get());
	}

	std::vector<UINT> ParticleMemoryPool::Defragment(const std::vector<PoolHandle>& activeHandles)
	{
	    std::vector<UINT> newOffsets;
	    newOffsets.reserve(activeHandles.size());

	    // Clear map - O(m) instead of O(n)
	    m_particleBlockMap.clear();

	    // Rebuild map from activeHandles
	    UINT currentBlock = 0;
	    for (const auto& handle : activeHandles) {
	        newOffsets.push_back(currentBlock * m_blockSize);

	        // Update map - O(log m)
	        m_particleBlockMap[currentBlock] = handle.blockCount;
	        currentBlock += handle.blockCount;
	    }

	    // Update caches
	    m_cachedUsedBlocks = currentBlock;
	    m_fragmentationDirty = true;
	    m_visualizationDirty = true;

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
		// Rebuild cache only when dirty flag is set
		if (m_fragmentationDirty) {
			m_cachedLastUsedBlock = 0;

			// Use pre-cached value instead of recounting
			m_cachedTotalUsedBlocks = m_cachedUsedBlocks;

			// Only need to find last used block
			for (const auto& [blockStart, blockCount] : m_particleBlockMap) {
				UINT blockEnd = blockStart + blockCount;
				if (blockEnd > m_cachedLastUsedBlock) {
					m_cachedLastUsedBlock = blockEnd;
				}
			}

			m_fragmentationDirty = false;
		}

#ifdef _DEBUG
		// Validation assertion in debug builds
		assert(m_cachedTotalUsedBlocks == m_cachedUsedBlocks);
#endif

		if (m_cachedLastUsedBlock == 0 || m_cachedTotalUsedBlocks == 0) return 0.0f;

		// Use cached values
		UINT gapBlocks = m_cachedLastUsedBlock - m_cachedTotalUsedBlocks;
		return static_cast<float>(gapBlocks) / m_cachedLastUsedBlock;
	}

	void ParticleMemoryPool::RebuildVisualizationCache() const
	{
		// Rebuild particle block cache
		m_particleBlockTableCache.assign(m_blockCount, false);
		for (const auto& [start, count] : m_particleBlockMap) {
			for (UINT i = 0; i < count; ++i) {
				if (start + i < m_blockCount) {
					m_particleBlockTableCache[start + i] = true;
				}
			}
		}

		// Rebuild spawn position block cache
		m_spawnPosBlockTableCache.assign(m_blockCount, false);
		for (const auto& [start, count] : m_spawnPosBlockMap) {
			for (UINT i = 0; i < count; ++i) {
				if (start + i < m_blockCount) {
					m_spawnPosBlockTableCache[start + i] = true;
				}
			}
		}
	}
}