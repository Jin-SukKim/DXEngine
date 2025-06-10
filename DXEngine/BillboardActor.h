#pragma once
#include "Actor.h"

namespace DE {
    class ModelComponent;
    class BoundComponent;

    struct BillboardConsts {
        float widthWorld; // Billboard의 width
        //Vector3 directionWorld; // Billboard가 이동해야하는 경우 사용 (ex: Firball 표현 등)
    };

    class BillboardActor : public Actor
    {
        using Super = Actor;
    public:
        BillboardActor(ComPtr<ID3D11Device>& device, const std::wstring& name);
        ~BillboardActor() override {}

        void Initialize() override;
        void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
        void Render(RenderBase& renderer) override;

        // Billboard를 여러개 만드는 경우도 있고 PixelShader만 다른걸 사용하는 경우가 많음
        void SetBillboard(ComPtr<ID3D11Device>& device, const std::vector<Vector3>& points, const float& width, const ComPtr<ID3D11PixelShader>& pixelShader);
    private:
        ModelComponent* m_billboardModel;
        ConstantBuffer<BillboardConsts> m_billboardConsts;
        BoundComponent* m_billboardBounds;
        ComPtr<ID3D11PixelShader> m_pixelShader;
    };
}
