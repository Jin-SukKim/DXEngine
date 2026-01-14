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

    struct ParticleConsts {
        float dt;
        float time;
        UINT spawnCount;
        UINT maxParticles;

        Vector3 localPos;
        float padding0;
        Vector3 spawnVolume;
        float spawnInnerRatio;
        Vector2 lifeRange;
        int spawnShape = 0; // 0: Box, 1: Sphere
        float padding1;

        Vector3 velocity;
        float padding2;
        Vector2 speedRange;
        Vector2 padding3;

        Vector3 randomDir;
        float drag;

        Vector3 gravity;

        float vortexStrength;
        Vector3 vortexCenter;
        float padding4;
        Vector3 vortexAxis;
        float vortexFalloff;
        Vector2 vortexPull;

        Vector2 sizeRange;

        Vector4 startColor;
        Vector4 endColor;

        Vector3 minRotation;
        float padding6;
        Vector3 maxRotation;
        float padding7;
        Vector3 minRotSpeed;
        float padding8;
        Vector3 maxRotSpeed;
        float padding9;
    };
}