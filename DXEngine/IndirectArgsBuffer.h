#pragma once
#include "pch.h"

namespace DE {

    struct DispatchArgs {
        UINT threadGroupCountX;
        UINT threadGroupCountY;
        UINT threadGroupCountZ;
    };

    struct DrawInstancedArgs {
        UINT vertexCountPerInstance;
        UINT instanceCount;
        UINT startVertexLocation;
        UINT startInstanceLocation;
    };

    struct DrawIndexedInstancedArgs {
        UINT indexCountPerInstance;
        UINT instanceCount;
        UINT startIndexLocation;
        UINT baseVertexLocation;
        UINT startInstanceLocation;
    };

    template <typename T_ELEMENT>
    class IndirectArgsBuffer {
    public:
        void Initialize(ID3D11Device* device, const T_ELEMENT& initData, UINT argCount);

        auto GetBuffer() const->ID3D11Buffer*;
        auto GetUAV() const->ID3D11UnorderedAccessView*;
        auto& GetCpu() { return m_cpu; }
    private:
        T_ELEMENT m_cpu; 
        ComPtr<ID3D11Buffer> m_gpu;
        ComPtr<ID3D11UnorderedAccessView> m_uav;
    };

    template<typename T_ELEMENT>
    void IndirectArgsBuffer<T_ELEMENT>::Initialize(ID3D11Device* device, const T_ELEMENT& initData, UINT argCount)
    {
        m_cpu = initData;

        D3D11Utils::CreateIndirectBuffer(device, static_cast<UINT>(sizeof(T_ELEMENT)), argCount, &m_cpu, m_gpu, m_uav);
    }

    template<typename T_ELEMENT>
    auto IndirectArgsBuffer<T_ELEMENT>::GetBuffer() const -> ID3D11Buffer*
    {
        return m_gpu.Get();
    }

    template<typename T_ELEMENT>
    auto IndirectArgsBuffer<T_ELEMENT>::GetUAV() const -> ID3D11UnorderedAccessView*
    {
        return m_uav.Get();
    }
}