#pragma once
#include "pch.h"

namespace DE {
	template <typename T> 
	class StagingBuffer {
	public:
        void Initialize(ID3D11Device* device, const std::vector<T>& data);
        void Download(ID3D11DeviceContext* context);

        //void SetData(const std::vector<T>& data);
        const std::vector<T>& GetCpu();
        const T& GetCpu(UINT idx);
        ID3D11Buffer* GetGpu();
	private:
		std::vector<T> m_cpu;
		ComPtr<ID3D11Buffer> m_gpu;
    };

    template<typename T>
    void StagingBuffer<T>::Initialize(ID3D11Device* device, const std::vector<T>& data)
    {
        m_cpu = data;
        D3D11Utils::CreateStagingBuffer(device, UINT(data.size()), sizeof(T),
            data.data(), m_gpu);
    }

    template<typename T>
    void StagingBuffer<T>::Download(ID3D11DeviceContext* context)
    {
        D3D11_MAPPED_SUBRESOURCE ms;
        context->Map(m_gpu.Get(), NULL, D3D11_MAP_READ, NULL, &ms);
        uint8_t* pData = (uint8_t*)ms.pData;
        memcpy(m_cpu.data(), &pData[0], sizeof(T) * m_cpu.size());
        context->Unmap(m_gpu.Get(), NULL);
    }

    //template<typename T>
    //void StagingBuffer<T>::SetData(const std::vector<T>& data)
    //{
    //    m_cpu = data;
    //}
    //

    template<typename T>
    const std::vector<T>& StagingBuffer<T>::GetCpu()
    {
        return m_cpu;
    }
    template<typename T>
    const T& StagingBuffer<T>::GetCpu(UINT idx)
    {
        return m_cpu[idx];
    }
    template<typename T>
    ID3D11Buffer* StagingBuffer<T>::GetGpu()
    {
        return m_gpu.Get();
    }
}