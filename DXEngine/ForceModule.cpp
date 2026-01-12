#include "pch.h"
#include "ForceModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void ForceModule::OnSpawn(ID3D11DeviceContext* context)
	{
		ParticleConsts& consts = m_owner->GetConsts();
		consts.velocity = velocity;
		consts.speedRange = speedRange;
		consts.randomDir = randomDir;
		consts.gravity = gravity;
		consts.drag = drag;
	}
}