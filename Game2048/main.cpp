#include "Game2048.hpp"

int main(void)
{
	Console_Input ci{};
	Console_Output co{};

	//游戏对象
	Game2048 game(ci, co);

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
