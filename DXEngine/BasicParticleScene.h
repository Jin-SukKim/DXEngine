#pragma once
#include "Scene.h"

namespace DE {
	class SquareActor;
	class ParticleSpawner;
	class SpawnEffectsActor;
	class SnowActor;
	class BillboardEffectsActor;
	class ForceEffectsActor;
	class MaterialEffectsActor;
	class MeshEffectsActor;
	class OrbitEffectsActor;
	class SubEmitterEffectsActor;
	class VisualEffectsActor;
	class VortexEffectsActor;
	class MagicEffect;
	class PortalEffect;
	class BreathEffect;
	class HolySwordEffect;
	class IceEffect;
	class NecroEffect;
	class NightSkyEffect;
	class PhoenixEffect;
	class StarEffect;
	class ThunderEffect;
	class TsunamiEffect;
	class BasicParticleScene : public Scene
	{
	public:
		BasicParticleScene();
		~BasicParticleScene();

		void Initialize() override;
		void Update(const float& deltaTime) override;

		void ClickEvent();
	private:
		SquareActor* ground;
		InputAction m_click;

		ParticleSpawner* m_spanwer;

		SpawnEffectsActor* m_spawnModule;
		SnowActor* m_snow;
		BillboardEffectsActor* m_billboardModule;
		ForceEffectsActor* m_forceModule;
		MaterialEffectsActor* m_materialModule;
		MeshEffectsActor* m_meshModule;
		OrbitEffectsActor* m_orbitModule;
		SubEmitterEffectsActor* m_subEmitSystem;
		VisualEffectsActor* m_visualModule;
		VortexEffectsActor* m_vortexModule;
		MagicEffect* m_magic;
		PortalEffect* m_portal;
		BreathEffect* m_breathEffect;
		HolySwordEffect* m_holySwordEffect;
		IceEffect* m_iceEffect;
		NecroEffect* m_necroEffect;
		NightSkyEffect* m_nightSky;
		PhoenixEffect* m_phoenixEffect;
		StarEffect* m_starEffect;
		ThunderEffect* m_thunderEffect;
		TsunamiEffect* m_tsunamiEffect;
	};


}

