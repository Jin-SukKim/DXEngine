#pragma once
#include "Component.h"
#include "Mesh.h"

namespace DE {
	struct MeshData;

	class ModelComponent : public Component
	{
		using Super = Component;
	public:
		ModelComponent(ComPtr<ID3D11Device>& device, const std::wstring& name) : Super(device, name, ComponentType::Model) {}
		~ModelComponent() override {}

		void Initialize() override;
		void Update(ComPtr<ID3D11DeviceContext>& context, const float& deltaTime) override;
		void Render(ComPtr<ID3D11DeviceContext>& context) override;
		void RenderNormal(ComPtr<ID3D11DeviceContext>& context);
		
		void SetModel(ComPtr<ID3D11Device>& device, const std::wstring& name, const std::string& basePath, const std::string& filename);
		void SetModel(ComPtr<ID3D11Device>& device, const std::vector<MeshData>& meshes);
		void SetModel(ComPtr<ID3D11Device>& device, const MeshData& mesh);
		void SetDrawNormal(bool draw) { m_drawNormal = draw; }
		bool IsDrawNormal() { return m_drawNormal; }
		
		const ComPtr<ID3D11Buffer> GetConsts() { return m_constant.Get(); }
		const ComPtr<ID3D11Buffer> GetBasicMaterial() { return m_basicMaterial.Get(); }
	private:
		bool updateWorldCpu();
	private:
		//Mesh triangle;
		std::vector<Mesh> m_meshes; // 하나의 모델이 내부적으로는 여러개의 메쉬로 구성

		ConstantBuffer<MeshConstants> m_constant;
		ConstantBuffer<BasicMaterialConstants> m_basicMaterial;

		bool m_drawNormal = true;
	};
}