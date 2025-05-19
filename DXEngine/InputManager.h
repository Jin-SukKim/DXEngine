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

		// 마우스 커서 좌표를 받고 ndcClamp가 true면 커서가 화면 밖으로 나가면 clamp
		void Update(float mouseX, float mouseY, bool ndcClamp = true);

		// Input와 함수를 연결
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