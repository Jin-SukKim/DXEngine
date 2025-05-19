#pragma once
#include "pch.h"
#include "InputTypes.h"

namespace DE {
	struct InputAxisAction {
		InputAxisAction() = default;
		InputAxisAction(InputButton upBtn, InputButton downBtn) : upButton(upBtn), downButton(downBtn) {}

		template<typename T>
		void BindInputAxis(T* object, void(T::* func)(float)) {
			bindFunc = std::bind(func, object, std::placeholders::_1);
		}

		float GetAxisInput() {
			bool isUp = ::GetAsyncKeyState(static_cast<int>(upButton));
			bool isDown = ::GetAsyncKeyState(static_cast<int>(downButton));

			// XOR로 둘 중 하나만 true면 pass
			if (isUp ^ isDown)
				return isDown ? -1.f : 1.f;

			// 둘 다 false이거나 true인 경우
			return 0.f;
		}

		void Execute() {
			if (bindFunc)
				bindFunc(GetAxisInput());
		}

		InputButton upButton = InputButton::None;
		InputButton downButton = InputButton::None;
		std::function<void(float)> bindFunc = nullptr;
	};

	struct InputAction {
		InputAction() = default;
		InputAction(InputButton btn) : button(btn) {}

		void SetState(InputState iState) { state = iState; }

		template<typename T>
		void BindAction(T* object, void(T::* func)(), InputState state) {
			bindFunc = std::bind(func, object);

			state = state;
		}

		const InputButton GetButton() const { return button; }

		bool GetButtonInput() const {
			// 버튼이 눌렸는지 확인
			return ::GetAsyncKeyState(static_cast<int>(button)) & 0x8000;
		}

		void UpdateInput() {
			prevState = GetButtonInput();
		}

		// TODO: 나중에 원하는 상태 별로 각각 다른 기능을 수행할 수 있게 변경
		// 원하는 버튼 state에 따라 bool return
		bool IsState() {
			const bool currentState = GetButtonInput();
			switch (state) {
			case InputState::None:
				break;
			case InputState::Pressed:
				return !prevState && currentState; // 이전에 눌린적 없음 && 현재 눌림
			case InputState::Pressing:
				return prevState && currentState; // 이전부터 계속 눌린 상태
			case InputState::Released:
				return prevState && !currentState; // 이전엔 눌린 상태 && 현재 안눌림
			}
			return false;
		}

		void Execute() {
			if (bindFunc && IsState())
				bindFunc();
			UpdateInput();
		}

		InputButton button = InputButton::None;
		std::function<void()> bindFunc = nullptr;
		InputState state = InputState::None; // Event가 어떤 상태에서 실행될지
		bool prevState = false; // 이전에 버튼이 눌렸는지 확인
	};
}