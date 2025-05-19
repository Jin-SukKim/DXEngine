#pragma once
#include "InputAction.h"
#include "InputTypes.h"

namespace DE {
	/*
		TODO: 현재 frame이 갱신되는 UpdateInput이 실행되는 순간 Key가 입력되어야 인식을 할텐데,
		입력키를 대기용 queue에 올려두고 UpdateInput()이 실행될 때 반영하면 키 씹힘 현상이 줄어들 것 같다.
		그냥 가정일 뿐 실제로 시도해보진 않았음

		입력을 대기용 queue에 올려두는 것을 고민해보기 (언제 어떻게 받아야 대기용 queue를 사용할 수 있을까?)
		아마 Multi-Threading을 사용해야 할듯
	*/
	class InputManager
	{
	public:
		InputManager() {}
		~InputManager() {}

		void Update();

		// Input와 함수를 연결
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