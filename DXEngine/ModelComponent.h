#pragma once
#include "Component.h"
#include "Mesh.h"

namespace DE {
	struct MeshData;

	class ModelComponent : public Component
	{
		using Super = Component;
	public:
		ModelComponent(const std::wstring& name) : Super(name, ComponentType::Model) {}
		~ModelComponent() override {}

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;
		void RenderNormal();
		void RenderPoints();
		
		void SetModel(const std::wstring& name, const std::string& basePath, const std::string& filename, bool isGLTF = false);
		void SetModel(const std::vector<MeshData>& meshes, bool isGLTF = false);
		void SetModel(const MeshData& mesh, bool isGLTF = false);
		void SetDrawNormal(bool draw) { m_drawNormal = draw; }
		bool IsDrawNormal() { return m_drawNormal; }

		const ComPtr<ID3D11Buffer> GetBasicMaterial() { return m_basicMaterial.Get(); }
		const ComPtr<ID3D11Buffer> GetMaterial() { return m_material.Get(); }
		const MaterialConstants GetMaterialCpu() { return m_material.GetCpu(); }
		MaterialConstants& GetMaterialCpuRef() { return m_material.GetCpu(); }

		const void SetBasicMaterial(const BasicMaterialConstants& consts);
		const void SetMaterial(const MaterialConstants& consts);
	private:
		//Mesh triangle;
		std::vector<Mesh> m_meshes; // 하나의 모델이 내부적으로는 여러개의 메쉬로 구성

		ConstantBuffer<BasicMaterialConstants> m_basicMaterial;
		ConstantBuffer<MaterialConstants> m_material;

		bool m_drawNormal = true;
	};
}