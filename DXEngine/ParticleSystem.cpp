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
		m_meshConsts.Initialize();
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
		, m_currentBuffer(0)
		, m_currentParticleOffset(0)
		, m_currentEmitterIndex(0)
		, m_maxTotalParticles(0)
		, m_maxEmitters(0)
		, m_currentBakedOffset(0)
		, m_currentCustomOffset(0)
	{
		for (const auto& emitter : other.m_emitters) {
			if (emitter) {
				m_emitters.push_back(std::make_unique<ParticleEmitter>(*emitter));
			}
		}
		m_meshConsts.Initialize();
		m_meshConsts.SetCpuData(other.m_meshConsts.GetCpu());
	}

	void ParticleSystem::Initialize()
	{
		m_maxTotalParticles = 0;
		m_maxEmitters = 0;
		m_currentParticleOffset = 0;
		m_currentEmitterIndex = 0;
		m_currentBakedOffset = 0;
		m_currentCustomOffset = 0;
		m_bakedOffset.clear();
		m_emitterIDs.clear();
		m_subEmitterPool.clear();
		m_activeSubEmitters.clear();

		std::vector<ParticleConsts> consts;
		std::vector<ParticleFrameConsts> frameConsts;
		std::vector<DrawIndexedInstancedArgs> initMeshArgs;
		std::vector<Vector3> bakedPositions;
		std::vector<Vector3> customPositions;
		std::vector<EmitterID> emitterIDs;

		InitializeCPU(consts, frameConsts, initMeshArgs, bakedPositions, customPositions, emitterIDs);
		InitializeGPU(consts, frameConsts, initMeshArgs, bakedPositions, customPositions, emitterIDs);

		UpdateTransform();
		if (m_state == ParticleState::Playing) Restart();
		else if (m_state == ParticleState::Paused) Pause();
		else if (m_state == ParticleState::Stopped) Stop();
	}

	void ParticleSystem::InitializeCPU(
		std::vector<ParticleConsts>& consts,
		std::vector<ParticleFrameConsts>& frameConsts,
		std::vector<DrawIndexedInstancedArgs>& initMeshArgs,
		std::vector<Vector3>& bakedPositions,
		std::vector<Vector3>& customPositions,
		std::vector<EmitterID>& emitterIDs)
	{
		// 1단계: Main Emitter 처리
		for (auto& emitter : m_emitters) {
			ProcessEmitter(emitter.get(), consts, frameConsts, initMeshArgs,
				bakedPositions, customPositions, emitterIDs);
		}

		// 2단계: SubEmitter 로드 (Main Emitter 처리 후)
		for (auto& emitter : m_emitters) {
			LoadSubEmitters(emitter.get(), consts, frameConsts, initMeshArgs,
				bakedPositions, customPositions, emitterIDs);
		}
	}

	void ParticleSystem::ProcessEmitter(
		ParticleEmitter* emitter,
		std::vector<ParticleConsts>& consts,
		std::vector<ParticleFrameConsts>& frameConsts,
		std::vector<DrawIndexedInstancedArgs>& initMeshArgs,
		std::vector<Vector3>& bakedPositions,
		std::vector<Vector3>& customPositions,
		std::vector<EmitterID>& emitterIDs)
	{
		ParticleConsts pConsts = {};
		ParticleFrameConsts pfConsts = {};
		DrawIndexedInstancedArgs pMeshArgs = { 0, 0, 0, 0, 0 };
		EmitterID eID = { 0, 0, 0, 0 };

		eID.emitterID = m_currentEmitterIndex;
		eID.particleOffset = m_currentParticleOffset;

		emitter->SetOwner(this);
		emitter->SetMemoryInfo(m_currentParticleOffset, m_currentEmitterIndex);
		emitter->Initialize(pConsts, pfConsts, pMeshArgs);

		emitter->SetEventCallback([this](EmitterEvent event, ParticleEmitter* em) {
			this->OnEmitterEvent(event, em);
		});

		if (!emitter->GetBakedPath().empty()) {
			RegisterBakedPos(emitter, bakedPositions, pConsts, eID);
		}
		else if (emitter->IsUsingCustomPositions()) {
			RegisterCustomPos(emitter, customPositions, eID);
		}

		UINT capacity = pfConsts.maxParticles;
		m_currentParticleOffset += capacity;
		++m_currentEmitterIndex;
		++m_maxEmitters;
		m_maxTotalParticles += capacity;

		consts.push_back(pConsts);
		frameConsts.push_back(pfConsts);
		initMeshArgs.push_back(pMeshArgs);
		emitterIDs.push_back(eID);
	}

	void ParticleSystem::LoadSubEmitters(
		ParticleEmitter* emitter,
		std::vector<ParticleConsts>& consts,
		std::vector<ParticleFrameConsts>& frameConsts,
		std::vector<DrawIndexedInstancedArgs>& initMeshArgs,
		std::vector<Vector3>& bakedPositions,
		std::vector<Vector3>& customPositions,
		std::vector<EmitterID>& emitterIDs)
	{
		// 먼저 현재 emitter의 SubEmitter 경로들을 복사 (iterator 무효화 방지)
		std::vector<SubEmitter> subEmittersCopy = emitter->GetSubEmitters();
		
		for (const auto& sub : subEmittersCopy) {
			// 이미 로드된 SubEmitter는 스킵
			if (m_subEmitterPool.contains(sub.emitterPath))
				continue;

			auto subEmitter = ParticleLoader::Load<ParticleEmitter>(sub.emitterPath);
			if (!subEmitter)
				continue;

			// 먼저 풀에 저장 (포인터 획득 전에)
			ParticleEmitter* rawPtr = subEmitter.get();
			m_subEmitterPool[sub.emitterPath] = std::move(subEmitter);

			// SubEmitter 초기화
			ProcessEmitter(rawPtr, consts, frameConsts, initMeshArgs,
				bakedPositions, customPositions, emitterIDs);

			// SubEmitter의 SubEmitter도 재귀적으로 로드
			LoadSubEmitters(rawPtr, consts, frameConsts, initMeshArgs,
				bakedPositions, customPositions, emitterIDs);
		}
	}

	void ParticleSystem::InitializeGPU(
		const std::vector<ParticleConsts>& consts,
		const std::vector<ParticleFrameConsts>& frameConsts,
		const std::vector<DrawIndexedInstancedArgs>& initMeshArgs,
		const std::vector<Vector3>& bakedPositions,
		const std::vector<Vector3>& customPositions,
		const std::vector<EmitterID>& emitterIDs)
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

		m_consts.SetData(consts);
		m_frameConsts.SetData(frameConsts);

		m_consts.Upload(context);
		m_frameConsts.Upload(context);

		if (!bakedPositions.empty()) {
			m_bakedSpawnPos.Initialize(device, (UINT)bakedPositions.size());
			m_bakedSpawnPos.SetData(bakedPositions);
			m_bakedSpawnPos.Upload(context);
		}
		if (!customPositions.empty()) {
			m_customPositions.Initialize(device, (UINT)customPositions.size());
			m_customPositions.SetData(customPositions);
			m_customPositions.Upload(context);
		}

		context->CSSetShaderResources(8, 1, m_consts.GetAddressOfSRV());

		for (size_t i = 0; i < emitterIDs.size(); ++i) {
			ConstantBuffer<EmitterID> cb;
			cb.Initialize();
			cb.SetCpuData(emitterIDs[i]);
			cb.Upload();
			m_emitterIDs.push_back(cb);
		}

		m_dispatchArgs = IndirectArgsBuffer<DispatchArgs>();
		std::vector<DispatchArgs> initialDispatch(m_maxEmitters, { 0, 1, 1 });
		m_dispatchArgs.Initialize(device, initialDispatch, m_maxEmitters, sizeof(DispatchArgs), 3);

		m_billboardArgsBuffer = IndirectArgsBuffer<DrawInstancedArgs>();
		std::vector<DrawInstancedArgs> initialBillboardArgs(m_maxEmitters, { 0, 1, 0, 0 });
		m_billboardArgsBuffer.Initialize(device, initialBillboardArgs, m_maxEmitters, sizeof(DrawInstancedArgs), 4);

		m_meshArgsBuffer = IndirectArgsBuffer<DrawIndexedInstancedArgs>();
		m_meshArgsBuffer.Initialize(device, initMeshArgs, m_maxEmitters, sizeof(DrawIndexedInstancedArgs), 5);
	}

	void ParticleSystem::OnSpawn()
	{
		// Main Emitter만 OnSpawn (SubEmitter는 이벤트 발생 시 활성화)
		for (auto& emitter : m_emitters) {
			emitter->OnSpawn();
		}

		ExecutePreWarm();
		TextureManager::Get().BindParticleTextures();
	}

	void ParticleSystem::Update(const float& dt)
	{
		if (m_state != ParticleState::Playing)
			return;

		ActivateSubEmitters();

		float newDt = dt * m_playRate;
		UpdateTransform();

		auto context = GET_SINGLE(RenderBase)->GetContext();
		context->CSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());

		const UINT clearVal[1] = { 0 };
		context->ClearUnorderedAccessViewUint(GetWriteCount().GetUAV(), clearVal);
		context->CSSetShaderResources(8, 1, m_consts.GetAddressOfSRV());

		if (m_vertexCount && m_indexCount) {
			ID3D11ShaderResourceView* srvs[] = { m_meshVertex.GetSRV(), m_meshIndices.GetSRV() };
			context->CSSetShaderResources(9, 2, srvs);
		}

		// PreUpdate (Main + Sub)
		for (auto& emitter : m_emitters)
			emitter->PreUpdate(newDt);
		for (auto* emitter : m_activeSubEmitters)
			emitter->PreUpdate(newDt);

		m_frameConsts.Upload(context.Get());
		context->CSSetShaderResources(7, 1, m_frameConsts.GetAddressOfSRV());

		// Update (Main + Sub)
		for (auto& emitter : m_emitters)
			emitter->Update(newDt);
		for (auto* emitter : m_activeSubEmitters)
			emitter->Update(newDt);

		SwapBuffer();

		UpdateArgs(context);

		// Cleanup
		ID3D11Buffer* nullB[] = { nullptr };
		context->CSSetConstantBuffers(4, 1, nullB);
		ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(6, 5, nullSRVs);

		// 완료된 SubEmitter 제거
		std::erase_if(m_activeSubEmitters, [](auto* em) { return em->IsCompleted(); });

		if (IsAllEmittersCompleted()) {
			m_looping ? Restart() : Stop();
		}
	}

	void ParticleSystem::ActivateSubEmitters()
	{
		// 대기 중인 SubEmitter 활성화
		for (auto& [emitter, pos] : m_pendingSubEmitters) {
			emitter->Reset();
			emitter->SetSpawnOffset(pos);
			emitter->OnSpawn();
			m_activeSubEmitters.push_back(emitter);
		}
		m_pendingSubEmitters.clear();
	}

	void ParticleSystem::UpdateArgs(Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
	{
		// ArgsUpdateCS
		auto& argsUpdateCS = RenderBase::computeCommon.particle.argsUpdateCS;
		context->CSSetShader(argsUpdateCS.computeShader.Get(), nullptr, 0);

		ID3D11UnorderedAccessView* uavs[] = { m_dispatchArgs.GetUAV(), m_billboardArgsBuffer.GetUAV() };
		context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
		context->CSSetShaderResources(0, 1, GetReadCount().GetAddressOfSRV());

		context->Dispatch((m_maxEmitters + 255) / 256, 1, 1);

		ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	}

	void ParticleSystem::Render()
	{
		if (m_state == ParticleState::Stopped)
			return;

		auto context = GET_SINGLE(RenderBase)->GetContext();

		context->VSSetConstantBuffers(6, 1, m_meshConsts.GetAddressOf());
		ID3D11ShaderResourceView* srvs[2] = {
			m_frameConsts.GetSRV(),
			m_consts.GetSRV()
		};
		context->CSSetShaderResources(7, 2, srvs);
		context->VSSetShaderResources(7, 2, srvs);
		context->PSSetShaderResources(7, 2, srvs);

		// Main Emitter 렌더링
		for (auto& emitter : m_emitters)
			emitter->Render();

		// Active SubEmitter 렌더링 (null 체크)
		for (auto* emitter : m_activeSubEmitters) {
			if (emitter)
				emitter->Render();
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

			// 중복 체크 후 추가
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

		// 이미 활성화된 경우 스킵
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

		static const float step = 1.f / 60.f;
		float t = 0.f;
		while (t < m_preWarmTime) {
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

		MeshConstants meshConsts;
		meshConsts.world = tr->GetTransformMatrix().Transpose();
		meshConsts.worldIT = meshConsts.world.Invert();
		this->SetTransform(meshConsts);
	}

	void ParticleSystem::RegisterEmitter(ParticleEmitter* emitter, uint32_t capacity, EmitterID& eID)
	{
		eID.emitterID = m_currentEmitterIndex;
		eID.particleOffset = m_currentParticleOffset;
		m_currentParticleOffset += capacity;
		++m_currentEmitterIndex;
		++m_maxEmitters;
		m_maxTotalParticles += capacity;
	}

	void ParticleSystem::RegisterBakedPos(ParticleEmitter* emitter, std::vector<Vector3>& positions, ParticleConsts& pConsts, EmitterID& eID)
	{
		auto it = m_bakedOffset.find(emitter->GetBakedPath());
		if (it != m_bakedOffset.end()) {
			emitter->SetBakedInfo(it->second.first);
			eID.bakedOffset = it->second.first;
			pConsts.spawn.bakedCount = it->second.second;
			return;
		}

		emitter->SetBakedInfo(m_currentBakedOffset);
		eID.bakedOffset = m_currentBakedOffset;
		UINT bakedCount = emitter->LoadBakedSpawnData(positions);
		m_currentBakedOffset += bakedCount;
		pConsts.spawn.bakedCount = bakedCount;
		m_bakedOffset[emitter->GetBakedPath()] = { eID.bakedOffset, bakedCount };
	}

	void ParticleSystem::RegisterCustomPos(ParticleEmitter* emitter, std::vector<Vector3>& customPos, EmitterID& eID)
	{
		emitter->SetCustomSpawnInfo(m_currentCustomOffset);
		eID.customOffset = m_currentCustomOffset;
		auto& positions = emitter->GetCustomPositions();
		for (const auto& pos : positions)
			customPos.push_back(pos);
		m_currentCustomOffset += static_cast<UINT>(positions.size());
	}

	void ParticleSystem::SetSpawnOffset(const Vector3& offset)
	{
		for (auto& emitter : m_emitters)
			emitter->SetSpawnOffset(offset);
	}
}