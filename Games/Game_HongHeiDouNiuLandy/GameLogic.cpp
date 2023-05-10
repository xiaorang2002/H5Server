#include "GameLogic.h"

#include <random>
#include <assert.h>
#include <cstring>
#include <algorithm>
//////////////////////////////////////////////////////////////////////////

//扑克数据
uint8_t CGameLogic::m_cbCardListData[52]=
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

};

//////////////////////////////////////////////////////////////////////////

//构造函数
CGameLogic::CGameLogic()
{
}

//析构函数
CGameLogic::~CGameLogic()
{
}

//逻辑数值
uint8_t CGameLogic::GetLogicValue(uint8_t cbCardData)
{
	//扑克属性
	uint8_t bCardValue = GetCardValue(cbCardData);

	//转换数值
	return (bCardValue > 10) ? (10) : bCardValue;
}
uint8_t CGameLogic::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount)
{
	assert(cbCardCount == MAX_COUNT);
	SortCardList(cbCardData, cbCardCount);
	//是否为五小牛
	if (IsFiveSmall(cbCardData))
	{
		return CT_FIVESMALL_NIU;
	}
	//是否为炸弹
	if (IsBomb(cbCardData))
	{
		return CT_BOMB_NIU;
	}

	//是否为五花牛
	if (IsWuHuaBull(cbCardData))
	{
		return CT_WUHUA_NIU;
	}

	//是否为银牛
	if (IsSiHuaBull(cbCardData))
	{
		return CT_SIHUA_NIU;
	}

	//牛1-10
	uint8_t bTemp[MAX_COUNT];
	uint8_t bSum = 0;
	for (uint8_t i = 0; i < cbCardCount; ++i)
	{
		bTemp[i] = GetLogicValue(cbCardData[i]);
		bSum += bTemp[i];
	}
	for (uint8_t i = 0; i < cbCardCount - 1; i++)
	{
		for (uint8_t j = i + 1; j < cbCardCount; j++)
		{
			if ((bSum - bTemp[i] - bTemp[j]) % 10 == 0)
			{
				return ((bTemp[i] + bTemp[j]) > 10) ? (bTemp[i] + bTemp[j] - 10) : (bTemp[i] + bTemp[j]);
			}
		}
	}

	return CT_NIU_NULL;	//无牛
}
//排列扑克
void CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount)
{
    std::sort(cbCardData,cbCardData+cbCardCount,[this](uint8_t a,uint8_t b){
        return GetCardLogicValue(a) > GetCardLogicValue(b);
    });

	return;
}

//混乱扑克
void CGameLogic::RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount)
{
	//CopyMemory(cbCardBuffer,m_cbCardListData,cbBufferCount);

	//混乱准备
    uint8_t cbCardData[sizeof(m_cbCardListData)];
    memcpy(cbCardData, m_cbCardListData,sizeof(m_cbCardListData));

	//混乱扑克
    uint8_t bRandCount=0,bPosition=0;
	do
	{
        bPosition=rand()%(sizeof(m_cbCardListData)-bRandCount);
		cbCardBuffer[bRandCount++]=cbCardData[bPosition];
        cbCardData[bPosition]=cbCardData[sizeof(m_cbCardListData)-bRandCount];
	} while (bRandCount<cbBufferCount);

	return;
}

//逻辑数值
uint8_t CGameLogic::GetCardLogicValue(uint8_t cbCardData)
{
	//扑克属性
    uint8_t bCardColor=GetCardColor(cbCardData);
    uint8_t bCardValue=GetCardValue(cbCardData);

	//转换数值
	return (bCardValue==1)?(bCardValue+13):bCardValue;
}

//对比扑克
uint8_t CGameLogic::CompareCard(uint8_t cbFirstData[], uint8_t cbNextData[], uint8_t cbCardCount)
{
	//设置变量
    uint8_t FirstData[MAX_COUNT],NextData[MAX_COUNT];
    memcpy(FirstData,cbFirstData,sizeof(FirstData));
    memcpy(NextData,cbNextData,sizeof(NextData));

	//大小排序
	SortCardList(FirstData,cbCardCount);
	SortCardList(NextData,cbCardCount);

	//获取类型
    uint8_t cbFirstType=GetCardType(FirstData,cbCardCount);
    uint8_t cbNextType=GetCardType(NextData,cbCardCount);

	//类型判断
	if (cbFirstType!=cbNextType) return (cbFirstType>cbNextType)?1:0;

	//牌型相同
	if (cbFirstType == CT_BOMB_NIU)
	{
		return (GetCardValue(FirstData[MAX_COUNT / 2]) > GetCardValue(NextData[MAX_COUNT / 2])) ? 1 : 0;
	}
    return DRAW;
}

long long CGameLogic::GetValue(uint8_t cbCardData[], uint8_t cbCardCount)
{
    uint8_t Level = GetCardType(cbCardData,cbCardCount);//返回类型
    uint8_t Value = GetCardLevelValue(cbCardData,cbCardCount, Level);
    int64_t Max = GetCardMaxValue(cbCardData, cbCardCount);//最大值
    long long u64Result = Level;
    u64Result = (u64Result << 8);
    u64Result |= Value;
    u64Result = (u64Result << 32);
    u64Result |= Max;
    return u64Result;
}

uint8_t CGameLogic::GetCardLevelValue(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t bLevel)
{
    //设置变量
    //大小排序
    SortCardList(cbCardData, cbCardCount);
    //简单类型
//    switch (bLevel)
//    {
//        case CT_SINGLE:		//单牌
//            return 0;                        //__GetCardMaxValue里面有大小
//        case CT_BAO_ZI:		//豹子
//        {
//            //return Poker::GetPokerValue(cbCardData[0]);//比最大的牌
//            return GetCardLogicValue(cbCardData[0]);
//        }
//        case CT_JIN_HUA:		//金花
//        {
//            return 0;// Poker::GetPokerType(cbCardData[0]); //比花色  先比大小 再比花色吧
//        }
//        case CT_SHUN_ZI:		//顺子
//        case CT_SHUN_JIN:		//顺金 432>A32
//        {
//            uint8_t cbNextValue = GetCardLogicValue(cbCardData[0]);
//            //特殊A32 最小的顺子
//            if (cbNextValue == 14 && GetCardLogicValue(cbCardData[2]) == 2)//为 A32的情况下
//            {
//                cbNextValue = 1;
//            }
//            else
//                cbNextValue = GetCardLogicValue(cbCardData[0]); //__GetCardPointValue(cbCardData[0]);
//            //对比数值
//            return cbNextValue;
//        }
//        case CT_DOUBLE:			//对子  如果对子大小一样还是要比花色吧
//        {
//            return  GetCardLogicValue(cbCardData[1]);
////            if (cbNextValue == GetCardLogicValue(cbCardData[1]))
////            {
//
////                //check the third card first ,when the third card equal value then check the double cards color
////                //return max double card(mask) and add the third card logic value(right move 5)
////                //(cbCardData[0]>cbCardData[1]?cbCardData[0]:cbCardData[1])+GetCardLogicValue(cbCardData[2])<<5 ;//
////                return GetCardLogicValue(cbCardData[1]);
////            }
////            else
////            {
////                //return __GetCardPointValue(cbCardData[1]);
////                return GetCardLogicValue(cbCardData[1]);
////            }
//        }
//    }
    return 0;
}

int64_t CGameLogic::GetCardMaxValue(uint8_t cbCardData[], uint8_t cbCardCount)
{
    SortCardList(cbCardData,cbCardCount);
    int64_t u16Value = 0;
    int64_t u16Type = 0;
    for(int64_t i = 0; i < 3; ++i)
    {
        uint8_t Value = GetCardLogicValue(cbCardData[i]);
        uint8_t Type = GetCardColor(cbCardData[i]);
        u16Value |= (Value << ((3 - i - 1) * 4));
        u16Type |= (Type << ((3 - i - 1) * 4));
    }
    int64_t u32TotalValue = u16Value;
    u32TotalValue = (u32TotalValue << 16);
    u32TotalValue += u16Type;
    return u32TotalValue;
}


//获取倍数
uint8_t CGameLogic::GetMultiple(uint8_t cbCardData[])
{
	uint8_t  cbCardType = GetCardType(cbCardData, MAX_COUNT);
	if (cbCardType >= CT_NIU_ONE && cbCardType <= CT_NIUNIU)return 6;
	else if (cbCardType != CT_NIUNIU)return 500;

	return 0;
}

//获取牛牛牌
bool CGameLogic::GetOxCard(uint8_t cbCardData[])
{
	//设置变量
	uint8_t cbTemp[MAX_COUNT] = { 0 };
	uint8_t cbTempData[MAX_COUNT] = { 0 };
	memcpy(cbTempData, cbCardData, sizeof(cbTempData));

	uint8_t bSum = 0;
	for (uint8_t i = 0; i < MAX_COUNT; ++i)
	{
		cbTemp[i] = GetLogicValue(cbCardData[i]);
		bSum += cbTemp[i];
	}

	//查找牛牛
	for (uint8_t i = 0; i < MAX_COUNT - 1; ++i)
	{
		for (uint8_t j = i + 1; j < MAX_COUNT; ++j)
		{
			if ((bSum - cbTemp[i] - cbTemp[j]) % 10 == 0)
			{
				uint8_t bCount = 0;
				for (uint8_t k = 0; k < MAX_COUNT; ++k)
				{
					if (k != i && k != j)
					{
						cbCardData[bCount++] = cbTempData[k];
					}
				}

				assert(bCount == 3);
				cbCardData[bCount++] = cbTempData[i];
				cbCardData[bCount++] = cbTempData[j];

				return true;
			}
		}
	}
	return false;
}

//炸弹牛4+1、四花牛牌型序
bool CGameLogic::SortCardListBH(uint8_t cbCardData[], uint8_t cbCardType)
{
	//设置变量
	uint8_t cbTemp[MAX_COUNT] = { 0 };
	uint8_t cbTempData[MAX_COUNT] = { 0 };
	memcpy(cbTempData, cbCardData, sizeof(cbTempData));
	SortCardList(cbTempData, MAX_COUNT);

	uint8_t cardValue = 0;
	uint8_t bCount = 0;
	if (cbCardType == (uint8_t)CT_SIHUA_NIU)
	{		
		for (uint8_t i = 0; i < MAX_COUNT; ++i)
		{
			cbCardData[bCount++] = cbTempData[i];
		}
	}
	else
	{
		cardValue = GetCardValue(cbTempData[MAX_COUNT / 2]);
		for (uint8_t i = 0; i < MAX_COUNT; ++i)
		{
			if (cardValue == GetCardValue(cbTempData[i]))
			{
				cbCardData[bCount++] = cbTempData[i];
			}
		}
		for (uint8_t i = 0; i < MAX_COUNT; ++i)
		{
			if (cardValue != GetCardValue(cbTempData[i]))
			{
				cbCardData[bCount++] = cbTempData[i];
			}
		}
	}
	
	return false;
}

//是否为五小
bool CGameLogic::IsFiveSmall(uint8_t cbCardData[MAX_COUNT])
{
	int wTotalValue = 0;
	//统计牌点总合
	for (int i = 0; i < MAX_COUNT; i++)
	{
		int wValue = GetCardValue(cbCardData[i]);
		if (wValue > 4)
		{
			return false;
		}
		wTotalValue += wValue;
	}
	//判断是否五小
	return (wTotalValue <= 10 && wTotalValue > 0) ? true : false;
}

//是否为炸弹
bool CGameLogic::IsBomb(uint8_t cbCardData[MAX_COUNT])
{
	//炸弹牌型
	uint8_t cbSameCount = 0;
	uint8_t cbTempCard[MAX_COUNT] = { 0 };
	memcpy(cbTempCard, cbCardData, sizeof(cbTempCard));
	//排序
	SortCardList(cbTempCard, MAX_COUNT);

	uint8_t bSecondValue = GetCardValue(cbTempCard[MAX_COUNT / 2]);
	for (uint8_t i = 0; i < MAX_COUNT; ++i)
	{
		if (bSecondValue == GetCardValue(cbTempCard[i]))
		{
			cbSameCount++;
		}
	}

	return  (4 == cbSameCount) ? true : false;
}

//是否为金牛(五花牛)
bool CGameLogic::IsWuHuaBull(uint8_t cbCardData[MAX_COUNT])
{
	uint8_t cbCount = 0;
	for (uint8_t i = 0; i < MAX_COUNT; ++i)
	{
		if (GetCardValue(cbCardData[i]) > 10)
		{//大于10
			++cbCount;
		}
		else
		{
			return false;
		}
	}
	return (5 == cbCount) ? true : false;
}

//是否为银牛[四花牛]
bool CGameLogic::IsSiHuaBull(uint8_t cbCardData[MAX_COUNT])
{
	uint8_t cbCount = 0;
	uint8_t cbCount1 = 0;
	for (uint8_t i = 0; i < MAX_COUNT; ++i)
	{
		uint8_t cardValue = GetCardValue(cbCardData[i]);
		if (cardValue > 10)
		{//大于等于10
			++cbCount;
		}
		else if (cardValue == 10)
		{
			++cbCount1;
		}
		else
		{
			return false;
		}
	}
	return (4 == cbCount && 1 == cbCount1) ? true : false;
}

//////////////////////////////////////////////////////////////////////////
