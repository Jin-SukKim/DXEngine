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
        UINT ownerID = UINT_MAX;
        UINT systemID = UINT_MAX;
	};

    struct EmitterID {
        UINT readParticleOffset;
        UINT writeParticleOffset;
        UINT emitterID;
        UINT spawnPosOffset;  // bakedOffset + customOffset 
        UINT systemID;
        UINT indexCount;
        UINT startIndexLocation;
        UINT baseVertexLocation;
    };

    struct ParticleFrameConsts {
        float dt;
        float time;
        UINT spawnCount;
        UINT maxParticles;
        float spawnRatio = 1.0f;       // Current frame spawn ratio (0.0 ~ 1.0)
        Vector3 padding2;
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

        // Overdraw Control - Size Scaling
        float sizeDistanceScale = 0.7f;      // Size multiplier at close range (e.g., 0.7 = 70%)
        float sizeDistanceMin = 2.0f;        // Distance where scaling starts (meters)
        float sizeDistanceMax = 10.0f;       // Distance where scaling ends (meters)
        UINT enableSizeScaling = 0;          // On/Off toggle (per-emitter)
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
        UINT useSorting;
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