#pragma once
#include "BillboardActor.h"

namespace DE {
    class TreeBillboard : public BillboardActor
    {
        using Super = BillboardActor;
    public:        
        TreeBillboard(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name);
        ~TreeBillboard() override {}

        void Initialize() override;
    };
}