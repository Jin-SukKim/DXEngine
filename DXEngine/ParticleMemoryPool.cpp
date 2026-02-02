#include "pch.h"
#include "ParticleMemoryPool.h"
#include "ParticleSystem.h"

namespace DE {
	void ParticleMemoryPool::Initialize(UINT maxParticles, UINT maxEmitters)
	{
		ID3D11Device* device = GET_SINGLE(RenderBase)->GetDevice().Get();
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		m_maxParticles = maxParticles;
		m_maxEmitters = maxEmitters;

		UINT blockCount = (maxParticles + m_blockSize - 1) / m_blockSize;
		m_particleBlockTable.assign(blockCount, false);
		m_emitterSlotTable.assign(maxEmitters, false);
		m_spawnPosBlockTable.assign(blockCount, false);

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

		// EmitterID ConstantBuffer Pool 미리 생성
		m_emitterIDBuffers.resize(maxEmitters);
		for (UINT i = 0; i < maxEmitters; ++i) {
			m_emitterIDBuffers[i].Initialize();
		}
	}

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
		PoolHandle handle;

		// 1. Particle Block 할당
		UINT neededBlocks = (reqParticleCount + m_blockSize - 1) / m_blockSize;
		UINT foundBlock = UINT_MAX;
		UINT consecutive = 0;

		for (size_t i = 0; i < m_particleBlockTable.size(); ++i) {
			if (!m_particleBlockTable[i]) {
				if (consecutive == 0) {
					foundBlock = static_cast<UINT>(i); // 시작 위치 저장
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
		
		// consecutive가 neededBlocks보다 작으면 실패
		if (consecutive < neededBlocks) {
			foundBlock = UINT_MAX;
		}

		// 2. Emitter Slot 할당
		UINT foundSlot = UINT_MAX;
		consecutive = 0;
		UINT slotStart = UINT_MAX;
		
		for (size_t i = 0; i < m_emitterSlotTable.size(); ++i) {
			if (!m_emitterSlotTable[i]) {
				if (consecutive == 0) {
					slotStart = static_cast<UINT>(i);
				}
				if (++consecutive == reqEmitterCount) {
					foundSlot = slotStart;
					break;
				}
			}
			else {
				consecutive = 0;
				slotStart = UINT_MAX;
			}
		}

		// 3. SpawnPosition Block 할당 (reqSpawnPosCount > 0인 경우)
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
		bool emitterOk = (foundSlot != UINT_MAX);
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && emitterOk && spawnPosOk) {
			// Particle blocks 마킹
			for (UINT i = 0; i < neededBlocks; ++i)
				m_particleBlockTable[foundBlock + i] = true;
			
			// Emitter slots 마킹
			for (UINT i = 0; i < reqEmitterCount; ++i)
				m_emitterSlotTable[foundSlot + i] = true;
			
			// SpawnPos blocks 마킹
			for (UINT i = 0; i < neededSpawnPosBlocks; ++i)
				m_spawnPosBlockTable[foundSpawnPosBlock + i] = true;

			handle.particleOffset = foundBlock * m_blockSize;
			handle.blockCount = neededBlocks;
			handle.emitterID = foundSlot;
			handle.emitterCount = reqEmitterCount;
			
			if (foundSpawnPosBlock != UINT_MAX) {
				handle.spawnPosOffset = foundSpawnPosBlock * m_blockSize;
				handle.spawnPosBlockCount = neededSpawnPosBlocks;
			}
		}
		// 실패 시 handle은 기본값(UINT_MAX)으로 반환되어 IsActive()가 false

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
		if (!handle.IsActive()) return;

		// Particle blocks 해제
		size_t startBlock = handle.particleOffset / m_blockSize;
		for (size_t i = 0; i < handle.blockCount; ++i) {
			if (startBlock + i < m_particleBlockTable.size()) {
				assert(m_particleBlockTable[startBlock + i] && "Double-free detected!");
				m_particleBlockTable[startBlock + i] = false;
			}
		}

		// Emitter slots 해제
		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterID + i < m_emitterSlotTable.size())
				m_emitterSlotTable[handle.emitterID + i] = false;
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

	void ParticleMemoryPool::PlanDefragmentation(const std::vector<ParticleSystem*>& activeSystems)
	{
		// TODO: Defragmentation 구현
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
	}

	void ParticleMemoryPool::UnbindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		ID3D11UnorderedAccessView* uavs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr);

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 5, srvs);
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

	void ParticleMemoryPool::UploadConsts(UINT offset, const std::vector<ParticleConsts>& data)
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		D3D11_BOX box;
		box.left = offset * sizeof(ParticleConsts);
		box.right = static_cast<UINT>((offset + data.size()) * sizeof(ParticleConsts));
		box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

		context->UpdateSubresource(m_consts.GetBuffer(), 0, &box, data.data(), 0, 0);
	}

	void ParticleMemoryPool::UploadFrameConsts(UINT offset, const std::vector<ParticleFrameConsts>& data)
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		D3D11_BOX box;
		box.left = offset * sizeof(ParticleFrameConsts);
		box.right = static_cast<UINT>((offset + data.size()) * sizeof(ParticleFrameConsts));
		box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;
		context->UpdateSubresource(m_frameConsts.GetBuffer(), 0, &box, data.data(), 0, 0);
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
}