#pragma once

namespace DE {
	class ComputeShader
	{
	public:
		void Initialize(const std::wstring& csName);
		void Dispatch(UINT groupX, UINT groupY, UINT groupZ, std::vector<ID3D11ShaderResourceView*>& srvs, std::vector<ID3D11UnorderedAccessView*>& uavs);
		void UpdateConsts(UINT startSlot, UINT numBuffers, ID3D11Buffer* const* ppConsts);
	private:
		void ComputeShaderBarrier(ID3D11DeviceContext* context, UINT srvNum, UINT uavNum);
	private:
		ComPtr<ID3D11ComputeShader> m_cs;
	};
}

