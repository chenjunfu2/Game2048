#pragma once

#include "Console_Input_Windows.hpp"

struct Keys
{
	constexpr static const Console_Input::Key W = { 'w', Console_Input::Code_NL };
	constexpr static const Console_Input::Key A = { 'a', Console_Input::Code_NL };
	constexpr static const Console_Input::Key S = { 's', Console_Input::Code_NL };
	constexpr static const Console_Input::Key D = { 'd', Console_Input::Code_NL };

	constexpr static const Console_Input::Key SHIFT_W = { 'W', Console_Input::Code_NL };
	constexpr static const Console_Input::Key SHIFT_A = { 'A', Console_Input::Code_NL };
	constexpr static const Console_Input::Key SHIFT_S = { 'S', Console_Input::Code_NL };
	constexpr static const Console_Input::Key SHIFT_D = { 'D', Console_Input::Code_NL };

	constexpr static const Console_Input::Key UP_ARROW = { 72, Console_Input::Code_E0 };
	constexpr static const Console_Input::Key LEFT_ARROW = { 75, Console_Input::Code_E0 };
	constexpr static const Console_Input::Key DOWN_ARROW = { 80, Console_Input::Code_E0 };
	constexpr static const Console_Input::Key RIGHT_ARROW = { 77, Console_Input::Code_E0 };

	constexpr static const Console_Input::Key Y = { 'y', Console_Input::Code_NL };
	constexpr static const Console_Input::Key N = { 'n', Console_Input::Code_NL };
	constexpr static const Console_Input::Key Q = { 'q', Console_Input::Code_NL };
	constexpr static const Console_Input::Key R = { 'r', Console_Input::Code_NL };

	constexpr static const Console_Input::Key SHIFT_Y = { 'Y', Console_Input::Code_NL };
	constexpr static const Console_Input::Key SHIFT_N = { 'N', Console_Input::Code_NL };
	constexpr static const Console_Input::Key SHIFT_Q = { 'Q', Console_Input::Code_NL };
	constexpr static const Console_Input::Key SHIFT_R = { 'R', Console_Input::Code_NL };
};
