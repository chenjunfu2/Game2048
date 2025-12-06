#pragma once

#include <stdio.h>
#include <stdint.h>

#if defined(_WIN32)
	//虚拟终端序列头文件
	#include <Windows.h>
	#include <locale.h>//setlocale

//启用虚拟终端序列
bool EnableVirtualTerminalProcessing(void) noexcept
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return false;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;//启用虚拟终端序列
	dwMode |= DISABLE_NEWLINE_AUTO_RETURN;//关闭自动换行

	return SetConsoleMode(hOut, dwMode);
}

void InitConsole(void) noexcept
{
	if (setlocale(LC_ALL, "en_US.UTF-8") == NULL)//设置代码页
	{
		fprintf(stderr, "Fatal error:setlocale failed.\n\n");
		system("pause");
		exit(-1);
	}

	if (!EnableVirtualTerminalProcessing())
	{
		fprintf(stderr, "Fatal error:\nConsole virtual terminal initialization failed.\n\n");
		system("pause");
		exit(-1);
	}
}
#elif defined(__linux__)
void InitConsole(void) noexcept
{
	return;//Linux默认UTF-8，默认虚拟终端序列支持，无须额外设置
}
#endif

class Console_Output
{
private:
	uint16_t u16BaseX;
	uint16_t u16BaseY;

	uint16_t u16CurX;
	uint16_t u16CurY;

public:
	Console_Output(uint16_t _u16BaseX = 1, uint16_t _u16BaseY = 1, uint16_t _u16CurX = 1, uint16_t _u16CurY = 1) :
		u16BaseX(_u16BaseX),
		u16BaseY(_u16BaseY),
		u16CurX(_u16CurX),
		u16CurY(_u16CurY)
	{
		InitConsole();
	}

	~Console_Output(void) = default;

	Console_Output(const Console_Output &) = default;
	Console_Output(Console_Output &&) = default;

	Console_Output &operator=(const Console_Output &) = default;
	Console_Output &operator=(Console_Output &&) = default;

	void SetBase(uint16_t _u16BaseX, uint16_t _u16BaseY)
	{
		u16BaseX = _u16BaseX;
		u16BaseY = _u16BaseY;
	}

	void SetCur(uint16_t _u16CurX, uint16_t _u16CurY)
	{
		u16CurX = _u16CurX;
		u16CurY = _u16CurY;
	}

	void GetBase(uint16_t &_u16BaseX, uint16_t &_u16BaseY)
	{
		_u16BaseX = u16BaseX;
		_u16BaseY = u16BaseY;
	}

	void GetCur(uint16_t &_u16CurX, uint16_t &_u16CurY)
	{
		_u16CurX = u16CurX;
		_u16CurY = u16CurY;
	}

	//功能
	void ShowCursor(void)
	{
		printf("\033[?25h");//显示光标
	}

	void HideCursor(void)
	{
		printf("\033[?25l");//隐藏光标
	}

	void ClearScreen(void)
	{
		printf("\033[2J");//清空屏幕
	}

	void ClearLine(void)
	{
		printf("\033[2K");//清空整行
	}

	void SetCursorCur(void)
	{
		printf("\033[%u;%uH", u16CurY, u16CurX);
	}

	void SetCursorPos(uint16_t u16PosX, uint16_t u16PosY)
	{
		printf("\033[%u;%uH", u16PosY, u16PosX);
	}

	void NextLine(uint16_t u16LineMove = 1)
	{
		u16CurY += u16LineMove;
		SetCursorCur();
	}

	void PrevLine(uint16_t u16LineMove = 1)
	{
		u16CurY -= u16LineMove;
		SetCursorCur();
	}

	void SetCursorBase(void)
	{
		u16CurX = u16BaseX;
		u16CurY = u16BaseY;
		SetCursorCur();
	}

};
