#pragma once
#include "pch.h"

// "Common.hlsli"와 동일해야 함
#define MAX_LIGHTS 3 // 보통 조명의 개수는 고정되어 있고 사용하지 않으면 OFF로 설정 (Particle System과 비슷)
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

namespace DE {
	//_declspec(align(256))는 C++에서 메모리 정렬을 지정하는 지시어로, 
	// 해당 구조체나 변수의 메모리 주소가 256바이트 경계에 맞춰지도록 강제로 정렬하는 역할
	// 정렬을 적용하면, 해당 구조체는 256바이트 단위로 메모리가 정렬되어 할당됨
	__declspec(align(256)) struct MeshConstants {
		Matrix world;
		Matrix worldIT; // World Inverse Transpose (Normal 변환에 사용)
	};
	
	__declspec(align(256)) struct BasicMaterialConstants {
		Vector3 ambient = Vector3(0.1f);
		float shininess = 1.0f;
		Vector3 diffuse = Vector3(1.f);
		float dummy1;
		Vector3 specular = Vector3(1.f);
		float dummy2;
	};

	struct Light {
		Vector3 radiance = Vector3(1.f); // 빛의 세기 (Strength)
		float fallOffStart = 0.f; // 빛의 강도가 약해지기 시작하는 거리 (point/spot light only)
		Vector3 direction = Vector3(0.0f, 0.0f, 1.0f); // 빛의 방향 (spot light only)
		float fallOffEnd = 10.0f; // 빛이 더이상 닿지 않아 어두워지는 거리 (point/spot light only)
		Vector3 position = Vector3(0.0f, 0.0f, -2.0f); // 빛의 위치 (point/spot light only)
		float spotPower = 0.f; // 빛이 한 지점에 모이는 강도 (spot light only)

		// Light type bitmasking
		// ex) LIGHT_SPOT | LIGHT_SHADOW
		uint32_t type = LIGHT_OFF;
		float radius = 0.f; // 반지름 (Volume Light 용)
		float dummy[2];
	};

	__declspec(align(256)) struct GlobalConstants {
		Matrix view;
		Matrix proj;

		Vector3 eyeWorld; // Camera 위치
		float dummy; 

		Light lights[MAX_LIGHTS];
	};
}