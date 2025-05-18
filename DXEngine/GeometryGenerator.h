#pragma once
#include "MeshData.h"

namespace DE{

class GeometryGenerator
{
public:
	static MeshData MakeTriangle(const float scale = 1.f, const Vector2 texScale = Vector2(1.f));
    static MeshData MakeSquare(const float scale = 1.0f,
        const Vector2 texScale = Vector2(1.0f));
    static MeshData MakeSquareGrid(const int numSlices, const int numStacks,
        const float scale = 1.0f,
        const Vector2 texScale = Vector2(1.0f));
    static MeshData MakeBox(const float scale = 1.0f);
    static MeshData MakeCylinder(const float bottomRadius,
        const float topRadius, float height,
        int numSlices);
    static MeshData MakeSphere(const float radius, const int numSlices,
        const int numStacks,
        const Vector2 texScale = Vector2(1.0f));
    static MeshData MakeTetrahedron();
    static MeshData MakeIcosahedron();
    // Subdivision Method�� Ȱ���ؼ� �� ���ػ� Mesh�� ���� ����
    static MeshData SubdivideToSphere(const float radius, MeshData meshData);
    
    // D3D11_PRIMITIVE_TOPOLOGY_LINELIST�� ������
    static MeshData MakeWireBox(const Vector3 center, const Vector3 extents);
    // D3D11_PRIMITIVE_TOPOLOGY_LINELIST�� ������ (LINESTRIP�� ����Ҽ��� ���� �׽�Ʈ �ʿ�)
    static MeshData MakeWireSphere(const Vector3 center, const float radius);
    // �⺻���� Ǯ�� 1���� Mesh
    static MeshData MakeGrass();
};

}
