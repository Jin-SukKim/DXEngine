#include "pch.h"
#include "InputManager.h"

namespace DE {
	void InputManager::Update() {
		// 매번 키보드의 입력을 받아 Update
		for (InputAxisAction& action : m_axisMap)
			action.Execute();

		for (InputAction& action : m_buttonMap)
			action.Execute();
		
	}
}