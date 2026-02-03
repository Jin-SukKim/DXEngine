#include "pch.h"
#include "ParticleManager.h"
#include "ParticleLoader.h"
#include "RenderBase.h"

#include <imgui.h>

namespace DE {
    void ParticleManager::Initialize()
    {
        m_memoryPool = std::make_unique<ParticleMemoryPool>();
        m_memoryPool->Initialize(1000000, 10000, 1000);
    }

    void ParticleManager::Update(const float& dt)
    {
        m_memoryPool->ClearWriteCount();
        m_memoryPool->BindCompute();

        for (auto* system : m_activeSystems) {
            if (system && system->GetPageHandle().IsActive()) {
                std::vector<ParticleFrameConsts> fsConsts(system->GetMaxEmitterCount());
                system->PreUpdate(dt, fsConsts);
                m_memoryPool->UploadFrameConsts(system->GetPageHandle().emitterIDs, fsConsts);
            }
        }

        m_memoryPool->UpdateArgs();

        for (auto* system : m_activeSystems) {
            if (system && system->GetPageHandle().IsActive()) {
                m_memoryPool->BindMeshConsts(system->GetPageHandle().systemSlot);
                system->Update(dt);
            }
        }

        m_memoryPool->UnbindCompute();

        if (m_memoryPool)
            m_memoryPool->SwapBuffer();
    }

    void ParticleManager::Render()
    {
        if (m_activeSystems.empty()) return;

        m_memoryPool->BindRender();
        for (auto* system : m_activeSystems) {
            if (system && system->GetPageHandle().IsActive()) {
                m_memoryPool->BindMeshConsts(system->GetPageHandle().systemSlot);
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

    bool ParticleManager::CanAllocate(UINT particleCount, UINT emitterCount, UINT spawnPosCount) const
    {
        UINT neededPages = (particleCount + PAGE_SIZE - 1) / PAGE_SIZE;
        UINT neededSpawnPages = (spawnPosCount + PAGE_SIZE - 1) / PAGE_SIZE;
        return m_memoryPool->GetFreePageCount() >= neededPages &&
               m_memoryPool->GetFreeSpawnPosPageCount() >= neededSpawnPages;
    }

    ParticleSystem* ParticleManager::CreateSystem(const std::wstring& path)
    {
        auto prototypeIt = m_prototypes.find(path);
        ParticleSystem* prototype = nullptr;

        if (prototypeIt != m_prototypes.end()) {
            prototype = prototypeIt->second.get();
        }
        else {
            auto newSystem = ParticleLoader::Load<ParticleSystem>(path);
            if (!newSystem) {
                return nullptr;
            }
            newSystem->Initialize();
            prototype = newSystem.get();
            m_prototypes[path] = std::move(newSystem);
        }

        auto cloned = std::make_unique<ParticleSystem>(*prototype);

        ParticleInitializer initialData;
        cloned->InitializeCPU(initialData);

        UINT spawnPosCount = static_cast<UINT>(initialData.spawnPositions.size());
        UINT particleCount = cloned->GetTotalParticleCount();
        UINT emitterCount = cloned->GetMaxEmitterCount();
        
        PageHandle handle = m_memoryPool->Allocate(particleCount, emitterCount, spawnPosCount);
        
        // 할당 실패 시 nullptr 반환
        if (!handle.IsActive()) {
            std::cout << "Failed to create ParticleSystem: Out of pages" << std::endl;
            return nullptr;
        }
        
        cloned->SetPageHandle(handle);

        // SpawnPositions 업로드
        if (!initialData.spawnPositions.empty() && !handle.spawnPosPageIndices.empty()) {
            m_memoryPool->UploadSpawnPositions(
                handle.spawnPosPageIndices, 
                initialData.spawnPositions
            );
        }

        cloned->InitializeGPU(initialData, 
            m_memoryPool->GetDispatchArgs(),
            m_memoryPool->GetBillboardArgs(),
            m_memoryPool->GetMeshArgs());

        UploadEmitterIDs(cloned.get(), initialData);
            
        m_memoryPool->UploadConsts(handle.emitterIDs, initialData.consts);
        m_memoryPool->UploadFrameConsts(handle.emitterIDs, initialData.frameConsts);

        cloned->Initialize(initialData);
        cloned->OnSpawn();

        auto* clonedPtr = cloned.get();
        m_instances.push_back(std::move(cloned));

        RegisterActiveSystem(clonedPtr);

        return clonedPtr;
    }
    
    void ParticleManager::DestroyInstance(ParticleSystem* system)
    {
        if (!system) return;

        // 1. active 목록에서 제거
        UnregisterActiveSystem(system);

        // 2. Pool에서 메모리 해제
        const PageHandle& handle = system->GetPageHandle();
        if (handle.IsActive()) {
            m_memoryPool->Free(handle);
        }

        // 3. instance 목록에서 제거
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

    void ParticleManager::UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData)
    {
        const PageHandle& handle = system->GetPageHandle();
        
        for (size_t i = 0; i < initialData.emitterIDs.size(); ++i) {
            EmitterIDPaged eID;
            eID.emitterID = handle.emitterIDs[i];
            eID.pageTableStart = handle.pageIndices.empty() ? 0 : handle.pageIndices[0];
            eID.pageCount = handle.pageCount;
            eID.localParticleMax = handle.totalCapacity / std::max(1u, handle.emitterCount);
            
            if (!handle.spawnPosPageIndices.empty()) {
                eID.spawnPosPageTableStart = handle.spawnPosPageIndices[0];
                eID.spawnPosPageCount = handle.spawnPosPageCount;
            }
            else {
                eID.spawnPosPageTableStart = INVALID_PAGE;
                eID.spawnPosPageCount = 0;
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

    void ParticleManager::RenderDebugGUI()
    {
        if (!m_memoryPool) return;
        if (!ImGui::Begin("Memory Pool Monitor")) {
            ImGui::End();
            return;
        }

        auto stats = m_memoryPool->GetStats();

        // 용량 정보
        ImGui::Text("Max Particles: %u", stats.maxParticles);
        ImGui::Text("Allocated:     %u", stats.allocatedParticleCapacity);
        
        ImGui::Separator();

        // 슬롯 정보
        ImGui::Text("Emitters: %u / %u", stats.usedEmitterSlots, stats.totalEmitterSlots);
        ImGui::Text("Systems:  %u / %u", stats.usedSystemSlots, stats.totalSystemSlots);
        ImGui::Text("Active:   %u", static_cast<UINT>(m_activeSystems.size()));
        
        ImGui::Separator();
        
        // 페이지 정보
        ImGui::Text("Pages:    %u / %u", stats.usedPages, stats.totalPages);
        ImGui::Text("SpawnPos: %u / %u", stats.usedSpawnPosPages, stats.totalSpawnPosPages);

        ImGui::Separator();

        // 페이지 맵
        auto DrawPageMap = [](const char* label, const std::vector<bool>& pageMap) {
            if (ImGui::CollapsingHeader(label)) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 startPos = ImGui::GetCursorScreenPos();
                const int cols = 64;
                const float size = 6.0f;
                const float gap = 1.0f;

                for (size_t i = 0; i < pageMap.size(); ++i) {
                    ImVec2 p1(startPos.x + (i % cols) * (size + gap), startPos.y + (i / cols) * (size + gap));
                    ImVec2 p2(p1.x + size, p1.y + size);
                    drawList->AddRectFilled(p1, p2, pageMap[i] ? IM_COL32(200, 80, 80, 255) : IM_COL32(80, 180, 80, 255));
                }
                ImGui::Dummy(ImVec2(cols * (size + gap), ((pageMap.size() + cols - 1) / cols) * (size + gap)));
            }
        };

        DrawPageMap("Page Map", m_memoryPool->GetPageUsageMap());
        DrawPageMap("SpawnPos Map", m_memoryPool->GetSpawnPosPageUsageMap());

        // 활성 시스템 목록
        if (ImGui::CollapsingHeader("Active Systems")) {
            for (size_t i = 0; i < m_activeSystems.size(); ++i) {
                const auto& h = m_activeSystems[i]->GetPageHandle();
                ImGui::Text("[%zu] Slot:%u Pages:%u Emitters:%u Cap:%u", 
                    i, h.systemSlot, h.pageCount, h.emitterCount, h.totalCapacity);
            }
        }

        ImGui::End();
    }
}