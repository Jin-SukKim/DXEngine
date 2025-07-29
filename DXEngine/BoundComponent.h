#pragma once
#include "Component.h"

namespace DE {
	struct Mesh;
	struct MeshData;

	class BoundComponent : public Component
	{
		using Super = Component;
	public:
		BoundComponent(const std::wstring& name) : Super(name, ComponentType::BoundingVolume) {}
		~BoundComponent() override {}

		void SetBoundingVolume(const std::vector<MeshData>& meshes);

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;

		bool IsPickable() { return m_isPickable; }
		const DirectX::BoundingBox& GetBoundingBox() const { return m_boundingBox; }
		const DirectX::BoundingSphere& GetBoundingSphere() const { return m_boundingSphere; }
		
		void SetVisibility(const bool& visible) { m_drawBound = visible; }
		void SetScale(const Vector3& scale);
	private:
		// Model 크기에 맞는 Bounding Box 설정
		void setBoundingBox(const std::vector<DE::MeshData>& meshes);
		// Model 크기에 맞는 Bounding Sphere 설정
		void setBoundingSphere(const std::vector<DE::MeshData>& meshes);
		
		// Model을 감싸는 BoundingBox 계산
		DirectX::BoundingBox getBoundingBox(const std::vector<Vertex>& vertices);
		// 다른 Bounding Box와 비교해서 더 큰 Bounding Box로 변환
		void extendBoundingBox(const DirectX::BoundingBox& inBox, DirectX::BoundingBox& outBox);

	private:
		DirectX::BoundingBox m_boundingBox;
		DirectX::BoundingSphere m_boundingSphere;

		std::shared_ptr<Mesh> m_boundingBoxMesh;
		std::shared_ptr<Mesh> m_boundingSphereMesh;

		bool m_drawBound = false;
		bool m_isPickable = true;
	};
}
