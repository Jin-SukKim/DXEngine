#pragma once
#include "EffectActor.h"

namespace DE {
	class ModelComponent;
	class BoxMeshEffect : public EffectActor
	{
		using Super = EffectActor;
	public:
		BoxMeshEffect(const std::wstring& name);
		virtual ~BoxMeshEffect() override;

		bool NeedsExternalPreset() const override { return false; }

		void Initialize() override;
		void Update(const float& deltaTime) override;
	private:
		ModelComponent* m_sample;
		float m_orbitAngle = 0.f;
	};
}