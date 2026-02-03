#pragma once
#include "ParticleSystem.h"
#include "ParticleMemoryPool.h"

namespace DE {

class ParticleManager
{
public:
    static ParticleManager& Get() {
        static ParticleManager instance;
        return instance;
    }

    void Initialize();
    void Update(const float& dt);
    void Render();
    
    void RegisterActiveSystem(ParticleSystem* system);
    void UnregisterActiveSystem(ParticleSystem* system);

    // 성공 시 포인터, 실패 시 nullptr 반환
    ParticleSystem* CreateSystem(const std::wstring& path);
    void DestroyInstance(ParticleSystem* system);

    void BindEmitterID(UINT globalSlotIndex);
    void UploadMeshConsts(UINT systemIndex, const MeshConstants& data);
    void BindMeshConsts(UINT systemIndex);

    // 통계
    UINT GetFreePageCount() const { return m_memoryPool ? m_memoryPool->GetFreePageCount() : 0; }
    UINT GetActiveSystemCount() const { return static_cast<UINT>(m_activeSystems.size()); }
    
    // 할당 가능 여부 확인
    bool CanAllocate(UINT particleCount, UINT emitterCount, UINT spawnPosCount = 0) const;

    void RenderDebugGUI();
    ParticleMemoryPool* GetMemoryPool() const { return m_memoryPool.get(); }
    const std::vector<ParticleSystem*>& GetActiveSystems() const { return m_activeSystems; }

private:
    void UploadEmitterIDs(ParticleSystem* system, const ParticleInitializer& initialData);

private:
    std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_prototypes;
    std::vector<std::unique_ptr<ParticleSystem>> m_instances;
    std::vector<ParticleSystem*> m_activeSystems;

    std::unique_ptr<ParticleMemoryPool> m_memoryPool;
};

}

