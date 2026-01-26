#pragma once
#include "Component.h"
#include "ModelManager.h"   // ModelManager 포함
#include "MaterialSystem.h" // MaterialSystem 포함

namespace DE {

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

		// ModelManager를 통해 로드하고 인덱스만 저장
		void SetModel(const std::string& name, const std::string& basePath = "", bool isGLTF = false);
		void SetModel(const std::string& name, const MeshData& meshData);

		void SetDrawNormal(bool draw) { m_drawNormal = draw; }
		bool IsDrawNormal() { return m_drawNormal; }

		// BasicMaterial은 Actor별 고유 데이터(HashID 등)이므로 유지
		const void SetBasicMaterial(const BasicMaterialConstants& consts);

		// 모델 정보 접근
		int GetModelIndex() const { return m_modelIndex; }
		void SetMaterialIndex(int index) { m_matIdx = index; }
	private:
		int m_modelIndex = -1; // ModelManager에서 관리하는 모델의 인덱스

		ConstantBuffer<BasicMaterialConstants> m_basicMaterial; // HashID 등 기본 정보
		bool m_drawNormal = true;
		int m_matIdx = -1;
	};
}