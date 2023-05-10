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
        IMAGE_CHERRY,		IMAGE_CHERRY,		IMAGE_STRAWBERRY,	IMAGE_WILD,			IMAGE_SEVEN,
        IMAGE_SEVEN,		IMAGE_BELL,			IMAGE_LEMON,		IMAGE_GRAPE,		IMAGE_BELL,
        IMAGE_SEVEN,		IMAGE_ORANGE,		IMAGE_BONUS,		IMAGE_BELL,			IMAGE_GRAPE,
    },
    {
        IMAGE_STAR,			IMAGE_STAR,			IMAGE_ORANGE,		IMAGE_CHERRY,		IMAGE_STRAWBERRY,
        IMAGE_LEMON,		IMAGE_GRAPE,		IMAGE_ORANGE,		IMAGE_WATERMELON,	IMAGE_ORANGE,
        IMAGE_GRAPE,		IMAGE_LEMON,		IMAGE_SEVEN,		IMAGE_STAR,			IMAGE_BELL,
    },
    {
        IMAGE_STRAWBERRY,	IMAGE_WATERMELON,	IMAGE_WATERMELON,	IMAGE_SEVEN,		IMAGE_GRAPE,
        IMAGE_STAR,			IMAGE_SEVEN,		IMAGE_GRAPE,		IMAGE_LEMON,		IMAGE_WILD,
        IMAGE_WATERMELON,	IMAGE_GRAPE,		IMAGE_BELL,			IMAGE_STAR,			IMAGE_CHERRY,
    },
    {
        IMAGE_BELL,			IMAGE_STAR,			IMAGE_CHERRY,		IMAGE_LEMON,		IMAGE_STRAWBERRY,
        IMAGE_WATERMELON,	IMAGE_STAR,			IMAGE_GRAPE,		IMAGE_SEVEN,		IMAGE_ORANGE,
        IMAGE_GRAPE,		IMAGE_CHERRY,		IMAGE_ORANGE,		IMAGE_GRAPE,		IMAGE_WATERMELON,
    },
    {
        IMAGE_ORANGE,		IMAGE_GRAPE,		IMAGE_STAR,			IMAGE_CHERRY,		IMAGE_ORANGE,
        IMAGE_ORANGE,		IMAGE_WATERMELON,	IMAGE_GRAPE,		IMAGE_LEMON,		IMAGE_ORANGE,
        IMAGE_ORANGE,		IMAGE_CHERRY,		IMAGE_STRAWBERRY,	IMAGE_LEMON,		IMAGE_SEVEN,
    },
    {
        IMAGE_WATERMELON,	IMAGE_SEVEN,		IMAGE_STRAWBERRY,	IMAGE_LEMON,		IMAGE_ORANGE,
        IMAGE_ORANGE,		IMAGE_CHERRY,		IMAGE_STAR,			IMAGE_LEMON,		IMAGE_ORANGE,
        IMAGE_LEMON,		IMAGE_CHERRY,		IMAGE_BELL,			IMAGE_STAR,			IMAGE_ORANGE,
    },
    {
        IMAGE_SEVEN,		IMAGE_SEVEN,		IMAGE_CHERRY,		IMAGE_CHERRY,		IMAGE_STRAWBERRY,
        IMAGE_ORANGE,		IMAGE_STAR,			IMAGE_WATERMELON,	IMAGE_ORANGE,		IMAGE_STRAWBERRY,
        IMAGE_LEMON,		IMAGE_GRAPE,		IMAGE_STAR,			IMAGE_BELL,			IMAGE_STAR,
    },
    {
        IMAGE_GRAPE,		IMAGE_WATERMELON,	IMAGE_BELL,			IMAGE_ORANGE,		IMAGE_SEVEN,
        IMAGE_ORANGE,		IMAGE_CHERRY,		IMAGE_STAR,			IMAGE_LEMON,		IMAGE_ORANGE,
        IMAGE_LEMON,		IMAGE_GRAPE,		IMAGE_STAR,			IMAGE_BELL,			IMAGE_STAR,
    },
    {
        IMAGE_STAR,			IMAGE_WATERMELON,	IMAGE_BELL,			IMAGE_STRAWBERRY,	IMAGE_ORANGE,
        IMAGE_SEVEN,		IMAGE_SEVEN,		IMAGE_WATERMELON,	IMAGE_BELL,			IMAGE_STRAWBERRY,
        IMAGE_STAR,			IMAGE_CHERRY,		IMAGE_ORANGE,		IMAGE_SEVEN,		IMAGE_BELL,
    },
    {
        IMAGE_SEVEN,		IMAGE_SEVEN,		IMAGE_CHERRY,		IMAGE_CHERRY,		IMAGE_STRAWBERRY,
        IMAGE_ORANGE,		IMAGE_CHERRY,		IMAGE_STAR,			IMAGE_LEMON,		IMAGE_ORANGE,
        IMAGE_STAR,			IMAGE_CHERRY,		IMAGE_ORANGE,		IMAGE_SEVEN,		IMAGE_BELL,
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
        { 0,	1,	7,	13,	14 },
        { 10,	11,	7,	3,	4 },
        { 5,	1,	7,	13,	9 },
        { 5,	11,	7,	3,	9 }
    };
    int __Fruit_Multi[FRUIT_MAX][Table_Col_Num] =
    {
        { 0,	0,		0,		0,		0 },		//0	 NULL
        { 0,	0,		3,		10,		50 },		//1  IMAGE_CHERRY
        { 0,	0,		4,		15,		75 },		//2  IMAGE_STRAWBERRY
        { 0,	0,		5,		20,		100 },		//3  IMAGE_ORANGE
        { 0,	0,		6,		25,		150 },		//4  IMAGE_LEMON
        { 0,	0,		7,		30,		200 },		//5  IMAGE_GRAPE
        { 0,	0,		8,		40,		300 },		//6  IMAGE_WATERMELON
        { 0,	0,		10,		50,		400 },		//7  IMAGE_BELL
        { 0,	0,		15,		75,		500 },		//8  IMAGE_STAR
        { 0,	0,		20,		100,	800 },		//9  IMAGE_SEVEN
        { 0,	0,		0,		0,		1000 },		//10 IMAGE_WILD
        { 0,	0,		0,		0,		0 },		//11 IMAGE_SCATTER
        { 0,	0,		0,		0,		0 }			//12 IMAGE_BONUS
    };

    memcpy(Fruit_Line, __Fruit_Line, sizeof(Fruit_Line));
    memcpy(Fruit_Multi, __Fruit_Multi, sizeof(Fruit_Multi));
}

GameLogic::~GameLogic()
{

}

//彩池百分比  水果盘 数组的大小  第几条线
int GameLogic::Multi_Line(int& PrizeFruit, int& PrizeNum, int Table[], int LightTable[], int TableCount, int LineNumber)
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
	int SameFruitArray[Table_Col_Num];

	memset(SameFruitArray, -1, sizeof(SameFruitArray));
	
	for (int i = 0; i < Table_Col_Num; i++)
	{
		Fruit[i] = Table[Fruit_Line[LineNumber][i]];
	}
	if (Fruit[0] == Fruit[1] &&
		Fruit[1] == Fruit[2] &&
		Fruit[2] == Fruit[3] &&
		Fruit[3] == Fruit[4])
	{
		if (Fruit[0] == IMAGE_WILD)
		{
			PrizeFruit = IMAGE_WILD;
			PrizeNum = 4;
			LightTable[Fruit_Line[LineNumber][0]] = 1;
			LightTable[Fruit_Line[LineNumber][1]] = 1;
			LightTable[Fruit_Line[LineNumber][2]] = 1;
			LightTable[Fruit_Line[LineNumber][3]] = 1;
			LightTable[Fruit_Line[LineNumber][4]] = 1;
			return Fruit_Multi[IMAGE_WILD][4];
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
				if (FruitCode != IMAGE_WILD)
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
	int Multi = Fruit_Multi[SameFruit][SameNum];
	if (Multi > 0 || SameFruit == IMAGE_BONUS || SameFruit == IMAGE_SCATTER)
	{
		PrizeFruit = SameFruit;
		PrizeNum = SameNum <= 0 ? 0 : SameNum;
		if (SameNum <= 1) SameNum = 0;
		if (SameNum>1)
		{
			for (int Index = 0; Index < min(SameNum + 1, 5); Index++)
			{
				LightTable[Fruit_Line[LineNumber][Index]] = 1;
			}
		}

	}
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
    if (First == IMAGE_WILD && Second != IMAGE_SCATTER && Second != IMAGE_BONUS)
    {
        return Second;
    }
    if (Second == IMAGE_WILD && First != IMAGE_SCATTER && First != IMAGE_BONUS)
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
