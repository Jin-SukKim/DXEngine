#pragma once
#include "ParticleSystem.h"

namespace DE {

class ParticlePool {
public:
    static ParticlePool& Get() {
        static ParticlePool instance;
        return instance;
    }

    void Initialize(const std::wstring& presetPath, int poolSize);
    ParticleSystem* Acquire(const std::wstring& presetPath);
    void Release(ParticleSystem* system);

private:
    std::unordered_map<std::wstring, std::vector<ParticleSystem*>> m_pools;
    std::unordered_map<std::wstring, std::unique_ptr<ParticleSystem>> m_prototypes;
};

}