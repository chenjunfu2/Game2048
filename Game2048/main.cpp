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
不同的数字则靠拢堆放
最后在剩余的空白处生成一个数字2或者4。

一旦获得任意一个相加后的值为2048的数字，
则游戏成功。

如果没有任何可以移动的空间，
且没有任何相邻的数可以合并，则游戏失败
*/


class Game2048
{
private:
	constexpr const static inline uint64_t u64Width = 4;
	constexpr const static inline uint64_t u64Height = 4;
	constexpr const static inline uint64_t u64TotalSize = u64Width * u64Height;

	uint64_t u64Tile[u64Height][u64Width];//空格子为0
	std::span<uint64_t, u64TotalSize> u64TileFlatView{ (uint64_t *)u64Tile, u64TotalSize };//提供二维数组的一维平坦视图

	uint64_t u64EmptyCount;//空余的的格子数
	std::mt19937_64 randGen;//梅森旋转算法随机数生成器
	std::discrete_distribution<uint64_t> valueDist;//值生成-离散分布
	std::uniform_int_distribution<uint64_t> posDist;//坐标生成-均匀分布

private:
	uint64_t GetRandTileValue(void)
	{
		constexpr const static uint64_t u64PossibleValues[] = { 2, 4 };
		return u64PossibleValues[valueDist(randGen)];
	}

	bool SpawnNumInEmptySpace(void)
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
			it = GetRandTileValue();
			break;
		}
		
		return true;
	}

	void ClearTile(void)
	{
		//清除格子数据
		std::ranges::fill(u64TileFlatView, (uint64_t)0);
		//设置空余的格子数为最大值
		u64EmptyCount = u64TotalSize;
	}

public:
	Game2048(uint32_t u32Seed, double dSpawnWeights_2 = 0.8, double dSpawnWeights_4 = 0.2) :
		u64Tile{},
		u64EmptyCount(u64TotalSize),
		randGen(u32Seed),
		valueDist({ dSpawnWeights_2, dSpawnWeights_4 }),
		posDist()
	{}

	~Game2048(void) = default;

	Game2048(const Game2048 &) = delete;
	Game2048(Game2048 &&) = delete;
	Game2048 &operator=(const Game2048 &) = delete;
	Game2048 &operator=(Game2048 &&) = delete;

	void Start(void)
	{
		//清空格子
		ClearTile();

		//在地图中随机两点生成
		SpawnNumInEmptySpace();
		SpawnNumInEmptySpace();
	}

	void Print(void)
	{
		for (auto &arrRow : u64Tile)
		{
			printf("---------------------\n");
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
			printf("|\n");
		}
		printf("---------------------\n");
	}


	using Direction_Raw = uint8_t;
	enum Direction : Direction_Raw
	{
		Up = 0,
		Dn,
		Lt,
		Rt,
		Enum_End,
	};

private:
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

	bool MoveAndAddTile(Direction dir, const Pos &posTarget)
	{
		if (GetTail(posTarget) == 0)
		{
			return false;
		}

		//获取移动量
		const auto &posMove = arrMoveDir[dir];

		//新位置
		Pos posNew = posTarget;
		while (true)
		{
			Pos posNext = posNew + posMove;//计算下一位置
			if (!TestTailIndexRange(posNext) ||//如果下一位置超出范围或
				(GetTail(posNext) != 0 && GetTail(posNext) != GetTail(posTarget)))//非0且不可合并
			{
				break;//则跳过
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
			++u64EmptyCount;//合并后让出空间
		}

		//直接把值加到当前位置
		//这样做，如果当前是0就相当于把值移动到当前位置，否则相当于合并值到当前位置，不用区分其他情况
		GetTail(posNew) += GetTail(posTarget);
		GetTail(posTarget) = 0;

		return true;
	}

	bool MoveDn(void)
	{
		bool bMove = false;
		for (int64_t i64Cow = u64Height - 1 - 1; i64Cow > 0 - 1; i64Cow += -1)
		{
			for (int64_t i64Row = 0; i64Row != u64Width; i64Row += 1)
			{
				bool bRet = MoveAndAddTile(Direction::Dn, { i64Row,i64Cow });//额外保存变量，防止短路求值跳过调用
				bMove |= bRet;
			}
		}

		return bMove;
	}

	bool MoveUp(void)
	{
		bool bMove = false;
		for (int64_t i64Cow = 0LL + 1; i64Cow < u64Height; i64Cow += 1)
		{
			for (int64_t i64Row = 0; i64Row != u64Width; i64Row += 1)
			{
				bool bRet = MoveAndAddTile(Direction::Up, { i64Row,i64Cow });//额外保存变量，防止短路求值跳过调用
				bMove |= bRet;
			}
		}

		return bMove;
	}

	bool MoveRt(void)
	{
		bool bMove = false;
		for (int64_t i64Row = u64Width - 1 - 1; i64Row > 0 - 1; i64Row += -1)
		{
			for (int64_t i64Cow = 0; i64Cow != u64Height; i64Cow += 1)
			{
				bool bRet = MoveAndAddTile(Direction::Rt, { i64Row,i64Cow });//额外保存变量，防止短路求值跳过调用
				bMove |= bRet;
			}
		}

		return bMove;
	}

	bool MoveLt(void)
	{
		bool bMove = false;
		for (int64_t i64Row = 0LL + 1; i64Row < u64Width; i64Row += 1)
		{
			for (int64_t i64Cow = 0; i64Cow != u64Height; i64Cow += 1)
			{
				bool bRet = MoveAndAddTile(Direction::Lt, { i64Row,i64Cow });//额外保存变量，防止短路求值跳过调用
				bMove |= bRet;
			}
		}

		return bMove;
	}

public:
	bool Move(Direction dMove)
	{
		bool bRet = false;

		switch (dMove)
		{
		case Game2048::Up:
			bRet = MoveUp();
			break;
		case Game2048::Dn:
			bRet = MoveDn();
			break;
		case Game2048::Lt:
			bRet = MoveLt();
			break;
		case Game2048::Rt:
			bRet = MoveRt();
			break;
		default:
			break;
		}

		if (bRet)
		{
			SpawnNumInEmptySpace();
		}

		return bRet;
	}

};


int main(void)
{
	uint32_t seed = std::random_device{}();
	printf("seed:%u\n", seed);
	Game2048 game(seed);

	game.Start();
	game.Print();

	Console_Input ci;

	ci.RegisterKey({ 'w' }, [&](auto &) -> long { return game.Move(Game2048::Up);});
	ci.RegisterKey({ 'W' }, [&](auto &) -> long { return game.Move(Game2048::Up);});

	ci.RegisterKey({ 'a' }, [&](auto &) -> long { return game.Move(Game2048::Lt);});
	ci.RegisterKey({ 'A' }, [&](auto &) -> long { return game.Move(Game2048::Lt);});

	ci.RegisterKey({ 's' }, [&](auto &) -> long { return game.Move(Game2048::Dn);});
	ci.RegisterKey({ 'S' }, [&](auto &) -> long { return game.Move(Game2048::Dn);});

	ci.RegisterKey({ 'd' }, [&](auto &) -> long { return game.Move(Game2048::Rt);});
	ci.RegisterKey({ 'D' }, [&](auto &) -> long { return game.Move(Game2048::Rt);});

	while (true)
	{
		if (ci.AtLeastOne() != 0)
		{
			game.Print();
		}
	}

	return 0;
}
