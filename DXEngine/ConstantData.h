#pragma once
#include "pch.h"

// "Common.hlsli"�� �����ؾ� ��
#define MAX_LIGHTS 3 // ���� ������ ������ �����Ǿ� �ְ� ������� ������ OFF�� ���� (Particle System�� ���)
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

namespace DE {
	//_declspec(align(256))�� C++���� �޸� ������ �����ϴ� ���þ��, 
	// �ش� ����ü�� ������ �޸� �ּҰ� 256����Ʈ ��迡 ���������� ������ �����ϴ� ����
	// ������ �����ϸ�, �ش� ����ü�� 256����Ʈ ������ �޸𸮰� ���ĵǾ� �Ҵ��
	__declspec(align(256)) struct MeshConstants {
		Matrix world;
		Matrix worldIT; // World Inverse Transpose (Normal ��ȯ�� ���)
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
		Vector3 radiance = Vector3(1.f); // ���� ���� (Strength)
		float fallOffStart = 0.f; // ���� ������ �������� �����ϴ� �Ÿ� (point/spot light only)
		Vector3 direction = Vector3(0.0f, 0.0f, 1.0f); // ���� ���� (spot light only)
		float fallOffEnd = 10.0f; // ���� ���̻� ���� �ʾ� ��ο����� �Ÿ� (point/spot light only)
		Vector3 position = Vector3(0.0f, 0.0f, -2.0f); // ���� ��ġ (point/spot light only)
		float spotPower = 0.f; // ���� �� ������ ���̴� ���� (spot light only)

		// Light type bitmasking
		// ex) LIGHT_SPOT | LIGHT_SHADOW
		uint32_t type = LIGHT_OFF;
		float radius = 0.f; // ������ (Volume Light ��)
		float dummy[2];
	};

	__declspec(align(256)) struct GlobalConstants {
		Matrix view;
		Matrix proj;

		Vector3 eyeWorld; // Camera ��ġ
		float dummy; 

		Light lights[MAX_LIGHTS];
	};
}