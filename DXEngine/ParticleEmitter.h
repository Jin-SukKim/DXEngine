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
		
		// Texture Bake 데이터 설정 (Spawn 모듈용)
		void LoadBakedSpawnData(const std::string& path);

		ParticleConsts& GetConstsData() { return m_consts.GetCpu(); }
		ParticleFrameConsts& GetFrameConstsData() { return m_frameConsts.GetCpu(); }
		ConstantBuffer<ParticleConsts>& GetConstBuffer() { return m_consts; }
		AppendBuffer<Particle>& GetConsumeBuffer() { return m_consume; }
		
		// Baked Spawn Position 접근자
		StructuredBuffer<Vector3>* GetBakedSpawnBuffer() { return &m_bakedSpawnPos; }
		UINT GetBakedCount() const { return m_bakedCount; }
		
		// Render 버퍼 접근자
		BitonicSort* GetSortBuffer() { return &m_sortBuffer; }
		IndirectArgsBuffer<DrawInstancedArgs>* GetBillboardArgsBuffer() { return &m_billboardArgsBuffer; }
		IndirectArgsBuffer<DrawIndexedInstancedArgs>* GetMeshArgsBuffer() { return &m_meshArgsBuffer; }
		
	private:
		// 초기화 관련 함수들
		void InitializeBuffers(ComPtr<ID3D11Device>& device);

		// 업데이트 단계별 함수들
		void UpdateArgsBuffers(ID3D11DeviceContext* context);

	private:
		std::wstring m_name;
		// 파티클 버퍼 (이중 버퍼링)
		AppendBuffer<Particle> m_consume;
		AppendBuffer<Particle> m_append;

		// 간접 디스패치
		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		
		// 활성 파티클 개수 버퍼를 카운터 버퍼
		ComPtr<ID3D11Buffer> m_countBuffer;
		ComPtr<ID3D11ShaderResourceView> m_countSRV;

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