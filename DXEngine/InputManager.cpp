#include "pch.h"
#include "InputManager.h"

namespace DE {
	void InputManager::Update() {
		// �Ź� Ű������ �Է��� �޾� Update
		for (InputAxisAction& action : m_axisMap)
			action.Execute();

		for (InputAction& action : m_buttonMap)
			action.Execute();
		
	}
}