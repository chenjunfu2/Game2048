#pragma once

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>//获取uintxx_t的对应printf格式化串
#include <stddef.h>
#include <random>
#include <span>
#include <algorithm>
#include <assert.h>

//根据平台切换输入
#if defined(_WIN32)
	#include "Console_Input_Windows.hpp"
	#include "Windows_Keys.hpp"
#elif defined(__linux__)
	#include "Console_Input_Linux.hpp"
	#include "Linux_Keys.hpp"
#endif

#include "Console_Output.hpp"

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

分数计算：
每次产生合并时，合并的值增加到分数上
比如一次移动中，2与2合并得到4，当前加4分
或者一次移动中，4与4合并得到8，2与2合并得到4，当前加12分
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
			return {
				i64X + _Right.i64X,
				i64Y + _Right.i64Y,
			};
		}

		Pos operator-(const Pos &_Right) const
		{
			return {
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

private:
	constexpr const static inline size_t szWidth = 4;
	constexpr const static inline size_t szHeight = 4;
	constexpr const static inline size_t szTotalSize = szWidth * szHeight;

	uint64_t u64Tile[szHeight][szWidth];//空格子为0
	const std::span<uint64_t, szTotalSize> u64TileFlatView{ (uint64_t *)u64Tile, szTotalSize };//提供二维数组的一维平坦视图

	size_t szEmptyCount;//空余的的格子数
	uint64_t u64GameScore;//游戏分数
	GameStatus enGameStatus;//游戏状态

	std::mt19937_64 randGen;//梅森旋转算法随机数生成器
	std::discrete_distribution<uint64_t> valueDist;//值生成-离散分布
	std::uniform_int_distribution<uint64_t> posDist;//坐标生成-均匀分布

	Console_Input &ci;//输入
	Console_Output &co;//输出

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
		return p.i64X >= 0 && p.i64X < szWidth &&
			   p.i64Y >= 0 && p.i64Y < szHeight;
	}

	//====================刷出数字====================
	bool HasPossibleMerges(void) const
	{
		//查找所有格子的相邻，如果没有任何相邻且数值相同的格子，那么游戏失败
		for (size_t Y = 0; Y < szHeight; ++Y)
		{
			for (size_t X = 0; X < szWidth; ++X)
			{
				uint64_t u64Cur = u64Tile[Y][X];

				//向右向下检测（避免越界）
				if (X + 1 < szWidth && u64Tile[Y][X + 1] == u64Cur ||
					Y + 1 < szHeight && u64Tile[Y + 1][X] == u64Cur)
				{
					return true;//有可合并的
				}
			}
		}

		//所有检测都没返回，那么不存在可合并情况，游戏失败
		return false;
	}

	bool SpawnRandomTile(void)
	{
		if (szEmptyCount == 0)
		{
			return false;
		}

		//还有空间，递减空格子数
		--szEmptyCount;

		//在剩余格子中均匀生成
		auto targetPos = posDist(randGen, decltype(posDist)::param_type(0, szEmptyCount));//因为取到端点，所以前面先递减

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
		if (szEmptyCount == 0)//只要没有剩余空间，就进行合并检测
		{
			if (!HasPossibleMerges())//没有任何一个方向可以合并
			{
				enGameStatus = LostGame;//设置输
			}
		}

		return true;
	}

	//====================移动合并====================
	bool MoveOrMergeTile(const Pos &posTarget, Pos &posLast, Direction dMove)
	{
		if (GetTile(posTarget) == 0)//直到非0
		{
			return false;
		}

		//反向移动量数组
		constexpr const static Pos arrReverseMoveDeltas[Direction::Enum_End] =
		{
			{ 0, 1 },//[Up] -> Dn
			{ 0,-1 },//[Dn] -> Up
			
			{ 1, 0 },//[Lt] -> Rt
			{-1, 0 },//[Rt] -> Lt
		};

		auto &valTarget = GetTile(posTarget);
		auto &valLast = GetTile(posLast);

		if (valLast == 0)//空位置，移动
		{
			valLast = valTarget;//移动后可能下次会触发合并，无须更新posLast
		}
		else if (valLast == valTarget)//值相等，合并
		{
			valLast += valTarget;
			posLast += arrReverseMoveDeltas[dMove];//合并后下次不能判断当前位置，移动到新位置

			++szEmptyCount;//合并后更新空位计数
			u64GameScore += valLast;//合并后更新分数

			//如果任何一个合并获得2048
			if (valLast == 2048)
			{
				enGameStatus = WinGame;//则设置游戏状态为赢
			}
		}
		else//值不相等，也不为空，移动到旁边堆放
		{
			//当前位置无法使用，移动到新位置
			posLast += arrReverseMoveDeltas[dMove];
			if (posLast == posTarget)//如果新位置和当前位置相同则跳过
			{
				return false;
			}
			
			//进行移动
			auto &valNewLast = GetTile(posLast);
			assert(valNewLast == 0);//这里必然是0
			valNewLast = valTarget;//移动后下次可能触发合并，无须更新posLast
		}

		//清空原始位置
		valTarget = 0;

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
		int64_t i64OuterEnd = bHorizontal ? szHeight : szWidth;//外层仅结束有影响，固定从0开始到结尾

		//计算内层大小
		int64_t i64InnerFirst, i64InnerBeg, i64InnerEnd, i64InnerStep;
		if (dMove == Up || dMove == Lt)//正序
		{
			i64InnerFirst = 0;//第一个元素的索引
			i64InnerBeg = i64InnerFirst + 1;//这里从1访问是因为第一排本身就是顶格的，没有移动的必要
			i64InnerEnd = bHorizontal ? szWidth : szHeight;//正序上边界（不会访问）
			i64InnerStep = i64InnerFirst + 1;//正序
		}
		else//倒序
		{
			i64InnerFirst = (bHorizontal ? szWidth : szHeight) - 1;//最后一个元素的索引
			i64InnerBeg = i64InnerFirst - 1;//这里从i64InnerFirst - 1访问是因为最后一排本身就是顶格的，没有移动的必要
			i64InnerEnd = -1;//倒序下边界（不会访问）
			i64InnerStep = -1;//倒序
		}


		//确认是否进行过移动
		bool bMove = false;
		for (int64_t i64Outer = 0; i64Outer != i64OuterEnd; ++i64Outer)//外层循环固定形式
		{
			//默认状态为可合并，对于移动方向的一排中的每两个只能存在一次合并，多排之间互不影响
			//实际上，只要确认上一次是否发生过合并，如果发生过，那么本次不允许合并，就会进行堆放，下次则继续允许合并，这样就能完成防止重复合并的逻辑
			
			//这里上一个合并的坐标初始化为这一行的起始坐标
			Pos pLast = bHorizontal ? Pos{ i64InnerFirst, i64Outer } : Pos{ i64Outer, i64InnerFirst };
			//目标存在外层循环固定值，根据移动方向初始化
			Pos pTarget = bHorizontal ? Pos{ 0, i64Outer } : Pos{ i64Outer, 0 };

			for (int64_t i64Inner = i64InnerBeg; i64Inner != i64InnerEnd; i64Inner += i64InnerStep)//根据实际水平或垂直处理内层
			{
				//根据移动方向更新变动的值
				if (bHorizontal)
				{
					pTarget.i64X = i64Inner;
				}
				else
				{
					pTarget.i64Y = i64Inner;
				}

				//移动与合并，合并时会设置是否赢，内部不会重复检测当前游戏状态，因为可能同时出现多个2048
				//返回值代表是否触发过合并或移动，以确认是否需要触发重绘与新值生成
				bMove |= MoveOrMergeTile(pTarget, pLast, dMove);
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
		co.SetCursorBase();//回到初始位置
#if defined(_WIN32)//仅Windows下每次都要隐藏，否则窗口改变会自动重新显示
		co.HideCursor();
#endif// defined(_WIN32)

		printf("Score:[%" PRIu64 "]", u64GameScore);//打印分数
		co.NextLine();
		printf("┌────┬────┬────┬────┐");//打印开头行
		co.NextLine();

		size_t szIndexY = 0;//控制最后一行不输出中间行的计数器
		for (auto &arrRow : u64Tile)
		{
			for (auto u64Elem : arrRow)
			{
				if (u64Elem != 0)
				{
					printf("│%4" PRIu64, u64Elem);//使用inttypes.h中的格式化串
				}
				else
				{
					printf("│    ");//输出空格以对齐
				}
			}
			printf("│");
			co.NextLine();

			if (++szIndexY != szHeight)//最后一行不输出
			{
				printf("├────┼────┼────┼────┤");//输出中间行
				co.NextLine();
			}
		}

		printf("└────┴────┴────┴────┘");//打印结尾行
		co.NextLine();
	}

	bool ShowMessageAndPrompt(const char *pMessage, const char *pPrompt) const
	{
		//co.SetCursorBase();//不用回到初始位置，当前位置即为输出的下一行
#if defined(_WIN32)//仅Windows下每次都要隐藏，否则窗口改变会自动重新显示
		co.HideCursor();
#endif// defined(_WIN32)

		//输出信息
		printf("%s", pMessage);
		co.NextLine();

		//询问信息
		printf("%s (Y/N)", pPrompt);
		co.NextLine();

		//等待按键
		auto waitKey = ci.WaitForKeys({ Keys::Y, Keys::SHIFT_Y, Keys::N, Keys::SHIFT_N });

		//保存按键信息
		bool bRet = false;
		if (waitKey == Keys::Y || waitKey == Keys::SHIFT_Y)
		{
			bRet = true;
		}
		else if (waitKey == Keys::N || waitKey == Keys::SHIFT_N)
		{
			bRet = false;
		}

		//擦掉刚才输出的信息
		//第一行
		co.PrevLine();
		co.ClearLine();
		//第二行
		co.PrevLine();
		co.ClearLine();

		//最后返回
		return bRet;
	}

	void PrintKeyInfo(void) const
	{
		//清屏并设置光标到指定绘制起始位置
		co.ClearScreen();
		co.SetCursorBase();
#if defined(_WIN32)//仅Windows下每次都要隐藏，否则窗口改变会自动重新显示
		co.HideCursor();
#endif// defined(_WIN32)

		//输出
		printf("========2048 Game========");
		co.NextLine();
		printf("--------Key Guide--------");
		co.NextLine();
		printf(" W / Up Arrow    -> Up");
		co.NextLine();
		printf(" S / Down Arrow  -> Down");
		co.NextLine();
		printf(" A / Left Arrow  -> Left");
		co.NextLine();
		printf(" D / Right Arrow -> Right");
		co.NextLine();
		printf("-------------------------");
		co.NextLine();
		printf(" R -> Restart");
		co.NextLine();
		printf(" Q -> Quit");
		co.NextLine();
		printf("-------------------------");
		co.NextLine(2);

		printf("Press Any key To Start...");
		co.NextLine();

		//等待任意键
		ci.WaitAnyKey();
		//清空屏幕
		co.ClearScreen();
	}

	//====================重置游戏====================
	void ResetGame(void)
	{
		//清除格子数据
		std::ranges::fill(u64TileFlatView, (uint64_t)0);
		//设置空余的格子数为最大值
		szEmptyCount = szTotalSize;
		//设置游戏分数为0
		u64GameScore = 0;
		//设置游戏状态为游戏中
		enGameStatus = InGame;

		//在地图中随机两点生成
		SpawnRandomTile();
		SpawnRandomTile();

		//清除屏幕
		printf("\033[2J\033[H");

		//打印一次
		PrintGameBoard();
	}

	//====================按键注册====================
	void RegisterKey(void)
	{
		//注册按键

		auto UpFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Up);
		};
		ci.RegisterKey(Keys::W, UpFunc);
		ci.RegisterKey(Keys::SHIFT_W, UpFunc);
		ci.RegisterKey(Keys::UP_ARROW, UpFunc);

		auto LtFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Lt);
		};
		ci.RegisterKey(Keys::A, LtFunc);
		ci.RegisterKey(Keys::SHIFT_A, LtFunc);
		ci.RegisterKey(Keys::LEFT_ARROW, LtFunc);

		auto DnFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Dn);
		};
		ci.RegisterKey(Keys::S, DnFunc);
		ci.RegisterKey(Keys::SHIFT_S, DnFunc);
		ci.RegisterKey(Keys::DOWN_ARROW, DnFunc);

		auto RtFunc = [&](auto &) -> long
		{
			return this->ProcessMove(Game2048::Rt);
		};
		ci.RegisterKey(Keys::D, RtFunc);
		ci.RegisterKey(Keys::SHIFT_D, RtFunc);
		ci.RegisterKey(Keys::RIGHT_ARROW, RtFunc);

		auto RestartFunc = [&](auto &) -> long
		{
			if (this->ShowMessageAndPrompt("You Press Restart Key!", "Restart?"))
			{
				ResetGame();
			}

			return 0;//任何时候此调用都返回0，不论是否重开，因为不触发外部绘制（重开内部会绘制）
		};
		ci.RegisterKey(Keys::R, RestartFunc);
		ci.RegisterKey(Keys::SHIFT_R, RestartFunc);

		auto QuitFunc = [&](auto &) -> long
		{
			if (this->ShowMessageAndPrompt("You Press Quit Key!", "Quit?"))
			{
				return -1;//退出返回-1
			}

			return 0;//否则返回0，就当无事发生
		};
		ci.RegisterKey(Keys::Q, QuitFunc);
		ci.RegisterKey(Keys::SHIFT_Q, QuitFunc);
	}

public:
	//构造
	Game2048(Console_Input &_ci, Console_Output &_co, uint32_t u32Seed = std::random_device{}(), double dSpawnWeights_2 = 0.9, double dSpawnWeights_4 = 0.1) :
		u64Tile{},

		szEmptyCount(szTotalSize),
		u64GameScore(0),
		enGameStatus(),

		randGen(u32Seed),
		valueDist({ dSpawnWeights_2, dSpawnWeights_4 }),
		posDist(),

		ci(_ci),
		co(_co)
	{
		co.HideCursor();//隐藏光标
	}
	~Game2048(void)
	{
		co.ShowCursor();//显示光标
	}

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
		u64Tile[0][0] = 0;
		u64Tile[0][1] = 2;
		u64Tile[0][2] = 4;
		u64Tile[0][3] = 8;

		u64Tile[1][0] = 16;
		u64Tile[1][1] = 32;
		u64Tile[1][2] = 64;
		u64Tile[1][3] = 128;

		u64Tile[2][0] = 256;
		u64Tile[2][1] = 512;
		u64Tile[2][2] = 1024;
		u64Tile[2][3] = 2048;

		u64Tile[3][0] = 4096;
		u64Tile[3][1] = 8192;
		u64Tile[3][2] = 0;
		u64Tile[3][3] = 0;

		szEmptyCount = 3;
		u64GameScore = 0;

		PrintGameBoard();
	}
#endif
};
