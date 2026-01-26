#pragma once
#include "Particle.h"
#include "ComputeShader.h"
#include "AppendBuffer.h"
#include "StagingBuffer.h"
#include "IndirectArgsBuffer.h"
#include "BitonicSort.h"
#include "ParticleModule.h"
#include "FileWatcher.h"
#include "MeshData.h"
#include <functional>

namespace DE {

	// Sub-Emitter 이벤트 타입
	enum class EmitterEvent : uint8_t {
		OnStart,       // Emitter 시작 시
		OnDurationEnd, // Duration 종료 시
		OnComplete     // Duration + Delay 완료 시
	};

	// Sub-Emitter 설정
	struct SubEmitterEntry {
		std::wstring emitterPath;
		EmitterEvent trigger = EmitterEvent::OnComplete;
		bool inheritPosition = true;
	};

	class ParticleEmitter
	{
	public:
		ParticleEmitter(const std::wstring& name);
		~ParticleEmitter();

		ParticleEmitter(const ParticleEmitter& other);

		void Initialize();
		void OnSpawn();
		void Update(const float& dt, const float& time);
		void Render();

		template<typename T>
		void AddModule(std::unique_ptr<ParticleModule>&& module);
		void AddModule(std::unique_ptr<ParticleModule>&& module);
		template<typename T>
		T* GetModule();
		
		void ClearModules() { m_modules.clear(); }
		void Reset();

		void SetParticleConfig(const ParticleConsts& config);
		void SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id);
		
		void LoadBakedSpawnData(const std::string& path);

		ParticleConsts& GetConstsData() { return m_consts.GetCpu(); }
		ParticleFrameConsts& GetFrameConstsData() { return m_frameConsts.GetCpu(); }
		ConstantBuffer<ParticleConsts>& GetConstBuffer() { return m_consts; }
		AppendBuffer<Particle>& GetConsumeBuffer() { return m_consume; }
		
		StructuredBuffer<Vector3>* GetBakedSpawnBuffer() { return &m_bakedSpawnPos; }
		UINT GetBakedCount() const { return m_bakedCount; }
		
		BitonicSort* GetSortBuffer() { return &m_sortBuffer; }
		IndirectArgsBuffer<DrawInstancedArgs>* GetBillboardArgsBuffer() { return &m_billboardArgsBuffer; }
		IndirectArgsBuffer<DrawIndexedInstancedArgs>* GetMeshArgsBuffer() { return &m_meshArgsBuffer; }
		
		// ========== Sub-Emitter 시스템 =========
		// 상태 관리
		void SetDuration(float duration) { m_duration = duration; }
		void SetCompletionDelay(float delay) { m_completionDelay = delay; }
		bool IsCompleted() const { return m_isCompleted; }
		float GetElapsedTime() const { return m_elapsedTime; }
		
		// Sub-Emitter 설정
		void AddSubEmitter(const SubEmitterEntry& entry) { m_subEmitters.push_back(entry); }
		void ClearSubEmitters() { m_subEmitters.clear(); }
		const std::vector<SubEmitterEntry>& GetSubEmitters() const { return m_subEmitters; }
		
		// 이벤트 콜백 (ParticleSystem이 등록)
		using EventCallback = std::function<void(EmitterEvent, ParticleEmitter*)>;
		void SetEventCallback(EventCallback cb) { m_eventCallback = std::move(cb); }
		
		// 위치 정보 (상속용)
		Vector3 GetSpawnPosition() const { return m_spawnOffset; }
		void SetSpawnOffset(const Vector3& offset);
		
		const std::wstring& GetName() const { return m_name; }

	private:
		void InitializeBuffers(ComPtr<ID3D11Device>& device);
		void UpdateArgsBuffers(ID3D11DeviceContext* context);
		void FireEvent(EmitterEvent event);

	private:
		std::wstring m_name;
		AppendBuffer<Particle> m_consume;
		AppendBuffer<Particle> m_append;

		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		
		ComPtr<ID3D11Buffer> m_countBuffer;
		ComPtr<ID3D11ShaderResourceView> m_countSRV;

		ConstantBuffer<ParticleFrameConsts> m_frameConsts;
		ConstantBuffer<ParticleConsts> m_consts;

		std::vector<std::unique_ptr<ParticleModule>> m_modules;

		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
		
		StructuredBuffer<Vector3> m_bakedSpawnPos;
		StructuredBuffer<Vector3> m_customPositions;
		UINT m_bakedCount = 0;
		
		BitonicSort m_sortBuffer;
		IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
		IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;
		
		// ========== Sub-Emitter 관련 =========
		float m_duration = -1.f;          // -1: 무한, >0: 지정 시간
		float m_completionDelay = 2.f;    // Duration 후 대기 시간
		float m_elapsedTime = 0.f;
		bool m_durationEnded = false;
		bool m_isCompleted = false;
		bool m_startFired = false;
		
		std::vector<SubEmitterEntry> m_subEmitters;
		EventCallback m_eventCallback;
		Vector3 m_spawnOffset = Vector3(0.f);
	};

	template<typename T>
	void ParticleEmitter::AddModule(std::unique_ptr<ParticleModule>&& module)
	{
		for (auto& existingModule : m_modules)
		{
			if (dynamic_cast<T*>(existingModule.get()) != nullptr)
			{
				existingModule = std::move(module);
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

	template<typename T>
	T* ParticleEmitter::GetModule()
	{
		for (auto& mod : m_modules)
		{
			if (auto* casted = dynamic_cast<T*>(mod.get()))
			{
				return casted;
			}
		}
		return nullptr;
	}
}