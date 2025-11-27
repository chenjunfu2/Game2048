#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <random>
#include <span>
#include <algorithm>

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

			if (targetPos != 0)//是空格，当前是目标位置吗？不是就继续
			{
				--targetPos;
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
			printf("-----------------------------\n");
			for (auto u64Elem : arrRow)
			{
				printf("|%-6llu", u64Elem);
			}
			printf("|\n");
		}
		printf("-----------------------------\n");
	}
};


int main(void)
{
	Game2048 game(std::random_device{}());

	game.Start();
	game.Print();


	return 0;
}
