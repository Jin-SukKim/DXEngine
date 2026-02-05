#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

namespace DE {
	void ParticleManager::Initialize()
	{
		m_memoryPool = std::make_unique<ParticleMemoryPool>();
		m_memoryPool->Initialize(10000000, 10000, 10000);
	}

	void ParticleManager::Update(const float& dt)
	{
		constexpr float DEFRAG_THRESHOLD = 0.2f;
		if (m_memoryPool->GetFragmentationRatio() >= DEFRAG_THRESHOLD) {
			// ★ 재구성 시간 측정
			auto start = std::chrono::high_resolution_clock::now();
			Defragment();
			
			auto end = std::chrono::high_resolution_clock::now();
			float elapsed = std::chrono::duration<float, std::milli>(end - start).count();
			
			m_rebuildCount++;
			m_totalRebuildTime += elapsed;
			m_avgRebuildTime = m_totalRebuildTime / m_rebuildCount;
		}
		
		m_memoryPool->ClearWriteCount();
		m_memoryPool->BindCompute();

		for (auto* system : m_activeSystems) {
			if (system) {
				std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
				system->PreUpdate(dt, fsConsts);
				m_memoryPool->UploadFrameConsts(system->GetPoolHandle().emitterIDs, fsConsts);
			}
		}

		m_memoryPool->UpdateArgs();

		for (auto* system : m_activeSystems) {
			if (system) {
				m_memoryPool->BindMeshConsts(system->GetPoolHandle().systemSlot);
				system->Update(dt);
			}
		}

		m_memoryPool->UnbindCompute();

		// Compute 후, SwapBuffer 전에 Read 오프셋 동기화
		if (m_needsSyncReadOffset) {
			SyncReadOffsets();
			m_needsSyncReadOffset = false;
		}

		if (m_memoryPool)
			m_memoryPool->SwapBuffer();
	}

	void ParticleManager::Render()
	{
		if (m_activeSystems.empty()) return;

		m_memoryPool->BindRender();
		for (auto* system : m_activeSystems) {
			if (system) {
				m_memoryPool->BindMeshConsts(system->GetPoolHandle().systemSlot);
				system->Render();
			}
		}
		m_memoryPool->UnbindRender();
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.basic.solidPSO);

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
		// =================================================================================
		// [1] 자동 집계용 프로파일러 (함수 스코프 전체를 관장)
		// =================================================================================
		struct CreateProfiler {
			ParticleManager* owner;
			float tProto = 0.f;
			float tClone = 0.f;
			float tInit = 0.f;
			bool isSuccess = false;

			// 생성자
			CreateProfiler(ParticleManager* m) : owner(m) {}

			// 소멸자: 함수가 리턴될 때 무조건 호출됨
			~CreateProfiler() {
				float total = tProto + tClone + tInit;

				if (isSuccess) {
					// [성공 시 집계]
					owner->m_createCount++;
					owner->m_totalCreateTime += total;
					owner->m_avgCreateTime = owner->m_totalCreateTime / owner->m_createCount;

					// 세부 구간 평균 갱신
					owner->m_totalPrototypeTime += tProto;
					owner->m_avgPrototypeTime = owner->m_totalPrototypeTime / owner->m_createCount;

					owner->m_totalCloneTime += tClone;
					owner->m_avgCloneTime = owner->m_totalCloneTime / owner->m_createCount;

					owner->m_totalInitalizeTime += tInit;
					owner->m_avgInitializeTime = owner->m_totalInitalizeTime / owner->m_createCount;
				}
				else {
					// [실패 시 집계]
					owner->m_failCount++;
					owner->m_totalFailTime += total;
					// 실패는 평균 시간에 섞지 않고 따로 보는 게 일반적입니다.
				}
			}
		};

		// 프로파일러 선언 (이제부터 이 함수가 끝나면 무조건 ~CreateProfiler가 실행됨)
		CreateProfiler profiler(this);

		// =================================================================================
		// [2] 로직 시작
		// =================================================================================

		ParticleSystem* prototype = nullptr;
		std::unique_ptr<ParticleSystem> cloned = nullptr;

		// 1. Prototype Load
		{
			// 람다에서 profiler.tProto에 바로 값을 넣습니다.
			ScopedTimer timer([&](float t) { profiler.tProto = t; });

			auto prototypeIt = m_prototypes.find(path);
			if (prototypeIt != m_prototypes.end()) {
				prototype = prototypeIt->second.get();
			}
			else {
				auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
				if (!newSystem) {
					return nullptr; // ★ 여기서 리턴해도 profiler 소멸자가 호출되어 "실패 통계"에 기록됨
				}
				newSystem->Initialize();
				prototype = newSystem.get();
				m_prototypes[path] = std::move(newSystem);
			}
		}

		// 2. Clone
		{
			ScopedTimer timer([&](float t) { profiler.tClone = t; });
			cloned = std::make_unique<ParticleSystem>(*prototype);
		}

		ParticleSystem* clonedPtr = cloned.get();

		// 3. Initialize & Allocation
		{
			ScopedTimer timer([&](float t) { profiler.tInit = t; });

			ParticleInitializer initialData;
			cloned->InitializeCPU(initialData);

			UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
			UINT particleCount = cloned->GetTotalParticleCount();
			UINT emitterCount = cloned->GetMaxEmitterCount();

			PoolHandle handle = RequestAllocation(particleCount, emitterCount, spawnPosCount);

			// ★ 할당 실패
			if (!handle.IsActive()) {
				return nullptr; // 여기서 리턴하면 tProto + tClone + tInit(여기까지 측정된 값)이 실패 시간에 누적됨
			}

			cloned->SetPoolHandle(handle);

			if (!initialData.spawnPositions.empty() && handle.spawnPosOffset != UINT_MAX) {
				m_memoryPool->UploadSpawnPositions(handle.spawnPosOffset, initialData.spawnPositions);
			}

			cloned->InitializeGPU(initialData,
				m_memoryPool->GetDispatchArgs(),
				m_memoryPool->GetBillboardArgs(),
				m_memoryPool->GetMeshArgs());

			UploadEmitterIDs(cloned.get(), cloned->GetInitialData());
			m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
			m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);

			cloned->Initialize(initialData);
			cloned->OnSpawn();

			m_instances.push_back(std::move(cloned));
			RegisterActiveSystem(clonedPtr);
		}

		// ★ 모든 과정 통과! 성공 플래그를 켭니다.
		profiler.isSuccess = true;

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

	void ParticleManager::BindEmitterID(UINT globalSlotIndex)
	{
		m_memoryPool->BindEmitterID(globalSlotIndex);
	}

	void ParticleManager::RenderMemoryPoolGUI()
	{
		if (!m_memoryPool) return;

		if (ImGui::Begin("Particle Memory Pool Status"))
		{
			if (ImGui::CollapsingHeader("Stats Overview", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				UINT usedBlocks = m_memoryPool->GetUsedBlockCount();
				float usage = (totalBlocks > 0) ? (float)usedBlocks / totalBlocks : 0.0f;

				ImGui::Text("Particle Blocks: %d / %d (Size: %d)", usedBlocks, totalBlocks, m_memoryPool->GetBlockSize());
				ImGui::ProgressBar(usage, ImVec2(-1, 0), "Particle Memory Usage");

				ImGui::Text("Max Total Particles Count : %d", usedBlocks * m_memoryPool->GetBlockSize());

				UINT totalEmitters = m_memoryPool->GetTotalEmitterSlots();
				UINT usedEmitters = m_memoryPool->GetUsedEmitterSlots();
				float emitterUsage = (totalEmitters > 0) ? (float)usedEmitters / totalEmitters : 0.0f;

				ImGui::Text("Emitter Slots: %d / %d", usedEmitters, totalEmitters);
				ImGui::ProgressBar(emitterUsage, ImVec2(-1, 0), "Emitter Slot Usage");

				UINT totalSystems = m_memoryPool->GetTotalSystemSlots();
				UINT usedSystems = m_memoryPool->GetUsedSystemSlots();
				ImGui::Text("Active Systems: %d / %d", usedSystems, totalSystems);
				
				// ★ 단편화 비율
				float fragRatio = m_memoryPool->GetFragmentationRatio();
				ImVec4 fragColor = (fragRatio < 0.2f) ? ImVec4(0,1,0,1) : 
								(fragRatio < 0.5f) ? ImVec4(1,1,0,1) : ImVec4(1,0,0,1);
				ImGui::TextColored(fragColor, "Fragmentation: %.1f%%", fragRatio * 100.0f);
			}
			
			// ★ 성능 메트릭 섹션 추가
			if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Rebuild Count: %d", m_rebuildCount);
				ImGui::Text("Avg Rebuild Time: %.3f ms", m_avgRebuildTime);
				ImGui::Text("Total Rebuild Time: %.3f ms", m_totalRebuildTime);
				
				if (ImGui::Button("Reset Metrics")) {
					m_rebuildCount = 0;
					m_avgRebuildTime = 0.0f;
					m_totalRebuildTime = 0.0f;
				}
			}
			
			// 2. 블록 시각화 (Grid Visualizer)
			// 녹색: 사용 중, 회색: 빈 공간
			if (ImGui::CollapsingHeader("Block Map (Visualizer)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UINT totalBlocks = m_memoryPool->GetTotalBlockCount();
				std::vector<std::string> blockOwners(totalBlocks, "Free");

				const auto& table = m_memoryPool->GetParticleBlockTable();
				int columns = 32; // 한 줄에 보여줄 블록 개수
				float cellSize = 10.0f;
				float spacing = 2.0f;

				ImVec2 p = ImGui::GetCursorScreenPos();
				float startX = p.x;
				float startY = p.y;

				for (size_t i = 0; i < table.size(); ++i)
				{
					float x = startX + (i % columns) * (cellSize + spacing);
					float y = startY + (i / columns) * (cellSize + spacing);

					ImU32 color = table[i] ? IM_COL32(50, 205, 50, 255) : IM_COL32(50, 50, 50, 255); // Green vs Gray

					ImGui::GetWindowDrawList()->AddRectFilled(
						ImVec2(x, y),
						ImVec2(x + cellSize, y + cellSize),
						color
					);

					// 마우스 오버 시 블록 번호 툴팁
					if (ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + cellSize, y + cellSize)))
					{
						ImGui::BeginTooltip();
						// [변경] 소유자 이름까지 출력
						ImGui::Text("Block ID: %llu", i);
						ImGui::TextColored(table[i] ? ImVec4(0, 1, 0, 1) : ImVec4(0.5, 0.5, 0.5, 1),
							"Status: %s", table[i] ? "Used" : "Free");

						if (table[i]) {
							ImGui::Text("Owner: %s", blockOwners[i].c_str());
						}
						ImGui::EndTooltip();
					}
				}

				// 그리드 높이만큼 커서 이동
				float totalHeight = ((table.size() + columns - 1) / columns) * (cellSize + spacing);
				ImGui::Dummy(ImVec2(0, totalHeight));
			}
		}
		ImGui::End();

		if (ImGui::Begin("Profile"))
		{
			if (ImGui::CollapsingHeader("Particle System Creation Metrics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 1. 전체 요약 정보
				ImGui::Text("Create Count: %d", m_createCount);
				ImGui::Separator();

				// (주의: 기존 코드에서 Total과 Avg 변수가 반대로 매핑되어 있어 수정했습니다)
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Total Create Time: %.3f ms", m_totalCreateTime);
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Avg Create Time  : %.3f ms", m_avgCreateTime);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Text("Detailed Breakdown (Avg per Call)");
				ImGui::Separator();

				// 2. 구간별 상세 시간 (Table 형식이 보기 좋습니다)
				if (ImGui::BeginTable("DetailedMetrics", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					ImGui::TableSetupColumn("Step");
					ImGui::TableSetupColumn("Total (ms)");
					ImGui::TableSetupColumn("Average (ms)");
					ImGui::TableHeadersRow();

					// Row 1: Prototype Load
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("1. Prototype");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalPrototypeTime);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", m_avgPrototypeTime);

					// Row 2: Instance Clone
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("2. Clone");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalCloneTime);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", m_avgCloneTime);

					// Row 3: Initialize & Upload
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("3. Init & GPU");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalInitalizeTime);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", m_avgInitializeTime);

					ImGui::EndTable();
				}

				ImGui::Spacing();
				if (m_failCount > 0) {
					ImGui::TextColored(ImVec4(1.f, 0.2f, 0.2f, 1.f), "Fail Count  : %d", m_failCount);
					ImGui::TextColored(ImVec4(1.f, 0.2f, 0.2f, 1.f), "Total Wasted: %.3f ms", m_totalFailTime);
				}
				else {
					ImGui::Text("Fail Count  : 0");
				}

				ImGui::Separator();

				// 3. 리셋 버튼 (모든 변수 초기화)
				if (ImGui::Button("Reset Metrics", ImVec2(-1, 0))) {
					m_createCount = 0;

					m_totalCreateTime = 0.0f;
					m_avgCreateTime = 0.0f;

					m_totalPrototypeTime = 0.0f;
					m_avgPrototypeTime = 0.0f;

					m_totalCloneTime = 0.0f;
					m_avgCloneTime = 0.0f;

					m_totalInitalizeTime = 0.0f;
					m_avgInitializeTime = 0.0f;
					m_failCount = 0;       // 리셋
					m_totalFailTime = 0.f; // 리셋
				}
			}
		}
		ImGui::End();
	}

	PoolHandle ParticleManager::RequestAllocation(UINT particleCount, UINT emitterCount, UINT spawnPosCount)
	{
	    PoolHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);

	    // 할당 실패 시 다음 프레임 Defragment 예약 (즉시 실행 X)
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
			eID.readParticleOffset = handle.particleOffset + initialData.emitterIDs[i].readParticleOffset;
			eID.writeParticleOffset = handle.particleOffset + initialData.emitterIDs[i].writeParticleOffset;
			
			// spawnPos를 사용하는 emitter만 오프셋 적용
			if (handle.spawnPosOffset != UINT_MAX && eID.spawnPosOffset != UINT_MAX) {
				eID.spawnPosOffset += handle.spawnPosOffset;
			}
			
			m_memoryPool->UpdateEmitterID(handle.emitterIDs[i], eID);
		}
	}

	void ParticleManager::UploadMeshConsts(UINT systemSlot, const MeshConstants& data)
	{
		ParticleMeshConsts pmConsts;
		pmConsts.world = data.world;
		pmConsts.worldIT = data.worldIT;
		pmConsts.vertexCount = 0;
		pmConsts.indexCount = 0;
		
		m_memoryPool->UploadMeshConsts(systemSlot, pmConsts);
	}

	void ParticleManager::BindMeshConsts(UINT systemSlot)
	{
		m_memoryPool->BindMeshConsts(systemSlot);
	}

	void ParticleManager::Defragment()
	{
		if (m_activeSystems.empty()) return;

		std::vector<PoolHandle> activeHandles;
		for (auto* system : m_activeSystems) {
			if (system && system->GetPoolHandle().IsActive()) {
				activeHandles.push_back(system->GetPoolHandle());
			}
		}

		std::vector<UINT> newOffsets = m_memoryPool->Defragment(activeHandles);

		size_t idx = 0;
		for (auto* system : m_activeSystems) {
			if (!system || !system->GetPoolHandle().IsActive()) continue;
			
			PoolHandle& handle = system->GetPoolHandle();
			UINT newOffset = newOffsets[idx++];
			
			if (handle.particleOffset != newOffset) {
				RecalculateEmitterOffsets(system, newOffset);
			}
		}

		m_needsSyncReadOffset = true;  // 이번 프레임 Compute 후 동기화
	}

	void ParticleManager::RecalculateEmitterOffsets(ParticleSystem* system, UINT newParticleOffset)
	{
		PoolHandle& handle = system->GetPoolHandle();
		const ParticleInitializer& initialData = system->GetInitialData();

		for (size_t i = 0; i < handle.emitterIDs.size(); ++i) {
            // writeParticleOffset만 새 위치로 갱신
            UINT globalEmitterID = handle.emitterIDs[i];
            UINT localOffset = initialData.emitterIDs[i].writeParticleOffset;
            
            m_memoryPool->UpdateWriteOffset(globalEmitterID, newParticleOffset + localOffset);
        }

        handle.particleOffset = newParticleOffset;
	}

	void ParticleManager::SyncReadOffsets()
	{
		for (auto* system : m_activeSystems) {
			if (!system || !system->GetPoolHandle().IsActive()) continue;
			
			const PoolHandle& handle = system->GetPoolHandle();
			for (UINT emitterID : handle.emitterIDs) {
				m_memoryPool->SyncReadOffset(emitterID);
			}
		}
	}

	void ParticleManager::ResetMetrics()
	{
		m_rebuildCount = 0;
		m_avgRebuildTime = 0.0f;
		m_totalRebuildTime = 0.0f;
	}
}