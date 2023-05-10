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
//冰糖葫芦,包子,仙桃,玉如意，福袋，金鱼，财主，弥勒佛，财神，金锣，财神到
#define  IMAGE_BingTang 	0		//冰糖葫芦
#define  IMAGE_BaoZi        1		//包子
#define  IMAGE_XianTao		2		//仙桃
#define  IMAGE_RuYi         3		//玉如意
#define  IMAGE_FuDai		4		//福袋
#define  IMAGE_JinYu        5		//金鱼
#define  IMAGE_CaiZhu		6		//财主
#define  IMAGE_MiLe			7		//弥勒佛
#define  IMAGE_CaiShen		8		//财神
#define  IMAGE_JinLuo		9		//金锣
#define  IMAGE_CaiShenDao	10		//财神到
#define	 FRUIT_MAX			11

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
    int Multi_Line(int & PrizeFruit, int & PrizeNum, int Table[], int LightTable[], int TableCount, int LineNumber);
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
