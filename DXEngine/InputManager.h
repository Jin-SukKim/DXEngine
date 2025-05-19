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

		void Update();

		// Input�� �Լ��� ����
		template<typename T> 
		void BindInputAxis(InputAxis axis, InputAxisAction action, T* object, void(T::* func)(float));

		template<typename T>
		void BindInputAction(InputAction action, InputState state, T* object, void(T::* func)());
	private:
		std::array<InputAxisAction, static_cast<size_t>(InputAxis::MaxAxis)> m_axisMap;
		std::array<InputAction, static_cast<size_t>(InputButton::MaxButtons)> m_buttonMap;
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
}