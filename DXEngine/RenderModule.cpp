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
		}
	}

	void RenderModule::LoadFromJson(const json& data)
	{
		if (data.contains("blendMode")) {
			std::string mode = data["blendMode"];
			if (mode == "Additive") blendMode = BlendMode::Additive;
			if (mode == "AlphaBlend") blendMode = BlendMode::AlphaBlend;
			if (mode == "Opaque") blendMode = BlendMode::Opaque;
		}
		if (data.contains("lowResolution")) {
			lowResolution = data["lowResolution"].get<bool>();
		}
	}

	void RenderModule::CopyBasicSettings(RenderModule* cloned) const
	{
		cloned->blendMode = this->blendMode;
		cloned->lowResolution = this->lowResolution;
		cloned->m_blendState = this->m_blendState;
		cloned->m_isEnabled = this->m_isEnabled;
	}

	// ===== BillboardRenderModule =====

	void BillboardRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);
		m_modelIdx = 0; // Billboard quad
		ctx.consts.render.softDistance = m_softDistance;
		ctx.consts.render.velocityStretchFactor = m_velocityStretchFactor;

		ctx.consts.render.uvDistortEnabled = m_uvDistortEnabled;
		ctx.consts.render.uvDistortFrequency = m_uvDistortFrequency;
		ctx.consts.render.uvDistortStrength = m_uvDistortStrength;
		ctx.consts.render.uvDistortScroll = m_uvDistortScroll;
	}

	void BillboardRenderModule::LoadFromJson(const json& data)
	{
		RenderModule::LoadFromJson(data);

		if (data.contains("softDistance"))
			m_softDistance = data["softDistance"].get<float>();
		if (data.contains("velocityStretchFactor"))
			m_velocityStretchFactor = data["velocityStretchFactor"].get<float>();

		if (data.contains("noiseUVDistort")) {
			const auto& n = data["noiseUVDistort"];
			m_uvDistortEnabled = n.value("enabled", false);
			m_uvDistortFrequency = n.value("frequency", 1.0f);
			m_uvDistortStrength = n.value("strength", 0.05f);
			if (n.contains("scrollSpeed")) m_uvDistortScroll = JsonToVec2(n["scrollSpeed"]);
		}
	}

	std::unique_ptr<ParticleModule> BillboardRenderModule::Clone() const
	{
		auto cloned = std::make_unique<BillboardRenderModule>();
		CopyBasicSettings(cloned.get()); // Only blendMode and m_blendState
		cloned->m_softDistance = this->m_softDistance;
		cloned->m_velocityStretchFactor = this->m_velocityStretchFactor;
		cloned->m_uvDistortEnabled = this->m_uvDistortEnabled;
		cloned->m_uvDistortFrequency = this->m_uvDistortFrequency;
		cloned->m_uvDistortStrength = this->m_uvDistortStrength;
		cloned->m_uvDistortScroll = this->m_uvDistortScroll;
		return cloned;
	}

	// ===== MeshRenderModule =====

	void MeshRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);

	}

	void MeshRenderModule::LoadFromJson(const json& data)
	{
		RenderModule::LoadFromJson(data);
		if (data.contains("model"))
			m_modelIdx = ModelManager::Get().LoadModel(data["model"], data.value("basePath", ""), data.value("isGLTF", false));
		else if (data.contains("defaultMesh")) {
			switch (static_cast<int>(data["defaultMesh"])) {
			case 0:
				m_modelIdx = ModelManager::Get().LoadModel("ParticleBox", GeometryGenerator::MakeBox());
				break;
			case 1:
				m_modelIdx = ModelManager::Get().LoadModel("ParticleSphere", GeometryGenerator::MakeSphere(1.f, 10, 10));
				break;
			}
		}
		else {
			m_modelIdx = ModelManager::Get().LoadModel("ParticleBox", GeometryGenerator::MakeBox());
		}
	}

	std::unique_ptr<ParticleModule> MeshRenderModule::Clone() const
	{
		auto cloned = std::make_unique<MeshRenderModule>();

		// ⺻  
		CopyBasicSettings(cloned.get());

		// Mesh  
		cloned->m_modelIdx = this->m_modelIdx;
		cloned->m_meshCount = this->m_meshCount;

		// GPU ۴ ParticleEmitter 

		return cloned;
	}

}