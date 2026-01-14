#pragma once
#include "Particle.h"
#include "ComputeShader.h"
#include "AppendBuffer.h"
#include "StagingBuffer.h"
#include "IndirectArgsBuffer.h"
#include "BitonicSort.h"
#include "ParticleModule.h"
#include "FileWatcher.h"

namespace DE {
	class ParticleEmitter
	{
	public:
		ParticleEmitter(const std::wstring& name);
		~ParticleEmitter();

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

		ParticleConsts& GetConstsData() { return m_consts.GetCpu(); }
		ConstantBuffer<ParticleConsts>& GetConstBuffer() { return m_consts; }
		AppendBuffer<Particle>& GetConsumeBuffer() { return m_consume; }
	private:
		// 초기화 헬퍼 함수들
		void InitializeShaders(ID3D11Device* device);
		void InitializeBuffers(ComPtr<ID3D11Device>& device);

		// 업데이트 단계별 함수들
		void UpdateArgsBuffers(ID3D11DeviceContext* context);
		void UpdateSimulationStage(ID3D11DeviceContext* context);

	private:
		std::wstring m_name;
		// 파티클 버퍼 (핑퐁 버퍼링)
		AppendBuffer<Particle> m_consume;
		AppendBuffer<Particle> m_append;

		// 간접 디스패치 및 드로우 인수
		IndirectArgsBuffer<DispatchArgs> m_dispatchArgs;
		IndirectArgsBuffer<DrawInstancedArgs> m_drawInstancedArgs;
		
		// 활성 파티클 개수 추적용 카운터 버퍼
		ComPtr<ID3D11Buffer> m_countBuffer;
		ComPtr<ID3D11ShaderResourceView> m_countSRV;

		// 컴퓨트 셰이더들
		ComputeShader m_argsUpdateCS;
		ComputeShader m_particleCS;
		
		// 상수 버퍼
		ConstantBuffer<ParticleConsts> m_consts;

		std::vector<std::unique_ptr<ParticleModule>> m_modules;

		// Hot-Reload 관리
		std::wstring m_jsonPath;
		FileWatcher::CallbackID m_watcherID = 0;
	};

	template<typename T>
	void ParticleEmitter::AddModule(std::unique_ptr<ParticleModule>&& module)
	{
		// 이미 해당 타입의 모듈이 있는지 확인하고 교체
		for (auto& existingModule : m_modules)
		{
			// unique_ptr의 get()으로 Raw 포인터를 꺼내서 타입 검사
			if (dynamic_cast<T*>(existingModule.get()) != nullptr)
			{
				// 기존 모듈을 새로운 모듈로 교체 (소유권 이전)
				existingModule = std::move(module);
				break;
			}
		}

		// 없으면 새로 추가
		if (module)
			m_modules.emplace_back(std::move(module));

		// Priority에 따라 정렬
		// unique_ptr이므로 람다 인자를 unique_ptr& 로 받아야 함
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