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

		m_pageTable.Initialize(device, blockCount);
		m_pageTableCPU.resize(m_blockCount, 0);
		m_pageTableUsedSize = 0;
		m_pageTableDirty = false;

		for (UINT i = 0; i < 2; ++i) {
			m_particles[i].Initialize(device, maxParticles);
			m_counts[i].Initialize(device, maxEmitters);

			std::vector<uint32_t> initialCount(maxEmitters, 0);
			m_counts[i].SetData(initialCount);
			m_counts[i].Upload(context);
		}

		m_consts.Initialize(device, maxEmitters);
		m_frameConsts.Initialize(device, maxEmitters);

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

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleBlockCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
		PoolHandle handle;

		if (reqParticleBlockCount >= m_particleBlockTable.size() ||
			reqEmitterCount >= m_emitterSlotTable.size())
			return handle;

		handle.systemSlot = AllocateSystemSlot();
		if (handle.systemSlot == UINT_MAX) {
			return handle;
		}

		std::vector<uint32_t> particleIndices;

		for (size_t i = 0; i < m_particleBlockTable.size(); ++i) {
			if (!m_particleBlockTable[i]) {
				particleIndices.push_back(static_cast<UINT>(i));
				if (particleIndices.size() == reqParticleBlockCount) {
					break;
				}
			}
		}

		std::vector<UINT> IDs;

		for (size_t i = 0; i < m_emitterSlotTable.size(); ++i) {
			if (!m_emitterSlotTable[i]) {
				IDs.push_back(static_cast<UINT>(i));
				if (IDs.size() == reqEmitterCount) {
					break;
				}
			}
		}

		UINT foundSpawnPosBlock = UINT_MAX;
		UINT neededSpawnPosBlocks = 0;
		
		if (reqSpawnPosCount > 0) {
			neededSpawnPosBlocks = (reqSpawnPosCount + m_blockSize - 1) / m_blockSize;
			UINT consecutive = 0;
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

		bool particleOk = (particleIndices.size() == reqParticleBlockCount);
		bool emitterOk = (IDs.size() == reqEmitterCount);
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && emitterOk && spawnPosOk) {
			for (UINT i = 0; i < reqParticleBlockCount; ++i)
				m_particleBlockTable[particleIndices[i]] = true;
			
			for (UINT i = 0; i < reqEmitterCount; ++i)
				m_emitterSlotTable[IDs[i]] = true;
			
			for (UINT i = 0; i < neededSpawnPosBlocks; ++i)
				m_spawnPosBlockTable[foundSpawnPosBlock + i] = true;

			handle.particleIndices = particleIndices;
			handle.blockCount = reqParticleBlockCount;
			handle.emitterIDs = IDs;
			handle.emitterCount = reqEmitterCount;
			
			if (foundSpawnPosBlock != UINT_MAX) {
				handle.spawnPosOffset = foundSpawnPosBlock * m_blockSize;
				handle.spawnPosBlockCount = neededSpawnPosBlocks;
			}
		}
		else {
			FreeSystemSlot(handle.systemSlot);
			handle.systemSlot = UINT_MAX;
		}

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
		if (!handle.IsActive()) return;

		FreeSystemSlot(handle.systemSlot);

		for (size_t i = 0; i < handle.blockCount; ++i) {
			if (handle.particleIndices[i] < m_particleBlockTable.size()) {
				m_particleBlockTable[handle.particleIndices[i]] = false;
			}
		}

		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterIDs[i] < m_emitterSlotTable.size())
				m_emitterSlotTable[handle.emitterIDs[i]] = false;
		}

		size_t startBlock;
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

		ID3D11ShaderResourceView* srvs[] = { 
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV(),
			m_spawnPositions.GetSRV()
		};
		context->CSSetShaderResources(6, 5, srvs);
		context->CSSetShaderResources(16, 1, m_pageTable.GetAddressOfSRV());
	}

	void ParticleMemoryPool::UnbindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		ID3D11UnorderedAccessView* uavs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 5, srvs);
		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->CSSetShaderResources(16, 1, &nullSRV);
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

		context->CSSetShaderResources(16, 1, m_pageTable.GetAddressOfSRV());
		context->VSSetShaderResources(16, 1, m_pageTable.GetAddressOfSRV());
		context->PSSetShaderResources(16, 1, m_pageTable.GetAddressOfSRV());
	}

	void ParticleMemoryPool::UnbindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 4, srvs);
		context->VSSetShaderResources(6, 4, srvs);
		context->PSSetShaderResources(6, 4, srvs);

		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->CSSetShaderResources(16, 1, &nullSRV);
		context->VSSetShaderResources(16, 1, &nullSRV);
		context->PSSetShaderResources(16, 1, &nullSRV);
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

	void ParticleMemoryPool::UpdateEmitterID(UINT slotIndex, const EmitterID& data)
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

	UINT ParticleMemoryPool::AppendToPageTable(const std::vector<UINT>& blockIndices)
	{
		UINT offset = m_pageTableUsedSize;
		
		for (UINT blockIdx : blockIndices) {
			if (m_pageTableUsedSize < m_pageTableCPU.size()) {
				m_pageTableCPU[m_pageTableUsedSize++] = blockIdx;
			}
		}
		
		// 추가된 부분만 GPU에 업로드
		if (!blockIndices.empty()) {
			auto context = GET_SINGLE(RenderBase)->GetContext().Get();
			D3D11_BOX box = { offset * sizeof(UINT), 0, 0, 
				m_pageTableUsedSize * sizeof(UINT), 1, 1 };
			context->UpdateSubresource(m_pageTable.GetBuffer(), 0, &box, 
				&m_pageTableCPU[offset], 0, 0);
		}
		
		return offset;
	}

	void ParticleMemoryPool::RebuildPageTable(const std::vector<ParticleSystem*>& activeSystems)
	{
		m_pageTableUsedSize = 0;
		
		for (auto* system : activeSystems) {
			if (!system) continue;
			
			const auto& handle = system->GetPoolHandle();
			UINT newOffset = m_pageTableUsedSize;
			
			for (UINT blockIdx : handle.particleIndices) {
				m_pageTableCPU[m_pageTableUsedSize++] = blockIdx;
			}
			
			// 오프셋 업데이트
			system->SetPageTableOffset(newOffset);
		}
		
		// 전체 업로드
		if (m_pageTableUsedSize > 0) {
			auto context = GET_SINGLE(RenderBase)->GetContext().Get();
			m_pageTable.SetData(std::vector<UINT>(m_pageTableCPU.begin(), 
				m_pageTableCPU.begin() + m_pageTableUsedSize));
			m_pageTable.Upload(context);
		}
		
		m_pageTableDirty = false;
	}
}