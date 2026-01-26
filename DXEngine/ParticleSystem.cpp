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
				FileWatcher::Get().Unregister(m_jsonPath, m_watcherID);
			}
			catch (...) {
			}
		}	
		ParticleManager::Get().UnregisterActiveSystem(this);
	}

	ParticleSystem::ParticleSystem(const ParticleSystem& other)
		: Object(other.m_watcherID + L"_Clone")
		, m_looping(other.m_looping)
		, m_duration(other.m_duration)
		, m_playRate(other.m_playRate)
		, m_time(0.0f)
		, m_preWarmTime(other.m_preWarmTime)
		, m_state(other.m_state)
		, m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)
		, m_vertexCount(other.m_vertexCount)
		, m_indexCount(other.m_indexCount)
	{
		for (const auto& emitter : other.m_emitters) {
			if (emitter) {
				auto clonedEmitter = std::make_unique<ParticleEmitter>(*emitter);
				m_emitters.push_back(std::move(clonedEmitter));
			}
		}

		// CPU 데이터만 복사, GPU 버퍼는 Initialize()에서 생성
		m_meshConsts.SetCpuData(other.m_meshConsts.GetCpuConst());
		
		// mesh 데이터도 CPU만 복사
		m_meshVertex.SetData(other.m_meshVertex.GetCpu());
		m_meshIndices.SetData(other.m_meshIndices.GetCpu());
	}

	void ParticleSystem::Initialize()
	{
		m_meshConsts.Initialize();
		UpdateTransform();
		
		for (auto& emitter : m_emitters) {
			emitter->Initialize();
			// 이벤트 콜백 등록
			emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
				this->OnEmitterEvent(event, em);
			});
		}

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

		UpdateTransform();
		
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		if (m_vertexCount && m_indexCount) {
			ID3D11ShaderResourceView* srvs[] = {
				m_meshVertex.GetSRV(),
				m_meshIndices.GetSRV()
			};
			context->CSSetShaderResources(9, 2, srvs);
		}

		// 주 Emitter 업데이트
		for (auto& emitter : m_emitters)
			emitter->Update(newDt, m_time);
		
		// 동적 Sub-Emitter 업데이트
		for (auto& emitter : m_dynamicEmitters)
			emitter->Update(newDt, m_time);
		
		// 완료된 Sub-Emitter 제거
		std::erase_if(m_dynamicEmitters, [](const auto& em) {
			return em->IsCompleted();
		});

		// 모든 Emitter 완료 체크 (Looping이 아닐 때만)
		if (!m_looping && IsAllEmittersCompleted()) {
			Stop();
		}

		ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 5, nullSRVs);
	}

	bool ParticleSystem::IsAllEmittersCompleted() const
	{
		// Looping이면 절대 완료 안됨
		if (m_looping)
			return false;
		
		// 주 Emitter 체크
		for (const auto& emitter : m_emitters) {
			if (!emitter->IsCompleted())
				return false;
		}
		
		// 동적 Sub-Emitter 체크
		if (!m_dynamicEmitters.empty())
			return false;
		
		return true;
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->VSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		// 주 Emitter 렌더링
		for (auto& emitter : m_emitters)
			emitter->Render();
		
		// 동적 Sub-Emitter 렌더링
		for (auto& emitter : m_dynamicEmitters)
			emitter->Render();

		ID3D11Buffer* nullCB[] = { nullptr };
		context->VSSetConstantBuffers(6, 1, nullCB);
	}

	void ParticleSystem::AddEmitter(const std::string& path)
	{
		std::wstring name(path.begin(), path.end());

		std::unique_ptr emitter = ParticleLoader::Load<ParticleEmitter>(name);
		if (emitter) {
			emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
				this->OnEmitterEvent(event, em);
			});
			m_emitters.emplace_back(std::move(emitter));
		}
	}

	void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter)
	{
		if (emitter) {
			emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
				this->OnEmitterEvent(event, em);
			});
			m_emitters.emplace_back(std::move(emitter));
		}
	}

	void ParticleSystem::ClearEmitters()
	{
		m_emitters.clear();
		m_dynamicEmitters.clear();
	}

	void ParticleSystem::LoadFromJson(const json& data)
	{
		if (data.contains("Name")) {
			std::string name = data["Name"];
			std::wstring wname(name.begin(), name.end());
			this->SetName(wname);
		}

		ClearEmitters();
		if (data.contains("Emitters")) {
			for (const auto& file : data["Emitters"]) {
				std::string s = file;
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
		m_dynamicEmitters.clear();
	}
	
	void ParticleSystem::Restart()
	{
		Stop();
		Reset();
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
		m_owner = owner;
		
		if (modelIdx >= 0) {
			SetTargetMesh(modelIdx);
		}

		UpdateTransform();
	}

	void ParticleSystem::Reset()
	{
		for (auto& emitter : m_emitters)
		{
			emitter->Reset();
		}
		m_dynamicEmitters.clear();
	}

	void ParticleSystem::ExecutePreWarm()
	{
		if (m_preWarmTime <= 0.f) return;

		static const float step = 1.f / 60.f;
		float t = 0.f;

		while (t < m_preWarmTime)
		{
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

		MeshConstants meshConsts;
		meshConsts.world = tr->GetTransformMatrix().Transpose();
		meshConsts.worldIT = meshConsts.world.Invert();

		this->SetTransform(meshConsts);
	}

	void ParticleSystem::OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter)
	{
		// 해당 이벤트에 대응하는 Sub-Emitter 생성
		for (const auto& entry : emitter->GetSubEmitters()) {
			if (entry.trigger == event) {
				Vector3 position = entry.inheritPosition ? emitter->GetSpawnPosition() : Vector3(0.f);
				SpawnSubEmitter(entry, position);
			}
		}
	}

	void ParticleSystem::SpawnSubEmitter(const SubEmitterEntry& entry, const Vector3& position)
	{
		auto subEmitter = ParticleLoader::Load<ParticleEmitter>(entry.emitterPath);
		if (!subEmitter) return;
		
		// 위치 상 속
		if (entry.inheritPosition) {
			subEmitter->SetSpawnOffset(position);
		}
		
		// Sub-Emitter도 이벤트 콜백 등록 (중첩 지원)
		subEmitter->SetEventCallback([this](EmitterEvent ev, ParticleEmitter* em) {
			this->OnEmitterEvent(ev, em);
		});
		
		subEmitter->Initialize();
		subEmitter->OnSpawn();
		
		m_dynamicEmitters.push_back(std::move(subEmitter));
	}
}