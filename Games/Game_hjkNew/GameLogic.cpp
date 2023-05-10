#include "GameLogic.h"
#include <time.h>
#include <algorithm>
#include "math.h"
#include <sys/time.h>
#include <glog/logging.h>

#include "public/GlobalFunc.h"

//#include <random>
//#include <random/RandomMT32.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////////
//扑克数据
uint8_t CGameLogic::m_cbCardListData[MAX_CARD_TOTAL] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D	//黑桃 A - K
};

//////////////////////////////////////////////////////////////////////////////////

//构造函数
CGameLogic::CGameLogic()
{

}

//析构函数
CGameLogic::~CGameLogic()
{

}

//获取类型
//uint8_t CGameLogic::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount)
//{
//    assert(cbCardCount == MAX_COUNT);

//    return CT_BUST;
//}

//排列扑克
void CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount)
{
    //转换数值
    uint8_t cbLogicValue[MAX_COUNT];
    for(uint8_t i = 0; i < cbCardCount; i++)
        cbLogicValue[i] = GetCardLogicValue(cbCardData[i]);

    //排序操作
    bool bSorted = true;
    uint8_t cbTempData, bLast = cbCardCount - 1;
    do
    {
        bSorted = true;
        for(uint8_t i = 0; i < bLast; i++)
        {
            if((cbLogicValue[i] < cbLogicValue[i + 1]) ||
                    ((cbLogicValue[i] == cbLogicValue[i + 1]) && (cbCardData[i] < cbCardData[i + 1])))
            {
                //交换位置
                cbTempData = cbCardData[i];
                cbCardData[i] = cbCardData[i + 1];
                cbCardData[i + 1] = cbTempData;
                cbTempData = cbLogicValue[i];
                cbLogicValue[i] = cbLogicValue[i + 1];
                cbLogicValue[i + 1] = cbTempData;
                bSorted = false;
            }
        }
        bLast--;
    }while(!bSorted);

    return;
}

//todo 人不一定是满的，但是一定是有序的
//todo 需要保存当前这副牌
//混乱扑克 (洗牌)
void CGameLogic::RandCardList(vector<uint8_t> &cbCardData)
{
//    vector<uint8_t> cbCardData;
    cbCardData.resize(MAX_CARD_TOTAL*USE_COUNT);
    for(int i = 0; i < USE_COUNT; i++ )
    {
        memcpy(&cbCardData[ i * MAX_CARD_TOTAL], m_cbCardListData, MAX_CARD_TOTAL);
    }
    
    random_shuffle(cbCardData.begin(), cbCardData.end());
    /*
    //发牌算法
    uint8_t bRandCount = 0;
    uint8_t index,a,b;
    while(bRandCount < cbBufferCount)
    {
        a = bRandCount%2;
        b = 0 << bRandCount/2;
        index = a + b*MAX_COUNT;
        cbCardBuffer[index] = cbCardData.back();
        cbCardData.pop_back();
        bRandCount++;
    }

    return cbCardData;
    */
}

//逻辑数值
uint8_t CGameLogic::GetCardLogicValue(uint8_t cbCardData)
{
    //扑克属性
    //BYTE bCardColor = GetCardColor(cbCardData);
    uint8_t bCardValue = GetCardValue(cbCardData);

    //转换数值
    return (bCardValue > 10) ? 10 : bCardValue;
}

//对比扑克
float CGameLogic::CompareCard(shared_ptr<Hand> &banker,shared_ptr<Hand> &player)
{
    //庄家永远是赢1倍
    float bankerMultiple = 1;//GetCardTypeMultiple(banker->cardtype);
    float playerMultiple = GetCardTypeMultiple(player->cardtype);
    if(player->cardtype == CT_BUST)
    {
        return bankerMultiple;
    }
    if(banker->cardtype == CT_BUST)
    {
        return -playerMultiple;
    }

    if(player->cardtype != banker->cardtype)
    {
        return player->cardtype > banker->cardtype ? -playerMultiple : bankerMultiple;
    }else
    {
        if(player->cardtype == CT_POINTS)
        {
            if( player->cardpoint != banker->cardpoint)
            {
                return player->cardpoint > banker->cardpoint ? -1 : 1;
            }else
            {
                return 0.0;
            }
        }else{
            return 0.0;
        }
    }
    return 0.0;
}

bool CGameLogic::UpdateAddOne(shared_ptr<Hand>  &hand, uint8_t carddata)
{
    uint8_t point = GetCardLogicValue(carddata);
    hand->cardpoint += point;
    hand->hasA = hand->hasA || point == 1;

    if( hand->hasA && hand->cardpoint + 10 > 21)
    {
        hand->hasA = false;
    }
    
    if(hand->cardpoint > 21)
    {
        hand->stopped = true;
        hand->cardtype = CT_BUST;
        hand->opercode = 0;
    }
    else if(hand->cardcount != 5)
    {
        if( hand->cardpoint == 21)//(hand->hasA && hand->cardpoint == 11)
        {
//            hand->cardpoint = 21;
            hand->stopped = true;
            hand->cardtype = CT_POINTS;
            hand->opercode = 0;
        }else
        {
            hand->opercode = OPER_CALL | OPER_STOP;
        }
    }else
    {
        hand->stopped = true;
        hand->cardtype = CT_FIVE_DRAGON;
        hand->opercode = 0;
    }

    if(hand->isbanker && hand->hasA)
    {
        if(hand->cardpoint + 10 >= 17 && hand->cardpoint + 10 <= 21)
        {
            hand->cardpoint = hand->cardpoint + 10;
            hand->stopped = true;
        }
    }
    return false;
}

void CGameLogic::UpdateInit(shared_ptr<Hand> &hand,uint8_t carddata[])
{
    uint8_t point;
    hand->cardpoint = 0;
    hand->hasA = false;
    hand->stopped = false;
    for(uint i = 0; i < hand->cardcount; i++)
    {
        point = GetCardLogicValue(carddata[i]);
        hand->cardpoint += point;
        hand->hasA = hand->hasA || point == 1;
    }

    if(hand->cardpoint == 11 && hand->hasA && !hand->issplited && !hand->issecond)
    {
//        if(!hand->issplited && !hand->issecond)
//        {
            hand->cardtype = CT_BLACKJACK;
//        }else
//        {
//            hand->cardtype = CT_POINTS;
//            hand->cardpoint = 21;
//        }
        hand->stopped = true;
    }else if(hand->cardpoint != 21)
    {
        hand->opercode = OPER_STOP | OPER_CALL | OPER_DOUBLE;
        hand->cardtype = CT_POINTS;
        if( GetCardLogicValue(carddata[0]) == GetCardLogicValue(carddata[1]))
        {
            if( !hand->issecond && !hand->issplited)
            {
                 hand->opercode = hand->opercode | OPER_SPLIT;
            }
        }
    }

    if(hand->isbanker && hand->hasA)
    {
        if(hand->cardpoint + 10 >= 17 && hand->cardpoint + 10 <= 21)
        {
            hand->cardpoint = hand->cardpoint + 10;
            hand->stopped = true;
        }
    }
}
/*
//reduce the special card item.
bool CGameLogic::recude27Cards(vector<uint8_t>& cbCardData, int reduceRatio)
{
    bool bSuccess = false;
    vector<uint8_t> vecTemp1, vecTemp2;
    do
    {
        // check if the input data and reduce ratio value.
        if(!cbCardData.size() || reduceRatio == 0)
        {
            break;
        }

        // get the data value.
        vecTemp1 = cbCardData;
        for(int i = 0; i < vecTemp1.size(); ++i)
        {
            int val = GetCardValue(vecTemp1[i]);
            if((val >= 2) && (val <= 7))
            {
                // check if skip current card item now.
                int fr = GlobalFunc::RandomInt64(0, 100);
                if(fr < reduceRatio)
                {
                    vecTemp1[i] = 0;
                }
            }
            // push the value now.
            if(vecTemp1[i] != 0)
            {
                vecTemp2.push_back(vecTemp1[i]);
            }
        }

        // return the value now.
        cbCardData = vecTemp2;
        bSuccess = true;
    }while(0);
    //Cleanup:
    return (bSuccess);
}
*/

//////////////////////////////////////////////////////////////////////////////////
