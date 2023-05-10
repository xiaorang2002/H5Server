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

const uint8_t CGameLogic::m_byHaveKingCardDataArray[MAX_CARD_TOTAL_KING]=
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,   //方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,   //梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,   //红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,   //黑桃 A - K
    0x4E,0x4F                                                           //小王 大王
};

int	CGameLogic::m_iBigDivisorSeed[10] = { 10139, 17807, 33767, 49727, 65687, 81647, 97607, 113567, 129527, 145487 };
int	CGameLogic::m_iSmallDivsorSeed[10] = { 607, 1949, 3271, 4133, 6197, 8501, 10169, 17443, 24593, 29927 };
int	CGameLogic::m_iMultiSeed[10] = { 41, 43, 47, 53, 59, 61, 67, 71, 73, 79 };
//////////////////////////////////////////////////////////////////////////


//构造函数
CGameLogic::CGameLogic() : m_gen(m_rd())
{
    m_bHaveKing = false;
    m_bPlaySixChooseFive = false;
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
    //混乱准备
    if (byBufferCount==MAX_CARD_TOTAL)
    {
        memcpy(byCardBuffer, m_byCardDataArray, MAX_CARD_TOTAL);
        std::shuffle(&byCardBuffer[0], &byCardBuffer[MAX_CARD_TOTAL],m_gen);
    }
    else
    {
        memcpy(byCardBuffer, m_byHaveKingCardDataArray, MAX_CARD_TOTAL_KING);
        std::shuffle(&byCardBuffer[0], &byCardBuffer[MAX_CARD_TOTAL_KING],m_gen);
    }
    
}

//排列扑克
void CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t bOrder)
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
            if (bOrder) //降序
            {
                if ((cbSortValue[i] > cbSortValue[i + 1]) ||
                    ((cbSortValue[i] == cbSortValue[i + 1]) && (cbCardData[i] > cbCardData[i + 1])))
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
            else
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
        }
        cbLast--;
    } while (bSorted == false);
}
void CGameLogic::SortCardListSix(uint8_t cbCardData[], uint8_t cbCardCount)
{
    assert(cbCardCount == MAX_COUNT_SIX);

    //数目过虑
    if (cbCardCount == 0)
        return;

    //转换数值
    uint8_t cbSortValue[MAX_COUNT_SIX];
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
uint8_t CGameLogic::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount,bool bKingChangCard)
{
    assert(cbCardCount == MAX_COUNT);

    //是否为同花顺牛
    if (!bKingChangCard && IsTierce(cbCardData))
    {
        return CT_TONGHUASHUN_NIU;
    }
    //是否为五小牛
    if (!bKingChangCard && IsFiveSmall(cbCardData))
    {
        return CT_FIVESMALL_NIU;
    }
    //是否为炸弹牛
    if (IsBomb(cbCardData))
    {
        return CT_BOMB_NIU;
    }
    //是否为葫芦牛
    if (!bKingChangCard && IsCalabash(cbCardData))
    {
        return CT_HULU_NIU;
    }
    //是否为同花牛
    if (!bKingChangCard && IsFlush(cbCardData))
    {
        return CT_TONGHUA_NIU;
    }
    //是否为五花牛
    if (IsGoldenBull(cbCardData))
    {
        return CT_WUHUA_NIU;
    }
    //是否为顺子牛
    if (!bKingChangCard && IsStraight(cbCardData))
    {
        return CT_SHUNZI_NIU;
    }
    
    //是否为银牛
    // if (IsSilveryBull(cbCardData))
    // {
    //     return OX_SILVERY_BULL;
    // }

    return IsOtherNiu(cbCardData);
}

//对比扑克
bool CGameLogic::CompareCard(uint8_t cbFirstCard[], uint8_t cbNextCard[],uint8_t cbFirstKingChangCard[2],uint8_t cbNextKingChangCard[2], bool bSelfSixChooseFive, bool bFirstBanker)
{
    //获取牌值
    uint8_t bFirstTemp[MAX_COUNT] = { 0 };
    uint8_t bNextTemp[MAX_COUNT] = { 0 };
    memcpy(bFirstTemp, cbFirstCard, MAX_COUNT);
    memcpy(bNextTemp, cbNextCard, MAX_COUNT);
    bool bFirstHaveKingChang = false;
    bool bNextHaveKingChang = false;
    bool bFirstHaveBigKing = false;
    bool bNextHaveBigKing = false;
	bool bFirstHaveSmallKing = false;
	bool bNextHaveSmallKing = false;
    bool bFirstHaveTwoKing = false;
    bool bNextHaveTwoKing = false;
    uint8_t FirsthaveKingCount = 0;
    uint8_t NexthaveKingCount = 0;
    uint8_t FirsKingChangCount = 0;
    uint8_t NextKingChangCount = 0;
	//先获取默认最大的牌型
	uint8_t kingChangCard[2] = { 0 };
	uint8_t cbMaxCardValue[MAX_COUNT] = { 0 };
	uint8_t bFirstmaxType = GetHaveKingBigCardType(bFirstTemp, cbMaxCardValue, 5, kingChangCard);
	uint8_t bNextmaxType = GetHaveKingBigCardType(bNextTemp, cbMaxCardValue, 5, kingChangCard);
	if (bFirstmaxType != bNextmaxType)
	{   //牌型不同
		return (bFirstmaxType > bNextmaxType) ? true : false;
	}
    //是否有王的变牌
    if (m_bHaveKing)
    {
        for (int i = 0; i < MAX_COUNT; ++i)
        {
            if (bFirstTemp[i]==0x4e || bFirstTemp[i]==0x4f)
                FirsthaveKingCount++;
            if (bNextTemp[i]==0x4e || bNextTemp[i]==0x4f)
                NexthaveKingCount++;
        }
        if (FirsthaveKingCount==2)
        {
            bFirstHaveTwoKing = true;
        }
        if (NexthaveKingCount==2)
        {
            bNextHaveTwoKing = true;
        }
        for (int i = 0; i < MAX_COUNT; ++i)
        {
            if (bFirstTemp[i]==0x4f)
            {
                bFirstHaveBigKing = true;
            }
            if (bNextTemp[i]==0x4f)
            {
                bNextHaveBigKing = true;
            }
			if (bFirstTemp[i] == 0x4e)
			{
				bFirstHaveSmallKing = true;
			}
			if (bNextTemp[i] == 0x4e)
			{
				bNextHaveSmallKing = true;
			}
            if (bFirstTemp[i]==0x4e && cbFirstKingChangCard[0]!=0)//小王
            {
                bFirstTemp[i]=cbFirstKingChangCard[0];
                bFirstHaveKingChang = true;
                FirsKingChangCount++;
            }
            if (bFirstTemp[i]==0x4f && cbFirstKingChangCard[1]!=0)//大王
            {
                bFirstTemp[i]=cbFirstKingChangCard[1];
                bFirstHaveKingChang = true;
                FirsKingChangCount++;
            }

            if (bNextTemp[i]==0x4e && cbNextKingChangCard[0]!=0)//小王
            {
                bNextTemp[i]=cbNextKingChangCard[0];
                bNextHaveKingChang = true;
                NextKingChangCount++;
            }
            if (bNextTemp[i]==0x4f && cbNextKingChangCard[1]!=0)//大王
            {
                bNextTemp[i]=cbNextKingChangCard[1];
                bNextHaveKingChang = true;
                NextKingChangCount++;
            }
        }
    }
    //获取点数
	uint8_t cbFirstType = bFirstmaxType; //GetCardType(bFirstTemp, MAX_COUNT, bFirstHaveKingChang);
	uint8_t cbNextType = bNextmaxType;   //GetCardType(bNextTemp, MAX_COUNT, bNextHaveKingChang);

    if(cbFirstType != cbNextType)
    {//牌型不同
        return (cbFirstType > cbNextType) ? true : false;
    }

    //牌型相同
    //排序大到小
    memcpy(bFirstTemp, cbFirstCard, MAX_COUNT);
    memcpy(bNextTemp, cbNextCard, MAX_COUNT);
    SortCardList(bFirstTemp, MAX_COUNT);
    SortCardList(bNextTemp, MAX_COUNT);
    uint8_t cbFirstMaxValue = 0;
    uint8_t cbNextMaxValue = 0;
    uint8_t cbFirstColor = 0;
    uint8_t cbNextColor = 0;

    //牌型相等
	//1:天豪的比较方法不用【同牌型(无牛~普通牛)同点之间比最大牌大小，若大小相同，则比第二大牌，以此类推，直到一方的牌比令一方大则胜出；若五张牌大小都相同，则比最大牌的花色。】
	//2:大唐的比较【最大牌的点数，最大牌的点数相同，则比较最大牌的花色（黑桃>红桃>梅花>方块）】
	int CompareType = 2;
    if (cbFirstType<=CT_NIUNIU)
    {
        if (m_bHaveKing)
        {            
            if (cbFirstType==CT_NIUNIU) 
            {             
                //牛牛牌型比较,一个玩家得到两个王时,另一个没王
                if ((bFirstHaveTwoKing && !bNextHaveTwoKing) || (!bFirstHaveTwoKing && bNextHaveTwoKing))
                {
                    if (bFirstHaveTwoKing && FirsKingChangCount==1 && NextKingChangCount==0) //两个王,一个变一个没变
                    {
                        return true;
                    }
                    if (bNextHaveTwoKing && NextKingChangCount==1 && FirsKingChangCount==0) //两个王,一个变一个没变
                    {
                        return false;
                    }
                }
                else // 6选5,自己手牌两王,两个王,变一个,另一手牌变两个
                {
                    if (bFirstHaveTwoKing == bNextHaveTwoKing)
                    {
                        if (bFirstHaveTwoKing && FirsKingChangCount==1 && NextKingChangCount==2) 
                        {
                            return true;
                        }
                        if (bNextHaveTwoKing && NextKingChangCount==1 && FirsKingChangCount==2)
                        {
                            return false;
                        }
                    }
                }
            }
            //王变,王没变,没变的大;
            if (!bFirstHaveKingChang && bNextHaveKingChang) 
            {
                return true;
            }
            if (bFirstHaveKingChang && !bNextHaveKingChang) 
            {
                return false;
            }
            //王同变或没变时,默认大王赢
            if (bFirstHaveKingChang == bNextHaveKingChang)
            {
                if (bFirstHaveBigKing && !bNextHaveBigKing)
                {
                    return true;
                }
                if (!bFirstHaveBigKing && bNextHaveBigKing)
                {
                    return false;
                }
            }
        }
		if (CompareType==1) //天豪的比较方法
		{
			//比较数值
			for (int i = 0; i < MAX_COUNT; ++i)
			{
				cbFirstMaxValue = GetCardValue(bFirstTemp[i]);
				cbNextMaxValue = GetCardValue(bNextTemp[i]);
				if (cbNextMaxValue != cbFirstMaxValue)
					return cbFirstMaxValue > cbNextMaxValue;
			}
			//比较颜色
			for (int i = 0; i < MAX_COUNT; ++i)
			{
				cbFirstColor = GetCardColor(bFirstTemp[i]);
				cbNextColor = GetCardColor(bNextTemp[i]);
				if (cbFirstColor != cbNextColor)
					return cbFirstColor > cbNextColor;
			}
		} 
		else //大唐的比较
		{
			if (bSelfSixChooseFive)
			{
				//比较数值
				for (int i = 0; i < MAX_COUNT; ++i)
				{
					cbFirstMaxValue = GetCardValue(bFirstTemp[i]);
					cbNextMaxValue = GetCardValue(bNextTemp[i]);
					if (cbNextMaxValue != cbFirstMaxValue)
						return cbFirstMaxValue > cbNextMaxValue;
				}
				//比较颜色
				for (int i = 0; i < MAX_COUNT; ++i)
				{
					cbFirstColor = GetCardColor(bFirstTemp[i]);
					cbNextColor = GetCardColor(bNextTemp[i]);
					if (cbFirstColor != cbNextColor)
						return cbFirstColor > cbNextColor;
				}
			}
			else
			{
				//比较数值
				cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
				cbNextMaxValue = GetCardValue(bNextTemp[0]);
				if (cbNextMaxValue != cbFirstMaxValue)
					return cbFirstMaxValue > cbNextMaxValue;
				//比较颜色
				return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
			}
		}
    }
    else //同特殊牌型
    {
        //同花顺牛,先对比顺子大小，若相同对比花色(910JQK最大、A2345最小)；
        //同花牛、五花牛比最大牌点数,点数相同比最大牌花色
        if (m_bHaveKing)
        {
            //同拥有王牌的五花牛情况下,默认大王赢
            if (cbFirstType == CT_WUHUA_NIU)
            {
                if (bFirstHaveBigKing)
                {
                    return true;
                }
                if (bNextHaveBigKing)
                {
                    return false;
                }
				if (bFirstHaveSmallKing)
				{
					return true;
				}
				if (bNextHaveSmallKing)
				{
					return false;
				}
            }
        }
        if (cbFirstType == CT_TONGHUASHUN_NIU || cbFirstType == CT_TONGHUA_NIU || cbFirstType == CT_WUHUA_NIU || cbFirstType == CT_SHUNZI_NIU) 
        {
            //比较数值
			if (bSelfSixChooseFive)
			{
				for (int i = 0; i < MAX_COUNT; i++)
				{
					cbFirstMaxValue = GetCardValue(bFirstTemp[i]);
					cbNextMaxValue = GetCardValue(bNextTemp[i]);
					if (cbNextMaxValue != cbFirstMaxValue)
						return cbFirstMaxValue > cbNextMaxValue;
				}
			} 
			else
			{
				cbFirstMaxValue = GetCardValue(bFirstTemp[0]);
				cbNextMaxValue = GetCardValue(bNextTemp[0]);
				if (cbNextMaxValue != cbFirstMaxValue)
					return cbFirstMaxValue > cbNextMaxValue;
			}
            
            //比较颜色
            // return GetCardColor(bFirstTemp[0]) > GetCardColor(bNextTemp[0]);
            for (int i = 0; i < MAX_COUNT; ++i)
            {
                cbFirstColor = GetCardColor(bFirstTemp[i]);
                cbNextColor = GetCardColor(bNextTemp[i]);
                if (cbFirstColor != cbNextColor)
                    return cbFirstColor > cbNextColor;
            }
        }
        else if (cbFirstType == CT_FIVESMALL_NIU) //同五小牛，默认庄赢
        {
            if (!bFirstBanker)
            {
                //比较数值
                for (int i = 0; i < MAX_COUNT; ++i)
                {
                    cbFirstMaxValue = GetCardValue(bFirstTemp[i]);
                    cbNextMaxValue = GetCardValue(bNextTemp[i]);
                    if (cbNextMaxValue != cbFirstMaxValue)
                        return cbFirstMaxValue > cbNextMaxValue;
                }                
                //比较颜色
                for (int i = 0; i < MAX_COUNT; ++i)
                {
                    cbFirstColor = GetCardColor(bFirstTemp[i]);
                    cbNextColor = GetCardColor(bNextTemp[i]);
                    if (cbFirstColor != cbNextColor)
                        return cbFirstColor > cbNextColor;
                }
            }
            return bFirstBanker;
        }
        else if (cbFirstType == CT_BOMB_NIU || cbFirstType == CT_HULU_NIU) //炸弹牛,比较四炸的大小；葫芦牛比三条的大小
        {
            if (GetCardValue(bFirstTemp[MAX_COUNT / 2]) == GetCardValue(bNextTemp[MAX_COUNT / 2]))
            {
                //比较数值
                for (int i = 0; i < MAX_COUNT; ++i)
                {
                    cbFirstMaxValue = GetCardValue(bFirstTemp[i]);
                    cbNextMaxValue = GetCardValue(bNextTemp[i]);
                    if (cbNextMaxValue != cbFirstMaxValue)
                        return cbFirstMaxValue > cbNextMaxValue;
                } 
                //比较颜色
                for (int i = 0; i < MAX_COUNT; ++i)
                {
                    cbFirstColor = GetCardColor(bFirstTemp[i]);
                    cbNextColor = GetCardColor(bNextTemp[i]);
                    if (cbFirstColor != cbNextColor)
                        return cbFirstColor > cbNextColor;
                }
            }
            else
            {
                return GetCardValue(bFirstTemp[MAX_COUNT / 2]) > GetCardValue(bNextTemp[MAX_COUNT / 2]);
            }
        }
    }
 
    return false;
}

//获取倍数
uint8_t CGameLogic::GetMultiple(uint8_t cbCardData[],uint8_t playGameType)
{
    uint8_t  cbCardType = GetCardType(cbCardData, MAX_COUNT);
    switch(playGameType)
    {
        case PlayGameType_Normal: //经典厅-5倍
        case PlayGameType_Huangjin: //黄金厅-六张5倍
        {
            if (cbCardType <= CT_NIU_SIX)return 1;
            else if (cbCardType <= CT_NIU_NINE)return 2;
            else if (cbCardType == CT_NIUNIU)return 3;
            else if (cbCardType == CT_SHUNZI_NIU)return 5;
            else if (cbCardType == CT_WUHUA_NIU)return 5;
            else if (cbCardType == CT_TONGHUA_NIU)return 5;
            else if (cbCardType == CT_HULU_NIU)return 5;
            else if (cbCardType == CT_BOMB_NIU)return 5;
            else if (cbCardType == CT_FIVESMALL_NIU)return 5;
            else if (cbCardType == CT_TONGHUASHUN_NIU)return 5;
            break;
        }
        case PlayGameType_Jinyu: //金玉厅-六张3倍
        case PlayGameType_HaoHua: //豪华厅-3倍
        {
            if (cbCardType <= CT_NIU_SIX)return 1;
            else if (cbCardType <= CT_NIU_NINE)return 2;
            else if (cbCardType == CT_NIUNIU)return 2;
            else if (cbCardType == CT_SHUNZI_NIU)return 3;
            else if (cbCardType == CT_WUHUA_NIU)return 3;
            else if (cbCardType == CT_TONGHUA_NIU)return 3;
            else if (cbCardType == CT_HULU_NIU)return 3;
            else if (cbCardType == CT_BOMB_NIU)return 3;
            else if (cbCardType == CT_FIVESMALL_NIU)return 3;
            else if (cbCardType == CT_TONGHUASHUN_NIU)return 3;
            break;
        }
        case PlayGameType_Huangjia: //皇家厅-百变        
        case PlayGameType_Chuanqi: //传奇厅-六张百变
        {
            if (cbCardType <= CT_NIU_ONE)return 1;
            else if (cbCardType == CT_NIU_TWO)return 2;
            else if (cbCardType == CT_NIU_THREE)return 3;
            else if (cbCardType == CT_NIU_FOUR)return 4;
            else if (cbCardType == CT_NIU_FIVE)return 5;
            else if (cbCardType == CT_NIU_SIX)return 6;
            else if (cbCardType == CT_NIU_SEVEN)return 7;
            else if (cbCardType == CT_NIU_EIGHT)return 8;
            else if (cbCardType == CT_NIU_NINE)return 9;
            else if (cbCardType == CT_NIUNIU)return 10;
            else if (cbCardType == CT_SHUNZI_NIU)return 11;
            else if (cbCardType == CT_WUHUA_NIU)return 12;
            else if (cbCardType == CT_TONGHUA_NIU)return 13;
            else if (cbCardType == CT_HULU_NIU)return 14;
            else if (cbCardType == CT_BOMB_NIU)return 15;
            else if (cbCardType == CT_FIVESMALL_NIU)return 16;
            else if (cbCardType == CT_TONGHUASHUN_NIU)return 17;
            break;
        }
        case PlayGameType_Fengsao: //风骚厅-六张10倍
        case PlayGameType_Rongyao: //荣耀厅-10倍
        {
            if (cbCardType <= CT_NIU_ONE)return 1;
            else if (cbCardType == CT_NIU_TWO)return 2;
            else if (cbCardType == CT_NIU_THREE)return 3;
            else if (cbCardType == CT_NIU_FOUR)return 4;
            else if (cbCardType == CT_NIU_FIVE)return 5;
            else if (cbCardType == CT_NIU_SIX)return 6;
            else if (cbCardType == CT_NIU_SEVEN)return 7;
            else if (cbCardType == CT_NIU_EIGHT)return 8;
            else if (cbCardType == CT_NIU_NINE)return 9;
            else if (cbCardType == CT_NIUNIU)return 10;
            else if (cbCardType == CT_SHUNZI_NIU)return 10;
            else if (cbCardType == CT_WUHUA_NIU)return 10;
            else if (cbCardType == CT_TONGHUA_NIU)return 10;
            else if (cbCardType == CT_HULU_NIU)return 10;
            else if (cbCardType == CT_BOMB_NIU)return 10;
            else if (cbCardType == CT_FIVESMALL_NIU)return 10;
            else if (cbCardType == CT_TONGHUASHUN_NIU)return 10;
            break;
        }
    }    

    return 0;
}

//获取牛牛牌
bool CGameLogic::GetOxCard(uint8_t cbCardData[],uint8_t cbKingChangCard[2])
{
    //设置变量
    uint8_t cbTemp[MAX_COUNT] = { 0 };
    uint8_t cbTempData[MAX_COUNT] = { 0 };
    memcpy(cbTempData, cbCardData, sizeof(cbTempData));

    uint8_t bSum = 0;
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        if (m_bHaveKing)
        {
            if (cbTempData[i]==0x4e && cbKingChangCard[0]!=0) //小王
            {                
                cbTempData[i] = cbKingChangCard[0];
            }
            if (cbTempData[i]==0x4f && cbKingChangCard[1]!=0) //大王
            {
                cbTempData[i] = cbKingChangCard[1];
            }
        }
        
        cbTemp[i] = GetLogicValue(cbTempData[i]);
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
                if (m_bHaveKing)
                {
                    bool bResetXking = false;
                    bool bResetBking = false;
                    for (int id = 0; id < MAX_COUNT; ++id)
                    {
                        if (cbCardData[id]==cbKingChangCard[0] && !bResetXking) //小王
                        {
                            cbCardData[id] = 0x4e;
                            bResetXking = true;
                        }
                        if (cbCardData[id]==cbKingChangCard[1] && !bResetBking) //大王
                        {
                            cbCardData[id] = 0x4f;
                            bResetBking = true;
                        }
                    }                    
                }
                return true;
            }
        }
    }
    return false;
}
//炸弹牛4+1、葫芦牛3+2牌型序
bool CGameLogic::SortCardListBH(uint8_t cbCardData[],int cbCardType)
{
    //设置变量
    uint8_t cbTemp[MAX_COUNT] = { 0 };
    uint8_t cbTempData[MAX_COUNT] = { 0 };
    memcpy(cbTempData, cbCardData, sizeof(cbTempData));
    SortCardList(cbTempData,MAX_COUNT);
    
    uint8_t cardValue = GetCardValue(cbTempData[MAX_COUNT/2]);
    uint8_t bCount = 0;
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        if (cardValue==GetCardValue(cbTempData[i]))
        {
            cbCardData[bCount++] = cbTempData[i];
        }
    }
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        if (cardValue!=GetCardValue(cbTempData[i]))
        {
            cbCardData[bCount++] = cbTempData[i];
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
        //带有王,不能作为出炸弹牛和五花牛之外的特殊牌型
        if (wValue > 4 || cbCardData[i]==0x4e || cbCardData[i]==0x4f)
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
        if (GetCardValue(cbCardData[i]) >= 10)
        {
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
//判断是否为同花顺
bool CGameLogic::IsTierce(uint8_t cbCardData[MAX_COUNT])
{
    bool isStraight = IsStraight(cbCardData);
    bool isFlush = IsFlush(cbCardData);
    if(isStraight && isFlush)
        return true;
    else
        return false;
}
//判断是否为葫芦
bool CGameLogic::IsCalabash(uint8_t cbCardData[MAX_COUNT])
{
    uint8_t cbCardValue[MAX_COUNT];
    memset(cbCardValue, 0, sizeof(cbCardValue));
    for (int i=0; i<MAX_COUNT; i++)
        cbCardValue[i] = GetCardValue(cbCardData[i]); 
    SortCardList(cbCardValue,MAX_COUNT);  
    //带有王,不能作为出炸弹牛和五花牛之外的特殊牌型
    for (int i = 0; i < i; ++i)
    {
        if (cbCardValue[i]==0x4e || cbCardValue[i]==0x4f) 
        {
            return false;
        }
    }
    
    if (cbCardValue[0]==cbCardValue[1] && cbCardValue[2]==cbCardValue[3] && cbCardValue[3]==cbCardValue[4])
    {
        return true;
    }
    else if(cbCardValue[0]==cbCardValue[1] && cbCardValue[1]==cbCardValue[2] && cbCardValue[3]==cbCardValue[4])
    {
        return true;
    }
    // else if(cbCardValue[0]==cbCardValue[1] && cbCardValue[2]==cbCardValue[3] && (cbCardData[4]==0x4e || cbCardData[4]==0x4f))
    // {
    //     return true;
    // }
    else
    {
        return false;
    }
}
//判断是否为同花
bool CGameLogic::IsFlush(uint8_t cbCardData[MAX_COUNT])
{
    int iCount = 0;
    uint8_t cbTempCard[MAX_COUNT] = {0};
    memcpy(cbTempCard, cbCardData, sizeof(cbTempCard));
    SortCardList(cbTempCard, MAX_COUNT);
    //带有王,不能作为出炸弹牛和五花牛之外的特殊牌型
    if (cbTempCard[0]==0x4e || cbTempCard[0]==0x4f) 
    {
        return false;
    }
    uint8_t cbCardColor = GetCardColor(cbCardData[0]);
    for (int i=0; i<MAX_COUNT; i++)
    {
        uint8_t cardColor = GetCardColor(cbCardData[i]);
        if (cbCardColor == cardColor)
            iCount++;
    }
    if(iCount >= MAX_COUNT)
        return true;
    else
        return false;
}
//判断是否为顺子
bool CGameLogic::IsStraight(uint8_t cbCardData[MAX_COUNT])
{
    int iCount = true;
    uint8_t cbTempCard[MAX_COUNT] = {0};
    memcpy(cbTempCard, cbCardData, sizeof(cbTempCard));
    //排序
    SortCardList(cbTempCard, MAX_COUNT);
    //带有王,不能作为出炸弹牛和五花牛之外的特殊牌型
    if (cbTempCard[0]==0x4e || cbTempCard[0]==0x4f) 
    {
        return false;
    }
    uint8_t cbCardValue = GetCardValue(cbTempCard[0]);
    // //判断特殊牌型10JQKA
    // uint8_t cbLastCardValue = GetCardValue(cbTempCard[cbCardCount-1]);
    // if(cbLastCardValue == 0x01)
    // {
    //     bool bSpecial = true;
    //     uint8_t nCardValue = 0x0e;
    //     for (int i=0; i<cbCardCount-1; i++)
    //     {
    //         uint8_t cardValue = GetCardValue(cbTempCard[i]);
    //         if (nCardValue-1 != cardValue)
    //         {
    //             bSpecial = false;
    //             break;
    //         }
    //         nCardValue--;
    //     }
    //     if(bSpecial)
    //         return true;
    // }

    //升序
    for (int i=1; i<MAX_COUNT; i++)
    {
        uint8_t cardValue = GetCardValue(cbTempCard[i]);
        if (cbCardValue != cardValue+1)
        {
            iCount = false;
            break;
        }
        cbCardValue = cardValue;
    }
    if(iCount)
        return true;
    else
        return false;
}
//判断其他牛(无牛、牛1-10)
uint8_t CGameLogic::IsOtherNiu(uint8_t cbCardData[MAX_COUNT])
{    
    uint8_t bTemp[MAX_COUNT];
    uint8_t bSum = 0;
    for (uint8_t i = 0; i < MAX_COUNT; ++i)
    {
        bTemp[i] = GetLogicValue(cbCardData[i]);
        bSum += bTemp[i];
    }
    for (uint8_t i = 0; i < MAX_COUNT - 1; i++)
    {
        for (uint8_t j = i + 1; j < MAX_COUNT; j++)
        {
            if ((bSum - bTemp[i] - bTemp[j]) % 10 == 0)
            {
                uint8_t cbPoint = ((bTemp[i] + bTemp[j]) > 10) ? (bTemp[i] + bTemp[j] - 10) : (bTemp[i] + bTemp[j]);
                return cbPoint;
            }
        }
    }

    return CT_NIU_NULL;
}
//获取玩家最优牌型组合
uint8_t CGameLogic::GetBigTypeCard(uint8_t cbHandCardData[], uint8_t cbCardData[MAX_COUNT])
{
    uint8_t bTempcard[MAX_COUNT] = { 0 };
    uint8_t maxType = 0;
    uint8_t tempType = 0;
    bool bFirsChoose = true;
    int count = 0;  
	uint8_t cbFirstKingChangCard[2] = { 0 };
	uint8_t cbNextKingChangCard[2] = { 0 };
	bool bSelfSixChooseFive = true;
    for (int i = 0; i < MAX_COUNT_SIX; ++i)
    {
        count = 0;
        for (int j = 0; j < MAX_COUNT_SIX; ++j)
        {
            if(i==j) continue;
            bTempcard[count++] = cbHandCardData[j];
        }        
        tempType = GetCardType(bTempcard,MAX_COUNT);
        if (tempType>maxType)
        {
            bFirsChoose=false;
            maxType = tempType;
            memcpy(cbCardData,bTempcard,sizeof(bTempcard));
        }
        else
        {
            if (bFirsChoose)
            {
                bFirsChoose=false;
                memcpy(cbCardData,bTempcard,sizeof(bTempcard));
            }
            else
            {
                if (CompareCard(bTempcard,cbCardData,cbFirstKingChangCard,cbFirstKingChangCard,bSelfSixChooseFive,false))
                {
                    memcpy(cbCardData,bTempcard,sizeof(bTempcard));
                }
            }
        }
    }
    return maxType;
}

//带百变王时手牌的牌型判断
uint8_t CGameLogic::GetHaveKingBigCardType(uint8_t cbHandCardData[], uint8_t cbCardData[MAX_COUNT],uint8_t cbHandCardcount,uint8_t kingChangCard[])
{
    uint8_t bTempcard[MAX_COUNT] = { 0 };
    uint8_t maxType = 0;
    uint8_t tempType = 0;
    bool bFirsChoose = true;
    int count = 0;
    int handCardCount = cbHandCardcount;
    uint8_t kingCount = 0;//百变王的张数
    if (m_bHaveKing)
    {
        for (int i = 0; i < cbHandCardcount; ++i)
        {
            if (cbHandCardData[i]==0x4e || cbHandCardData[i]==0x4f)
            {
                kingCount++;
            }
        }
    }
    if (kingCount>0) //手牌有王
    {        
        maxType = GetCardTypeHaveKing(cbHandCardData, bTempcard, cbHandCardcount, kingCount, kingChangCard);
        memcpy(cbCardData,bTempcard,sizeof(bTempcard));
    }
    else //手牌没王
    {
        if (cbHandCardcount==MAX_COUNT)
        {
            maxType = GetCardType(cbHandCardData, MAX_COUNT);
            memcpy(cbCardData,cbHandCardData,sizeof(uint8_t)*MAX_COUNT);
        }
        else if (cbHandCardcount==MAX_COUNT_SIX)
        {
            maxType = GetBigTypeCard(cbHandCardData, bTempcard);
            memcpy(cbCardData,bTempcard,sizeof(bTempcard));
        }        
    }
    return maxType;
}

//带百变王时手牌的牌型判断
uint8_t CGameLogic::GetCardTypeHaveKing(uint8_t cbHandCardData[], uint8_t cbCardData[MAX_COUNT], uint8_t cbHandCardcount, uint8_t haveKingCount,uint8_t kingChangCard[])
{
    uint8_t bTempcard[MAX_COUNT] = { 0 };
    uint8_t maxType = 0;
    uint8_t tempType = 0;
    bool bFirsChoose = true;
    int count = 0;
    uint8_t handCardCount = cbHandCardcount;
    if (haveKingCount==1)
    {
        maxType = GetCardTypeOneKing(cbHandCardData,bTempcard,cbHandCardcount,kingChangCard);
        memcpy(cbCardData,bTempcard,sizeof(bTempcard));
    }
    else if (haveKingCount==2)
    {
        maxType = GetCardTypeTwoKing(cbHandCardData,bTempcard,cbHandCardcount,kingChangCard);
        memcpy(cbCardData,bTempcard,sizeof(bTempcard));
    }
    
    return maxType;
}

//带1张百变王时的牌型判断
uint8_t CGameLogic::GetCardTypeOneKing(uint8_t cbHandCardData[], uint8_t cbCardData[], uint8_t cbHandCardcount, uint8_t kingChangCard[])
{
    uint8_t cardType = 0;
    uint8_t TempcardType = 0;
    uint8_t maxType = 0;
    uint8_t bTempcard[MAX_COUNT] = { 0 };
    uint8_t bFisrtCard = bTempcard[0];
    uint8_t changcard = 0;
    bool bHaveChooseBestCard = false;
    int searchType = 0;
    uint8_t bTempChooseCard[MAX_COUNT] = { 0 };
    uint8_t bTempChooseKing[2] = { 0 };
    auto searchMaxType = [&](uint8_t bcard[MAX_COUNT],uint8_t cbType,uint8_t cbchangcard)
    {
        if(IsBomb(bcard))
        {
            searchType = CT_BOMB_NIU;
            return CT_BOMB_NIU;
        }  
        if(IsGoldenBull(bcard))
        {
            searchType = CT_WUHUA_NIU;
            return CT_WUHUA_NIU;
        }         
        SortCardList(bcard, MAX_COUNT);  
        searchType = IsOtherNiu(bcard); //王没百变情况下同点之间最大
        for (int i = CT_NIUNIU; i >= CT_NIU_NULL; i--)
        {
            bHaveChooseBestCard = false;
            for (int j = 0; j < 13; ++j)
            {
                bcard[0] = m_byHaveKingCardDataArray[j];
                TempcardType = IsOtherNiu(bcard);
                if (i==TempcardType && TempcardType>searchType)
                {
                    searchType = TempcardType;
                    changcard = 48 + bcard[0];
                    bHaveChooseBestCard = true;
                }
            }
            if (bHaveChooseBestCard)
            {
                break;
            }
        }
    };
    if (cbHandCardcount==MAX_COUNT)
    {
        // int changcard = 0;
        memcpy(bTempcard,cbHandCardData,sizeof(bTempcard));
        memcpy(cbCardData,cbHandCardData,sizeof(uint8_t)*MAX_COUNT);
        searchMaxType(bTempcard,searchType,changcard);
        maxType = searchType;
        int kingIndex = 0;
        for (int i = 0; i < MAX_COUNT; ++i)
        {
            if(cbHandCardData[i]==0x4f)
            {
                kingIndex=1;
                break;
            }
        }
        kingChangCard[kingIndex] = changcard;
    }
    else if (cbHandCardcount==MAX_COUNT_SIX)
    {
        int count = 0;  
        int chooseKing = 0;
        bool bFirsChoose = true;
        // int changcard = 0;
        int kingIndex = 0;
        memset(bTempChooseCard,0,sizeof(bTempChooseCard));
        memset(bTempChooseKing,0,sizeof(bTempChooseKing));
        // SortCardListSix(cbHandCardData, MAX_COUNT_SIX); 
        for (int i = 0; i < MAX_COUNT_SIX; ++i)
        {
            count = 0;
            chooseKing = 0;
            kingIndex = 0;
            for (int j = 0; j < MAX_COUNT_SIX; ++j)
            {
                if(i==j) continue;
                bTempcard[count++] = cbHandCardData[j];
                if (cbHandCardData[j] == 0x4e || cbHandCardData[j] == 0x4f)
                {
                    chooseKing++;
                }
                if (cbHandCardData[j] == 0x4f)
                {
                    kingIndex = 1;
                }
            }    
            memcpy(bTempChooseCard,bTempcard,sizeof(bTempChooseCard));         
            if (chooseKing==1) //选到一张王
            {
                changcard = 0;
                searchMaxType(bTempcard,searchType,changcard);
                if (searchType>maxType)
                {
                    bFirsChoose = false;
                    maxType = searchType;
                    bTempChooseKing[kingIndex] = changcard;
                    memcpy(cbCardData,bTempChooseCard,sizeof(bTempChooseCard));
                }
                else if (searchType==maxType) //同点、判断王是否百变
                {
                    uint8_t btempCurCard[2] = {0};
                    btempCurCard[kingIndex] = changcard;
                    if (CompareCard(bTempChooseCard,cbCardData,btempCurCard,bTempChooseKing,true,false)) //王没百变同点之间最大
                    {
                        bFirsChoose = false;
                        maxType = searchType;
                        bTempChooseKing[kingIndex] = changcard;
                        memcpy(cbCardData,bTempChooseCard,sizeof(bTempChooseCard));
                    }
                }
            }
            else //不选到王
            {
                int tempType = GetCardType(bTempcard,MAX_COUNT);
                uint8_t notChooseKing[2] = {0};
                if (tempType>maxType)
                {
                    bFirsChoose=false;
                    maxType = tempType;
                    memcpy(bTempChooseKing,notChooseKing,sizeof(bTempChooseKing));
                    memcpy(cbCardData,bTempcard,sizeof(bTempcard));
                }
                else
                {
                    if (bFirsChoose)
                    {
                        bFirsChoose=false;
                        memcpy(bTempChooseKing,notChooseKing,sizeof(bTempChooseKing));
                        memcpy(cbCardData,bTempcard,sizeof(bTempcard));
                    }
                    else
                    {
                        if (CompareCard(bTempcard,cbCardData,notChooseKing,bTempChooseKing,true,false))
                        {
                            memcpy(bTempChooseKing,notChooseKing,sizeof(bTempChooseKing));
                            memcpy(cbCardData,bTempcard,sizeof(bTempcard));
                        }
                    }
                }
            }
        }
        memcpy(kingChangCard,bTempChooseKing,sizeof(bTempChooseKing));
    }
    return maxType;
}
//带2张百变王时的牌型判断
uint8_t CGameLogic::GetCardTypeTwoKing(uint8_t cbHandCardData[], uint8_t cbCardData[], uint8_t cbHandCardcount,uint8_t kingChangCard[])
{
    uint8_t cardType = 0;
    uint8_t maxType = 0;
    uint8_t bTempcard[MAX_COUNT] = { 0 };
    int searchType = 0;
    uint8_t changcard[2] = {0};    
    uint8_t tempChangcard[2] = {0};     
    auto searchMaxType = [&](uint8_t bcard[MAX_COUNT],int cbType,uint8_t cbChangcard[2])
    {
        SortCardList(bcard, MAX_COUNT);
        uint8_t cbCount = 0;
        //两个王，变牌后只能作为五花牛,或者是普通牛,只做五花牛和普通牛的判断
        for (uint8_t i = 2; i < MAX_COUNT; ++i)
        {
            if (GetCardValue(bcard[i]) >= 10)
            {
                ++cbCount;
            }
        }
        if (3 == cbCount) //五花牛
        {
            searchType = CT_WUHUA_NIU;
        }
        else //普通牛
        {
            searchType = (int)IsOtherNiu(bcard); //王没百变情况下同点之间最大
            if (searchType==CT_NIUNIU)
            {
                searchType = CT_NIUNIU;
            }
            else
            {
                for (int i = 0; i < 13; ++i)
                {
                    bcard[1] = m_byHaveKingCardDataArray[i]; //小王的变牌值
                    int TempcardType = (int)IsOtherNiu(bcard);
                    if (CT_NIUNIU==TempcardType)
                    {
                        searchType = TempcardType;
                        changcard[0] = 48 + bcard[1]; //保存小王的变牌值
                        tempChangcard[0] = 48 + bcard[1]; //保存小王的变牌值
                    }  
                }  
                if (CT_NIUNIU!=searchType) //只变一张王,不能构成牛牛时,就要变两张
                {
                    for (int i = 0; i < 13; ++i)
                    {
                        bcard[1] = m_byHaveKingCardDataArray[i]; //小王的变牌值
                        for (int j = 0; j < 13; ++j)
                        {
                            bcard[0] = m_byHaveKingCardDataArray[j]; //大王的变牌值
                            int TempcardType = (int)IsOtherNiu(bcard);
                            if (CT_NIUNIU==TempcardType)
                            {
                                searchType = TempcardType;
                                changcard[1] = 48 + bcard[0]; //保存大王的变牌值
                                changcard[0] = 48 + bcard[1]; //保存小王的变牌值
                                tempChangcard[1] = 48 + bcard[0]; //保存大王的变牌值
                                tempChangcard[0] = 48 + bcard[1]; //保存小王的变牌值
                            }
                        } 
                    }    
                }  
            }
        }
    };
    if (cbHandCardcount==MAX_COUNT)
    {
        memcpy(bTempcard,cbHandCardData,sizeof(bTempcard));
        memcpy(cbCardData,cbHandCardData,sizeof(bTempcard));        
        searchMaxType(bTempcard,searchType,changcard);
        maxType = searchType;
        memcpy(kingChangCard,changcard,sizeof(changcard));
    }
    else if (cbHandCardcount==MAX_COUNT_SIX)
    {
        int count = 0;   
        int chooseKing = 0;
        uint8_t kingcard = 0;
        uint8_t oneKingChangCard[2] = {0};//单张大王时的变牌
        uint8_t twoKingChangCard[2] = {0}; //两张王时的变牌       
        uint8_t tempHandCardData[MAX_COUNT_SIX] = {0};
        memcpy(tempHandCardData,cbHandCardData,sizeof(tempHandCardData));
        SortCardListSix(tempHandCardData,MAX_COUNT_SIX);
        uint8_t value = GetCardValue(tempHandCardData[MAX_COUNT_SIX/2]);
        int sameValuecount = 0;
        bool bChooseTwoking = false;
        bool bChooseOneking = false;        
        uint8_t cbTmpMaxChangCard[MAX_COUNT] = {0};
        for (int i = 2; i < MAX_COUNT_SIX; ++i)
        {
            if (GetLogicValue(tempHandCardData[i])>=10)
            {
                count++;
            }
            if (GetCardValue(tempHandCardData[i])==value)
            {
                sameValuecount++;
            }
        }
        if (sameValuecount==4) //炸弹牛
        {
            int index = 0;
            for (int i = 0; i < MAX_COUNT_SIX; ++i)
            {
                if (cbHandCardData[i]!=0x4e)
                {
                    bTempcard[index++] = cbHandCardData[i];
                }
            }
            maxType = CT_BOMB_NIU;
            memcpy(cbCardData,bTempcard,sizeof(bTempcard));
            return maxType;
        }
        if (count>=3)  //五花牛
        {
            int index = 0;
            for (int i = 0; i < MAX_COUNT_SIX; ++i)
            {
                if (cbHandCardData[i]!=tempHandCardData[5])
                {
                    bTempcard[index++] = cbHandCardData[i];
                }
            }
            maxType = CT_WUHUA_NIU;
            memcpy(cbCardData,bTempcard,sizeof(bTempcard));
            return maxType;
        }
        //不是炸弹牛和五花牛的话,最大只能是牛牛了
        for (int i = 0; i < MAX_COUNT_SIX; ++i)
        {
            count = 0;
            chooseKing = 0;
            kingcard = 0;
            for (int j = 0; j < MAX_COUNT_SIX; ++j)
            {
                if(i==j) continue;
                bTempcard[count++] = cbHandCardData[j];
                if (cbHandCardData[j] == 0x4e || cbHandCardData[j] == 0x4f)
                {
                    chooseKing++;
                    kingcard = cbHandCardData[j];
                }
            } 
            uint8_t cbMaxCard[MAX_COUNT]={0};
            //王百变参与比牌:
            //1、王百变的情况下同点之间未最小;
            //2、王没百变的情况下同点之间最大;
            //3、同王百变牛牛的情况下或同王不变牛牛情况下,默认大王赢;
            //4、同拥有王牌的五花牛情况下，默认大王赢;
            if (chooseKing==1 && kingcard==0x4f) //选到一张王时,默认大王赢
            {
                memset(tempChangcard,0,sizeof(tempChangcard));
                cardType = GetCardTypeOneKing(bTempcard,cbMaxCard,5,tempChangcard); 
                bChooseOneking = true;
                if (cardType==CT_NIUNIU && maxType==0)
                {
                    maxType = cardType;
                    oneKingChangCard[1] = tempChangcard[1];
                    memcpy(cbCardData,cbMaxCard,sizeof(cbMaxCard));  
                    memcpy(cbTmpMaxChangCard,bTempcard,sizeof(bTempcard));
                }  
                else if (cardType==CT_NIUNIU) 
                {
                    //已经存在有两张王的牛牛了
                    if (twoKingChangCard[0]!=0 && twoKingChangCard[1]!=0) //大小王都变牌时,是最小的牛牛了(这里感觉应该是不会进来的)
                    {
                        maxType = cardType;
                        oneKingChangCard[0] = 0;
                        oneKingChangCard[1] = tempChangcard[1];
                        twoKingChangCard[0] = 0;
                        twoKingChangCard[1] = tempChangcard[1];
                        memcpy(cbCardData,cbMaxCard,sizeof(cbMaxCard));
                        memcpy(cbTmpMaxChangCard,bTempcard,sizeof(bTempcard));
                    } 
                }
            }
            else if (chooseKing==2) //选到两张王
            {     
                uint8_t cbTempChangCard[MAX_COUNT]={0};
                memset(tempChangcard,0,sizeof(tempChangcard));
                memcpy(cbTempChangCard,bTempcard,sizeof(bTempcard)); 
                searchMaxType(bTempcard,searchType,tempChangcard);
                if (searchType==CT_NIUNIU && maxType==0)
                {
                    maxType = searchType;
                    memcpy(twoKingChangCard,tempChangcard,sizeof(tempChangcard));
                    memcpy(cbCardData,cbTempChangCard,sizeof(cbTempChangCard)); 
                    memcpy(cbTmpMaxChangCard,cbTempChangCard,sizeof(cbTempChangCard));
                    bChooseTwoking = true;
                }
                else if (searchType == CT_NIUNIU)
                {
                    if (bChooseOneking)
                    {
                        if ((tempChangcard[0]==0 && tempChangcard[1]==0) || (tempChangcard[0]!=0 && tempChangcard[1]==0)||(tempChangcard[0]==0 && tempChangcard[1]!=0))
                        {
                            if (bChooseTwoking)
                            {
                                if (CompareCard(cbTempChangCard,cbCardData,tempChangcard,twoKingChangCard,true,false))
                                {
                                    maxType = searchType;
                                    memcpy(twoKingChangCard,tempChangcard,sizeof(tempChangcard));                    
                                    memcpy(cbCardData,cbTempChangCard,sizeof(cbTempChangCard)); 
                                    memcpy(cbTmpMaxChangCard,cbTempChangCard,sizeof(cbTempChangCard));                         
                                }
                            }
                            else
                            {
                                maxType = searchType;
                                memcpy(twoKingChangCard,tempChangcard,sizeof(tempChangcard));                    
                                memcpy(cbCardData,cbTempChangCard,sizeof(cbTempChangCard)); 
                                memcpy(cbTmpMaxChangCard,cbTempChangCard,sizeof(cbTempChangCard));
                                bChooseTwoking = true;
                            }
                        } 
                        else
                        {
                            if (CompareCard(cbTempChangCard,cbCardData,tempChangcard,twoKingChangCard,true,false))
                            {
                                maxType = searchType;
                                memcpy(twoKingChangCard,tempChangcard,sizeof(tempChangcard));                    
                                memcpy(cbCardData,cbTempChangCard,sizeof(cbTempChangCard)); 
                                memcpy(cbTmpMaxChangCard,cbTempChangCard,sizeof(cbTempChangCard));                         
                            }
                        }                       
                    }                   
                    else
                    {
                        if (bChooseTwoking)
                        {
                            if (CompareCard(cbTempChangCard,cbCardData,tempChangcard,twoKingChangCard,true,false))
                            {
                                maxType = searchType;
                                memcpy(twoKingChangCard,tempChangcard,sizeof(tempChangcard));                    
                                memcpy(cbCardData,cbTempChangCard,sizeof(cbTempChangCard)); 
                                memcpy(cbTmpMaxChangCard,cbTempChangCard,sizeof(cbTempChangCard));                         
                            }
                        }
                        else
                        {
                            maxType = searchType;
                            memcpy(twoKingChangCard,tempChangcard,sizeof(tempChangcard));                     
                            memcpy(cbCardData,cbTempChangCard,sizeof(cbTempChangCard));
                            memcpy(cbTmpMaxChangCard,cbTempChangCard,sizeof(cbTempChangCard)); 
                            bChooseTwoking = true;
                        }
                    }
                }
            }
        } 
        memcpy(kingChangCard,twoKingChangCard,sizeof(twoKingChangCard));
        memcpy(cbCardData,cbTmpMaxChangCard,sizeof(cbTmpMaxChangCard));      
    }

    return maxType;
}

//检验手牌牌值是否有效
bool CGameLogic::CheckHanCardIsOK(uint8_t cbHandCardData[], uint8_t cardcount)
{
	for (int i = 0; i < cardcount; ++i)
	{
		if (cbHandCardData[i] == 0)
        {
			return false;
		}
	}

	return true;
}
