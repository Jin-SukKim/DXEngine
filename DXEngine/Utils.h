#pragma once
#include "pch.h"

namespace DE {
	template <typename T_CONST>
	class ConstantBuffer {
	public:
		// GPU에 Constant Buffer 생성
		void Initialize() {
			D3D11Utils::CreateConstantBuffer(GET_SINGLE(RenderBase)->GetDevice(), m_cpu, m_gpu);
		}

		// CPU 데이터를 GPU로 복사
		void Upload() {
			D3D11Utils::UpdateBuffer(GET_SINGLE(RenderBase)->GetContext(), m_cpu, m_gpu);
		}

		T_CONST& GetCpu() { return m_cpu; }
		const auto Get() { return m_gpu.Get(); }
		const auto GetAddressOf() const { return m_gpu.GetAddressOf(); }
	private:
		T_CONST m_cpu;
		ComPtr<ID3D11Buffer> m_gpu;
	};
}