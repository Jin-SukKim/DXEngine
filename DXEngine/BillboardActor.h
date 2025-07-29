#pragma once
#include "Actor.h"

namespace DE {
    class ModelComponent;
    class BoundComponent;

    struct BillboardConsts {
        float widthWorld = 1.f; // Billboard의 width
        Vector3 directionWorld; // Billboard가 이동해야하는 경우 사용 (ex: Firball 표현 등)
        UINT arraySize = 0;
        float dummy[3] = { 0.f, 0.f, 0.f };
    };

    class BillboardActor : public Actor
    {
        using Super = Actor;
    public:
        BillboardActor(const std::wstring& name);
        ~BillboardActor() override {}

        void Initialize() override;
        void Update(const float& deltaTime) override;
        void Render() override;

        // Billboard를 여러개 만드는 경우도 있고 PixelShader만 다른걸 사용하는 경우가 많음
        void SetBillboard(const std::vector<Vector3>& points, const float& width, const std::vector<std::string>& filenames, const ComPtr<ID3D11PixelShader>& pixelShader = nullptr);
    private:
        ModelComponent* m_billboardModel;
        ConstantBuffer<BillboardConsts> m_billboardConsts;
        BoundComponent* m_billboardBounds;
        ComPtr<ID3D11PixelShader> m_pixelShader;
        
        // Texture Array
        Texture2D m_texArray;  // C++에서는 Texture2D이지만 HLSL에선 TextureArray를 사용
    };
}
