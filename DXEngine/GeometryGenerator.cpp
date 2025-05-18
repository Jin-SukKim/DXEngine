#include "pch.h"
#include "GeometryGenerator.h"

namespace DE {
    MeshData GeometryGenerator::MakeTriangle(const float scale, const Vector2 texScale)
    {
        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<Vector2> tex;

        positions.emplace_back(Vector3(-1.f, -1.f, 0.f));
        positions.emplace_back(Vector3(0.f, 1.f, 0.f));
        positions.emplace_back(Vector3(1.f, -1.f, 0.f));
        normals.emplace_back(Vector3(0.f, 0.f, -1.f));
        normals.emplace_back(Vector3(0.f, 0.f, -1.f));
        normals.emplace_back(Vector3(0.f, 0.f, -1.f));
        tex.emplace_back(Vector2(0.f, 1.f));
        tex.emplace_back(Vector2(0.5f, 0.f));
        tex.emplace_back(Vector2(1.f, 1.f));
        
        MeshData meshData;

        for (size_t i = 0; i < positions.size(); ++i) {
            Vertex v;
            v.position = positions[i];
            v.normalModel = normals[i];
            v.texcoord = tex[i];

            meshData.vertices.emplace_back(v);
        }

        meshData.indices = {
            0, 1, 2
        };

        return meshData;
    }
}