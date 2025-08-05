#pragma once
#include "Gui.h"

namespace DE {
	class DetailGui : public Gui
	{
	public:
        void Update() override;
	private:
		void UpdateActor();
		void UpdateCamera();
		void UpdateLight();
	};
}
