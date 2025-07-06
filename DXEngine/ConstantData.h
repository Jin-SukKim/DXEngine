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
		int useHeightMap = 0; 
		float heightScale = 1.f;
		float dummy[2];
	};
	
	__declspec(align(256)) struct BasicMaterialConstants {
		Vector3 ambient = Vector3(0.0f);
		float shininess = 0.01f;
		Vector3 diffuse = Vector3(1.0f);
		float dummy1;
		Vector3 specular = Vector3(1.0f);
		float dummy2;
		Vector3 fresnelR0 = Vector3(0.05f, 0.05f, 0.05f);
		float dummy3;
		int hashID = 0;
	};

	__declspec(align(256)) struct MaterialConstants {
		Vector3 albedoFactor = Vector3(1.f); // 기본 색이라 생각할 수 있음
		float roughnessFactor = 0.0f; // 물체 표면의 거칠기
		float metallicFactor = 1.0f; // 금속에 가까운지 비금속에 가까운지 결정하는 값
		Vector3 emissionFactor = Vector3(0.f);

		// 여러 옵션들에 uint32를 flag로 하나만 사용할 수도 있음
		int useAlbedoMap = 0;
		int useNormalMap = 0;
		int useAOMap = 0; // 간접광(Ambient LIghting)으로 사용
		int invertNormalMapY = 0;
		int useMetallicMap = 0;
		int useRoughnessMap = 0;
		int useEmissiveMap = 0;
		float dummy = 0.f;
	};

	struct Light {
		Vector3 radiance = Vector3(1.f); // 빛의 세기 (Strength) - 빛의 R, G, B 강도
		float fallOffStart = 0.f; // 빛의 강도가 약해지기 시작하는 거리 (point/spot light only)
		Vector3 direction = Vector3(0.0f, 0.0f, 1.0f); // 빛의 방향 (spot light only)
		float fallOffEnd = 10.0f; // 빛이 더이상 닿지 않아 어두워지는 거리 (point/spot light only)
		Vector3 position = Vector3(0.0f, 0.0f, -2.0f); // 빛의 위치 (point/spot light only)
		float spotPower = 6.f; // 빛이 한 지점에 모이는 강도 (spot light only)

		// Light type bitmasking
		// ex) LIGHT_SPOT | LIGHT_SHADOW
		uint32_t type = LIGHT_OFF;
		float radius = 0.f; // 반지름 (Volume Light 용)
		float dummy[2] = { 0.f, 0.f };
	};

	__declspec(align(256)) struct GlobalConstants {
		Matrix view;
		Matrix proj;
		Matrix viewProj;

		Vector3 eyeWorld; // Camera 위치
		float dummy3 = 0.f; 

		Light lights[MAX_LIGHTS];
	};

	template <typename T_CONST>
	class ConstantBuffer {
	public:
		// GPU에 Constant Buffer 생성
		void Initialize(ComPtr<ID3D11Device>& device) {
			D3D11Utils::CreateConstantBuffer(device, m_cpu, m_gpu);
		}

		// CPU 데이터를 GPU로 복사
		void Upload(ComPtr<ID3D11DeviceContext>& context) {
			D3D11Utils::UpdateBuffer(context, m_cpu, m_gpu);
		}

		T_CONST& GetCpu() { return m_cpu; }
		const auto Get() { return m_gpu.Get(); }
		const auto GetAddressOf() const { return m_gpu.GetAddressOf(); }
	private:
		T_CONST m_cpu;
		ComPtr<ID3D11Buffer> m_gpu;
	};
}