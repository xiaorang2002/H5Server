#ifndef GAMELOGIC_SGJ_H
#define GAMELOGIC_SGJ_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <vector>

#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))
//水果代号
#define  IMAGE_NULL			0

#define  IMAGE_10               1		//10
#define  IMAGE_J                2		//J
#define  IMAGE_Q                3		//Q
#define  IMAGE_K                4		//K
#define  IMAGE_A                5		//A
#define  IMAGE_Jersey           6		//球衣
#define  IMAGE_Gloves           7		//手套
#define  IMAGE_Stopwatch        8		//秒表
#define  IMAGE_GoldenBoots		9		//金靴
#define  IMAGE_GoldenBall		10		//金球
#define  IMAGE_GoldenCup		11      //金杯
#define	 IMAGE_FootBall         12		//可变足球

#define	 FRUIT_MAX              13

#define	 LINE_MAX	25
//行
#define Table_Row_Num	3
//列
#define	Table_Col_Num	5

#define GAME_PLAYER								1								//游戏人数

//范围 1-N 平均分布
static int Random(int start, int end)
{
    return (int)(((double)rand()) / RAND_MAX * (end - start) + start);
}

class GameLogic
{
public:
    GameLogic();
    virtual ~GameLogic();
    //计算倍数	返回倍数
    float Multi_Line(int & PrizeFruit, int & PrizeNum, int Table[], int LightTable[], int TableCount, int LineNumber,int winArr[][5],float lineMut[]);
    //是否相同
    int IsSame(int First, int Second);
    //随机
    int RandomFruitArea(int nLen,int32_t nArray[]);
    //创建一个不中的数组
    void RandNoneArray(int32_t Table[], int Size);
    int64_t SumArray(int num, int64_t * Array);

private:
    float Fruit_Multi[FRUIT_MAX][Table_Col_Num];
    int Fruit_Line[LINE_MAX][Table_Col_Num];
};

#endif // GAMELOGIC_H
