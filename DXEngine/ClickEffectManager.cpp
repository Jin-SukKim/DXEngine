#include "pch.h"
#include "ClickEffectManager.h"
#include "Scene.h"
#include "AppBase.h"

namespace DE {

void ClickEffectManager::Initialize() {
    // 기본 프리셋 등록
    RegisterPreset("fire", L"Particles\\TempEffect.json");
    RegisterPreset("Firework", L"Particles\\Firework.json");
    RegisterPreset("Burst", L"Particles\\SmokeEffect.json");
}

void ClickEffectManager::SpawnEffectAtMousePosition(const std::wstring& presetPath) {
    if (!m_scene)
        return;
    // InputManager에서 마우스 NDC 좌표 가져오기
    InputManager& inputMgr = AppBase::GetInputManager();
    Vector2 mouseNDC = inputMgr.GetMouseNDC();
    
    Vector3 worldPos = ScreenToWorldPosition(mouseNDC.x, mouseNDC.y);
    SpawnEffectAtWorldPosition(presetPath, worldPos);
}

void ClickEffectManager::SpawnEffectAtWorldPosition(const std::wstring& presetPath, const Vector3& worldPos) {
    if (!m_scene)
        return;
    m_scene->SpawnEffect(L"ClickEffect", presetPath, worldPos);
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

Vector3 ClickEffectManager::ScreenToWorldPosition(float mouseNdcX, float mouseNdcY) {
    // 간단히 평면에 투영 (Y=0)
    Vector3 worldPos(mouseNdcX * 10.0f, 0.0f, mouseNdcY * 10.0f);
    return worldPos;
}

}