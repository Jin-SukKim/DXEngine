#include "pch.h"
#include "BitonicSort.h"

namespace DE {
    void BitonicSort::Initialize(ID3D11Device* device, const UINT numElements)
    {
        // Clear existing resources
        m_array = StructuredBuffer<Element>();

        if (numElements == 0)
            return;

        // Round up to nearest power of 2
        UINT num = 1;
        while (num < numElements)
            num *= 2;
        assert(num > 0);
        assert((num & (num - 1)) == 0);

        m_numElements = num;

        m_array.Initialize(device, num);

        // Initialize constant buffer (will be updated each pass)
        m_constBuffer.Initialize();
    }

    void BitonicSort::SortInternal(ID3D11DeviceContext* context,
                                    ID3D11UnorderedAccessView* uav, UINT sortSize)
    {
        auto& sortPSOs = RenderBase::computeCommon.sort;
        context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

        // GPU 메모리 종류
        //  - Global Memory (VRAM) : 크지만 느리고 모든 Thread가 접근 가능
        //  - groupshared (LDS) : 작지만 빠르고 같은 Thread group 안에서만 공유
        if (sortSize >= BLOCK_SIZE) {
            // Phase 1: LDS block sort (1 dispatch)
            // 각 Block 안에서 정렬을 LDS에서 끝내기
            context->CSSetShader(sortPSOs.bitonicBlockSortCS.computeShader.Get(), 0, 0);
            context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);

            // Phase 2+3: Global merge
            for (uint32_t k = BLOCK_SIZE * 2; k <= sortSize; k *= 2) {
                // Phase 2: outer j passes (global memory)
                // LDS 범위를 넘어가는 Block 단위 전체 정렬
                // j가 작을수록 비교 대상이 가까이에 있음
                //  (j=2048: 인덱스 0과 2048을 비교 → 서로 다른 threadgroup → Global 필수)
                //  (j=1024: 인덱스 0과 1024를 비교 → 같은 threadgroup!    → LDS 가능!)
                context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
                for (uint32_t j = k / 2; j >= BLOCK_SIZE; j /= 2) {
                    m_constBuffer.SetCpuData({ k, j });
                    m_constBuffer.Upload();
                    context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                    context->Dispatch((sortSize + 1023) / 1024, 1, 1);
                }
                // Phase 3: inner j passes (LDS, 1 dispatch)
                // 전체 Block 단위로 정렬하고 남은 부분을 LDS로 정렬
                m_constBuffer.SetCpuData({ k, 0 });
                m_constBuffer.Upload();
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                context->CSSetShader(sortPSOs.bitonicInnerSortCS.computeShader.Get(), 0, 0);
                context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);
            }
        } else {
            // Fallback: standard bitonic sort for small arrays (< 2048)
            context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
            // k가 커지면서 점점 큰 묶음을 정렬하고, j가 줄어들면서 묶음 안을 정렬
            // 몇 개씩 묶어서 정렬할지 (2, 4, 8, 16, ...)
            for (uint32_t k = 2; k <= sortSize; k *= 2) {
                // 묶음 안에서 얼마나 떨어진 것과 비교할지 (k/2, k/4, ..., 1)
                for (uint32_t j = k / 2; j > 0; j /= 2) {
                    m_constBuffer.SetCpuData({ k, j });
                    m_constBuffer.Upload();
                    context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                    context->Dispatch((sortSize + 1023) / 1024, 1, 1);
                }
            }
        }
    }

    void BitonicSort::Sort(ID3D11DeviceContext* context)
    {
        if (m_numElements == 0)
            return;

        SortInternal(context, m_array.GetUAV(), m_numElements);

        // Unbind resources
        ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
        context->CSSetShaderResources(0, 2, nullSRV);
        ID3D11UnorderedAccessView* nullUAV[2] = { nullptr, nullptr };
        context->CSSetUnorderedAccessViews(0, 2, nullUAV, nullptr);
        context->CSSetShader(nullptr, 0, 0);
    }

    void BitonicSort::Sort(ID3D11DeviceContext* context,
                           ID3D11UnorderedAccessView* sortBufferUAV,
                           UINT elementCount)
    {
        if (elementCount == 0) return;

        // Round up to power of 2
        UINT sortSize = 1;
        while (sortSize < elementCount) sortSize *= 2;

        SortInternal(context, sortBufferUAV, sortSize);

        // Unbind
        ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
        context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
        context->CSSetShader(nullptr, 0, 0);
    }
}
