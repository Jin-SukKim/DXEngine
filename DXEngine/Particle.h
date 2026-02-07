#pragma once
#include "ParticleProperties.h"

namespace DE {
    // Forward declarations for batching
    enum class BlendMode : int;

    // ========== Batch Rendering Structures ==========

    // GPU-side: 배치 내 각 Emitter의 파티클 범위 정보
    struct BatchEmitterInfo {
        UINT globalEmitterID;    // -> consts[], emitterIDs[] 인덱싱
        UINT readParticleOffset; // 파티클 버퍼 내 시작 위치
        UINT particleCount;      // 해당 emitter의 파티클 수
        UINT batchVertexStart;   // 배치 내 누적 vertex/instance ID 시작점
    };

    // GPU-side: 배치별 상수
    struct BatchConsts {
        UINT batchEmitterCount;  // 이 배치에 포함된 emitter 수
        UINT batchInfoOffset;    // batchInfo 버퍼 내 시작 인덱스
        UINT batchPadding[2];
    };

    // CPU-side: 배치 그룹 키 (같은 키를 가진 emitter들은 하나의 draw call로 병합)
    struct BatchKey {
        bool isMesh;
        int blendMode;        // BlendMode enum 값
        int textureMode;      // 0=Material, 1=SingleTexture, 2=TextureArray
        int singleTextureIdx; // SingleTexture 모드일 때만 사용
        int modelIdx;         // Mesh일 때만 사용 (-1이면 Billboard)

        bool operator<(const BatchKey& o) const {
            if (isMesh != o.isMesh) return isMesh < o.isMesh;
            if (blendMode != o.blendMode) return blendMode < o.blendMode;
            if (textureMode != o.textureMode) return textureMode < o.textureMode;
            if (singleTextureIdx != o.singleTextureIdx) return singleTextureIdx < o.singleTextureIdx;
            return modelIdx < o.modelIdx;
        }

        bool operator==(const BatchKey& o) const {
            return isMesh == o.isMesh && blendMode == o.blendMode
                && textureMode == o.textureMode && singleTextureIdx == o.singleTextureIdx
                && modelIdx == o.modelIdx;
        }
    };

    // CPU-side: 배치 그룹 (같은 BatchKey를 공유하는 emitter 목록)
    struct BatchGroup {
        BatchKey key;
        std::vector<UINT> globalEmitterIDs;
    };
	struct Particle {
		Vector3 position = Vector3(0.f, 0.f, 0.f);
		Vector3 velocity = Vector3(0.f, 0.f, 0.f);
		Vector4 color = Vector4(0.f);
		float life = -1.f;
		float lifeMax = 0.f;
		float size = 1.f;
		Vector3 rotation = Vector3(0.f);
		Vector3 rotSpeed = Vector3(0.f);
        UINT ownerID = UINT_MAX;
	};

    struct EmitterID {
        UINT readParticleOffset;
        UINT writeParticleOffset;
        UINT emitterID;
        UINT spawnPosOffset;  // bakedOffset + customOffset ����
    };

    struct ParticleFrameConsts {
        float dt;
        float time;
        UINT spawnCount;
        UINT maxParticles;
    };

    struct SpawnConsts {
        Vector3 localPos;
        float padding;

        Vector3 spawnVolume;
        float spawnInnerRatio;
        
        Vector2 lifeRange;
        int spawnShape = 0; // 0: Box, 1: Sphere, 2 : Vertex, 3: Surface
        UINT bakedCount = 0;
        UINT simulationSpace = 0; // 0 : Local, 1 : world

        UINT spawnStartIndex;
        Vector2 padding1;
    };

    struct VisualConsts {
        Vector2 sizeRange;
        Vector2 padding2;

        Vector4 startColor;
        Vector4 endColor;

        Vector3 minRotation;
        float padding3;
        Vector3 maxRotation;
        float padding4;
        Vector3 minRotSpeed;
        float padding5;
        Vector3 maxRotSpeed;
        float padding6;
    };

    struct ForceConsts {
        Vector3 velocity;
        float padding7;
        Vector2 speedRange;
        Vector2 padding8;

        Vector3 randomDir;
        float drag;
        Vector3 gravity;
        float padding9;
    };

    struct VortexConsts {
        float vortexStrength;
        Vector3 vortexCenter; 

        Vector3 vortexAxis;
        float vortexFalloff;  

        Vector2 vortexPull;
        float padding10;    
        UINT active = 0;
    };

    struct OrbitConsts {
        Vector3 center;
        float rotationRate;
        Vector3 axis;
        float initialOffset;
        UINT active = 0;
        Vector3 orbitPadding;
    };

    struct RenderConsts {
        int textureIdx;
        UINT frameCount;
        Vector2 frameTiles;
        UINT indexCount;
        UINT textureMode;
        int singleTextureIdx;
        UINT useSorting; // �߰�
    };

    struct ParticleConsts {
        SpawnConsts spawn;
        VisualConsts visual;
        ForceConsts force;
        RenderConsts render;
        VortexConsts vortex;
        OrbitConsts orbit;
    };
}