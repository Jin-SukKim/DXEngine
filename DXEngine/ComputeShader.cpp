#include "pch.h"
#include "ComputeShader.h"

namespace DE {
	void ComputeShader::Initialize(ID3D11Device* device, const std::wstring& csName)
	{
		D3D11Utils::CreateCS(device, csName, m_cs);
	}

	void ComputeShader::Dispatch(ID3D11DeviceContext* context, UINT groupX, UINT groupY, UINT groupZ)
	{
		context->CSSetShader(m_cs.Get(), 0, 0);
		context->Dispatch(groupX, groupY, groupZ);
		ComputeShaderBarrier(context);
	}

	void ComputeShader::UpdateConsts(ID3D11DeviceContext* context, UINT startSlot, UINT numBuffers, ID3D11Buffer* const* ppConsts)
	{
		context->CSSetConstantBuffers(startSlot, numBuffers, ppConsts);
	}

	void ComputeShader::SetSRVs(ID3D11DeviceContext* context, UINT startSlot, const std::vector<ID3D11ShaderResourceView*>& srvs)
	{
		m_srvs = srvs;
		context->CSSetShaderResources(startSlot, static_cast<UINT>(srvs.size()), srvs.data());
	}

	void ComputeShader::SetUAVs(ID3D11DeviceContext* context, UINT startSlot, const std::vector<ID3D11UnorderedAccessView*>& uavs, const UINT* initCounts)
	{
		m_uavs = uavs;
		context->CSSetUnorderedAccessViews(startSlot, static_cast<UINT>(uavs.size()), uavs.data(), initCounts);
	}

	void ComputeShader::ComputeShaderBarrier(ID3D11DeviceContext* context)
	{
		std::fill(m_srvs.begin(), m_srvs.end(), nullptr);
		context->CSSetShaderResources(0, static_cast<UINT>(m_srvs.size()), m_srvs.data());

		std::fill(m_uavs.begin(), m_uavs.end(), nullptr);
		context->CSSetUnorderedAccessViews(0, static_cast<UINT>(m_uavs.size()), m_uavs.data(), NULL);

		context->CSSetShader(nullptr, 0, 0);
	}
}