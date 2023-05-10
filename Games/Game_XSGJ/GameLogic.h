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

#define  IMAGE_CHERRY		1		//樱桃
#define  IMAGE_STRAWBERRY	2		//草莓
#define  IMAGE_ORANGE		3		//橘子
#define  IMAGE_LEMON		4		//柠檬
#define  IMAGE_GRAPE		5		//葡萄
#define  IMAGE_WATERMELON	6		//西瓜
#define  IMAGE_BELL			7		//铃铛
#define  IMAGE_STAR			8		//星星
#define  IMAGE_SEVEN		9		//七
#define  IMAGE_WILD			10		//WILD
#define  IMAGE_SCATTER		11		//SCATTER
#define	 IMAGE_BONUS		12		//BONUS

#define	 FRUIT_MAX			13

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
