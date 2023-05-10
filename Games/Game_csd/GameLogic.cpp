#include "GameLogic.h"
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include "math.h"
#include <glog/logging.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))


//不中奖的出法
static int NoneTable[10][Table_Col_Num*Table_Row_Num] =
{
    {4  ,4 , 3 , 1  ,2 ,2 , 9 , 1  ,0  ,4 ,2 , 4  ,5  ,4  ,2},
    {4  ,1 , 5 , 6  ,1 ,2 , 2 , 0  ,4  ,5 ,4 , 5  ,1  ,5  ,5},
    {1  ,0 , 9 , 4  ,4 ,0 , 1 , 2  ,3  ,6 ,3 , 2  ,3  ,3  ,1},
    {0  ,3 , 4 , 4  ,1 ,2 , 9 , 3  ,8  ,1 ,8 , 1  ,1  ,2  ,9},
    {3  ,6 , 3 , 2  ,2 ,4 , 5 , 1  ,0  ,1 ,4 , 2  ,0  ,0  ,4},
    {0  ,3 , 0 , 9  ,4 ,2 , 1 , 0  ,4  ,5 ,4 , 2  ,3  ,2  ,1},
    {2  ,0 , 6 , 0  ,2 ,0 , 4 , 9  ,5  ,1 ,3 , 4  ,4  ,3  ,3},
    {1  ,4 , 2 , 0  ,3 ,3 , 7 , 2  ,4  ,1 ,0 , 0  ,1  ,8  ,1},
    {3  ,2 , 1 , 4  ,1 ,1 , 4 , 3  ,0  ,3 ,1 , 2  ,6  ,4  ,1},
    {5  ,1 , 2 , 2  ,10 ,6 , 2 , 1  ,1  ,9 ,3 , 9  ,7  ,4  ,2}
};

GameLogic::GameLogic()
{
}

GameLogic::~GameLogic()
{

}

//彩池百分比  水果盘 数组的大小  第几条线
int GameLogic::Multi_Line(int& PrizeFruit, int& PrizeNum, int Table[], int LightTable[], int TableCount, int LineNumber)
{

    return 0;
}


int	GameLogic::IsSame(int First, int Second)
{

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
    //assert(Table == nullptr);
    int noneindex = rand() % 10;
    for (int Index = 0; Index < 15; Index++)
    {
        Table[Index] = NoneTable[noneindex][Index];
    }
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
