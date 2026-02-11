#pragma once
#include "ParticleContext.h"

namespace DE {
class ParticleEmitter;

enum class ModulePriority : uint8_t {
	Spawn = 1,
	Visual = 2,
	Force = 3,
	UpdateForce = 4,
	Material = 5,
	Render = 6
};

class ParticleModule
{
public:
	virtual ~ParticleModule() {}

	virtual void Initialize(ParticleInitContext& ctx) {}
	virtual void OnSpawn(SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void OnPreUpdate(const SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void OnUpdate(const SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void LateUpdate(SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void OnRender(const RenderContext& context) { if (!m_isEnabled) return; }
	
	//virtual void LoadFromJson();
	virtual ModulePriority GetPriority() = 0;
	virtual void LoadFromJson(const json& data) = 0;
	void SetEnable(const bool& enable) { m_isEnabled = enable; }

	virtual std::unique_ptr<ParticleModule> Clone() const = 0;
protected:
	bool m_isEnabled = true;
};

}
