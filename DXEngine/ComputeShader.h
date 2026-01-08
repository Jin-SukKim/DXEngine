#pragma once

namespace DE {
	class ComputeShader
	{
	public:
		void Initialize(ID3D11Device* device, const std::wstring& csName);
		void Dispatch(ID3D11DeviceContext* context, UINT groupX, UINT groupY, UINT groupZ);
		void UpdateConsts(ID3D11DeviceContext* context, UINT startSlot, UINT numBuffers, ID3D11Buffer* const* ppConsts);

		void SetSRVs(ID3D11DeviceContext* context, UINT startSlot, const std::vector<ID3D11ShaderResourceView*>& srvs);
		void SetUAVs(ID3D11DeviceContext* context, UINT startSlot, const std::vector<ID3D11UnorderedAccessView*>& uavs, const UINT* initCounts);
	private:
		void ComputeShaderBarrier(ID3D11DeviceContext* context);
	private:
		ComPtr<ID3D11ComputeShader> m_cs;
		std::vector<ID3D11ShaderResourceView*> m_srvs;
		std::vector<ID3D11UnorderedAccessView*> m_uavs;
	};
}

