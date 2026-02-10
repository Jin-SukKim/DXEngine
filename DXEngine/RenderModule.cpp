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
		ctx.context->OMSetBlendState(m_blendState, RenderBase::graphicsCommon.particle.animPSO.blendFactor, 0xffffffff);
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
		RenderConsts& consts = ctx.consts.render;
		consts.textureIdx = m_textureIdx;
		consts.frameTiles = m_frameTiles;
		consts.frameCount = m_frameCount;
		consts.textureMode = static_cast<UINT>(m_textureMode);
		consts.singleTextureIdx = m_singleTextureIdx;
	}

	void BillboardRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);

		// GS 없는 인스턴싱 빌보드 PSO 사용
		
		ID3D11ShaderResourceView* texSRV = nullptr;
		switch (m_textureMode)
		{
		case BillboardTextureMode::Material:
			if (ctx.materialModule) {
				ctx.materialModule->BindMaterialForMesh(0);
			}
			break;

		case BillboardTextureMode::SingleTexture:
			if (m_singleTextureIdx >= 0) {
				texSRV = TextureManager::Get().GetTextureSRV(m_singleTextureIdx);
				ctx.context->PSSetShaderResources(0, 1, &texSRV);
			}
			break;

		case BillboardTextureMode::TextureArray:
		default:
			break;
		}

		// DrawIndexedInstancedIndirect (쿼드 메쉬 인스턴싱)
		ctx.context->DrawIndexedInstancedIndirect(ctx.billboardArgs->buffer, ctx.billboardArgs->offset);
	}

	void BillboardRenderModule::LoadFromJson(const json& data)
	{
		RenderModule::LoadFromJson(data);

		if (data.contains("useMaterial") && data["useMaterial"] == true)
			m_textureMode = BillboardTextureMode::Material;
		else if (data.contains("useSingleTexture") && data["useSingleTexture"] == true) {
			m_textureMode = BillboardTextureMode::SingleTexture;
			if (data.contains("texture"))
				m_singleTextureIdx = TextureManager::Get().LoadTexture(data["texture"], data.value("isSRGB", true));
		}
		else {
			m_textureMode = BillboardTextureMode::TextureArray;
			if (data.contains("texture")) {
				const auto [path, idx] = TextureManager::Get().LoadParticleTexture(data["texture"]);
				m_texturePath = path;
				m_textureIdx = idx;
			}
		}

		if (data.contains("sprite")) {
			auto& sprite = data["sprite"];
			if (sprite.contains("frameTiles")) m_frameTiles = JsonToVec2(sprite["frameTiles"]);
			if (sprite.contains("frameCount")) m_frameCount = sprite["frameCount"];
		}
	}

	std::unique_ptr<ParticleModule> BillboardRenderModule::Clone() const
	{
		auto cloned = std::make_unique<BillboardRenderModule>();

		// ⺻  
		CopyBasicSettings(cloned.get());

		// Billboard  
		cloned->m_textureMode = this->m_textureMode;
		cloned->m_texturePath = this->m_texturePath;
		cloned->m_textureIdx = this->m_textureIdx;
		cloned->m_singleTextureIdx = this->m_singleTextureIdx;
		cloned->m_frameTiles = this->m_frameTiles;
		cloned->m_frameCount = this->m_frameCount;

		// GPU ۴ ParticleEmitter 

		return cloned;
	}

	// ===== MeshRenderModule =====

	void MeshRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);

		if (m_modelIdx < 0)
			return;
		Model* model = ModelManager::Get().GetModel(m_modelIdx);
		if (!model)
			return;

		// ParticleEmitter ޽ Args  ʱȭ
		auto& mesh = model->meshes[0];
		ctx.consts.render.indexCount = mesh.indexCount;
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

			ctx.context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			ctx.context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

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