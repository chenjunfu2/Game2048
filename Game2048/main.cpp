#include "Game2048.hpp"

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
			fprintf(stderr, "Fatal error:setlocale failed.\n\nPress any key to exit...\n");
			Console_Input::WaitAnyKey();//此功能仅在Windows下为任意键
			exit(-1);
		}
	
		if (!EnableVirtualTerminalProcessing())
		{
			fprintf(stderr, "Fatal error:\nConsole virtual terminal initialization failed.\n\nPress any key to exit...\n");
			Console_Input::WaitAnyKey();//此功能仅在Windows下为任意键
			exit(-1);
		}
	}
#elif defined(__linux__)
	void InitConsole(void) noexcept
	{
		return;//Linux默认UTF-8，默认虚拟终端序列支持，无须额外设置
	}
#endif

int main(void)
{
	//初始化控制台
	InitConsole();

	//游戏对象
	Game2048 game{};

	//初始化
	game.Init();

	//调试
#ifdef _DEBUG
	game.Debug();
#endif

	//游戏循环
	while (game.Loop())
	{
		continue;
	}

	return 0;
}
