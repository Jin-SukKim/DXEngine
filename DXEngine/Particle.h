#pragma once
#include "ParticleProperties.h"

namespace DE {
	struct Particle {
		Vector3 position = Vector3(0.f, 0.f, 0.f);
		Vector3 velocity = Vector3(0.f, 0.f, 0.f);
		Vector4 color = Vector4(0.f);
		float life = -1.f;
		float lifeMax = 0.f;
		float size = 1.f;
		Vector3 rotation = Vector3(0.f);
		Vector3 rotSpeed = Vector3(0.f);
		UINT ownerID = 0;  // ★ 추가
	};

    struct EmitterID {
        UINT readParticleOffset;
        UINT writeParticleOffset;
        UINT emitterID;
        UINT spawnPosOffset;
        UINT systemSlot;
        UINT padding[3];  // 32바이트 정렬
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
        UINT active = 0;  // ★ 추가
        float padding10;    
    };

    struct OrbitConsts {
        Vector3 center;
        float rotationRate;
        Vector3 axis;
        float initialOffset;
        UINT active = 0;  // ★ 추가
        Vector3 paddingOrbit;
    };

    struct RenderConsts {
        int textureIdx;
        UINT frameCount;
        Vector2 frameTiles;
        UINT indexCount;
        UINT textureMode;
        int singleTextureIdx;
        UINT useSorting; // 추가
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