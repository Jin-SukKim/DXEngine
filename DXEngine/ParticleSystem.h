#pragma once
#include "Actor.h"
#include "ParticleEmitter.h"

namespace DE {

class ParticleSystem : public Actor
{
public:
	ParticleSystem(const std::wstring& name);
	~ParticleSystem() override {}

	void Initialize() override;
	void OnSpawn();
	void Update(const float& dt) override;
	void Render() override;

	void AddEmitter(const std::string& path);
	void AddEmitter(std::unique_ptr<ParticleEmitter>&& emitter);
	void ClearEmitters();
	void LoadFromJson(const json& data);
private:
	std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
	UINT m_looping = 1;
	std::string m_state = "Play";
};

}
