#include "pch.h"
#include "BoundComponent.h"
#include "MeshData.h"
#include "GeometryGenerator.h"
#include "Mesh.h"

#include "Actor.h"
#include "ModelComponent.h"
#include "TransformComponent.h"
#include "RenderBase.h"

namespace DE {
	void BoundComponent::SetBoundingVolume(const std::vector<MeshData>& meshes)
	{
		// Model 크기에 맞는 Bounding Box 설정
		setBoundingBox(meshes);

		// Model 크기에 맞는 Bounding Sphere 설정
		setBoundingSphere(meshes);

		SetVisibility(true);
	}

	void BoundComponent::Initialize()
	{
	}

	void BoundComponent::Update(const float& deltaTime)
	{

	}

	void BoundComponent::Render()
	{
		if (!m_drawBound)
			return;

		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		ID3D11Buffer* constBuffers[3] = {
			// Bounding Box와 Bounding Sphere의 Constant Data는 같음
			m_boundingBoxMesh->basicMaterialConstGPU.Get(),
			m_boundingBoxMesh->meshConstGPU.Get(),
			m_boundingBoxMesh->materialConstGPU.Get(),
		};
		context->VSSetConstantBuffers(1, 3, constBuffers);

		// Bounding Box Rendering
		context->IASetVertexBuffers(0, 1, m_boundingBoxMesh->vertexBuffer.GetAddressOf(), &m_boundingBoxMesh->stride, &m_boundingBoxMesh->offset);
		context->IASetIndexBuffer(m_boundingBoxMesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(m_boundingBoxMesh->indexCount, 0, 0);

		// Bounding Sphere Rendering
		context->IASetVertexBuffers(0, 1, m_boundingSphereMesh->vertexBuffer.GetAddressOf(), &m_boundingSphereMesh->stride, &m_boundingSphereMesh->offset);
		context->IASetIndexBuffer(m_boundingSphereMesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(m_boundingSphereMesh->indexCount, 0, 0);
	}

	void BoundComponent::SetScale(const Vector3& scale)
	{
		m_boundingBox.Extents = m_boundingBox.Extents * scale;
		m_boundingSphere.Radius *= std::max({ scale.x, scale.y, scale.z });
	}

	void BoundComponent::setBoundingBox(const std::vector<DE::MeshData>& meshes)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		// 모델은 여러 개의 Mesh들로 이루어진 경우가 많으니 다른 Mesh들의 Bounding Box의 크기도 비교해
		// 최종적으로 가장 큰 Bounding Box를 사용
		m_boundingBox = getBoundingBox(meshes[0].vertices);
		for (size_t i = 1; i < meshes.size(); ++i) {
			const DirectX::BoundingBox bb = getBoundingBox(meshes[i].vertices);
			extendBoundingBox(bb, m_boundingBox);
		}
		// Bounding Box 크기에 맞는 Wire Box를 생성해 시각적으로 표현
		//MeshData meshData = GeometryGenerator::MakeWireBox(m_boundingBox.Center, Vector3(m_boundingBox.Extents) + Vector3(1e-3f));
		MeshData meshData = GeometryGenerator::MakeWireBox(m_boundingBox.Center, m_boundingBox.Extents);
		m_boundingBoxMesh = std::make_shared<Mesh>();
		D3D11Utils::CreateVertexBuffer(device, meshData.vertices, m_boundingBoxMesh->vertexBuffer);
		m_boundingBoxMesh->vertexCount = UINT(meshData.vertices.size());
		m_boundingBoxMesh->indexCount = UINT(meshData.indices.size());
		m_boundingBoxMesh->stride = UINT(sizeof(Vertex));
		D3D11Utils::CreateIndexBuffer(device, meshData.indices, m_boundingBoxMesh->indexBuffer);

		// TODO: 임시, Model을 렌더링하고 BoundingComponent를 렌더링하면 굳이 여기서 설정해줄 필요가 없어보임
		Actor* owner = static_cast<Actor*>(this->GetOwner());
		ModelComponent* model = owner->GetComponent<ModelComponent>();
		if (model) {
			m_boundingBoxMesh->meshConstGPU = model->GetConsts();
			m_boundingBoxMesh->basicMaterialConstGPU = model->GetBasicMaterial();
			m_boundingBoxMesh->materialConstGPU = model->GetMaterial();
		}
	}

	void BoundComponent::setBoundingSphere(const std::vector<DE::MeshData>& meshes)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();

		// Model 크기에 맞는 Bounding Sphere 설정
		float maxRadius = 0.5f;
		// Sphere의 최대 Radius 찾기
		for (const MeshData& mesh : meshes) {
			for (const Vertex& v : mesh.vertices) {
				// Center는 (0.0, 0.0, 0.0)
				maxRadius = std::max(maxRadius,
					(Vector3(m_boundingSphere.Center) - v.position).Length());
			}
		}
		//maxRadius += 1e-2f; // 살짝 크게 설정
		m_boundingSphere = DirectX::BoundingSphere(m_boundingSphere.Center, maxRadius);
		// Bounding Sphere에 맞는 Wire Sphere Mesh 생성
		m_boundingSphereMesh = std::make_shared<Mesh>();
		MeshData meshData = GeometryGenerator::MakeWireSphere(m_boundingSphere.Center, m_boundingSphere.Radius);
		D3D11Utils::CreateVertexBuffer(device, meshData.vertices, m_boundingSphereMesh->vertexBuffer);
		m_boundingSphereMesh->vertexCount = UINT(meshData.vertices.size());
		m_boundingSphereMesh->indexCount = UINT(meshData.indices.size());
		m_boundingSphereMesh->stride = UINT(sizeof(Vertex));
		D3D11Utils::CreateIndexBuffer(device, meshData.indices, m_boundingSphereMesh->indexBuffer);

		// TODO: 임시, Model을 렌더링하고 BoundingComponent를 렌더링하면 굳이 여기서 설정해줄 필요가 없어보임
		Actor* owner = static_cast<Actor*>(this->GetOwner());
		ModelComponent* model = owner->GetComponent<ModelComponent>();
		if (model) {
			m_boundingSphereMesh->meshConstGPU = model->GetConsts();
			m_boundingSphereMesh->basicMaterialConstGPU = model->GetBasicMaterial();
			m_boundingSphereMesh->materialConstGPU = model->GetMaterial();
		}
	}

	DirectX::BoundingBox BoundComponent::getBoundingBox(const std::vector<Vertex>& vertices)
	{
		if (vertices.size() == 0)
			return DirectX::BoundingBox();

		// Model의 Vertex 중 가장 작은 위치와 큰 위치를 찾기
		Vector3 minCorner = vertices[0].position;
		Vector3 maxCorner = vertices[0].position;
		
		for (size_t i = 1; i < vertices.size(); ++i) {
			minCorner = Vector3::Min(minCorner, vertices[i].position);
			maxCorner = Vector3::Max(maxCorner, vertices[i].position);
		}

		// 모델의 중심 좌표를 구하고 얼마나 큰 박스여야지 모델을 다 감쌀수 있는지 extents를 계산
		Vector3 center = (minCorner + maxCorner) * 0.5f;
		Vector3 extents = maxCorner - center;

		return DirectX::BoundingBox(center, extents);
	}

	void BoundComponent::extendBoundingBox(const DirectX::BoundingBox& inBox, DirectX::BoundingBox& outBox)
	{
		Vector3 minCorner = Vector3(inBox.Center) - Vector3(inBox.Extents);
		Vector3 maxCorner = Vector3(inBox.Center) + Vector3(inBox.Extents);

		minCorner = Vector3::Min(minCorner,
			Vector3(outBox.Center) - Vector3(outBox.Extents));
		maxCorner = Vector3::Max(maxCorner,
			Vector3(outBox.Center) + Vector3(outBox.Extents));

		outBox.Center = (minCorner + maxCorner) * 0.5f;
		outBox.Extents = maxCorner - outBox.Center;
	}
}