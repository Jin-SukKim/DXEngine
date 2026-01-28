#pragma once
#include "LightActor.h"

namespace DE
{
    class PointLight : public LightActor
    {
        using Super = LightActor;
    public:
        PointLight(const std::wstring& name);
        ~PointLight() override {}

        void Initialize() override;
        void Update(const float& deltaTime) override;
		void RenderShadow(const std::vector<std::unique_ptr<Actor>>* actorLists, size_t count) override;
        void UpdateShadowGlobals(const Matrix& view, const Matrix& proj, const int& i);
    private:
		// 6개의 면에 대한 View Matrix를 생성
        Matrix GetCubemapViewMatrix(int idx);
    };
}

