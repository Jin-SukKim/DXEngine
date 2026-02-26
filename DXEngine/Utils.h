#pragma once
#include "pch.h"

namespace DE {
	template <typename T_CONST>
	class ConstantBuffer {
	public:
		// GPU Constant Buffer 
		void Initialize() {
			D3D11Utils::CreateConstantBuffer(GET_SINGLE(RenderBase)->GetDevice().Get(), m_cpu, m_gpu);
		}

		// CPU ͸ GPU 
		void Upload() {
			D3D11Utils::UpdateBuffer(GET_SINGLE(RenderBase)->GetContext(), m_cpu, m_gpu);
		}

		void SetCpuData(const T_CONST& data) { m_cpu = data; }
		T_CONST& GetCpu() { return m_cpu; }
		const T_CONST& GetCpu() const { return m_cpu; }
		const auto Get() { return m_gpu.Get(); }
		const auto GetAddressOf() const { return m_gpu.GetAddressOf(); }

		void Bind(UINT slot) {
			auto context = GET_SINGLE(RenderBase)->GetContext();
			context->CSSetConstantBuffers(slot, 1, m_gpu.GetAddressOf());
		}
	private:
		T_CONST m_cpu;
		ComPtr<ID3D11Buffer> m_gpu;
	};

	static Vector3 JsonToVec3(const json& j) {
		if (j.is_array() && j.size() >= 3)
			return Vector3(j[0], j[1], j[2]);
		return Vector3(0.f);
	}
	static Vector2 JsonToVec2(const json& j) {
		if (j.is_array() && j.size() >= 2)
			return Vector2(j[0], j[1]);
		return Vector2(0.f);
	}

	static Vector4 JsonToVec4(const json& j) {
		if (j.is_array() && j.size() >= 4)
			return Vector4(j[0], j[1], j[2], j[3]);
		return Vector4(0.f);
	}


}