#include "pch.h"
#include "BillboardEffectsActor.h"
#include "ParticleManager.h"

namespace DE {
	BillboardEffectsActor::BillboardEffectsActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\SingleTexture.json");
		m_particle->SetTarget(this);

		m_sprite = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\SpriteAnim.json");
		m_sprite->SetTarget(this);

		m_textures = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\TextureArray.json");
		m_textures->SetTarget(this);

		m_centerWhite = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\BillboardRenderModule\\CenterWhite.json");
		m_centerWhite->SetTarget(this);
	}

	BillboardEffectsActor::~BillboardEffectsActor()
	{
		ParticleManager::Get().DestroyInstance(m_sprite);
		ParticleManager::Get().DestroyInstance(m_textures);
		ParticleManager::Get().DestroyInstance(m_centerWhite);
	}
}