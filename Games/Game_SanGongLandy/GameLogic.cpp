#include "GameLogic.h"
#include <time.h>
#include <algorithm>
#include "math.h"
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <boost/algorithm/string.hpp>

//#include "muduo/base/Timestamp.hud"
#include <muduo/base/Timestamp.h>

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
//    struct tm *t;
//    time_t tt;
//    time(&tt);
//    t = localtime(&tt);

//    return t->tm_mday * 24 * 3600 + t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
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
    assert(MAX_CARD_TOTAL == byBufferCount);
    //混乱准备
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

    //是否为爆玖
    if (IsBombNine(cbCardData))
    {
        return SG_BOMB_NINE;
    }

    //是否为炸弹
    if (IsBomb(cbCardData))
    {
        return SG_BOMB;
    }

    //是否为炸弹
    if(IsThreePoker(cbCardData))
    {
        return SG_THREE_POKER;
    }

    //点数
    uint8_t bTemp[MAX_COUNT];
    uint8_t bSum = 0;
    for (uint8_t i = 0; i < cbCardCount; ++i)
    {
        bTemp[i] = GetLogicValue(cbCardData[i]);
        bSum += bTemp[i];
    }

    return (bSum%10);
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
    if (cbFirstType == SG_BOMB)
    {
        //比较炸弹的大小
        return GetCardValue(bFirstTemp[0]) > GetCardValue(bNextTemp[0]);
    }
    else if(cbFirstType == SG_THREE_POKER)
    {
        //比较三公的大小
        uint8_t cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
        uint8_t cbNextMaxValue = GetCardValue(bNextTemp[0]);
        if (cbNextMaxValue != cbFirstMaxValue)
            return cbFirstMaxValue > cbNextMaxValue;

        //比较颜色
        return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
    }
    else
    {
        //比较点数相同的大小,比较点数相同的大小，优先比较公仔牌的个数
        uint8_t cbFirstDollCard = 0;
        for(int i = 0; i < 3; ++i)
        {
            if(GetCardValue(bFirstTemp[i]) > 10) ++cbFirstDollCard;
        }
        uint8_t cbNextDollCard = 0;
        for(int i = 0; i < 3; ++i)
        {
            if(GetCardValue(bNextTemp[i]) > 10) ++cbNextDollCard;
        }
        if(cbFirstDollCard != cbNextDollCard)
            return cbFirstDollCard > cbNextDollCard;

        //公仔牌的个数相同,比较最大牌的点数
        if(GetCardValue(bFirstTemp[0]) != GetCardValue(bNextTemp[0]))
        {
            return GetCardValue(bFirstTemp[0]) > GetCardValue(bNextTemp[0]);
        }

        //最大牌的点数相同,比较最大牌颜色
        return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
    }
}

//获取倍数
uint8_t CGameLogic::GetMultiple(uint8_t cbCardData[])
{
    uint8_t  cbCardType = GetCardType(cbCardData, MAX_COUNT);
    if (cbCardType <= 6)return 1;
    else if (cbCardType <= 9)return 2;
    else if (cbCardType == SG_THREE_POKER)return 3;
    else if (cbCardType == SG_BOMB)return 4;
    else if (cbCardType == SG_BOMB_NINE)return 5;

    return 0;
}

//是否为爆玖
bool CGameLogic::IsBombNine(uint8_t cbCardData[MAX_COUNT])
{
    //炸弹牌型
    uint8_t cbSameCount = 0;
    uint8_t cbTempCard[MAX_COUNT] = {0};
    memcpy(cbTempCard, cbCardData, sizeof(cbTempCard));

    uint8_t bSecondValue = 0;
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        bSecondValue = GetCardValue(cbTempCard[i]);
        if (bSecondValue == 3)
        {
            cbSameCount++;
        }
        else
        {
            break;
        }
    }

    return  (3 == cbSameCount) ? true : false;
}

//是否为炸弹
bool CGameLogic::IsBomb(uint8_t cbCardData[MAX_COUNT])
{
    uint8_t cbSameCount = 0;
    uint8_t cbTempCard[MAX_COUNT] = {0};
    memcpy(cbTempCard, cbCardData, sizeof(cbTempCard));

    uint8_t bSecondValue = GetCardValue(cbTempCard[0]);
    for (uint8_t i = 1; i < MAX_COUNT; ++i)
    {
        if (bSecondValue == GetCardValue(cbTempCard[i]))
        {
            cbSameCount++;
        }
    }

    return  (2 == cbSameCount) ? true : false;
}

//是否为三公
bool CGameLogic::IsThreePoker(uint8_t cbCardData[MAX_COUNT])
{
    uint8_t cbCount = 0;
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        if (GetCardValue(cbCardData[i]) > 10) //大于10
        {
            ++cbCount;
        }
        else
        {
            return false;
        }
    }
    return (3 == cbCount) ? true : false;
}

bool CGameLogic::IsUsedCard(uint8_t cbCardData)
{
    bool isUsered = false;
    std::vector<uint8_t>::iterator  it = m_vecCardVal.begin();
    for (; it != m_vecCardVal.end(); ++it)
    {
        if (*it == cbCardData)
        {
            isUsered = true;
            break;
        }
    }
    return isUsered;
}

void CGameLogic::InsertUsedCard(uint8_t cbCardData)
{
    m_vecCardVal.push_back(cbCardData);
}

void CGameLogic::ClearUsedCard()
{
    m_vecCardVal.clear();
}

uint8_t CGameLogic::GetCardData(uint8_t cbColor, uint8_t cbValue)
{
    return cbColor*16+cbValue;
}

bool CGameLogic::GetMatchPointCard(int32_t iPoint,uint8_t cbCardData[MAX_COUNT], uint8_t byCardDataArray[MAX_CARD_TOTAL])
{
    uint8_t nColorValue = 0;
    int nFind = 0;
    int index = 0;
    uint8_t bLogicValue1 = 0;
    uint8_t bLogicValue2 = 0;
    uint8_t bNeedValue = 0;
    uint8_t bCardData = 0;

    //先获取两张牌
    if(iPoint < SG_THREE_POKER)
    {
        do
        {
            cbCardData[0] = byCardDataArray[index++];
            if(index == 52) index = 0;
        }while(IsUsedCard(cbCardData[0]));
        InsertUsedCard(cbCardData[0]);

        do
        {
            cbCardData[1] = byCardDataArray[index++];
            if(index == 52) index = 0;
        }while(IsUsedCard(cbCardData[1]) || (cbCardData[0] == cbCardData[1]));
        InsertUsedCard(cbCardData[1]);

        if(GetCardValue(cbCardData[0]) > 10 && GetCardValue(cbCardData[1]) > 10)
        {
            if(iPoint == 0)
                bNeedValue = 10;
            else if(iPoint == 1)
                bNeedValue = 1;
            else if(iPoint == 2)
                bNeedValue = 2;
            else if(iPoint == 3)
                bNeedValue = 3;
            else if(iPoint == 4)
                bNeedValue = 4;
            else if(iPoint == 5)
                bNeedValue = 5;
            else if(iPoint == 6)
                bNeedValue = 6;
            else if(iPoint == 7)
                bNeedValue = 7;
            else if(iPoint == 8)
                bNeedValue = 8;
            else if(iPoint == 9)
                bNeedValue = 9;
        }
        else
        {
            bLogicValue1 = GetLogicValue(cbCardData[0]);
            bLogicValue2 = GetLogicValue(cbCardData[1]);
            if(iPoint == 0)
                bNeedValue = 10 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 1)
                bNeedValue = 11 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 2)
                bNeedValue = 12 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 3)
                bNeedValue = 13 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 4)
                bNeedValue = 14 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 5)
                bNeedValue = 15 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 6)
                bNeedValue = 16 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 7)
                bNeedValue = 17 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 8)
                bNeedValue = 18 - (bLogicValue1 + bLogicValue2)%10;
            else if(iPoint == 9)
                bNeedValue = 19 - (bLogicValue1 + bLogicValue2)%10;

            if(bNeedValue > 10)
            {
                bNeedValue = bNeedValue%10;
            }

            if(bNeedValue == 0 || bNeedValue == 10)
            {
                srand(time(NULL));
                bNeedValue = rand()%4+10;
            }
        }

        srand(time(NULL));
        //随机获取花色
        nColorValue = (uint8_t)(rand()%4);
        nFind = 0;
        bCardData = 0;
        while(nFind<4)
        {
            bCardData = GetCardData(nColorValue, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[2] = bCardData;
                break;
            }
            uint8_t bXXX = 4;
            uint8_t nColorValueA = ++nColorValue;
            nColorValue = (uint8_t)(nColorValueA%bXXX);
            nFind++;
        }
        if(nFind == 4)
        {
            return false;
        }
        else
        {
            InsertUsedCard(cbCardData[2]);
            return true;
        }
    }
    else if(iPoint == SG_BOMB_NINE)
    {
        //随机获取花色
        bNeedValue = 3;

        srand(time(NULL));
        nColorValue = rand()%4;
        if(nColorValue == 0) //爆玖里面没有方块3
        {
            bCardData = GetCardData(1, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[0] = bCardData;
                InsertUsedCard(cbCardData[0]);
            }
            else
            {
                return false;
            }

            bCardData = GetCardData(2, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[1] = bCardData;
                InsertUsedCard(cbCardData[1]);
            }
            else
            {
                return false;
            }

            bCardData = GetCardData(3, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[2] = bCardData;
                InsertUsedCard(cbCardData[2]);
            }
            else
            {
                return false;
            }
        }
        else if(nColorValue == 1)//爆玖里面没有梅花3
        {
            bCardData = GetCardData(0, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[0] = bCardData;
                InsertUsedCard(cbCardData[0]);
            }
            else
            {
                return false;
            }

            bCardData = GetCardData(2, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[1] = bCardData;
                InsertUsedCard(cbCardData[1]);
            }
            else
            {
                return false;
            }

            bCardData = GetCardData(3, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[2] = bCardData;
                InsertUsedCard(cbCardData[2]);
            }
            else
            {
                return false;
            }
        }
        else if(nColorValue == 2)//爆玖里面没有红桃3
        {
            bCardData = GetCardData(0, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[0] = bCardData;
                InsertUsedCard(cbCardData[0]);
            }
            else
            {
                return false;
            }
            bCardData = GetCardData(1, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[1] = bCardData;
                InsertUsedCard(cbCardData[1]);
            }
            else
            {
                return false;
            }
            bCardData = GetCardData(3, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[2] = bCardData;
                InsertUsedCard(cbCardData[2]);
            }
            else
            {
                return false;
            }
        }
        else if(nColorValue == 3)//爆玖里面没有黑桃3
        {
            bCardData = GetCardData(0, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[0] = bCardData;
                InsertUsedCard(cbCardData[0]);
            }
            else
            {
                return false;
            }
            bCardData = GetCardData(1, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[1] = bCardData;
                InsertUsedCard(cbCardData[1]);
            }
            else
            {
                return false;
            }
            bCardData = GetCardData(2, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[2] = bCardData;
                InsertUsedCard(cbCardData[2]);
            }
            else
            {
                return false;
            }
        }
    }
    else if(iPoint == SG_BOMB)
    {
        do
        {
            srand(time(NULL));
            bNeedValue = rand()%13+1;
            nFind= 0;
            for(int j = 0 ; j < 4; j++)
            {
                bCardData = GetCardData(1, bNeedValue);
                if(!IsUsedCard(bCardData)) nFind++;
            }

        }while(bNeedValue == 3 || nFind < 3); //除去爆玖

        if(nFind == 4)
        {
            index = 0;
            srand(time(NULL));
            nColorValue = (uint8_t)(rand()%4); //nColorValue定义为不使用的花色
            uint8_t nColor = nColorValue;
            while(index < 3)
            {
                uint8_t nColorTemp = ++nColor;
                nColor = nColorTemp%4;
                bCardData = GetCardData(nColor, bNeedValue);
                cbCardData[index++] = bCardData;
                InsertUsedCard(bCardData);
            }
            return true;
        }
        else
        {
            if(nFind < 3) return false;
            index = 0;
            nColorValue = 0;
            while(index < 3)
            {
                bCardData = GetCardData(nColorValue, bNeedValue);
                if(!IsUsedCard(bCardData))
                {
                    cbCardData[index++] = bCardData;
                    InsertUsedCard(bCardData);
                }
            }

        }

    }
    else if(iPoint == SG_THREE_POKER)
    {
        do
        {
            cbCardData[0] = byCardDataArray[index++];
            if(index == 52) index = 0;
        }while(IsUsedCard(cbCardData[0]) || GetCardValue(cbCardData[0]) < 11);
        InsertUsedCard(cbCardData[0]);

        do
        {
            cbCardData[1] = byCardDataArray[index++];
            if(index == 52) index = 0;
        }while(IsUsedCard(cbCardData[1]) || (cbCardData[0] == cbCardData[1]) || GetCardValue(cbCardData[1]) < 11);
        InsertUsedCard(cbCardData[1]);

        do
        {
            cbCardData[2] = byCardDataArray[index++];
            if(index == 52) index = 0;
        }while(IsUsedCard(cbCardData[2]) || GetCardValue(cbCardData[2]) < 11);
        InsertUsedCard(cbCardData[2]);
        return true;
    }

    return false;
}

bool CGameLogic::GetResolvePointCard(string& sValue,uint8_t cbCardData[MAX_COUNT], uint8_t byCardDataArray[MAX_CARD_TOTAL])
{
    string flagCardValue = ",";
    string flagCardType = "suiji";

    string::size_type position1 = 0;
    int Index = 0;
    position1 = sValue.find(flagCardValue);
    if(position1 != sValue.npos)
    {
        //find card value
        vector<string> vStr;
        boost::split( vStr, sValue, boost::is_any_of(flagCardValue), boost::token_compress_on );
        Index = 0;
        for( vector<string>::iterator it = vStr.begin(); it != vStr.end(); ++ it )
        {

            cbCardData[Index++] = stod((*it).c_str());
        }
        return true;
    }
    else
    {
        //not find card value
        string::size_type position2 = 0;
        position2 = sValue.find(flagCardType);
        if(position2 != sValue.npos)
        {
            //find card type
            string sTestValue = sValue.substr(5);
            int iPoint =atoi(sTestValue.c_str());
            return GetMatchPointCard(iPoint, cbCardData, byCardDataArray);
        }
        else
        {
            return false;
        }
    }

    return false;
}
