#pragma once
#include "pch.h"
#include "StructuredBuffer.h"

namespace DE {

// Consume/Apeend Buffer
template <typename T_ELEMENT>
class AppendBuffer : public StructuredBuffer<T_ELEMENT>
{
	using Super = StructuredBuffer<T_ELEMENT>;

public:
    void Initialize(ID3D11Device* device, const UINT numElements) override;
    void Initialize(ID3D11Device* device) override;
    friend void swap(AppendBuffer<T_ELEMENT>& lhs, AppendBuffer<T_ELEMENT>& rhs) {
        std::swap(lhs.m_cpu, rhs.m_cpu);
        std::swap(lhs.m_gpu, rhs.m_gpu);
        std::swap(lhs.m_srv, rhs.m_srv);
        std::swap(lhs.m_uav, rhs.m_uav);
    }

    auto GetRWuav() const->ID3D11UnorderedAccessView*;
    auto GetAddressOfRWuav() const->ID3D11UnorderedAccessView* const*;
protected:
    ComPtr<ID3D11UnorderedAccessView> m_rwUav;
};

template<typename T_ELEMENT>
void AppendBuffer<T_ELEMENT>::Initialize(ID3D11Device* device, const UINT numElements)
{
    Super::m_cpu.resize(numElements);
    Initialize(device);
}

template<typename T_ELEMENT>
void AppendBuffer<T_ELEMENT>::Initialize(ID3D11Device* device)
{
    D3D11Utils::CreateAppendBuffer(device, static_cast<UINT>(Super::m_cpu.size()), static_cast<UINT>(sizeof(T_ELEMENT)), Super::m_cpu.data(), Super::m_gpu, Super::m_srv, Super::m_uav, m_rwUav);
    // StagingBuffer »ý¼º
    D3D11Utils::CreateStagingBuffer(device, static_cast<UINT>(Super::m_cpu.size()), sizeof(T_ELEMENT), Super::m_cpu.data(), Super::m_staging);
}
template<typename T_ELEMENT>
inline auto AppendBuffer<T_ELEMENT>::GetRWuav() const -> ID3D11UnorderedAccessView*
{
    return m_rwUav.Get();
}

template<typename T_ELEMENT>
inline auto AppendBuffer<T_ELEMENT>::GetAddressOfRWuav() const -> ID3D11UnorderedAccessView* const*
{
    return m_rwUav.GetAddressOf();
}


}