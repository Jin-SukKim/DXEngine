#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleLoader.h"

namespace DE {
	ParticleSystem::ParticleSystem(const std::wstring& name) : Actor(name)
	{
	}

	void ParticleSystem::Initialize()
	{
		for (auto& emitter : m_emitters)
			emitter->Initialize();

		OnSpawn();
	}

	void ParticleSystem::OnSpawn()
	{
		for (auto& emitter : m_emitters)
			emitter->OnSpawn();
	}

	void ParticleSystem::Update(const float& dt)
	{
		for (auto& emitter : m_emitters)
			emitter->Update(dt);
	}

	void ParticleSystem::Render()
	{
		for (auto& emitter : m_emitters)
			emitter->Render();
	}

	void ParticleSystem::AddEmitter(const std::string& path)
	{
		std::wstring name(path.begin(), path.end());

		std::unique_ptr emitter = ParticleLoader::Load<ParticleEmitter>(name);
		m_emitters.emplace_back(std::move(emitter));
	}

	void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter)
	{
		m_emitters.emplace_back(std::move(emitter));
	}

	void ParticleSystem::ClearEmitters()
	{
		m_emitters.clear();
	}

	void ParticleSystem::LoadFromJson(const json& data)
	{
		if (data.contains("Transform")) {

		}
		if (data.contains("Emitters")) {
			auto& emitters = data["Emitters"];
			for (auto& name : emitters) {
				AddEmitter(name);
			}
		}
		if (data.contains("Looping")) m_looping = data["Looping"];
		if (data.contains("State")) m_state = data["State"];
	}
}