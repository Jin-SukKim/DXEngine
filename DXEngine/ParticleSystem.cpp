#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleLoader.h"
#include "TransformComponent.h"
#include "TextureManager.h"
#include "SpawnModule.h"
#include "RenderModule.h"
#include "Mesh.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "IndirectArgsBuffer.h"
#include "StructuredBuffer.h"

namespace DE {
	ParticleSystem::ParticleSystem(const std::wstring& name) : Object(name)
	{
		m_meshConsts.Initialize();
		m_creationTime = 0.0f;
		m_basePriority = 0.5f;
	}

	ParticleSystem::~ParticleSystem()
	{
		if (m_watcherID != 0 && !m_jsonPath.empty()) {
			try {
				FileWatcher::Get().Unregister(m_jsonPath, m_watcherID);
			}
			catch (...) {}
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
		, m_watcherID(0)
		, m_vertexCount(other.m_vertexCount)
		, m_indexCount(other.m_indexCount)
		, m_owner(nullptr)
		, m_currentEmitterIndex(0)
		, m_maxTotalParticles(0)
		, m_maxEmitters(0)
		, m_currentParticleOffset(0)
	{
		// Main Emitter ����
		for (const auto& emitter : other.m_emitters) {
			if (emitter) {
				m_emitters.push_back(std::make_unique<ParticleEmitter>(*emitter));
			}
		}
		m_meshConsts.Initialize();
		m_meshConsts.SetCpuData(other.m_meshConsts.GetCpu());

		// SubEmitter ����
		for (const auto& [path, emitterPtr] : other.m_subEmitterPool) {
			if (emitterPtr) {
				m_subEmitterPool[path] = std::make_unique<ParticleEmitter>(*emitterPtr);
			}
		}
	}

	void ParticleSystem::Initialize()
	{
		m_maxTotalParticles = 0;
		m_currentParticleOffset = 0;
		m_maxEmitters = 0;
		m_currentEmitterIndex = 0;
		m_currentSpawnPosOffset = 0;  // ���յ� offset
		m_spawnPosCache.clear();      // ���յ� ĳ��
		m_subEmitterPool.clear();
		m_activeSubEmitters.clear();

		ParticleInitializer initialData;
		InitializeCPU(initialData);
		UpdateTransform();
	}

	void ParticleSystem::Initialize(ParticleInitializer& initialData)
	{
		UpdateTransform();
		if (m_state == ParticleState::Playing) Restart();
		else if (m_state == ParticleState::Paused) Pause();
		else if (m_state == ParticleState::Stopped) Stop();
	}

	void ParticleSystem::InitializeCPU(
		ParticleInitializer& initialData)
	{
		std::set<std::wstring> processedPaths;

		// 1. Main Emitter ó�� ���� ����
		for (auto& emitter : m_emitters) {
			ProcessEmitter(emitter.get(), initialData);
		}


		// 2. SubEmitter ó�� ���� ���� (���Ⱑ ���� ���� ����)
		for (auto& emitter : m_emitters) {
			LoadSubEmitters(emitter.get(), initialData, processedPaths);
		}

		m_initialData = initialData;
	}

	void ParticleSystem::ProcessEmitter(
		ParticleEmitter* emitter,
		ParticleInitializer& initialData)
	{
		ParticleConsts pConsts = {};
		ParticleFrameConsts pfConsts = {};
		DrawIndexedInstancedArgs pMeshArgs = { 0, 0, 0, 0, 0 };
		EmitterID eID = { 0, 0, 0, 0 };

		eID.emitterID = m_currentEmitterIndex;
		eID.readParticleOffset = m_currentParticleOffset;
		eID.writeParticleOffset = m_currentParticleOffset;

		emitter->SetOwner(this);
		emitter->SetMemoryInfo(m_currentEmitterIndex);
		emitter->Initialize(pConsts, pfConsts, pMeshArgs);

		emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
			this->OnEmitterEvent(event, em);
		});

		// ���յ� SpawnPosition ó��
		RegisterSpawnPositions(emitter, initialData.spawnPositions, pConsts, eID);

		UINT capacity = pfConsts.maxParticles;

		m_currentParticleOffset += capacity;
		++m_currentEmitterIndex;
		++m_maxEmitters;
		m_maxTotalParticles += capacity;

		initialData.consts.push_back(pConsts);
		initialData.frameConsts.push_back(pfConsts);
		initialData.initMeshArgs.push_back(pMeshArgs);
		initialData.emitterIDs.push_back(eID);
	}

	void ParticleSystem::LoadSubEmitters(
		ParticleEmitter* emitter,
		ParticleInitializer& initialData, std::set<std::wstring>& processedPaths)
	{
		// ���� ���� emitter�� SubEmitter ��ε��� ���� (iterator ��ȿȭ ����)
		std::vector<SubEmitter> subEmittersCopy = emitter->GetSubEmitters();
		
		for (const auto& sub : subEmittersCopy) {
			ParticleEmitter* targetEmitter = nullptr;

			// [Pool �˻�]
			auto it = m_subEmitterPool.find(sub.emitterPath);
			if (it != m_subEmitterPool.end()) {
				// �� HIT: ���� ������ ���п� �̹� �޸𸮿� ������! (���� �ε� X)
				targetEmitter = it->second.get();
			}
			else {
				// MISS: Prototype�� ó�� ������ ���� ����� ���� (���� �ε� O)
				auto loaded = ParticleLoader::Load<ParticleEmitter>(sub.emitterPath);
				if (!loaded) continue;

				targetEmitter = loaded.get();
				m_subEmitterPool[sub.emitterPath] = std::move(loaded);
			}

			// [�ʱ�ȭ ����] Pool�� �־��� ���� �ε��ߵ�, �̹� �ý����� ���� �������� �Ҵ��ؾ� ��
			if (targetEmitter) {
				ProcessEmitter(targetEmitter, initialData);

				// ó�� �Ϸ� ǥ��
				processedPaths.insert(sub.emitterPath);

				// ��� ȣ�� (�ڽ��� �ڽ� ó��)
				LoadSubEmitters(targetEmitter, initialData, processedPaths);
			}
		}
	}

	void ParticleSystem::InitializeGPU(ParticleInitializer& initialData,
		IndirectArgsBuffer<DispatchArgs>& dispatchArgs,
		IndirectArgsBuffer<DrawIndexedInstancedArgs>& billboardArgsBuffer,
		IndirectArgsBuffer<DrawIndexedInstancedArgs>& meshArgsBuffer)
	{
		m_dispatchArgs = &dispatchArgs;
		m_billboardArgsBuffer = &billboardArgsBuffer;
		m_meshArgsBuffer = &meshArgsBuffer;
	}

	void ParticleSystem::OnSpawn()
	{
		// Main Emitter�� OnSpawn (SubEmitter�� �̺�Ʈ �߻� �� Ȱ��ȭ)
		for (auto& emitter : m_emitters) {
			emitter->OnSpawn();
		}

		//ExecutePreWarm(*m_dispatchArgs);
		TextureManager::Get().BindParticleTextures();
	}

	void ParticleSystem::PreUpdate(const float& dt, std::vector<ParticleFrameConsts>& fsConsts)
	{
		if (m_state != ParticleState::Playing)
			return;

		float newDt = dt * m_playRate;
		UpdateTransform();

		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		if (m_vertexCount && m_indexCount) {
			ID3D11ShaderResourceView* srvs[] = { m_meshVertex.GetSRV(), m_meshIndices.GetSRV() };
			context->CSSetShaderResources(2, 2, srvs);
		}

		// PreUpdate (Main + Sub)
		for (auto& emitter : m_emitters)
			emitter->PreUpdate(newDt, fsConsts[m_poolHandle.emitterIDs[emitter->GetEmitterID()]]);
		for (auto* emitter : m_activeSubEmitters)
			emitter->PreUpdate(newDt, fsConsts[m_poolHandle.emitterIDs[emitter->GetEmitterID()]]);
	}

	void ParticleSystem::Update(const float& dt)
	{
		if (m_state != ParticleState::Playing)
			return;

		float newDt = dt * m_playRate;
		// Update (Main + Sub)
		for (auto& emitter : m_emitters)
			emitter->Update(newDt, 
				{ m_dispatchArgs->GetBuffer(), 
				GetDispatchArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()]) 
				});
		for (auto* emitter : m_activeSubEmitters)
			emitter->Update(newDt,
				{ m_dispatchArgs->GetBuffer(),
				GetDispatchArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()])
				});

		auto context = GET_SINGLE(RenderBase)->GetContext();

		// �Ϸ�� SubEmitter ����
		std::erase_if(m_activeSubEmitters, [](auto* em) { return em->IsCompleted(); });

		ActivateSubEmitters();

		if (IsAllEmittersCompleted()) {
			m_looping ? Restart() : Stop();
		}
	}

	void ParticleSystem::ActivateSubEmitters()
	{
		// ��� ���� SubEmitter Ȱ��ȭ
		for (auto& [emitter, pos] : m_pendingSubEmitters) {
			emitter->Reset();
			emitter->SetSpawnOffset(pos);
			emitter->OnSpawn();
			m_activeSubEmitters.push_back(emitter);
		}
		m_pendingSubEmitters.clear();
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		context->VSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		// Main Emitter ������
		for (auto& emitter : m_emitters)
			emitter->Render({
				m_billboardArgsBuffer->GetBuffer(),
				GetBillboardArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()]) 
				},
				{
				m_meshArgsBuffer->GetBuffer(),
				GetMeshArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()])
				});

		// Active SubEmitter ������ (null üũ)
		for (auto* emitter : m_activeSubEmitters) {
			if (emitter)
				emitter->Render({
				m_billboardArgsBuffer->GetBuffer(),
				GetBillboardArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()])
					},
				{
				m_meshArgsBuffer->GetBuffer(),
				GetMeshArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()])
				});
		}
	}

	void ParticleSystem::OnEmitterEvent(EmitterEvent event, ParticleEmitter* emitter)
	{
		for (const auto& sub : emitter->GetSubEmitters()) {
			if (sub.trigger != event)
				continue;

			auto it = m_subEmitterPool.find(sub.emitterPath);
			if (it == m_subEmitterPool.end())
				continue;

			ParticleEmitter* subEmitter = it->second.get();
			Vector3 pos = sub.inheritPosition ? emitter->GetSpawnPosition() : Vector3(0.f);

			// �ߺ� üũ �� �߰�
			auto isMatch = [subEmitter](const auto& p) { return p.first == subEmitter; };
			bool alreadyPending = std::ranges::any_of(m_pendingSubEmitters, isMatch);
			bool alreadyActive = std::ranges::find(m_activeSubEmitters, subEmitter) != m_activeSubEmitters.end();

			if (!alreadyPending && !alreadyActive)
				m_pendingSubEmitters.emplace_back(subEmitter, pos);
		}
	}

	void ParticleSystem::ActivateSubEmitter(ParticleEmitter* subEmitter, const Vector3& position)
	{
		if (!subEmitter)
			return;

		// �̹� Ȱ��ȭ�� ��� ��ŵ
		for (auto* em : m_activeSubEmitters) {
			if (em == subEmitter)
				return;
		}

		subEmitter->Reset();
		subEmitter->SetSpawnOffset(position);
		subEmitter->OnSpawn();

		m_activeSubEmitters.push_back(subEmitter);
	}

	void ParticleSystem::AddEmitter(const std::string& path)
	{
		std::wstring name(path.begin(), path.end());
		auto emitter = ParticleLoader::Load<ParticleEmitter>(name);
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
			this->SetName(std::wstring(name.begin(), name.end()));
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
	}

	void ParticleSystem::Restart()
	{
		Stop();
		m_activeSubEmitters.clear();
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
		for (const auto& vertex : meshes.vertexCPU)
			vertices.push_back(vertex.position);

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
		UpdateTransform();
	}

	bool ParticleSystem::IsAllEmittersCompleted() const
	{
		for (const auto& emitter : m_emitters)
			if (!emitter->IsCompleted()) return false;

		if (!m_activeSubEmitters.empty())
			return false;

		return true;
	}

	void ParticleSystem::BindConstantID(UINT emitterID)
	{
		// Manager�� ���� ���ε�
		ParticleManager::Get().BindEmitterID(m_poolHandle.emitterIDs[emitterID]);
	}

	void ParticleSystem::Reset()
	{
		for (auto& emitter : m_emitters)
			emitter->Reset();
		m_activeSubEmitters.clear();
	}

	void ParticleSystem::ExecutePreWarm(IndirectArgsBuffer<DispatchArgs>& dispatchArgs)
	{
		if (m_preWarmTime <= 0.f) return;

		static const float step = 1.f / 60.f;
		float t = 0.f;
		while (t < m_preWarmTime) {
			t += step;
			for (auto& emitter : m_emitters) {
				emitter->Update(step,
					{ dispatchArgs.GetBuffer(),
				GetDispatchArgsOffset(
					m_poolHandle.emitterIDs[emitter->GetEmitterID()])
					});
			}
				
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

		// Pool�� ���ε� (System �ε��� ���)
		ParticleManager::Get().UpdateMeshConsts(m_poolHandle.systemSlot, meshConsts);
	}

	Vector3 ParticleSystem::GetWorldPosition() const
	{
		// owner가 있으면 TransformMatrix에서 위치 추출
		if (m_owner) {
			TransformComponent* tr = m_owner->GetComponent<TransformComponent>();
			if (tr) {
				// GetTransformMatrix()의 translation 부분 추출
				Matrix worldMat = tr->GetTransformMatrix();
				return Vector3(worldMat._41, worldMat._42, worldMat._43);
			}
		}

		// owner가 없으면 meshConsts에서 추출
		const auto& cpuData = m_meshConsts.GetCpu();
		// world 행렬의 4번째 열 (translation) - 이미 Transpose된 상태일 수 있음
		return Vector3(cpuData.world._41, cpuData.world._42, cpuData.world._43);
	}

	void ParticleSystem::RegisterSpawnPositions(
		ParticleEmitter* emitter, 
		std::vector<Vector3>& outPositions, 
		ParticleConsts& pConsts, 
		EmitterID& eID)
	{
		eID.spawnPosOffset = UINT_MAX;  // �⺻��: �̻��
		
		if (!emitter->GetBakedPath().empty()) {
			auto it = m_spawnPosCache.find(emitter->GetBakedPath());
			if (it != m_spawnPosCache.end()) {
				// ĳ�� ��Ʈ
				emitter->SetSpawnPosInfo(it->second.first);
				eID.spawnPosOffset = it->second.first;
				pConsts.spawn.bakedCount = it->second.second;
				return;
			}

			// ĳ�� �̽�
			emitter->SetSpawnPosInfo(m_currentSpawnPosOffset);
			eID.spawnPosOffset = m_currentSpawnPosOffset;
			
			UINT bakedCount = emitter->LoadBakedSpawnData(outPositions);
			pConsts.spawn.bakedCount = bakedCount;
			
			m_spawnPosCache[emitter->GetBakedPath()] = { m_currentSpawnPosOffset, bakedCount };
			m_currentSpawnPosOffset += bakedCount;
		}
		// Custom Position ó��
		else if (emitter->IsUsingCustomPositions()) {
			emitter->SetSpawnPosInfo(m_currentSpawnPosOffset);
			eID.spawnPosOffset = m_currentSpawnPosOffset;
			
			const auto& positions = emitter->GetCustomPositions();
			for (const auto& pos : positions)
				outPositions.push_back(pos);
			
			pConsts.spawn.bakedCount = static_cast<UINT>(positions.size());
			m_currentSpawnPosOffset += static_cast<UINT>(positions.size());
		}
	}

	void ParticleSystem::SetSpawnOffset(const Vector3& offset)
	{
		for (auto& emitter : m_emitters)
			emitter->SetSpawnOffset(offset);
	}

	bool ParticleSystem::HasMeshRenderModule() const
	{
		for (const auto& emitter : m_emitters) {
			if (emitter->GetModule<MeshRenderModule>() != nullptr) {
				return true;
			}
		}
		return false;
	}

	bool ParticleSystem::HasBillboardRenderModule() const
	{
		for (const auto& emitter : m_emitters) {
			if (emitter->GetModule<BillboardRenderModule>() != nullptr) {
				return true;
			}
		}
		return false;
	}

	void ParticleSystem::UpdateSpawnRatios(const Vector3& cameraPos)
	{
		Vector3 systemPos = GetWorldPosition();
		float distance = Vector3::Distance(systemPos, cameraPos);

		auto& memoryPool = ParticleManager::Get().GetMemoryPool();

		// Update spawn ratio for all emitters in this system
		for (auto& emitter : m_emitters) {
			auto& settings = emitter->GetOverdrawSettings();
			float spawnRatio = 1.0f;

			if (settings.enableSpawnLimiting) {
				// Calculate spawn ratio based on distance
				float t = (distance - settings.nearDistance) / (settings.farDistance - settings.nearDistance);
				t = std::max(0.0f, std::min(1.0f, t)); // clamp to [0, 1]
				spawnRatio = settings.nearSpawnRatio + t * (1.0f - settings.nearSpawnRatio);
			}

			// Set spawn ratio in render module
			auto* renderModule = emitter->GetModule<RenderModule>();
			if (renderModule) {
				renderModule->SetSpawnRatio(spawnRatio);
			}

			// Upload to GPU
			UINT globalEmitterID = m_poolHandle.emitterIDs[emitter->GetEmitterID()];
			memoryPool.UpdateRenderConst(globalEmitterID, spawnRatio);
		}

		// Also update active sub-emitters
		for (auto* emitter : m_activeSubEmitters) {
			if (!emitter) continue;

			auto& settings = emitter->GetOverdrawSettings();
			float spawnRatio = 1.0f;

			if (settings.enableSpawnLimiting) {
				float t = (distance - settings.nearDistance) / (settings.farDistance - settings.nearDistance);
				t = std::max(0.0f, std::min(1.0f, t));
				spawnRatio = settings.nearSpawnRatio + t * (1.0f - settings.nearSpawnRatio);
			}

			auto* renderModule = emitter->GetModule<RenderModule>();
			if (renderModule) {
				renderModule->SetSpawnRatio(spawnRatio);
			}

			// Upload to GPU
			UINT globalEmitterID = m_poolHandle.emitterIDs[emitter->GetEmitterID()];
			memoryPool.UpdateRenderConst(globalEmitterID, spawnRatio);
		}
	}
}