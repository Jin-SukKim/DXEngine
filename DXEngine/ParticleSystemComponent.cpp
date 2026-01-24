#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleManager.h"
#include "TransformComponent.h"
#include "Actor.h"

namespace DE {
	ParticleSystemComponent::ParticleSystemComponent(const std::wstring& name) 
		: Component(name, ComponentType::ParticleSystem)
		, m_system(nullptr)
	{
	}

	ParticleSystemComponent::~ParticleSystemComponent()
	{
		// 활성 시스템에서 제거
		if (m_system) {
			ParticleManager::Get().UnregisterActiveSystem(m_system);
		}
		m_system = nullptr;
	}

	void ParticleSystemComponent::Initialize()
	{
		Component::Initialize();
		
		// ParticleManager에서 초기화하므로 여기서는 Transform만 업데이트
		UpdateTransform();
	}

	void ParticleSystemComponent::Update(const float& dt)
	{
		Component::Update(dt);
		
		// Transform 업데이트 (매 프레임 Actor의 Transform 반영)
		UpdateTransform();
		
		// ParticleManager에서 업데이트하므로 여기서는 스킵
	}

	void ParticleSystemComponent::Render()
	{
		Component::Render();
		
		// ParticleManager에서 렌더링하므로 여기서는 스킵
	}

	void ParticleSystemComponent::UpdateTransform()
	{
		if (!m_system) return;

		// Owner Actor의 TransformComponent 가져오기
		Actor* owner = dynamic_cast<Actor*>(GetOwner());
		if (!owner) return;

		TransformComponent* tr = owner->GetComponent<TransformComponent>();
		if (!tr) return;

		// Transform 정보를 ParticleSystem에 전달
		MeshConstants meshConsts;
		meshConsts.world = tr->GetTransformMatrix().Transpose();
		meshConsts.worldIT = meshConsts.world.Invert();
		
		m_system->SetTransform(meshConsts);
	}
		
	void ParticleSystemComponent::SetSystem(const std::wstring& path, const int& modelIdx)
	{
		// 기존 시스템 해제
		if (m_system) {
			ParticleManager::Get().UnregisterActiveSystem(m_system);
		}

		// 새 시스템 생성 (자동으로 활성 시스템에 등록됨)
		m_system = ParticleManager::Get().CreateSystem(path);
		
		if (m_system && modelIdx >= 0) {
			m_system->SetTargetMesh(modelIdx);
			// 타겟 메시 변경 후 재초기화
			m_system->Initialize();
			m_system->OnSpawn();
		}

		// Transform 초기 설정
		UpdateTransform();
	}

	ParticleSystem* ParticleSystemComponent::GetSystem()
	{
		return m_system;
	}
}