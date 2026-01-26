#include "pch.h"
#include "ClickEffectManager.h"
#include "ParticleManager.h"
#include "AppBase.h"
#include "TransformComponent.h"

namespace DE {

void ClickEffectManager::Initialize() {
    // 기본 프리셋 등록
    RegisterPreset("fire", L"Particles\\TempEffect.json");
    RegisterPreset("Firework", L"Particles\\Firework.json");
    RegisterPreset("Smoke", L"Particles\\SmokeEffect.json");
}

void ClickEffectManager::Update(float dt) {
    // 모든 이펙트 업데이트
    for (auto& effect : m_activeEffects) {
        if (effect) effect->Update(dt);
    }

    // 완료된 이펙트 제거
    CleanupFinishedEffects();
}

void ClickEffectManager::Render() {
    // 이펙트 렌더링
    for (auto& effect : m_activeEffects) {
        if (effect) effect->Render();
    }
}

void ClickEffectManager::SpawnEffectAtMousePosition(const std::wstring& presetPath) {
    // InputManager에서 마우스 NDC 좌표 가져오기
    InputManager& inputMgr = AppBase::GetInputManager();
    Vector2 mouseNDC = inputMgr.GetMouseNDC();
    
    Vector3 worldPos = ScreenToWorldPosition(mouseNDC.x, mouseNDC.y);
    SpawnEffectAtWorldPosition(presetPath, worldPos);
}

void ClickEffectManager::SpawnEffectAtWorldPosition(const std::wstring& presetPath, const Vector3& worldPos) {
    // EffectActor 생성
    std::unique_ptr<EffectActor> actor = std::make_unique<EffectActor>(L"ClickEffect");

    // Transform 컴포넌트 추가 (없으면)
    auto* transform = actor->GetComponent<TransformComponent>();
    if (!transform) {
        transform = actor->AddComponent<TransformComponent>(L"Transform");
    }
    
    // 위치 설정
    transform->SetPos(worldPos);

    // 파티클 시스템 설정
    actor->SetParticlePreset(presetPath);

    // 초기화
    actor->Initialize();

    m_activeEffects.push_back(std::move(actor));
}

void ClickEffectManager::RegisterPreset(const std::string& name, const std::wstring& presetPath) {
    m_presets[name] = presetPath;
}

void ClickEffectManager::TriggerPreset(const std::string& name) {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) {
        SpawnEffectAtMousePosition(it->second);
    }
}

void ClickEffectManager::Clear() {
    m_activeEffects.clear();
}

void ClickEffectManager::CleanupFinishedEffects() {
    // IsFinished()가 true인 이펙트 제거
    auto it = std::remove_if(m_activeEffects.begin(), m_activeEffects.end(),
        [](const std::unique_ptr<EffectActor>& effect) {
            if (!effect) return true;
            return effect->IsFinished();
        });
    
    m_activeEffects.erase(it, m_activeEffects.end());
}

Vector3 ClickEffectManager::ScreenToWorldPosition(float mouseNdcX, float mouseNdcY) {
    // 간단한 변환 (Y=0 평면에 투영)
    Vector3 worldPos(mouseNdcX * 10.0f, 0.0f, mouseNdcY * 10.0f);
    return worldPos;
}

}