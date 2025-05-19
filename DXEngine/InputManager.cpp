#include "pch.h"
#include "InputManager.h"

namespace DE {
	void InputManager::Update(float mouseX, float mouseY, bool ndcClamp) {
		// Ŀ���� ȭ�� ������ ������ ��� ���� ����
		// ���ӿ����� Ŭ������ ���� ���� �ֽ��ϴ�.
		if (m_mouseMove) {
			if (ndcClamp) {
				m_mouseNdcX = std::clamp(mouseX, -1.0f, 1.0f);
				m_mouseNdcY = std::clamp(mouseY, -1.0f, 1.0f);
			}
			m_mouseMove(m_mouseNdcX, m_mouseNdcY);
		}
		
		// �Ź� Ű������ �Է��� �޾� Update
		for (InputAxisAction& action : m_axisMap)
			action.Execute();

		for (InputAction& action : m_buttonMap)
			action.Execute();
		
	}
}