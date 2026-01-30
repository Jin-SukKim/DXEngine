#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleLoader.h"
#include "TransformComponent.h"
#include "TextureManager.h"
#include "SpawnModule.h"
#include "Mesh.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "IndirectArgsBuffer.h"
#include "StructuredBuffer.h"

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

		ParticleManager::Get().UnregisterActiveSystem(this);
	}

	ParticleSystem::ParticleSystem(const ParticleSystem& other)
		: Object(other.m_watcherID + L"_Clone")
		, m_looping(other.m_looping)
		, m_duration(other.m_duration)
		, m_playRate(other.m_playRate)
		, m_preWarmTime(other.m_preWarmTime)
		, m_state(other.m_state)
		, m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)  // Hot-Reload는 복사 안 함
		, m_vertexCount(other.m_vertexCount)
		, m_indexCount(other.m_indexCount)
	{
		// Emitter 복제
		for (const auto& emitter : other.m_emitters) {
			if (emitter) {
				auto clonedEmitter = std::make_unique<ParticleEmitter>(*emitter);
				m_emitters.push_back(std::move(clonedEmitter));
			}
		}

		// CPU 데이터만 복사, GPU 버퍼는 Initialize()에서 생성
		m_consts.SetData(other.m_consts.GetCpu());
		m_frameConsts.SetData(other.m_frameConsts.GetCpu());
		m_meshConsts.SetCpuData(other.m_meshConsts.GetCpu());

		m_emitterIDs.resize(other.m_emitterIDs.size());
		for (UINT i = 0; i < m_maxEmitters; ++i) {
			m_emitterIDs[i].SetCpuData(other.m_emitterIDs[i].GetCpu());
		}

		// mesh 데이터도 CPU만 복사
		m_meshVertex.SetData(other.m_meshVertex.GetCpu());
		m_meshIndices.SetData(other.m_meshIndices.GetCpu());

		// baked 데이터도 CPU만 복사 (Initialize에서 GPU 버퍼 생성)
		m_bakedSpawnPos.SetData(other.m_bakedSpawnPos.GetCpu());

	}

	void ParticleSystem::Initialize()
	{
		ID3D11Device* device = GET_SINGLE(RenderBase)->GetDevice().Get();
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		for (UINT i = 0; i < 2; ++i) {
			m_particles[i] = StructuredBuffer<Particle>();
			m_particles[i].Initialize(device, m_maxTotalParticles);
		
			m_activeCounts[i] = StructuredBuffer<uint32_t>();
			m_activeCounts[i].Initialize(device, m_maxEmitters);

			std::vector<uint32_t> initialCount(m_maxEmitters, 0);
			m_activeCounts[i].SetData(initialCount);
			m_activeCounts[i].Upload(context);
		}

		m_consts.Initialize(device, m_maxEmitters);
		m_frameConsts.Initialize(device, m_maxEmitters);
		m_meshConsts.Initialize();

		for (auto& cb : m_emitterIDs)
			cb.Initialize();

		for (size_t i = m_emitterIDs.size(); i < m_maxEmitters; ++i) {
			ConstantBuffer<EmitterID> cb;
			cb.Initialize();
			m_emitterIDs.push_back(cb);
		}

		m_dispatchArgs = IndirectArgsBuffer<DispatchArgs>();
		std::vector<DispatchArgs> initialDispatch(m_maxEmitters, { 0, 1, 1 });
		m_dispatchArgs.Initialize(device, initialDispatch, m_maxEmitters, sizeof(DispatchArgs), 3);
		
		m_billboardArgsBuffer = IndirectArgsBuffer<DrawInstancedArgs>();
		std::vector<DrawInstancedArgs> initialBillboardArgs(m_maxEmitters, { 0, 1, 0, 0 });
		m_billboardArgsBuffer.Initialize(device, initialBillboardArgs, m_maxEmitters, sizeof(DrawInstancedArgs), 4);
		
		m_bakedSpawnPos.Initialize(device, m_maxTotalParticles);

		UpdateTransform();

		m_initMeshArgs = std::vector<DrawIndexedInstancedArgs>(m_maxEmitters, { 0, 0, 0, 0, 0 });
		for (auto& emitter : m_emitters) {
			emitter->SetOwner(this);
			emitter->SetMemoryInfo(m_currentParticleOffset, m_currentEmitterIndex);
			
			emitter->Initialize();  // 이제 m_emitterID가 올바르게 설정됨

			// EventCallback 등록
			emitter->SetEventCallback(
				[this](EmitterEvent event, ParticleEmitter* em) {
					this->OnEmitterEvent(event, em); // SubEmitter 생성 함수
				});

			// Buffer 메모리 offet, index 할당
			UINT capacity = m_frameConsts.Get(m_currentEmitterIndex).maxParticles;
			RegisterEmitter(emitter.get(), capacity);

			// SubEmitter 전부 등록
			LoadSubEmitter(emitter.get());
		}

		m_meshArgsBuffer = IndirectArgsBuffer<DrawIndexedInstancedArgs>();
		m_meshArgsBuffer.Initialize(device, m_initMeshArgs, m_maxEmitters, sizeof(DrawIndexedInstancedArgs), 5);

		if (m_state == ParticleState::Playing) Restart();
		else if (m_state == ParticleState::Paused) Pause();
		else if (m_state == ParticleState::Stopped) Stop();
	}

	void ParticleSystem::OnSpawn()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();

		// 1. 먼저 Baked 데이터 등록 (bakedCount 설정)
		for (auto& emitter : m_emitters) {
			// bakedPath가 설정되어 있으면 등록
			if (!emitter->GetBakedPath().empty()) {
				RegisterBakedPos(emitter.get());
			}
		}

		// 2. 그 다음 OnSpawn 호출 (이때 bakedCount가 이미 설정되어 있음)
		for (auto& emitter : m_emitters)
			emitter->OnSpawn();

		for (auto& id : m_emitterIDs) {
			id.Upload();
		}

		for (auto& emitter : m_subEmitterPool)
			emitter.second->OnSpawn();

		m_bakedSpawnPos.Upload(context);
		m_consts.Upload(context);
		context->CSSetShaderResources(8, 1, m_consts.GetAddressOfSRV());
		ExecutePreWarm();
		TextureManager::Get().BindParticleTextures();
	}

	void ParticleSystem::Update(const float& dt)
	{
		if (m_state != ParticleState::Playing)
			return;

		float newDt = dt * m_playRate;

		UpdateTransform();

		// Transform을 Compute Shader에 바인딩 (Spawn, Force 등에서 사용)
		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		// activeCount를 0으로 초기화
		const UINT clearVal[1] = { 0 };
		context->ClearUnorderedAccessViewUint(GetWriteCount().GetUAV(), clearVal);

		context->CSSetShaderResources(8, 1, m_consts.GetAddressOfSRV());

		if (m_vertexCount && m_indexCount) {
			ID3D11ShaderResourceView* srvs[] = {
				m_meshVertex.GetSRV(),
				m_meshIndices.GetSRV()
			};
			context->CSSetShaderResources(9, 2, srvs);
		}

		// Main Emitter 업데이트
		for (auto& emitter : m_emitters)
			emitter->PreUpdate(newDt);

		// Sub-Emitter 업데이트
		for (auto& emitter : m_activeSubEmitters)
			emitter->PreUpdate(newDt);

		m_frameConsts.Upload(context.Get());
		context->CSSetShaderResources(7, 1, m_frameConsts.GetAddressOfSRV());

		// Main Emitter 업데이트
		for (auto& emitter : m_emitters)
			emitter->Update(newDt);

		// Sub-Emitter 업데이트
		for (auto& emitter : m_activeSubEmitters)
			emitter->Update(newDt);


		ID3D11Buffer* nullB[] = {
			nullptr
		};
		context->CSSetConstantBuffers(4, 1, nullB);
		ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 5, nullSRVs);

		SwapBuffer();

		//// 완료된 Sub-Emitter 제거
		//std::erase_if(m_activeSubEmitters, [](const auto& em) {
		//	return em->IsCompleted();
		//	});

		// Looping이 아니고 모든 Emitter가 종료되면 Stop
		//if (!m_looping && IsAllEmittersCompleted())
		//	Stop();
		if (IsAllEmittersCompleted()) {
			if (m_looping)
				// 루핑이 켜져있다면 재시작 (Stop -> Reset -> Play)
				Restart();
			else
				// 루핑이 꺼져있다면 완전 정지
				Stop();
		}
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

		// Transform Constant Buffer 바인딩 (Slot 1)
		auto context = GET_SINGLE(RenderBase)->GetContext();

		context->VSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());
		ID3D11ShaderResourceView* srvs[2] = {
			m_frameConsts.GetSRV(),
			m_consts.GetSRV()
		};
		context->CSSetShaderResources(7, 2, srvs);
		context->VSSetShaderResources(7, 2, srvs);
		context->VSSetShaderResources(7, 2, srvs);
		context->PSSetShaderResources(7, 2, srvs);

		// 주 Emitter 렌더링
		for (auto& emitter : m_emitters)
			emitter->Render();

		// 동적 Sub-Emitter 렌더링
		for (auto& emitter : m_activeSubEmitters)
			emitter->Render();
	}

	void ParticleSystem::AddEmitter(const std::string& path)
	{
		std::wstring name(path.begin(), path.end());

		std::unique_ptr emitter = ParticleLoader::Load<ParticleEmitter>(name);
		if (emitter) {
			emitter->SetOwner(this);
			emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
				this->OnEmitterEvent(event, em);
				});
			m_emitters.emplace_back(std::move(emitter));
		}
	}

	void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter)
	{
		if (emitter) {
			emitter->SetOwner(this);
			emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
				this->OnEmitterEvent(event, em);
				});
			m_emitters.emplace_back(std::move(emitter));
		}
	}

	void ParticleSystem::ClearEmitters()
	{
		m_emitters.clear();
		m_subEmitterPool.clear();
		m_activeSubEmitters.clear();
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
	}
	void ParticleSystem::Restart()
	{
		Stop();
		//Reset(); // 버퍼 비우기
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
		if (modelIdx >= 0) 
			SetTargetMesh(modelIdx);

		// Transform 초기 설정
		UpdateTransform();
	}

	bool ParticleSystem::IsAllEmittersCompleted() const
	{
		// Looping이면 절대 완료 안됨
		//if (m_looping)
		//	return false;

		// 주 Emitter 체크
		for (const auto& emitter : m_emitters) {
			if (!emitter->IsCompleted())
				return false;
		}

		// 동적 Sub-Emitter 체크
		if (!m_activeSubEmitters.empty())
			return false;

		return true;
	}

	void ParticleSystem::BindConstantID(UINT emitterID)
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(5, 1, m_emitterIDs[emitterID].GetAddressOf());
		context->PSSetConstantBuffers(5, 1, m_emitterIDs[emitterID].GetAddressOf());
		context->VSSetConstantBuffers(5, 1, m_emitterIDs[emitterID].GetAddressOf());
	}

	void ParticleSystem::Reset()
	{
		for (auto& emitter : m_emitters)
			emitter->Reset();
		
		m_activeSubEmitters.clear();
	}

	void ParticleSystem::ExecutePreWarm()
	{
		if (m_preWarmTime <= 0.f) return;

		// 고정 프레임(60FPS)으로 시뮬레이션
		static const float step = 1.f / 60.f;
		float t = 0.f;

		while (t < m_preWarmTime)
		{
			t += step;

			for (auto& emitter : m_emitters)
				emitter->Update(step);
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

	void ParticleSystem::RegisterEmitter(ParticleEmitter* emitter, uint32_t capacity)
	{
		// Emitter에게 할당된 영역 정보를 설정
		EmitterID& id = m_emitterIDs[m_currentEmitterIndex].GetCpu();
		id.emitterID = m_currentEmitterIndex;
		id.particleOffset = m_currentParticleOffset;

		m_currentParticleOffset += capacity;
		++m_currentEmitterIndex;
	}

	void ParticleSystem::RegisterBakedPos(ParticleEmitter* emitter)
	{
		auto it = m_bakedOffset.find(emitter->GetBakedPath());
		if (it != m_bakedOffset.end()) {
			emitter->SetBakedInfo(it->second.first);
			m_consts.Get(emitter->GetEmitterID()).spawn.bakedCount = it->second.second;
			return;
		}

		emitter->SetBakedInfo(m_currentBakedOffset);
		auto& positions = m_bakedSpawnPos.GetCpu();
		EmitterID& id = m_emitterIDs[emitter->GetEmitterID()].GetCpu();
		id.bakedOffset = m_currentBakedOffset;

		UINT bakedCount = emitter->LoadBakedSpawnData(m_bakedSpawnPos);
		m_currentBakedOffset += bakedCount;
		m_consts.Get(emitter->GetEmitterID()).spawn.bakedCount = bakedCount;
		m_bakedOffset[emitter->GetBakedPath()] = {id.bakedOffset, bakedCount};
	}

	void ParticleSystem::OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter)
	{
		// 해당 Event에 대응하는 Sub-Emitter 생성
		for (const auto& sub : emitter->GetSubEmitters()) {
			if (sub.trigger == event) {
				Vector3 pos = sub.inheritPosition ? emitter->GetSpawnPosition() : Vector3(0.f);
				SpawnSubEmitter(sub, pos);
			}
		}
	}
	void ParticleSystem::SpawnSubEmitter(const SubEmitter& sub, const Vector3& position)
	{
		// 풀에서 해당 경로의 Emitter 중 현재 사용 중이지 않은 것 찾기
		ParticleEmitter* target = nullptr;

		auto it = m_subEmitterPool.find(sub.emitterPath);
		if (it == m_subEmitterPool.end())
			return;

		target = it->second.get();

		// 상태 리셋 및 활성화
		target->Reset();
		if (sub.inheritPosition) target->SetSpawnOffset(position);
		target->OnSpawn();

		m_activeSubEmitters.push_back(target);
	}

	void ParticleSystem::LoadSubEmitter(ParticleEmitter* emitter)
	{
		auto subEmitters = emitter->GetSubEmitters();
		for (const auto& sub : subEmitters) {
			auto subEmitter = ParticleLoader::Load<ParticleEmitter>(sub.emitterPath);
			if (!subEmitter)
				return;

			subEmitter->SetOwner(this);
			subEmitter->SetMemoryInfo(m_currentParticleOffset, m_currentEmitterIndex);

			subEmitter->Initialize();  // 이제 m_emitterID가 올바르게 설정됨

			subEmitter->SetName(sub.emitterPath);
			// EventCallback 등록
			subEmitter->SetEventCallback(
				[this](EmitterEvent event, ParticleEmitter* em) {
					this->OnEmitterEvent(event, em); // SubEmitter 생성 함수
				});

			// Buffer 메모리 offet, index 할당
			UINT capacity = m_frameConsts.Get(m_currentEmitterIndex).maxParticles;
			RegisterEmitter(subEmitter.get(), capacity);

			// SubEmitter 전부 등록
			LoadSubEmitter(subEmitter.get());

			m_subEmitterPool[sub.emitterPath] = std::move(subEmitter);
		}
	}

	void ParticleSystem::SetSpawnOffset(const Vector3& offset)
	{
		for (auto& emitter : m_emitters) {
			emitter->SetSpawnOffset(offset);
		}
	}
}