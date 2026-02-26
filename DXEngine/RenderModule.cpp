// RenderModule.cpp ()
#include "pch.h"
#include "RenderModule.h"
#include "ParticleEmitter.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"
#include "IndirectArgsBuffer.h"
#include "Vertex.h"
#include "ParticleManager.h"

namespace DE {

	void RenderModule::Initialize(ParticleInitContext& ctx)
	{
		ParticleModule::Initialize(ctx);
	}

	void RenderModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		SetBlendState();
	}

	void RenderModule::SetBlendState()
	{
		switch (blendMode)
		{
		case BlendMode::Additive:
			m_blendState = RenderBase::graphicsCommon.accumulateBS.Get();
			break;
		case BlendMode::AlphaBlend:
			m_blendState = RenderBase::graphicsCommon.alphaBS.Get();
			break;
		case BlendMode::Opaque:
			m_blendState = nullptr;
			break;
		case BlendMode::Modulate:
			m_blendState = RenderBase::graphicsCommon.modulateBS.Get();
			break;
		}
	}

	void RenderModule::LoadFromJson(const json& data)
	{
		if (data.contains("blendMode")) {
			std::string mode = data["blendMode"];
			if (mode == "Additive") blendMode = BlendMode::Additive;
			if (mode == "AlphaBlend") blendMode = BlendMode::AlphaBlend;
			if (mode == "Opaque") blendMode = BlendMode::Opaque;
			if (mode == "Modulate") blendMode = BlendMode::Modulate;
		}
		if (data.contains("lowResolution")) {
			lowResolution = data["lowResolution"].get<bool>();
		}
		// Modulate는 low-res 버퍼에서 무의미 (클리어 값 [0,0,0,0]과 곱연산 = 0)
		if (blendMode == BlendMode::Modulate) {
			lowResolution = false;
		}
		// Opaque는 씬 depth buffer에 직접 write해야 하므로 full-res 필수
		if (blendMode == BlendMode::Opaque) {
			lowResolution = false;
		}
	}

	void RenderModule::CopyBasicSettings(RenderModule* cloned) const
	{
		cloned->blendMode = this->blendMode;
		cloned->lowResolution = this->lowResolution;
		cloned->m_blendState = this->m_blendState;
		cloned->m_isEnabled = this->m_isEnabled;
	}
}