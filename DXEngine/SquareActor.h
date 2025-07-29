#pragma once
#include "Actor.h"

namespace DE {
	class ModelComponent;

	class SquareActor : public Actor
	{
		using Super = Actor;
	public:
		SquareActor(const std::wstring& name);
		virtual ~SquareActor() override {}

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void Render() override;
	private:
		ModelComponent* m_sample;
	};
}