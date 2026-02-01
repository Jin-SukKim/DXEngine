#pragma once
#include "Particle.h"
#include "ComputeShader.h"
#include "AppendBuffer.h"
#include "StagingBuffer.h"
#include "IndirectArgsBuffer.h"
#include "ParticleModule.h"
#include "FileWatcher.h"
#include "MeshData.h"

namespace DE {
	class ParticleSystem;
	enum class EmitterEvent : uint8_t {
		OnStart, // Emitter 시작 시
		OnDurationEnd, // Duration 종료 시
		OnComplete // Duration + Delay 완료 시
	};

	struct SubEmitter {
		std::wstring emitterPath;
		EmitterEvent trigger = EmitterEvent::OnComplete;
		bool inheritPosition = true; // trigger되는 emitter의 위치 상속
	};

	class ParticleEmitter
	{
	public:
		ParticleEmitter(const std::wstring& name);
		~ParticleEmitter();

		ParticleEmitter(const ParticleEmitter& other);

		void Initialize(ParticleConsts& pConsts, ParticleFrameConsts& pfConsts, DrawIndexedInstancedArgs& pMeshArgs);
		void OnSpawn();
		void PreUpdate(const float& dt);
		void Update(const float& dt);
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
		
		// Texture Bake 데이터 설정 (Spawn 모듈용)
		void SetBakedSpawnPath(const std::string& path);
		UINT LoadBakedSpawnData(std::vector<Vector3>& outBakedSpawnPos);
		void SetBakedInfo(UINT offset) { m_bakedPoolOffset = offset; }
		const std::string& GetBakedPath() const { return m_bakedPath; }

		void SetCustomSpawnInfo(UINT offset) { m_customPoolOffset = offset; }
		bool IsUsingCustomPositions() const { return m_useCustomPositions; }
		const std::vector<Vector3>& GetCustomPositions() const { return m_customPositions; }

		UINT& GetBakedCount() { return m_bakedCount; }
		
		void SetDuration(float duration) { m_duration = duration; }
		void SetCompletionDelay(float delay) { m_completionDelay = delay; }
		bool IsCompleted() const { return m_isCompleted; }
		float GetElapsedTime() const { return m_elapsedTime; }

		UINT GetSubEmitterCount() const { return static_cast<UINT>(m_subEmitters.size()); }
		void AddSubEmitter(const SubEmitter& sub);
		void ClearSubEmitters();
		const std::vector<SubEmitter>& GetSubEmitters() const;

		// ParticleSystem이 등록 (어떤 Event때 이 Emitter를 사용할지)
		using EventCallback = std::function<void(EmitterEvent, ParticleEmitter*)>; 
		// ParticleSystem에서 SubEmitter 생성 함수 등록
		void SetEventCallback(EventCallback cb);

		Vector3 GetSpawnPosition() const;
		void SetSpawnOffset(const Vector3& offset);
		const std::wstring& GetName() const;

		void SetMemoryInfo(UINT offset, UINT index);
		void SetOwner(ParticleSystem* system);
		UINT GetEmitterID() { return m_emitterID; };
		void SetName(std::wstring name) { m_name = name; }
	private:
		// 초기화 관련 함수들
		void InitializeBuffers(ComPtr<ID3D11Device>& device);

		// 업데이트 단계별 함수들
		void UpdateArgsBuffers(ID3D11DeviceContext* context);

		void ExecuteEvent(EmitterEvent event);
	private:
		std::wstring m_name;

		// Buffer Memory에 데이터가 저장될 위치
		ParticleSystem* m_ownerSystem;
		UINT m_poolOffset = 0; // Particle Memory 내에서의 시작 index
		UINT m_emitterID = 0; // Emitter Index (count, constant 접근용)

		std::vector<std::unique_ptr<ParticleModule>> m_modules;

		// Hot-Reload 정보
		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
		
		// Baked Spawn Position 데이터 (Texture Spawn용)
		UINT m_bakedPoolOffset = 0;
		std::string m_bakedPath = "";
		UINT m_bakedCount = 0;
		std::vector<Vector3> m_customPositions;
		UINT m_customPoolOffset = 0;
		bool m_useCustomPositions = false;

		// SubEmitter
		float m_duration = -1.f; // -1: 무한(Looping), 0 >= : 지정 시간
		float m_completionDelay = 2.f; // Duration 후 대기 시간
		float m_elapsedTime = 0.f;
		bool m_isDurationEnded = false;
		bool m_isCompleted = false;
		bool m_isStarted = false;

		std::vector<SubEmitter> m_subEmitters;
		EventCallback m_eventCallback;
		Vector3 m_spawnOffset = Vector3(0.f);

		Vector3 m_initialSpawnPos = Vector3(0.f);  // 추가: 초기 위치 저장
	};

	template<typename T>
	void ParticleEmitter::AddModule(std::unique_ptr<ParticleModule>&& module)
	{
		// 이미 해당 타입이 존재할 있는지 확인하고 교체
		for (auto& existingModule : m_modules)
		{
			// unique_ptr의 get()으로 Raw 포인터를 가져와 타입 검사
			if (dynamic_cast<T*>(existingModule.get()) != nullptr)
			{
				// 기존 모듈을 새로운 모듈로 교체 (이동 의미)
				existingModule = std::move(module);
				break;
			}
		}

		// 남아있으면 추가
		if (module)
			m_modules.emplace_back(std::move(module));

		// Priority로 정렬 정렬
		// unique_ptr이므로 참조 포인터를 unique_ptr& 로 받아야 함
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
			// unique_ptr -> Raw Pointer -> dynamic_cast
			if (auto* casted = dynamic_cast<T*>(mod.get()))
			{
				return casted;
			}
		}
		return nullptr;
	}
}