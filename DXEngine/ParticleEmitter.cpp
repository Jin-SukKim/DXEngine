#include "pch.h"
#include "ParticleEmitter.h"
#include "RenderModule.h"
#include "ParticleModuleFactory.h"

#include "SpawnModule.h"
#include "VisualModule.h"
#include "ForceModule.h"
#include "VortexModule.h"
#include "RenderModule.h"	
#include "MaterialModule.h"	
#include "ParticleContext.h"
#include "Mesh.h"
#include "TextureSpawnBake.h"

namespace DE {

	ParticleEmitter::ParticleEmitter(const std::wstring& name) : m_name(name)
	{
	}

	ParticleEmitter::~ParticleEmitter()
	{
		if (m_watcherID != 0 && !m_jsonPath.empty()) {
			try {
				FileWatcher::Get().Unregister(m_jsonPath, m_watcherID);
			}
			catch (...) {
				// 프로그램 종료 시 무시
			}
		}
	}

	ParticleEmitter::ParticleEmitter(const ParticleEmitter& other)
		: m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)
		, m_bakedCount(other.m_bakedCount)
		, m_name(other.m_name)
		, m_duration(other.m_duration)
		, m_completionDelay(other.m_completionDelay)
		, m_subEmitters(other.m_subEmitters)
		, m_initialSpawnPos(other.m_initialSpawnPos)  // 추가
	{
		for (const auto& mod : other.m_modules) {
			if (mod) {
				auto clonedModule = mod->Clone();
				if (clonedModule)
					m_modules.push_back(std::move(clonedModule));
			}
		}

		// CPU 데이터만 복사, GPU 버퍼는 Initialize()에서 새로 생성
		m_consts.SetCpuData(other.m_consts.GetCpu());
		m_frameConsts.SetCpuData(other.m_frameConsts.GetCpu());

		// baked 데이터도 CPU만 복사 (Initialize에서 GPU 버퍼 생성)
		m_bakedSpawnPos.SetData(other.m_bakedSpawnPos.GetCpu());
		m_customPositions.SetData(other.m_customPositions.GetCpu());
	}

	void ParticleEmitter::Initialize()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		
		m_consts.Initialize();
		m_frameConsts.Initialize();

		ParticleInitContext initCtx = { 
			device.Get(), 
			m_consts.GetCpu(),
			m_frameConsts.GetCpu() 
		};

		// Render 버퍼 초기화 (RenderModule이 사용)
		m_sortBuffer.Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);

		for (auto& mod : m_modules)
			mod->Initialize(initCtx);

		InitializeBuffers(device);

		// Baked 데이터가 CPU에 있으면 GPU 버퍼 생성 및 업로드
		if (m_bakedCount > 0 && m_bakedSpawnPos.Size() > 0) {
			m_bakedSpawnPos.Initialize(device.Get());
			m_bakedSpawnPos.Upload(context.Get());
		}

		// Custom positions도 동일하게 처리
		if (m_customPositions.Size() > 0) {
			m_customPositions.Initialize(device.Get());
			m_customPositions.Upload(context.Get());
		}

		// 초기 spawn 위치 저장 (Reset 시 복원용)
		m_initialSpawnPos = m_consts.GetCpu().spawn.localPos;
	}

	void ParticleEmitter::OnSpawn()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_elapsedTime = 0.f;
		m_isDurationEnded = false;
		m_isCompleted = false;
		m_isStarted = false;

		SimulationContext simCtx = {
			context.Get(),
			m_consts,
			m_frameConsts,
			GetReadBuffer(),
			GetWriteBuffer(),
			GetReadCount(),
			GetWriteCount(),
			0.f,
			0.f,
			m_dispatchArgs.GetBuffer(),
			device.Get(),
			this->GetModule<RenderModule>(),
			&m_bakedSpawnPos,
			&m_customPositions,
			m_bakedCount,
			&m_sortBuffer,
			&m_billboardArgsBuffer,
			&m_meshArgsBuffer
		};

		if (m_spawnOffset != Vector3(0.f)) 
			m_consts.GetCpu().spawn.localPos += m_spawnOffset;

		// m_spawnOffset에 최종 위치 저장 (Sub-Emitter 상속용)
		m_spawnOffset = m_consts.GetCpu().spawn.localPos;

		for (auto& mod : m_modules)
			mod->OnSpawn(simCtx);

		m_consts.Upload();
		context->CSSetConstantBuffers(5, 1, m_consts.GetAddressOf());

		// OnStart 이벤트
		if (!m_isStarted) {
			ExecuteEvent(EmitterEvent::OnStart);
			m_isStarted = true;
		}
	}

	void ParticleEmitter::Reset()
	{
		m_elapsedTime = 0.f;
		m_isDurationEnded = false;
		m_isCompleted = false;
		m_isStarted = false;
		m_spawnOffset = Vector3(0.f);
		
		// 초기 spawn 위치로 복원
		m_consts.GetCpu().spawn.localPos = m_initialSpawnPos;
	}

	void ParticleEmitter::SetParticleConfig(const ParticleConsts& config)
	{
		m_consts.SetCpuData(config);
	}

	void ParticleEmitter::SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id)
	{
		m_jsonPath = path;
		m_watcherID = id;
	}

	void ParticleEmitter::LoadBakedSpawnData(const std::string& path)
	{
		TextureSpawnBake::Get().LoadBakedData(path, m_bakedSpawnPos, m_bakedCount);
	}

	void ParticleEmitter::InitializeBuffers(ComPtr<ID3D11Device>& device)
	{
		// 기존 버퍼 해제
		std::vector<uint32_t> initialCount = { 0 };
		for (UINT i = 0; i < 2; ++i) {
			m_particles[i] = StructuredBuffer<Particle>();
			m_activeCounts[i] = StructuredBuffer<uint32_t>();

			m_particles[i].Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);
			m_activeCounts[i].Initialize(device.Get(), 1);

			m_activeCounts[i].SetData(initialCount);
			m_activeCounts[i].Upload(GET_SINGLE(RenderBase)->GetContext().Get());
		}

		m_dispatchArgs = IndirectArgsBuffer<DispatchArgs>();
		m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 }, 4);
	}

	void ParticleEmitter::Update(const float& dt)
	{
		// 완료된 경우 (Loop가 아닐때 종료된 경우)
		if (m_isCompleted)
			return; 

		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// activeCount를 0으로 초기화
		const UINT clearVal[1] = { 0 };
		context->ClearUnorderedAccessViewUint(GetWriteCount().GetUAV(), clearVal);
		
		m_elapsedTime += dt;

		// Duration 체크 (m_duration > 0일 때만)
		if (m_duration > 0.f) {
			if (!m_isDurationEnded && m_elapsedTime >= m_duration) {
				m_isDurationEnded = true;
				ExecuteEvent(EmitterEvent::OnDurationEnd);
			}

			// Complete 체크 (Duration 후 Delay 경과)
			if (m_isDurationEnded && m_elapsedTime >= (m_duration + m_completionDelay)) {
				m_isCompleted = true;
				ExecuteEvent(EmitterEvent::OnComplete);
				return;
			}
		}

		// [최적화] Frame constants 먼저 업데이트
		ParticleFrameConsts& frameConsts = m_frameConsts.GetCpu();
		frameConsts.dt = dt;
		frameConsts.time = m_elapsedTime;
		
		SimulationContext simCtx = {
			context.Get(),
			m_consts,
			m_frameConsts,
			GetReadBuffer(),
			GetWriteBuffer(),
			GetReadCount(),
			GetWriteCount(),
			dt,
			m_elapsedTime,
			m_dispatchArgs.GetBuffer(),
			device.Get(),
			nullptr,
			&m_bakedSpawnPos,
			&m_customPositions,
			m_bakedCount,
			&m_sortBuffer,
			&m_billboardArgsBuffer,
			&m_meshArgsBuffer
		};

		// Duration 종료 전 혹은 Looping일때만 계속 Spawn
		if (!m_isDurationEnded) {
			for (auto& mod : m_modules)
				mod->OnUpdateCPU(simCtx);
		}
		else {
			// Duration 종료 후에는 spawnCount = 0
			frameConsts.spawnCount = 0;
		}

		m_frameConsts.Upload();
		ID3D11Buffer* constBuffers[] = {
			m_frameConsts.Get(),
			m_consts.Get()
		};
		context->CSSetConstantBuffers(4, 2, constBuffers);

		UpdateArgsBuffers(context.Get()); 

		for (auto& mod : m_modules)
			mod->OnUpdate(simCtx);

		// Duration 종료 전 혹은 Looping일때만 계속 계산
		if (!m_isDurationEnded) {
			for (auto& mod : m_modules)
				mod->LateUpdate(simCtx); // 현재 SpawnModule에서만 Particle을 생성하기 위해 사용중
		}

		SwapBuffer();
	}

	void ParticleEmitter::UpdateArgsBuffers(ID3D11DeviceContext* context)
	{
		ID3D11UnorderedAccessView* argUAVs[] = {
			m_dispatchArgs.GetUAV()
		};

		context->CSSetShaderResources(0, 1, GetReadCount().GetAddressOfSRV());
		context->CSSetUnorderedAccessViews(0, 1, argUAVs, nullptr);

		auto& argsUpdateCS = RenderBase::computeCommon.particle.argsUpdateCS;
		context->CSSetShader(argsUpdateCS.computeShader.Get(), 0, 0);
		context->Dispatch(1, 1, 1);
		
		ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
		ID3D11UnorderedAccessView* nullUAVs[1] = { nullptr };
		context->CSSetShaderResources(0, 1, nullSRVs);
		context->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
		context->CSSetShader(nullptr, 0, 0);
	}

	void ParticleEmitter::ExecuteEvent(EmitterEvent event)
	{
		// 현재 이 Emitter가 가진 SubEmitter를 사용해 Emitter 생성
		if (m_eventCallback)
			m_eventCallback(event, this);
	}

	void ParticleEmitter::Render()
	{
		// 완료되면 Skip
		if (m_isCompleted)
			return;

		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		RenderContext renderCtx = {
			context,
			m_consts,
			m_frameConsts,
			GetReadBuffer(),
			GetWriteBuffer(),
			GetReadCount(),
			GetWriteCount(),
			this->GetModule<MaterialModule>(),
			&m_sortBuffer,
			&m_billboardArgsBuffer,
			&m_meshArgsBuffer
		};

		for (auto& mod : m_modules)
			mod->UpdateArgs(renderCtx);

		context->PSSetConstantBuffers(5, 1, m_consts.GetAddressOf());
		for (auto& mod : m_modules)
			mod->OnRender(renderCtx);
	}

	void ParticleEmitter::AddModule(std::unique_ptr<ParticleModule>&& module) {
		for (auto& mod : m_modules) {
			if (typeid(*mod) == typeid(*module)) {
				mod = std::move(module);
				break;
			}
		}

		if (module)
			m_modules.emplace_back(std::move(module));

		std::sort(m_modules.begin(), m_modules.end(),
			[](const std::unique_ptr<ParticleModule>& a, const std::unique_ptr<ParticleModule>& b) {
				return a->GetPriority() < b->GetPriority();
			});
	}

	void ParticleEmitter::AddSubEmitter(const SubEmitter& sub)
	{
		m_subEmitters.push_back(sub);
	}
	void ParticleEmitter::ClearSubEmitters()
	{
		m_subEmitters.clear();
	}
	const std::vector<SubEmitter>& ParticleEmitter::GetSubEmitters() const
	{
		return m_subEmitters;
	}
	void ParticleEmitter::SetEventCallback(EventCallback cb)
	{
		m_eventCallback = std::move(cb);
	}
	Vector3 ParticleEmitter::GetSpawnPosition() const
	{
		return m_spawnOffset;
	}
	void ParticleEmitter::SetSpawnOffset(const Vector3& offset)
	{
		m_spawnOffset = offset;
	}
	const std::wstring& ParticleEmitter::GetName() const
	{
		return m_name;
	}
}