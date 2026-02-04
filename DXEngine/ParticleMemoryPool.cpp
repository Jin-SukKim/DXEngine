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

		// EmitterID ConstantBuffer Pool
		m_emitterIDBuffers.resize(maxEmitters);
		for (UINT i = 0; i < maxEmitters; ++i) {
			m_emitterIDBuffers[i].Initialize();
		}

		// MeshConsts Pool (System별)
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

		// 1. System Slot 할당
		handle.systemSlot = AllocateSystemSlot();
		if (handle.systemSlot == UINT_MAX) {
			return handle;
		}

		// 2. Particle Block 할당
		std::vector<uint32_t> particleIndices;

		for (size_t i = 0; i < m_particleBlockTable.size(); ++i) {
			if (!m_particleBlockTable[i]) {
				particleIndices.push_back(i);
				if (particleIndices.size() == reqParticleBlockCount) {
					break;
				}
			}
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

		// 할당 성공 여부 확인
		bool particleOk = !particleIndices.empty();
		bool emitterOk = !IDs.empty();
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && emitterOk && spawnPosOk) {
			// Particle blocks 마킹
			for (UINT i = 0; i < reqParticleBlockCount; ++i)
				m_particleBlockTable[particleIndices[i]] = true;
			
			// Emitter slots 마킹
			for (UINT i = 0; i < reqEmitterCount; ++i)
				m_emitterSlotTable[IDs[i]] = true;
			
			// SpawnPos blocks 마킹
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
		for (size_t i = 0; i < handle.blockCount; ++i) {
			if (handle.particleIndices[i] < m_particleBlockTable.size()) {
				m_particleBlockTable[handle.particleIndices[i]] = false;
			}
		}

		// Emitter slots 해제
		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterIDs[i] < m_emitterSlotTable.size())
				m_emitterSlotTable[handle.emitterIDs[i]] = false;
		}

		// SpawnPos blocks 해제
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
		if (emitterIDs.size() != data.size()) return; // 안전 장치

		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < emitterIDs.size(); ++i)
		{
			D3D11_BOX box;
			// GPU 버퍼 내의 위치 (비연속적인 슬롯 ID 사용)
			box.left = emitterIDs[i] * sizeof(ParticleConsts);
			box.right = static_cast<UINT>(box.left + sizeof(ParticleConsts));
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

			// 중요: CPU 데이터 소스 위치를 i만큼 이동시켜야 함
			const void* pSrcData = data.data() + i;

			context->UpdateSubresource(m_consts.GetBuffer(), 0, &box, pSrcData, 0, 0);
		}
	}

	// FrameConsts도 동일하게 수정
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

			// 중요: 소스 데이터 포인터 오프셋 적용
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

	void ParticleMemoryPool::UploadPageTable(const std::vector<UINT>& pageTableData)
	{
		if (pageTableData.empty()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext().Get();

		// CPU 벡터를 GPU 버퍼로 전송
		m_pageTable.SetData(pageTableData);
		m_pageTable.Upload(context);
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