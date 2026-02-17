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
        // https://stackoverflow.com/questions/108318/how-can-i-test-whether-a-number-is-a-power-of-2
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

    void BitonicSort::Sort(ID3D11DeviceContext* context)
    {
        if (m_numElements == 0)
            return;

        auto& bitonicSortCS = RenderBase::computeCommon.sort.bitonicSortCS;

        // Set shader and UAV once (stays bound for all passes)
        context->CSSetShader(bitonicSortCS.computeShader.Get(), 0, 0);
        context->CSSetUnorderedAccessViews(0, 1, m_array.GetAddressOfUAV(), NULL);

        // Bitonic sort: O(log^2 n) passes
        // NOTE: Can be replaced with BitonicMergeSort for better performance
        //       (local shared memory sort + global merge passes)
        for (uint32_t k = 2; k <= m_numElements; k *= 2)
        {
            for (uint32_t j = k / 2; j > 0; j /= 2)
            {
                // Update constant buffer for this pass
                m_constBuffer.SetCpuData({ k, j });
                m_constBuffer.Upload();
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());

                // Dispatch compute shader
                context->Dispatch((m_numElements + 1023) / 1024, 1, 1);
            }
        }

        // Unbind resources
        ID3D11ShaderResourceView* nullSRV[2] = { 0, 0 };
        context->CSSetShaderResources(0, 2, nullSRV);
        ID3D11UnorderedAccessView* nullUAV[2] = { 0, 0 };
        context->CSSetUnorderedAccessViews(0, 2, nullUAV, NULL);
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

        auto& bitonicSortCS = RenderBase::computeCommon.sort.bitonicSortCS;
        context->CSSetShader(bitonicSortCS.computeShader.Get(), 0, 0);
        context->CSSetUnorderedAccessViews(0, 1, &sortBufferUAV, NULL);

        for (uint32_t k = 2; k <= sortSize; k *= 2) {
            for (uint32_t j = k / 2; j > 0; j /= 2) {
                m_constBuffer.SetCpuData({ k, j });
                m_constBuffer.Upload();
                context->CSSetConstantBuffers(0, 1, m_constBuffer.GetAddressOf());
                context->Dispatch((sortSize + 1023) / 1024, 1, 1);
            }
        }

        // Unbind
        ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
        context->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);
        context->CSSetShader(nullptr, 0, 0);
    }
}