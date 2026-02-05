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
			}
		}
	}

	ParticleEmitter::ParticleEmitter(const ParticleEmitter& other)
		: m_name(other.m_name)
		, m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)
		, m_bakedPath(other.m_bakedPath)
		, m_bakedCount(other.m_bakedCount)
		, m_spawnPosPoolOffset(0)
		, m_customPositions(other.m_customPositions)
		, m_useCustomPositions(other.m_useCustomPositions)
		, m_duration(other.m_duration)
		, m_completionDelay(other.m_completionDelay)
		, m_subEmitters(other.m_subEmitters)
		, m_elapsedTime(0.f)
		, m_isDurationEnded(false)
		, m_isCompleted(false)
		, m_isStarted(false)
		, m_spawnOffset(Vector3(0.f))
		, m_initialSpawnPos(other.m_initialSpawnPos)
		, m_ownerSystem(nullptr)
		, m_emitterID(0)
	{
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

		for (auto& mod : m_modules)
			mod->Initialize(initCtx);

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
			{ context.Get() },
			0.f,
			nullptr,
			nullptr
		};

		for (auto& mod : m_modules)
			mod->OnSpawn(simCtx);
	}

	void ParticleEmitter::PreUpdate(const float& dt, ParticleFrameConsts& fsConsts)
	{
		if (m_isCompleted)
			return;

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		if (!m_isStarted) {
			ExecuteEvent(EmitterEvent::OnStart);
			m_isStarted = true;
		}

		m_elapsedTime += dt;
		if (m_duration > 0.f) {
			if (!m_isDurationEnded && m_elapsedTime >= m_duration) {
				m_isDurationEnded = true;
				ExecuteEvent(EmitterEvent::OnDurationEnd);
			}

			if (m_isDurationEnded && m_elapsedTime >= (m_duration + m_completionDelay)) {
				m_isCompleted = true;
				ExecuteEvent(EmitterEvent::OnComplete);
				return;
			}
		}

		fsConsts.dt = dt;
		fsConsts.time = m_elapsedTime;

		SimulationContext simCtx = {
			{ context.Get() },
			dt,
			nullptr,
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

	void ParticleEmitter::Update(const float& dt, ArgsParam args)
	{
		if (m_isCompleted)
			return; 

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		SimulationContext simCtx = {
			{ context.Get() },
			dt,
			&args,
			nullptr
		};

		for (auto& mod : m_modules)
			mod->OnUpdate(simCtx);


		if (!m_isDurationEnded) {
			m_ownerSystem->BindConstantID(m_emitterID);
			
			for (auto& mod : m_modules)
				mod->LateUpdate(simCtx);
		}
	}

	void ParticleEmitter::ExecuteEvent(EmitterEvent event)
	{
		if (m_eventCallback)
			m_eventCallback(event, this);
	}

	void ParticleEmitter::Render(ArgsParam billboardArgs, ArgsParam meshArgs)
	{
		if (m_isCompleted)
			return;

		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		RenderContext renderCtx = {
			{ context },
			this->GetModule<MaterialModule>(),
			m_emitterID,
			&billboardArgs,
			&meshArgs,
		};

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
	void ParticleEmitter::SetMemoryInfo(UINT index)
	{
		m_emitterID = index;
	}
	void ParticleEmitter::SetOwner(ParticleSystem* system)
	{
		m_ownerSystem = system;
	}
}