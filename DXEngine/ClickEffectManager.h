#pragma once
#include "EffectActor.h"

namespace DE {

struct ClickEffect {
    std::vector<std::wstring> presetPath;
    Vector3 worldPosition;
    float lifetime;
    std::unique_ptr<EffectActor> system;
};

class ClickEffectManager {
public:
    static ClickEffectManager& Get() {
        static ClickEffectManager instance;
        return instance;
    }

    void Initialize();
    void Update(float dt);
    void Render();

    // 마우스 위치에 이펙트 생성
    void SpawnEffectAtMousePosition(const std::wstring& presetPath);
    
    // 월드 좌표에 이펙트 생성
    void SpawnEffectAtWorldPosition(const std::wstring& presetPath, const Vector3& worldPos);

    // 여러 프리셋 등록
    void RegisterPreset(const std::string& name, const std::wstring& path);
    void TriggerPreset(const std::string& name);

private:
    Vector3 ScreenToWorldPosition(float mouseNdcX, float mouseNdcY);

private:
    std::vector<ClickEffect> m_activeEffects;
    std::unordered_map<std::string, std::wstring> m_presets;
};

}