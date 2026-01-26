#pragma once
#include <string>
#include <unordered_map>

namespace DE {

class Scene;

// ClickEffectManager: Effect 생성 요청만 담당 (소유권 없음)
class ClickEffectManager {
public:
    static ClickEffectManager& Get() {
        static ClickEffectManager instance;
        return instance;
    }

    void Initialize();
    
    // Scene 설정 (필수)
    void SetActiveScene(Scene* scene) { m_scene = scene; }

    // 마우스 위치에 이펙트 생성
    void SpawnEffectAtMousePosition(const std::wstring& presetPath);
    
    // 월드 좌표에 이펙트 생성
    void SpawnEffectAtWorldPosition(const std::wstring& presetPath, const Vector3& worldPos);

    // 프리셋 등록 및 실행
    void RegisterPreset(const std::string& name, const std::wstring& path);
    void TriggerPreset(const std::string& name);

private:
    ClickEffectManager() = default;
    Vector3 ScreenToWorldPosition(float mouseNdcX, float mouseNdcY);

private:
    Scene* m_scene = nullptr;
    std::unordered_map<std::string, std::wstring> m_presets;
};

}