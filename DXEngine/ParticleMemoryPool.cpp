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

		// Particle Buffer
		UINT blockCount = (maxParticles + m_blockSize - 1) / m_blockSize;
		m_particleBlockTable.assign(blockCount, false);
		m_emitterSlotTable.assign(maxEmitters, false);

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
	}

	PoolHandle ParticleMemoryPool::Allocate(UINT reqParticleCount, UINT reqEmitterCount)
	{
		PoolHandle handle;

		// ParticleBlock 찾기
		UINT neededBlocks = (reqParticleCount + m_blockSize - 1) / m_blockSize;
		UINT foundBlock = -1;
		UINT consecutive = 0;

		for (size_t i = 0; i < m_particleBlockTable.size(); ++i) {
			if (!m_particleBlockTable[i]) {
				if (++consecutive == neededBlocks) {
					foundBlock = static_cast<UINT>(i - neededBlocks + 1);
					break;
				}
			}
			else
				consecutive = 0;
		}

		// Emitter Slot 찾기
		UINT foundSlot = -1;
		consecutive = 0;
		for (size_t i = 0; i < m_emitterSlotTable.size(); ++i) {
			if (!m_emitterSlotTable[i]) {
				if (++consecutive == reqEmitterCount) {
					foundSlot = static_cast<UINT>(i - reqEmitterCount + 1);
					break;
				}
			}
			else
				consecutive = 0;
		}

		if (foundBlock != -1 && foundSlot != -1) {
			for (size_t i = 0; i < neededBlocks; ++i)
				m_particleBlockTable[foundBlock + i] = true;
			for (size_t i = 0; i < reqEmitterCount; ++i)
				m_emitterSlotTable[foundSlot + i] = true;

			handle.particleOffset = foundBlock * m_blockSize;
			handle.blockCount = neededBlocks;
			handle.emitterID = foundSlot;
			handle.emitterCount = reqEmitterCount;
		}

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
		if (!handle.IsActive()) return;

		size_t startBlock = handle.particleOffset / m_blockSize;
		for (size_t i = 0; i < handle.blockCount; ++i) {
			if (startBlock + i < m_particleBlockTable.size())
				m_particleBlockTable[startBlock + i] = false;
		}

		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterID + i < m_emitterSlotTable.size())
				m_emitterSlotTable[handle.emitterID + i] = false;
		}
	}

	void ParticleMemoryPool::PlanDefragmentation(const std::vector<ParticleSystem*>& activeSystems)
	{
		// 초기화
		//std::fill(m_particleBlockTable.begin(), m_particleBlockTable.end(), false);

		//UINT currentCompactOffset = 0;

		//// Active한 System을 앞부터 차례대로 배치
		//for (auto* system : activeSystems) {
		//	if (!system) continue;

		//	UINT neededParticles = system->GetMaxParticles();
		//	// ParticleSystem에게 다음 Frame에 offset을 새 offset을 사용하라고 지정
		//	system->SetNextOffset(currentCompactOffset);

		//	// 갱신
		//	UINT neededBlocks = (neededParticles + m_blockSize - 1) / m_blockSize;
		//	UINT startBlock = currentCompactOffset / m_blockSize;

		//	for (UINT i = 0; i < neededBlocks; ++i) {
		//		if (startBlock + i < m_particleBlockTable.size())
		//			m_particleBlockTable[startBlock + i] = true;
		//	}

		//	// Cursor 이동 (Block 단위 정렬 유지)
		//	currentCompactOffset += (neededBlocks * m_blockSize);
		//}
	}

	void ParticleMemoryPool::BindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		ID3D11UnorderedAccessView* uavs[] = { 
			GetWriteBuffer().GetUAV(),
			GetWriteCount().GetUAV()
		};
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr); // u0

		ID3D11ShaderResourceView* srvs[] = { 
			GetReadBuffer().GetSRV(),
			GetReadCount().GetSRV(),
			m_frameConsts.GetSRV(),
			m_consts.GetSRV()
		};
		context->CSSetShaderResources(6, 4, srvs); // t5
	}

	void ParticleMemoryPool::UnbindCompute()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		ID3D11UnorderedAccessView* uavs[] = {
			nullptr,
			nullptr
		};
		context->CSSetUnorderedAccessViews(6, 2, uavs, nullptr); // u0

		ID3D11ShaderResourceView* srvs[] = {
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};
		context->CSSetShaderResources(6, 4, srvs); // t5
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

		ID3D11ShaderResourceView* srvs[] = {
			nullptr,
			nullptr,
			nullptr,
			nullptr
		};
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
		// 실제로는 UpdateSubresource나 Map/Unmap으로 해당 오프셋 부분만 업데이트
        // 예시: D3D11_BOX 사용
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

	void ParticleMemoryPool::UploadFrameConsts()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		m_frameConsts.Upload(context);
	}

	void ParticleMemoryPool::UpdateArgs()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		// ArgsUpdateCS
		auto& argsUpdateCS = RenderBase::computeCommon.particle.argsUpdateCS;
		context->CSSetShader(argsUpdateCS.computeShader.Get(), nullptr, 0);

		ID3D11UnorderedAccessView* uavs[] = { m_dispatchArgs.GetUAV(), m_billboardArgsBuffer.GetUAV() };
		context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

		context->Dispatch((m_maxEmitters + 255) / 256, 1, 1);

		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	}
}