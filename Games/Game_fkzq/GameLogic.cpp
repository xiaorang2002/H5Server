#include "GameLogic.h"
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include "math.h"
#include <glog/logging.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))


//不中奖的出法
static int NoneTable[10][Table_Col_Num*Table_Row_Num] = {
    {
        IMAGE_A,		IMAGE_10,		IMAGE_K,       IMAGE_K,		IMAGE_J,
        IMAGE_Q,		IMAGE_10,		IMAGE_K,		IMAGE_K,		IMAGE_A,
        IMAGE_GoldenBoots,		IMAGE_10,		IMAGE_K,		IMAGE_10,		IMAGE_10,
    },
    {
        IMAGE_K,		IMAGE_K,		IMAGE_Jersey,       IMAGE_10,		IMAGE_K,
        IMAGE_10,		IMAGE_A,		IMAGE_GoldenBoots,		IMAGE_J,		IMAGE_Q,
        IMAGE_K,		IMAGE_Q,		IMAGE_10,		IMAGE_K,		IMAGE_A,
    },
    {
        IMAGE_10,		IMAGE_A,		IMAGE_10,       IMAGE_10,		IMAGE_Gloves,
        IMAGE_Q,		IMAGE_J,		IMAGE_10,		IMAGE_J,		IMAGE_Q,
        IMAGE_A,		IMAGE_K,		IMAGE_K,		IMAGE_A,		IMAGE_Gloves,
    },
    {
        IMAGE_J,		IMAGE_Gloves,		IMAGE_K,       IMAGE_K,		IMAGE_Stopwatch,
        IMAGE_Gloves,		IMAGE_10,		IMAGE_GoldenBoots,		IMAGE_10,		IMAGE_Jersey,
        IMAGE_Stopwatch,		IMAGE_10,		IMAGE_J,		IMAGE_10,		IMAGE_A,
    },
    {
        IMAGE_GoldenCup,		IMAGE_Q,		IMAGE_Q,       IMAGE_J,		IMAGE_K,
        IMAGE_Jersey,		IMAGE_10,		IMAGE_K,		IMAGE_A,		IMAGE_J,
        IMAGE_GoldenBall,		IMAGE_10,		IMAGE_A,		IMAGE_J,		IMAGE_Q,
    },
    {
        IMAGE_Q,		IMAGE_10,		IMAGE_10,       IMAGE_A,		IMAGE_A,
        IMAGE_A,		IMAGE_10,		IMAGE_Stopwatch,		IMAGE_A,		IMAGE_Jersey,
        IMAGE_Q,		IMAGE_GoldenBall,		IMAGE_Gloves,		IMAGE_A,		IMAGE_GoldenBall,
    },
    {
        IMAGE_A,		IMAGE_K,		IMAGE_Q,       IMAGE_Q,		IMAGE_J,
        IMAGE_GoldenBoots,		IMAGE_Q,		IMAGE_GoldenBall,		IMAGE_GoldenBoots,		IMAGE_Q,
        IMAGE_J,		IMAGE_J,		IMAGE_10,		IMAGE_A,		IMAGE_10,
    },
    {
        IMAGE_10,		IMAGE_J,		IMAGE_J,       IMAGE_Gloves,		IMAGE_Q,
        IMAGE_10,		IMAGE_GoldenBall,		IMAGE_A,		IMAGE_Stopwatch,		IMAGE_10,
        IMAGE_10,		IMAGE_GoldenBall,		IMAGE_K,		IMAGE_Gloves,		IMAGE_GoldenCup,
    },
    {
        IMAGE_10,		IMAGE_10,		IMAGE_GoldenCup,       IMAGE_10,		IMAGE_A,
        IMAGE_GoldenBall,		IMAGE_Gloves,		IMAGE_Q,		IMAGE_A,		IMAGE_10,
        IMAGE_GoldenBall,		IMAGE_Stopwatch,		IMAGE_GoldenBall,		IMAGE_10,		IMAGE_K,
    },
    {
        IMAGE_J,		IMAGE_K,		IMAGE_GoldenCup,       IMAGE_A,		IMAGE_K,
        IMAGE_Q,		IMAGE_10,		IMAGE_A,		IMAGE_10,		IMAGE_10,
        IMAGE_GoldenBoots,		IMAGE_10,		IMAGE_Stopwatch,		IMAGE_Q,		IMAGE_K,
    },
};

GameLogic::GameLogic()
{
    //9线位置
    int __Fruit_Line[LINE_MAX][Table_Col_Num] =
    {
        { 5,	6,	7,	8,	9 },
        { 0,	1,	2,	3,	4 },
        { 10,	11,	12,	13,	14 },
        { 0,	6,	12, 8,	4 },
        { 10,	6,	2,	8,	14 },
        { 0,	6,	2,	8,	4 },
        { 10,	6,	12,	8,	14 },
        { 5,	1,	7,	3,	9 },
        { 5,	11,	7,	13,	9 },
        { 0,	6,	7,	8,	4 },
        { 10,	6,	7,	8,	14 },
        { 5,	1,	2,	3,	9 },
        { 5,	11,	12,	13,	9 },
        { 5,	6,	2,	8,	9 },
        { 5,	6,	12,	8,	9 },
        { 0,	1,	7,	3,	4 },
        { 10,	11,	7,	13,	14 },
        { 0,	11,	12,	13,	4 },
        { 10,	1,	2,	3,	14 },
        { 10,	11,	2,	13,	14 },
        { 0,	1,	12,	3,	4 },
        { 5,	1,	12,	3,	9 },
        { 5,	11,	2,	13,	9 },
        { 0,	11,	7,	13,	4 },
        { 10,	1,	7,	3,	14 }
    };
    float __Fruit_Multi[FRUIT_MAX][Table_Col_Num] =
    {
        { 0,	0,		0,		0,		0 },		//0	 NULL
        { 0,	0,		12.5,	25,		50 },		//1  IMAGE_10
        { 0,	0,		12.5,	25,		50 },		//2  IMAGE_J
        { 0,	0,		12.5,	25,		62.5 },		//3  IMAGE_Q
        { 0,	0,		12.5,	25,		62.5 },		//4  IMAGE_K
        { 0,	0,		12.5,	25,		75 },		//5  IMAGE_A
        { 0,	0,		25,		50,		75 },		//6  IMAGE_yifu
        { 0,	0,		25,		62.5,	150 },		//7  IMAGE_shoutao
        { 0,	0,		25,		75,		200 },		//8  IMAGE_miaobiao
        { 0,	0,		37.5,	150,	250 },		//9  IMAGE_jinxue
        { 0,	0,		37.5,	150,	250 },		//10 IMAGE_jinqiu
        { 0,	0,		62.5,	250,	937.5 },	//11 IMAGE_jinbei
        { 0,	0,		0,		0,		0 }        //12 IMAGE_zuqiu = wild
    };

    memcpy(Fruit_Line, __Fruit_Line, sizeof(Fruit_Line));
    memcpy(Fruit_Multi, __Fruit_Multi, sizeof(Fruit_Multi));
}

GameLogic::~GameLogic()
{

}

//彩池百分比  水果盘 数组的大小  第几条线
float GameLogic::Multi_Line(int& PrizeFruit, int& PrizeNum, int Table[], int LightTable[], int TableCount, int LineNumber,int winArr[][5],float lineMut[])
{
    assert(LineNumber < LINE_MAX);
	//校验
	if (TableCount != Table_Col_Num * Table_Row_Num || Table == NULL)
	{
		assert(false);
		return 0;
	}
	for (int i = 0; i < 15; i++)
	{
		assert(Table[i] > 0 && Table[i] < FRUIT_MAX + 1);
	}

	int Fruit[Table_Col_Num];
	memset(Fruit, -1, sizeof(Fruit));

	int SameNum = 0;	//相同的水果个数
	int SameFruit = 0;

	
	for (int i = 0; i < Table_Col_Num; i++)
	{
		Fruit[i] = Table[Fruit_Line[LineNumber][i]];
	}
	if (Fruit[0] == Fruit[1] &&
		Fruit[1] == Fruit[2] &&
		Fruit[2] == Fruit[3] &&
		Fruit[3] == Fruit[4])
	{
        if (Fruit[0] == IMAGE_FootBall)
		{
            PrizeFruit = IMAGE_FootBall;
			PrizeNum = 4;
			LightTable[Fruit_Line[LineNumber][0]] = 1;
			LightTable[Fruit_Line[LineNumber][1]] = 1;
			LightTable[Fruit_Line[LineNumber][2]] = 1;
			LightTable[Fruit_Line[LineNumber][3]] = 1;
			LightTable[Fruit_Line[LineNumber][4]] = 1;
            winArr[LineNumber][0] = Fruit_Line[LineNumber][0];
            winArr[LineNumber][1] = Fruit_Line[LineNumber][1];
            winArr[LineNumber][2] = Fruit_Line[LineNumber][2];
            winArr[LineNumber][3] = Fruit_Line[LineNumber][3];
            winArr[LineNumber][4] = Fruit_Line[LineNumber][4];
            lineMut[LineNumber] = Fruit_Multi[IMAGE_FootBall][4];
            return Fruit_Multi[IMAGE_FootBall][4];
		}
	}

	for (int i = 0; i < Table_Col_Num - 1; i++)
	{
		int CurrentFruit = Table[Fruit_Line[LineNumber][i]];
		int Next = Table[Fruit_Line[LineNumber][i + 1]];
		int FruitCode = IsSame(CurrentFruit, Next);
		//相等
		if (FruitCode != 0)
		{
			if (SameFruit == 0)
			{
                if (FruitCode != IMAGE_FootBall)
				{
					SameFruit = FruitCode;
				}
				SameNum++;
			}
			else {
				if (IsSame(FruitCode, SameFruit) != 0)
				{
					SameNum++;
				}
				else {
					break;
				}
			}
		}
		else {
			break;
		}
	}
	if (SameFruit <= 0 || SameFruit >= FRUIT_MAX)
	{
		return 0;
	}
	if (SameNum > 5 || SameNum < 0)
	{
		return 0;
	}
    float Multi = Fruit_Multi[SameFruit][SameNum];
    if (Multi > 0 || SameFruit == IMAGE_FootBall)
	{
		PrizeFruit = SameFruit;
		PrizeNum = SameNum <= 0 ? 0 : SameNum;
		if (SameNum <= 1) SameNum = 0;
		if (SameNum>1)
		{
			for (int Index = 0; Index < min(SameNum + 1, 5); Index++)
			{
				LightTable[Fruit_Line[LineNumber][Index]] = 1;
                winArr[LineNumber][Index] = Fruit_Line[LineNumber][Index];
			}
		}

	}
    lineMut[LineNumber] = Multi;
	return Multi;
}


int	GameLogic::IsSame(int First, int Second)
{
    assert(First < FRUIT_MAX + 1 && First >= 0);
    assert(Second < FRUIT_MAX + 1 && Second >= 0);
    if (First == Second)
    {
        return First;
    }
    if (First == IMAGE_FootBall)
    {
        return Second;
    }
    if (Second == IMAGE_FootBall )
    {
        return First;
    }
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
