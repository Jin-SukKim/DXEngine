#pragma once
#include "LightActor.h"

namespace DE {
    class SpotLight : public LightActor
    {
        using Super = LightActor;
    public:
        SpotLight(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name);
        ~SpotLight() override {}

        void Initialize() override;
        void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
        float GetLightFrustumWidth(const Matrix& proj) override;
    };
}
