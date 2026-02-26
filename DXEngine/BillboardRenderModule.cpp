#include "pch.h"
#include "RenderModule.h"

namespace DE {

	// ===== BillboardRenderModule =====

	void BillboardRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);
		m_modelIdx = 0; // Billboard quad
		ctx.consts.render.softDistance = m_softDistance;
		ctx.consts.render.softMaxDist = m_softMaxDist;
		ctx.consts.render.softNearDist = m_softNearDist;
		ctx.consts.render.velocityStretchFactor = m_velocityStretchFactor;

		ctx.consts.render.uvDistortEnabled = m_uvDistortEnabled;
		ctx.consts.render.uvDistortFrequency = m_uvDistortFrequency;
		ctx.consts.render.uvDistortStrength = m_uvDistortStrength;
		ctx.consts.render.uvDistortScroll = m_uvDistortScroll;
		ctx.consts.render.alphaClipThreshold = m_alphaClipThreshold;
		ctx.consts.render.solidCircle = m_solidCircle;
		ctx.consts.render.centerWhiteIntensity = m_centerWhiteIntensity;
		ctx.consts.render.normalBillboard = m_normalBillboard;
	}

	void BillboardRenderModule::LoadFromJson(const json& data)
	{
		RenderModule::LoadFromJson(data);

		if (data.contains("softDistance"))
			m_softDistance = data["softDistance"].get<float>();
		if (data.contains("softMaxDist"))
			m_softMaxDist = data["softMaxDist"].get<float>();
		if (data.contains("softNearDist"))
			m_softNearDist = data["softNearDist"].get<float>();
		if (data.contains("velocityStretchFactor"))
			m_velocityStretchFactor = data["velocityStretchFactor"].get<float>();

		if (data.contains("noiseUVDistort")) {
			const auto& n = data["noiseUVDistort"];
			m_uvDistortEnabled = n.value("enabled", false);
			m_uvDistortFrequency = n.value("frequency", 1.0f);
			m_uvDistortStrength = n.value("strength", 0.05f);
			if (n.contains("scrollSpeed")) m_uvDistortScroll = JsonToVec2(n["scrollSpeed"]);
		}
		if (data.contains("alphaClipThreshold"))
			m_alphaClipThreshold = data["alphaClipThreshold"].get<float>();
		if (data.contains("solidCircle"))
			m_solidCircle = data["solidCircle"].get<bool>() ? 1 : 0;
		if (data.contains("centerWhiteIntensity"))
			m_centerWhiteIntensity = data["centerWhiteIntensity"].get<float>();
		if (data.contains("normalBillboard"))
			m_normalBillboard = data["normalBillboard"].get<bool>() ? 1 : 0;
	}

	std::unique_ptr<ParticleModule> BillboardRenderModule::Clone() const
	{
		auto cloned = std::make_unique<BillboardRenderModule>();
		CopyBasicSettings(cloned.get()); // Only blendMode and m_blendState
		cloned->m_softDistance = this->m_softDistance;
		cloned->m_softMaxDist = this->m_softMaxDist;
		cloned->m_softNearDist = this->m_softNearDist;
		cloned->m_velocityStretchFactor = this->m_velocityStretchFactor;
		cloned->m_uvDistortEnabled = this->m_uvDistortEnabled;
		cloned->m_uvDistortFrequency = this->m_uvDistortFrequency;
		cloned->m_uvDistortStrength = this->m_uvDistortStrength;
		cloned->m_uvDistortScroll = this->m_uvDistortScroll;
		cloned->m_alphaClipThreshold = this->m_alphaClipThreshold;
		cloned->m_solidCircle = this->m_solidCircle;
		cloned->m_centerWhiteIntensity = this->m_centerWhiteIntensity;
		cloned->m_normalBillboard = this->m_normalBillboard;
		return cloned;
	}

}