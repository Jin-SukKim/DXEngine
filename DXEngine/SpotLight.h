#pragma once
#include "LightActor.h"

namespace DE {
    class SpotLight : public LightActor
    {
        using Super = LightActor;
    public:
        SpotLight(const std::wstring& name);
        ~SpotLight() override {}

        void Initialize() override;
        void Update(const float& deltaTime) override;
        float GetLightFrustumWidth(const Matrix& proj) override;

    };
}
