#include "pch.h"
#include "ParticleModule.h"

namespace DE {
	void ParticleModule::Initialize(ID3D11Device* device, ParticleEmitter* owner)
	{
		m_owner = owner;
	}
}