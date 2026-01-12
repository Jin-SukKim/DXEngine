#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::OnSpawn(ID3D11DeviceContext* context)
	{
		m_owner->GetConsts().startColor = startColor;
		m_owner->GetConsts().endColor = endColor;
		m_owner->GetConsts().sizeRange = sizeRange;
	}
}