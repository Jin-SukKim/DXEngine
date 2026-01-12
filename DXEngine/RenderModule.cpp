#include "pch.h"
#include "RenderModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void RenderModule::Initialize(ID3D11Device* device, ParticleEmitter* owner)
	{
		ParticleModule::Initialize(device, owner);

		m_device = device;
	}

	void RenderModule::OnSpawn(ID3D11DeviceContext* context)
	{
		SetBlendState();
	}

	void RenderModule::SetBlendState()
	{
		switch (blendMode)
		{
		case BlendMode::Additive:
			RenderBase::graphicsCommon.particle.animPSO.blendState = RenderBase::graphicsCommon.accumulateBS;
			break;
		case BlendMode::AlphaBlend:
			RenderBase::graphicsCommon.particle.animPSO.blendState = RenderBase::graphicsCommon.alphaBS;
			break;
		case BlendMode::Opaque:
			RenderBase::graphicsCommon.particle.animPSO.blendState = nullptr;
			break;
		}
	}
}