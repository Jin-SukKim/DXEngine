#pragma once

namespace DE {
	enum class InputAxis {
		XAxis = 0,
		YAxis,
		ZAxis,
		WAxis,
		MaxAxis // InputAxis 개수
	};

	enum class InputButton {
		None,
		LButton = VK_LBUTTON,
		RButton = VK_RBUTTON,
		PgUp = VK_PRIOR,
		PgDown = VK_NEXT,
		End = VK_END,
		Home = VK_HOME,
		Left = VK_LEFT,
		Up = VK_UP,
		Right = VK_RIGHT,
		Down = VK_DOWN,
		A = 0x41,
		D = 'D',
		E = 'E',
		F = 'F',
		Q = 'Q',
		Z = 'Z',
		S = 0x53,
		W = 0x57,
		F1 = VK_F1,
		F2 = VK_F2,
		F3 = VK_F3,
		F4 = VK_F4,
		F5 = VK_F5,
		MaxButtons // InputButton 개수
	};

	enum class InputState {
		None,
		Pressed,
		Pressing,
		Released
	};
}