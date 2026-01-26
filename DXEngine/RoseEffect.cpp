#include "pch.h"
#include "RoseEffect.h"
#include "ParticleManager.h"
#include "ParticleSystem.h"

namespace DE {
RoseEffect::RoseEffect(const std::wstring& name) : Super(name)
{
	m_orbit = ParticleManager::Get().CreateSystem(L"Particles\\OrbitEffect.json");
	m_orbit->SetTarget(this);
}

RoseEffect::~RoseEffect()
{
	ParticleManager::Get().DestroyInstance(m_orbit);
}

void RoseEffect::Initialize()
{
}

void RoseEffect::Update(const float& deltaTime)
{
}

bool RoseEffect::IsFinished() const
{
	return false;
}

}
