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

		m_sort.Sort(ctx.context);
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

	void BillboardRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);
		
		// Initialize args buffer
		DrawInstancedArgs args = {};
		args.vertexCountPerInstance = 0;  // Will be filled by CopyStructureCount later
		args.instanceCount = 1;           
		args.startVertexLocation = 0;
		args.startInstanceLocation = 0;

		m_argsBuffer.Initialize(ctx.device, args, 4);
	}

	void BillboardRenderModule::OnSpawn(SimulationContext& ctx)
	{
		RenderModule::OnSpawn(ctx);

		RenderConsts& consts = ctx.constBuffer.GetCpu().render;
		consts.textureIdx = m_textureIdx;
		consts.frameTiles = m_frameTiles;
		consts.frameCount = m_frameCount;
		consts.textureMode = static_cast<UINT>(m_textureMode);
		consts.singleTextureIdx = m_singleTextureIdx;
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

		// IndirectDraw
		ID3D11ShaderResourceView* sortSRVs[] = {
			ctx.particleSRV,
			m_sort.GetSRV()
		};

		ctx.context->VSSetShaderResources(0, 2, sortSRVs);
		
		ID3D11ShaderResourceView* texSRV = nullptr;
		switch (m_textureMode)
		{
		case BillboardTextureMode::Material:
			// Material ��� ��: MaterialModule���� ���ε� ���� (t0 ~ t4 �� ���)
			if (ctx.materialModule) {
				ctx.materialModule->BindMaterialForMesh(0);
			}
			break;

		case BillboardTextureMode::SingleTexture:
			// ���� �ؽ�ó ��� ��: t15 ���Կ� ���ε� (ParticleCommon.hlsli�� ��ġ)
			if (m_singleTextureIdx >= 0) {
				texSRV = TextureManager::Get().GetTextureSRV(m_singleTextureIdx);
				ctx.context->PSSetShaderResources(6, 1, &texSRV);
			}
			break;

		case BillboardTextureMode::TextureArray:
		default:
			// Texture Array�� ���� Global�ϰ� t14�� ���ε��Ǿ� �ִٰ� ����
			// ���� ���⼭ ���������� ���ε��ؾ� �Ѵٸ� TextureManager::Get().BindParticleTextures() ȣ��
			break;
		}

		ctx.context->DrawInstancedIndirect(m_argsBuffer.GetBuffer(), 0);

		// ����
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

	void MeshRenderModule::Initialize(ParticleInitContext& ctx)
	{
		RenderModule::Initialize(ctx);

		m_argsUpdateCS.Initialize(ctx.device, L"ParticleMeshArgsUpdateCS.hlsl");
		
		// Initialize mesh args buffer if model is already loaded
		if (m_modelIdx >= 0) {
			Model* model = ModelManager::Get().GetModel(m_modelIdx);
			if (model) {
				m_meshCount = static_cast<UINT>(model->meshes.size());
				
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
		}
	}

	void MeshRenderModule::OnSpawn(SimulationContext& ctx)
	{
		RenderModule::OnSpawn(ctx);

		if (m_modelIdx < 0)
			return;

		Model* model = ModelManager::Get().GetModel(m_modelIdx);
		if (!model)
			return;

		ctx.constBuffer.GetCpu().render.numMeshes = m_meshCount;
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

		// IndirectDraw
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

			// Args ����ü�� 20 byte (5 * 4byte)
			UINT argsOffset = i * 20;
			ctx.context->DrawIndexedInstancedIndirect(m_meshArgs.GetBuffer(), argsOffset);
		}

		// ����
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
}