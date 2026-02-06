#include "pch.h"
#include "VortexModule.h"
#include "ParticleEmitter.h"

namespace DE {
	void VortexModule::Initialize(ParticleInitContext& ctx)
	{
		// 상수 값 초기화 (OnSpawn에서 이동)
		VortexConsts& consts = ctx.consts.vortex;
		consts.vortexCenter = m_vortexCenter;
		consts.vortexStrength = m_vortexStrength;
		consts.vortexAxis = m_vortexAxis;
		consts.vortexFalloff = m_vortexFalloff;
		consts.vortexPull = m_vortexPull;
		consts.active = 1;
	}

	void VortexModule::LoadFromJson(const json& data)
	{
		if (data.contains("center")) m_vortexCenter = JsonToVec3(data["center"]);
		if (data.contains("strength")) m_vortexStrength = data["strength"];
		if (data.contains("axis")) m_vortexAxis = JsonToVec3(data["axis"]);
		if (data.contains("vortexFalloff")) m_vortexFalloff = data["vortexFalloff"];
		if (data.contains("pull")) m_vortexPull = JsonToVec2(data["pull"]);
	}

	std::unique_ptr<ParticleModule> VortexModule::Clone() const
	{
		auto cloned = std::make_unique<VortexModule>();

		cloned->m_vortexStrength = this->m_vortexStrength;
		cloned->m_vortexCenter = this->m_vortexCenter;
		cloned->m_vortexAxis = this->m_vortexAxis;
		cloned->m_vortexFalloff = this->m_vortexFalloff;
		cloned->m_vortexPull = this->m_vortexPull;
		cloned->m_isEnabled = this->m_isEnabled;

		return cloned;
	}
}