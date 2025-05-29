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
		depthStencilStates	= pso.depthStencilStates;
		rasterizerState		= pso.rasterizerState;
		for (int i = 0; i < 4; ++i)
			blendFactor[i]	= pso.blendFactor[i];
		stencilRef			= pso.stencilRef;

		pritivieTopology	= pso.pritivieTopology;
	}
	void GraphicsPSO::SetBlendFactor(const float blendFactor[4])
	{
		memcpy(this->blendFactor, blendFactor, sizeof(float) * 4);
	}
}