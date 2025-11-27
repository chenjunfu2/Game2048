#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <random>
#include <span>
#include <algorithm>

#include "Console_Input.hpp"

/*
游戏规则:
数字2出现的概率相对4较大。

在4*4的界面内
一开始会出现两个数字，这两个数字有可能是2或者4，
每次可以选择上下左右其中一个方向去滑动，每一次滑动，
所有的数字方块都会往滑动的方向靠拢，
相同数字的方块在靠拢时会相加合并成一个，
但是每次移动方向上那一排只会合并相邻的两个，而不会连续合并，
不同的数字则靠拢堆放，
最后在剩余的空白处生成一个数字2或者4。

一旦获得任意一个相加后的值为2048的数字，
则游戏成功。

如果没有任何可以移动的空间，
且没有任何相邻的数可以合并，则游戏失败
*/


class Game2048
{
public:
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
	uint64_t GetRandTileValue(void)
	{
		constexpr const static uint64_t u64PossibleValues[] = { 2, 4 };
		return u64PossibleValues[valueDist(randGen)];
	}

	bool TestMerge(void)
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

	bool SpawnNumInEmptySpace(void)
	{
		if (u64EmptyCount == 0 || enGameStatus != InGame)
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
			it = GetRandTileValue();
			break;
		}

		//检测必须在生成后，因为前面先进行递减然后才进行生成
		if (u64EmptyCount == 0)//只要没有剩余空间，就进行合并检测
		{
			if (!TestMerge())//没有任何一个方向可以合并
			{
				enGameStatus = LostGame;//设置输
			}
		}
		
		return true;
	}

	void ClearTile(void)
	{
		//清除格子数据
		std::ranges::fill(u64TileFlatView, (uint64_t)0);
		//设置空余的格子数为最大值
		u64EmptyCount = u64TotalSize;
		//设置游戏状态为游戏中
		enGameStatus = InGame;
	}

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


	bool TestTailIndexRange(const Pos &p)
	{
		return	p.i64X >= 0 && p.i64X < u64Width &&
			p.i64Y >= 0 && p.i64Y < u64Height;
	}

	uint64_t &GetTail(const Pos &posTarget)
	{
		return u64Tile[posTarget.i64Y][posTarget.i64X];
	}

	bool MoveAndAddTile(const Pos &posMove, const Pos &posTarget, bool &bMerge)
	{
		if (GetTail(posTarget) == 0)
		{
			return false;
		}

		//新位置
		Pos posNew = posTarget;
		while (true)
		{
			Pos posNext = posNew + posMove;//计算下一位置
			if (!TestTailIndexRange(posNext))//如果下一位置超出范围
			{
				break;//则跳过
			}

			if (GetTail(posNext) != 0)//如果下一位置非0
			{
				if (!bMerge || GetTail(posNext) != GetTail(posTarget))//当前不允许合并或值无法合并
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

		if (GetTail(posNew) == GetTail(posTarget))
		{
			bMerge = false;//设置状态为不可合并
			++u64EmptyCount;//合并后更新空位计数
		}

		//直接把值加到当前位置
		//这样做，如果当前是0就相当于把值移动到当前位置，否则相当于合并值到当前位置，不用区分其他情况
		GetTail(posNew) += GetTail(posTarget);
		GetTail(posTarget) = 0;

		if (GetTail(posNew) == 2048)//如果任何一个合并获得2048
		{
			enGameStatus = WinGame;//则设置游戏状态为赢
		}

		return true;
	}

	bool GeneralMove(Direction dMove)
	{
		bool bMove = false;

		if (enGameStatus != InGame)//不是游戏状态，退出
		{
			return bMove;
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

		for (int64_t i64Outer = 0; i64Outer != i64OuterEnd; ++i64Outer)//外层循环固定形式
		{
			bool bMerge = true;//默认状态为可合并，对于移动方向的一排中只能存在一次合并，多排之间互不影响
			for (int64_t i64Inner = i64InnerBeg; i64Inner != i64InnerEnd; i64Inner += i64InnerStep)//根据实际水平或垂直处理内层
			{
				Pos p = bHorizontal ? Pos{ i64Inner, i64Outer } : Pos{ i64Outer, i64Inner };
				bool bRet = MoveAndAddTile(arrMoveDir[dMove], p, bMerge);//移动与合并，合并时会设置输赢状态，内部不会重复检测当前游戏状态，因为可能同时出现多个2048
				bMove |= bRet;
			}
		}

		return bMove;
	}

	bool Move(Direction dMove)
	{
		bool bRet = GeneralMove(dMove);//这里会设置是否赢

		if (bRet)
		{
			SpawnNumInEmptySpace();//这里会设置是否输
		}

		return bRet;
	}

	void RegisterKey(void)
	{
		//注册按键
		using CILC = Console_Input::LeadCode;

		auto UpFunc = [&](auto &) -> long
		{
			return this->Move(Game2048::Up);
		};
		ci.RegisterKey({ 'w' }, UpFunc);
		ci.RegisterKey({ 'W' }, UpFunc);
		ci.RegisterKey({ 72, CILC::Code_E0 }, UpFunc);

		auto LtFunc = [&](auto &) -> long
		{
			return this->Move(Game2048::Lt);
		};
		ci.RegisterKey({ 'a' }, LtFunc);
		ci.RegisterKey({ 'A' }, LtFunc);
		ci.RegisterKey({ 75, CILC::Code_E0 }, LtFunc);

		auto DnFunc = [&](auto &) -> long
		{
			return this->Move(Game2048::Dn);
		};
		ci.RegisterKey({ 's' }, DnFunc);
		ci.RegisterKey({ 'S' }, DnFunc);
		ci.RegisterKey({ 80, CILC::Code_E0 }, DnFunc);

		auto RtFunc = [&](auto &) -> long
		{
			return this->Move(Game2048::Rt);
		};
		ci.RegisterKey({ 'd' }, RtFunc);
		ci.RegisterKey({ 'D' }, RtFunc);
		ci.RegisterKey({ 77, CILC::Code_E0 }, RtFunc);

		auto RestartFunc = [&](auto &) -> long
		{
			if (this->PrintInfoAndQuery("You Press Restart Key!", "Restart?"))
			{
				StartOrRestart();
			}

			return 0;//任何时候此调用都返回0，不论是否重开，因为不触发外部绘制（重开内部会绘制）
		};
		ci.RegisterKey({ 'r' }, RestartFunc);
		ci.RegisterKey({ 'R' }, RestartFunc);

		auto QuitFunc = [&](auto &) -> long
		{
			if (this->PrintInfoAndQuery("You Press Quit Key!", "Quit?"))
			{
				return -1;//退出返回-1
			}

			return 0;//否则返回0，就当无事发生
		};
		ci.RegisterKey({ 'q' }, QuitFunc);
		ci.RegisterKey({ 'Q' }, QuitFunc);
	}

	void StartOrRestart(void)
	{
		//清空格子
		ClearTile();

		//在地图中随机两点生成
		SpawnNumInEmptySpace();
		SpawnNumInEmptySpace();

		//打印一次
		Print();
	}

	void Print(void) const//控制台起始坐标，注意不是从0开始的，行列都从1开始
	{
		//缓存一下，不要修改原始变量
		uint16_t u16StartY = u16PrintStartY;
		uint16_t u16StartX = u16PrintStartX;

		printf("\033[%u;%uH", u16StartY, u16StartX);
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

	bool PrintInfoAndQuery(const char *pInfo, const char *pQuery) const
	{
		//缓存一下，不要修改原始变量
		uint16_t u16StartY = u16PrintStartY;
		uint16_t u16StartX = u16PrintStartX;

		//输出信息
		printf("\033[%u;%uH%s", u16StartY += (u64Height * 2 + 1), u16StartX, pInfo);

		//询问是否重开
		printf("\033[%u;%uH%s (Y/N)", ++u16StartY, u16StartX, pQuery);
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
		auto ClearPrint = [](auto Y, auto X, const char *pText, const char *pExp = "") -> void
		{
			printf("\033[%u;%uH", Y, X);

			size_t szLength = strlen(pText) + strlen(pExp);
			while (szLength-- > 0)
			{
				putchar(' ');
			}
		};

		//重置Y
		u16StartY = u16PrintStartY;
		//擦除
		ClearPrint(u16StartY += (u64Height * 2 + 1), u16StartX, pInfo);
		ClearPrint(++u16StartY, u16StartX, pQuery, " (Y/N)");

		//最后返回
		return bRet;
	}

public:
	Game2048(uint32_t u32Seed, uint16_t _u16PrintStartX = 1, uint16_t _u16PrintStartY = 1, double dSpawnWeights_2 = 0.8, double dSpawnWeights_4 = 0.2) :
		u64Tile{},

		u64EmptyCount(u64TotalSize),
		enGameStatus(),

		u16PrintStartX(_u16PrintStartX),
		u16PrintStartY(_u16PrintStartY),

		randGen(u32Seed),
		valueDist({ dSpawnWeights_2, dSpawnWeights_4 }),
		posDist()
	{}
	~Game2048(void) = default;
	Game2048(const Game2048 &) = delete;
	Game2048(Game2048 &&) = delete;
	Game2048 &operator=(const Game2048 &) = delete;
	Game2048 &operator=(Game2048 &&) = delete;

	void Init(void)
	{
		//这里必须先处理游戏
		StartOrRestart();
		//然后才注册按键，防止出现提前按键问题
		RegisterKey();
		//初始化后，后续直接调用StartOrRestart则无问题
	}
	
	bool Loop(void)
	{
		switch (ci.AtLeastOne())//处理一次按键
		{
		case 0://调用失败（没有移动）
			return true;//直接返回
			break;
		case 1://调用成功
			Print();//打印，不急着返回，后续判断输赢
			break;
		case -1://用户提前退出
			return false;//直接返回
		default:
			break;
		}

		switch (enGameStatus)//判断一下输赢
		{
		case Game2048::WinGame:
			if (!PrintInfoAndQuery("You Win!", "Restart?"))
			{
				return false;
			}
			
			StartOrRestart();
			break;
		case Game2048::LostGame:
			if (!PrintInfoAndQuery("You Lost...", "Restart?"))
			{
				return false;
			}
			
			StartOrRestart();
			break;
		default:
			break;
		}

		return true;//返回true继续循环，否则跳出结束程序
	}
};


int main(void)
{
	Game2048 game(std::random_device{}());

	//初始化
	game.Init();
	
	//游戏循环
	while (game.Loop())
	{
		continue;
	}

	return 0;
}
