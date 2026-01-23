#pragma once
#include "Component.h"

namespace DE {
	class ParticleSystem;
	struct MeshData;

class ParticleSystemComponent : public Component
{
public:
	ParticleSystemComponent(const std::wstring& name);
	~ParticleSystemComponent() override;

	void Initialize() override;
	void Update(const float& dt) override;
	void Render() override;

	void SetSystem(const std::wstring& path, const MeshData* meshData = nullptr);
	ParticleSystem* GetSystem();
private:
	ParticleSystem* m_system;
};
}

