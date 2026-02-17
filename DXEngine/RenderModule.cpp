// RenderModule.cpp ()
#include "pch.h"
#include "RenderModule.h"
#include "ParticleEmitter.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"
#include "IndirectArgsBuffer.h"
#include "MaterialSystem.h"
#include "MaterialModule.h"
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

	void RenderModule::OnRender(const RenderContext& ctx)
	{
		ParticleModule::OnRender(ctx);
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
	}

	void RenderModule::CopyBasicSettings(RenderModule* cloned) const
	{
		cloned->blendMode = this->blendMode;
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
	}

	void BillboardRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);

		// Always use Material system for texture binding
		if (ctx.materialModule)
			ctx.materialModule->BindMaterialForMesh(0);

		// DrawIndexedInstancedIndirect (instanced billboard quad)
		ctx.context->DrawIndexedInstancedIndirect(ctx.billboardArgs->buffer, ctx.billboardArgs->offset);
	}

	void BillboardRenderModule::LoadFromJson(const json& data)
	{
		RenderModule::LoadFromJson(data); // Only parses blendMode
		if (data.contains("softDistance"))
			m_softDistance = data["softDistance"].get<float>();
		if (data.contains("velocityStretchFactor"))
			m_velocityStretchFactor = data["velocityStretchFactor"].get<float>();
		// All texture loading moved to MaterialModule
	}

	std::unique_ptr<ParticleModule> BillboardRenderModule::Clone() const
	{
		auto cloned = std::make_unique<BillboardRenderModule>();
		CopyBasicSettings(cloned.get()); // Only blendMode and m_blendState
		cloned->m_softDistance = this->m_softDistance;
		cloned->m_velocityStretchFactor = this->m_velocityStretchFactor;
		return cloned;
	}

	// ===== MeshRenderModule =====

	void MeshRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);

	}

	void MeshRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);

		Model* model = ModelManager::Get().GetModel(m_modelIdx);
		if (!model) return;

		for (UINT i = 0; i < model->meshes.size(); ++i) {
			auto& mesh = model->meshes[i];

			if (ctx.materialModule)
				ctx.materialModule->BindMaterialForMesh(i);
			else
				MaterialSystem::Get().BindMaterial(0);

			ctx.context->DrawIndexedInstancedIndirect(ctx.meshArgs->buffer, ctx.meshArgs->offset);
		}
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