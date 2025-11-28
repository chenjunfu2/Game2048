#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <random>
#include <span>
#include <algorithm>

#include "Console_Input.hpp"

/*
游戏规则:

在4*4的界面内，一开始会出现两个数字，这两个数字有可能是2或者4，
任何时候，数字2出现的概率相对4较大，也就是90%出现2，10%出现4。

玩家每次可以选择上下左右其中一个方向去滑动，
如果当前方向无法滑动，则什么也不做，
否则滑动所有的数字方块都会往滑动的方向靠拢，
相同数字的方块在靠拢时会相加合并成一个，不同的数字则靠拢堆放，
每次移动方向上的每一排，已经合并过的数字不会与下一个合并，
即便下一个数字的值可以继续合并，也只会进行堆放，
移动或合并后，在剩余的空白处生成一个数字2或者4。
注解：
	一排2 2 2 2合并之后是4 4，而不是8
	一排2 2 4  合并之后是4 4，而不是8
	也就是已经合并过的数字不会参与下次合并

一旦获得任意一个相加后的值为2048的数字，则游戏成功。
如果没有任何空白的移动空间，且没有任何相邻的数可以合并，则游戏失败。
*/


class Game2048
{
private:
	using Direction_Raw = uint8_t;
	enum Direction : Direction_Raw
	{
		Up = 0,
		Dn,
		Lt,
		Rt,
		Enum_End,
	};

	enum GameStatus
	{
		InGame = 0,
		WinGame,
		LostGame,
	};

	struct Pos
	{
	public:
		int64_t i64X, i64Y;

	public:
		Pos operator+(const Pos &_Right) const
		{
			return
			{
				i64X + _Right.i64X,
				i64Y + _Right.i64Y,
			};
		}

		Pos operator-(const Pos &_Right) const
		{
			return
			{
				i64X - _Right.i64X,
				i64Y - _Right.i64Y,
			};
		}

		Pos &operator+=(const Pos &_Right)
		{
			i64X += _Right.i64X;
			i64Y += _Right.i64Y;

			return *this;
		}

		Pos &operator-=(const Pos &_Right)
		{
			i64X -= _Right.i64X;
			i64Y -= _Right.i64Y;

			return *this;
		}

		bool operator==(const Pos &_Right) const
		{
			return i64X == _Right.i64X && i64Y == _Right.i64Y;
		}

		bool operator!=(const Pos &_Right) const
		{
			return i64X != _Right.i64X || i64Y != _Right.i64Y;
		}
	};

	constexpr const static inline Pos arrMoveDir[Direction::Enum_End] =
	{
		{ 0,-1},
		{ 0, 1},
		{-1, 0},
		{ 1, 0},
	};

private:
	constexpr const static inline uint64_t u64Width = 4;
	constexpr const static inline uint64_t u64Height = 4;
	constexpr const static inline uint64_t u64TotalSize = u64Width * u64Height;

	uint64_t u64Tile[u64Height][u64Width];//空格子为0
	const std::span<uint64_t, u64TotalSize> u64TileFlatView{ (uint64_t *)u64Tile, u64TotalSize };//提供二维数组的一维平坦视图

	uint64_t u64EmptyCount;//空余的的格子数
	GameStatus enGameStatus;//游戏状态
	
	uint16_t u16PrintStartX = 1;//打印起始位置X
	uint16_t u16PrintStartY = 1;//打印起始位置Y

	Console_Input ci;//按键注册

	std::mt19937_64 randGen;//梅森旋转算法随机数生成器
	std::discrete_distribution<uint64_t> valueDist;//值生成-离散分布
	std::uniform_int_distribution<uint64_t> posDist;//坐标生成-均匀分布

private:
	//====================辅助函数====================
	uint64_t &GetTile(const Pos &posTarget)
	{
		return u64Tile[posTarget.i64Y][posTarget.i64X];
	}

	uint64_t GenerateRandTileVal(void)
	{
		constexpr const static uint64_t u64PossibleValues[] = { 2, 4 };
		return u64PossibleValues[valueDist(randGen)];
	}

	bool IsTilePosValid(const Pos &p) const
	{
		return	p.i64X >= 0 && p.i64X < u64Width &&
				p.i64Y >= 0 && p.i64Y < u64Height;
	}
	
	//====================刷出数字====================
	bool HasPossibleMerges(void) const
	{
		//查找所有格子的相邻，如果没有任何相邻且数值相同的格子，那么游戏失败
		for (uint64_t Y = 0; Y < u64Height; ++Y)
		{
			for (uint64_t X = 0; X < u64Width; ++X)
			{
				uint64_t u64Cur = u64Tile[Y][X];

				//向右向下检测（避免越界）
				if (X + 1 < u64Width && u64Tile[Y][X + 1] == u64Cur ||
					Y + 1 < u64Height && u64Tile[Y + 1][X] == u64Cur)
				{
					return true; //有可合并的
				}
			}
		}

		//所有检测都没返回，那么不存在可合并情况，游戏失败
		return false;
	}

	bool SpawnRandomTile(void)
	{
		if (u64EmptyCount == 0)
		{
			return false;
		}

		//还有空间，递减空格子数
		--u64EmptyCount;

		//在剩余格子中均匀生成
		auto targetPos = posDist(randGen, decltype(posDist)::param_type(0, u64EmptyCount));

		//遍历并找到第targetPos个格子
		for (auto &it : u64TileFlatView)
		{
			if (it != 0)//不是空格，继续
			{
				continue;
			}

			if (targetPos != 0)//是空格，当前是目标位置吗
			{
				--targetPos;//不是就递减并继续
				continue;
			}

			//是目标位置，生成并退出
			it = GenerateRandTileVal();
			break;
		}

		//检测必须在生成后，因为前面先进行递减然后才进行生成
		if (u64EmptyCount == 0)//只要没有剩余空间，就进行合并检测
		{
			if (!HasPossibleMerges())//没有任何一个方向可以合并
			{
				enGameStatus = LostGame;//设置输
			}
		}
		
		return true;
	}

	//====================移动合并====================
	bool MoveOrMergeTile(const Pos &posMove, const Pos &posTarget, bool &bMerge)
	{
		if (GetTile(posTarget) == 0)
		{
			return false;
		}

		//新位置
		Pos posNew = posTarget;
		while (true)
		{
			Pos posNext = posNew + posMove;//计算下一位置
			if (!IsTilePosValid(posNext))//如果下一位置超出范围
			{
				break;//则跳过
			}

			if (GetTile(posNext) != 0)//如果下一位置非0
			{
				if (!bMerge || GetTile(posNext) != GetTile(posTarget))//当前不允许合并或值无法合并
				{
					break;//则跳过
				}
			}

			//反之，当前索引没超出范围且posNext为0或可以合并

			//移动到下一位置
			posNew = posNext;
		}

		//根本没有移动
		if (posNew == posTarget)
		{
			return false;
		}

		if (GetTile(posNew) == GetTile(posTarget))
		{
			bMerge = false;//触发合并，下一次不允许合并
			++u64EmptyCount;//合并后更新空位计数
		}
		else
		{
			bMerge = true;//本次无合并，下一次可以触发合并
		}

		//直接把值加到当前位置
		//这样做，如果当前是0就相当于把值移动到当前位置，否则相当于合并值到当前位置，不用区分其他情况
		GetTile(posNew) += GetTile(posTarget);
		GetTile(posTarget) = 0;//清除原先的值

		if (GetTile(posNew) == 2048)//如果任何一个合并获得2048
		{
			enGameStatus = WinGame;//则设置游戏状态为赢
		}

		return true;
	}

	bool ProcessMove(Direction dMove)
	{
		if (enGameStatus != InGame)//不是游戏状态，直接退出
		{
			return false;
		}

		//判断方向，左右则水平，否则垂直
		bool bHorizontal = (dMove == Lt || dMove == Rt);

		//计算外层大小
		int64_t i64OuterEnd = bHorizontal ? u64Height : u64Width;//外层仅结束有影响，固定从0开始到结尾

		//计算内层大小
		int64_t i64InnerBeg, i64InnerEnd, i64InnerStep;
		if (dMove == Up || dMove == Lt)
		{
			i64InnerBeg = 1;//这里从1访问是因为第一排本身就是顶格的，没有移动的必要
			i64InnerEnd = bHorizontal ? u64Width : u64Height;//正序上边界（不会访问）
			i64InnerStep = 1;//正序
		}
		else
		{
			i64InnerBeg = (bHorizontal ? u64Width : u64Height) - 2;//这里从(bHorizontal ? u64Width : u64Height) - 2访问是因为最后一排本身就是顶格的，没有移动的必要
			i64InnerEnd = -1;//倒序下边界（不会访问）
			i64InnerStep = -1;//倒序
		}


		//确认是否进行过移动
		bool bMove = false;
		for (int64_t i64Outer = 0; i64Outer != i64OuterEnd; ++i64Outer)//外层循环固定形式
		{
			//默认状态为可合并，对于移动方向的一排中的每两个只能存在一次合并，多排之间互不影响
			//实际上，只要确认上一次是否发生过合并，如果发生过，那么本次不允许合并，就会进行堆放，下次则继续允许合并，这样就能完成防止重复合并的逻辑
			bool bMerge = true;
			for (int64_t i64Inner = i64InnerBeg; i64Inner != i64InnerEnd; i64Inner += i64InnerStep)//根据实际水平或垂直处理内层
			{
				Pos p = bHorizontal ? Pos{ i64Inner, i64Outer } : Pos{ i64Outer, i64Inner };
				bool bRet = MoveOrMergeTile(arrMoveDir[dMove], p, bMerge);//移动与合并，合并时会设置是否赢，内部不会重复检测当前游戏状态，因为可能同时出现多个2048
				bMove |= bRet;//返回值代表是否触发过合并或移动，以确认是否需要触发重绘与新值生成
			}
		}

		if (bMove && enGameStatus == InGame)//移动过且还是游戏状态，如果上面已经赢了，就没必要生成新值了，直接跳过
		{
			SpawnRandomTile();//这里会设置是否输
		}

		return bMove;
	}

	//====================打印信息====================
	void PrintGameBoard(void) const//控制台起始坐标，注意不是从0开始的，行列都从1开始
	{
		//缓存一下，不要修改原始变量
		uint16_t u16StartY = u16PrintStartY;
		uint16_t u16StartX = u16PrintStartX;

		printf("\033[?25l\033[%u;%uH", u16StartY, u16StartX);//\033[?25l 隐藏光标，每次都要设置因为用户修改控制台窗口后光标可能恢复显示
		for (auto &arrRow : u64Tile)
		{
			printf("---------------------\033[%u;%uH", ++u16StartY, u16StartX);
			for (auto u64Elem : arrRow)
			{
				if (u64Elem != 0)
				{
					printf("|%-4llu", u64Elem);
				}
				else
				{
					printf("|%-4c", ' ');
				}
			}
			printf("|\033[%u;%uH", ++u16StartY, u16StartX);
		}
		printf("---------------------\033[%u;%uH", ++u16StartY, u16StartX);
	}

	bool ShowMessageAndPrompt(const char *pMessage, const char *pPrompt) const
	{
		//缓存一下，不要修改原始变量
		uint16_t u16StartY = u16PrintStartY;
		uint16_t u16StartX = u16PrintStartX;

		//输出信息
		printf("\033[%u;%uH%s", u16StartY += (u64Height * 2 + 1), u16StartX, pMessage);

		//询问是否重开
		printf("\033[%u;%uH%s (Y/N)", ++u16StartY, u16StartX, pPrompt);
		auto waitKey = ci.WaitForKeys({ {'y'},{'Y'},{'n'},{'N'} });

		//保存按键信息
		bool bRet = false;
		if (waitKey.u16KeyCode == 'y' || waitKey.u16KeyCode == 'Y')
		{
			bRet = true;
		}
		else if(waitKey.u16KeyCode == 'n' || waitKey.u16KeyCode == 'N')
		{
			bRet = false;
		}

		//擦掉刚才输出的信息
		auto ClearPrint = [](auto Y, auto X) -> void
		{
			printf("\033[%u;%uH\033[2K", Y, X);//清空整行
		};

		//重置Y
		u16StartY = u16PrintStartY;
		//擦除
		ClearPrint(u16StartY += (u64Height * 2 + 1), u16StartX);
		ClearPrint(++u16StartY, u16StartX);

		//最后返回
		return bRet;
	}

	void PrintKeyInfo(void) const
	{
		//缓存一下，不要修改原始变量
		uint16_t u16StartY = u16PrintStartY;
		uint16_t u16StartX = u16PrintStartX;

		//设置到指定位置
		printf("\033[%u;%uH", u16StartY, u16StartX);

		auto NewLine = [&](uint16_t u16LineMove = 1) -> void
		{
			printf("\033[%u;%uH", u16StartY += u16LineMove, u16StartX);
		};
		
		printf("========2048 Game========"); NewLine();
		printf("--------Key Guide--------"); NewLine();
		printf(" W / Up Arrow    -> Up"); NewLine();
		printf(" S / Down Arrow  -> Down"); NewLine();
		printf(" A / Left Arrow  -> Left"); NewLine();
		printf(" D / Right Arrow -> Right"); NewLine();
		printf("-------------------------"); NewLine();
		printf(" R -> Restart"); NewLine();
		printf(" Q -> Quit"); NewLine();
		printf("-------------------------"); NewLine(2);

		printf("Press Any key To Start...");

		ci.WaitAnyKey();
		printf("\033[2J\033[H");//清空屏幕并把光标回到左上角
	}

	//====================重置游戏====================
	void ResetGame(void)
	{
		//清除格子数据
		std::ranges::fill(u64TileFlatView, (uint64_t)0);
		//设置空余的格子数为最大值
		u64EmptyCount = u64TotalSize;
		//设置游戏状态为游戏中
		enGameStatus = InGame;

		//在地图中随机两点生成
		SpawnRandomTile();
		SpawnRandomTile();

		//打印一次
		PrintGameBoard();
	}

	//====================按键注册====================
	void RegisterKey(void)
	{
		//注册按键
		using CILC = Console_Input::LeadCode;

		auto UpFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Up);
		};
		ci.RegisterKey({ 'w' }, UpFunc);
		ci.RegisterKey({ 'W' }, UpFunc);
		ci.RegisterKey({ 72, CILC::Code_E0 }, UpFunc);

		auto LtFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Lt);
		};
		ci.RegisterKey({ 'a' }, LtFunc);
		ci.RegisterKey({ 'A' }, LtFunc);
		ci.RegisterKey({ 75, CILC::Code_E0 }, LtFunc);

		auto DnFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Dn);
		};
		ci.RegisterKey({ 's' }, DnFunc);
		ci.RegisterKey({ 'S' }, DnFunc);
		ci.RegisterKey({ 80, CILC::Code_E0 }, DnFunc);

		auto RtFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Rt);
		};
		ci.RegisterKey({ 'd' }, RtFunc);
		ci.RegisterKey({ 'D' }, RtFunc);
		ci.RegisterKey({ 77, CILC::Code_E0 }, RtFunc);

		auto RestartFunc = [&](auto &) -> long
		{
			if (this->ShowMessageAndPrompt("You Press Restart Key!", "Restart?"))
			{
				ResetGame();
			}

			return 0;//任何时候此调用都返回0，不论是否重开，因为不触发外部绘制（重开内部会绘制）
		};
		ci.RegisterKey({ 'r' }, RestartFunc);
		ci.RegisterKey({ 'R' }, RestartFunc);

		auto QuitFunc = [&](auto &) -> long
		{
			if (this->ShowMessageAndPrompt("You Press Quit Key!", "Quit?"))
			{
				return -1;//退出返回-1
			}

			return 0;//否则返回0，就当无事发生
		};
		ci.RegisterKey({ 'q' }, QuitFunc);
		ci.RegisterKey({ 'Q' }, QuitFunc);
	}

public:
	//构造
	Game2048(uint32_t u32Seed, uint16_t _u16PrintStartX = 1, uint16_t _u16PrintStartY = 1, double dSpawnWeights_2 = 0.9, double dSpawnWeights_4 = 0.1) :
		u64Tile{},

		u64EmptyCount(u64TotalSize),
		enGameStatus(),

		u16PrintStartX(_u16PrintStartX),
		u16PrintStartY(_u16PrintStartY),

		randGen(u32Seed),
		valueDist({ dSpawnWeights_2, dSpawnWeights_4 }),
		posDist()
	{}
	~Game2048(void) = default;//默认析构

	//删除移动、拷贝方式
	Game2048(const Game2048 &) = delete;
	Game2048(Game2048 &&) = delete;
	Game2048 &operator=(const Game2048 &) = delete;
	Game2048 &operator=(Game2048 &&) = delete;

	//初始化
	void Init(void)
	{
		//打印一次按键信息
		PrintKeyInfo();
		//这里必须先处理游戏
		ResetGame();
		//然后才注册按键，防止出现提前按键问题
		RegisterKey();
		//初始化后，后续直接调用ResetGame则无问题
	}
	
	//循环
	bool Loop(void)
	{
		switch (ci.AtLeastOne())//处理一次按键
		{
		default://其它返回，跳过处理
		case 0://调用失败（没有移动）
			return true;//直接返回
			break;
		case 1://调用成功
			PrintGameBoard();//打印，不急着返回，后续判断输赢
			break;
		case -1://用户提前退出
			return false;//直接返回
		}

		switch (enGameStatus)//判断一下输赢
		{
		case Game2048::WinGame:
			if (!ShowMessageAndPrompt("You Win!", "Restart?"))
			{
				return false;//退出
			}
			ResetGame();//重置
			break;
		case Game2048::LostGame:
			if (!ShowMessageAndPrompt("You Lost...", "Restart?"))
			{
				return false;//退出
			}
			ResetGame();//重置
			break;
		default:
			break;
		}

		return true;//返回true继续循环，否则跳出结束程序
	}

	//调试
#ifdef _DEBUG
	void Debug(void)
	{
		u64Tile[0][0] = 2;
		u64Tile[0][1] = 2;
		u64Tile[0][2] = 2;
		u64Tile[0][3] = 2;

		u64Tile[1][0] = 2;
		u64Tile[1][1] = 2;
		u64Tile[1][2] = 4;
		u64Tile[1][3] = 0;

		u64Tile[2][0] = 4;
		u64Tile[2][1] = 2;
		u64Tile[2][2] = 2;
		u64Tile[2][3] = 2;

		u64Tile[3][0] = 2;
		u64Tile[3][1] = 2;
		u64Tile[3][2] = 0;
		u64Tile[3][3] = 2;

		u64EmptyCount = 2;

		PrintGameBoard();
	}
#endif
};

#ifdef _WIN32
#include <windows.h>
//Windows平台下启用虚拟终端序列的函数
bool EnableVirtualTerminalProcessing(void)
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

// 自动启用虚拟终端处理的宏
#define INIT_CONSOLE()\
	do {\
	    if (!EnableVirtualTerminalProcessing())\
		{\
	        exit(-1);\
	    }\
	} while(0)
#else
//非Windows平台通常默认支持ANSI转义序列，无需额外操作
#define INIT_CONSOLE() (void)0  //空操作
#endif

int main(void)
{
	INIT_CONSOLE();//Windows福报

	Game2048 game(std::random_device{}());

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
