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
        IMAGE_AX,           IMAGE_AX,		    IMAGE_TASSEL,       IMAGE_LEGEND,		IMAGE_AX,
        IMAGE_LEGEND,		IMAGE_KILLFORGOD,   IMAGE_ZHISHENLU,	IMAGE_CHONGLIN,		IMAGE_KILLFORGOD,
        IMAGE_LEGEND,		IMAGE_DAGGER,		IMAGE_LEGEND,		IMAGE_KILLFORGOD,	IMAGE_CHONGLIN,
    },
    {
        IMAGE_LOYALTYHAUSE,		IMAGE_LOYALTYHAUSE,		IMAGE_DAGGER,		IMAGE_AX,           IMAGE_TASSEL,
        IMAGE_ZHISHENLU,		IMAGE_CHONGLIN,         IMAGE_DAGGER,		IMAGE_JIANGSONG,	IMAGE_DAGGER,
        IMAGE_CHONGLIN,         IMAGE_ZHISHENLU,		IMAGE_AX,           IMAGE_LOYALTYHAUSE,	IMAGE_KILLFORGOD,
    },
    {
        IMAGE_TASSEL,       IMAGE_JIANGSONG,	IMAGE_JIANGSONG,	IMAGE_LEGEND,               IMAGE_CHONGLIN,
        IMAGE_LOYALTYHAUSE,	IMAGE_AX,           IMAGE_CHONGLIN,		IMAGE_ZHISHENLU,            IMAGE_LEGEND,
        IMAGE_JIANGSONG,	IMAGE_CHONGLIN,		IMAGE_KILLFORGOD,	IMAGE_LOYALTYHAUSE,			IMAGE_AX,
    },
    {
        IMAGE_KILLFORGOD,	IMAGE_LOYALTYHAUSE,			IMAGE_AX,           IMAGE_ZHISHENLU,	IMAGE_TASSEL,
        IMAGE_JIANGSONG,	IMAGE_LOYALTYHAUSE,			IMAGE_CHONGLIN,		IMAGE_LEGEND,		IMAGE_DAGGER,
        IMAGE_CHONGLIN,		IMAGE_AX,                   IMAGE_DAGGER,		IMAGE_CHONGLIN,		IMAGE_JIANGSONG,
    },
    {
        IMAGE_DAGGER,		IMAGE_CHONGLIN,		IMAGE_LOYALTYHAUSE,	IMAGE_AX,               IMAGE_DAGGER,
        IMAGE_DAGGER,		IMAGE_JIANGSONG,	IMAGE_CHONGLIN,		IMAGE_ZHISHENLU,		IMAGE_DAGGER,
        IMAGE_DAGGER,		IMAGE_AX,           IMAGE_TASSEL,       IMAGE_ZHISHENLU,		IMAGE_LEGEND,
    },
    {
        IMAGE_JIANGSONG,	IMAGE_LEGEND,		IMAGE_TASSEL,               IMAGE_ZHISHENLU,		IMAGE_DAGGER,
        IMAGE_DAGGER,		IMAGE_AX,           IMAGE_LOYALTYHAUSE,			IMAGE_ZHISHENLU,		IMAGE_DAGGER,
        IMAGE_ZHISHENLU,	IMAGE_AX,           IMAGE_KILLFORGOD,			IMAGE_LOYALTYHAUSE,		IMAGE_DAGGER,
    },
    {
        IMAGE_LEGEND,		IMAGE_KILLFORGOD,	IMAGE_AX,           IMAGE_AX,           IMAGE_TASSEL,
        IMAGE_DAGGER,		IMAGE_LOYALTYHAUSE,	IMAGE_JIANGSONG,	IMAGE_DAGGER,		IMAGE_TASSEL,
        IMAGE_ZHISHENLU,	IMAGE_CHONGLIN,		IMAGE_LOYALTYHAUSE,	IMAGE_KILLFORGOD,	IMAGE_LOYALTYHAUSE,
    },
    {
        IMAGE_CHONGLIN,		IMAGE_JIANGSONG,	IMAGE_KILLFORGOD,			IMAGE_DAGGER,           IMAGE_LEGEND,
        IMAGE_DAGGER,		IMAGE_AX,           IMAGE_LOYALTYHAUSE,			IMAGE_ZHISHENLU,		IMAGE_DAGGER,
        IMAGE_ZHISHENLU,	IMAGE_CHONGLIN,		IMAGE_LOYALTYHAUSE,			IMAGE_KILLFORGOD,		IMAGE_LOYALTYHAUSE,
    },
    {
        IMAGE_LOYALTYHAUSE,			IMAGE_JIANGSONG,	IMAGE_KILLFORGOD,	IMAGE_TASSEL,       IMAGE_DAGGER,
        IMAGE_LEGEND,               IMAGE_AX,           IMAGE_JIANGSONG,	IMAGE_KILLFORGOD,	IMAGE_TASSEL,
        IMAGE_LOYALTYHAUSE,			IMAGE_AX,           IMAGE_DAGGER,		IMAGE_LEGEND,		IMAGE_KILLFORGOD,
    },
    {
        IMAGE_LEGEND,		IMAGE_CHONGLIN,	IMAGE_AX,           IMAGE_AX,           IMAGE_TASSEL,
        IMAGE_DAGGER,		IMAGE_AX,		IMAGE_LOYALTYHAUSE,	IMAGE_ZHISHENLU,	IMAGE_DAGGER,
        IMAGE_LOYALTYHAUSE,	IMAGE_AX,		IMAGE_DAGGER,		IMAGE_CHONGLIN,		IMAGE_KILLFORGOD,
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
        { 0,	1,	7,	3,	4 },
        { 10,	11,	7,	13,	14 },
        {5,	11,	12,	13,	9  },
        {5,	1,	2,	3,	9  }
    };
    int __Fruit_Multi_L[FRUIT_MAX][Table_Col_Num] =
    {
        { 0,	0,		0,		0,		0 },		//0	 NULL
        { 0,	0,		2,		5,		20 },		//1  IMAGE_AX
        { 0,	0,		3,		10,		40 },		//2  IMAGE_TASSEL
        { 0,	0,		5,		15,		60 },		//3  IMAGE_DAGGER
        { 0,	0,		7,		20,		100 },		//4  IMAGE_ZHISHENLU
        { 0,	0,		10,		30,		160 },		//5  IMAGE_CHONGLIN
        { 0,	0,		15,		40,		200 },		//6  IMAGE_JIANGSONG
        { 0,	0,		20,		80,		400 },		//7  IMAGE_KILLFORGOD
        { 0,	0,		50,		200,	1000 },		//8  IMAGE_LOYALTYHAUSE
        { 0,	0,		0,		0,      2000 }		//9  IMAGE_LEGEND
    };
    memcpy(Fruit_Line, __Fruit_Line, sizeof(Fruit_Line));
    memcpy(Fruit_Multi, __Fruit_Multi_L, sizeof(Fruit_Multi));
}

GameLogic::~GameLogic()
{

}

//彩池百分比  水果盘 数组的大小  第几条线
int GameLogic::Multi_Line(int& PrizeFruitRight, int& PrizeNumRight, int& multiRight,int& PrizeFruitLeft, int& PrizeNumLeft,int& multiLeft,int& marryNum,int Table[], int LightTable[], int TableCount, int LineNumber)
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

    int SameNumRight = 0;	//左边相同的水果个数
    int SameFruitRight = 0;

    int SameNumLeft = 0;	//右边相同的水果个数
    int SameFruitLeft = 0;
	
	for (int i = 0; i < Table_Col_Num; i++)
	{
		Fruit[i] = Table[Fruit_Line[LineNumber][i]];
    }
	if (Fruit[0] == Fruit[1] &&
		Fruit[1] == Fruit[2] &&
		Fruit[2] == Fruit[3] &&
		Fruit[3] == Fruit[4])
	{
        if (Fruit[0] == IMAGE_LEGEND)
		{
            PrizeFruitLeft = IMAGE_LEGEND;
            PrizeFruitRight = IMAGE_NULL;
            PrizeNumLeft = 4;
            PrizeNumRight = 0;
            marryNum=3;
			LightTable[Fruit_Line[LineNumber][0]] = 1;
			LightTable[Fruit_Line[LineNumber][1]] = 1;
			LightTable[Fruit_Line[LineNumber][2]] = 1;
			LightTable[Fruit_Line[LineNumber][3]] = 1;
			LightTable[Fruit_Line[LineNumber][4]] = 1;

            multiLeft = Fruit_Multi[IMAGE_LEGEND][4];
            multiRight  = 0;
            return Fruit_Multi[IMAGE_LEGEND][4];
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
            if (SameFruitLeft == 0)
			{
                if (FruitCode != IMAGE_LEGEND)
				{
                    SameFruitLeft = FruitCode;
				}
                SameNumLeft++;
			}
			else {
                if (IsSame(FruitCode, SameFruitLeft) != 0)
				{
                    SameNumLeft++;
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

    if(Fruit[0] == IMAGE_LEGEND
       &&Fruit[0]==Fruit[1]
       &&Fruit[1]==Fruit[2]
       &&Fruit[2]!=Fruit[3])
    {
        marryNum=1;
    }
    else if(Fruit[0] == IMAGE_LEGEND
            &&Fruit[0]==Fruit[1]
            &&Fruit[1]==Fruit[2]
            &&Fruit[2]==Fruit[3]
            &&Fruit[3]!=Fruit[4])
    {
        marryNum=2;
    }
    else if(Fruit[4] == IMAGE_LEGEND
            &&Fruit[4]==Fruit[3]
            &&Fruit[3]==Fruit[2]
            &&Fruit[2]!=Fruit[1])
    {
        marryNum=1;
    }
    else if(Fruit[4] == IMAGE_LEGEND
            &&Fruit[4]==Fruit[3]
            &&Fruit[3]==Fruit[2]
            &&Fruit[2]==Fruit[1]
            &&Fruit[1]!=Fruit[0])
    {
        marryNum=2;
    }
    multiLeft = Fruit_Multi[SameFruitLeft][SameNumLeft];

    if(multiLeft>0)
    {
        PrizeFruitLeft = SameFruitLeft;
        PrizeNumLeft = SameNumLeft;
        if (PrizeNumLeft>=2)
        {
            for (int Index = 0; Index <= PrizeNumLeft; Index++)
            {
                LightTable[Fruit_Line[LineNumber][Index]] = 1;
            }
        }
    }
    //顺序是三个的时候才可能反过来也有不一样的线成立
    //if(SameNumLeft<=3)
    {

        for (int i = Table_Col_Num - 1; i >=1 ; i--)
        {
            int CurrentFruit = Table[Fruit_Line[LineNumber][i]];
            int Next = Table[Fruit_Line[LineNumber][i - 1]];
            int FruitCode = IsSame(CurrentFruit, Next);
            //相等
            if (FruitCode != 0)
            {
                if (SameFruitRight == 0)
                {
                    if (FruitCode != IMAGE_LEGEND)
                    {
                        SameFruitRight = FruitCode;
                    }
                    SameNumRight++;
                }
                else {
                    if (IsSame(FruitCode, SameFruitRight) != 0)
                    {
                        SameNumRight++;
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
    }
    multiRight = Fruit_Multi[SameFruitRight][SameNumRight];

    if(multiRight>0)
    {
        PrizeFruitRight = SameFruitRight;
        PrizeNumRight = SameNumRight;
        if (PrizeNumRight>=2)
        {
            for (int Index = Table_Col_Num - 1; Index >=Table_Col_Num-1-PrizeNumRight; Index--)
            {
                LightTable[Fruit_Line[LineNumber][Index]] = 1;
            }
        }
    }
    return 0;
}


int	GameLogic::IsSame(int First, int Second)
{
    assert(First < FRUIT_MAX + 1 && First >= 0);
    assert(Second < FRUIT_MAX + 1 && Second >= 0);
    if (First == Second)
    {
        return First;
    }
    if (First == IMAGE_LEGEND )
    {
        return Second;
    }
    if (Second == IMAGE_LEGEND )
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
