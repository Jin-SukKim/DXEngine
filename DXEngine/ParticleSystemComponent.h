#pragma once
#include "Component.h"

namespace DE {
	class ParticleSystem;

	class ParticleSystemComponent : public Component
	{
		using Super = Component;
	public:
		ParticleSystemComponent(const std::wstring& name);
		~ParticleSystemComponent() override;

		void Initialize() override;
		void Update(const float& dt) override;
		void Render() override;

		void SetSystem(const std::wstring& path, const int& modelIdx = -1);
		ParticleSystem* GetSystem();

	private:
		void UpdateTransform(); // Transform 업데이트 함수 추가

	private:
		ParticleSystem* m_system;
	};
}

