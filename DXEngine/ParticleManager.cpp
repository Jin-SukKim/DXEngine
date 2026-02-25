#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include <DirectXCollision.h> // [Added] For Frustum Culling
#include "MaterialSystem.h"
#include "RenderModule.h"

namespace DE {
	namespace {
		ID3D11BlendState* GetBlendState(BlendMode mode) {
			switch (mode) {
				case BlendMode::Opaque:
					return nullptr;
				case BlendMode::Additive:
					return RenderBase::graphicsCommon.accumulateBS.Get();
				case BlendMode::AlphaBlend:
					return RenderBase::graphicsCommon.alphaBS.Get();
				case BlendMode::Modulate:
					return RenderBase::graphicsCommon.modulateBS.Get();
				default:
					return RenderBase::graphicsCommon.accumulateBS.Get();
			}
		}
	}
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 20000, 20000);
		TextureManager::Get().BindParticleTextures();

		m_batchRenderArgsCB.Initialize();
		m_buildAliveCB.Initialize();
		m_sortParamsCB.Initialize();
	}

	void ParticleManager::Update(const float& dt)
	{
		// Track time for priority age calculation
		m_currentTime += dt;

		constexpr float DEFRAG_THRESHOLD = 0.2f;
		if (m_memoryPool->GetFragmentationRatio() >= DEFRAG_THRESHOLD) {
			Defragment();
		}

		m_memoryPool->ClearWriteAliveCount();

		for (auto* system : m_activeSystems) {
			if (system) {
				system->PreUpdate(dt, m_memoryPool->GetFrameConsts().GetCpu());
			}
		}

		m_memoryPool->UploadFrameConsts();
		m_memoryPool->UploadEmitterIDs();
		m_memoryPool->UploadMeshConsts();

		m_memoryPool->UpdateArgs();

		{
			m_memoryPool->BindSpawnCompute();
			auto& spawnCS = RenderBase::computeCommon.particle.spawnCS;
			auto context = GET_SINGLE(RenderBase)->GetContext();
			context->CSSetShader(spawnCS.computeShader.Get(), 0, 0);

			for (auto* system : m_activeSystems) {
				if (system) {
					system->Update(dt);
				}
			}

			context->CSSetShader(nullptr, 0, 0);
			m_memoryPool->UnbindSpawnCompute();

			m_memoryPool->BindSimulationCompute();

			// Curl Noise 3D Texture + Sampler binding for CS
			TextureManager::Get().BindCurlNoiseTexture(26);
			// Curve LUT
			TextureManager::Get().BindCurveLUTArray(28);
			{
				auto context = GET_SINGLE(RenderBase)->GetContext();
				ID3D11SamplerState* samplers[] = {
					RenderBase::graphicsCommon.linearWrapSS.Get(),   // s0 (curl noise)
					RenderBase::graphicsCommon.linearClampSS.Get()   // s1 (curve LUT)
				};
				context->CSSetSamplers(0, 2, samplers);
			}

			m_memoryPool->ExcuteParticleLogic();
			m_memoryPool->UnbindSimulationCompute();

			// Unbind curl noise texture
			{
				auto context = GET_SINGLE(RenderBase)->GetContext();
				ID3D11ShaderResourceView* nullSRV = nullptr;
				context->CSSetShaderResources(26, 1, &nullSRV);
				context->CSSetShaderResources(28, 1, &nullSRV);
			}
		}

		// Compute , SwapBuffer  Read  ȭ
		if (m_needsSyncReadOffset) {
			SyncReadOffsets();
			m_needsSyncReadOffset = false;
		}

		if (m_memoryPool) {
			m_memoryPool->SwapAliveIndices();
		}
	}

	void ParticleManager::Render()
	{
		if (m_activeSystems.empty()) return;

		// 캐싱된 frustum/cameraPos 사용
		for (auto* sys : m_activeSystems)
			if (sys) sys->UpdateSpawnRatios(m_cachedCameraPos);

		UINT totalCount = 0, visibleCount = 0;
		GatherVisibleEmitters(m_cachedFrustum, m_cachedCameraPos, totalCount, visibleCount);
		BuildBatchDescriptors();

		m_memoryPool->UploadFrameConsts();
		m_memoryPool->BindRender();
		m_memoryPool->UpdateRenderArgs();

		if (!m_batchEmitterList.empty())
			DispatchBatchCompute();

		// batchSortParams를 GPU에서 CPU로 Download (1회)
		{
			auto context = GET_SINGLE(RenderBase)->GetContext();
			m_memoryPool->GetBatchSortParams().Download(context.Get());
		}

		SortAlphaBlendEmitters();

		m_memoryPool->BindBatchAliveIndices();
		// 전역 VB/IB Binding (Mesh와 Billboard가 공유)
		ModelManager::Get().BindBuffersForRender();

		DrawMeshBatches();
		DrawFullResBillboardBatches();
		DrawBillboardBatches();

		m_memoryPool->UnbindRender();
		GET_SINGLE(RenderBase)->RenderCompositeLowResParticles();

	}

	void ParticleManager::GatherVisibleEmitters(
		const DirectX::BoundingFrustum& frustum,
		const Vector3& cameraPos,
		UINT& totalCount, UINT& visibleCount)
	{
		m_meshJobs.clear();
		m_billboardJobs.clear();
		m_fullResBillboardJobs.clear();

		for (auto* system : m_activeSystems) {
			if (!system) continue;
			totalCount++;

			Vector3 posWorld = system->GetWorldPosition();
			Vector3 posView = Vector3::Transform(posWorld, m_view);
			float radius = system->GetBoundingRadius();
			DirectX::BoundingSphere sphere(posView, radius);

			if (frustum.Intersects(sphere)) {
				visibleCount++;
				system->GatherActiveEmitters(m_meshJobs, m_billboardJobs);
			}
		}

		std::sort(m_meshJobs.begin(), m_meshJobs.end(),
			[](const EmitterJob& a, const EmitterJob& b) {
				if (a.materialKey != b.materialKey) return a.materialKey < b.materialKey;
				return a.modelIndex < b.modelIndex;
			});

		// Partition billboard jobs into lowRes and fullRes
		auto partitionIt = std::stable_partition(m_billboardJobs.begin(), m_billboardJobs.end(),
			[](const EmitterJob& job) {
				RenderModule* render = job.emitter->GetModule<RenderModule>();
				return render ? !render->lowResolution : false; // fullRes first
			});

		// Move fullRes jobs to separate list
		m_fullResBillboardJobs.assign(m_billboardJobs.begin(), partitionIt);
		m_billboardJobs.erase(m_billboardJobs.begin(), partitionIt);

		// Sort helper: by blendMode then materialKey
		auto billboardSortFunc = [](const EmitterJob& a, const EmitterJob& b) {
			RenderModule* renderA = a.emitter->GetModule<RenderModule>();
			RenderModule* renderB = b.emitter->GetModule<RenderModule>();

			BlendMode blendA = renderA ? renderA->blendMode : BlendMode::Additive;
			BlendMode blendB = renderB ? renderB->blendMode : BlendMode::Additive;

			if (blendA != blendB) {
				return blendA < blendB;  // Opaque(0) < Additive(1) < AlphaBlend(2)
			}
			return a.materialKey < b.materialKey;
		};

		std::sort(m_fullResBillboardJobs.begin(), m_fullResBillboardJobs.end(), billboardSortFunc);
		std::sort(m_billboardJobs.begin(), m_billboardJobs.end(), billboardSortFunc);

		BuildMeshBatches(m_meshJobs, m_meshBatches);
		BuildBatches(m_fullResBillboardJobs, m_fullResBillboardBatches);
		BuildBatches(m_billboardJobs, m_billboardBatches);
	}

	void ParticleManager::BuildBatchDescriptors()
	{
		// Mesh Batch와 Billbaord Batch를 하나의 Descriptor 배열로 합쳐서 Compute Shader로 전달
		m_batchEmitterList.clear();
		m_batchDescriptors.clear();

		UINT globalInstanceOffset = 0;

		// Descriptor 배열 앞에 Mesh Batch를 추가
		// Mesh batch descriptors
		for (const auto& batch : m_meshBatches) {
			BatchDescriptor desc{};
			desc.emitterCount = static_cast<UINT>(batch.emitterIDs.size());
			desc.emitterListOffset = static_cast<UINT>(m_batchEmitterList.size());
			desc.instanceOffset = globalInstanceOffset;
			MeshRange range = ModelManager::Get().GetMeshRange(batch.modelIndex);
			desc.indexCount = range.indexCount;
			desc.startIndexLocation = range.startIndexLocation;
			desc.baseVertexLocation = range.baseVertexLocation;
			desc.isMesh = 1;
			desc.padding = 0;
			m_batchDescriptors.push_back(desc);

			for (UINT eid : batch.emitterIDs)
				globalInstanceOffset += m_memoryPool->GetReadAliveCount().GetCpu()[eid];

			m_batchEmitterList.insert(m_batchEmitterList.end(),
				batch.emitterIDs.begin(), batch.emitterIDs.end());
		}

		// Full-res billboard batch descriptors (mesh 뒤, low-res billboard 앞)
		MeshRange quadRange = ModelManager::Get().GetMeshRange(0);
		m_fullResBillboardDescStartIdx = static_cast<UINT>(m_batchDescriptors.size());

		for (const auto& batch : m_fullResBillboardBatches) {
			BatchDescriptor desc{};
			desc.emitterCount = static_cast<UINT>(batch.emitterIDs.size());
			desc.emitterListOffset = static_cast<UINT>(m_batchEmitterList.size());
			desc.instanceOffset = globalInstanceOffset;
			desc.indexCount = quadRange.indexCount;
			desc.startIndexLocation = quadRange.startIndexLocation;
			desc.baseVertexLocation = quadRange.baseVertexLocation;
			desc.isMesh = 0;
			desc.padding = 0;
			m_batchDescriptors.push_back(desc);

			for (UINT eid : batch.emitterIDs)
				globalInstanceOffset += m_memoryPool->GetReadAliveCount().GetCpu()[eid];

			m_batchEmitterList.insert(m_batchEmitterList.end(),
				batch.emitterIDs.begin(), batch.emitterIDs.end());
		}

		// Low-res billboard batch descriptors (full-res billboard 뒤)
		m_billboardDescStartIdx = static_cast<UINT>(m_batchDescriptors.size());

		for (const auto& batch : m_billboardBatches) {
			BatchDescriptor desc{};
			desc.emitterCount = static_cast<UINT>(batch.emitterIDs.size());
			desc.emitterListOffset = static_cast<UINT>(m_batchEmitterList.size());
			desc.instanceOffset = globalInstanceOffset;
			desc.indexCount = quadRange.indexCount;
			desc.startIndexLocation = quadRange.startIndexLocation;
			desc.baseVertexLocation = quadRange.baseVertexLocation;
			desc.isMesh = 0;
			desc.padding = 0;
			m_batchDescriptors.push_back(desc);

			for (UINT eid : batch.emitterIDs)
				globalInstanceOffset += m_memoryPool->GetReadAliveCount().GetCpu()[eid];

			m_batchEmitterList.insert(m_batchEmitterList.end(),
				batch.emitterIDs.begin(), batch.emitterIDs.end());
		}
	}

	void ParticleManager::DispatchBatchCompute()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();

		m_memoryPool->UploadBatchData(m_batchEmitterList, m_batchDescriptors);

		// 공통 SRV 바인딩 (Pass 1, Pass 2 공유) — t16~t29
		ID3D11ShaderResourceView* commonSRVs[] = {
			m_memoryPool->GetParticleBuffer().GetSRV(),    // t16
			m_memoryPool->GetReadAliveCount().GetSRV(),    // t17
			m_memoryPool->GetFrameConsts().GetSRV(),       // t18
			nullptr,                                        // t19
			nullptr,                                        // t20
			m_memoryPool->GetEmitterIDs().GetSRV(),        // t21
			nullptr,                                        // t22
			nullptr,                                        // t23
			m_memoryPool->GetBatchEmitterList().GetSRV(),  // t24
			m_memoryPool->GetBatchDescriptors().GetSRV(),  // t25
			nullptr,                                        // t26
			nullptr,                                        // t27
			m_memoryPool->GetReadAliveIndices().GetSRV(),  // t28
			m_memoryPool->GetReadAliveCount().GetSRV()     // t29
		};
		context->CSSetShaderResources(16, 14, commonSRVs);

		// Pass 1: BatchRenderArgsCS
		UINT numBatches = static_cast<UINT>(m_batchDescriptors.size());
		m_batchRenderArgsCB.SetCpuData({ numBatches, Vector3(0,0,0) });
		m_batchRenderArgsCB.Upload();

		context->CSSetConstantBuffers(0, 1, m_batchRenderArgsCB.GetAddressOf());

		ID3D11UnorderedAccessView* uavs[] = {
			m_memoryPool->GetBatchBillboardArgs().GetUAV(),   // u0
			m_memoryPool->GetEmitterWriteOffsets().GetUAV(),   // u1
			m_memoryPool->GetBatchSortParams().GetUAV()        // u2
		};
		context->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);

		auto& batchArgsCS = RenderBase::computeCommon.particle.batchRenderArgsCS;
		context->CSSetShader(batchArgsCS.computeShader.Get(), nullptr, 0);
		context->Dispatch(1, 1, 1);

		// Unbind Pass 1 UAVs
		context->CSSetShader(nullptr, nullptr, 0);
		ID3D11UnorderedAccessView* nullUAVs3[] = { nullptr, nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 3, nullUAVs3, nullptr);

		// Pass 2: BuildAliveIndicesCS
		UINT numFlatEmitters = static_cast<UINT>(m_batchEmitterList.size());
		if (numFlatEmitters > 0) {
			m_buildAliveCB.SetCpuData({ numFlatEmitters, Vector3(0,0,0) });
			m_buildAliveCB.Upload();

			context->CSSetConstantBuffers(0, 1, m_buildAliveCB.GetAddressOf());

			ID3D11ShaderResourceView* aliveSRVs[] = {
				m_memoryPool->GetEmitterWriteOffsets().GetSRV()
			};
			context->CSSetShaderResources(0, 1, aliveSRVs);

			ID3D11UnorderedAccessView* aliveUAVs[] = {
				m_memoryPool->GetBatchAliveIndices().GetUAV()
			};
			context->CSSetUnorderedAccessViews(0, 1, aliveUAVs, nullptr);

			auto& buildAliveCS = RenderBase::computeCommon.particle.buildAliveIndicesCS;
			context->CSSetShader(buildAliveCS.computeShader.Get(), nullptr, 0);
			context->Dispatch(numFlatEmitters, 1, 1);

			// Unbind Pass 2
			context->CSSetShader(nullptr, nullptr, 0);
			ID3D11UnorderedAccessView* nullUAV1[] = { nullptr };
			context->CSSetUnorderedAccessViews(0, 1, nullUAV1, nullptr);
			ID3D11ShaderResourceView* nullSRV1[] = { nullptr };
			context->CSSetShaderResources(0, 1, nullSRV1);
		}

		ID3D11ShaderResourceView* nullSRVs[14] = { nullptr };
		context->CSSetShaderResources(16, 14, nullSRVs);
	}

	void ParticleManager::SortAlphaBlendEmitters()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		Vector3 cameraForward(m_view._13, m_view._23, m_view._33);
		cameraForward.Normalize();

		// Lambda for sorting a batch group
		auto sortBatchGroup = [&](const std::vector<BatchGroup>& batches, UINT descStartIdx) {
			UINT firstBatchIdx = UINT_MAX;
			UINT lastBatchIdx = UINT_MAX;
			UINT totalParticleCount = 0;
			UINT baseOffset = UINT_MAX;

			// 1. AlphaBlend Batch들의 전체 범위(Boundary) 계산
			for (size_t batchIdx = 0; batchIdx < batches.size(); batchIdx++) {
				if (batches[batchIdx].blendMode == BlendMode::AlphaBlend) {
					UINT globalBatchIdx = descStartIdx + static_cast<UINT>(batchIdx);

					if (firstBatchIdx == UINT_MAX) {
						firstBatchIdx = globalBatchIdx;
						baseOffset = m_memoryPool->GetBatchSortParams().GetCpu()[globalBatchIdx].baseOffset;
					}
					lastBatchIdx = globalBatchIdx;
					totalParticleCount += m_memoryPool->GetBatchSortParams().GetCpu()[globalBatchIdx].particleCount;
				}
			}

			if (totalParticleCount == 0) return;

			// 2. 단 한 번의 정렬을 위한 파라미터 세팅
			UINT sortSize = 1;
			while (sortSize < totalParticleCount) sortSize *= 2;

			m_sortParamsCB.GetCpu().sortBaseOffset = baseOffset;
			m_sortParamsCB.GetCpu().sortParticleCount = totalParticleCount;
			m_sortParamsCB.GetCpu().firstBatchIdx = firstBatchIdx;
			m_sortParamsCB.GetCpu().lastBatchIdx = lastBatchIdx;
			m_sortParamsCB.GetCpu().cameraForward = cameraForward;
			m_sortParamsCB.Upload();
			m_sortParamsCB.Bind(5);

			// GlobalCB 바인딩 (BitonicSort용)
			ComPtr<ID3D11Buffer> globalCB;
			context->VSGetConstantBuffers(0, 1, globalCB.GetAddressOf());
			if (globalCB) context->CSSetConstantBuffers(0, 1, globalCB.GetAddressOf());

			// batchSortParams를 t1에 바인딩 (GenerateSortKeysCS에서 Batch Boundary 파악용)
			ID3D11ShaderResourceView* sortParamsSRV = m_memoryPool->GetBatchSortParams().GetSRV();
			context->CSSetShaderResources(1, 1, &sortParamsSRV);

			// 1. GenerateSortKeys
			auto& genKeysCS = RenderBase::computeCommon.particle.generateSortKeysCS;
			context->CSSetShader(genKeysCS.computeShader.Get(), 0, 0);

			ID3D11ShaderResourceView* batchSRV = m_memoryPool->GetBatchAliveIndices().GetSRV();
			context->CSSetShaderResources(0, 1, &batchSRV);
			ID3D11ShaderResourceView* particleSRV = m_memoryPool->GetParticleBuffer().GetSRV();
			context->CSSetShaderResources(16, 1, &particleSRV);

			ID3D11UnorderedAccessView* sortUAV = m_memoryPool->GetBitonicSort().GetUAV();
			context->CSSetUnorderedAccessViews(0, 1, &sortUAV, nullptr);

			context->Dispatch((sortSize + 1023) / 1024, 1, 1);

			// UAV barrier
			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

			// 2. BitonicSort
			m_memoryPool->GetBitonicSort().Sort(context.Get(), sortUAV, totalParticleCount);

			// UAV barrier: Sort 완료 후 UAV unbind하여 쓰기 완료 보장
			{
				ID3D11UnorderedAccessView* nullBarrier = nullptr;
				context->CSSetUnorderedAccessViews(0, 1, &nullBarrier, nullptr);
			}

			// BitonicSort 후 b5 재바인딩 (BitonicSort가 b0, SRV 상태를 변경할 수 있음)
			m_sortParamsCB.Bind(5);

			// 3. CopySortedIndices
			auto& copyCS = RenderBase::computeCommon.particle.copySortedIndicesCS;
			context->CSSetShader(copyCS.computeShader.Get(), 0, 0);

			ID3D11ShaderResourceView* sortedSRV = m_memoryPool->GetBitonicSort().GetSRV();
			context->CSSetShaderResources(0, 1, &sortedSRV);

			ID3D11UnorderedAccessView* batchUAV = m_memoryPool->GetBatchAliveIndices().GetUAV();
			context->CSSetUnorderedAccessViews(0, 1, &batchUAV, nullptr);

			context->Dispatch((totalParticleCount + 1023) / 1024, 1, 1);

			// UAV barrier: 다음 배치의 GenerateSortKeys가 batchAliveIndices를
			// SRV로 바인딩하기 전에 UAV 바인딩을 해제 (SRV/UAV 충돌 방지)
			{
				ID3D11UnorderedAccessView* nullBarrier = nullptr;
				context->CSSetUnorderedAccessViews(0, 1, &nullBarrier, nullptr);
			}
		};

		sortBatchGroup(m_fullResBillboardBatches, m_fullResBillboardDescStartIdx);
		sortBatchGroup(m_billboardBatches, m_billboardDescStartIdx);

		// Cleanup
		context->CSSetShader(nullptr, nullptr, 0);
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->CSSetShaderResources(0, 1, &nullSRV);
		context->CSSetShaderResources(16, 1, &nullSRV);
	}

	void ParticleManager::DrawMeshBatches()
	{
		if (m_meshBatches.empty()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.meshPSO);

		ID3D11ShaderResourceView* meshBatchSRVs[] = {
			m_memoryPool->GetBatchEmitterList().GetSRV(),
			m_memoryPool->GetBatchDescriptors().GetSRV()
		};
		context->VSSetShaderResources(24, 2, meshBatchSRVs);

		// Material이 변경될때만 Binding해서 렌더링
		int lastMaterialKey = INT_MIN;
		for (size_t i = 0; i < m_meshBatches.size(); i++) {
			const auto& batch = m_meshBatches[i];
			const auto& desc = m_batchDescriptors[i];

			if (batch.materialKey != lastMaterialKey) {
				lastMaterialKey = batch.materialKey;
				if (batch.materialKey < 0) {
					m_memoryPool->BindDefaultParticleMaterial();
				} else {
					MaterialSystem::Get().BindMaterial(batch.materialKey);
				}
			}

			m_memoryPool->BindBatchInfo(desc.emitterCount, desc.emitterListOffset, desc.instanceOffset);

			ID3D11Buffer* batchArgs = m_memoryPool->GetBatchBillboardArgs().GetBuffer();
			context->DrawIndexedInstancedIndirect(batchArgs, static_cast<UINT>(i) * 20);
		}

		ID3D11ShaderResourceView* nullMeshSRVs[2] = { nullptr };
		context->VSSetShaderResources(24, 2, nullMeshSRVs);
	}

	void ParticleManager::DrawFullResBillboardBatches()
	{
		if (m_fullResBillboardBatches.empty()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.billboardInstancedPSO);

		// Bind full-res scene depth for soft particles
		ID3D11ShaderResourceView* depthSRV = GET_SINGLE(RenderBase)->GetFullResSceneDepthSRV();
		context->PSSetShaderResources(7, 1, &depthSRV);

		TextureManager::Get().Bind2DCurlNoiseTexture(27);

		ID3D11ShaderResourceView* batchSRVs[] = {
			m_memoryPool->GetBatchEmitterList().GetSRV(),
			m_memoryPool->GetBatchDescriptors().GetSRV()
		};
		context->VSSetShaderResources(24, 2, batchSRVs);

		BlendMode lastBlendMode = static_cast<BlendMode>(-1);

		for (size_t batchIdx = 0; batchIdx < m_fullResBillboardBatches.size(); batchIdx++) {
			const auto& batch = m_fullResBillboardBatches[batchIdx];
			UINT globalBatchIdx = m_fullResBillboardDescStartIdx + static_cast<UINT>(batchIdx);
			const auto& desc = m_batchDescriptors[globalBatchIdx];

			if (batch.blendMode != lastBlendMode) {
				lastBlendMode = batch.blendMode;
				context->OMSetBlendState(GetBlendState(batch.blendMode),
					RenderBase::graphicsCommon.particle.animPSO.blendFactor,
					0xffffffff);
				if (batch.blendMode == BlendMode::Opaque) {
					context->OMSetDepthStencilState(RenderBase::graphicsCommon.drawDSS.Get(), 0);
				} else {
					context->OMSetDepthStencilState(RenderBase::graphicsCommon.particleDDS.Get(), 0);
				}
			}

			if (batch.materialKey < 0) {
				m_memoryPool->BindDefaultParticleMaterial();
			} else {
				MaterialSystem::Get().BindMaterial(batch.materialKey);
			}

			m_memoryPool->BindBatchInfo(desc.emitterCount, desc.emitterListOffset, desc.instanceOffset);

			ID3D11Buffer* batchArgs = m_memoryPool->GetBatchBillboardArgs().GetBuffer();
			context->DrawIndexedInstancedIndirect(batchArgs, globalBatchIdx * 20);
		}

		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr };
		context->VSSetShaderResources(24, 2, nullSRVs);

		ID3D11ShaderResourceView* nullDepthSRV = nullptr;
		context->PSSetShaderResources(7, 1, &nullDepthSRV);
	}

	void ParticleManager::DrawBillboardBatches()
	{
		auto context = GET_SINGLE(RenderBase)->GetContext();
		GET_SINGLE(RenderBase)->SetLowResRender();

		if (m_billboardBatches.empty()) return;

		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.billboardInstancedPSO);

		// Bind scene depth for soft particles
		ID3D11ShaderResourceView* depthSRV = GET_SINGLE(RenderBase)->GetLowResSceneDepthSRV();
		context->PSSetShaderResources(7, 1, &depthSRV);

		TextureManager::Get().Bind2DCurlNoiseTexture(27);

		ID3D11ShaderResourceView* batchSRVs[] = {
			m_memoryPool->GetBatchEmitterList().GetSRV(),
			m_memoryPool->GetBatchDescriptors().GetSRV()
		};
		context->VSSetShaderResources(24, 2, batchSRVs);

		BlendMode lastBlendMode = static_cast<BlendMode>(-1);

		// Material이 변경될때만 Binding해서 렌더링
		for (size_t batchIdx = 0; batchIdx < m_billboardBatches.size(); batchIdx++) {
			const auto& batch = m_billboardBatches[batchIdx];
			// Billboard는 Mesh Batch 뒤에 저장했으므로 Billboard Batch의 시작 위치 더해주기
			UINT globalBatchIdx = m_billboardDescStartIdx + static_cast<UINT>(batchIdx);
			const auto& desc = m_batchDescriptors[globalBatchIdx];

			// BlendMode 변경 시에만 BlendState 설정
			if (batch.blendMode != lastBlendMode) {
				lastBlendMode = batch.blendMode;
				context->OMSetBlendState(GetBlendState(batch.blendMode),
					RenderBase::graphicsCommon.particle.animPSO.blendFactor,
					0xffffffff);
				if (batch.blendMode == BlendMode::Opaque) {
					context->OMSetDepthStencilState(RenderBase::graphicsCommon.drawDSS.Get(), 0);
				} else {
					context->OMSetDepthStencilState(RenderBase::graphicsCommon.particleDDS.Get(), 0);
				}
			}

			if (batch.materialKey < 0) {
				m_memoryPool->BindDefaultParticleMaterial();
			} else {
				MaterialSystem::Get().BindMaterial(batch.materialKey);
			}

			m_memoryPool->BindBatchInfo(desc.emitterCount, desc.emitterListOffset, desc.instanceOffset);

			ID3D11Buffer* batchArgs = m_memoryPool->GetBatchBillboardArgs().GetBuffer();
			context->DrawIndexedInstancedIndirect(batchArgs, globalBatchIdx * 20);
		}

		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr };
		context->VSSetShaderResources(24, 2, nullSRVs);

		// Unbind scene depth SRV
		ID3D11ShaderResourceView* nullDepthSRV = nullptr;
		context->PSSetShaderResources(7, 1, &nullDepthSRV);
	}

	void ParticleManager::RenderDepth()
	{
		if (m_activeSystems.empty()) return;

		UINT totalCount = 0, visibleCount = 0;
		GatherVisibleEmitters(m_cachedFrustum, m_cachedCameraPos, totalCount, visibleCount);

		// Depth pass only renders mesh particles - clear billboard data
		m_billboardJobs.clear();
		m_billboardBatches.clear();
		m_fullResBillboardJobs.clear();
		m_fullResBillboardBatches.clear();

		BuildBatchDescriptors();

		m_memoryPool->UploadFrameConsts();
		m_memoryPool->BindRender();
		m_memoryPool->UpdateRenderArgs();

		if (!m_batchEmitterList.empty())
			DispatchBatchCompute();

		m_memoryPool->BindBatchAliveIndices();
		ModelManager::Get().BindBuffersForRender();

		DrawMeshBatchesDepth();

		m_memoryPool->UnbindRender();
	}

	void ParticleManager::DrawMeshBatchesDepth()
	{
		if (m_meshBatches.empty()) return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		ID3D11ShaderResourceView* meshBatchSRVs[] = {
			m_memoryPool->GetBatchEmitterList().GetSRV(),
			m_memoryPool->GetBatchDescriptors().GetSRV()
		};
		context->VSSetShaderResources(24, 2, meshBatchSRVs);

		for (size_t i = 0; i < m_meshBatches.size(); i++) {
			const auto& desc = m_batchDescriptors[i];

			m_memoryPool->BindBatchInfo(desc.emitterCount, desc.emitterListOffset, desc.instanceOffset);

			ID3D11Buffer* batchArgs = m_memoryPool->GetBatchBillboardArgs().GetBuffer();
			context->DrawIndexedInstancedIndirect(batchArgs, static_cast<UINT>(i) * 20);
		}

		ID3D11ShaderResourceView* nullMeshSRVs[2] = { nullptr };
		context->VSSetShaderResources(24, 2, nullMeshSRVs);
	}

	void ParticleManager::RegisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;

		auto it = std::find(m_activeSystems.begin(), m_activeSystems.end(), system);
		if (it == m_activeSystems.end()) {
			m_activeSystems.push_back(system);
		}
	}

	void ParticleManager::UnregisterActiveSystem(ParticleSystem* system)
	{
		if (!system) return;

		auto it = std::find(m_activeSystems.begin(), m_activeSystems.end(), system);
		if (it != m_activeSystems.end()) {
			m_activeSystems.erase(it);
		}
	}

	ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
	{
		ParticleSystem* prototype = nullptr;

		// 1. Prototype Load (Cache üũ)
		auto prototypeIt = m_prototypes.find(path);
		if (prototypeIt != m_prototypes.end())
		{
			prototype = prototypeIt->second.get();
		}
		else
		{
			auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
			if (!newSystem) return nullptr;

			newSystem->Initialize();
			prototype = newSystem.get();
			m_prototypes[path] = std::move(newSystem);
		}

		// 2. Clone Instance
		auto cloned = std::make_unique<ParticleSystem>(*prototype);
		ParticleSystem* clonedPtr = cloned.get();

		// 3. CPU Initialize & Memory Allocation
		ParticleInitializer initialData;
		clonedPtr->InitializeCPU(initialData);

		UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
		UINT particleCount = clonedPtr->GetTotalParticleCount();
		UINT emitterCount = clonedPtr->GetMaxEmitterCount();

		// Pool-level spawn pos caching for both baked and custom positions
		bool spawnPosCacheHit = false;
		UINT cachedSpawnPosOffset = UINT_MAX;
		if (!initialData.bakedPosKey.empty() && spawnPosCount > 0) {
			spawnPosCacheHit = m_memoryPool->AllocateSpawnPosForBakedPath(
				initialData.bakedPosKey, spawnPosCount, cachedSpawnPosOffset);
		}

		// If cache hit, skip spawn pos block allocation
		UINT allocSpawnPosCount = (cachedSpawnPosOffset != UINT_MAX) ? 0 : spawnPosCount;
		PoolHandle handle = RequestAllocation(particleCount, emitterCount, allocSpawnPosCount);

		// Try eviction if allocation failed
		if (!handle.IsActive())
		{
			if (!TryEvictAndRetry(particleCount, emitterCount, allocSpawnPosCount, handle))
			{
				// Still failed after eviction attempts
				return nullptr;
			}
		}

		// Apply cached spawn pos offset
		if (cachedSpawnPosOffset != UINT_MAX) {
			handle.spawnPosOffset = cachedSpawnPosOffset;
			handle.bakedPosKey = initialData.bakedPosKey;
		}

		// Set creation timestamp
		clonedPtr->SetCreationTime(m_currentTime);

		clonedPtr->SetPoolHandle(handle);

		// 4. GPU Resource Upload
		if (!initialData.spawnPositions.empty() && handle.spawnPosOffset != UINT_MAX && !spawnPosCacheHit)
		{
			m_memoryPool->UploadSpawnPositions(handle.spawnPosOffset, initialData.spawnPositions);
		}

		clonedPtr->InitializeGPU(initialData,
			m_memoryPool->GetDispatchArgs());

		UploadEmitterIDs(clonedPtr, clonedPtr->GetInitialData());
		m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
		m_memoryPool->UpdateFrameConsts(handle.emitterIDs, initialData.frameConsts);

		// 5. Finalize & Registration
		clonedPtr->Initialize(initialData);
		clonedPtr->OnSpawn();

		m_instances.push_back(std::move(cloned));
		RegisterActiveSystem(clonedPtr);

		return clonedPtr;
	}

	void ParticleManager::DestroyInstance(ParticleSystem* system)
	{
		if (!system) return;

		const PoolHandle& handle = system->GetPoolHandle();
		if (handle.IsActive()) {
			m_memoryPool->Free(handle);
		}

		UnregisterActiveSystem(system);

		auto it = std::find_if(m_instances.begin(), m_instances.end(),
			[system](const std::unique_ptr<ParticleSystem>& ptr) {
				return ptr.get() == system;
			});

		if (it != m_instances.end()) {
			m_instances.erase(it);
		}
	}

	void ParticleManager::BuildBatches(const std::vector<EmitterJob>& jobs, std::vector<BatchGroup>& outBatches)
	{
		outBatches.clear();
		if (jobs.empty()) return;

		// Phase 1: Collect no-material emitters into separate batch
		BatchGroup noMaterialBatch;
		noMaterialBatch.materialKey = -1;
		noMaterialBatch.modelIndex = -1;
		noMaterialBatch.blendMode = BlendMode::Additive;
		noMaterialBatch.instanceOffset = 0;

		std::vector<EmitterJob> materialJobs;  // Jobs with valid materials

		for (const auto& job : jobs) {
			if (job.materialKey < 0) {  // No material (invalid materialKey)
				noMaterialBatch.emitterIDs.push_back(job.globalEmitterID);
			} else {
				materialJobs.push_back(job);  // Has material - process later
			}
		}

		// Add no-material batch first (if any)
		if (!noMaterialBatch.emitterIDs.empty()) {
			outBatches.push_back(noMaterialBatch);
		}

		// Phase 2: Build material batches - group by materialKey AND BlendMode
		if (materialJobs.empty()) return;

		// 첫 번째 배치 초기화
		RenderModule* firstRender = materialJobs[0].emitter->GetModule<RenderModule>();
		BatchGroup currentBatch;
		currentBatch.materialKey = materialJobs[0].materialKey;
		currentBatch.modelIndex = -1;
		currentBatch.blendMode = firstRender ? firstRender->blendMode : BlendMode::Additive;
		currentBatch.emitterIDs.push_back(materialJobs[0].globalEmitterID);
		currentBatch.instanceOffset = 0;

		for (size_t i = 1; i < materialJobs.size(); ++i) {
			RenderModule* renderMod = materialJobs[i].emitter->GetModule<RenderModule>();
			BlendMode jobBlendMode = renderMod ? renderMod->blendMode : BlendMode::Additive;

			if (materialJobs[i].materialKey == currentBatch.materialKey &&
				jobBlendMode == currentBatch.blendMode) {
				// 같은 Material & BlendMode - 현재 배치에 추가
				currentBatch.emitterIDs.push_back(materialJobs[i].globalEmitterID);
			} else {
				// 다른 Material 또는 BlendMode - 새 배치 생성
				outBatches.push_back(currentBatch);
				currentBatch.materialKey = materialJobs[i].materialKey;
				currentBatch.modelIndex = -1;
				currentBatch.blendMode = jobBlendMode;
				currentBatch.emitterIDs.clear();
				currentBatch.emitterIDs.push_back(materialJobs[i].globalEmitterID);
			}
		}

		// Add last batch
		outBatches.push_back(currentBatch);
	}

	void ParticleManager::BuildMeshBatches(const std::vector<EmitterJob>& jobs, std::vector<BatchGroup>& outBatches)
	{
		outBatches.clear();
		if (jobs.empty()) return;

		BatchGroup currentBatch;
		currentBatch.materialKey = jobs[0].materialKey;
		currentBatch.modelIndex = jobs[0].modelIndex;
		currentBatch.emitterIDs.push_back(jobs[0].globalEmitterID);
		currentBatch.instanceOffset = 0;

		// 이미 Material과 Model 순으로 정렬된 상태
		// 연속된 같은 Material과 Model을 가진 emitter들을 하나의 batch로 합치기
		for (size_t i = 1; i < jobs.size(); ++i) {
			// 같은 배치에 추가
			if (jobs[i].materialKey == currentBatch.materialKey &&
				jobs[i].modelIndex == currentBatch.modelIndex) {
				currentBatch.emitterIDs.push_back(jobs[i].globalEmitterID);
			} else {
				// 새 배치 시작
				outBatches.push_back(currentBatch);
				currentBatch.materialKey = jobs[i].materialKey;
				currentBatch.modelIndex = jobs[i].modelIndex;
				currentBatch.emitterIDs.clear();
				currentBatch.emitterIDs.push_back(jobs[i].globalEmitterID);
			}
		}
		outBatches.push_back(currentBatch);
	}

	void ParticleManager::BindEmitterID(UINT globalSlotIndex)
	{
		m_memoryPool->BindEmitterID(globalSlotIndex);
	}

	PoolHandle ParticleManager::RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount)
	{
		PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);

		// Defragment  
		if (!handle.IsActive()) {
			m_needsDefragment = true;
		}

		return handle;
	}

	void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
	{
		const PoolHandle& handle = system->GetPoolHandle();

		for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
			EmitterID eID = initialData.emitterIDs[i];

			eID.emitterID = handle.emitterIDs[i];
			eID.systemID = handle.systemSlot;
			eID.readParticleOffset = handle.particleOffset + initialData.emitterIDs[i].readParticleOffset;
			eID.writeParticleOffset = handle.particleOffset + initialData.emitterIDs[i].writeParticleOffset;

			// spawnPos ϴ emitter  
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}

			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::UpdateMeshConsts(UINT systemSlot, const ParticleMeshConsts& data)
	{
		m_memoryPool->UpdateMeshConsts(systemSlot, data);
	}

	void ParticleManager::Defragment()
	{
		if (m_activeSystems.empty()) return;

		// 복사본으로 순회 — Defragment 중 m_activeSystems 변경 시 iterator invalidation 방지
		auto systemsCopy = m_activeSystems;

		std::vector<PoolHandle> activeHandles;
		for (auto* system : systemsCopy) {
			if (system && system->GetPoolHandle().IsActive()) {
				activeHandles.push_back(system->GetPoolHandle());
			}
		}

		std::vector<UINT> newOffsets = m_memoryPool->Defragment(activeHandles);

		size_t idx = 0;
		for (auto* system : systemsCopy) {
			if (!system || !system->GetPoolHandle().IsActive()) continue;

			PoolHandle& handle = system->GetPoolHandle();
			UINT newOffset = newOffsets[idx++];

			if (handle.particleOffset != newOffset) {
				RecalculateEmitterOffsets(system, newOffset);
			}
		}

		m_needsSyncReadOffset = true;
	}

	void ParticleManager::RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset)
	{
		PoolHandle& handle = system->GetPoolHandle();
		const ParticleInitializer& initialData = system->GetInitialData();

		for (size_t i = 0; i < handle.emitterIDs.size(); ++i) {
			// writeParticleOffset  ġ 
			UINT globalEmitterID = handle.emitterIDs[i];
			UINT localOffset = initialData.emitterIDs[i].writeParticleOffset;

			m_memoryPool->UpdateWriteOffset(globalEmitterID, newParticleOffset + localOffset);
		}

		handle.particleOffset = newParticleOffset;
	}

	void ParticleManager::SyncReadOffsets()
	{
		// 두 배열 모두 순회
		for (auto* system : m_activeSystems) {
			if (!system || !system->GetPoolHandle().IsActive()) continue;

			const PoolHandle& handle = system->GetPoolHandle();
			for (UINT emitterID : handle.emitterIDs) {
				m_memoryPool->SyncReadOffset(emitterID);
			}
		}
	}

	float ParticleManager::CalculatePriority(
		ParticleSystem* system,
		const Vector3& cameraPos,
		const DirectX::BoundingFrustum& frustum) const
	{
		// 1. Distance Factor (closer = higher priority)
		Vector3 systemPos = system->GetWorldPosition();
		float distance = Vector3::Distance(systemPos, cameraPos);

		// Normalize to [0, 1] with inverse square falloff
		constexpr float MAX_DISTANCE = 100.0f;
		float distanceFactor = 1.0f - std::min(distance / MAX_DISTANCE, 1.0f);
		distanceFactor = distanceFactor * distanceFactor;  // Square for sharper falloff

		// 2. Visibility Factor (inside frustum = 1.0, outside = 0.0)
		Vector3 posView = Vector3::Transform(systemPos, m_view);
		float radius = system->GetBoundingRadius();
		DirectX::BoundingSphere sphere(posView, radius);
		float visibilityFactor = frustum.Intersects(sphere) ? 1.0f : 0.0f;

		// 3. Age Factor (newer = slightly higher priority)
		float age = m_currentTime - system->GetCreationTime();
		constexpr float AGE_THRESHOLD = 5.0f;  // Systems < 5s old get bonus
		float ageFactor = std::max(0.0f, 1.0f - (age / AGE_THRESHOLD));

		// 4. Base Priority Factor (user-defined)
		float basePriorityFactor = system->GetBasePriority();

		// Weighted sum: distance(40%) + visibility(30%) + age(20%) + base(10%)
		float priority = (distanceFactor * 0.4f) +
			(visibilityFactor * 0.3f) +
			(ageFactor * 0.2f) +
			(basePriorityFactor * 0.1f);

		return priority;
	}

	ParticleSystem* ParticleManager::FindLowestPrioritySystem(UINT particleCount)
	{
		if (m_instances.empty())
			return nullptr;

		// 캐싱된 frustum/cameraPos 사용
		ParticleSystem* lowestPrioritySystem = nullptr;
		float lowestPriority = FLT_MAX;

		for (const auto& instance : m_instances)
		{
			ParticleSystem* system = instance.get();
			if (!system) continue;

			float priority = CalculatePriority(system, m_cachedCameraPos, m_cachedFrustum);

			if (priority < lowestPriority)
			{
				lowestPriority = priority;
				lowestPrioritySystem = system;
			}
		}

		return lowestPrioritySystem;
	}

	bool ParticleManager::TryEvictAndRetry(
		UINT particleCount,
		UINT emitterCount,
		UINT spawnPosCount,
		PoolHandle& outHandle)
	{
		constexpr int MAX_EVICTION_ATTEMPTS = 3;

		for (int attempt = 0; attempt < MAX_EVICTION_ATTEMPTS; ++attempt)
		{
			ParticleSystem* victim = FindLowestPrioritySystem(particleCount);

			if (!victim)
				return false;

			DestroyInstance(victim);

			outHandle = RequestAllocation(particleCount, emitterCount, spawnPosCount);

			if (outHandle.IsActive())
				return true;
		}

		return false;
	}
}