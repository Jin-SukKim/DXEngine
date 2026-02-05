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
		// [1] 자동 집계용 프로파일러
		// =================================================================================
		struct CreateProfiler {
			ParticleManager* owner;
			float tProto = 0.f;
			float tClone = 0.f;

			// [변경] tAlloc을 두 개로 분할
			float tSetup = 0.f;      // InitializeCPU (데이터 구성)
			float tPoolAlloc = 0.f;  // RequestAllocation (메모리 탐색)

			float tGPU = 0.f;
			float tFinal = 0.f;

			float tCacheSearch = 0.f;
			float tFileLoad = 0.f;
			bool isCacheHit = false;
			bool isSuccess = false;
			float tSetupMain = 0.f;
			float tSetupSub = 0.f;

			CreateProfiler(ParticleManager* m) : owner(m) {}

			~CreateProfiler() {
				// 전체 시간 합산 (tAlloc = tSetup + tPoolAlloc)
				float tAllocTotal = tSetup + tPoolAlloc;
				float total = tProto + tClone + tAllocTotal + tGPU + tFinal;

				if (isSuccess) {
					owner->m_createCount++;
					owner->m_totalCreateTime += total;
					owner->m_avgCreateTime = owner->m_totalCreateTime / owner->m_createCount;

					// 1. Prototype
					owner->m_totalPrototypeTime += tProto;
					owner->m_avgPrototypeTime = owner->m_totalPrototypeTime / owner->m_createCount;

					// 2. Clone
					owner->m_totalCloneTime += tClone;
					owner->m_avgCloneTime = owner->m_totalCloneTime / owner->m_createCount;

					// 3. CPU Alloc (합계 저장)
					owner->m_totalAllocTime += tAllocTotal;
					owner->m_avgAllocTime = owner->m_totalAllocTime / owner->m_createCount;

					// ★ [3-A] Setup 상세 통계 갱신
					owner->m_totalSetupTime += tSetup;
					owner->m_avgSetupTime = owner->m_totalSetupTime / owner->m_createCount;

					owner->m_totalInitMainTime += tSetupMain;
					owner->m_avgInitMainTime = owner->m_totalInitMainTime / owner->m_createCount;

					owner->m_totalInitSubTime += tSetupSub;
					owner->m_avgInitSubTime = owner->m_totalInitSubTime / owner->m_createCount;

					// ★ 3-B. Pool Alloc (신규)
					owner->m_totalPoolAllocTime += tPoolAlloc;
					owner->m_avgPoolAllocTime = owner->m_totalPoolAllocTime / owner->m_createCount;

					// 4. GPU Upload
					owner->m_totalGpuTime += tGPU;
					owner->m_avgGpuTime = owner->m_totalGpuTime / owner->m_createCount;

					// 5. Finalize
					float initTotal = tAllocTotal + tGPU + tFinal;
					owner->m_totalInitalizeTime += initTotal;
					owner->m_avgInitializeTime = owner->m_totalInitalizeTime / owner->m_createCount;

					// Cache 통계
					if (isCacheHit) {
						owner->m_cacheHitCount++;
						owner->m_totalCacheSearchTime += tCacheSearch;
						owner->m_avgCacheSearchTime = owner->m_totalCacheSearchTime / owner->m_cacheHitCount;
					}
					else {
						owner->m_cacheMissCount++;
						owner->m_totalFileLoadTime += tFileLoad;
						owner->m_avgFileLoadTime = owner->m_totalFileLoadTime / owner->m_cacheMissCount;
					}
				}
				else {
					owner->m_failCount++;
					owner->m_totalFailTime += total;
				}
			}
		};

		CreateProfiler profiler(this);

		// =================================================================================
		// [2] 로직 시작
		// =================================================================================

		ParticleSystem* prototype = nullptr;
		std::unique_ptr<ParticleSystem> cloned = nullptr;

		// 1. Prototype Load
		{
			ScopedTimer protoTimer([&](float t) { profiler.tProto = t; });

			{
				ScopedTimer cacheTimer([&](float t) { profiler.tCacheSearch = t; });
				auto prototypeIt = m_prototypes.find(path);
				if (prototypeIt != m_prototypes.end()) {
					prototype = prototypeIt->second.get();
					profiler.isCacheHit = true;
				}
			}

			if (!profiler.isCacheHit) {
				ScopedTimer loadTimer([&](float t) { profiler.tFileLoad = t; });
				auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
				if (!newSystem) return nullptr;
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

		// 데이터 준비 (이 변수들은 스코프 밖에서 유지되어야 함)
		ParticleInitializer initialData;
		PoolHandle handle;

		// 3-1. CPU Init & Allocation (구간 분리)
		{
			// [A] InitializeCPU 측정
			{
				ScopedTimer timer([&](float t) { profiler.tSetup = t; });

				// ★ 여기서 반환값(stats)을 받아서 프로파일러에 기록
				CPUInitStats stats = cloned->InitializeCPU(initialData);
				profiler.tSetupMain = stats.mainEmitterTime;
				profiler.tSetupSub = stats.subEmitterTime;
			}

			// [B] RequestAllocation 측정
			{
				ScopedTimer timer([&](float t) { profiler.tPoolAlloc = t; });

				UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
				UINT particleCount = cloned->GetTotalParticleCount();
				UINT emitterCount = cloned->GetMaxEmitterCount();

				handle = RequestAllocation(particleCount, emitterCount, spawnPosCount);
			}
			if (!handle.IsActive()) {
				return nullptr;
			}

			cloned->SetPoolHandle(handle);
		}

		// 3-2. GPU Upload (구간 분리)
		{
			ScopedTimer timer([&](float t) { profiler.tGPU = t; });

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
		}

		// 3-3. Finalize
		{
			ScopedTimer timer([&](float t) { profiler.tFinal = t; });

			cloned->Initialize(initialData);
			cloned->OnSpawn();

			m_instances.push_back(std::move(cloned));
			RegisterActiveSystem(clonedPtr);
		}

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

				// ★ Cache 통계 추가
				if (m_createCount > 0) {
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Cache Statistics");
					ImGui::Text("  Cache Hit : %d times", m_cacheHitCount);
					ImGui::Text("  Cache Miss: %d times", m_cacheMissCount);

					if (m_cacheHitCount + m_cacheMissCount > 0) {
						float hitRate = (float)m_cacheHitCount / (m_cacheHitCount + m_cacheMissCount) * 100.0f;
						ImVec4 hitColor = (hitRate > 80.0f) ? ImVec4(0, 1, 0, 1) :
							(hitRate > 50.0f) ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1);
						ImGui::TextColored(hitColor, "  Hit Rate  : %.1f%%", hitRate);
					}
				}

				ImGui::Separator();

				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Total Create Time: %.3f ms", m_totalCreateTime);
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Avg Create Time  : %.3f ms", m_avgCreateTime);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Text("Detailed Breakdown (Avg per Call)");
				ImGui::Separator();

				// 2. 구간별 상세 시간
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

					// ★ Prototype 세부 정보 (들여쓰기)
					if (m_cacheHitCount > 0) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("   - Cache Search");
						ImGui::TableSetColumnIndex(1); ImGui::Text("-");
						ImGui::TableSetColumnIndex(2);
						ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%.4f", m_avgCacheSearchTime);
					}

					if (m_cacheMissCount > 0) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0); ImGui::Text("   - File Load");
						ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalFileLoadTime);
						ImGui::TableSetColumnIndex(2);
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%.3f", m_avgFileLoadTime);
					}

					// Row 2: Instance Clone
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("2. Clone");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalCloneTime);
					ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", m_avgCloneTime);

					// 3. Init & Alloc (CPU Total)
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("3. Init (CPU Total)");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalAllocTime);
					ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.3f", m_avgAllocTime);

					// ★ 3-A. Setup (InitializeCPU)
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("   - InitCPU (Logic)");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalSetupTime);
					ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "%.3f", m_avgSetupTime);
					// ★★★ [상세] Main Emitter
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("     > Main Emitters");
					ImGui::TableSetColumnIndex(1); ImGui::Text("-");
					ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%.4f", m_avgInitMainTime);

					// ★★★ [상세] Sub Emitter (병목 지점)
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("     > Sub Emitters");
					ImGui::TableSetColumnIndex(1); ImGui::Text("-");
					ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%.4f", m_avgInitSubTime);
					// ★ 3-B. Pool (RequestAllocation)
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("   - Pool Alloc (Search)");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalPoolAllocTime);
					ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "%.3f", m_avgPoolAllocTime);

					// 4. Init (GPU Upload)
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::Text("4. Init (GPU Upload)");
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", m_totalGpuTime);
					ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%.3f", m_avgGpuTime);
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

				// 3. 리셋 버튼
				if (ImGui::Button("Reset Metrics", ImVec2(-1, 0))) {
					m_createCount = 0;
					m_failCount = 0;

					m_totalCreateTime = 0.0f;
					m_avgCreateTime = 0.0f;
					m_totalFailTime = 0.0f;

					m_totalPrototypeTime = 0.0f;
					m_avgPrototypeTime = 0.0f;

					m_totalCloneTime = 0.0f;
					m_avgCloneTime = 0.0f;

					m_totalInitalizeTime = 0.0f;
					m_avgInitializeTime = 0.0f;

					// ★ Cache 통계 초기화
					m_cacheHitCount = 0;
					m_cacheMissCount = 0;
					m_totalCacheSearchTime = 0.0f;
					m_avgCacheSearchTime = 0.0f;
					m_totalFileLoadTime = 0.0f;
					m_avgFileLoadTime = 0.0f;

					m_totalAllocTime = 0.0f; m_avgAllocTime = 0.0f;
					m_totalGpuTime = 0.0f; m_avgGpuTime = 0.0f;
					m_totalInitalizeTime = 0.0f; m_avgInitializeTime = 0.0f;
					m_totalInitMainTime = 0.0f; m_avgInitMainTime = 0.0f;
					m_totalInitSubTime = 0.0f; m_avgInitSubTime = 0.0f;
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