#include "pch.h"
#include "ComputeShader.h"

namespace DE {
	void ComputeShader::Initialize(const std::wstring& csName)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		D3D11Utils::CreateCS(device.Get(), csName, m_cs);
	}

	void ComputeShader::Dispatch(UINT groupX, UINT groupY, UINT groupZ, std::vector<ID3D11ShaderResourceView*>& srvs, std::vector<ID3D11UnorderedAccessView*>& uavs)
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		context->CSSetShaderResources(0, static_cast<UINT>(srvs.size()), srvs.data());
		context->CSSetUnorderedAccessViews(0, static_cast<UINT>(uavs.size()), uavs.data(), nullptr);
		context->CSSetShader(m_cs.Get(), 0, 0);
		context->Dispatch(groupX, groupY, groupZ);
		ComputeShaderBarrier(context, static_cast<UINT>(srvs.size()), static_cast<UINT>(uavs.size()));
	}

	void ComputeShader::ComputeShaderBarrier(ID3D11DeviceContext* context, UINT srvNum, UINT uavNum)
	{
		std::vector<ID3D11ShaderResourceView*> nullSRV(srvNum, 0);
		context->CSSetShaderResources(0, srvNum, nullSRV.data());

		std::vector<ID3D11UnorderedAccessView*> nullUAV(uavNum, 0);
		context->CSSetUnorderedAccessViews(0, uavNum, nullUAV.data(), NULL);

		context->CSSetShader(nullptr, 0, 0);
	}
}