#pragma once
#include "Actor.h"

namespace DE {
    class HaloEffect;
    class BoundComponent;

    class LightActor : public Actor
    {
        using Super = Actor;
    public:
        LightActor(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name);
        virtual ~LightActor() override {}

        virtual void Initialize() override;
        virtual void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
        virtual void Render(RenderBase& renderer) override;
        virtual void RenderShadow(RenderBase& renderer, const std::vector<std::shared_ptr<Actor>>& actorList);

		virtual void UpdateShadowGlobals(ComPtr<ID3D11DeviceContext>& context, const Matrix& view, const Matrix& proj);
        virtual float GetLightFrustumWidth(const Matrix& proj);
        virtual Matrix GetLightViewMatrix();
        virtual Matrix GetLightProjMatrix();
        virtual void SetGlobals(ComPtr<ID3D11DeviceContext>& context, const ComPtr<ID3D11Buffer>& globalConstsGPU);
        
		Vector3 GetPos() const { return m_light.position; }
        Light& GetLight() { return m_light; }
		UINT GetLightID() const { return m_lightID; }
    private:
        static UINT lightID;
        UINT m_lightID;

        float m_lightFov = 90.f;
        float m_nearZ = 0.1f;
        float m_farZ = 10.f;
        const float m_aspectRatio = 1.f;

        // Shadow¸ÊÀ» ·»´õ¸µÇÒ¶§ »ç¿ëÇÒ GlobalConsts
        ConstantBuffer<GlobalConstants> m_shadowGlobalConsts;

        std::shared_ptr<HaloEffect> m_halo;

        BoundComponent* m_boundVolume = nullptr;

        Light m_light;
    };

}