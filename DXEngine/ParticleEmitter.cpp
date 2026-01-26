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
			}
		}
	}

	ParticleEmitter::ParticleEmitter(const ParticleEmitter& other)
		: m_name(other.m_name)
		, m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)
		, m_bakedCount(other.m_bakedCount)
		, m_duration(other.m_duration)
		, m_completionDelay(other.m_completionDelay)
		, m_subEmitters(other.m_subEmitters)
	{
		for (const auto& mod : other.m_modules) {
			if (mod) {
				auto clonedModule = mod->Clone();
				if (clonedModule) {
					m_modules.push_back(std::move(clonedModule));
				}
			}
		}

		m_consts = other.m_consts;
		m_frameConsts = other.m_frameConsts;
		m_bakedSpawnPos = other.m_bakedSpawnPos;
		m_customPositions = other.m_customPositions;
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

		m_sortBuffer.Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);

		for (auto& mod : m_modules)
			mod->Initialize(initCtx);

		InitializeBuffers(device);
	}

	void ParticleEmitter::OnSpawn()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// 상태 초기화
		m_elapsedTime = 0.f;
		m_durationEnded = false;
		m_isCompleted = false;
		m_startFired = false;

		SimulationContext simCtx = {
			context.Get(),
			m_consts,
			m_frameConsts,
			0.f,
			0.f,
			m_consume,
			m_append,
			m_countSRV.Get(),
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

		// Spawn 위치 오프셋 적용
		if (m_spawnOffset != Vector3(0.f)) {
			m_consts.GetCpu().spawn.localPos += m_spawnOffset;
		}
		
		// m_spawnOffset에 최종 위치 저장 (Sub-Emitter 상속용)
		m_spawnOffset = m_consts.GetCpu().spawn.localPos;

		for (auto& mod : m_modules)
			mod->OnSpawn(simCtx);

		m_consts.Upload();
		context->CSSetConstantBuffers(5, 1, m_consts.GetAddressOf());
		
		// OnStart 이벤트
		FireEvent(EmitterEvent::OnStart);
		m_startFired = true;
	}

	void ParticleEmitter::Reset()
	{
		m_elapsedTime = 0.f;
		m_durationEnded = false;
		m_isCompleted = false;
		m_startFired = false;
		m_spawnOffset = Vector3(0.f);
		Initialize();
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
		m_consume.Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);
		m_append.Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);
		m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 }, 4);

		D3D11Utils::CreateBuffer(device.Get(), sizeof(UINT), nullptr,
			DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);
	}

	void ParticleEmitter::Update(const float& dt, const float& time)
	{
		// 이미 완료된 경우 스킵
		if (m_isCompleted)
			return;

		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_elapsedTime += dt;

		// Duration 체크 (m_duration > 0일 때만)
		if (m_duration > 0.f) {
			if (!m_durationEnded && m_elapsedTime >= m_duration) {
				m_durationEnded = true;
				FireEvent(EmitterEvent::OnDurationEnd);
			}

			// Complete 체크 (Duration 후 Delay 경과)
			if (m_durationEnded && m_elapsedTime >= (m_duration + m_completionDelay)) {
				m_isCompleted = true;
				FireEvent(EmitterEvent::OnComplete);
				return;
			}
		}

		ParticleFrameConsts& frameConsts = m_frameConsts.GetCpu();
		frameConsts.dt = dt;
		frameConsts.time = time;
		
		SimulationContext simCtx = {
			context.Get(),
			m_consts,
			m_frameConsts,
			dt,
			time,
			m_consume,
			m_append,
			m_countSRV.Get(),
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

		// Duration 종료 전에만 Spawn (또는 Duration 무한일 때)
		if (!m_durationEnded) {
			for (auto& mod : m_modules)
				mod->OnUpdateCPU(simCtx);
		} else {
			// Duration 종료 후에는 spawnCount = 0
			frameConsts.spawnCount = 0;
		}

		m_frameConsts.Upload();
		ID3D11Buffer* constBuffers[] = {
			m_frameConsts.Get(),
			m_consts.Get()
		};
		context->CSSetConstantBuffers(4, 2, constBuffers);

		if (!m_durationEnded) {
			for (auto& mod : m_modules)
				mod->PreUpdate(simCtx);
		}

		UpdateArgsBuffers(context.Get());

		for (auto& mod : m_modules)
			mod->UpdateArgs(simCtx);

		for (auto& mod : m_modules)
			mod->OnUpdate(simCtx);

		swap(m_consume, m_append);
	}

	void ParticleEmitter::UpdateArgsBuffers(ID3D11DeviceContext* context)
	{
		context->CopyStructureCount(m_countBuffer.Get(), 0, m_consume.GetUAV());

		ID3D11UnorderedAccessView* argUAVs[] = {
			m_dispatchArgs.GetUAV()
		};

		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
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

	void ParticleEmitter::Render()
	{
		// 완료된 경우 렌더링 스킵
		if (m_isCompleted)
			return;

		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		RenderContext renderCtx = {
			context,
			m_consts,
			m_frameConsts,
			m_consume.GetSRV(),
			this->GetModule<MaterialModule>(),
			&m_sortBuffer,
			&m_billboardArgsBuffer,
			&m_meshArgsBuffer
		};

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

	void ParticleEmitter::SetSpawnOffset(const Vector3& offset)
	{
		m_spawnOffset = offset;
	}

	void ParticleEmitter::FireEvent(EmitterEvent event)
	{
		if (m_eventCallback) {
			m_eventCallback(event, this);
		}
	}
}