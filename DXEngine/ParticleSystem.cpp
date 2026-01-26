#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleLoader.h"
#include "TransformComponent.h"
#include "TextureManager.h"
#include "SpawnModule.h"
#include "Mesh.h"
#include "ModelManager.h"
#include "ParticleManager.h"

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
		ParticleManager::Get().DestroyInstance(this);
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
		m_meshConsts = other.m_meshConsts;
	}

	void ParticleSystem::Initialize()
	{
		m_meshConsts.Initialize();
		UpdateTransform();
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

		UpdateTransform();
		// Transform을 Compute Shader에 바인딩 (Spawn, Force 등에서 사용)
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		if (m_vertexCount && m_indexCount) {
			ID3D11ShaderResourceView* srvs[] = {
				m_meshVertex.GetSRV(),
				m_meshIndices.GetSRV()
			};
			context->CSSetShaderResources(9, 2, srvs);
		}

		for (auto& emitter : m_emitters)
			emitter->Update(newDt, m_time);

		ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 5, nullSRVs);
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

		// Transform Constant Buffer 바인딩 (Slot 1)
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->VSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		for (auto& emitter : m_emitters)
			emitter->Render();

		ID3D11Buffer* nullCB[] = { nullptr };
		context->VSSetConstantBuffers(6, 1, nullCB);
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
	}
	void ParticleSystem::Restart()
	{
		Stop();
		Reset(); // 버퍼 비우기
		Play();
	}

	void ParticleSystem::SetTargetMesh(const int& modelIdx)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		Model* target = ModelManager::Get().GetModel(modelIdx);
		if (!target || target->meshes.empty())
			return;

		const Mesh2& meshes = target->meshes[0];

		m_vertexCount = static_cast<UINT>(meshes.vertexCPU.size());
		m_indexCount = static_cast<UINT>(meshes.indexCPU.size());

		m_meshVertex.Initialize(device.Get(), m_vertexCount);
		m_meshIndices.Initialize(device.Get(), m_indexCount);

		std::vector<Vector3> vertices;
		for (const auto& vertex : meshes.vertexCPU) {
			vertices.push_back(vertex.position);
		}

		m_meshVertex.SetData(vertices);
		m_meshIndices.SetData(meshes.indexCPU);

		auto& cpuData = m_meshConsts.GetCpu();
		cpuData.vertexCount = m_vertexCount;
		cpuData.indexCount = m_indexCount;

		m_meshVertex.Upload(context.Get());
		m_meshIndices.Upload(context.Get());
		m_meshConsts.Upload();
	}

	void ParticleSystem::SetTransform(const MeshConstants& transform)
	{
		auto& cpuData = m_meshConsts.GetCpu();
		cpuData.world = transform.world;
		cpuData.worldIT = transform.worldIT;
		m_meshConsts.Upload();
	}

	void ParticleSystem::SetTarget(Actor* owner, const int& modelIdx)
	{
		if (modelIdx >= 0) {
			SetTargetMesh(modelIdx);
			// 타겟 메시 변경 후 재초기화
			Initialize();
			OnSpawn();
		}

		m_owner = owner;
		// Transform 초기 설정
		UpdateTransform();
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

	void ParticleSystem::UpdateTransform()
	{
		if (!m_owner) return;

		TransformComponent* tr = m_owner->GetComponent<TransformComponent>();
		if (!tr) return;

		// Transform 정보를 ParticleSystem에 전달
		MeshConstants meshConsts;
		meshConsts.world = tr->GetTransformMatrix().Transpose();
		meshConsts.worldIT = meshConsts.world.Invert();

		this->SetTransform(meshConsts);
	}
}