#pragma once
#include "pch.h"

namespace DE {

template <typename T_ELEMENT>
class StructuredBuffer
{
public:
	// 최대 용량으로 풀 초기화
	virtual void Initialize(ID3D11Device* device, const UINT maxCapacity);
	virtual void Initialize(ID3D11Device* device);
	
	void Upload(ID3D11DeviceContext* context);
	void Upload(ID3D11DeviceContext* context, UINT count);
	void UploadAll(ID3D11DeviceContext* context);
	void Download(ID3D11DeviceContext* context);
	void Download(ID3D11DeviceContext* context, UINT count);
	void DownloadAll(ID3D11DeviceContext* context);

	auto GetBuffer() const->ID3D11Buffer*;
	auto GetSRV() const->ID3D11ShaderResourceView*;
	auto GetUAV() const->ID3D11UnorderedAccessView*;

	auto GetAddressOfSRV() const->ID3D11ShaderResourceView* const*;
	auto GetAddressOfUAV() const->ID3D11UnorderedAccessView* const*;
	
	void SetData(std::vector<T_ELEMENT> data);
	auto Get(UINT idx) -> T_ELEMENT&;
	const T_ELEMENT& Get(UINT idx) const;
	
	// 풀 관리 메서드
	UINT Capacity() const { return m_capacity; }
	UINT Size() const { return m_activeCount; }
	void SetActiveCount(UINT count) { m_activeCount = std::min(count, m_capacity); }
	void Clear() { m_activeCount = 0; }
	
	bool Push(const T_ELEMENT& element);
	bool Pop();
	
	const std::vector<T_ELEMENT>& GetCpu() const { return m_cpu; }
	std::vector<T_ELEMENT>& GetCpu() { return m_cpu; }

protected:
	UINT m_capacity = 0;
	UINT m_activeCount = 0;
	
	std::vector<T_ELEMENT> m_cpu;
	ComPtr<ID3D11Buffer> m_gpu;
	ComPtr<ID3D11Buffer> m_staging;

	ComPtr<ID3D11ShaderResourceView> m_srv;
	ComPtr<ID3D11UnorderedAccessView> m_uav;
};

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Initialize(ID3D11Device* device, const UINT maxCapacity)
{
	m_capacity = maxCapacity;
	m_activeCount = 0;
	m_cpu.resize(maxCapacity);
	Initialize(device);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Initialize(ID3D11Device* device)
{
	if (m_cpu.empty()) {
		throw std::runtime_error("StructuredBuffer: CPU buffer is empty before Initialize()");
	}
	
	m_capacity = static_cast<UINT>(m_cpu.size());
	
	D3D11Utils::CreateStructuredBuffer(device, m_capacity, sizeof(T_ELEMENT), m_cpu.data(), m_gpu, m_srv, m_uav);
	D3D11Utils::CreateStagingBuffer(device, m_capacity, sizeof(T_ELEMENT), m_cpu.data(), m_staging);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Upload(ID3D11DeviceContext* context)
{
	Upload(context, m_activeCount);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Upload(ID3D11DeviceContext* context, UINT count)
{
	if (count == 0) return;
	count = std::min(count, m_capacity);
	
	const UINT byteSize = count * sizeof(T_ELEMENT);
	
	D3D11Utils::CopyToStagingBuffer(context, m_staging.Get(), byteSize, m_cpu.data());

	D3D11_BOX srcBox = {};
	srcBox.left = 0;
	srcBox.right = byteSize;
	srcBox.top = 0;
	srcBox.bottom = 1;
	srcBox.front = 0;
	srcBox.back = 1;
	
	context->CopySubresourceRegion(m_gpu.Get(), 0, 0, 0, 0, m_staging.Get(), 0, &srcBox);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::UploadAll(ID3D11DeviceContext* context)
{
	Upload(context, m_capacity);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Download(ID3D11DeviceContext* context)
{
	Download(context, m_activeCount);
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::Download(ID3D11DeviceContext* context, UINT count)
{
	if (count == 0) return;
	count = std::min(count, m_capacity);
	
	const UINT byteSize = count * sizeof(T_ELEMENT);
	
	D3D11_BOX srcBox = {};
	srcBox.left = 0;
	srcBox.right = byteSize;
	srcBox.top = 0;
	srcBox.bottom = 1;
	srcBox.front = 0;
	srcBox.back = 1;
	
	context->CopySubresourceRegion(m_staging.Get(), 0, 0, 0, 0, m_gpu.Get(), 0, &srcBox);

	D3D11Utils::CopyFromStagingBuffer(context, m_cpu.data(), byteSize, m_staging.Get());
}

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::DownloadAll(ID3D11DeviceContext* context)
{
	Download(context, m_capacity);
}

template<typename T_ELEMENT>
bool StructuredBuffer<T_ELEMENT>::Push(const T_ELEMENT& element)
{
	if (m_activeCount >= m_capacity) return false;
	m_cpu[m_activeCount++] = element;
	return true;
}

template<typename T_ELEMENT>
bool StructuredBuffer<T_ELEMENT>::Pop()
{
	if (m_activeCount == 0) return false;
	--m_activeCount;
	return true;
}

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetBuffer() const -> ID3D11Buffer* { return m_gpu.Get(); }

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetSRV() const -> ID3D11ShaderResourceView* { return m_srv.Get(); }

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetUAV() const -> ID3D11UnorderedAccessView* { return m_uav.Get(); }

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetAddressOfSRV() const -> ID3D11ShaderResourceView* const* { return m_srv.GetAddressOf(); }

template<typename T_ELEMENT>
auto StructuredBuffer<T_ELEMENT>::GetAddressOfUAV() const -> ID3D11UnorderedAccessView* const* { return m_uav.GetAddressOf(); }

template<typename T_ELEMENT>
void StructuredBuffer<T_ELEMENT>::SetData(std::vector<T_ELEMENT> data)
{
	// GPU 버퍼가 이미 생성되어 있으면 capacity 변경 불가
	if (m_gpu) {
		m_activeCount = static_cast<UINT>(std::min(data.size(), static_cast<size_t>(m_capacity)));
		if (data.size() > m_capacity) {
			// 경고: 데이터가 잘림
			data.resize(m_capacity);
		}
	}
	else {
		// 초기화 전이면 capacity를 데이터 크기로 설정
		m_capacity = static_cast<UINT>(data.size());
		m_activeCount = m_capacity;
	}
	
	m_cpu = std::move(data);
	m_cpu.resize(m_capacity);
}

template<typename T_ELEMENT>
T_ELEMENT& StructuredBuffer<T_ELEMENT>::Get(UINT idx) 
{ 
	if (idx >= m_capacity) {
		throw std::out_of_range("StructuredBuffer::Get() - Index out of range");
	}
	return m_cpu[idx]; 
}

template<typename T_ELEMENT>
const T_ELEMENT& StructuredBuffer<T_ELEMENT>::Get(UINT idx) const
{
	if (idx >= m_capacity) {
		throw std::out_of_range("StructuredBuffer::Get() - Index out of range");
	}
	return m_cpu[idx];
}

}

