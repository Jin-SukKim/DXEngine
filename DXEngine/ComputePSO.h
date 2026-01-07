#pragma once

namespace DE {


class ComputePSO
{
public:
	void operator=(const ComputePSO& pso);
public:
	ComPtr<ID3D11ComputeShader> computeShader;
};
}

