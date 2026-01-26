#include "pch.h"
#include "ClickEffectManager.h"
#include "ParticleManager.h"
#include "AppBase.h"

namespace DE {

void ClickEffectManager::Initialize() {
    // 기본 프리셋 등록
    RegisterPreset("fire", L"Particles\\TempEffect.json");
}

void ClickEffectManager::Update(float dt) {
    auto it = std::remove_if(m_activeEffects.begin(), m_activeEffects.end(),
        [dt](ClickEffect& effect) {
            effect.lifetime -= dt;

            bool shouldRemove = (effect.lifetime <= 0.0f);
            if (effect.system && effect.system->IsStopped()) {
                shouldRemove = true;
            }

            if (shouldRemove) {
                // [수정] Manager에 파괴 요청
                if (effect.system) {
                    ParticleManager::Get().DestroyInstance(effect.system);
                    effect.system = nullptr;
                }
                return true;
            }
            return false;
        });

    m_activeEffects.erase(it, m_activeEffects.end());
}

void ClickEffectManager::Render() {
    // ParticleManager가 자동으로 렌더링
}

void ClickEffectManager::SpawnEffectAtMousePosition(const std::wstring& presetPath) {
    // InputManager에서 마우스 NDC 좌표 가져오기
    InputManager& inputMgr = AppBase::GetInputManager();
    Vector2 mouseNDC = inputMgr.GetMouseNDC();
    
    Vector3 worldPos = ScreenToWorldPosition(mouseNDC.x, mouseNDC.y);
    SpawnEffectAtWorldPosition(presetPath, worldPos);
}

void ClickEffectManager::SpawnEffectAtWorldPosition(const std::wstring& presetPath, const Vector3& worldPos) {
    ParticleSystem* system = ParticleManager::Get().CreateSystem(presetPath);
    if (system) {
        MeshConstants meshConst;
        meshConst.world = Matrix::CreateTranslation(worldPos).Transpose();
        system->SetTransform(meshConst);

        system->Initialize();
        system->OnSpawn();
        system->Play();
        system->SetLooping(false); // 일회성 이펙트
        system->SetDuration(2.0f);

        ClickEffect effect;
        effect.presetPath = presetPath;
        effect.worldPosition = worldPos;
        effect.lifetime = 2.0f;
        effect.system = system;

        m_activeEffects.push_back(effect);
    }
}

void ClickEffectManager::RegisterPreset(const std::string& name, const std::wstring& path) {
    m_presets[name] = L"..\\Assets\\" + path;
}

void ClickEffectManager::TriggerPreset(const std::string& name) {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) {
        SpawnEffectAtMousePosition(it->second);
    }
}

Vector3 ClickEffectManager::ScreenToWorldPosition(float mouseNdcX, float mouseNdcY) {
    // 간단히 평면에 투영 (Y=0)
    Vector3 worldPos(mouseNdcX * 10.0f, 0.0f, mouseNdcY * 10.0f);
    return worldPos;
}

}