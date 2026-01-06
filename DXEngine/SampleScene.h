#pragma once
#include "Scene.h"

namespace DE {
	class SquareActor;
	class Outliner;
	class DetailGui;
	class PointLight;

	class SampleScene : public Scene
	{
	public:
		SampleScene();
		~SampleScene();

		void Initialize() override;
		void Update(const float& deltaTime) override;
		void UpdateGUI() override;
		void Render() override;

	private:
		SquareActor* m_ground;
		Outliner* m_outliner;
		DetailGui* m_detailGui;
		PointLight* m_pointLight;

	};


}

