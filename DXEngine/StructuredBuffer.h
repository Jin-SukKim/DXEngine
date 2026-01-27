#pragma once
#include "pch.h"

namespace DE {

template <typename T_ELEMENT>
class StructuredBuffer
{
public:
	virtual void Initialize(ID3D11Device* device, const UINT numElements);
	virtual void Initialize(ID3D11Device* device);
	void Upload(ID3D11DeviceContext* context);
	void Download(ID3D11DeviceContext* context);

	auto GetBuffer() const->ID3D11Buffer*;
	auto GetSRV() const->ID3D11ShaderResourceView*;
	auto GetUAV() const->ID3D11UnorderedAccessView*;

	auto GetAddressOfSRV() const->ID3D11ShaderResourceView* const*;
	auto GetAddressOfUAV() const->ID3D11UnorderedAccessView* const*;
	
	void SetData(std::vector<T_ELEMENT> data);
	auto Get(UINT idx) -> T_ELEMENT&;
	auto Size() -> UINT;
	const std::vector<T_ELEMENT>& GetCpu() const { return m_cpu; }
protected:
	std::vector<T_ELEMENT> m_cpu;
	ComPtr<ID3D11Buffer> m_gpu;
	ComPtr<ID3D11Buffer> m_staging;

	ComPtr<ID3D11ShaderResourceView> m_srv;
	ComPtr<ID3D11UnorderedAccessView> m_uav;
};

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Initialize(ID3D11Device* device, const UINT numElements)
{
	m_cpu.resize(numElements);
	Initialize(device);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Initialize(ID3D11Device* device)
{
	// StructuredBuffer 持失
	D3D11Utils::CreateStructuredBuffer(device, static_cast<UINT>(m_cpu.size()), sizeof(T_ELEMENT), m_cpu.data(), m_gpu, m_srv, m_uav);

	// StagingBuffer 持失
	D3D11Utils::CreateStagingBuffer(device, static_cast<UINT>(m_cpu.size()), sizeof(T_ELEMENT), m_cpu.data(), m_staging);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Upload(ID3D11DeviceContext* context)
{
	// CPU -> Staging 
	D3D11Utils::CopyToStagingBuffer(context, m_staging.Get(), 
		static_cast<UINT>(m_cpu.size() * sizeof(T_ELEMENT)), m_cpu.data());

	// Staging -> GPU
	context->CopyResource(m_gpu.Get(), m_staging.Get());
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Download(ID3D11DeviceContext* context)
{
	// GPU -> Staging
	context->CopyResource(m_staging.Get(), m_gpu.Get());

	// Staging -> CPU
	D3D11Utils::CopyFromStagingBuffer(context, m_cpu.data(), 
		static_cast<UINT>(m_cpu.size() * sizeof(T_ELEMENT)), m_staging.Get());
}

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetBuffer() const -> ID3D11Buffer*
{
	return m_gpu.Get();
}

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetSRV() const -> ID3D11ShaderResourceView*
{
	return m_srv.Get();
}

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetUAV() const -> ID3D11UnorderedAccessView*
{
	return m_uav.Get();
}

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetAddressOfSRV() const -> ID3D11ShaderResourceView* const*
{
	return m_srv.GetAddressOf();
}

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetAddressOfUAV() const -> ID3D11UnorderedAccessView* const*
{
	return m_uav.GetAddressOf();
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::SetData(std::vector<T_ELEMENT> data)
{
	m_cpu = std::move(data);
}
template<typename T_ELEMENT>
T_ELEMENT& StructuredBuffer<T_ELEMENT>::Get(UINT idx)
{
	return m_cpu[idx];
}
template<typename T_ELEMENT>
UINT StructuredBuffer<T_ELEMENT>::Size()
{
	return static_cast<UINT>(m_cpu.size());
}
}

