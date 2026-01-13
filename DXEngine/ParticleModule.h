#pragma once
#include "ParticleContext.h"

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

	virtual void Initialize(ParticleInitContext& ctx) {}
	virtual void OnSpawn(SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void PreUpdate(SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void OnUpdate(const SimulationContext& context) { if (!m_isEnabled) return; }
	virtual void OnRender(const RenderContext& context) { if (!m_isEnabled) return; }
	//virtual void LoadFromJson();
	virtual ModulePriority GetPriority() = 0;
	virtual void LoadFromJson(const json& data) = 0;
	void SetEnable(const bool& enable) { m_isEnabled = enable; }
protected:
	bool m_isEnabled = true;
};

}
