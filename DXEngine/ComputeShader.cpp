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

	void ComputeShader::DispatchIndirect(ID3D11DeviceContext* context, ID3D11Buffer* args)
	{
		context->CSSetShader(m_cs.Get(), 0, 0);
		context->DispatchIndirect(args, 0);
		ComputeShaderBarrier(context);
	}

	void ComputeShader::UpdateConsts(ID3D11DeviceContext* context, UINT startSlot, UINT numBuffers, ID3D11Buffer* const* ppConsts)
	{
		context->CSSetConstantBuffers(startSlot, numBuffers, ppConsts);
	}

	void ComputeShader::ComputeShaderBarrier(ID3D11DeviceContext* context)
	{
		ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
		ID3D11UnorderedAccessView* nullUAVs[8] = { nullptr };

		context->CSSetShaderResources(0, 8, nullSRVs);
		context->CSSetUnorderedAccessViews(0, 8, nullUAVs, nullptr);

		context->CSSetShader(nullptr, 0, 0);
	}
}