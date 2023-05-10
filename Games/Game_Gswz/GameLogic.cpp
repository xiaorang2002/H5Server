#include "GameLogic.h"
#include <time.h>
#include <algorithm>
#include "math.h"
#include <sys/time.h>
#include <glog/logging.h>
#include <iostream>
//#include "public/GlobalFunc.h"

//#include <random>
//#include <random/RandomMT32.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////////
//扑克数据
uint8_t CGameLogic::m_cbCardListData[MAX_CARD_TOTAL] =
{
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x01,	//方块 A - K
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x11,	//梅花 A - K
    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x21,	//红桃 A - K
    0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x31	//黑桃 A - K
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
uint8_t CGameLogic::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount)
{
    assert(cbCardCount > 0 && cbCardCount < 6);

    vector<uint8_t> cardBuff;
    cardBuff.resize(cbCardCount); //
    memcpy(&cardBuff[0],cbCardData,cbCardCount);
    SortCardList(cardBuff);

    bool bSameFlower = false;
    bool bLink = false;
    switch (cbCardCount)
    {
        case 2:
            if(isDouble(cardBuff.begin(),cbCardCount))
                return CT_DOUBLE;
        break;
        case 3 :
            if(isThird(cardBuff.begin(),cbCardCount))
                return CT_THIRD;
            else if(isDouble(cardBuff.begin(),cbCardCount))
                return CT_DOUBLE;
        break;
        case 4:
            bSameFlower=isSameFlower(cardBuff.begin(),cbCardCount);
            bLink=isLink(cardBuff.begin(),cbCardCount);

            if(bSameFlower && bLink)
                return CT_SAME_FLOWER_LINK;
            else if(isFour(cardBuff.begin(),cbCardCount))
                return CT_FOUR;
            else if(bSameFlower)
                return CT_SAME_FLOWER;
            else if(bLink)
                return CT_LINK;
            else if(isThird(cardBuff.begin(),cbCardCount))
                return CT_THIRD;
            else if(isTwoDouble(cardBuff.begin(),cbCardCount))
                return CT_TWO_DOUBLE;
            else if(isDouble(cardBuff.begin(),cbCardCount))
                return CT_DOUBLE;
        break;
        case 5 :
            bool bSameFlower =isSameFlower(cardBuff.begin(),cbCardCount);
            bool bLink=isLink(cardBuff.begin(),cbCardCount);
            if(bSameFlower && bLink)
            {
                if(GetCardLogicValue(cardBuff[4]) ==10)
                    return CT_MAX_SAME_FLOWER_LINK;

                return CT_SAME_FLOWER_LINK;
            }
            else if(isFour(cardBuff.begin(),cbCardCount))
                return CT_FOUR;
            else if(bSameFlower)
                return CT_SAME_FLOWER;
            else if (isCalabash(cardBuff.begin(),cbCardCount))
                return CT_CALABASH;
            else if(bLink)
                return CT_LINK;
            else if(isThird(cardBuff.begin(),cbCardCount))
                return CT_THIRD;
            else if(isTwoDouble(cardBuff.begin(),cbCardCount))
                return CT_TWO_DOUBLE;
            else if(isDouble(cardBuff.begin(),cbCardCount))
                return CT_DOUBLE;

        break;
    };
    return CT_SINGLE;
}

//排列扑克
void CGameLogic::SortCardList(vector<uint8_t> &cards)
{
    //转换数值
    std::sort(cards.begin(),cards.end(),[this](uint8_t a,uint8_t b)
    {
        return CompareSingleCard(a , b);
    });
    return;
}

//混乱扑克 (洗牌)
void CGameLogic::RandCardList()
{
    m_vecCardBuffer.clear();
    m_vecCardBuffer.resize(MAX_CARD_TOTAL);
    memcpy(&m_vecCardBuffer[0], m_cbCardListData, MAX_CARD_TOTAL);
    //random_shuffle(cbCardData.begin(), cbCardData.end());
    int randomIndex= 0;
    int replaceIndex=0;
    while(randomIndex<MAX_CARD_TOTAL)
    {
        replaceIndex=rand()%MAX_CARD_TOTAL;
        swap(m_vecCardBuffer[randomIndex],m_vecCardBuffer[replaceIndex]);
        randomIndex++;
    }

}

void CGameLogic::GetHandCards(uint8_t cbHandCards[], uint8_t count)
{
    for(int i = 0 ; i <count; i++)
    {
        cbHandCards[i]=m_vecCardBuffer.back();
        m_vecCardBuffer.pop_back();
    }
}

//逻辑数值
uint8_t CGameLogic::GetCardLogicValue(uint8_t cbCardData)
{
    //扑克属性
    uint8_t bCardValue = GetCardValue(cbCardData);

    //转换数值
    return (bCardValue == 1) ? (bCardValue + 13) : bCardValue;
}

//对比扑克
uint8_t CGameLogic::CompareCard(uint8_t cbFirstData[], uint8_t cbNextData[], uint8_t cbCardCount, uint8_t bCompareColor/* = 0*/)
{
    //设置变量
    uint8_t firstType=GetCardType(cbFirstData,cbCardCount);
    uint8_t secondType=GetCardType(cbNextData,cbCardCount);
    //printCardType(firstType);
    //printCardType(secondType);
    if(secondType==firstType)
    {
        vector<uint8_t> firstData,secondData;
        firstData.resize(cbCardCount);
        secondData.resize(cbCardCount);
        memcpy(&firstData[0],cbFirstData,cbCardCount);
        memcpy(&secondData[0],cbNextData,cbCardCount);
        SortCardList(firstData);
        SortCardList(secondData);
        switch (firstType) {
        case CT_SINGLE:
        case CT_SAME_FLOWER:
            return  compareSingle(firstData.begin(),secondData.begin(),cbCardCount);
        case CT_DOUBLE:
            return compareRepeat(firstData.begin(),secondData.begin(),2,cbCardCount);
        case CT_THIRD:
        case CT_CALABASH:
            return compareRepeat(firstData.begin(),secondData.begin(),3,cbCardCount);
        case CT_FOUR:
            return compareRepeat(firstData.begin(),secondData.begin(),4,cbCardCount);
        case CT_LINK:
        case CT_SAME_FLOWER_LINK:
        case CT_MAX_SAME_FLOWER_LINK:
            return compareLink(firstData.begin(),secondData.begin(),cbCardCount);
        case CT_TWO_DOUBLE:
            return compareTwoDouble(firstData.begin(),secondData.begin(),cbCardCount);
        default:
            break;
        }

    }else
        return firstType>secondType;


}

bool CGameLogic::CompareSingleCard(uint8_t first, uint8_t second)
{
    uint8_t a=GetCardLogicValue(first);
    uint8_t b=GetCardLogicValue(second);
    if(a == b)
        return first > second;
    else
        return a> b;
}

void CGameLogic::GetMaxCards(uint8_t cbHandCards[], uint8_t cardBuffer[], uint8_t cardCount, uint8_t cards[])
{


}

void CGameLogic::GetSingle(uint8_t cbCardBuffer[],uint8_t cbCard[])
{
    int tindex = 0;
    while (true) {

        tindex++;
        if(tindex>100) return;
        vector<uint8_t> cbCardData;
        cbCardData.resize(MAX_CARD_TOTAL);

        memcpy(&cbCardData[0], m_cbCardListData, MAX_CARD_TOTAL);
        random_shuffle(cbCardData.begin(), cbCardData.end());

        //混乱扑克
        uint8_t bRandCount = 0;
        while(bRandCount < 1)
        {
            uint8_t card = cbCardData.back();
//            if(card == 0) continue;
            cbCardBuffer[bRandCount++] = card;//cbCardData.back();
            cbCardData.pop_back();
        }
        while(bRandCount < MAX_COUNT)
        {
            uint8_t card = cbCardData.back();
            if(GetCardLogicValue(card) == GetCardLogicValue(cbCardBuffer[0]))
            {
                cbCardBuffer[bRandCount++] = card;
            }
            cbCardData.pop_back();
        }

        if(bRandCount == MAX_COUNT) return;
    }
}

void CGameLogic::GetCalabash(uint8_t cbCardBuffer[], uint8_t cbCard[])
{

}
void CGameLogic::GetSameFlowerLink(uint8_t cbCardBuffer[],uint8_t cbCard[])
{
    int tindex = 0;
    while (true) {

        tindex++;
        if(tindex>100) return;
        vector<uint8_t> cbCardData;
        cbCardData.resize(MAX_CARD_TOTAL);
        memcpy(&cbCardData[0], m_cbCardListData, MAX_CARD_TOTAL);
        random_shuffle(cbCardData.begin(), cbCardData.end());


        //混乱扑克
        uint8_t bRandCount = 0;
        while(bRandCount < 1)
        {
            uint8_t card = cbCardData.back();
            if(GetCardLogicValue(card) >12 || card ==0) {
                cbCardData.pop_back();
                continue;
            }
            cbCardBuffer[bRandCount++] = card;//cbCardData.back();
            cbCardData.pop_back();
        }
        while(bRandCount < MAX_COUNT)
        {
            uint8_t card = cbCardData.back();
            if((GetCardLogicValue(card) == (GetCardLogicValue(cbCardBuffer[0])+1) ||GetCardLogicValue(card) == (GetCardLogicValue(cbCardBuffer[0])+2))
                    && GetCardColor(card) == GetCardColor(cbCardBuffer[0]))
            {
                cbCardBuffer[bRandCount++] = card;
            }
            cbCardData.pop_back();
        }
        if(bRandCount == MAX_COUNT) return;
    }
}
void CGameLogic::GetSameFlower(uint8_t cbCardBuffer[],uint8_t cbCard[])
{
    int tindex = 0;
    while (true) {

        tindex++;
        if(tindex>100) return;
        vector<uint8_t> cbCardData;
        cbCardData.resize(MAX_CARD_TOTAL);

        memcpy(&cbCardData[0], m_cbCardListData, MAX_CARD_TOTAL);
        random_shuffle(cbCardData.begin(), cbCardData.end());


        //混乱扑克
        uint8_t bRandCount = 0;
        while(bRandCount < 1)
        {
            uint8_t card = cbCardData.back();
            if(card ==0)
            {
                cbCardData.pop_back();
                continue;
            }
            cbCardBuffer[bRandCount++] = card;//cbCardData.back();
            cbCardData.pop_back();
        }
        while(bRandCount < MAX_COUNT)
        {
            uint8_t card = cbCardData.back();
            if(GetCardColor(card) == GetCardColor(cbCardBuffer[0]))
            {
                cbCardBuffer[bRandCount++] = card;
            }
            cbCardData.pop_back();
        }

        if(bRandCount == MAX_COUNT) return;
    }
}
void CGameLogic::GetLink(uint8_t cbCardBuffer[],uint8_t cbCard[])
{
    int tindex = 0;
    while (true) {

        tindex++;
        if(tindex>100) return;
        vector<uint8_t> cbCardData;
        cbCardData.resize(MAX_CARD_TOTAL);

        memcpy(&cbCardData[0], m_cbCardListData, MAX_CARD_TOTAL);
        random_shuffle(cbCardData.begin(), cbCardData.end());


        //混乱扑克
        uint8_t bRandCount = 0;
        while(bRandCount < 1)
        {
            uint8_t card = cbCardData.back();
            if(GetCardLogicValue(card) >12 || card ==0)
            {
                cbCardData.pop_back();
                continue;
            }
            cbCardBuffer[bRandCount++] = card;//cbCardData.back();
            cbCardData.pop_back();
        }
        while(bRandCount < MAX_COUNT)
        {
            uint8_t card = cbCardData.back();
            if(GetCardLogicValue(card) == (GetCardLogicValue(cbCardBuffer[0])+bRandCount)
                    && GetCardColor(card) != GetCardColor(cbCardBuffer[0]))
            {
                cbCardBuffer[bRandCount++] = card;
            }
            cbCardData.pop_back();
        }

        if(bRandCount == MAX_COUNT) return;
    }
}
void CGameLogic::GetDouble(uint8_t cbCardBuffer[],uint8_t cbCard[])
{

}

void CGameLogic::GetThird(uint8_t cbCardBuffer[], uint8_t cbCard[])
{

}

void CGameLogic::GetTwoDouble(uint8_t cbCardBuffer[], uint8_t cbCard[])
{

}

void CGameLogic::GetFour(uint8_t cbCardBuffer[],uint8_t cbCard[])
{

}

bool CGameLogic::compareSingle(SortedBuffer sortedFirstCards, SortedBuffer sortedSecondCards, uint8_t cardCount)
{
    assert(cardCount>0 &&cardCount<6);
    int compareCount=0;
    while(compareCount<cardCount)
    {
        if (GetCardLogicValue(*sortedFirstCards)>GetCardLogicValue(*sortedSecondCards))
            return true;
        if (GetCardLogicValue(*sortedFirstCards)<GetCardLogicValue(*sortedSecondCards))
            return false;
        sortedFirstCards++;
        sortedSecondCards++;
        compareCount ++;
    }
    return *(sortedFirstCards-compareCount)>*(sortedSecondCards-compareCount);
}

bool CGameLogic::compareRepeat(SortedBuffer sortedFirstCards,SortedBuffer sortedSecondCards,uint8_t repeatCount, uint8_t cardCount)
{
    vector<uint8_t> firstCards;
    vector<uint8_t> secondCards;
    uint8_t tmpFirstCard=*sortedFirstCards,tmpSecondCard=*sortedSecondCards;
    //先得出对牌，再依次比较剩下的牌

    uint8_t firstRepeatCard,SecondRepeatCard;
    if(repeatCount == 2)
    {
        firstRepeatCard=isDouble(sortedFirstCards,cardCount);
        SecondRepeatCard=isDouble(sortedSecondCards,cardCount);
    }
    if(repeatCount == 3)
    {
        firstRepeatCard=isThird(sortedFirstCards,cardCount);
        SecondRepeatCard=isThird(sortedSecondCards,cardCount);
    }
    if(repeatCount == 4)
    {
        firstRepeatCard=isFour(sortedFirstCards,cardCount);
        SecondRepeatCard=isFour(sortedSecondCards,cardCount);
    }
    if (GetCardLogicValue(firstRepeatCard) == GetCardLogicValue(SecondRepeatCard))
    {
        for(uint8_t i=0;i<cardCount;++i)
        {
            if(GetCardLogicValue(firstRepeatCard)!=GetCardLogicValue(*(sortedFirstCards+i)))
                firstCards.push_back(*(sortedFirstCards+i));

            if(GetCardLogicValue(SecondRepeatCard)!=GetCardLogicValue(*(sortedSecondCards+i)))
                secondCards.push_back(*(sortedSecondCards+i));
        }
        for(uint8_t i=0;i<firstCards.size();++i)
        {
            if (GetCardLogicValue(firstCards[i])>GetCardLogicValue(secondCards[i]))
                return true;
            if (GetCardLogicValue(firstCards[i])<GetCardLogicValue(secondCards[i]))
                return false;
        }
        return firstRepeatCard>SecondRepeatCard;
    }
    if (GetCardLogicValue(firstRepeatCard) > GetCardLogicValue(SecondRepeatCard))
        return true;
    else
        return false;
}

//just compare the max card
bool CGameLogic::compareLink(SortedBuffer sortedFirstCards, SortedBuffer sortedSecondCards, uint8_t cardCount)
{
    assert(cardCount>3&&cardCount<6);
    //special A J 10 9 8
    uint8_t firstMaxCard=*sortedFirstCards;
    uint8_t secondMaxCard=*sortedSecondCards;
    if(GetCardLogicValue(*sortedFirstCards)==14 && GetCardLogicValue(*(sortedFirstCards+4))==8)
    {
        firstMaxCard=*(sortedFirstCards+1);
    }
    if(GetCardLogicValue(*sortedSecondCards)==14 && GetCardLogicValue(*(sortedSecondCards+4))==8)
    {
        secondMaxCard=*(sortedSecondCards+1);
    }

    if(GetCardLogicValue(firstMaxCard)==GetCardLogicValue(secondMaxCard))
        return GetCardColor(firstMaxCard)>GetCardColor(secondMaxCard);

    return GetCardLogicValue(firstMaxCard)>GetCardLogicValue(secondMaxCard);
}

bool CGameLogic::compareTwoDouble(SortedBuffer sortedFirstCards, SortedBuffer sortedSecondCards, uint8_t cardCount)
{
    assert(cardCount==4 ||cardCount == 5);
    uint8_t firstMaxDouble=*(sortedFirstCards+1);
    uint8_t firstMinDouble=*(sortedFirstCards+3);
    uint8_t secondMaxDouble=*(sortedSecondCards+1);
    uint8_t secondMinDouble=*(sortedSecondCards+3);

    if(GetCardLogicValue(firstMaxDouble)>GetCardLogicValue(secondMaxDouble))
        return true;

    if(GetCardLogicValue(firstMaxDouble)<GetCardLogicValue(secondMaxDouble))
        return false;


    if(GetCardLogicValue(firstMinDouble)>GetCardLogicValue(secondMinDouble))
        return true;
    if(GetCardLogicValue(firstMinDouble)<GetCardLogicValue(secondMinDouble))
        return false;
    if(cardCount==4)
    {
        return *sortedFirstCards>*sortedSecondCards;
    }else
    {
        uint8_t firstSingleCard=0,secondSingleCard=0;
        if(GetCardLogicValue(*sortedFirstCards)==GetCardLogicValue(firstMaxDouble))
            firstMaxDouble=*sortedFirstCards;
        else
            firstSingleCard=*sortedFirstCards;

        if(0==firstSingleCard)
        {
            if(GetCardLogicValue(*(sortedFirstCards+2))==GetCardLogicValue(firstMinDouble))
                firstSingleCard=*(sortedFirstCards+4);
            else
                firstSingleCard=*(sortedFirstCards+2);
        }

        if(GetCardLogicValue(*sortedSecondCards)==GetCardLogicValue(secondMaxDouble))
            secondMaxDouble=*sortedSecondCards;
        else
            secondSingleCard=*sortedSecondCards;

        if(0==secondSingleCard)
        {
            if(GetCardLogicValue(*(sortedSecondCards+2))==GetCardLogicValue(secondMinDouble))
                secondSingleCard=*(sortedSecondCards+4);
            else
                secondSingleCard=*(sortedSecondCards+2);
        }
        if(GetCardLogicValue(firstSingleCard)==GetCardLogicValue(secondSingleCard))
            return firstMaxDouble>secondMaxDouble;

        return GetCardLogicValue(firstSingleCard)>GetCardLogicValue(secondSingleCard);
    }
    assert(false);
}

uint8_t CGameLogic::isDouble(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    for(uint8_t i=0;i<cardCount-1;++i)
    {
        sortedCards++;
        if (GetCardLogicValue(*sortedCards) == GetCardLogicValue(tmpCard))
            return tmpCard;

         tmpCard=*sortedCards;
    }
    return 0;
}

uint8_t CGameLogic::isThird(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    uint8_t sameCount = 1;
    for(uint8_t i=0;i<cardCount-1;++i)
    {
        sortedCards++;
        if (GetCardLogicValue(*sortedCards) == GetCardLogicValue(tmpCard))
            sameCount++;
        else
            sameCount=1;

        if (sameCount == 3)
            return tmpCard;

        tmpCard=*sortedCards;
    }
    return 0;
}

uint8_t CGameLogic::isFour(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    uint8_t sameCount = 1;
    for(uint8_t i=0;i<cardCount-1;++i)
    {
        ++sortedCards;
        if (GetCardLogicValue(*sortedCards) == GetCardLogicValue(tmpCard))
            sameCount++;
        else
            sameCount=1;

        if (sameCount == 4)
            return tmpCard;

        tmpCard=*sortedCards;
    }
    return 0;
}

bool CGameLogic::isCalabash(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    uint8_t sameCount = 1;
    bool haveDouble = false;
    bool haveThird = false;
    //先找到第一个3个或者2个，然后根据前面的结果继续找下个
    for(uint8_t i=0;i<cardCount-1;++i)
    {
        sortedCards++;
        if (GetCardLogicValue(*sortedCards) == GetCardLogicValue(tmpCard))
            sameCount++;
        else
        {
            if(sameCount == 2)
                haveDouble = true;
            else if(sameCount == 3)
                haveThird =true;

            break;
        }
    }
    if(haveDouble)
        haveThird=isThird(sortedCards,cardCount-2);
    else if (haveThird)
        haveDouble=isDouble(sortedCards,cardCount-3);

    return haveDouble&&haveThird;
}

bool CGameLogic::isLink(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    uint8_t linkCount = 1;
    for(uint8_t i=1;i<cardCount;++i)
    {
        if (GetCardLogicValue(*(sortedCards+i)) == GetCardLogicValue(tmpCard)-1)
            linkCount++;
        else
        {
            linkCount=1;
        }//return false;

        tmpCard=*(sortedCards+i);
    }
    //A 8 9 10 J 算顺子
    if(linkCount==4 && cardCount==5)
    {
        if(GetCardLogicValue(*sortedCards)==14 && GetCardLogicValue(*(sortedCards+4))==8)
        {
            linkCount++;
        }
    }
    return linkCount == cardCount;
}

bool CGameLogic::isTwoDouble(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    bool firstDouble = false;
    uint8_t sameCount =1;
    //找到第一个两对再找下个两对，在一个循环里边界的时候容易出问题
    for(cardCount--;0<cardCount;cardCount--)
    {
        sortedCards++;
        if (GetCardLogicValue(*sortedCards) == GetCardLogicValue(tmpCard))
            sameCount++;
        else
        {
            if(sameCount == 2)
            {
                firstDouble= true;
                break;
            }
            sameCount=1;
        }

        tmpCard=*sortedCards;
    }
    if (firstDouble &&cardCount>1&&isDouble(sortedCards,cardCount))
        return true;
    return false;
}

bool CGameLogic::isSameFlower(SortedBuffer sortedCards, uint8_t cardCount)
{
    uint8_t tmpCard = *sortedCards;
    uint8_t color = GetCardColor(tmpCard);
    for(uint8_t i=0;i<cardCount-1;++i)
    {
        sortedCards++;
        if (GetCardColor(*sortedCards) != color)
            return false;
    }

    return true;
}

void CGameLogic::printCardType(uint8_t type)
{
    switch (type) {
    case CT_SINGLE:
        cout<<"CT_SINGLE"<<endl;
        break;
    case CT_DOUBLE:
        cout<<"CT_DOUBLE"<<endl;
        break;
    case CT_TWO_DOUBLE:
        cout<<"CT_TWO_DOUBLE"<<endl;
        break;
    case CT_THIRD:
        cout<<"CT_THIRD"<<endl;
        break;
    case CT_FOUR:
        cout<<"CT_FOUR"<<endl;
        break;
    case CT_LINK:
        cout<<"CT_LINK"<<endl;
        break;
    case CT_CALABASH:
        cout<<"CT_CALABASH"<<endl;
        break;
    case CT_SAME_FLOWER:
        cout<<"CT_SAME_FLOWER"<<endl;
        break;
    case CT_SAME_FLOWER_LINK:
        cout<<"CT_SAME_FLOWER_LINK"<<endl;
        break;
    case CT_MAX_SAME_FLOWER_LINK:
        cout<<"CT_MAX_SAME_FLOWER_LINK"<<endl;
        break;
    default:
        break;
    }
}

