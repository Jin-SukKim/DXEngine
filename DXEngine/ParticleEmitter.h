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
	class RenderModule; // Forward declaration
	
	enum class EmitterEvent : uint8_t {
		OnStart,
		OnDurationEnd,
		OnComplete
	};

	struct SubEmitter {
		std::wstring emitterPath;
		EmitterEvent trigger = EmitterEvent::OnComplete;
		bool inheritPosition = true;
	};

	struct ArgsParam {
		ID3D11Buffer* buffer;
		UINT offset;
	};

	class ParticleEmitter
	{
	public:
		ParticleEmitter(const std::wstring& name);
		~ParticleEmitter();

		ParticleEmitter(const ParticleEmitter& other);

		void Initialize(ParticleConsts& pConsts, ParticleFrameConsts& pfConsts, DrawIndexedInstancedArgs& pMeshArgs);
		void OnSpawn();
		void PreUpdate(const float& dt, ParticleFrameConsts& fsConsts);
		void Update(const float& dt, ArgsParam args);
		void Render(ArgsParam billboardArgs, ArgsParam meshArgs);

		template<typename T>
		void AddModule(std::unique_ptr<ParticleModule>&& module);
		void AddModule(std::unique_ptr<ParticleModule>&& module);
		template<typename T>
		T* GetModule();
		
		void ClearModules() { m_modules.clear(); }
		void Reset();

		void SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id);
		
		// Texture Bake ������ ���� (Spawn ���)
		void SetBakedSpawnPath(const std::string& path);
		UINT LoadBakedSpawnData(std::vector<Vector3>& outBakedSpawnPos);
		const std::string& GetBakedPath() const { return m_bakedPath; }

		// ���յ� SpawnPos ����
		void SetSpawnPosInfo(UINT offset) { m_spawnPosPoolOffset = offset; }
		UINT GetSpawnPosOffset() const { return m_spawnPosPoolOffset; }
		
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

		using EventCallback = std::function<void(EmitterEvent, ParticleEmitter*)>; 
		void SetEventCallback(EventCallback cb);

		Vector3 GetSpawnPosition() const;
		void SetSpawnOffset(const Vector3& offset);
		const std::wstring& GetName() const;

		void SetMemoryInfo(UINT index);
		void SetOwner(ParticleSystem* system);
		UINT GetEmitterID() { return m_emitterID; };
		void SetName(std::wstring name) { m_name = name; }

		// 배치 렌더링 지원
		RenderModule* GetRenderModule();
	private:
		void ExecuteEvent(EmitterEvent event);
	private:
		std::wstring m_name;

		ParticleSystem* m_ownerSystem;
		UINT m_emitterID = 0;

		std::vector<std::unique_ptr<ParticleModule>> m_modules;

		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
		
		// SpawnPosition ���� (Baked/Custom)
		UINT m_spawnPosPoolOffset = 0;
		std::string m_bakedPath = "";
		UINT m_bakedCount = 0;
		std::vector<Vector3> m_customPositions;
		bool m_useCustomPositions = false;

		// SubEmitter
		float m_duration = -1.f;
		float m_completionDelay = 2.f;
		float m_elapsedTime = 0.f;
		bool m_isDurationEnded = false;
		bool m_isCompleted = false;
		bool m_isStarted = false;

		std::vector<SubEmitter> m_subEmitters;
		EventCallback m_eventCallback;
		Vector3 m_spawnOffset = Vector3(0.f);

		Vector3 m_initialSpawnPos = Vector3(0.f);
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