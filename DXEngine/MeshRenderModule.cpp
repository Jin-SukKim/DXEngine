#include "pch.h"
#include "RenderModule.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"

namespace DE {
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

		CopyBasicSettings(cloned.get());

		cloned->m_modelIdx = this->m_modelIdx;
		cloned->m_meshCount = this->m_meshCount;

		return cloned;
	}

}