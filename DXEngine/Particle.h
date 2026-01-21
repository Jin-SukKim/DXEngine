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
        int spawnShape = 0; // 0: Box, 1: Sphere
        UINT vertexCount = 0;
        UINT indexCount = 0;
        Vector3 padding1;
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
        Vector2 padding10;    
    };

    struct RenderConsts {
        int textureIdx;
        UINT frameCount;
        Vector2 frameTiles;
        UINT numMeshes;
        Vector3 padding11;
    };

    struct ParticleConsts {
        SpawnConsts spawn;
        VisualConsts visual;
        ForceConsts force;
        RenderConsts render;
        VortexConsts vortex;
    };
}