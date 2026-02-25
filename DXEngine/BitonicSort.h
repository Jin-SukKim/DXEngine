#pragma once
#include "StructuredBuffer.h"

namespace DE {
    class BitonicSort {
    public:
        struct Element {
            uint32_t key[2]; // x: Batch ID (Major), y : Depth (Minor)
            uint32_t value;
        };

        __declspec(align(256)) struct Consts {
            uint32_t k;
            uint32_t j;
        };

        BitonicSort() {};

        BitonicSort(ID3D11Device* device, const UINT numElements) {
            Initialize(device, numElements);
        };

        void Initialize(ID3D11Device* device, const UINT numElements);

        void Sort(ID3D11DeviceContext* context);

        // 외부 UAV를 받아 정렬 (elementCount개만 유효, 나머지는 패딩)
        void Sort(ID3D11DeviceContext* context,
                  ID3D11UnorderedAccessView* sortBufferUAV,
                  UINT elementCount);

        ID3D11ShaderResourceView* GetSRV() const { return m_array.GetSRV(); }
        ID3D11UnorderedAccessView* GetUAV() const { return m_array.GetUAV(); }
    protected:
        static constexpr UINT BLOCK_SIZE = 2048;

        // 공통 정렬 로직 헬퍼
        void SortInternal(ID3D11DeviceContext* context,
                          ID3D11UnorderedAccessView* uav, UINT sortSize);

        // Single constant buffer (updated each pass instead of pre-allocating all)
        ConstantBuffer<Consts> m_constBuffer;
        StructuredBuffer<Element> m_array;

        UINT m_numElements = 0;
    };

}
