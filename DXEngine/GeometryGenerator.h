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
    // Subdivision Method를 활용해서 더 고해상도 Mesh의 구로 변형
    static MeshData SubdivideToSphere(const float radius, MeshData meshData);
    
    // D3D11_PRIMITIVE_TOPOLOGY_LINELIST로 렌더링
    static MeshData MakeWireBox(const Vector3 center, const Vector3 extents);
    // D3D11_PRIMITIVE_TOPOLOGY_LINELIST로 렌더링 (LINESTRIP을 써야할수도 있음 테스트 필요)
    static MeshData MakeWireSphere(const Vector3 center, const float radius);
    // 기본적인 풀잎 1개의 Mesh
    static MeshData MakeGrass();

    // fbx, gltf 등 파일로부터 모델과 Texture 파일들 읽어오기
    static std::vector<MeshData> ReadFromFile(std::string basePath, std::string filename, bool revertNormals = false, bool calculateNormals = false);
    // 파일로부터 읽은 모델의 크기가 전부 제각각이기 때문에 정규화를 거쳐서 일정한 규격을 유지할 수 있도록 하기
    // 기본적으로 center = Vector3(0.f) 모델 좌표계의 중심점, longestLength = 1.f를 사용하는걸 추천
    static void Normalize(const Vector3 center, const float longestLength, std::vector<MeshData>& meshes);
};

}
