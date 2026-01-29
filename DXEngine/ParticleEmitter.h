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

namespace DE {
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

		void Initialize();
		void OnSpawn();
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
		void LoadBakedSpawnData(const std::string& path);

		ParticleConsts& GetConstsData() { return m_consts.GetCpu(); }
		ParticleFrameConsts& GetFrameConstsData() { return m_frameConsts.GetCpu(); }
		ConstantBuffer<ParticleConsts>& GetConstBuffer() { return m_consts; }
		
		// Baked Spawn Position 접근자
		StructuredBuffer<Vector3>* GetBakedSpawnBuffer() { return &m_bakedSpawnPos; }
		UINT GetBakedCount() const { return m_bakedCount; }
		
		// Render 버퍼 접근자
		BitonicSort* GetSortBuffer() { return &m_sortBuffer; }
		IndirectArgsBuffer<DrawInstancedArgs>* GetBillboardArgsBuffer() { return &m_billboardArgsBuffer; }
		IndirectArgsBuffer<DrawIndexedInstancedArgs>* GetMeshArgsBuffer() { return &m_meshArgsBuffer; }
		
		// SubEmitter
		void SetDuration(float duration) { m_duration = duration; }
		void SetCompletionDelay(float delay) { m_completionDelay = delay; }
		bool IsCompleted() const { return m_isCompleted; }
		float GetElapsedTime() const { return m_elapsedTime; }

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

		StructuredBuffer<Particle>& GetReadBuffer() { return m_particles[m_currentBuffer]; }
		StructuredBuffer<Particle>& GetWriteBuffer() { return m_particles[1 - m_currentBuffer]; }
		StructuredBuffer<uint32_t>& GetReadCount() { return m_activeCounts[m_currentBuffer]; }
		StructuredBuffer<uint32_t>& GetWriteCount() { return m_activeCounts[1 - m_currentBuffer]; }

		void SwapBuffer() { m_currentBuffer = 1 - m_currentBuffer; }

	private:
		// 초기화 관련 함수들
		void InitializeBuffers(ComPtr<ID3D11Device>& device);

		// 업데이트 단계별 함수들
		void UpdateArgsBuffers(ID3D11DeviceContext* context);

		void ExecuteEvent(EmitterEvent event);
	private:
		std::wstring m_name;
		// 파티클 버퍼 (이중 버퍼링)
		StructuredBuffer<uint32_t> m_activeCounts[2];
		StructuredBuffer<Particle> m_particles[2];
		UINT m_currentBuffer = 0;

		// 간접 디스패치
		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;

		// 상수 버퍼
		ConstantBuffer<ParticleFrameConsts> m_frameConsts;
		ConstantBuffer<ParticleConsts> m_consts;

		std::vector<std::unique_ptr<ParticleModule>> m_modules;

		// Hot-Reload 정보
		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
		
		// Baked Spawn Position 데이터 (Texture Spawn용)
		StructuredBuffer<Vector3> m_bakedSpawnPos;
		StructuredBuffer<Vector3> m_customPositions;
		UINT m_bakedCount = 0;
		
		// Render 관련 버퍼
		BitonicSort m_sortBuffer;
		IndirectArgsBuffer<DrawInstancedArgs> m_billboardArgsBuffer;
		IndirectArgsBuffer<DrawIndexedInstancedArgs> m_meshArgsBuffer;

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