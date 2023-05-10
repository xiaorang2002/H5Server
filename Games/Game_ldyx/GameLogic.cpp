#include "GameLogic.h"
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include "math.h"
#include <glog/logging.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))


//不中奖的出法


GameLogic::GameLogic()
{

}

GameLogic::~GameLogic()
{

}

//彩池百分比  水果盘 数组的大小  第几条线
int GameLogic::Multi_Line(int& PrizeFruitRight, int& PrizeNumRight, int& multiRight,int& PrizeFruitLeft, int& PrizeNumLeft,int& multiLeft,int& marryNum,int Table[], int LightTable[], int TableCount, int LineNumber)
{
    return 0;
}


int	GameLogic::IsSame(int First, int Second)
{   
    return 0;
}


//随机区域
int GameLogic::RandomFruitArea(int nLen,int32_t nArray[])
{
    int nIndex = 0;
    int nACTotal = 0;
    for (int i = 0; i < nLen; i++)
    {
        nACTotal += nArray[i]; 
        // LOG(WARNING) <<  nArray[i]; 
    }
 
    assert(nACTotal > 0);

    if (nACTotal > 0)
    {
        int nRandNum = Random(0, nACTotal - 1);
        for (size_t i = 0; i < nLen; i++)
        {
            nRandNum -= nArray[i];
            if (nRandNum < 0)
            {
                nIndex = i;
                break;
            }
        }
    }
    else
    {
        nIndex = Random(0, nLen - 1);//rand() % nLen;
    }
    
    //水果下标从1开始
    nIndex = nIndex + 1;

#if DEBUG_INFO
    LOG(WARNING) << "nACTotal: " <<nACTotal <<" nLen:"<< nLen << " nIndex:" << nIndex ;
#endif

    return nIndex;
}


//创建一个不中奖的出法
void GameLogic::RandNoneArray(int32_t Table[], int Size)
{

}

int64_t	GameLogic::SumArray(int num, int64_t * Array) {
    if (num > 0 && Array != NULL)
    {
        int64_t  allsum = 0L;
        for (int i = 0; i < num; i++)
        {
            allsum += Array[i];
        }
        return allsum;
    }

    return 0;
}
