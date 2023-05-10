#include "GameLogic.h"

#include <random>
#include <assert.h>
#include <cstring>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////

static bool sorting(int a,int b)
{

    if((a&LOGIC_MASK_VALUE)>(b&LOGIC_MASK_VALUE))
    {
        return true;
    }
    else if((a&LOGIC_MASK_VALUE)==(b&LOGIC_MASK_VALUE))
    {
         if((a&LOGIC_MASK_COLOR)<(b&LOGIC_MASK_COLOR))
         {
             return true;
         }
         else
         {
             return false;
         }
    }
    else
    {
        return false;
    }

}

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
    isOpenTenMu=false;
}

//析构函数
CGameLogic::~CGameLogic()
{
}

//判断是否是五小牛
bool CGameLogic::JudgeFiveSmallNiu(uint8_t arr[], uint8_t cbCardCount)
{
    uint8_t wTotalValue = 0;
    //统计牌点总合
    for (int i = 0; i < cbCardCount; i++)
    {
        uint8_t wValue = GetCardValue(arr[i]);
        if (wValue > 4)
        {
            return false;
        }
        wTotalValue += wValue;
    }
    //判断是否五小
    return (wTotalValue <= 10 && wTotalValue > 0) ? true : false;
}

//判断是否为炸弹
bool CGameLogic::JudgeBomb(uint8_t arr[], uint8_t cbCardCount)
{
    uint8_t iTempCard[5] = {0};
    int iSameCount = 0;
    memcpy(iTempCard, arr, sizeof(iTempCard));
    //排序
    SortCardList(iTempCard,cbCardCount);

    uint8_t iSecondValue = GetCardValue(iTempCard[2]); //获取牌值5个数(0,1,2,3,4)的中间值是2
    for (int i = 0; i < cbCardCount; ++i)
    {
        if (iSecondValue == GetCardValue(iTempCard[i]))
        {
            iSameCount++;
        }
    }

    return  (4 == iSameCount) ? true : false;
}

//判断是否为五花牛
bool CGameLogic::JudgeGoldenBull(uint8_t arr[], uint8_t cbCardCount)
{
    uint8_t cbCount = 0;
    for (int i = 0; i < cbCardCount; ++i)
    {
        if (GetCardValue(arr[i]) > 10)
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

//判断是否为四花牛
bool CGameLogic::JudgeSilveryBull(uint8_t arr[], uint8_t cbCardCount)
{
    uint8_t iTempCard[5] = {0};
    memcpy(iTempCard, arr, sizeof(iTempCard));
    //排序
    SortCardList(iTempCard,MAX_COUNT);
    if(GetCardValue(iTempCard[4]) == 10)//最小的一张牌的牌值应该为10
    {
        uint8_t cbCount = 0;
        for (int i = 3; i >= 0; --i) //其余的四张牌的牌值大于10
        {
            if (GetCardValue(iTempCard[i]) > 10)
            {
                ++cbCount;
            }
            else
            {
                return false;
            }
        }
        return (4 == cbCount) ? true : false;
    }
    else
    {
        return false;
    }
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

//根据花色和数值获取对应的牌值
uint8_t CGameLogic::GetCardData(uint8_t cbColor, uint8_t cbValue)
{
    uint8_t cbParam = 16;
    return cbColor*cbParam+cbValue;
}

bool CGameLogic::GetMatchPointCard(int32_t iPoint,uint8_t cbCardData[MAX_COUNT], uint8_t byCardDataArray[52])
{
    int nColorValue = 0;
    int nFind = 0;
    int index = 0;
    uint8_t bLogicValue1 = 0;
    uint8_t bLogicValue2 = 0;
    uint8_t bLogicValue3 = 0;
    uint8_t bLogicValue4 = 0;
    uint8_t bNeedValue = 0;
    uint8_t bCardData = 0;

    //先获取两张牌
    if(iPoint < 12 && iPoint > 1)
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

        bLogicValue1 = GetLogicValue(cbCardData[0]);
        bLogicValue2 = GetLogicValue(cbCardData[1]);
        bool flag = true;
        do
        {
            cbCardData[2] = byCardDataArray[index++];
            if(index == 52) index = 0;
            if(IsUsedCard(cbCardData[2])) continue;
            bLogicValue3 = GetLogicValue(cbCardData[2]);
            if(((bLogicValue1 + bLogicValue2 + bLogicValue3) %10 == 0)) flag = false;
        }while(flag);
        InsertUsedCard(cbCardData[2]);

        //再取一张牌
        do
        {
            cbCardData[3] = byCardDataArray[index++];
            if(index == 52) index = 0;
        }while(IsUsedCard(cbCardData[3]));
        InsertUsedCard(cbCardData[3]);

        bLogicValue4 = GetLogicValue(cbCardData[3]);

        uint8_t bFilter = 1;
        for(uint8_t i = 1; i<11; ++i)
        {
            if(((iPoint - bFilter)%10) == ((bLogicValue4 + i)%10))
            {
                bNeedValue = i;
                break;
            }
        }

        srand(time(NULL));
        //随机获取花色
        nColorValue = rand()%4;
        if(bNeedValue == 10)
            bNeedValue += (uint8_t)rand()%4;
        nFind = 0;
        bCardData = 0;
        while(nFind<4)
        {
            bCardData = GetCardData(nColorValue, bNeedValue);
            if(!IsUsedCard(bCardData))
            {
                cbCardData[4] = bCardData;
                break;
            }
            nColorValue = ((++nColorValue)%4);
            nFind++;
        }

        if(nFind == 4)
        {
            return false;
        }
        else
        {
            InsertUsedCard(cbCardData[4]);
            return true;
        }
    }
    else if(iPoint == 1)
    {
        bool bHasNiu = true;
        int iIndex = 0;
        while(iIndex < 52)
        {
            while(iIndex < 52)
            {
                cbCardData[0] = byCardDataArray[iIndex];
                if(IsUsedCard(cbCardData[0]))
                {
                    ++iIndex;
                    continue;
                }
                for(int j = iIndex+1; j < 52; ++j)
                {
                    cbCardData[1] = byCardDataArray[j];
                    if(IsUsedCard(cbCardData[1]) || cbCardData[0] == cbCardData[1]) continue;
                    for(int k = j+1; k < 52; ++k)
                    {
                        cbCardData[2] = byCardDataArray[k];
                        if(IsUsedCard(cbCardData[2]) || cbCardData[2] == cbCardData[1] || cbCardData[2] == cbCardData[0]) continue;
                        bCardData = GetLogicValue(cbCardData[0]) + GetLogicValue(cbCardData[1]) + GetLogicValue(cbCardData[2]);
                        if((bCardData % 10) != 0)
                        {
                            bHasNiu = false;
                            break;
                        }
                    }
                    if(!bHasNiu) break;
                }
                if(!bHasNiu) break;
                ++iIndex;
            }

            for(int i = iIndex+1 ; i < 52; ++i)
            {
                cbCardData[3] = byCardDataArray[i];
                if(IsUsedCard(cbCardData[3]) || cbCardData[3] == cbCardData[2] || cbCardData[3] == cbCardData[1]
                        || cbCardData[3] == cbCardData[0])
                    continue;
                for(int j = i+1; j < 52; ++j)
                {
                    cbCardData[4] = byCardDataArray[j];
                    if(IsUsedCard(cbCardData[4]) || cbCardData[4] == cbCardData[3] || cbCardData[4] == cbCardData[2]
                            || cbCardData[4] == cbCardData[1] || cbCardData[4] == cbCardData[0])
                        continue;
                }
            }

            if(SortNiuNiuCard(cbCardData, 5) == 1)
            {
                return true;
            }
            else ++iIndex;
        }

    }
    return false;
}

//排列扑克
void CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount)
{
    //转换数值
    uint8_t cbLogicValue[MAX_COUNT];
    for (uint8_t i=0;i<cbCardCount;i++)
        cbLogicValue[i]=GetCardValue(cbCardData[i]);

    //排序操作
    bool bSorted=true;
    uint8_t cbTempData,bLast=cbCardCount-1;
    do
    {
        bSorted=true;
        for (uint8_t i=0;i<bLast;i++)
        {
            if ((cbLogicValue[i]<cbLogicValue[i+1])||
                ((cbLogicValue[i]==cbLogicValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
            {
                //交换位置
                cbTempData=cbCardData[i];
                cbCardData[i]=cbCardData[i+1];
                cbCardData[i+1]=cbTempData;
                cbTempData=cbLogicValue[i];
                cbLogicValue[i]=cbLogicValue[i+1];
                cbLogicValue[i+1]=cbTempData;
                bSorted=false;
            }
        }
        bLast--;
    } while(bSorted==false);

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

uint8_t CGameLogic::GetLogicValue(uint8_t cbCardData)
{
    //扑克属性
    uint8_t bCardValue = GetCardValue(cbCardData);

    //转换数值
    return (bCardValue > 10) ? (10) : bCardValue;
}

//bool CGameLogic::MatchCard()
//{
//    uint8_t	cbCardListDataTemp[52];				//扑克定义
//    memcpy(m_cbCardListData, cbCardListDataTemp, sizeof(cbCardListDataTemp);
//    bool IsOpen=false;
//    int times=0;
//    bool IsRight[4]={false};
//    while(!IsOpen)
//    {
//        bool isok=GoupCardOpen(ifo,vecAllcards);
//        IsOpen=true;
//        for(int i=0;i<2;i++)
//        {
//            IsRight[i]=vecCardGroup[i+1].CompareGroupWitchBig(vecCardGroup[0]);
//        }
//        for(int i=0;i<2;i++)
//        {
//            if(IsRight[i]!=ifo.PlayerWinorLose[i])
//            {
//                IsOpen=false;
//                break;
//            }
//        }
//        for(int i=0;i<3;i++)
//        {
//            for(int j=0;j<5;j++)
//            {
//                if(vecCardGroup[i].m_iCard[j]<0)
//                {
//                    IsOpen=false;
//                    break;
//                }

//            }
//        }
//        if(!IsOpen||!isok)
//        {
//            vecAllcards.clear();
//            for(int i=0;i<vecWholeCar.size();i++)
//            {
//                vecAllcards.push_back(vecWholeCar[i]);
//            }
//            ReststarVec(vecAllcards);
//        }
//        else
//        {
//            return true;
//        }
//        times++;

//        if(times>5000)
//        {
//            return false;
//        }
//    }
//    return false;
//}
int32_t CGameLogic::CalculateMultiples10(uint8_t arr[],uint8_t brr[], uint8_t cbCardCount)
{
    uint8_t ax[5];
    uint8_t bx[5];
    for(int i=0;i<cbCardCount;i++)
    {
        ax[i]=arr[i];
        bx[i]=brr[i];
    }
    //先看牌型
    int32_t res1 = SortNiuNiuCard(ax, cbCardCount);
    int32_t res2 = SortNiuNiuCard(bx, cbCardCount);
    //再排序
    SortCardList(ax, cbCardCount);
    SortCardList(bx, cbCardCount);
    if(res1==res2)
    {
        //暂时不在逻辑类里面加四花牛、炸弹、五花牛、五小牛的判断，因为通过对代码的分析，牌的结果是在Algorithm->oxOnebk(1,0,0,&tag);中获取
        //炸弹之间的比较大小：比四炸的大小,不存在两个炸弹相同
        if(res1 == Brnn::CARD_COIN_BOMB)
        {
            if(GetCardValue(ax[2]) > GetCardValue(bx[2]))
            {
                return MAX_MUL;
            }
            else
            {
                return -MAX_MUL;
            }
        }

        if(GetCardValue(ax[0])>GetCardValue(bx[0])) //非炸弹的比较
        {
           if(res1 <= Brnn::CARD_COIN_1)
               return Brnn::CARD_COIN_1-1;
           else
           {
               if(res1>11)
               {
                   return MAX_MUL;
               }
               else
               {
                   return res1-1;
               }

           }

        }
        else if(GetCardValue(ax[0])==GetCardValue(bx[0]))
        {
            if(GetCardColor(ax[0])<GetCardColor(bx[0]))
            {

                if(res2 <= Brnn::CARD_COIN_1)
                    return -(Brnn::CARD_COIN_1-1);
                else
                {
                    if(res2>11)
                    {
                        return -MAX_MUL;
                    }
                    else
                    {
                        return -(res2-1);
                    }

                }
            }
            else
            {


                if(res1 <= Brnn::CARD_COIN_1)
                    return Brnn::CARD_COIN_1-1;
                else
                {
                    if(res1>11)
                    {
                        return MAX_MUL;
                    }
                    else
                    {
                        return (res1-1);
                    }

                }
            }
        }
        else
        {
            if(res2 <= Brnn::CARD_COIN_1)
                return -(Brnn::CARD_COIN_1-1);
            else
            {
                if(res2>11)
                {
                    return -MAX_MUL;
                }
                else
                {
                    return -(res2-1);
                }

            }
        }
    }
    else if(res1 > res2)
    {

        if(res1 <= Brnn::CARD_COIN_1)
            return Brnn::CARD_COIN_1-1;
        else
        {
            if(res1>11)
            {
                return MAX_MUL;
            }
            else
            {
                return (res1-1);
            }

        }

    }
    else
    {
        if(res2 <= Brnn::CARD_COIN_1)
            return -(Brnn::CARD_COIN_1-1);
        else
        {
            if(res2>11)
            {
                return -MAX_MUL;
            }
            else
            {
                return -(res2-1);
            }

        }
    }

}
int32_t CGameLogic::CalculateMultiples3(uint8_t arr[],uint8_t brr[], uint8_t cbCardCount)
{
    uint8_t ax[5];
    uint8_t bx[5];
    for(int i=0;i<cbCardCount;i++)
    {
        ax[i]=arr[i];
        bx[i]=brr[i];
    }
    //先看牌型
    int32_t res1 = SortNiuNiuCard(ax, cbCardCount);
    int32_t res2 = SortNiuNiuCard(bx, cbCardCount);

    //再排序
    SortCardList(ax, cbCardCount);
    SortCardList(bx, cbCardCount);

    if(res1==res2)
    {
        //暂时不在逻辑类里面加四花牛、炸弹、五花牛、五小牛的判断，因为通过对代码的分析，牌的结果是在Algorithm->oxOnebk(1,0,0,&tag);中获取
        //炸弹之间的比较大小：比四炸的大小,不存在两个炸弹相同
        if(res1 == Brnn::CARD_COIN_BOMB)
        {
            if(GetCardValue(ax[2]) > GetCardValue(bx[2]))
            {
                return BOMB_MUL;
            }
            else
            {
                return -BOMB_MUL;
            }
        }

        if(GetCardValue(ax[0])>GetCardValue(bx[0])) //非炸弹的比较
        {
           if(res1 < Brnn::CARD_COIN_7)
               return NORMAL_MUL;
           else if(res1 < Brnn::CARD_COIN_NIUNIU)
               return NN_MUL;
           else
               return SPECIAL_MUL;
        }
        else if(GetCardValue(ax[0])==GetCardValue(bx[0]))
        {
            if(GetCardColor(ax[0])<GetCardColor(bx[0]))
            {
                if(res1 < Brnn::CARD_COIN_7)
                    return -NORMAL_MUL;
                else if(res1 < Brnn::CARD_COIN_NIUNIU)
                    return -NN_MUL;
                else
                    return -SPECIAL_MUL;
            }
            else
            {
                if(res1 < Brnn::CARD_COIN_7)
                    return NORMAL_MUL;
                else if(res1 < Brnn::CARD_COIN_NIUNIU)
                    return NN_MUL;
                else
                    return SPECIAL_MUL;
            }
        }
        else
        {
            if(res1 < Brnn::CARD_COIN_7)
                return -NORMAL_MUL;
            else if(res1 < Brnn::CARD_COIN_NIUNIU)
                return -NN_MUL;
            else
                return -SPECIAL_MUL;
        }
    }
    else if(res1 > res2)
    {
        if(res1 <= Brnn::CARD_COIN_6)
        {
            return NORMAL_MUL;
        }else if( res1 <= Brnn::CARD_COIN_9)
        {
            return NN_MUL;
        }else{
            return SPECIAL_MUL;
        }
#if 0
        switch(res1)
        {
        case Brnn::CARD_COIN_1:
        {
            return 1;
            break;
        }
        case Brnn::CARD_COIN_2:
        {
            return 1;
            break;
        }
        case Brnn::CARD_COIN_3:
        {
            return 1;
            break;
        }
        case Brnn::CARD_COIN_4:
        {
            return 1;
            break;
        }
        case Brnn::CARD_COIN_5:
        {
            return 1;
            break;
        }
        case Brnn::CARD_COIN_6:
        {
            return 1;
            break;
        }
        case Brnn::CARD_COIN_7:
        {
            return 2;
            break;
        }
        case Brnn::CARD_COIN_8:
        {
            return 2;
            break;
        }
        case Brnn::CARD_COIN_9:
        {
            return 2;
            break;
        }
        case Brnn::CARD_COIN_NIUNIU:
        {
            return 3;
            break;
        }
        case Brnn::CARD_COIN_SILVERY_NIU: //四花牛（银牛） Add by guansheng 20190308
        {
            return 3;
            break;
        }
        case Brnn::CARD_COIN_BOMB:   //炸弹  Add by guansheng 20190308
        {
            return 3;
            break;
        }
        case Brnn::CARD_COIN_GOLDEN_BULL: //五花牛（金牛） Add by guansheng 20190308
        {
            return 3;
            break;
        }
        case Brnn::CARD_COIN_FIVE_SMALL: //五小牛        Add by guansheng 20190308
        {
            return 3;
            break;
        }
        case Brnn::CARD_COIN_0:
        {
            return 1;
            break;
        }
        }
#endif
    }
    else
    {
        if(res2 <= Brnn::CARD_COIN_6)
        {
            return -NORMAL_MUL;
        }else if( res2 <= Brnn::CARD_COIN_9)
        {
            return -NN_MUL;
        }else{
            return -SPECIAL_MUL;
        }
#if 0
        switch(res2)
        {
        case Brnn::CARD_COIN_1:
        {
            return -1;
            break;
        }
        case Brnn::CARD_COIN_2:
        {
            return -1;
            break;
        }
        case Brnn::CARD_COIN_3:
        {
            return -1;
            break;
        }
        case Brnn::CARD_COIN_4:
        {
            return -1;
            break;
        }
        case Brnn::CARD_COIN_5:
        {
            return -1;
            break;
        }
        case Brnn::CARD_COIN_6:
        {
            return -1;
            break;
        }
        case Brnn::CARD_COIN_7:
        {
            return -2;
            break;
        }
        case Brnn::CARD_COIN_8:
        {
            return -2;
            break;
        }
        case Brnn::CARD_COIN_9:
        {
            return -2;
            break;
        }
        case Brnn::CARD_COIN_NIUNIU:
        {
            return -3;
            break;
        }
        case Brnn::CARD_COIN_SILVERY_NIU: //四花牛（银牛） Add by guansheng 20190308
        {
            return -3;
            break;
        }
        case Brnn::CARD_COIN_BOMB:   //炸弹  Add by guansheng 20190308
        {
            return -3;
            break;
        }
        case Brnn::CARD_COIN_GOLDEN_BULL: //五花牛（金牛） Add by guansheng 20190308
        {
            return -3;
            break;
        }
        case Brnn::CARD_COIN_FIVE_SMALL: //五小牛        Add by guansheng 20190308
        {
            return -3;
            break;
        }
        case Brnn::CARD_COIN_0:
        {
            return -1;
            break;
        }
        }
#endif
    }

}
int32_t CGameLogic::CalculateMultiples(uint8_t arr[],uint8_t brr[], uint8_t cbCardCount)
{

    if(isOpenTenMu)
    {
       return CalculateMultiples10(arr,brr, cbCardCount);
    }
    else
    {
       return CalculateMultiples3(arr,brr, cbCardCount);
    }
}

int CGameLogic::CalculatePoints(uint8_t arr[], uint8_t cbCardCount)
{
    // Add by guansheng 20190308 Start
    if(JudgeFiveSmallNiu(arr, cbCardCount)) //加上五小牛的判断
    {
        return Brnn::CARD_COIN_FIVE_SMALL;
    }
    if(JudgeBomb(arr, cbCardCount)) //加上炸弹的判断
    {
        return Brnn::CARD_COIN_BOMB;
    }
    if(JudgeGoldenBull(arr, cbCardCount)) //加上五花牛的判断
    {
        return Brnn::CARD_COIN_GOLDEN_BULL;
    }
    if(JudgeSilveryBull(arr, cbCardCount)) //加上四花牛的判断
    {
        return Brnn::CARD_COIN_SILVERY_NIU;
    }
    //Add by guansheng 20190308 End

    if(JudgeFiveHaveNiu(arr,cbCardCount))
    {
        int xxx=0;
        for(int j=0;j<5;j++)
        {
            xxx+=GetCardValue(arr[j]);
        }
        if(xxx%10==0)
        {
            return Brnn::CARD_COIN_NIUNIU;
        }
        return xxx%10;
    }
    else
    {
        return Brnn::CARD_COIN_0;
    }
}

int32_t CGameLogic::SortNiuNiuCard(uint8_t bCardData[], uint8_t bCardCount)
{
    int32_t nNiuType = Brnn::CARD_COIN_0;
    bool bNiu = false;
    bool bFind = false;
    uint8_t bCardDataTemp[5];
    uint8_t bNiuCard[3];
    memcpy(bCardDataTemp, bCardData, bCardCount);
    uint8_t CardValue = 0;

    if(JudgeFiveSmallNiu(bCardData, bCardCount)) //五小牛的判断
    {
        bNiu = true;
        nNiuType = Brnn::CARD_COIN_FIVE_SMALL;
    }

    if(!bNiu)
    {
        if(JudgeBomb(bCardData, bCardCount)) //炸弹的判断
        {
            bNiu = true;
            nNiuType = Brnn::CARD_COIN_BOMB;
        }
    }

    if(!bNiu)
    {
        if(JudgeGoldenBull(bCardData, bCardCount)) //五花牛的判断
        {
            bNiu = true;
            nNiuType = Brnn::CARD_COIN_GOLDEN_BULL;
        }
    }

    if(!bNiu)
    {
        if(JudgeSilveryBull(bCardData, bCardCount)) //四花牛的判断
        {
            bNiu = true;
            nNiuType = Brnn::CARD_COIN_SILVERY_NIU;
        }
    }

    if(!bNiu)
    {
        for(int i = 0 ; i < 5; i++)
        {
            for(int j = i+1; j < 5; j++)
            {
                for(int k = j+1; k < 5; k++)
                {
                    CardValue = GetLogicValue(bCardDataTemp[i]) +
                            GetLogicValue(bCardDataTemp[j]) +
                            GetLogicValue(bCardDataTemp[k]);
                    if((CardValue % 10) == 0)
                    {
                        bNiu = true;
                        bNiuCard[0] = bCardDataTemp[i];
                        bNiuCard[1] = bCardDataTemp[j];
                        bNiuCard[2] = bCardDataTemp[k];
                        break;
                    }
                }
                if(bNiu) break;
            }
            if(bNiu) break;
        }
    }

    if(bNiu)
    {
        if(nNiuType == Brnn::CARD_COIN_0)
        {
            int iFlag = 3;
            memcpy(bCardData, bNiuCard, 3);
            for(int i = 0; i < 5; i++)
            {
                bFind = false;
                for(int j = 0; j < 3; j++)
                {
                    if(bCardDataTemp[i] == bNiuCard[j])
                    {
                        bFind = true;
                        break;
                    }
                }
                if(!bFind)
                {
                    bCardData[iFlag++] = bCardDataTemp[i];
                    if(iFlag == 5) break;
                }
            }

            if((GetLogicValue(bCardData[3]) + GetLogicValue(bCardData[4]))%10 == 0)
            {
                nNiuType = Brnn::CARD_COIN_NIUNIU;
            }
            else
            {
                nNiuType = (GetLogicValue(bCardData[3]) + GetLogicValue(bCardData[4]))%10+1;
            }

        }
        else if(nNiuType == Brnn::CARD_COIN_BOMB)
        {
            //排序
            SortCardList(bCardDataTemp,bCardCount);
            uint8_t bBombValue = GetCardValue(bCardDataTemp[2]);
            int iIndex = 0;
            for(int i = 0; i < bCardCount; ++i)
            {
                if(GetCardValue(bCardDataTemp[i]) == bBombValue)
                    bCardData[iIndex++] = bCardDataTemp[i];
                else
                    bCardData[4] = bCardDataTemp[i];
            }
        }
        else if(nNiuType == Brnn::CARD_COIN_SILVERY_NIU)
        {
            int iIndex = 0;
            for(int i = 0; i < bCardCount; ++i)
            {
                if(GetCardValue(bCardDataTemp[i]) == 10)
                     bCardData[4] = bCardDataTemp[i];
                else
                    bCardData[iIndex++] = bCardDataTemp[i];
            }
        }
    }
    return nNiuType;
}

bool CGameLogic::JudgeFiveHaveNiu(uint8_t arr[],uint8_t cbCardCount)
{
    std::vector<uint8_t> FiveVct;
    for(int i=0;i<cbCardCount;i++)
    {

        for(int j=0;j<cbCardCount;j++)
        {
            FiveVct.clear();
            for(int n=0;n<cbCardCount;n++)
            {
                FiveVct.push_back(arr[n]);
            }
            FiveVct.erase(FiveVct.begin()+i);
            std::vector<uint8_t> FiveVct1;
            for(int k=0;k<4;k++)
            {
                FiveVct1.clear();
                for( std::vector<uint8_t>::iterator FiveVctit=FiveVct.begin();FiveVctit!=FiveVct.end();FiveVctit++)
                {
                    FiveVct1.push_back(*FiveVctit);
                }
                FiveVct1.erase(FiveVct1.begin()+k);
                int res=0;
                for(std::vector<uint8_t>::iterator FiveVctit1=FiveVct1.begin();FiveVctit1!=FiveVct1.end();FiveVctit1++)
                {
                    res+=GetCardValue(*FiveVctit1);
                }
                if(res%10==0)
                {
                    return true;
                }
            }

        }

    }
    return false;
}
//////////////////////////////////////////////////////////////////////////
