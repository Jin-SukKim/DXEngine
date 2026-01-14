#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleLoader.h"
#include "TransformComponent.h"

namespace DE {
	ParticleSystem::ParticleSystem(const std::wstring& name) : Actor(name)
	{
	}

	ParticleSystem::~ParticleSystem()
	{
		if (m_watcherID)
			FileWatcher::Get().Unregister(m_jsonPath, m_watcherID);
	}

	void ParticleSystem::Initialize()
	{
		Actor::Initialize();
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
		Actor::Update(dt);
		for (auto& emitter : m_emitters)
			emitter->Update(dt);
	}

	void ParticleSystem::Render()
	{
		Actor::Render();
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
			auto tr = this->GetComponent<TransformComponent>();
			if (tr) {
				if (data.contains("position")) tr->SetPos(JsonToVec3(data["position"]));
				if (data.contains("rotation")) tr->SetRotation(JsonToVec3(data["rotation"]));
				if (data.contains("size")) tr->SetScale(JsonToVec3(data["size"]));
			}
		}
		if (data.contains("Emitters")) {
			auto& emitters = data["Emitters"];
			for (auto& name : emitters) {
				AddEmitter(name);
			}
		}
		if (data.contains("Looping")) m_looping = data["Looping"];
		if (data.contains("State")) m_state = data["State"];

		ClearEmitters();
		if (data.contains("Emitters")) {
			for (const auto& file : data["Emitters"]) {
				std::string s = file;
				// Emitter 로드 (각각의 FileWatcher가 등록됨)
				auto emitter = ParticleLoader::Load<ParticleEmitter>(std::wstring(s.begin(), s.end()));
				if (emitter)
					this->AddEmitter(std::move(emitter));
			}
		}
	}
	
	void ParticleSystem::SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id)
	{
		m_jsonPath = path;
		m_watcherID = id;
	}
}