#pragma once
#include "EffectActor.h"

namespace DE {

class Scene;

class ClickEffectManager {
public:
    static ClickEffectManager& Get() {
        static ClickEffectManager instance;
        return instance;
    }

    void Initialize();

    // 마우스 위치에 이펙트 생성
    void SpawnEffectAtMousePosition(const std::wstring& presetPath);
    
    // 월드 좌표에 이펙트 생성
    void SpawnEffectAtWorldPosition(const std::wstring& presetPath, const Vector3& worldPos);

    // 여러 프리셋 등록
    void RegisterPreset(const std::string& name, const std::wstring& path);
    void TriggerPreset(const std::string& name);

    void SetScene(Scene* scene) { m_scene = scene; }
private:
    ClickEffectManager() = default;
    Vector3 ScreenToWorldPosition(float mouseNdcX, float mouseNdcY);
private:
    Scene* m_scene = nullptr;
    std::unordered_map<std::string, std::wstring> m_presets;
};

}