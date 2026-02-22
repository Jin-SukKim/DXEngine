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

        if (sortSize >= BLOCK_SIZE) {
            // Phase 1: LDS block sort (1 dispatch)
            context->CSSetShader(sortPSOs.bitonicBlockSortCS.computeShader.Get(), 0, 0);
            context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);

            // Phase 2+3: Global merge
            for (uint32_t k = BLOCK_SIZE * 2; k <= sortSize; k *= 2) {
                // Phase 2: outer j passes (global memory)
                context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
                for (uint32_t j = k / 2; j >= BLOCK_SIZE; j /= 2) {
                    m_constBuffer.SetCpuData({ k, j });
                    m_constBuffer.Upload();
                    context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                    context->Dispatch((sortSize + 1023) / 1024, 1, 1);
                }
                // Phase 3: inner j passes (LDS, 1 dispatch)
                m_constBuffer.SetCpuData({ k, 0 });
                m_constBuffer.Upload();
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                context->CSSetShader(sortPSOs.bitonicInnerSortCS.computeShader.Get(), 0, 0);
                context->Dispatch(sortSize / BLOCK_SIZE, 1, 1);
            }
        } else {
            // Fallback: standard bitonic sort for small arrays (< 2048)
            context->CSSetShader(sortPSOs.bitonicSortCS.computeShader.Get(), 0, 0);
            for (uint32_t k = 2; k <= sortSize; k *= 2) {
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
