#pragma once
#include "StructuredBuffer.h"
#include "ComputeShader.h"

namespace DE {
    class BitonicSort {
    public:
        struct Element {
            uint32_t key;
            uint32_t value;
        };

        __declspec(align(256)) struct Consts {
            uint32_t k;
            uint32_t j;
        };

        BitonicSort() {};

        BitonicSort(ID3D11Device* device, const UINT numElements,
            const std::wstring shaderFilename) {
            Initialize(device, numElements, shaderFilename);
        };

        void Initialize(ID3D11Device* device, const UINT numElements,
            const std::wstring shaderFilename);

        void Sort(ID3D11DeviceContext* context);

        ID3D11ShaderResourceView* GetSRV() const { return m_array.GetSRV(); }
        ID3D11UnorderedAccessView* GetUAV() const { return m_array.GetUAV(); }
    protected:
        std::vector<Consts> m_constsCpu;
        std::vector<ComPtr<ID3D11Buffer>> m_constsGpu;
        StructuredBuffer<Element> m_array;

		ComputeShader m_bitonicSortCS;

        UINT m_numElements = 0;
    };

}
