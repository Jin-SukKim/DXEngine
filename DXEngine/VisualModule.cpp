#include "pch.h"
#include "VisualModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VisualModule::OnSpawn(ID3D11DeviceContext* context)
	{
		ParticleConsts& consts = m_owner->GetConstsData();
		consts.startColor = startColor;
		consts.endColor = endColor;
		consts.sizeRange = sizeRange;
	}
}