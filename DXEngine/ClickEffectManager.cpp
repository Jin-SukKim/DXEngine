#include "pch.h"
#include "ClickEffectManager.h"
#include "ParticleManager.h"
#include "AppBase.h"
#include "TransformComponent.h"

namespace DE {

void ClickEffectManager::Initialize() {
    // 기본 프리셋 등록
    RegisterPreset("fire", { L"Particles\\TempEffect.json" });
}

void ClickEffectManager::Update(float dt) {
    for (auto& effect : m_activeEffects) {
        if (effect.system) effect.system->Update(dt);
    }

    auto it = std::remove_if(m_activeEffects.begin(), m_activeEffects.end(),
        [dt](ClickEffect& effect) {
            effect.lifetime -= dt;

            bool isDead = effect.lifetime <= 0.0f;

            auto& particles = effect.system->GetParticleSystem();
            for (auto& ps : particles)
                if (ps && ps->IsPlaying())
                    isDead = false;
            return isDead;
        });
    m_activeEffects.erase(it, m_activeEffects.end());
}

void ClickEffectManager::Render() {
    // ParticleManager가 자동으로 렌더링
}

void ClickEffectManager::SpawnEffectAtMousePosition(std::vector<std::wstring>& presetPath) {
    // InputManager에서 마우스 NDC 좌표 가져오기
    InputManager& inputMgr = AppBase::GetInputManager();
    Vector2 mouseNDC = inputMgr.GetMouseNDC();
    
    Vector3 worldPos = ScreenToWorldPosition(mouseNDC.x, mouseNDC.y);
    SpawnEffectAtWorldPosition(presetPath, worldPos);
}

void ClickEffectManager::SpawnEffectAtWorldPosition(std::vector<std::wstring>& presetPath, const Vector3& worldPos) {
    // EffectActor 생성
    std::unique_ptr<EffectActor> actor = std::make_unique<EffectActor>(L"ClickEffect");

    // 위치 설정
    actor->GetComponent<TransformComponent>()->SetPos(worldPos);

    // 파티클 설정
    actor->SetParticlePreset(presetPath);

    // 일회성 설정 (필요 시)
    auto& particles = actor->GetParticleSystem();
    for (auto& ps : particles) {
        if (ps) {
            ps->SetLooping(false);
            ps->SetDuration(2.0f);
        }
    }

    actor->Initialize();

    // 관리 리스트 추가
    ClickEffect effect;
    effect.system = std::move(actor);
    effect.lifetime = 2.0f;
    m_activeEffects.push_back(std::move(effect));
}

void ClickEffectManager::RegisterPreset(const std::string& name, const std::vector<std::wstring>& presetPath) {
    for (const std::wstring& path : presetPath)
        m_presets[name].push_back(L"..\\Assets\\" + path);
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