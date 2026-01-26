#pragma once
#include "Actor.h"
#include "Utils.h"

namespace DE {
    class BoundComponent;

    class LightActor : public Actor
    {
        using Super = Actor;
    public:
        LightActor(const std::wstring& name);
        virtual ~LightActor() override = default;  // cpp에서 정의

        virtual void Initialize() override;
        virtual void Update(const float& deltaTime) override;
        virtual void Render() override;
        
        // 포인터로 배열 전달
        virtual void RenderShadow(const std::vector<std::unique_ptr<Actor>>* actorLists, size_t count);

        virtual void UpdateShadowGlobals(const Matrix& view, const Matrix& proj);
        virtual float GetLightFrustumWidth(const Matrix& proj);
        virtual Matrix GetLightViewMatrix();
        virtual Matrix GetLightProjMatrix();
        virtual void SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU);
        void TurnOff();
        void TurnOn();
        
        Vector3 GetPos() const { return m_light.position; }
        Light& GetLight() { return m_light; }
        UINT GetLightID() const { return m_lightID; }
        UINT GetShadowWidth() const { return m_shadowWidth; }
        UINT GetShadowHeight() const { return m_shadowHeight; }

    protected:
        static UINT lightID;
        UINT m_lightID;

        float m_lightFov = 90.f;
        float m_nearZ = 0.1f;
        float m_farZ = 10.f;
        const float m_aspectRatio = 1.f;

        ConstantBuffer<GlobalConstants> m_shadowGlobalConsts;

        BoundComponent* m_boundVolume = nullptr;

        Light m_light;

    private:
        int m_shadowWidth = 1280;
        int m_shadowHeight = 1280;
    };

}