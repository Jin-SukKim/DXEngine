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

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount, UINT reqSpawnPosCount)
	{
		PoolHandle handle;

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
		// 1. 모든 관리 테이블 초기화 (싹 비우기)
		std::fill(m_particleBlockTable.begin(), m_particleBlockTable.end(), false);
		std::fill(m_emitterSlotTable.begin(), m_emitterSlotTable.end(), false);
		std::fill(m_spawnPosBlockTable.begin(), m_spawnPosBlockTable.end(), false);
		std::fill(m_systemSlotTable.begin(), m_systemSlotTable.end(), false);

		// 2. 커서(Cursor) 초기화 - 모두 0번지부터 다시 시작
		UINT particleCursor = 0; // 블록 단위
		UINT emitterCursor = 0;  // 개수 단위
		UINT spawnPosCursor = 0; // 블록 단위
		UINT systemCursor = 0;   // 개수 단위

		// 3. 활성 시스템 순회 및 재배치 (Compaction)
		for (auto* system : activeSystems)
		{
			if (!system) continue;

			// 현재 시스템이 가지고 있는 핸들(정보) 가져오기
			PoolHandle oldHandle = system->GetPoolHandle();
			if (!oldHandle.IsActive()) continue;

			PoolHandle newHandle;

			// --- A. Particle Offset 재할당 (Block 단위) ---
			newHandle.blockCount = oldHandle.blockCount;
			newHandle.particleOffset = particleCursor * m_blockSize;

			// 테이블 마킹
			for (UINT i = 0; i < newHandle.blockCount; ++i) {
				if (particleCursor + i < m_particleBlockTable.size())
					m_particleBlockTable[particleCursor + i] = true;
			}
			particleCursor += newHandle.blockCount;

			// --- B. Emitter ID 재할당 (Slot 단위) ---
			newHandle.emitterCount = oldHandle.emitterCount;
			newHandle.emitterID = emitterCursor;

			for (UINT i = 0; i < newHandle.emitterCount; ++i) {
				if (emitterCursor + i < m_emitterSlotTable.size())
					m_emitterSlotTable[emitterCursor + i] = true;
			}
			emitterCursor += newHandle.emitterCount;

			// --- C. SpawnPos Offset 재할당 (Block 단위) ---
			if (oldHandle.spawnPosOffset != UINT_MAX)
			{
				newHandle.spawnPosBlockCount = oldHandle.spawnPosBlockCount;
				newHandle.spawnPosOffset = spawnPosCursor * m_blockSize;

				for (UINT i = 0; i < newHandle.spawnPosBlockCount; ++i) {
					if (spawnPosCursor + i < m_spawnPosBlockTable.size())
						m_spawnPosBlockTable[spawnPosCursor + i] = true;
				}
				spawnPosCursor += newHandle.spawnPosBlockCount;
			}
			else
			{
				newHandle.spawnPosOffset = UINT_MAX;
				newHandle.spawnPosBlockCount = 0;
			}

			// --- D. System Slot 재할당 (Slot 단위) ---
			newHandle.systemSlot = systemCursor;
			if (systemCursor < m_systemSlotTable.size())
				m_systemSlotTable[systemCursor] = true;
			systemCursor++;

			// 4. 시스템에게 "다음 프레임부터 이 핸들을 써라"고 통보
			// (이 함수는 다음 프레임 Update 전까지 m_nextHandle에 저장해둠)
			system->SetNextPoolHandle(newHandle);
		}

		m_startDefrag = true;
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

	void ParticleMemoryPool::UpdateEmitterID(UINT slotIndex, const PoolHandle& next, const UINT& emitterID)
	{
		if (slotIndex >= m_maxEmitters) return;

		m_emitterIDBuffers[slotIndex].GetCpu().writeEmitterID = next.emitterID + emitterID;
		m_emitterIDBuffers[slotIndex].GetCpu().writeParticleOffset = next.particleOffset;

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