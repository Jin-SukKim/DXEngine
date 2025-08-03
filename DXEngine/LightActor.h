#pragma once
#include "Actor.h"
#include "Utils.h"

namespace DE {
    class HaloEffect;
    class BoundComponent;

    // 기본적으로 Spot Light으로 구현되어 있음
    class LightActor : public Actor
    {
        using Super = Actor;
    public:
        LightActor(const std::wstring& name);
        virtual ~LightActor() override {}

        virtual void Initialize() override;
        virtual void Update(const float& deltaTime) override;
        virtual void Render() override;
        virtual void RenderShadow(const std::vector<std::vector<std::shared_ptr<Actor>>>& actorLists);

		virtual void UpdateShadowGlobals(const Matrix& view, const Matrix& proj);
        virtual float GetLightFrustumWidth(const Matrix& proj);
        virtual Matrix GetLightViewMatrix();
        virtual Matrix GetLightProjMatrix();
        virtual void SetGlobals(const ComPtr<ID3D11Buffer>& globalConstsGPU);
        
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

        // Shadow맵을 렌더링할때 사용할 GlobalConsts
        ConstantBuffer<GlobalConstants> m_shadowGlobalConsts;

        std::shared_ptr<HaloEffect> m_halo;

        BoundComponent* m_boundVolume = nullptr;

        Light m_light;

    private:
        // Shadow
        // Shadow Map은 해상도가 다른데 화면 해상도와 같을 필요가 없음
        // 보통 Texture가 정사각형이기 때문에 ratio가 1:1인 해상도로 설정
        int m_shadowWidth = 1280;
        int m_shadowHeight = 1280;
    };

}