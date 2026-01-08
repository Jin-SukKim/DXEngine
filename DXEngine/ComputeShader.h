#pragma once

namespace DE {
	class ComputeShader
	{
	public:
		void Initialize(ID3D11Device* device, const std::wstring& csName);
		void Dispatch(ID3D11DeviceContext* context, UINT groupX, UINT groupY, UINT groupZ);
		void DispatchIndirect(ID3D11DeviceContext* context, ID3D11Buffer* args);
		void UpdateConsts(ID3D11DeviceContext* context, UINT startSlot, UINT numBuffers, ID3D11Buffer* const* ppConsts);
	private:
		void ComputeShaderBarrier(ID3D11DeviceContext* context);
	private:
		ComPtr<ID3D11ComputeShader> m_cs;
		const UINT maxSRV = 8;
		const UINT maxUAV= 8;
	};
}

