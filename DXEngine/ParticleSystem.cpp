#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleLoader.h"
#include "TransformComponent.h"
#include "TextureManager.h"
#include "SpawnModule.h"
#include "Mesh.h"

namespace DE {
	ParticleSystem::ParticleSystem(const std::wstring& name) : Object(name)
	{
	}

	ParticleSystem::~ParticleSystem()
	{
		if (m_watcherID != 0 && !m_jsonPath.empty()) {
			try {
				// FileWatcher가 유효한지 확인
				FileWatcher::Get().Unregister(m_jsonPath, m_watcherID);
			}
			catch (...) {
				// 프로그램 종료 시 무시
			}
		}	
	}

	ParticleSystem::ParticleSystem(const ParticleSystem& other)
		: Object(other.m_watcherID + L"_Clone")
		, m_looping(other.m_looping)
		, m_duration(other.m_duration)
		, m_playRate(other.m_playRate)
		, m_time(0.0f)  // 시간은 0으로 초기화
		, m_preWarmTime(other.m_preWarmTime)
		, m_state(other.m_state)
		, m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)  // Hot-Reload는 복사 안 함
	{
		// Emitter 복제
		for (const auto& emitter : other.m_emitters) {
			if (emitter) {
				auto clonedEmitter = std::make_unique<ParticleEmitter>(*emitter);
				m_emitters.push_back(std::move(clonedEmitter));
			}
		}

		// Transform 복사
		m_transform = other.m_transform;
	}

	void ParticleSystem::Initialize()
	{
		m_transform.Initialize();
		for (auto& emitter : m_emitters)
			emitter->Initialize();

		if (m_state == ParticleState::Playing) Restart();
		else if (m_state == ParticleState::Paused) Pause();
		else if (m_state == ParticleState::Stopped) Stop();
	}

	void ParticleSystem::OnSpawn()
	{
		for (auto& emitter : m_emitters)
			emitter->OnSpawn();

		ExecutePreWarm();
		TextureManager::Get().BindParticleTextures();
	}

	void ParticleSystem::Update(const float& dt)
	{
		if (m_state != ParticleState::Playing)
			return;

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

		// Transform을 Compute Shader에 바인딩 (Spawn, Force 등에서 사용)
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(1, 1, m_transform.GetAddressOf());

		for (auto& emitter : m_emitters)
			emitter->Update(newDt, m_time);
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

		// Transform Constant Buffer 바인딩 (Slot 1)
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->VSSetConstantBuffers(1, 1, m_transform.GetAddressOf());

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
		if (data.contains("Name")) {
			std::string name = data["Name"];
			std::wstring wname(name.begin(), name.end());
			this->SetName(wname);
		}
		if (data.contains("Transform")) {
			/*auto jsonTr = data["Transform"];
			auto tr = this->GetComponent<TransformComponent>();
			if (tr) {
				if (jsonTr.contains("position"))
					tr->SetPos(JsonToVec3(jsonTr["position"]));
				if (jsonTr.contains("rotation"))
					tr->SetRotation(JsonToVec3(jsonTr["rotation"]));
				if (jsonTr.contains("size"))
					tr->SetScale(JsonToVec3(jsonTr["size"]));
			}*/
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

		if (data.contains("Looping")) m_looping = data["Looping"];
		if (data.contains("Duration")) m_duration = data["Duration"];
		if (data.contains("PlayRate")) m_playRate = data["PlayRate"];
		if (data.contains("PreWarmTime")) m_preWarmTime = data["PreWarmTime"];
		
		if (data.contains("State")) {
			std::string state = data["State"];
			//if (state == "Play")  Restart();
			//else if (state == "Pause") Pause();
			//else if (state == "Stop") Stop();

			if (state == "Play") m_state = ParticleState::Playing;
			else if (state == "Pause") m_state = ParticleState::Paused;
			else if (state == "Stop") m_state = ParticleState::Stopped;
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

	void ParticleSystem::SetTargetMesh(const int& modelIdx)
	{
		for (auto& emitter : m_emitters) {
			if (emitter) {
				emitter->SetTargetMesh(modelIdx);
			}
		}
	}

	void ParticleSystem::SetTransform(const MeshConstants& transform)
	{
		m_transform.SetCpuData(transform);
		m_transform.Upload();
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