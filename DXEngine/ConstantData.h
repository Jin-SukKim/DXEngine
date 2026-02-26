#pragma once
#include "pch.h"

#define MAX_LIGHTS 3
#define MAX_SPOT 2
#define MAX_POINT 1
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_OFF 0x08
#define LIGHT_SHADOW 0x10

namespace DE {
	__declspec(align(256)) struct MeshConstants {
		Matrix world;
		Matrix worldIT; // World Inverse Transpose (Normal)
	};
	
	__declspec(align(256)) struct BasicMaterialConstants {
		Vector3 ambient = Vector3(1.0f);
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
		Vector3 albedoFactor = Vector3(0.3f);  
		float roughnessFactor = 0.5f; 
		float metallicFactor = 0.5f;
		Vector3 emissionFactor = Vector3(0.0f);

		int useAlbedoMap = 0;
		int useNormalMap = 0;
		int useAOMap = 0; // (Ambient LIghting) 
		int invertNormalMapY = 0;
		int useMetallicMap = 0;
		int useRoughnessMap = 0;
		int useEmissiveMap = 0;
		int useHeightMap = 0;
		float heightScale = 1.f;
		float dummy[3];
	};

	struct Light {
		Vector3 radiance = Vector3(1.f); //   (Strength) -  R, G, B 
		float fallOffStart = 0.f; // point/spot light only
		Vector3 direction = Vector3(0.0f, 0.0f, 1.0f); // (spot light only)
		float fallOffEnd = 10.0f; // point/spot light only
		Vector3 position = Vector3(0.0f, 0.0f, -2.0f); // point/spot light only
		float spotPower = 6.f; // spot light only

		// Light type bitmasking
		// ex) LIGHT_SPOT | LIGHT_SHADOW
		uint32_t type = LIGHT_OFF;
		float radius = 0.02f; //  (Volume Light )
		float nearPlane;
		float frustumWidth;
		
		// TODO: Shader (Light)
		float haloRadius = 0.0f;
		float haloStrength = 0.f;
		Vector2 dummy;

		Matrix viewProj[6]; 
		Matrix invProj;
	};

	__declspec(align(256)) struct GlobalConstants {
		Matrix view;
		Matrix proj;
		Matrix viewProj;
		Matrix invProj; // Porjection -> View 
		Matrix invViewProj; // Proj -> World

		Vector3 eyeWorld; // Camera
		float strengthIBL = 1.f;

		Light lights[MAX_LIGHTS];
	};
}