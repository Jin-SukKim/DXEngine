#include "pch.h"
#include "BillboardEffectsActor.h"
#include "ParticleManager.h"

namespace DE {
	BillboardEffectsActor::BillboardEffectsActor(const std::wstring& name) : Super(name)
	{
		m_single = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\SingleTexture.json");
		m_single->SetTarget(this);

		m_sprite = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\SpriteAnim.json");
		m_sprite->SetTarget(this);

		m_textures = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\TextureArray.json");
		m_textures->SetTarget(this);
	}

	BillboardEffectsActor::~BillboardEffectsActor()
	{
		ParticleManager::Get().DestroyInstance(m_single);
		ParticleManager::Get().DestroyInstance(m_sprite);
		ParticleManager::Get().DestroyInstance(m_textures);
	}

	void BillboardEffectsActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

		if (m_particle->IsStopped())
			return;
	}
}