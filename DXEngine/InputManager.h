#pragma once
#include "InputAction.h"
#include "InputTypes.h"

namespace DE {
	/*
		TODO: ���� frame�� ���ŵǴ� UpdateInput�� ����Ǵ� ���� Key�� �ԷµǾ�� �ν��� ���ٵ�,
		�Է�Ű�� ���� queue�� �÷��ΰ� UpdateInput()�� ����� �� �ݿ��ϸ� Ű ���� ������ �پ�� �� ����.
		�׳� ������ �� ������ �õ��غ��� �ʾ���

		�Է��� ���� queue�� �÷��δ� ���� ����غ��� (���� ��� �޾ƾ� ���� queue�� ����� �� ������?)
		�Ƹ� Multi-Threading�� ����ؾ� �ҵ�
	*/
	class InputManager
	{
	public:
		InputManager() {}
		~InputManager() {}

		// ���콺 Ŀ�� ��ǥ�� �ް� ndcClamp�� true�� Ŀ���� ȭ�� ������ ������ clamp
		void Update(float mouseX, float mouseY, bool ndcClamp = true);

		// Input�� �Լ��� ����
		template<typename T> 
		void BindInputAxis(InputAxis axis, InputAxisAction action, T* object, void(T::* func)(float));

		template<typename T>
		void BindInputAction(InputAction action, InputState state, T* object, void(T::* func)());

		template<typename T>
		void BindMouseMove(T* object, void(T::* func)(float, float));
	private:
		std::array<InputAxisAction, static_cast<size_t>(InputAxis::MaxAxis)> m_axisMap;
		std::array<InputAction, static_cast<size_t>(InputButton::MaxButtons)> m_buttonMap;
		std::function<void(float, float)> m_mouseMove = nullptr;
		float m_mouseNdcX = 0.f;
		float m_mouseNdcY = 0.f;
	};

	template<typename T>
	inline void InputManager::BindInputAxis(InputAxis axis, InputAxisAction action, T* object, void(T::* func)(float))
	{
		action.BindInputAxis(object, func);
		m_axisMap[static_cast<size_t>(axis)] = action;
	}

	template<typename T>
	inline void InputManager::BindInputAction(InputAction action, InputState state, T* object, void(T::* func)())
	{
		action.BindAction(object, func);
		m_buttonMap[static_cast<size_t>(action.GetButton())] = action;
	}

	template<typename T>
	inline void InputManager::BindMouseMove(T* object, void(T::* func)(float, float))
	{
		m_mouseMove = std::bind(func, object, std::placeholders::_1, std::placeholders::_2);
	}
}