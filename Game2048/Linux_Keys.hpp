#pragma once

#include "Console_Input_Linux.hpp"

struct Keys
{
	constexpr static const Console_Input::Key W = { 'w', false };
	constexpr static const Console_Input::Key A = { 'a', false };
	constexpr static const Console_Input::Key S = { 's', false };
	constexpr static const Console_Input::Key D = { 'd', false };

	constexpr static const Console_Input::Key SHIFT_W = { 'W', false };
	constexpr static const Console_Input::Key SHIFT_A = { 'A', false };
	constexpr static const Console_Input::Key SHIFT_S = { 'S', false };
	constexpr static const Console_Input::Key SHIFT_D = { 'D', false };

	constexpr static const Console_Input::Key UP_ARROW = { 'A', true };
	constexpr static const Console_Input::Key LEFT_ARROW = { 'D', true };
	constexpr static const Console_Input::Key DOWN_ARROW = { 'B', true };
	constexpr static const Console_Input::Key RIGHT_ARROW = { 'C', true };

	constexpr static const Console_Input::Key Y = { 'y', false };
	constexpr static const Console_Input::Key N = { 'n', false };
	constexpr static const Console_Input::Key Q = { 'q', false };
	constexpr static const Console_Input::Key R = { 'r', false };

	constexpr static const Console_Input::Key SHIFT_Y = { 'Y', false };
	constexpr static const Console_Input::Key SHIFT_N = { 'N', false };
	constexpr static const Console_Input::Key SHIFT_Q = { 'Q', false };
	constexpr static const Console_Input::Key SHIFT_R = { 'R', false };
};
