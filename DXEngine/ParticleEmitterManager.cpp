#include "pch.h"
#include "ParticleEmitterManager.h"
#include "ParticleSystem.h"
#include "ParticleManager.h"
#include "RenderModule.h"
#include "MaterialModule.h"
#include "MaterialSystem.h"
#include "ModelManager.h"
#include "RenderBase.h"

namespace DE {

	UINT ParticleEmitterManager::CreateEmitter(std::unique_ptr<ParticleEmitter>&& emitter, ParticleSystem* ownerSystem)
	{
		UINT index;

		if (!m_freeSlots.empty()) {
			// Reuse freed slot
			index = m_freeSlots.front();
			m_freeSlots.pop();

			m_emitterSlots[index].emitter = std::move(emitter);
			m_emitterSlots[index].ownerSystem = ownerSystem;
			m_emitterSlots[index].isActive = true;
		}
		else {
			// Create new slot
			index = static_cast<UINT>(m_emitterSlots.size());
			m_emitterSlots.push_back({ std::move(emitter), ownerSystem, true });
		}

		return index;
	}

	void ParticleEmitterManager::DestroyEmitter(UINT emitterIndex)
	{
		if (emitterIndex >= m_emitterSlots.size()) return;

		auto& slot = m_emitterSlots[emitterIndex];
		slot.emitter.reset();
		slot.ownerSystem = nullptr;
		slot.isActive = false;

		m_freeSlots.push(emitterIndex);
	}

	ParticleEmitter* ParticleEmitterManager::GetEmitter(UINT emitterIndex)
	{
		if (emitterIndex >= m_emitterSlots.size()) return nullptr;

		auto& slot = m_emitterSlots[emitterIndex];
		if (!slot.isActive) return nullptr;

		return slot.emitter.get();
	}

	void ParticleEmitterManager::RebuildBatchGroups(const std::vector<ParticleSystem*>& visibleSystems)
	{
		m_meshBatchGroups.clear();
		m_billboardBatchGroups.clear();

		std::unordered_map<UINT, BatchGroup> meshGroupMap;
		std::unordered_map<UINT, BatchGroup> billboardGroupMap;

		// Collect all emitters from visible systems
		for (auto* system : visibleSystems) {
			// Main emitters
			for (UINT index : system->GetMainEmitterIndices()) {
				if (auto* emitter = GetEmitter(index)) {
					GroupEmitter(emitter, index, meshGroupMap, billboardGroupMap);
				}
			}

			// Active SubEmitters
			for (UINT index : system->GetActiveMeshSubEmitters()) {
				if (auto* emitter = GetEmitter(index)) {
					GroupEmitter(emitter, index, meshGroupMap, billboardGroupMap);
				}
			}
			for (UINT index : system->GetActiveBillboardSubEmitters()) {
				if (auto* emitter = GetEmitter(index)) {
					GroupEmitter(emitter, index, meshGroupMap, billboardGroupMap);
				}
			}
		}

		// Convert maps to vectors
		for (auto& [key, group] : meshGroupMap) {
			m_meshBatchGroups.push_back(std::move(group));
		}
		for (auto& [key, group] : billboardGroupMap) {
			m_billboardBatchGroups.push_back(std::move(group));
		}
	}

	void ParticleEmitterManager::GroupEmitter(
		ParticleEmitter* emitter, UINT emitterIndex,
		std::unordered_map<UINT, BatchGroup>& meshGroups,
		std::unordered_map<UINT, BatchGroup>& billboardGroups)
	{
		auto* renderModule = emitter->GetModule<RenderModule>();
		auto* materialModule = emitter->GetModule<MaterialModule>();

		if (!renderModule) return;

		int materialIndex = materialModule ? materialModule->GetMaterialIndex(0) : 0;
		BlendMode blendMode = renderModule->blendMode;
		UINT groupKey = HashBatchKey(materialIndex, blendMode);

		if (dynamic_cast<MeshRenderModule*>(renderModule)) {
			auto& group = meshGroups[groupKey];
			group.materialIndex = materialIndex;
			group.blendMode = blendMode;
			group.emitterIndices.push_back(emitterIndex);
		}
		else if (dynamic_cast<BillboardRenderModule*>(renderModule)) {
			auto& group = billboardGroups[groupKey];
			group.materialIndex = materialIndex;
			group.blendMode = blendMode;
			group.emitterIndices.push_back(emitterIndex);
		}
	}

	void ParticleEmitterManager::RenderMeshBatched(const std::vector<ParticleSystem*>& visibleSystems)
	{
		RebuildBatchGroups(visibleSystems);

		auto context = GET_SINGLE(RenderBase)->GetContext();
		ModelManager::Get().BindBuffersForRender();
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.meshPSO);

		// Render per batch group
		for (const auto& batch : m_meshBatchGroups) {
			// 1. Bind material once per batch
			MaterialSystem::Get().BindMaterial(batch.materialIndex);

			// 2. Set BlendState
			ID3D11BlendState* blendState = GetBlendStateForMode(batch.blendMode);
			context->OMSetBlendState(blendState, nullptr, 0xffffffff);

			// 3. Render all emitters with the same material
			for (UINT emitterIndex : batch.emitterIndices) {
				if (auto* emitter = GetEmitter(emitterIndex)) {
					auto* owner = emitter->GetOwner();
					if (!owner) continue;

					UINT globalEmitterID = owner->GetPoolHandle().emitterIDs[emitter->GetEmitterID()];
					ArgsParam meshArgs = {
						ParticleManager::Get().GetMemoryPool().GetMeshArgs().GetBuffer(),
						owner->GetMeshArgsOffset(globalEmitterID)
					};

					// Material already bound, so skip material binding in emitter render
					emitter->Render({}, meshArgs);
				}
			}
		}
	}

	void ParticleEmitterManager::RenderBillboardBatched(const std::vector<ParticleSystem*>& visibleSystems)
	{
		// Batch groups already built by RenderMeshBatched if called first;
		// rebuild only if billboard groups are empty and there are visible systems
		if (m_billboardBatchGroups.empty() && !visibleSystems.empty()) {
			RebuildBatchGroups(visibleSystems);
		}

		auto context = GET_SINGLE(RenderBase)->GetContext();
		auto& memoryPool = ParticleManager::Get().GetMemoryPool();

		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.billboardInstancedPSO);

		// Render per batch group
		for (const auto& batch : m_billboardBatchGroups) {
			// 1. Bind material once per batch
			MaterialSystem::Get().BindMaterial(batch.materialIndex);

			// 2. Set BlendState
			ID3D11BlendState* blendState = GetBlendStateForMode(batch.blendMode);
			context->OMSetBlendState(blendState, nullptr, 0xffffffff);

			// 3. Render all emitters with the same material
			for (UINT emitterIndex : batch.emitterIndices) {
				if (auto* emitter = GetEmitter(emitterIndex)) {
					auto* owner = emitter->GetOwner();
					if (!owner) continue;

					UINT globalEmitterID = owner->GetPoolHandle().emitterIDs[emitter->GetEmitterID()];
					ArgsParam billboardArgs = {
						memoryPool.GetBillboardArgs().GetBuffer(),
						owner->GetBillboardArgsOffset(globalEmitterID)
					};

					// Material already bound, so skip material binding in emitter render
					emitter->Render(billboardArgs, {});
				}
			}
		}
	}

	void ParticleEmitterManager::BuildBillboardBatchInfo(const std::vector<ParticleSystem*>& visibleSystems)
	{
		m_billboardBatchInfos.clear();

		// 1. Group by material (same as Phase 2)
		std::unordered_map<UINT, BillboardBatchInfo> batchMap;

		for (auto* system : visibleSystems) {
			// Main emitters
			for (UINT index : system->GetMainEmitterIndices()) {
				if (auto* emitter = GetEmitter(index)) {
					if (!emitter->GetModule<BillboardRenderModule>()) continue;

					auto* materialModule = emitter->GetModule<MaterialModule>();
					auto* renderModule = emitter->GetModule<RenderModule>();

					int materialIndex = materialModule ? materialModule->GetMaterialIndex(0) : 0;
					BlendMode blendMode = renderModule->blendMode;
					UINT groupKey = HashBatchKey(materialIndex, blendMode);

					auto& batch = batchMap[groupKey];
					batch.materialIndex = materialIndex;
					batch.blendMode = blendMode;
					batch.emitterIndices.push_back(index);
				}
			}

			// Active SubEmitters (Billboard only)
			for (UINT index : system->GetActiveBillboardSubEmitters()) {
				if (auto* emitter = GetEmitter(index)) {
					auto* materialModule = emitter->GetModule<MaterialModule>();
					auto* renderModule = emitter->GetModule<RenderModule>();

					int materialIndex = materialModule ? materialModule->GetMaterialIndex(0) : 0;
					BlendMode blendMode = renderModule->blendMode;
					UINT groupKey = HashBatchKey(materialIndex, blendMode);

					auto& batch = batchMap[groupKey];
					batch.materialIndex = materialIndex;
					batch.blendMode = blendMode;
					batch.emitterIndices.push_back(index);
				}
			}
		}

		// 2. Calculate particle counts and assign offsets
		UINT currentOffset = 0;
		for (auto& [key, batch] : batchMap) {
			batch.startParticleOffset = currentOffset;

			// Sum particle counts from all emitters in batch
			for (UINT emitterIndex : batch.emitterIndices) {
				if (auto* emitter = GetEmitter(emitterIndex)) {
					// Get current particle count for this emitter
					// This would need to be tracked properly - for now use placeholder
					// TODO: Implement proper particle count tracking
					batch.totalParticleCount += 100; // Placeholder
				}
			}

			currentOffset += batch.totalParticleCount;
			m_billboardBatchInfos.push_back(std::move(batch));
		}
	}

	ID3D11BlendState* ParticleEmitterManager::GetBlendStateForMode(BlendMode mode)
	{
		switch (mode) {
		case BlendMode::Additive:
			return RenderBase::graphicsCommon.accumulateBS.Get();
		case BlendMode::AlphaBlend:
			return RenderBase::graphicsCommon.alphaBS.Get();
		case BlendMode::Opaque:
			return nullptr;
		default:
			return nullptr;
		}
	}

}
