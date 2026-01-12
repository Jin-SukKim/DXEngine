#pragma once

namespace DE {
class ParticleEmitter;

enum class ModulePriority : uint8_t {
	Spawn = 1,
	Visual = 2,
	Force = 3,
	Render = 4
};

class ParticleModule
{
public:
	virtual ~ParticleModule() {}

	virtual void Initialize(ID3D11Device* device, ParticleEmitter* owner);
	virtual void OnSpawn(ID3D11DeviceContext* context) {}
	virtual void PreUpdate(ID3D11DeviceContext* context, float dt) {}
	virtual void OnUpdate(ID3D11DeviceContext* context, float dt) {}
	virtual void OnRender(ID3D11DeviceContext* context) {}
	void SetEnable(const bool& enable) { m_isEnabled = enable; }
	//virtual void LoadFromJson();
	virtual ModulePriority GetPriority() = 0;
protected:
	ParticleEmitter* m_owner;
	bool m_isEnabled = true;
};

}
