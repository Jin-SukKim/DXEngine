#pragma once
#include "EffectActor.h"
namespace DE {
	class MeshEffectsActor : public EffectActor
	{
		using Super = EffectActor;
	public:
		MeshEffectsActor(const std::wstring& name);
		virtual ~MeshEffectsActor() override;

		bool NeedsExternalPreset() const override { return false; }
	private:
		ParticleSystem* m_sphere = nullptr;
		ParticleSystem* m_model = nullptr;
	};
}