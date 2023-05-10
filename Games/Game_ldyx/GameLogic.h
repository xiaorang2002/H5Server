#ifndef GAMELOGIC_SGJ_H
#define GAMELOGIC_SGJ_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <vector>

#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))
//水果代号

#define  IMAGE_BIG_APPLE           0		//大苹果
#define  IMAGE_SMA_APPLE           1		//小苹果
#define  IMAGE_BIG_ORANGE          2		//大橙子
#define  IMAGE_SMA_ORANGE          3		//小橙子
#define  IMAGE_BIG_MONGO           4		//大芒果
#define  IMAGE_SMA_MONGO           5		//小芒果
#define  IMAGE_BIG_BELLS           6		//大铃铛
#define  IMAGE_SMA_BELLS           7		//小铃铛
#define  IMAGE_BIG_WATERMELON      8		//大西瓜
#define  IMAGE_SMA_WATERMELON      9		//小西瓜
#define  IMAGE_BIG_STAR            10       //大星星
#define  IMAGE_SMA_STAR            11       //小星星
#define  IMAGE_BIG_SEVEN           12       //大七七
#define  IMAGE_SMA_SEVEN           13       //小七七
#define  IMAGE_BIG_BAR             14       //大BAR
#define  IMAGE_SMA_BAR             15       //小BAR

#define  IMAGE_BLUE_LUCKY          16       //蓝色 lucky
#define  IMAGE_YELLO_LUCKY         17       //黄色 lucky

#define	 FRUIT_MAX                 18

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

};

#endif // GAMELOGIC_H
