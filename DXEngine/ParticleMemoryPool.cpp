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

		// MeshConsts Pool (System��)
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

		// GPU Frustum Culling Buffers
		m_visibleIndices.Initialize(device, maxParticles);
		m_visibleCounts.Initialize(device, maxEmitters);
		m_frustumCullingConsts.Initialize();

		// Initialize visibleIndices to [0, 1, 2, ..., maxParticles-1] for pass-through
		std::vector<uint32_t> initialIndices(maxParticles);
		for (uint32_t i = 0; i < maxParticles; ++i) {
			initialIndices[i] = i;
		}
		m_visibleIndices.SetData(initialIndices);
		m_visibleIndices.Upload(context);

		// Initialize visibleCounts to readCounts initially
		std::vector<uint32_t> initialCounts(maxEmitters, 0);
		m_visibleCounts.SetData(initialCounts);
		m_visibleCounts.Upload(context);
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

		// 1. System Slot �Ҵ�
		handle.systemSlot = AllocateSystemSlot();
		if (handle.systemSlot == UINT_MAX) {
			return handle;
		}

		// 2. Particle Block �Ҵ�
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

		// 3. Emitter Slot �Ҵ�
		std::vector<UINT> IDs;

		for (size_t i = 0; i < m_emitterSlotTable.size(); ++i) {
			if (!m_emitterSlotTable[i]) {
				IDs.push_back(i);
				if (IDs.size() == reqEmitterCount) {
					break;
				}
			}
		}

		// 4. SpawnPosition Block �Ҵ� (reqSpawnPosCount > 0�� ���)
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

		// �Ҵ� ���� ���� Ȯ��
		bool particleOk = (foundBlock != UINT_MAX);
		bool emitterOk = (IDs.size() == reqEmitterCount);
		bool spawnPosOk = (reqSpawnPosCount == 0) || (foundSpawnPosBlock != UINT_MAX);

		if (particleOk && emitterOk && spawnPosOk) {
			// Particle blocks ��ŷ
			for (UINT i = 0; i < neededBlocks; ++i)
				m_particleBlockTable[foundBlock + i] = true;
			
			// Emitter slots ��ŷ
			for (UINT i = 0; i < reqEmitterCount; ++i)
				m_emitterSlotTable[IDs[i]] = true;
			
			// SpawnPos blocks ��ŷ
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
			// �Ҵ� ���� �� System Slot ����
			FreeSystemSlot(handle.systemSlot);
			handle.systemSlot = UINT_MAX;
		}

		return handle;
	}

	void ParticleMemoryPool::Free(const PoolHandle& handle)
	{
		if (!handle.IsActive()) return;

		// System slot ����
		FreeSystemSlot(handle.systemSlot);

		// Particle blocks ����
		size_t startBlock = handle.particleOffset / m_blockSize;
		for (size_t i = 0; i < handle.blockCount; ++i) {
			if (startBlock + i < m_particleBlockTable.size()) {
				m_particleBlockTable[startBlock + i] = false;
			}
		}

		// Emitter slots ����
		for (size_t i = 0; i < handle.emitterCount; ++i) {
			if (handle.emitterIDs[i] < m_emitterSlotTable.size())
				m_emitterSlotTable[handle.emitterIDs[i]] = false;
		}

		// SpawnPos blocks ����
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

		// Bind particle data for rendering
		// t6 = readParticles (unchanged - simulation continues)
		// t7 = visibleCounts (was readCount - now shows visible particles)
		// t8 = frameConsts
		// t9 = consts
		ID3D11ShaderResourceView* srvs[] = {
			GetReadBuffer().GetSRV(),      // t6: readParticles
			m_visibleCounts.GetSRV(),      // t7: visibleCounts (changed from readCount)
			m_frameConsts.GetSRV(),        // t8: frameConsts
			m_consts.GetSRV()              // t9: consts
		};
		context->CSSetShaderResources(6, 4, srvs);
		context->VSSetShaderResources(6, 4, srvs);
		context->PSSetShaderResources(6, 4, srvs);

		// Bind visible indices for vertex shader indirection (t13)
		context->VSSetShaderResources(13, 1, m_visibleIndices.GetAddressOfSRV());
	}

	void ParticleMemoryPool::UnbindRender()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 4, srvs);
		context->VSSetShaderResources(6, 4, srvs);
		context->PSSetShaderResources(6, 4, srvs);

		// Unbind visible indices
		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->VSSetShaderResources(13, 1, &nullSRV);
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
		if (emitterIDs.size() != data.size()) return; // ���� ��ġ

		auto context = GET_SINGLE(RenderBase)->GetContext();

		for (size_t i = 0; i < emitterIDs.size(); ++i)
		{
			D3D11_BOX box;
			// GPU ���� ���� ��ġ (�񿬼����� ���� ID ���)
			box.left = emitterIDs[i] * sizeof(ParticleConsts);
			box.right = static_cast<UINT>(box.left + sizeof(ParticleConsts));
			box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;

			// �߿�: CPU ������ �ҽ� ��ġ�� i��ŭ �̵����Ѿ� ��
			const void* pSrcData = data.data() + i;

			context->UpdateSubresource(m_consts.GetBuffer(), 0, &box, pSrcData, 0, 0);
		}
	}

	// FrameConsts�� �����ϰ� ����
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

	void ParticleMemoryPool::UpdateFrustumData(const Matrix& view, const Matrix& proj)
	{
		FrustumCullingConsts consts;
		consts.viewMatrix = view;
		consts.enableCulling = m_enableParticleFrustumCulling ? 1 : 0;

		// Extract frustum planes from projection matrix
		// Projection matrix has frustum planes embedded in its rows
		// Left: row4 + row1, Right: row4 - row1
		// Bottom: row4 + row2, Top: row4 - row2
		// Near: row3, Far: row4 - row3
		Matrix projT = proj.Transpose();

		// Left plane
		consts.frustum.planes[0] = Vector4(
			projT._14 + projT._11,
			projT._24 + projT._21,
			projT._34 + projT._31,
			projT._44 + projT._41
		);

		// Right plane
		consts.frustum.planes[1] = Vector4(
			projT._14 - projT._11,
			projT._24 - projT._21,
			projT._34 - projT._31,
			projT._44 - projT._41
		);

		// Bottom plane
		consts.frustum.planes[2] = Vector4(
			projT._14 + projT._12,
			projT._24 + projT._22,
			projT._34 + projT._32,
			projT._44 + projT._42
		);

		// Top plane
		consts.frustum.planes[3] = Vector4(
			projT._14 - projT._12,
			projT._24 - projT._22,
			projT._34 - projT._32,
			projT._44 - projT._42
		);

		// Near plane
		consts.frustum.planes[4] = Vector4(
			projT._13,
			projT._23,
			projT._33,
			projT._43
		);

		// Far plane
		consts.frustum.planes[5] = Vector4(
			projT._14 - projT._13,
			projT._24 - projT._23,
			projT._34 - projT._33,
			projT._44 - projT._43
		);

		// Normalize planes
		for (int i = 0; i < 6; ++i)
		{
			float length = sqrtf(
				consts.frustum.planes[i].x * consts.frustum.planes[i].x +
				consts.frustum.planes[i].y * consts.frustum.planes[i].y +
				consts.frustum.planes[i].z * consts.frustum.planes[i].z
			);
			if (length > 0.0001f)
			{
				consts.frustum.planes[i].x /= length;
				consts.frustum.planes[i].y /= length;
				consts.frustum.planes[i].z /= length;
				consts.frustum.planes[i].w /= length;
			}
		}

		m_frustumCullingConsts.SetCpuData(consts);

		auto context = GET_SINGLE(RenderBase)->GetContext();
		m_frustumCullingConsts.Upload();
	}

	void ParticleMemoryPool::PerformParticleFrustumCulling()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		auto& frustumCullingCS = RenderBase::computeCommon.particle.frustumCullingCS;

		// 1. Clear visible counts
		const UINT clearVal[1] = { 0 };
		context->ClearUnorderedAccessViewUint(m_visibleCounts.GetUAV(), clearVal);

		// 2. Bind frustum constants (b7)
		context->CSSetConstantBuffers(7, 1, m_frustumCullingConsts.GetAddressOf());

		// 3. Bind input resources (t6=readParticles, t7=readCount, t11=emitterIDs)
		ID3D11ShaderResourceView* srvs[] = {
			GetReadBuffer().GetSRV(),     // t6: readParticles
			GetReadCount().GetSRV(),      // t7: readCount
			m_frameConsts.GetSRV(),       // t8: frameConsts
			m_consts.GetSRV(),            // t9: consts
			m_spawnPositions.GetSRV(),    // t10: spawnPositions
			m_emitterIDs.GetSRV()         // t11: emitterIDs
		};
		context->CSSetShaderResources(6, 6, srvs);

		// 4. Bind output UAVs (u4=visibleIndices, u5=visibleCount)
		ID3D11UnorderedAccessView* uavs[] = {
			m_visibleIndices.GetUAV(),  // u4
			m_visibleCounts.GetUAV()    // u5
		};
		context->CSSetUnorderedAccessViews(4, 2, uavs, nullptr);

		// 5. Dispatch culling CS (use batch dispatch indirect)
		context->CSSetShader(frustumCullingCS.computeShader.Get(), nullptr, 0);
		context->DispatchIndirect(m_batchDispatchArgs.GetBuffer(), 0);

		// 6. Unbind
		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(4, 2, nullUAVs, nullptr);
		ID3D11Buffer* nullCBs[] = { nullptr };
		context->CSSetConstantBuffers(7, 1, nullCBs);
		context->CSSetShader(nullptr, 0, 0);
	}

	std::vector<UINT> ParticleMemoryPool::Defragment(const std::vector<PoolHandle>& activeHandles)
	{
	    std::vector<UINT> newOffsets;
	    newOffsets.reserve(activeHandles.size());
	    
	    // ���� ���̺� �ʱ�ȭ
	    std::fill(m_particleBlockTable.begin(), m_particleBlockTable.end(), false);
	    
	    UINT currentBlock = 0;
	    for (const auto& handle : activeHandles) {
	        newOffsets.push_back(currentBlock * m_blockSize);
	        
	        // ���� ���̺� ������Ʈ
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
		if (m_particleBlockTable.empty()) return 0.0f;

		// ���������� ��� ���� ���� ã��
		UINT lastUsedBlock = 0;
		UINT totalUsedBlocks = 0;

		for (UINT i = 0; i < static_cast<UINT>(m_particleBlockTable.size()); ++i) {
			if (m_particleBlockTable[i]) {
				lastUsedBlock = i + 1;  // ��� ������ ��
				++totalUsedBlocks;
			}
		}

		if (lastUsedBlock == 0 || totalUsedBlocks == 0) return 0.0f;

		// ����ȭ�� = (��� ���� �� �� ����) / (��� ����)
		UINT gapBlocks = lastUsedBlock - totalUsedBlocks;
		return static_cast<float>(gapBlocks) / lastUsedBlock;
	}
}