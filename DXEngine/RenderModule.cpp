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

		m_InitSortKeysCS.Initialize(ctx.device, L"InitBitonicSortCS.hlsl");
		m_sort.Initialize(ctx.device, ctx.frameConsts.maxParticles, L"BitonicSortCS.hlsl");
	}

	void RenderModule::OnSpawn(SimulationContext& ctx)
	{
		ParticleModule::OnSpawn(ctx);
		SetBlendState();
	}

	void RenderModule::UpdateArgs(const SimulationContext& ctx)
	{
		ParticleModule::UpdateArgs(ctx);
	}

	void RenderModule::OnUpdate(const SimulationContext& ctx)
	{
		ID3D11UnorderedAccessView* uav[1] = { m_sort.GetUAV() };
		ctx.context->CSSetUnorderedAccessViews(0, 1, uav, nullptr);
		ID3D11ShaderResourceView* srvs[] = {
			ctx.appendBuffer.GetSRV(),
			ctx.countSRV
		};
		ctx.context->CSSetShaderResources(0, 2, srvs);
		m_InitSortKeysCS.Dispatch(ctx.context, (ctx.frameConstBuffer.GetCpu().maxParticles + 255) / 256, 1, 1);

		//m_sort.Sort(ctx.context);
	}

	void RenderModule::OnRender(const RenderContext& ctx)
	{
		ParticleModule::OnRender(ctx);
		ctx.context->OMSetBlendState(m_blendState, RenderBase::graphicsCommon.particle.animPSO.blendFactor, 0xffffffff);
		ctx.context->VSSetConstantBuffers(5, 1, ctx.constBuffer.GetAddressOf());
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

	//  공통 복사 메서드 추가
	void RenderModule::CopyBasicSettings(RenderModule* cloned) const
	{
		cloned->blendMode = this->blendMode;
		cloned->m_blendState = this->m_blendState;
		cloned->m_isEnabled = this->m_isEnabled;
		// m_sort, m_InitSortKeysCS는 Initialize()에서 재생성
	}

	// ===== BillboardRenderModule =====

	void BillboardRenderModule::OnSpawn(SimulationContext& ctx)
	{
		RenderModule::OnSpawn(ctx);

		RenderConsts& consts = ctx.constBuffer.GetCpu().render;
		consts.textureIdx = m_textureIdx;
		consts.frameTiles = m_frameTiles;
		consts.frameCount = m_frameCount;
		consts.textureMode = static_cast<UINT>(m_textureMode);
		consts.singleTextureIdx = m_singleTextureIdx;

		m_argsBuffer.Reset();
		DrawInstancedArgs args = {};
		args.vertexCountPerInstance = 0;
		args.instanceCount = 1;
		args.startVertexLocation = 0;
		args.startInstanceLocation = 0;

		m_argsBuffer.Initialize(ctx.device, args, 4);
	}

	void BillboardRenderModule::UpdateArgs(const SimulationContext& ctx)
	{
		RenderModule::UpdateArgs(ctx);
		ctx.context->CopyStructureCount(m_argsBuffer.GetBuffer(), 0, ctx.consumeBuffer.GetUAV());
	}

	void BillboardRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);
		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.animPSO);

		ID3D11ShaderResourceView* sortSRVs[] = {
			ctx.particleSRV,
			m_sort.GetSRV()
		};
		ctx.context->VSSetShaderResources(0, 2, sortSRVs);

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
				ctx.context->PSSetShaderResources(6, 1, &texSRV);
			}
			break;

		case BillboardTextureMode::TextureArray:
		default:
			break;
		}

		ctx.context->DrawInstancedIndirect(m_argsBuffer.GetBuffer(), 0);

		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		ctx.context->VSSetShaderResources(0, 2, nullSRVs);
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

	//  Clone 수정
	std::unique_ptr<ParticleModule> BillboardRenderModule::Clone() const
	{
		auto cloned = std::make_unique<BillboardRenderModule>();

		// 기본 설정 복사
		CopyBasicSettings(cloned.get());

		// Billboard 설정 복사
		cloned->m_textureMode = this->m_textureMode;
		cloned->m_texturePath = this->m_texturePath;
		cloned->m_textureIdx = this->m_textureIdx;
		cloned->m_singleTextureIdx = this->m_singleTextureIdx;
		cloned->m_frameTiles = this->m_frameTiles;
		cloned->m_frameCount = this->m_frameCount;

		//  GPU 버퍼는 복사하지 않음 (OnSpawn에서 재생성)
		// m_argsBuffer는 복사 안 함!

		return cloned;
	}

	// ===== MeshRenderModule =====

	void MeshRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);
		m_argsUpdateCS.Initialize(ctx.device, L"ParticleMeshArgsUpdateCS.hlsl");
	}

	void MeshRenderModule::OnSpawn(SimulationContext& ctx)
	{
		RenderModule::OnSpawn(ctx);

		if (m_modelIdx < 0)
			return;

		Model* model = ModelManager::Get().GetModel(m_modelIdx);
		if (!model)
			return;

		m_meshCount = static_cast<UINT>(model->meshes.size());
		ctx.constBuffer.GetCpu().render.numMeshes = m_meshCount;

		std::vector<DrawIndexedInstancedArgs> allArgs(m_meshCount);
		for (size_t i = 0; i < model->meshes.size(); ++i) {
			auto& mesh = model->meshes[i];

			allArgs[i].indexCountPerInstance = mesh.indexCount;
			allArgs[i].instanceCount = 0;
			allArgs[i].startIndexLocation = 0;
			allArgs[i].baseVertexLocation = 0;
			allArgs[i].startInstanceLocation = 0;
		}

		m_meshArgs.Initialize(ctx.device, allArgs, m_meshCount, static_cast<UINT>(sizeof(DrawIndexedInstancedArgs)), 5);
	}

	void MeshRenderModule::UpdateArgs(const SimulationContext& ctx)
	{
		ctx.context->CSSetShaderResources(0, 1, &ctx.countSRV);

		ID3D11UnorderedAccessView* uavs[] = { m_meshArgs.GetUAV() };
		ctx.context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
		UINT groupCount = (m_meshCount + 255) / 256;
		m_argsUpdateCS.Dispatch(ctx.context, groupCount, 1, 1);
	}

	void MeshRenderModule::OnRender(const RenderContext& ctx)
	{
		RenderModule::OnRender(ctx);

		Model* model = ModelManager::Get().GetModel(m_modelIdx);
		if (!model) return;

		GET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.meshPSO);

		ID3D11ShaderResourceView* sortSRVs[] = { ctx.particleSRV, m_sort.GetSRV() };
		ctx.context->VSSetShaderResources(1, 2, sortSRVs);

		for (UINT i = 0; i < model->meshes.size(); ++i) {
			auto& mesh = model->meshes[i];

			if (ctx.materialModule)
				ctx.materialModule->BindMaterialForMesh(i);
			else
				MaterialSystem::Get().BindMaterial(0);

			ctx.context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &mesh.stride, &mesh.offset);
			ctx.context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

			UINT argsOffset = i * 20;
			ctx.context->DrawIndexedInstancedIndirect(m_meshArgs.GetBuffer(), argsOffset);
		}

		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		ctx.context->VSSetShaderResources(0, 2, nullSRVs);
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

	//  Clone 수정
	std::unique_ptr<ParticleModule> MeshRenderModule::Clone() const
	{
		auto cloned = std::make_unique<MeshRenderModule>();

		// 기본 설정 복사
		CopyBasicSettings(cloned.get());

		// Mesh 설정 복사
		cloned->m_modelIdx = this->m_modelIdx;
		cloned->m_meshCount = this->m_meshCount;

		//  GPU 버퍼는 복사하지 않음 (OnSpawn에서 재생성)
		// m_meshArgs는 복사 안 함!

		return cloned;
	}

}