#include "pch.h"
#include "GraphicsPSO.h"

namespace DE {
	void GraphicsPSO::operator=(const GraphicsPSO& pso)
	{
		inputLayout			= pso.inputLayout;
		vertexShader		= pso.vertexShader;
		pixelShader			= pso.pixelShader;
		hullShader			= pso.hullShader;
		domainShader		= pso.domainShader;
		geometryShader		= pso.geometryShader;
		blendState			= pso.blendState;
		depthStencilState = pso.depthStencilState;
		rasterizerState		= pso.rasterizerState;
		for (int i = 0; i < 4; ++i)
			blendFactor[i]	= pso.blendFactor[i];
		stencilRef			= pso.stencilRef;

		primitiveTopology = pso.primitiveTopology;
	}
	void GraphicsPSO::SetBlendFactor(const float blendFactor[4])
	{
		memcpy(this->blendFactor, blendFactor, sizeof(float) * 4);
	}
}