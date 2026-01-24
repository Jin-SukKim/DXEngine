#include "pch.h"
#include "BitonicSort.h"

namespace DE {
    void BitonicSort::Initialize(ID3D11Device* device, const UINT numElements)
    {
        // 2의 제곱인지 확인
        // https://stackoverflow.com/questions/108318/how-can-i-test-whether-a-number-is-a-power-of-2
        UINT num = 1;
        while (num < numElements)
            num *= 2;
        assert(num > 0);
        assert((num & (num - 1)) == 0);

        m_numElements = num;

        m_array.Initialize(device, num);

        // 필요한 ConstBuffer 들을 미리 만들어 두기
        for (uint32_t k = 2; k <= num; k *= 2)
            for (uint32_t j = k / 2; j > 0; j /= 2) {
                Consts c;
                c.j = j;
                c.k = k;
                m_constsCpu.push_back(c);
            }
        m_constsGpu.resize(m_constsCpu.size());
        for (size_t i = 0; i < m_constsCpu.size(); i++) {
            D3D11Utils::CreateConstantBuffer(device, m_constsCpu[i], m_constsGpu[i]);
        }
    }

    void BitonicSort::Sort(ID3D11DeviceContext* context)
    {
        // ComputeCommon의 공유 ComputePSO 사용
        auto& bitonicSortCS = RenderBase::computeCommon.sort.bitonicSortCS;

        size_t constCount = 0;
        for (uint32_t k = 2; k <= m_numElements; k *= 2)
            for (uint32_t j = k / 2; j > 0; j /= 2) {
                context->CSSetConstantBuffers(0, 1, m_constsGpu[constCount++].GetAddressOf());
                context->CSSetShader(bitonicSortCS.computeShader.Get(), 0, 0);
                context->CSSetUnorderedAccessViews(0, 1, m_array.GetAddressOfUAV(), NULL);
                context->Dispatch(UINT(ceil(m_numElements / 256)), 1, 1);
            }

        // UAV Barrier
        ID3D11ShaderResourceView* nullSRV[2] = { 0, 0 };
        context->CSSetShaderResources(0, 2, nullSRV);
        ID3D11UnorderedAccessView* nullUAV[2] = { 0, 0 };
        context->CSSetUnorderedAccessViews(0, 2, nullUAV, NULL);
        context->CSSetShader(nullptr, 0, 0);
    }
}