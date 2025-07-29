#pragma once
#include "BillboardActor.h"

namespace DE {
    class TreeBillboard : public BillboardActor
    {
        using Super = BillboardActor;
    public:        
        TreeBillboard(const std::wstring& name);
        ~TreeBillboard() override {}

        void Initialize() override;
    };
}