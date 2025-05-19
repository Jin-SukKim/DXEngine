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

			// XOR�� �� �� �ϳ��� true�� pass
			if (isUp ^ isDown)
				return isDown ? -1.f : 1.f;

			// �� �� false�̰ų� true�� ���
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
			// ��ư�� ���ȴ��� Ȯ��
			return ::GetAsyncKeyState(static_cast<int>(button)) & 0x8000;
		}

		void UpdateInput() {
			prevState = GetButtonInput();
		}

		// TODO: ���߿� ���ϴ� ���� ���� ���� �ٸ� ����� ������ �� �ְ� ����
		// ���ϴ� ��ư state�� ���� bool return
		bool IsState() {
			const bool currentState = GetButtonInput();
			switch (state) {
			case InputState::None:
				break;
			case InputState::Pressed:
				return !prevState && currentState; // ������ ������ ���� && ���� ����
			case InputState::Pressing:
				return prevState && currentState; // �������� ��� ���� ����
			case InputState::Released:
				return prevState && !currentState; // ������ ���� ���� && ���� �ȴ���
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
		InputState state = InputState::None; // Event�� � ���¿��� �������
		bool prevState = false; // ������ ��ư�� ���ȴ��� Ȯ��
	};
}