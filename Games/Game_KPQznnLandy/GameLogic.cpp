#include "GameLogic.h"
#include <time.h>
#include <algorithm>
#include "math.h"
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include "muduo/base/Timestamp.h"

//////////////////////////////////////////////////////////////////////////

//扑克数据
const uint8_t CGameLogic::m_byCardDataArray[MAX_CARD_TOTAL]=
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D	//黑桃 A - K
};

int	CGameLogic::m_iBigDivisorSeed[10] = { 10139, 17807, 33767, 49727, 65687, 81647, 97607, 113567, 129527, 145487 };
int	CGameLogic::m_iSmallDivsorSeed[10] = { 607, 1949, 3271, 4133, 6197, 8501, 10169, 17443, 24593, 29927 };
int	CGameLogic::m_iMultiSeed[10] = { 41, 43, 47, 53, 59, 61, 67, 71, 73, 79 };
//////////////////////////////////////////////////////////////////////////


//构造函数
CGameLogic::CGameLogic() : m_gen(m_rd())
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

unsigned int CGameLogic::GetSeed()
{
    return muduo::Timestamp::now().secondsSinceEpoch();
}

void CGameLogic::GenerateSeed(int TableId)
{
    int iSeed = GetSeed() % m_iBigDivisorSeed[rand() % 10] + rand() % m_iSmallDivsorSeed[rand() % 10] + TableId * m_iMultiSeed[rand() % 10] + 311;

    srand(iSeed);
}

//混乱扑克
void CGameLogic::RandCardData(uint8_t byCardBuffer[], uint8_t byBufferCount )
{
    //混乱准备
//    uint8_t cbCardData[MAX_CARD_TOTAL] = {0};
    memcpy(byCardBuffer, m_byCardDataArray, MAX_CARD_TOTAL);

    std::shuffle(&byCardBuffer[0], &byCardBuffer[MAX_CARD_TOTAL],m_gen);
//    random_shuffle(byCardBuffer, byCardBuffer+MAX_CARD_TOTAL);
}

//排列扑克
void CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount)
{
    assert(cbCardCount == MAX_COUNT);

    //数目过虑
    if (cbCardCount == 0)
        return;

    //转换数值
    uint8_t cbSortValue[MAX_COUNT];
    for (uint8_t i = 0; i < cbCardCount; i++)
        cbSortValue[i] = GetCardValue(cbCardData[i]);

    //排序操作
    bool bSorted = true;
    uint8_t cbSwitchData = 0, cbLast = cbCardCount - 1;

    do
    {
        bSorted = true;
        for (uint8_t i = 0; i < cbLast; i++)
        {
            if ((cbSortValue[i] < cbSortValue[i + 1]) ||
                ((cbSortValue[i] == cbSortValue[i + 1]) && (cbCardData[i] < cbCardData[i + 1])))
            {
                //设置标志
                bSorted = false;

                //扑克数据
                cbSwitchData = cbCardData[i];
                cbCardData[i] = cbCardData[i + 1];
                cbCardData[i + 1] = cbSwitchData;

                //排序权位
                cbSwitchData = cbSortValue[i];
                cbSortValue[i] = cbSortValue[i + 1];
                cbSortValue[i + 1] = cbSwitchData;
            }
        }
        cbLast--;
    } while (bSorted == false);
}

//获取类型
uint8_t CGameLogic::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount)
{
    assert(cbCardCount == MAX_COUNT);

    //是否为五小牛
    if (IsFiveSmall(cbCardData))
    {
        return OX_FIVE_SMALL;
    }
    //是否为炸弹
    if (IsBomb(cbCardData))
    {
        return OX_BOMB;
    }

    //是否为五花牛
    if (IsGoldenBull(cbCardData))
    {
        return OX_GOLDEN_BULL;
    }

    //是否为银牛
    if (IsSilveryBull(cbCardData))
    {
        return OX_SILVERY_BULL;
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

    return OX_VALUE0;	//无牛
}

//对比扑克
bool CGameLogic::CompareCard(uint8_t cbFirstCard[], uint8_t cbNextCard[])
{
    //获取点数
    uint8_t cbFirstType = GetCardType(cbFirstCard, MAX_COUNT);
    uint8_t cbNextType = GetCardType(cbNextCard, MAX_COUNT);

    if(cbFirstType != cbNextType)
    {//牌型不同
        return (cbFirstType > cbNextType) ? true : false;
    }

    //牌型相同
    //排序大到小
    uint8_t bFirstTemp[MAX_COUNT] = { 0 };
    uint8_t bNextTemp[MAX_COUNT] = { 0 };
    memcpy(bFirstTemp, cbFirstCard, MAX_COUNT);
    memcpy(bNextTemp, cbNextCard, MAX_COUNT);
    SortCardList(bFirstTemp, MAX_COUNT);
    SortCardList(bNextTemp, MAX_COUNT);

    //牌型相等
    if (cbFirstType == OX_BOMB)
    {
        return GetCardValue(bFirstTemp[MAX_COUNT / 2]) > GetCardValue(bNextTemp[MAX_COUNT / 2]);
    }

    //有牛牌型(非牛牛)比较：比牛数；牛数相同庄吃闲。
    //if (cbFirstType > OX_VALUE0 && cbFirstType < 10)
    //{
    //	return true;
    //}
    //比较数值
    uint8_t cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
    uint8_t cbNextMaxValue = GetCardValue(bNextTemp[0]);
    if (cbNextMaxValue != cbFirstMaxValue)
        return cbFirstMaxValue > cbNextMaxValue;

    //比较颜色
    return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);

    return false;
}

//获取倍数
uint8_t CGameLogic::GetMultiple(uint8_t cbCardData[])
{
    uint8_t  cbCardType = GetCardType(cbCardData, MAX_COUNT);
    if (cbCardType <= 6)return 1;
    else if (cbCardType <= 9)return 2;
    else if (cbCardType == 10)return 3;
    else if (cbCardType == OX_BOMB)return 4;
    else if (cbCardType == OX_SILVERY_BULL)return 4;
    else if (cbCardType == OX_GOLDEN_BULL)return 4;
    else if (cbCardType == OX_FIVE_SMALL)return 4;

    return 0;
}

////获取倍数
//uint8_t CGameLogic::GetMultiple(uint8_t cbCardData[])
//{
//    uint8_t cbCardType = GetCardType(cbCardData, MAX_COUNT);
//    if (cbCardType <= 6)
//        return 1;
//    else if (cbCardType <= 8)
//        return 2;
//    else if (cbCardType <= 9)
//        return 3;
//    else if (cbCardType == 10)
//        return 4;
//    else if (cbCardType == OX_BOMB)
//        return 5;
//    else if (cbCardType == OX_GOLDEN_BULL)
//        return 5;
//    else if (cbCardType == OX_FIVE_SMALL)
//        return 5;

//    return 0;
//}

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
    uint8_t cbTempCard[MAX_COUNT] = {0};
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
bool CGameLogic::IsGoldenBull(uint8_t cbCardData[MAX_COUNT])
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

//是否为银牛
bool CGameLogic::IsSilveryBull(uint8_t cbCardData[MAX_COUNT])
{
    uint8_t cbCount = 0;
    uint8_t cbCount1 = 0;
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        uint8_t cardValue = GetCardValue(cbCardData[i]);
        if (cardValue > 10)
        {//大于等于10
            ++cbCount;
        }else if(cardValue == 10)
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


