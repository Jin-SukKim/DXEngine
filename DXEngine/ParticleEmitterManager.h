#pragma once
#include "ParticleEmitter.h"
#include "RenderModule.h"
#include <queue>
#include <unordered_map>

namespace DE {
	class ParticleSystem;

	// Phase 2: Material batching structure
	struct BatchGroup {
		int materialIndex = -1;
		BlendMode blendMode = BlendMode::Additive;
		std::vector<UINT> emitterIndices;  // Emitters in this batch
	};

	// Phase 3: Billboard true batching structure
	struct BillboardBatchInfo {
		int materialIndex = -1;
		BlendMode blendMode = BlendMode::Additive;
		UINT startParticleOffset = 0;  // Start offset in particle buffer
		UINT totalParticleCount = 0;   // Total particles in this batch
		std::vector<UINT> emitterIndices;
	};

	class ParticleEmitterManager {
	public:
		static ParticleEmitterManager& Get() {
			static ParticleEmitterManager instance;
			return instance;
		}

		// Emitter lifecycle management
		UINT CreateEmitter(std::unique_ptr<ParticleEmitter>&& emitter, ParticleSystem* ownerSystem);
		void DestroyEmitter(UINT emitterIndex);
		ParticleEmitter* GetEmitter(UINT emitterIndex);

		// Batch rendering (Phase 2-3)
		void RenderMeshBatched(const std::vector<ParticleSystem*>& visibleSystems);
		void RenderBillboardBatched(const std::vector<ParticleSystem*>& visibleSystems);

	private:
		struct EmitterSlot {
			std::unique_ptr<ParticleEmitter> emitter;
			ParticleSystem* ownerSystem = nullptr;
			bool isActive = true;
		};

		std::vector<EmitterSlot> m_emitterSlots;
		std::queue<UINT> m_freeSlots;

		// Batch groups (rebuilt every frame)
		std::vector<BatchGroup> m_meshBatchGroups;
		std::vector<BatchGroup> m_billboardBatchGroups;

		// Phase 3: Billboard batching info
		std::vector<BillboardBatchInfo> m_billboardBatchInfos;

		// Phase 2: Batch group building
		void RebuildBatchGroups(const std::vector<ParticleSystem*>& visibleSystems);
		void GroupEmitter(ParticleEmitter* emitter, UINT emitterIndex,
			std::unordered_map<UINT, BatchGroup>& meshGroups,
			std::unordered_map<UINT, BatchGroup>& billboardGroups);

		UINT HashBatchKey(int materialIndex, BlendMode blendMode) {
			return (materialIndex << 8) | static_cast<UINT>(blendMode);
		}

		// Phase 3: Billboard batching
		void BuildBillboardBatchInfo(const std::vector<ParticleSystem*>& visibleSystems);

		ID3D11BlendState* GetBlendStateForMode(BlendMode mode);
	};
}
