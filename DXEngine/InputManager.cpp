#include "pch.h"
#include "InputManager.h"

namespace DE {
	void InputManager::Update(float mouseX, float mouseY, bool ndcClamp) {
		// 커서가 화면 밖으로 나갔을 경우 범위 조절
		// 게임에서는 클램프를 안할 수도 있습니다.
		if (m_mouseMove) {
			if (ndcClamp) {
				m_mouseNdcX = std::clamp(mouseX, -1.0f, 1.0f);
				m_mouseNdcY = std::clamp(mouseY, -1.0f, 1.0f);
			}
			m_mouseMove(m_mouseNdcX, m_mouseNdcY);
		}
		
		// 매번 키보드의 입력을 받아 Update
		for (InputAxisAction& action : m_axisMap)
			action.Execute();

		for (InputAction& action : m_buttonMap)
			action.Execute();
		
	}
}