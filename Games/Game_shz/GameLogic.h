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

#define  IMAGE_AX           1		//斧头
#define  IMAGE_TASSEL	    2		//红缨枪
#define  IMAGE_DAGGER		3		//匕首
#define  IMAGE_ZHISHENLU	4		//鲁智深
#define  IMAGE_CHONGLIN		5		//林冲
#define  IMAGE_JIANGSONG	6		//宋江
#define  IMAGE_KILLFORGOD	7		//替天行道
#define  IMAGE_LOYALTYHAUSE	8		//忠义堂
#define  IMAGE_LEGEND		9		//水浒传---可以变

#define	 FRUIT_MAX			10

#define	 LINE_MAX	9
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
    int Multi_Line(int& PrizeFruitRight, int& PrizeNumRight, int& multiRight,int& PrizeFruitLeft, int& PrizeNumLeft,int& multiLeft, int& marryNum,int Table[], int LightTable[], int TableCount, int LineNumber);
    //是否相同
    int IsSame(int First, int Second);
    //随机
    int RandomFruitArea(int nLen,int32_t nArray[]);
    //创建一个不中的数组
    void RandNoneArray(int32_t Table[], int Size);
    int64_t SumArray(int num, int64_t * Array);

private:
    int Fruit_Multi[FRUIT_MAX][Table_Col_Num];
    int Fruit_Line[LINE_MAX][Table_Col_Num];
};

#endif // GAMELOGIC_H
