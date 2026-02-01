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
#include "ParticleSystem.h"

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
		, m_initialSpawnPos(other.m_initialSpawnPos)
	{
		for (const auto& mod : other.m_modules) {
			if (mod) {
				auto clonedModule = mod->Clone();
				if (clonedModule)
					m_modules.push_back(std::move(clonedModule));
			}
		}
	}

	void ParticleEmitter::Initialize()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		
		ParticleInitContext initCtx = { 
			device.Get(), 
			m_ownerSystem->GetConstsData(m_emitterID),
			m_ownerSystem->GetFrameConstsData(m_emitterID),
			m_ownerSystem->GetInitMeshArgs(m_emitterID),
			GetModule<RenderModule>(),
			m_emitterID,
			m_customPositions,
			m_useCustomPositions
		};

		// 모듈들의 상수 설정이 여기서 수행됨
		for (auto& mod : m_modules)
			mod->Initialize(initCtx);

		InitializeBuffers(device);

		// 초기 spawn 위치 저장 (Reset 시 복원용)
		m_initialSpawnPos = m_ownerSystem->GetConstsData(m_emitterID).spawn.localPos;
	}

	void ParticleEmitter::OnSpawn()
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_elapsedTime = 0.f;
		m_isDurationEnded = false;
		m_isCompleted = false;
		m_isStarted = false;  // OnStart는 첫 Update에서 처리

		SimulationContext simCtx = {
			context.Get(),
			m_ownerSystem->GetConstsData(m_emitterID),
			m_ownerSystem->GetFrameConstsData(m_emitterID),
			m_ownerSystem->GetReadBuffer(),
			m_ownerSystem->GetWriteBuffer(),
			m_ownerSystem->GetReadCount(),
			m_ownerSystem->GetWriteCount(),
			0.f,
			m_ownerSystem->GetDispatchArgs().GetBuffer(),
			m_ownerSystem->GetDispatchArgsOffset(m_emitterID),
			m_ownerSystem->GetBakedSpawnBuffer(),
			m_ownerSystem->GetCustomPositions()
		};

		if (m_spawnOffset != Vector3(0.f)) 
			m_ownerSystem->GetConstsData(m_emitterID).spawn.localPos += m_spawnOffset;

		m_spawnOffset = m_ownerSystem->GetConstsData(m_emitterID).spawn.localPos;

		for (auto& mod : m_modules)
			mod->OnSpawn(simCtx);

		// ❌ 제거: OnStart 이벤트를 여기서 실행하지 않음
		// if (!m_isStarted) {
		//     ExecuteEvent(EmitterEvent::OnStart);
		//     m_isStarted = true;
		// }
	}

	void ParticleEmitter::PreUpdate(const float& dt)
	{
		if (m_isCompleted)
			return;

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		// OnStart 이벤트를 첫 Update에서 실행 (지연 실행)
		if (!m_isStarted) {
			ExecuteEvent(EmitterEvent::OnStart);
			m_isStarted = true;
		}

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
		ParticleFrameConsts& frameConsts = m_ownerSystem->GetFrameConstsData(m_emitterID);
		frameConsts.dt = dt;
		frameConsts.time = m_elapsedTime;

		SimulationContext simCtx = {
			context.Get(),
			m_ownerSystem->GetConstsData(m_emitterID),
			m_ownerSystem->GetFrameConstsData(m_emitterID),
			m_ownerSystem->GetReadBuffer(),
			m_ownerSystem->GetWriteBuffer(),
			m_ownerSystem->GetReadCount(),
			m_ownerSystem->GetWriteCount(),
			dt,
			m_ownerSystem->GetDispatchArgs().GetBuffer(),
			m_ownerSystem->GetDispatchArgsOffset(m_emitterID),
			m_ownerSystem->GetBakedSpawnBuffer(),
			m_ownerSystem->GetCustomPositions()
		};

		for (auto& mod : m_modules)
			mod->OnPreUpdate(simCtx);
	}

	void ParticleEmitter::Reset()
	{
		m_elapsedTime = 0.f;
		m_isDurationEnded = false;
		m_isCompleted = false;
		m_isStarted = false;
		m_spawnOffset = Vector3(0.f);
		
		// 초기 spawn 위치로 복원
		m_ownerSystem->GetConstsData(m_emitterID).spawn.localPos = m_initialSpawnPos;
	}

	void ParticleEmitter::SetParticleConfig(const ParticleConsts& config)
	{
		m_ownerSystem->GetConstsData(m_emitterID) = config;
	}

	void ParticleEmitter::SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id)
	{
		m_jsonPath = path;
		m_watcherID = id;
	}

	void ParticleEmitter::SetBakedSpawnPath(const std::string& path)
	{
		m_bakedPath = path;
		m_bakedCount = 1;
	}

	UINT ParticleEmitter::LoadBakedSpawnData(StructuredBuffer<Vector3>& outBakedSpawnPos)
	{
		TextureSpawnBake::Get().LoadBakedData(m_bakedPath, outBakedSpawnPos, m_bakedCount, m_bakedPoolOffset);
		m_ownerSystem->GetConstsData(m_emitterID).spawn.bakedCount = m_bakedCount;
		return m_bakedCount;
	}

	void ParticleEmitter::InitializeBuffers(ComPtr<ID3D11Device>& device)
	{
	}

	void ParticleEmitter::Update(const float& dt)
	{
		// 완료된 경우 (Loop가 아닐때 종료된 경우)
		if (m_isCompleted)
			return; 

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		SimulationContext simCtx = {
			context.Get(),
			m_ownerSystem->GetConstsData(m_emitterID),
			m_ownerSystem->GetFrameConstsData(m_emitterID),
			m_ownerSystem->GetReadBuffer(),
			m_ownerSystem->GetWriteBuffer(),
			m_ownerSystem->GetReadCount(),
			m_ownerSystem->GetWriteCount(),
			dt,
			m_ownerSystem->GetDispatchArgs().GetBuffer(),
			m_ownerSystem->GetDispatchArgsOffset(m_emitterID),
			m_ownerSystem->GetBakedSpawnBuffer(),
			m_ownerSystem->GetCustomPositions()
		};

		m_ownerSystem->BindConstantID(m_emitterID);

		UpdateArgsBuffers(context.Get()); 

		for (auto& mod : m_modules)
			mod->OnUpdate(simCtx);

		// Duration 종료 전 혹은 Looping일때만 계속 계산
		if (!m_isDurationEnded) {
			for (auto& mod : m_modules)
				mod->LateUpdate(simCtx);
		}
		else
			// Duration 종료 후에는 spawnCount = 0
			m_ownerSystem->GetFrameConstsData(m_emitterID).spawnCount = 0;
	}

	void ParticleEmitter::UpdateArgsBuffers(ID3D11DeviceContext* context)
	{
		ID3D11UnorderedAccessView* argUAVs[] = {
			m_ownerSystem->GetDispatchArgs().GetUAV()
		};

		context->CSSetShaderResources(0, 1, m_ownerSystem->GetReadCount().GetAddressOfSRV());
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
			m_ownerSystem->GetConstsData(m_emitterID),
			m_ownerSystem->GetFrameConstsData(m_emitterID),
			m_ownerSystem->GetReadBuffer(),
			m_ownerSystem->GetWriteBuffer(),
			m_ownerSystem->GetReadCount(),
			m_ownerSystem->GetWriteCount(),
			this->GetModule<MaterialModule>(),
			m_emitterID,
			m_ownerSystem->GetBillboardArgs().GetBuffer(),
			m_ownerSystem->GetBillboardArgsOffset(m_emitterID),
			m_ownerSystem->GetMeshArgs(),
			m_ownerSystem->GetMeshArgsOffset(m_emitterID),
		};

		m_ownerSystem->BindConstantID(m_emitterID);
		for (auto& mod : m_modules)
			mod->UpdateArgs(renderCtx);

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
	void ParticleEmitter::SetMemoryInfo(UINT offset, UINT index)
	{
		m_poolOffset = offset;
		m_emitterID = index;
	}
	void ParticleEmitter::SetOwner(ParticleSystem* system)
	{
		m_ownerSystem = system;
	}
}