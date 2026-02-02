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
		: m_name(other.m_name)
		, m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)  // Hot-Reload는 복사 안 함
		// Baked Spawn 관련
		, m_bakedPath(other.m_bakedPath)           //  추가 (핵심!)
		, m_bakedCount(other.m_bakedCount)
		, m_bakedPoolOffset(0)                      // Initialize에서 재설정됨
		// Custom Position 관련
		, m_customPositions(other.m_customPositions) //  추가
		, m_customPoolOffset(0)                      // Initialize에서 재설정됨
		, m_useCustomPositions(other.m_useCustomPositions) //  추가
		// SubEmitter 관련
		, m_duration(other.m_duration)
		, m_completionDelay(other.m_completionDelay)
		, m_subEmitters(other.m_subEmitters)
		// 상태 (초기값으로 시작)
		, m_elapsedTime(0.f)
		, m_isDurationEnded(false)
		, m_isCompleted(false)
		, m_isStarted(false)
		, m_spawnOffset(Vector3(0.f))
		, m_initialSpawnPos(other.m_initialSpawnPos)
		// Initialize에서 설정됨
		, m_ownerSystem(nullptr)
		, m_poolOffset(0)
		, m_emitterID(0)
	{
		// 모듈 복제
		for (const auto& mod : other.m_modules) {
			if (mod) {
				auto clonedModule = mod->Clone();
				if (clonedModule)
					m_modules.push_back(std::move(clonedModule));
			}
		}
	}

	void ParticleEmitter::Initialize(ParticleConsts& pConsts, ParticleFrameConsts& pfConsts, DrawIndexedInstancedArgs& pMeshArgs)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		
		ParticleInitContext initCtx = { 
			device.Get(), 
			pConsts,
			pfConsts,
			pMeshArgs,
			GetModule<RenderModule>(),
			m_emitterID,
			m_customPositions,
			m_useCustomPositions
		};

		// 모듈들의 상수 설정이 여기서 수행됨
		for (auto& mod : m_modules)
			mod->Initialize(initCtx);

		// 초기 spawn 위치 저장 (Reset 시 복원용)
		m_initialSpawnPos = pConsts.spawn.localPos;

		if (m_spawnOffset != Vector3(0.f))
			pConsts.spawn.localPos += m_spawnOffset;

		m_spawnOffset = pConsts.spawn.localPos;

	}

	void ParticleEmitter::OnSpawn()
	{
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_elapsedTime = 0.f;
		m_isDurationEnded = false;
		m_isCompleted = false;
		m_isStarted = false;

		SimulationContext simCtx = {
			context.Get(),
			0.f,
			nullptr,
			0,
			m_ownerSystem->GetBakedSpawnBuffer(),
			m_ownerSystem->GetCustomPositions()
		};

		for (auto& mod : m_modules)
			mod->OnSpawn(simCtx);
	}

	void ParticleEmitter::PreUpdate(const float& dt, ParticleFrameConsts& fsConsts)
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

		// Frame constants 먼저 업데이트
		fsConsts.dt = dt;
		fsConsts.time = m_elapsedTime;

		SimulationContext simCtx = {
			context.Get(),
			fsConsts.dt,
			nullptr,
			0,
			m_ownerSystem->GetBakedSpawnBuffer(),
			m_ownerSystem->GetCustomPositions(),
			&fsConsts
		};

		for (auto& mod : m_modules)
			mod->OnPreUpdate(simCtx);

		if (m_isDurationEnded)
			fsConsts.spawnCount = 0;
	}

	void ParticleEmitter::Reset()
	{
		m_elapsedTime = 0.f;
		m_isDurationEnded = false;
		m_isCompleted = false;
		m_isStarted = false;
		m_spawnOffset = Vector3(0.f);
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

	UINT ParticleEmitter::LoadBakedSpawnData(std::vector<Vector3>& outBakedSpawnPos)
	{
		TextureSpawnBake::Get().LoadBakedData(m_bakedPath, outBakedSpawnPos, m_bakedCount);
		return m_bakedCount;
	}

	void ParticleEmitter::Update(const float& dt, IndirectArgsBuffer<DispatchArgs>& dispatchArgs, UINT dispatchOffset)
	{
		// 완료된 경우 (Loop가 아닐때 종료된 경우)
		if (m_isCompleted)
			return; 

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		SimulationContext simCtx = {
			context.Get(),
			0.f,
			dispatchArgs.GetBuffer(),
			dispatchOffset,
			m_ownerSystem->GetBakedSpawnBuffer(),
			m_ownerSystem->GetCustomPositions()
		};

		m_ownerSystem->BindConstantID(m_emitterID);

		for (auto& mod : m_modules)
			mod->OnUpdate(simCtx);

		// Duration 종료 전 혹은 Looping일때만 계속 계산
		if (!m_isDurationEnded) {
			for (auto& mod : m_modules)
				mod->LateUpdate(simCtx);
		}
	}

	void ParticleEmitter::ExecuteEvent(EmitterEvent event)
	{
		// 현재 이 Emitter가 가진 SubEmitter를 사용해 Emitter 생성
		if (m_eventCallback)
			m_eventCallback(event, this);
	}

	void ParticleEmitter::Render(IndirectArgsBuffer<DrawInstancedArgs>& billboardArgs, UINT billboardOffset)
	{
		// 완료되면 Skip
		if (m_isCompleted)
			return;

		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		RenderContext renderCtx = {
			context,
			this->GetModule<MaterialModule>(),
			m_emitterID,
			billboardArgs,
			billboardOffset,
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