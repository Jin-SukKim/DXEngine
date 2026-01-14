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
	}

	void ParticleSystem::OnSpawn()
	{
		for (auto& emitter : m_emitters)
			emitter->OnSpawn();

		ExecutePreWarm();
	}

	void ParticleSystem::Update(const float& dt)
	{
		if (m_state != ParticleState::Playing)
			return;

		Actor::Update(dt);

		float newDt = dt * m_playRate;
		m_time += newDt;

		// Loop 및 종료 체크
		if (m_duration <= m_time) {
			if (m_looping) {
				m_time = 0.f;
			}
			else {
				m_time = m_duration;
				Stop();
				return;
			}
		}

		for (auto& emitter : m_emitters)
			emitter->Update(newDt, m_time);
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

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
		if (data.contains("Looping")) m_looping = data["Looping"];
		if (data.contains("Duration")) m_duration = data["Duration"];
		if (data.contains("PlayRate")) m_playRate = data["PlayRate"];
		if (data.contains("PreWarmTime")) m_preWarmTime = data["PreWarmTime"];
		if (data.contains("State")) {
			std::string state = data["State"];
			if (state == "Play") Restart();
			else if (state == "Pause") Pause();
			else if (state == "Stop") Stop();
		}

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
	void ParticleSystem::Play()
	{
		if (m_state == ParticleState::Stopped)
			OnSpawn();
		m_state = ParticleState::Playing;
	}
	void ParticleSystem::Pause()
	{
		if (m_state == ParticleState::Playing)
			m_state = ParticleState::Paused;
	}
	void ParticleSystem::Stop()
	{
		m_state = ParticleState::Stopped;
		m_time = 0.f;
		Reset(); // 버퍼 비우기
	}
	void ParticleSystem::Restart()
	{
		Stop();
		Play();
	}

	void ParticleSystem::Reset()
	{
		for (auto& emitter : m_emitters)
		{
			emitter->Reset();
		}
	}

	void ParticleSystem::ExecutePreWarm()
	{
		if (m_preWarmTime <= 0.f) return;

		// 고정 프레임(60FPS)으로 시뮬레이션
		static const float step = 1.f / 60.f;
		float t = 0.f;

		while (t < m_preWarmTime)
		{
			//m_time += step; // 필요하면 활성화
			t += step;

			for (auto& emitter : m_emitters)
				emitter->Update(step, m_time);
		}
	}
}