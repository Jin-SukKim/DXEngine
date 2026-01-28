#include "pch.h"
#include "SnowActor.h"
#include "ParticleManager.h"
#include "TransformComponent.h"

namespace DE {
	SnowActor::SnowActor(const std::wstring& name) : Super(name)
	{
		m_particle = ParticleManager::Get().CreateSystem(L"Particles\\Effects\\SpawnModule\\SpawnWorldSpace.json");
		m_particle->SetTarget(this);
	}

	SnowActor::~SnowActor()
	{
		ParticleManager::Get().DestroyInstance(m_particle);
	}

	void SnowActor::Update(const float& deltaTime)
	{
		Super::Update(deltaTime);

        auto* tr = this->GetComponent<TransformComponent>();
        if (tr) {
            // 1. 회전의 중심점 (Pivot) 설정
            Vector3 center(-20.0f, 0.0f, 0.0f);

            // 2. 현재 위치를 가져와서 중심점 기준의 상대 좌표(Relative Position)로 변환
            Vector3 currentPos = tr->GetPos();
            Vector3 relativePos = currentPos - center; // 중심점이 (0,0,0)인 것처럼 이동

            // 3. 회전 계산 (Y축 기준)
            float orbitSpeed = 0.3f; // 회전 속도
            float theta = orbitSpeed * deltaTime;

            // 회전 행렬 공식 적용 (x, z 평면 회전)
            float newRelX = relativePos.x * cos(theta) - relativePos.z * sin(theta);
            float newRelZ = relativePos.x * sin(theta) + relativePos.z * cos(theta);

            // 4. 회전된 상대 좌표에 중심점을 다시 더해 월드 좌표로 설정
            Vector3 finalPos = center + Vector3(newRelX, relativePos.y, newRelZ);

            tr->SetPos(finalPos);

            // (선택 사항) 액터가 바라보는 방향도 회전 방향에 맞추고 싶다면
            // tr->Rotate(0.0f, theta, 0.0f); // 공전과 함께 자전도 시키거나
            // tr->LookAt(finalPos + (finalPos - currentPos)); // 이동 방향을 보게 설정 가능
        }
	}
}