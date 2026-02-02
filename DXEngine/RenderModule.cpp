// RenderModule.cpp (수정)
#include "pch.h"
#include "RenderModule.h"
#include "ParticleEmitter.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"
#include "IndirectArgsBuffer.h"
#include "MaterialSystem.h"
#include "MaterialModule.h"

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

	void RenderModule::UpdateArgs(const RenderContext& ctx)
	{
		ParticleModule::UpdateArgs(ctx);
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

	void BillboardRenderModule::UpdateArgs(const RenderContext& ctx)
	{
		RenderModule::UpdateArgs(ctx);

	}

	void BillboardRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);

		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);

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

		ctx.context->DrawInstancedIndirect(ctx.billboardArgs.GetBuffer(), ctx.billbaordArgsOffset);
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

		// 기본 설정 복사
		CopyBasicSettings(cloned.get());

		// Billboard 전용 복사
		cloned->m_textureMode = this->m_textureMode;
		cloned->m_texturePath = this->m_texturePath;
		cloned->m_textureIdx = this->m_textureIdx;
		cloned->m_singleTextureIdx = this->m_singleTextureIdx;
		cloned->m_frameTiles = this->m_frameTiles;
		cloned->m_frameCount = this->m_frameCount;

		// GPU 버퍼는 ParticleEmitter가 관리

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

		// ParticleEmitter의 메시 Args 버퍼 초기화
		auto& mesh = model->meshes[0];
		ctx.meshArgs.indexCountPerInstance = mesh.indexCount;
		m_meshCount = static_cast<UINT>(model->meshes.size());
		ctx.consts.render.numMeshes = m_meshCount;
	}

	void MeshRenderModule::UpdateArgs(const RenderContext& ctx)
	{
		ID3D11UnorderedAccessView* uavs[] = { ctx.meshArgsBuffer.GetUAV() };
		ctx.context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
		
		// ComputeCommon의 공용 ComputePSO 사용
		auto& meshArgsUpdateCS = RenderBase::computeCommon.particle.meshArgsUpdateCS;
		ctx.context->CSSetShader(meshArgsUpdateCS.computeShader.Get(), 0, 0);
		UINT groupCount = (m_meshCount + 1023) / 1024;
		ctx.context->Dispatch(groupCount, 1, 1);
		
		// Barrier
		ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
		ID3D11UnorderedAccessView* nullUAVs[1] = { nullptr };
		ctx.context->CSSetShaderResources(0, 1, nullSRVs);
		ctx.context->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
		ctx.context->CSSetShader(nullptr, 0, 0);
	}

	void MeshRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);

		Model* model = ModelManager::Get().GetModel(m_modelIdx);
		if (!model) return;

		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.meshPSO);

		for (UINT i = 0; i < model->meshes.size(); ++i) {
			auto& mesh = model->meshes[i];

			if (ctx.materialModule)
				ctx.materialModule->BindMaterialForMesh(i);
			else
				MaterialSystem::Get().BindMaterial(0);

			ctx.context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			ctx.context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

			ctx.context->DrawIndexedInstancedIndirect(ctx.meshArgsBuffer.GetBuffer(), ctx.meshArgsOffset);
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

		// 기본 설정 복사
		CopyBasicSettings(cloned.get());

		// Mesh 전용 복사
		cloned->m_modelIdx = this->m_modelIdx;
		cloned->m_meshCount = this->m_meshCount;

		// GPU 버퍼는 ParticleEmitter가 관리

		return cloned;
	}

}