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

        // GPU-readable sort step info (matches SortStepGPU in HLSL)
        struct SortStepGPU {
            uint32_t k;
            uint32_t j;
            uint32_t minSortSize;
            uint32_t phase; // 1=BlockSort, 2=GlobalSort, 3=InnerSort
        };

        // CPU-side sort step info
        struct SortStepInfo {
            uint32_t k;
            uint32_t j;
            uint32_t phase; // 1, 2, or 3
            uint32_t minSortSize;
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

        // GPU-driven indirect sort (no CPU readback, no per-dispatch CB upload)
        void InitIndirect(UINT maxSortSize);
        void SortIndirect(ID3D11DeviceContext* context,
                          ID3D11UnorderedAccessView* sortBufferUAV,
                          ID3D11Buffer* indirectArgsBuffer,
                          UINT argsBaseByteOffset);

        UINT GetNumSortSteps() const { return static_cast<UINT>(m_sortStepTable.size()); }
        ID3D11ShaderResourceView* GetStepTableSRV() const { return m_sortStepTableBuffer.GetSRV(); }

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

        // Indirect sort resources
        std::vector<SortStepInfo> m_sortStepTable;
        std::vector<ConstantBuffer<Consts>> m_stepConstBuffers;
        StructuredBuffer<SortStepGPU> m_sortStepTableBuffer;
    };

}
