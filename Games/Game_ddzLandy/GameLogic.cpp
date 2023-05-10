#include "GameLogic.h"
#include <vector>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////////
//静态变量

//索引变量
const uint8_t cbIndexCount=5;

//扑克数据
const uint8_t	CGameLogic::m_cbCardData[FULL_COUNT]=
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
};
//好牌数据//2015.4.28
const uint8_t	CGameLogic::m_cbGoodcardData[GOOD_CARD_COUTN]=
{
    0x01,0x02,						//方块 A - 2
    0x11,0x12,						//梅花 A - 2
    0x21,0x22,						//红桃 A - 2
    0x31,0x32,						//黑桃 A - 2
    0x4E,0x4F,						//大小皇
    0x07,0x08,0x09,					//789
    0x17,0x18,0x19,					//789
    0x27,0x28,0x29,					//789
    0x37,0x38,0x39,					//789
    0x0A,0x0B,0x0C,0x0D,			//10JQK
    0x1A,0x1B,0x1C,0x1D,			//10JQK
    0x2A,0x2B,0x2C,0x2D,			//10JQK
    0x3A,0x3B,0x3C,0x3D				//10JQK
};
//////////////////////////////////////////////////////////////////////////////////

//构造函数
CGameLogic::CGameLogic() : m_gen(m_rd())
{
}

//析构函数
CGameLogic::~CGameLogic()
{
}

//获取类型
uint8_t CGameLogic::GetCardType(const uint8_t cbCardData[], uint8_t cbCardCount)
{
    //简单牌型
    switch (cbCardCount)
    {
    case 0:	//空牌
        {
            return CT_ERROR;
        }
    case 1: //单牌
        {
            return CT_SINGLE;
        }
    case 2:	//对牌火箭
        {
            //牌型判断
            if ((cbCardData[0]==0x4F)&&(cbCardData[1]==0x4E)) return CT_MISSILE_CARD;
            if (GetCardLogicValue(cbCardData[0])==GetCardLogicValue(cbCardData[1])) return CT_DOUBLE_CARD;

            return CT_ERROR;
        }
    }

    //分析扑克
    tagAnalyseResult AnalyseResult;
    AnalysebCardData(cbCardData,cbCardCount,AnalyseResult);

    //四牌判断
    if (AnalyseResult.cbBlockCount[3]>0)
    {
        //牌型判断
        if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==4)) return CT_BOMB_CARD;
        if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==6)) return CT_FOUR_TAKE_ONE;
        if ((AnalyseResult.cbBlockCount[3]==1)&&(cbCardCount==8)&&(AnalyseResult.cbBlockCount[1]==2)) return CT_FOUR_TAKE_TWO;

        return CT_ERROR;
    }

    //三牌判断
    if (AnalyseResult.cbBlockCount[2]>0)
    {
        //连牌判断
        if (AnalyseResult.cbBlockCount[2]>1)
        {
            //变量定义
            uint8_t cbCardData=AnalyseResult.cbCardData[2][0];
            uint8_t cbFirstLogicValue=GetCardLogicValue(cbCardData);

            //错误过虑
            if (cbFirstLogicValue>=15) return CT_ERROR;

            //连牌判断
            for (uint8_t i=1;i<AnalyseResult.cbBlockCount[2];i++)
            {
                uint8_t cbCardData=AnalyseResult.cbCardData[2][i*3];
                if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
            }
        }
        else if( cbCardCount == 3 ) return CT_THREE;

        //牌形判断
        if (AnalyseResult.cbBlockCount[2]*3==cbCardCount) return CT_THREE_LINE;
        if (AnalyseResult.cbBlockCount[2]*4==cbCardCount) return CT_THREE_TAKE_ONE;
        if ((AnalyseResult.cbBlockCount[2]*5==cbCardCount)&&(AnalyseResult.cbBlockCount[1]==AnalyseResult.cbBlockCount[2])) return CT_THREE_TAKE_TWO;

        return CT_ERROR;
    }

    //两张类型
    if (AnalyseResult.cbBlockCount[1]>=3)
    {
        //变量定义
        uint8_t cbCardData=AnalyseResult.cbCardData[1][0];
        uint8_t cbFirstLogicValue=GetCardLogicValue(cbCardData);

        //错误过虑
        if (cbFirstLogicValue>=15) return CT_ERROR;

        //连牌判断
        for (uint8_t i=1;i<AnalyseResult.cbBlockCount[1];i++)
        {
            uint8_t cbCardData=AnalyseResult.cbCardData[1][i*2];
            if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
        }

        //二连判断
        if ((AnalyseResult.cbBlockCount[1]*2)==cbCardCount) return CT_DOUBLE_LINE;

        return CT_ERROR;
    }

    //单张判断
    if ((AnalyseResult.cbBlockCount[0]>=5)&&(AnalyseResult.cbBlockCount[0]==cbCardCount))
    {
        //变量定义
        uint8_t cbCardData=AnalyseResult.cbCardData[0][0];
        uint8_t cbFirstLogicValue=GetCardLogicValue(cbCardData);

        //错误过虑
        if (cbFirstLogicValue>=15) return CT_ERROR;

        //连牌判断
        for (uint8_t i=1;i<AnalyseResult.cbBlockCount[0];i++)
        {
            uint8_t cbCardData=AnalyseResult.cbCardData[0][i];
            if (cbFirstLogicValue!=(GetCardLogicValue(cbCardData)+i)) return CT_ERROR;
        }

        return CT_SINGLE_LINE;
    }

    return CT_ERROR;
}

//排列扑克
VOID CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t cbSortType)
{
    //数目过虑
    if (cbCardCount==0) return;
    if (cbSortType==ST_CUSTOM) return;

    //转换数值
    uint8_t cbSortValue[MAX_COUNT] = { 0 };
    for (uint8_t i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardLogicValue(cbCardData[i]);

    //排序操作
    bool bSorted=true;
    uint8_t cbSwitchData=0,cbLast=cbCardCount-1;
    do
    {
        bSorted=true;
        for (uint8_t i=0;i<cbLast;i++)
        {
            if ((cbSortValue[i]<cbSortValue[i+1])||
                ((cbSortValue[i]==cbSortValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
            {
                //设置标志
                bSorted=false;

                //扑克数据
                cbSwitchData=cbCardData[i];
                cbCardData[i]=cbCardData[i+1];
                cbCardData[i+1]=cbSwitchData;

                //排序权位
                cbSwitchData=cbSortValue[i];
                cbSortValue[i]=cbSortValue[i+1];
                cbSortValue[i+1]=cbSwitchData;
            }
        }
        cbLast--;
    } while(bSorted==false);

    //数目排序
    if (cbSortType==ST_COUNT)
    {
        //变量定义
        uint8_t cbCardIndex=0;

        //分析扑克
        tagAnalyseResult AnalyseResult;
        AnalysebCardData(&cbCardData[cbCardIndex],cbCardCount-cbCardIndex,AnalyseResult);

        //提取扑克
        for (uint8_t i=0;i<CountArray(AnalyseResult.cbBlockCount);i++)
        {
            //拷贝扑克
            uint8_t cbIndex=CountArray(AnalyseResult.cbBlockCount)-i-1;
            CopyMemory(&cbCardData[cbCardIndex],AnalyseResult.cbCardData[cbIndex],AnalyseResult.cbBlockCount[cbIndex]*(cbIndex+1)*sizeof(uint8_t));

            //设置索引
            cbCardIndex+=AnalyseResult.cbBlockCount[cbIndex]*(cbIndex+1)*sizeof(uint8_t);
        }
    }

    return;
}

//混乱扑克
VOID CGameLogic::RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount)
{
    //混乱准备
    //uint8_t cbCardData[CountArray(m_cbCardData)];
    //CopyMemory(cbCardData,m_cbCardData,sizeof(m_cbCardData));

    ////混乱扑克
    //uint8_t cbRandCount=0,cbPosition=0;
    //do
    //{
    //    cbPosition=rand()%(cbBufferCount-cbRandCount);
    //    cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
    //    cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
    //} while (cbRandCount<cbBufferCount);

	memcpy(cbCardBuffer, m_cbCardData, FULL_COUNT);
	std::shuffle(&cbCardBuffer[0], &cbCardBuffer[FULL_COUNT], m_gen);

	//模仿真实洗牌，洗牌规则：1、分开两手牌（AB），2、A先放1-3张，B再放1-3张，3、将1和2的步骤重复4次，意为新买的牌洗5次
	//uint8_t ucACount = 0;
	//uint8_t ucBCount = 0;
	//uint8_t ucAData[FULL_COUNT] = {0};
	//uint8_t ucBData[FULL_COUNT] = {0};
	//uint8_t ucRandTimes = 4;
	//uint8_t ucTmp = 0;
	//uint8_t ucRand = 0;
	//先随机抽取几个炸弹
	//#define BOM_COUNT 3	//最多3个炸弹
	//uint8_t ucBomTmpCount = rand()%(BOM_COUNT);	//0-2个炸弹
	////出现2个炸弹，再降低一半概率
	//if (ucBomTmpCount >= 2 && rand()%2==0)
	//{
	//	ucBomTmpCount = 1;
	//}
	//std::vector<int> vecIndex;	vecIndex.clear();
	//uint8_t ucTmpIndex = 0;
	//uint8_t ucBomData[BOM_COUNT][4] = {0};	//保存炸弹数据
	//uint8_t ucLeftCardCount = FULL_COUNT;
	//uint8_t cbLeftCardData[CountArray(cbCardData)];
	//int iIndex = 0;
	//if (0 < ucBomTmpCount)
	//{
	//	ucLeftCardCount = 0;
	//	for (uint8_t i=0; i!=ucBomTmpCount; ++i)
	//	{
	//		ucTmpIndex = rand()%13;	//从A-K
	//		std::vector<int>::iterator iter = std::find(vecIndex.begin(), vecIndex.end(), ucTmpIndex);
	//		if (iter != vecIndex.end())
	//		{
	//			continue;
	//		}
	//		vecIndex.push_back(ucTmpIndex);
	//		ucBomData[iIndex][0] = cbCardData[ucTmpIndex];
	//		cbCardData[ucTmpIndex] = 0xFF;
	//		ucTmpIndex += 13;
	//		ucBomData[iIndex][1] = cbCardData[ucTmpIndex];
	//		cbCardData[ucTmpIndex] = 0xFF;
	//		ucTmpIndex += 13;
	//		ucBomData[iIndex][2] = cbCardData[ucTmpIndex];
	//		cbCardData[ucTmpIndex] = 0xFF;
	//		ucTmpIndex += 13;
	//		ucBomData[iIndex][3] = cbCardData[ucTmpIndex];
	//		cbCardData[ucTmpIndex] = 0xFF;
	//		++iIndex;
	//	}
	//	//计算剩下的扑克数
	//	for (int i=0; i!=FULL_COUNT; ++i)
	//	{
	//		if (0xFF != cbCardData[i])
	//		{
	//			cbLeftCardData[ucLeftCardCount++] = cbCardData[i];
	//		}
	//	}
	//}
	//else
	//{
	//	CopyMemory(cbLeftCardData,cbCardData,sizeof(cbCardData));
	//}
	//for (int i = 0; i != ucRandTimes; ++i)
	//{
	//	ucACount = ucLeftCardCount / 2;
	//	ucBCount = ucLeftCardCount - ucACount;
	//	memcpy(ucAData, cbLeftCardData, ucACount);
	//	memcpy(ucBData, cbLeftCardData+ucACount, ucBCount);
	//	//第一次的时候将B手牌的最面上两张（王炸）随机交换到AB手牌中，不然经常会出现王炸
	//	if (0 == i)
	//	{
	//		ucRand = rand()%ucACount;
	//		ucTmp=ucAData[ucRand];	ucAData[ucRand]=ucBData[ucBCount-2];		ucBData[ucBCount-2]=ucTmp;
	//		ucRand = rand()%ucBCount;
	//		ucTmp=ucBData[ucRand];		ucBData[ucRand]=ucBData[ucBCount-1];		ucBData[ucBCount-1]=ucTmp;
	//	}
	//	uint8_t ucATmp = 0;
	//	uint8_t ucBTmp = 0;
	//	uint8_t ucAIndex = 0;
	//	uint8_t ucBIndex = 0;
	//	uint8_t ucIndex = 0;
	//	while (ucACount > 0 || ucBCount > 0)
	//	{
	//		if (ucACount > 0)
	//		{
	//			ucATmp = ucBCount>0?(rand()%2 + 1):ucACount;
	//			if (ucATmp > ucACount)
	//			{
	//				ucATmp = ucACount;
	//			}
	//			memcpy(cbLeftCardData+ucIndex, ucAData+ucAIndex, ucATmp);
	//			ucAIndex += ucATmp;
	//			ucIndex += ucATmp;
	//			ucACount -= ucATmp;
	//		}
	//		if (ucBCount > 0)
	//		{
	//			ucBTmp = ucACount>0?(rand()%2 + 1):ucBCount;
	//			if (ucBTmp > ucBCount)
	//			{
	//				ucBTmp = ucBCount;
	//			}
	//			memcpy(cbLeftCardData+ucIndex, ucBData+ucBIndex, ucBTmp);
	//			ucBIndex += ucBTmp;
	//			ucIndex += ucBTmp;
	//			ucBCount -= ucBTmp;
	//		}
	//	}
	//}
	////将炸弹和剩下扑克组合
	//uint8_t ucIndex = 0;
	//uint8_t ucData = 0;
	////保持底牌不变
	//ucLeftCardCount -= 3;
	//uint8_t cbBankerCard[3] = {0};
	//CopyMemory(cbBankerCard, cbLeftCardData+ucLeftCardCount, sizeof(cbBankerCard));
	//for (int i=0; i!=vecIndex.size(); ++i)
	//{
	//	uint8_t ucRand = rand()%13;
	//	if (ucRand > ucLeftCardCount || i==(vecIndex.size()-1))
	//	{
	//		ucRand = ucLeftCardCount;
	//	}
	//	CopyMemory(cbCardData+ucIndex, cbLeftCardData+ucData, ucRand*sizeof(cbCardData[0]));
	//	ucLeftCardCount -= ucRand;
	//	ucData += ucRand;
	//	ucIndex += ucRand;
	//	CopyMemory(cbCardData+ucIndex, ucBomData[i], 4*sizeof(cbCardData[0]));
	//	ucIndex += 4*sizeof(cbCardData[0]);
	//}
	//CopyMemory(cbCardData+ucIndex, cbBankerCard, sizeof(cbBankerCard));
	//CopyMemory(cbCardBuffer, cbCardData, sizeof(cbCardData));

    return;
}

VOID CGameLogic::RandCardListEX(uint8_t cbCardBuffer[], uint8_t cbBufferCount)
{
    CopyMemory(cbCardBuffer, m_cbCardData, sizeof(m_cbCardData));
}

//删除扑克
bool CGameLogic::RemoveCardList(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount)
{
    //检验数据
    ASSERT(cbRemoveCount<=cbCardCount);

    //定义变量
    uint8_t cbDeleteCount=0,cbTempCardData[MAX_COUNT]={0};
    if (cbCardCount>CountArray(cbTempCardData)) return false;
    CopyMemory(cbTempCardData,cbCardData,sizeof(uint8_t)*cbCardCount);

    //置零扑克
    for (uint8_t i=0;i<cbRemoveCount;i++)
    {
        for (uint8_t j=0;j<cbCardCount;j++)
        {
            if (cbRemoveCard[i]==cbTempCardData[j])
            {
                cbDeleteCount++;
                cbTempCardData[j]=0;
                break;
            }
        }
    }
    if (cbDeleteCount!=cbRemoveCount) return false;

	memset(cbCardData, 0, sizeof(uint8_t)*cbCardCount);
    //清理扑克
    uint8_t cbCardPos=0;
    for (uint8_t i=0;i<cbCardCount;i++)
    {
        if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
    }

    return true;
}

//逻辑数值
uint8_t CGameLogic::GetCardLogicValue(uint8_t cbCardData)
{
    //扑克属性
    uint8_t cbCardColor=GetCardColor(cbCardData);
    uint8_t cbCardValue=GetCardValue(cbCardData);

    //转换数值
    if (cbCardColor==0x40) return cbCardValue+2;
    return (cbCardValue<=2)?(cbCardValue+13):cbCardValue;
}

//对比扑克
bool CGameLogic::CompareCard(const uint8_t cbFirstCard[], const uint8_t cbNextCard[], uint8_t cbFirstCount, uint8_t cbNextCount)
{
    //获取类型
    uint8_t cbNextType=GetCardType(cbNextCard,cbNextCount);
    uint8_t cbFirstType=GetCardType(cbFirstCard,cbFirstCount);

    //类型判断
    if (cbNextType==CT_ERROR) return false;
    if (cbNextType==CT_MISSILE_CARD) return true;
	if (cbFirstType == CT_MISSILE_CARD) return false;
    //炸弹判断
    if ((cbFirstType!=CT_BOMB_CARD)&&(cbNextType==CT_BOMB_CARD)) return true;
    if ((cbFirstType==CT_BOMB_CARD)&&(cbNextType!=CT_BOMB_CARD)) return false;

    //规则判断
    if ((cbFirstType!=cbNextType)||(cbFirstCount!=cbNextCount)) return false;

    //开始对比
    switch (cbNextType)
    {
    case CT_SINGLE:
    case CT_DOUBLE_CARD:
    case CT_THREE:
    case CT_SINGLE_LINE:
    case CT_DOUBLE_LINE:
    case CT_THREE_LINE:
    case CT_BOMB_CARD:
        {
            //获取数值
            uint8_t cbNextLogicValue=GetCardLogicValue(cbNextCard[0]);
            uint8_t cbFirstLogicValue=GetCardLogicValue(cbFirstCard[0]);

            //对比扑克
            return cbNextLogicValue>cbFirstLogicValue;
        }
    case CT_THREE_TAKE_ONE:
    case CT_THREE_TAKE_TWO:
        {
            //分析扑克
            tagAnalyseResult NextResult;
            tagAnalyseResult FirstResult;
            AnalysebCardData(cbNextCard,cbNextCount,NextResult);
            AnalysebCardData(cbFirstCard,cbFirstCount,FirstResult);

            //获取数值
            uint8_t cbNextLogicValue=GetCardLogicValue(NextResult.cbCardData[2][0]);
            uint8_t cbFirstLogicValue=GetCardLogicValue(FirstResult.cbCardData[2][0]);

            //对比扑克
            return cbNextLogicValue>cbFirstLogicValue;
        }
    case CT_FOUR_TAKE_ONE:
    case CT_FOUR_TAKE_TWO:
        {
            //分析扑克
            tagAnalyseResult NextResult;
            tagAnalyseResult FirstResult;
            AnalysebCardData(cbNextCard,cbNextCount,NextResult);
            AnalysebCardData(cbFirstCard,cbFirstCount,FirstResult);

            //获取数值
            uint8_t cbNextLogicValue=GetCardLogicValue(NextResult.cbCardData[3][0]);
            uint8_t cbFirstLogicValue=GetCardLogicValue(FirstResult.cbCardData[3][0]);

            //对比扑克
            return cbNextLogicValue>cbFirstLogicValue;
        }
    }

    return false;
}

//构造扑克
uint8_t CGameLogic::MakeCardData(uint8_t cbValueIndex, uint8_t cbColorIndex)
{
    return (cbColorIndex<<4)|(cbValueIndex+1);
}

//分析扑克
VOID CGameLogic::AnalysebCardData(const uint8_t cbCardData[], uint8_t cbCardCount, tagAnalyseResult & AnalyseResult)
{
    //设置结果
    ZeroMemory(&AnalyseResult,sizeof(AnalyseResult));

    //扑克分析
    for (uint8_t i=0;i<cbCardCount;i++)
    {
        //变量定义
        uint8_t cbSameCount=1,cbCardValueTemp=0;
        uint8_t cbLogicValue=GetCardLogicValue(cbCardData[i]);

        //搜索同牌
        for (uint8_t j=i+1;j<cbCardCount;j++)
        {
            //获取扑克
            if (GetCardLogicValue(cbCardData[j])!=cbLogicValue) break;

            //设置变量
            cbSameCount++;
        }

        if(cbSameCount>4)
        {
            ASSERT(FALSE);
            //设置结果
            ZeroMemory(&AnalyseResult,sizeof(AnalyseResult));
            return;
        }

        //设置结果
        uint8_t cbIndex=AnalyseResult.cbBlockCount[cbSameCount-1]++;
        for (uint8_t j=0;j<cbSameCount;j++) AnalyseResult.cbCardData[cbSameCount-1][cbIndex*cbSameCount+j]=cbCardData[i+j];

        //设置索引
        i+=cbSameCount-1;
    }

    return;
}

//分析分布
VOID CGameLogic::AnalysebDistributing(const uint8_t cbCardData[], uint8_t cbCardCount, tagDistributing & Distributing)
{
    //设置变量
    ZeroMemory(&Distributing,sizeof(Distributing));

    //设置变量
    for (uint8_t i=0;i<cbCardCount;i++)
    {
        if (cbCardData[i]==0) continue;

        //获取属性
        uint8_t cbCardColor=GetCardColor(cbCardData[i]);
        uint8_t cbCardValue=GetCardValue(cbCardData[i]);

        //分布信息
        Distributing.cbCardCount++;
        Distributing.cbDistributing[cbCardValue-1][cbIndexCount]++;
        Distributing.cbDistributing[cbCardValue-1][cbCardColor>>4]++;
    }

    return;
}

//出牌搜索
uint8_t CGameLogic::SearchOutCard( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                               tagSearchCardResult *pSearchCardResult )
{
    //设置结果
    ASSERT( pSearchCardResult != NULL );
    if( pSearchCardResult == NULL )
        return 0;

    ZeroMemory(pSearchCardResult,sizeof(tagSearchCardResult));

    //变量定义
    uint8_t cbResultCount = 0;
    tagSearchCardResult tmpSearchCardResult = {};

    //构造扑克
    uint8_t cbCardData[MAX_COUNT] = { 0 };
    uint8_t cbCardCount=cbHandCardCount;
    CopyMemory(cbCardData,cbHandCardData,sizeof(uint8_t)*cbHandCardCount);

    //排列扑克
    SortCardList(cbCardData,cbCardCount,ST_ORDER);

    //获取类型
    uint8_t cbTurnOutType=GetCardType(cbTurnCardData,cbTurnCardCount);

    //出牌分析
    switch (cbTurnOutType)
    {
    case CT_ERROR:					//错误类型
        {
            //提取各种牌型一组
            ASSERT( pSearchCardResult );
            if( !pSearchCardResult ) return 0;

            //是否一手出完
            if( GetCardType(cbCardData,cbCardCount) != CT_ERROR )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = cbCardCount;
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],cbCardData,
                    sizeof(uint8_t)*cbCardCount );
                cbResultCount++;
            }

            //如果最小牌不是单牌，则提取
            uint8_t cbSameCount = 0;
            if( cbCardCount > 1 && GetCardValue(cbCardData[cbCardCount-1]) == GetCardValue(cbCardData[cbCardCount-2]) )
            {
                cbSameCount = 1;
                pSearchCardResult->cbResultCard[cbResultCount][0] = cbCardData[cbCardCount-1];
                uint8_t cbCardValue = GetCardValue(cbCardData[cbCardCount-1]);
                for( int i = cbCardCount-2; i >= 0; i-- )
                {
                    if( GetCardValue(cbCardData[i]) == cbCardValue )
                    {
                        pSearchCardResult->cbResultCard[cbResultCount][cbSameCount++] = cbCardData[i];
                    }
                    else break;
                }

                pSearchCardResult->cbCardCount[cbResultCount] = cbSameCount;
                cbResultCount++;
            }

            //单牌
            uint8_t cbTmpCount = 0;
            if( cbSameCount != 1 )
            {
                cbTmpCount = SearchSameCard( cbCardData,cbCardCount,0,1,&tmpSearchCardResult );
                if( cbTmpCount > 0 )
                {
                    pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                    CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                        sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                    cbResultCount++;
                }
            }

            //对牌
            if( cbSameCount != 2 )
            {
                cbTmpCount = SearchSameCard( cbCardData,cbCardCount,0,2,&tmpSearchCardResult );
                if( cbTmpCount > 0 )
                {
                    pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                    CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                        sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                    cbResultCount++;
                }
            }

            //三条
            if( cbSameCount != 3 )
            {
                cbTmpCount = SearchSameCard( cbCardData,cbCardCount,0,3,&tmpSearchCardResult );
                if( cbTmpCount > 0 )
                {
                    pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                    CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                        sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                    cbResultCount++;
                }
            }

            //三带一单
            cbTmpCount = SearchTakeCardType( cbCardData,cbCardCount,0,3,1,&tmpSearchCardResult );
            if( cbTmpCount > 0 )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                    sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                cbResultCount++;
            }

            //三带一对
            cbTmpCount = SearchTakeCardType( cbCardData,cbCardCount,0,3,2,&tmpSearchCardResult );
            if( cbTmpCount > 0 )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                    sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                cbResultCount++;
            }

            //单连
            cbTmpCount = SearchLineCardType( cbCardData,cbCardCount,0,1,0,&tmpSearchCardResult );
            if( cbTmpCount > 0 )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                    sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                cbResultCount++;
            }

            //连对
            cbTmpCount = SearchLineCardType( cbCardData,cbCardCount,0,2,0,&tmpSearchCardResult );
            if( cbTmpCount > 0 )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                    sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                cbResultCount++;
            }

            //三连
            cbTmpCount = SearchLineCardType( cbCardData,cbCardCount,0,3,0,&tmpSearchCardResult );
            if( cbTmpCount > 0 )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                    sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                cbResultCount++;
            }

            //飞机
            cbTmpCount = SearchThreeTwoLine( cbCardData,cbCardCount,&tmpSearchCardResult );
            if( cbTmpCount > 0 )
            {
                pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                    sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                cbResultCount++;
            }

            //炸弹
            if( cbSameCount != 4 )
            {
                cbTmpCount = SearchSameCard( cbCardData,cbCardCount,0,4,&tmpSearchCardResult );
                if( cbTmpCount > 0 )
                {
                    pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[0];
                    CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[0],
                        sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[0] );
                    cbResultCount++;
                }
            }

            //搜索火箭
            if ((cbCardCount>=2)&&(cbCardData[0]==0x4F)&&(cbCardData[1]==0x4E))
            {
                //设置结果
                pSearchCardResult->cbCardCount[cbResultCount] = 2;
                pSearchCardResult->cbResultCard[cbResultCount][0] = cbCardData[0];
                pSearchCardResult->cbResultCard[cbResultCount][1] = cbCardData[1];

                cbResultCount++;
            }

            pSearchCardResult->cbSearchCount = cbResultCount;
            return cbResultCount;
        }
    case CT_SINGLE:					//单牌类型
    case CT_DOUBLE_CARD:					//对牌类型
    case CT_THREE:					//三条类型
        {
            //变量定义
            uint8_t cbReferCard=cbTurnCardData[0];
            uint8_t cbSameCount = 1;
            if( cbTurnOutType == CT_DOUBLE_CARD ) cbSameCount = 2;
            else if( cbTurnOutType == CT_THREE ) cbSameCount = 3;

            //搜索相同牌
            cbResultCount = SearchSameCard( cbCardData,cbCardCount,cbReferCard,cbSameCount,pSearchCardResult );

            break;
        }
    case CT_SINGLE_LINE:		//单连类型
    case CT_DOUBLE_LINE:		//对连类型
    case CT_THREE_LINE:				//三连类型
        {
            //变量定义
            uint8_t cbBlockCount = 1;
            if( cbTurnOutType == CT_DOUBLE_LINE ) cbBlockCount = 2;
            else if( cbTurnOutType == CT_THREE_LINE ) cbBlockCount = 3;

            uint8_t cbLineCount = cbTurnCardCount/cbBlockCount;

            //搜索边牌
            cbResultCount = SearchLineCardType( cbCardData,cbCardCount,cbTurnCardData[0],cbBlockCount,cbLineCount,pSearchCardResult );

            break;
        }
    case CT_THREE_TAKE_ONE:	//三带一单
    case CT_THREE_TAKE_TWO:	//三带一对
        {
            //效验牌数
            if( cbCardCount < cbTurnCardCount ) break;

            //如果是三带一或三带二
            if( cbTurnCardCount == 4 || cbTurnCardCount == 5 )
            {
                uint8_t cbTakeCardCount = cbTurnOutType==CT_THREE_TAKE_ONE?1:2;

                //搜索三带牌型
                cbResultCount = SearchTakeCardType( cbCardData,cbCardCount,cbTurnCardData[2],3,cbTakeCardCount,pSearchCardResult );
            }
            else
            {
                //变量定义
                uint8_t cbBlockCount = 3;
                uint8_t cbLineCount = cbTurnCardCount/(cbTurnOutType==CT_THREE_TAKE_ONE?4:5);
                uint8_t cbTakeCardCount = cbTurnOutType==CT_THREE_TAKE_ONE?1:2;

                //搜索连牌
                uint8_t cbTmpTurnCard[MAX_COUNT] = {0};
                CopyMemory( cbTmpTurnCard,cbTurnCardData,sizeof(uint8_t)*cbTurnCardCount );
                SortOutCardList( cbTmpTurnCard,cbTurnCardCount );
                cbResultCount = SearchLineCardType( cbCardData,cbCardCount,cbTmpTurnCard[0],cbBlockCount,cbLineCount,pSearchCardResult );

                //提取带牌
                bool bAllDistill = true;
                for( uint8_t i = 0; i < cbResultCount; i++ )
                {
                    uint8_t cbResultIndex = cbResultCount-i-1;

                    //变量定义
                    uint8_t cbTmpCardData[MAX_COUNT] = { 0 };
                    uint8_t cbTmpCardCount = cbCardCount;

                    //删除连牌
                    CopyMemory( cbTmpCardData,cbCardData,sizeof(uint8_t)*cbCardCount );
                   // VERIFY(RemoveCard( pSearchCardResult->cbResultCard[cbResultIndex],pSearchCardResult->cbCardCount[cbResultIndex],
                    //    cbTmpCardData,cbTmpCardCount));
                    cbTmpCardCount -= pSearchCardResult->cbCardCount[cbResultIndex];

                    //分析牌
                    tagAnalyseResult  TmpResult = {};
                    AnalysebCardData( cbTmpCardData,cbTmpCardCount,TmpResult );

                    //提取牌
                    uint8_t cbDistillCard[MAX_COUNT] = {0};
                    uint8_t cbDistillCount = 0;
                    for( uint8_t j = cbTakeCardCount-1; j < CountArray(TmpResult.cbBlockCount); j++ )
                    {
                        if( TmpResult.cbBlockCount[j] > 0 )
                        {
                            if( j+1 == cbTakeCardCount && TmpResult.cbBlockCount[j] >= cbLineCount )
                            {
                                uint8_t cbTmpBlockCount = TmpResult.cbBlockCount[j];
                                CopyMemory( cbDistillCard,&TmpResult.cbCardData[j][(cbTmpBlockCount-cbLineCount)*(j+1)],
                                    sizeof(uint8_t)*(j+1)*cbLineCount );
                                cbDistillCount = (j+1)*cbLineCount;
                                break;
                            }
                            else
                            {
                                for( uint8_t k = 0; k < TmpResult.cbBlockCount[j]; k++ )
                                {
                                    uint8_t cbTmpBlockCount = TmpResult.cbBlockCount[j];
                                    CopyMemory( &cbDistillCard[cbDistillCount],&TmpResult.cbCardData[j][(cbTmpBlockCount-k-1)*(j+1)],
                                        sizeof(uint8_t)*cbTakeCardCount );
                                    cbDistillCount += cbTakeCardCount;

                                    //提取完成
                                    if( cbDistillCount == cbTakeCardCount*cbLineCount ) break;
                                }
                            }
                        }

                        //提取完成
                        if( cbDistillCount == cbTakeCardCount*cbLineCount ) break;
                    }

                    //提取完成
                    if( cbDistillCount == cbTakeCardCount*cbLineCount )
                    {
                        //复制带牌
                        uint8_t cbCount = pSearchCardResult->cbCardCount[cbResultIndex];
                        CopyMemory( &pSearchCardResult->cbResultCard[cbResultIndex][cbCount],cbDistillCard,
                            sizeof(uint8_t)*cbDistillCount );
                        pSearchCardResult->cbCardCount[cbResultIndex] += cbDistillCount;
                    }
                    //否则删除连牌
                    else
                    {
                        bAllDistill = false;
                        pSearchCardResult->cbCardCount[cbResultIndex] = 0;
                    }
                }

                //整理组合
                if( !bAllDistill )
                {
                    pSearchCardResult->cbSearchCount = cbResultCount;
                    cbResultCount = 0;
                    for( uint8_t i = 0; i < pSearchCardResult->cbSearchCount; i++ )
                    {
                        if( pSearchCardResult->cbCardCount[i] != 0 )
                        {
                            tmpSearchCardResult.cbCardCount[cbResultCount] = pSearchCardResult->cbCardCount[i];
                            CopyMemory( tmpSearchCardResult.cbResultCard[cbResultCount],pSearchCardResult->cbResultCard[i],
                                sizeof(uint8_t)*pSearchCardResult->cbCardCount[i] );
                            cbResultCount++;
                        }
                    }
                    tmpSearchCardResult.cbSearchCount = cbResultCount;
                    CopyMemory( pSearchCardResult,&tmpSearchCardResult,sizeof(tagSearchCardResult) );
                }
            }

            break;
        }
    case CT_FOUR_TAKE_ONE:		//四带两单
    case CT_FOUR_TAKE_TWO:		//四带两双
        {
            uint8_t cbTakeCount = cbTurnOutType==CT_FOUR_TAKE_ONE?1:2;

            uint8_t cbTmpTurnCard[MAX_COUNT] = {0};
            CopyMemory( cbTmpTurnCard,cbTurnCardData,sizeof(uint8_t)*cbTurnCardCount );
            SortOutCardList( cbTmpTurnCard,cbTurnCardCount );

            //搜索带牌
            cbResultCount = SearchTakeCardType( cbCardData,cbCardCount,cbTmpTurnCard[0],4,cbTakeCount,pSearchCardResult );

            break;
        }
    }

    //搜索炸弹
    if ((cbCardCount>=4)&&(cbTurnOutType!=CT_MISSILE_CARD))
    {
        //变量定义
        uint8_t cbReferCard = 0;
        if (cbTurnOutType==CT_BOMB_CARD) cbReferCard=cbTurnCardData[0];

        //搜索炸弹
        uint8_t cbTmpResultCount = SearchSameCard( cbCardData,cbCardCount,cbReferCard,4,&tmpSearchCardResult );
        for( uint8_t i = 0; i < cbTmpResultCount; i++ )
        {
            pSearchCardResult->cbCardCount[cbResultCount] = tmpSearchCardResult.cbCardCount[i];
            CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSearchCardResult.cbResultCard[i],
                sizeof(uint8_t)*tmpSearchCardResult.cbCardCount[i] );
            cbResultCount++;
        }
    }

    //搜索火箭
    if (cbTurnOutType!=CT_MISSILE_CARD&&(cbCardCount>=2)&&(cbCardData[0]==0x4F)&&(cbCardData[1]==0x4E))
    {
        //设置结果
        pSearchCardResult->cbCardCount[cbResultCount] = 2;
        pSearchCardResult->cbResultCard[cbResultCount][0] = cbCardData[0];
        pSearchCardResult->cbResultCard[cbResultCount][1] = cbCardData[1];

        cbResultCount++;
    }

    pSearchCardResult->cbSearchCount = cbResultCount;
    return cbResultCount;
}

//同牌搜索
uint8_t CGameLogic::SearchSameCard( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint8_t cbReferCard, uint8_t cbSameCardCount,
                                tagSearchCardResult *pSearchCardResult )
{
    //设置结果
    if( pSearchCardResult )
        ZeroMemory(pSearchCardResult,sizeof(tagSearchCardResult));
    uint8_t cbResultCount = 0;

    //构造扑克
    uint8_t cbCardData[MAX_COUNT] = { 0 };
    uint8_t cbCardCount=cbHandCardCount;
    CopyMemory(cbCardData,cbHandCardData,sizeof(uint8_t)*cbHandCardCount);

    //排列扑克
    SortCardList(cbCardData,cbCardCount,ST_ORDER);

    //分析扑克
    tagAnalyseResult AnalyseResult = {};
    AnalysebCardData( cbCardData,cbCardCount,AnalyseResult );

    uint8_t cbReferLogicValue = cbReferCard==0?0:GetCardLogicValue(cbReferCard);
    uint8_t cbBlockIndex = cbSameCardCount-1;
    do
    {
        for( uint8_t i = 0; i < AnalyseResult.cbBlockCount[cbBlockIndex]; i++ )
        {
            uint8_t cbIndex = (AnalyseResult.cbBlockCount[cbBlockIndex]-i-1)*(cbBlockIndex+1);
            if( GetCardLogicValue(AnalyseResult.cbCardData[cbBlockIndex][cbIndex]) > cbReferLogicValue )
            {
                if( pSearchCardResult == NULL ) return 1;

                ASSERT( cbResultCount < CountArray(pSearchCardResult->cbCardCount) );

                //复制扑克
                CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],&AnalyseResult.cbCardData[cbBlockIndex][cbIndex],
                    cbSameCardCount*sizeof(uint8_t) );
                pSearchCardResult->cbCardCount[cbResultCount] = cbSameCardCount;

                cbResultCount++;
            }
        }

        cbBlockIndex++;
    }while( cbBlockIndex < CountArray(AnalyseResult.cbBlockCount) );

    if( pSearchCardResult )
        pSearchCardResult->cbSearchCount = cbResultCount;
    return cbResultCount;
}

//带牌类型搜索(三带一，四带一等)
uint8_t CGameLogic::SearchTakeCardType( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint8_t cbReferCard, uint8_t cbSameCount, uint8_t cbTakeCardCount,
                                    tagSearchCardResult *pSearchCardResult )
{
    //设置结果
    if( pSearchCardResult )
        ZeroMemory(pSearchCardResult,sizeof(tagSearchCardResult));
    uint8_t cbResultCount = 0;

    //效验
    ASSERT( cbSameCount == 3 || cbSameCount == 4 );
    ASSERT( cbTakeCardCount == 1 || cbTakeCardCount == 2 );
    if( cbSameCount != 3 && cbSameCount != 4 )
        return cbResultCount;
    if( cbTakeCardCount != 1 && cbTakeCardCount != 2 )
        return cbResultCount;

    //长度判断
    if( cbSameCount == 4 && cbHandCardCount<cbSameCount+cbTakeCardCount*2 ||
        cbHandCardCount < cbSameCount+cbTakeCardCount )
        return cbResultCount;

    //构造扑克
    uint8_t cbCardData[MAX_COUNT] = { 0 };
    uint8_t cbCardCount=cbHandCardCount;
    CopyMemory(cbCardData,cbHandCardData,sizeof(uint8_t)*cbHandCardCount);

    //排列扑克
    SortCardList(cbCardData,cbCardCount,ST_ORDER);

    //搜索同张
    tagSearchCardResult SameCardResult = {};
    uint8_t cbSameCardResultCount = SearchSameCard( cbCardData,cbCardCount,cbReferCard,cbSameCount,&SameCardResult );

    if( cbSameCardResultCount > 0 )
    {
        //分析扑克
        tagAnalyseResult AnalyseResult;
        AnalysebCardData(cbCardData,cbCardCount,AnalyseResult);

        //需要牌数
        uint8_t cbNeedCount = cbSameCount+cbTakeCardCount;
        if( cbSameCount == 4 ) cbNeedCount += cbTakeCardCount;

        //提取带牌
        for( uint8_t i = 0; i < cbSameCardResultCount; i++ )
        {
            bool bMerge = false;

            for( uint8_t j = cbTakeCardCount-1; j < CountArray(AnalyseResult.cbBlockCount); j++ )
            {
                for( uint8_t k = 0; k < AnalyseResult.cbBlockCount[j]; k++ )
                {
                    //从小到大
                    uint8_t cbIndex = (AnalyseResult.cbBlockCount[j]-k-1)*(j+1);

                    //过滤相同牌
                    if( GetCardValue(SameCardResult.cbResultCard[i][0]) ==
                        GetCardValue(AnalyseResult.cbCardData[j][cbIndex]) )
                        continue;

                    //复制带牌
                    uint8_t cbCount = SameCardResult.cbCardCount[i];
                    CopyMemory( &SameCardResult.cbResultCard[i][cbCount],&AnalyseResult.cbCardData[j][cbIndex],
                        sizeof(uint8_t)*cbTakeCardCount );
                    SameCardResult.cbCardCount[i] += cbTakeCardCount;

                    if( SameCardResult.cbCardCount[i] < cbNeedCount ) continue;

                    if( pSearchCardResult == NULL ) return 1;

                    //复制结果
                    CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],SameCardResult.cbResultCard[i],
                        sizeof(uint8_t)*SameCardResult.cbCardCount[i] );
                    pSearchCardResult->cbCardCount[cbResultCount] = SameCardResult.cbCardCount[i];
                    cbResultCount++;

                    bMerge = true;

                    //下一组合
                    break;
                }

                if( bMerge ) break;
            }
        }
    }

    if( pSearchCardResult )
        pSearchCardResult->cbSearchCount = cbResultCount;
    return cbResultCount;
}


//排列扑克
VOID CGameLogic::SortOutCardList(uint8_t cbCardData[], uint8_t cbCardCount)
{
    //获取牌型
    uint8_t cbCardType = GetCardType(cbCardData,cbCardCount);

    if( cbCardType == CT_THREE_TAKE_ONE || cbCardType == CT_THREE_TAKE_TWO )
    {
        //分析牌
        tagAnalyseResult AnalyseResult = {};
        AnalysebCardData( cbCardData,cbCardCount,AnalyseResult );

        cbCardCount = AnalyseResult.cbBlockCount[2]*3;
        CopyMemory( cbCardData,AnalyseResult.cbCardData[2],sizeof(uint8_t)*cbCardCount );
        for( int i = CountArray(AnalyseResult.cbBlockCount)-1; i >= 0; i-- )
        {
            if( i == 2 ) continue;

            if( AnalyseResult.cbBlockCount[i] > 0 )
            {
                CopyMemory( &cbCardData[cbCardCount],AnalyseResult.cbCardData[i],
                    sizeof(uint8_t)*(i+1)*AnalyseResult.cbBlockCount[i] );
                cbCardCount += (i+1)*AnalyseResult.cbBlockCount[i];
            }
        }
    }
    else if( cbCardType == CT_FOUR_TAKE_ONE || cbCardType == CT_FOUR_TAKE_TWO )
    {
        //分析牌
        tagAnalyseResult AnalyseResult = {};
        AnalysebCardData( cbCardData,cbCardCount,AnalyseResult );

        cbCardCount = AnalyseResult.cbBlockCount[3]*4;
        CopyMemory( cbCardData,AnalyseResult.cbCardData[3],sizeof(uint8_t)*cbCardCount );
        for( int i = CountArray(AnalyseResult.cbBlockCount)-1; i >= 0; i-- )
        {
            if( i == 3 ) continue;

            if( AnalyseResult.cbBlockCount[i] > 0 )
            {
                CopyMemory( &cbCardData[cbCardCount],AnalyseResult.cbCardData[i],
                    sizeof(uint8_t)*(i+1)*AnalyseResult.cbBlockCount[i] );
                cbCardCount += (i+1)*AnalyseResult.cbBlockCount[i];
            }
        }
    }

    return;
}

//删除扑克
bool CGameLogic::RemoveCard(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount)
{
    //检验数据
    ASSERT(cbRemoveCount<=cbCardCount);

    //定义变量
    uint8_t cbDeleteCount=0,cbTempCardData[MAX_COUNT] = { 0 };
    if (cbCardCount>CountArray(cbTempCardData)) return false;
    CopyMemory(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

    //置零扑克
    for (uint8_t i=0;i<cbRemoveCount;i++)
    {
        for (uint8_t j=0;j<cbCardCount;j++)
        {
            if (cbRemoveCard[i]==cbTempCardData[j])
            {
                cbDeleteCount++;
                cbTempCardData[j]=0;
                break;
            }
        }
    }
    if (cbDeleteCount!=cbRemoveCount) return false;

    //清理扑克
    uint8_t cbCardPos=0;
    for (uint8_t i=0;i<cbCardCount;i++)
    {
        if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
    }

    return true;
}
//连牌搜索
uint8_t CGameLogic::SearchLineCardType( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint8_t cbReferCard, uint8_t cbBlockCount, uint8_t cbLineCount,
                                    tagSearchCardResult *pSearchCardResult )
{
    //设置结果
    if( pSearchCardResult )
        ZeroMemory(pSearchCardResult,sizeof(tagSearchCardResult));
    uint8_t cbResultCount = 0;

    //定义变量
    uint8_t cbLessLineCount = 0;
    if( cbLineCount == 0 )
    {
        if( cbBlockCount == 1 )
            cbLessLineCount = 5;
        else if( cbBlockCount == 2 )
            cbLessLineCount = 3;
        else cbLessLineCount = 2;
    }
    else cbLessLineCount = cbLineCount;

    uint8_t cbReferIndex = 2;
    if( cbReferCard != 0 )
    {
        ASSERT( GetCardLogicValue(cbReferCard)-cbLessLineCount>=2 );
        cbReferIndex = GetCardLogicValue(cbReferCard)-cbLessLineCount+1;
    }
    //超过A
    if( cbReferIndex+cbLessLineCount > 14 ) return cbResultCount;

    //长度判断
    if( cbHandCardCount < cbLessLineCount*cbBlockCount ) return cbResultCount;

    //构造扑克
    uint8_t cbCardData[MAX_COUNT] = { 0 };
    uint8_t cbCardCount=cbHandCardCount;
    CopyMemory(cbCardData,cbHandCardData,sizeof(uint8_t)*cbHandCardCount);

    //排列扑克
    SortCardList(cbCardData,cbCardCount,ST_ORDER);

    //分析扑克
    tagDistributing Distributing = {};
    AnalysebDistributing(cbCardData,cbCardCount,Distributing);

    //搜索顺子
    uint8_t cbTmpLinkCount = 0;
    uint8_t cbValueIndex = 0;
    for (cbValueIndex=cbReferIndex;cbValueIndex<13;cbValueIndex++)
    {
        //继续判断
        if ( Distributing.cbDistributing[cbValueIndex][cbIndexCount]<cbBlockCount )
        {
            if( cbTmpLinkCount < cbLessLineCount )
            {
                cbTmpLinkCount=0;
                continue;
            }
            else cbValueIndex--;
        }
        else
        {
            cbTmpLinkCount++;
            //寻找最长连
            if( cbLineCount == 0 ) continue;
        }

        if( cbTmpLinkCount >= cbLessLineCount )
        {
            if( pSearchCardResult == NULL ) return 1;

            ASSERT( cbResultCount < CountArray(pSearchCardResult->cbCardCount) );

            //复制扑克
            uint8_t cbCount = 0;
            for( uint8_t cbIndex = cbValueIndex+1-cbTmpLinkCount; cbIndex <= cbValueIndex; cbIndex++ )
            {
                uint8_t cbTmpCount = 0;
                for (uint8_t cbColorIndex=0;cbColorIndex<4;cbColorIndex++)
                {
                    for( uint8_t cbColorCount = 0; cbColorCount < Distributing.cbDistributing[cbIndex][3-cbColorIndex]; cbColorCount++ )
                    {
                        pSearchCardResult->cbResultCard[cbResultCount][cbCount++]=MakeCardData(cbIndex,3-cbColorIndex);

                        if( ++cbTmpCount == cbBlockCount ) break;
                    }
                    if( cbTmpCount == cbBlockCount ) break;
                }
            }

            //设置变量
            pSearchCardResult->cbCardCount[cbResultCount] = cbCount;
            cbResultCount++;

            if( cbLineCount != 0 )
            {
                cbTmpLinkCount--;
            }
            else
            {
                cbTmpLinkCount = 0;
            }
        }
    }

    //特殊顺子
    if( cbTmpLinkCount >= cbLessLineCount-1 && cbValueIndex == 13 )
    {
        if( Distributing.cbDistributing[0][cbIndexCount] >= cbBlockCount ||
            cbTmpLinkCount >= cbLessLineCount )
        {
            if( pSearchCardResult == NULL ) return 1;

            ASSERT( cbResultCount < CountArray(pSearchCardResult->cbCardCount) );

            //复制扑克
            uint8_t cbCount = 0;
            uint8_t cbTmpCount = 0;
            for( uint8_t cbIndex = cbValueIndex-cbTmpLinkCount; cbIndex < 13; cbIndex++ )
            {
                cbTmpCount = 0;
                for (uint8_t cbColorIndex=0;cbColorIndex<4;cbColorIndex++)
                {
                    for( uint8_t cbColorCount = 0; cbColorCount < Distributing.cbDistributing[cbIndex][3-cbColorIndex]; cbColorCount++ )
                    {
                        pSearchCardResult->cbResultCard[cbResultCount][cbCount++]=MakeCardData(cbIndex,3-cbColorIndex);

                        if( ++cbTmpCount == cbBlockCount ) break;
                    }
                    if( cbTmpCount == cbBlockCount ) break;
                }
            }
            //复制A
            if( Distributing.cbDistributing[0][cbIndexCount] >= cbBlockCount )
            {
                cbTmpCount = 0;
                for (uint8_t cbColorIndex=0;cbColorIndex<4;cbColorIndex++)
                {
                    for( uint8_t cbColorCount = 0; cbColorCount < Distributing.cbDistributing[0][3-cbColorIndex]; cbColorCount++ )
                    {
                        pSearchCardResult->cbResultCard[cbResultCount][cbCount++]=MakeCardData(0,3-cbColorIndex);

                        if( ++cbTmpCount == cbBlockCount ) break;
                    }
                    if( cbTmpCount == cbBlockCount ) break;
                }
            }

            //设置变量
            pSearchCardResult->cbCardCount[cbResultCount] = cbCount;
            cbResultCount++;
        }
    }

    if( pSearchCardResult )
        pSearchCardResult->cbSearchCount = cbResultCount;
    return cbResultCount;
}

//搜索飞机
uint8_t CGameLogic::SearchThreeTwoLine( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, tagSearchCardResult *pSearchCardResult )
{
    //设置结果
    if( pSearchCardResult )
        ZeroMemory(pSearchCardResult,sizeof(tagSearchCardResult));

    //变量定义
    tagSearchCardResult tmpSearchResult = {};
    tagSearchCardResult tmpSingleWing = {};
    tagSearchCardResult tmpDoubleWing = {};
    uint8_t cbTmpResultCount = 0;

    //搜索连牌
    cbTmpResultCount = SearchLineCardType( cbHandCardData,cbHandCardCount,0,3,0,&tmpSearchResult );

    if( cbTmpResultCount > 0 )
    {
        //提取带牌
        for( uint8_t i = 0; i < cbTmpResultCount; i++ )
        {
            //变量定义
            uint8_t cbTmpCardData[MAX_COUNT] = { 0 };
            uint8_t cbTmpCardCount = cbHandCardCount;

            //不够牌
            if( cbHandCardCount-tmpSearchResult.cbCardCount[i] < tmpSearchResult.cbCardCount[i]/3 )
            {
                uint8_t cbNeedDelCount = 3;
                while( cbHandCardCount+cbNeedDelCount-tmpSearchResult.cbCardCount[i] < (tmpSearchResult.cbCardCount[i]-cbNeedDelCount)/3 )
                    cbNeedDelCount += 3;
                //不够连牌
                if( (tmpSearchResult.cbCardCount[i]-cbNeedDelCount)/3 < 2 )
                {
                    //废除连牌
                    continue;
                }

                //拆分连牌
                RemoveCard( tmpSearchResult.cbResultCard[i],cbNeedDelCount,tmpSearchResult.cbResultCard[i],
                    tmpSearchResult.cbCardCount[i] );
                tmpSearchResult.cbCardCount[i] -= cbNeedDelCount;
            }

            if( pSearchCardResult == NULL ) return 1;

            //删除连牌
            CopyMemory( cbTmpCardData,cbHandCardData,sizeof(uint8_t)*cbHandCardCount );
            //VERIFY( RemoveCard( tmpSearchResult.cbResultCard[i],tmpSearchResult.cbCardCount[i],
            //    cbTmpCardData,cbTmpCardCount ) );
            cbTmpCardCount -= tmpSearchResult.cbCardCount[i];

            //组合飞机
            uint8_t cbNeedCount = tmpSearchResult.cbCardCount[i]/3;
            ASSERT( cbNeedCount <= cbTmpCardCount );

            uint8_t cbResultCount = tmpSingleWing.cbSearchCount++;
            CopyMemory( tmpSingleWing.cbResultCard[cbResultCount],tmpSearchResult.cbResultCard[i],
                sizeof(uint8_t)*tmpSearchResult.cbCardCount[i] );
			RemoveCardList(tmpSearchResult.cbResultCard[i], tmpSearchResult.cbCardCount[i], cbTmpCardData, cbHandCardCount);
            CopyMemory( &tmpSingleWing.cbResultCard[cbResultCount][tmpSearchResult.cbCardCount[i]],
                &cbTmpCardData[cbTmpCardCount-cbNeedCount],sizeof(uint8_t)*cbNeedCount );
            tmpSingleWing.cbCardCount[i] = tmpSearchResult.cbCardCount[i]+cbNeedCount;

            //不够带翅膀
            if( cbTmpCardCount < tmpSearchResult.cbCardCount[i]/3*2 )
            {
                uint8_t cbNeedDelCount = 3;
                while( cbTmpCardCount+cbNeedDelCount-tmpSearchResult.cbCardCount[i] < (tmpSearchResult.cbCardCount[i]-cbNeedDelCount)/3*2 )
                    cbNeedDelCount += 3;
                //不够连牌
                if( (tmpSearchResult.cbCardCount[i]-cbNeedDelCount)/3 < 2 )
                {
                    //废除连牌
                    continue;
                }

                //拆分连牌
                RemoveCard( tmpSearchResult.cbResultCard[i],cbNeedDelCount,tmpSearchResult.cbResultCard[i],
                    tmpSearchResult.cbCardCount[i] );
                tmpSearchResult.cbCardCount[i] -= cbNeedDelCount;

                //重新删除连牌
                CopyMemory( cbTmpCardData,cbHandCardData,sizeof(uint8_t)*cbHandCardCount );
               // VERIFY( RemoveCard( tmpSearchResult.cbResultCard[i],tmpSearchResult.cbCardCount[i],
                //    cbTmpCardData,cbTmpCardCount ) );
                cbTmpCardCount = cbHandCardCount-tmpSearchResult.cbCardCount[i];
            }

            //分析牌
            tagAnalyseResult  TmpResult = {};
            AnalysebCardData( cbTmpCardData,cbTmpCardCount,TmpResult );

            //提取翅膀
            uint8_t cbDistillCard[MAX_COUNT] = { 0 };
            uint8_t cbDistillCount = 0;
            uint8_t cbLineCount = tmpSearchResult.cbCardCount[i]/3;
            for( uint8_t j = 1; j < CountArray(TmpResult.cbBlockCount); j++ )
            {
                if( TmpResult.cbBlockCount[j] > 0 )
                {
                    if( j+1 == 2 && TmpResult.cbBlockCount[j] >= cbLineCount )
                    {
                        uint8_t cbTmpBlockCount = TmpResult.cbBlockCount[j];
                        CopyMemory( cbDistillCard,&TmpResult.cbCardData[j][(cbTmpBlockCount-cbLineCount)*(j+1)],
                            sizeof(uint8_t)*(j+1)*cbLineCount );
                        cbDistillCount = (j+1)*cbLineCount;
                        break;
                    }
                    else
                    {
                        for( uint8_t k = 0; k < TmpResult.cbBlockCount[j]; k++ )
                        {
                            uint8_t cbTmpBlockCount = TmpResult.cbBlockCount[j];
                            CopyMemory( &cbDistillCard[cbDistillCount],&TmpResult.cbCardData[j][(cbTmpBlockCount-k-1)*(j+1)],
                                sizeof(uint8_t)*2 );
                            cbDistillCount += 2;

                            //提取完成
                            if( cbDistillCount == 2*cbLineCount ) break;
                        }
                    }
                }

                //提取完成
                if( cbDistillCount == 2*cbLineCount ) break;
            }

            //提取完成
            if( cbDistillCount == 2*cbLineCount )
            {
                //复制翅膀
                cbResultCount = tmpDoubleWing.cbSearchCount++;
                CopyMemory( tmpDoubleWing.cbResultCard[cbResultCount],tmpSearchResult.cbResultCard[i],
                    sizeof(uint8_t)*tmpSearchResult.cbCardCount[i] );
                CopyMemory( &tmpDoubleWing.cbResultCard[cbResultCount][tmpSearchResult.cbCardCount[i]],
                    cbDistillCard,sizeof(uint8_t)*cbDistillCount );
                tmpDoubleWing.cbCardCount[i] = tmpSearchResult.cbCardCount[i]+cbDistillCount;
            }
        }

        //复制结果
        for( uint8_t i = 0; i < tmpDoubleWing.cbSearchCount; i++ )
        {
            uint8_t cbResultCount = pSearchCardResult->cbSearchCount++;

            CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpDoubleWing.cbResultCard[i],
                sizeof(uint8_t)*tmpDoubleWing.cbCardCount[i] );
            pSearchCardResult->cbCardCount[cbResultCount] = tmpDoubleWing.cbCardCount[i];
        }
        for( uint8_t i = 0; i < tmpSingleWing.cbSearchCount; i++ )
        {
            uint8_t cbResultCount = pSearchCardResult->cbSearchCount++;

            CopyMemory( pSearchCardResult->cbResultCard[cbResultCount],tmpSingleWing.cbResultCard[i],
                sizeof(uint8_t)*tmpSingleWing.cbCardCount[i] );
            pSearchCardResult->cbCardCount[cbResultCount] = tmpSingleWing.cbCardCount[i];
        }
    }

    return pSearchCardResult==NULL?0:pSearchCardResult->cbSearchCount;
}

//得到好牌
void CGameLogic::GetGoodCardData(uint8_t cbGoodCardData[NORMAL_COUNT])
{
    //混乱准备
    uint8_t cbCardData[CountArray(m_cbGoodcardData)];
    uint8_t cbCardBuffer[CountArray(m_cbGoodcardData)];
    CopyMemory(cbCardData,m_cbGoodcardData,sizeof(m_cbGoodcardData));

    //混乱扑克
    uint8_t cbRandCount=0,cbPosition=0;
    uint8_t cbBufferCount=CountArray(m_cbGoodcardData);
    do
    {
        cbPosition=rand()%(cbBufferCount-cbRandCount);
        cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
        cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
    } while (cbRandCount<cbBufferCount);

    //复制好牌
    CopyMemory(cbGoodCardData, cbCardBuffer, NORMAL_COUNT) ;
}

//删除好牌
bool CGameLogic::RemoveGoodCardData(uint8_t cbGoodcardData[NORMAL_COUNT], uint8_t cbGoodCardCount, uint8_t cbCardData[FULL_COUNT], uint8_t cbCardCount)
{
    //检验数据
    ASSERT(cbGoodCardCount<=cbCardCount);
    if(cbGoodCardCount>cbCardCount)
        return false ;

    //定义变量
    uint8_t cbDeleteCount=0,cbTempCardData[FULL_COUNT]={0};
    if (cbCardCount>CountArray(cbTempCardData)) return false;
    CopyMemory(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

    //置零扑克
    for (uint8_t i=0;i<cbGoodCardCount;i++)
    {
        for (uint8_t j=0;j<cbCardCount;j++)
        {
            if (cbGoodcardData[i]==cbTempCardData[j])
            {
                cbDeleteCount++;
                cbTempCardData[j]=0;
                break;
            }
        }
    }
    ASSERT(cbDeleteCount==cbGoodCardCount) ;
    if (cbDeleteCount!=cbGoodCardCount) return false;

    //清理扑克
    uint8_t cbCardPos=0;
    for (uint8_t i=0;i<cbCardCount;i++)
    {
        if (cbTempCardData[i]!=0) cbCardData[cbCardPos++]=cbTempCardData[i];
    }

    return true;
}

//组合算法
VOID CGameLogic::Combination(uint8_t cbCombineCardData[], uint8_t cbResComLen, uint8_t cbResultCardData[100][5], uint8_t &cbResCardLen, uint8_t cbSrcCardData[], uint8_t cbCombineLen1, uint8_t cbSrcLen, const uint8_t cbCombineLen2)
{
	if (cbResComLen == cbCombineLen2)
	{
		CopyMemory(&cbResultCardData[cbResCardLen], cbCombineCardData, cbResComLen);
		++cbResCardLen;
		ASSERT(cbResCardLen < 255);
	}
	else
	{
		if (cbCombineLen1 >= 1 && cbSrcLen > 0 && (cbSrcLen + 1) >= 0) {
			cbCombineCardData[cbCombineLen2 - cbCombineLen1] = cbSrcCardData[0];
			++cbResComLen;
			Combination(cbCombineCardData, cbResComLen, cbResultCardData, cbResCardLen, cbSrcCardData + 1, cbCombineLen1 - 1, cbSrcLen - 1, cbCombineLen2);

			--cbResComLen;
			Combination(cbCombineCardData, cbResComLen, cbResultCardData, cbResCardLen, cbSrcCardData + 1, cbCombineLen1, cbSrcLen - 1, cbCombineLen2);
		}
	}
}
//分析手牌
VOID CGameLogic::AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12 + 1])
{
	ZeroMemory(CardTypeResult, sizeof(CardTypeResult[0]) * 12);
	uint8_t cbTmpCardData[MAX_COUNT] = { 0 };
	//保留扑克，防止分析时改变扑克
	uint8_t cbReserveCardData[MAX_COUNT] = { 0 };
	CopyMemory(cbReserveCardData, cbHandCardData, cbHandCardCount);
	SortCardList(cbReserveCardData, cbHandCardCount, ST_ORDER);
	CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount);
	//单牌类型
	for (uint8_t i = 0; i < cbHandCardCount; ++i)
	{
		uint8_t Index = CardTypeResult[CT_SINGLE].cbCardTypeCount;
		CardTypeResult[CT_SINGLE].cbCardType = CT_SINGLE;
		CardTypeResult[CT_SINGLE].cbCardData[Index][0] = cbTmpCardData[i];
		CardTypeResult[CT_SINGLE].cbEachHandCardCount[Index] = 1;
		CardTypeResult[CT_SINGLE].cbCardTypeCount++;

		ASSERT(CardTypeResult[CT_SINGLE].cbCardTypeCount < MAX_TYPE_COUNT);
	}
	//对牌类型
	{
		uint8_t cbDoubleCardData[MAX_COUNT] = { 0 };
		uint8_t cbDoubleCardcount = 0;
		GetAllDoubleCard(cbTmpCardData, cbHandCardCount, cbDoubleCardData, cbDoubleCardcount);
		for (uint8_t i = 0; i < cbDoubleCardcount; i += 2)
		{
			uint8_t Index = CardTypeResult[CT_DOUBLE_CARD].cbCardTypeCount;
			CardTypeResult[CT_DOUBLE_CARD].cbCardType = CT_DOUBLE_CARD;
			CardTypeResult[CT_DOUBLE_CARD].cbCardData[Index][0] = cbDoubleCardData[i];
			CardTypeResult[CT_DOUBLE_CARD].cbCardData[Index][1] = cbDoubleCardData[i + 1];
			CardTypeResult[CT_DOUBLE_CARD].cbEachHandCardCount[Index] = 2;
			CardTypeResult[CT_DOUBLE_CARD].cbCardTypeCount++;

			ASSERT(CardTypeResult[CT_DOUBLE_CARD].cbCardTypeCount < MAX_TYPE_COUNT);
		}
	}
	//三条类型
	{
		uint8_t cbThreeCardData[MAX_COUNT] = { 0 };
		uint8_t cbThreeCardCount = 0;
		GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbThreeCardData, cbThreeCardCount);
		for (uint8_t i = 0; i < cbThreeCardCount; i += 3)
		{
			uint8_t Index = CardTypeResult[CT_THREE].cbCardTypeCount;
			CardTypeResult[CT_THREE].cbCardType = CT_THREE;
			CardTypeResult[CT_THREE].cbCardData[Index][0] = cbThreeCardData[i];
			CardTypeResult[CT_THREE].cbCardData[Index][1] = cbThreeCardData[i + 1];
			CardTypeResult[CT_THREE].cbCardData[Index][2] = cbThreeCardData[i + 2];
			CardTypeResult[CT_THREE].cbEachHandCardCount[Index] = 3;
			CardTypeResult[CT_THREE].cbCardTypeCount++;

			ASSERT(CardTypeResult[CT_THREE].cbCardTypeCount < MAX_TYPE_COUNT);
		}
	}
	//炸弹类型
	{
		uint8_t cbFourCardData[MAX_COUNT] = { 0 };
		uint8_t cbFourCardCount = 0;
		if (cbHandCardCount >= 2 && 0x4F == cbTmpCardData[0] && 0x4E == cbTmpCardData[1])
		{
			uint8_t Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
			CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbTmpCardData[0];
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbTmpCardData[1];
			CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 2;
			CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;
			GetAllBomCard(cbTmpCardData + 2, cbHandCardCount - 2, cbFourCardData, cbFourCardCount);
		}
		else GetAllBomCard(cbTmpCardData, cbHandCardCount, cbFourCardData, cbFourCardCount);
		for (uint8_t i = 0; i < cbFourCardCount; i += 4)
		{
			uint8_t Index = CardTypeResult[CT_BOMB_CARD].cbCardTypeCount;
			CardTypeResult[CT_BOMB_CARD].cbCardType = CT_BOMB_CARD;
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][0] = cbFourCardData[i];
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][1] = cbFourCardData[i + 1];
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][2] = cbFourCardData[i + 2];
			CardTypeResult[CT_BOMB_CARD].cbCardData[Index][3] = cbFourCardData[i + 3];
			CardTypeResult[CT_BOMB_CARD].cbEachHandCardCount[Index] = 4;
			CardTypeResult[CT_BOMB_CARD].cbCardTypeCount++;

			ASSERT(CardTypeResult[CT_BOMB_CARD].cbCardTypeCount < MAX_TYPE_COUNT);
		}
	}
	//单连类型
	{
		//恢复扑克，防止分析时改变扑克
		CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount);

		uint8_t cbFirstCard = 0;
		//去除2和王
		for (uint8_t i = 0; i < cbHandCardCount; ++i)
		{
			if (GetCardLogicValue(cbTmpCardData[i]) < 15)
			{
				cbFirstCard = i;
				break;
			}
		}
		uint8_t cbSingleLineCard[12];
		uint8_t cbSingleLineCount = 1;
		uint8_t cbLeftCardCount = cbHandCardCount;
		bool bFindSingleLine = true;
		//连牌判断
		while (cbLeftCardCount >= 5 && bFindSingleLine)
		{
			cbSingleLineCount = 1;
			bFindSingleLine = false;
			uint8_t cbLastCard = cbTmpCardData[cbFirstCard];
			cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCardData[cbFirstCard];
			for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount; i++)
			{
				uint8_t cbCardData = cbTmpCardData[i];

				//连续判断
				if (1 != (GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard) != GetCardValue(cbCardData))
				{
					cbLastCard = cbTmpCardData[i];
					//是否合法
					if (cbSingleLineCount < 5)
					{
						cbSingleLineCount = 1;
						cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCardData[i];
						continue;
					}
					else break;
				}
				//同牌判断
				else if (GetCardValue(cbLastCard) != GetCardValue(cbCardData))
				{
					cbLastCard = cbCardData;
					cbSingleLineCard[cbSingleLineCount] = cbCardData;
					++cbSingleLineCount;
				}
			}
			//保存数据
			if (cbSingleLineCount >= 5)
			{
				uint8_t Index;
				//所有连牌
				uint8_t cbStart = 0;
				//从大到小
				while (cbSingleLineCount - cbStart >= 5)
				{
					Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount;
					uint8_t cbThisLineCount = cbSingleLineCount - cbStart;
					CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE;
					CopyMemory(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard + cbStart, sizeof(uint8_t)*(cbThisLineCount));
					CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbThisLineCount;
					CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++;

					ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);
					cbStart++;
				}
				//从小到大
				cbStart = 1;
				while (cbSingleLineCount - cbStart >= 5)
				{
					Index = CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount;
					uint8_t cbThisLineCount = cbSingleLineCount - cbStart;
					CardTypeResult[CT_SINGLE_LINE].cbCardType = CT_SINGLE_LINE;
					CopyMemory(CardTypeResult[CT_SINGLE_LINE].cbCardData[Index], cbSingleLineCard, sizeof(uint8_t)*(cbThisLineCount));
					CardTypeResult[CT_SINGLE_LINE].cbEachHandCardCount[Index] = cbThisLineCount;
					CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount++;

					ASSERT(CardTypeResult[CT_SINGLE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);
					cbStart++;
				}
				RemoveCard(cbSingleLineCard, cbSingleLineCount, cbTmpCardData, cbLeftCardCount);
				cbLeftCardCount -= cbSingleLineCount;
				bFindSingleLine = true;
			}
		}
	}
	//对连类型
	{
		//恢复扑克，防止分析时改变扑克
		CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount);
		//连牌判断
		uint8_t cbFirstCard = 0;
		//去除2和王
		for (uint8_t i = 0; i < cbHandCardCount; ++i)	if (GetCardLogicValue(cbTmpCardData[i]) < 15) { cbFirstCard = i; break; }
		uint8_t cbLeftCardCount = cbHandCardCount - cbFirstCard;
		bool bFindDoubleLine = true;
		uint8_t cbDoubleLineCount = 0;
		uint8_t cbDoubleLineCard[24];
		//开始判断
		while (cbLeftCardCount >= 6 && bFindDoubleLine)
		{
			uint8_t cbLastCard = cbTmpCardData[cbFirstCard];
			uint8_t cbSameCount = 1;
			cbDoubleLineCount = 0;
			bFindDoubleLine = false;
			for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount + cbFirstCard; ++i)
			{
				//搜索同牌
				while (GetCardLogicValue(cbLastCard) == GetCardLogicValue(cbTmpCardData[i]) && i < cbLeftCardCount + cbFirstCard)
				{
					++cbSameCount;
					++i;
				}
				uint8_t cbLastDoubleCardValue;
				if (cbDoubleLineCount > 0) cbLastDoubleCardValue = GetCardLogicValue(cbDoubleLineCard[cbDoubleLineCount - 1]);
				//重新开始
				if ((cbSameCount < 2 || (cbDoubleLineCount > 0 && (cbLastDoubleCardValue - GetCardLogicValue(cbLastCard)) != 1)) && i <= cbLeftCardCount + cbFirstCard)
				{
					if (cbDoubleLineCount >= 6) break;
					//回退
					if (cbSameCount >= 2) i -= cbSameCount;
					cbLastCard = cbTmpCardData[i];
					cbDoubleLineCount = 0;
				}
				//保存数据
				else if (cbSameCount >= 2)
				{
					cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i - cbSameCount];
					cbDoubleLineCard[cbDoubleLineCount + 1] = cbTmpCardData[i - cbSameCount + 1];
					cbDoubleLineCount += 2;

					//结尾判断
					if (i == (cbLeftCardCount + cbFirstCard - 2))
						if ((GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbTmpCardData[i])) == 1 && (GetCardLogicValue(cbTmpCardData[i]) == GetCardLogicValue(cbTmpCardData[i + 1])))
						{
							cbDoubleLineCard[cbDoubleLineCount] = cbTmpCardData[i];
							cbDoubleLineCard[cbDoubleLineCount + 1] = cbTmpCardData[i + 1];
							cbDoubleLineCount += 2;
							break;
						}
				}
				cbLastCard = cbTmpCardData[i];
				cbSameCount = 1;
			}
			//保存数据
			if (cbDoubleLineCount >= 6)
			{
				uint8_t Index;
				Index = CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount;
				CardTypeResult[CT_DOUBLE_LINE].cbCardType = CT_DOUBLE_LINE;
				CopyMemory(CardTypeResult[CT_DOUBLE_LINE].cbCardData[Index], cbDoubleLineCard, sizeof(uint8_t)*cbDoubleLineCount);
				CardTypeResult[CT_DOUBLE_LINE].cbEachHandCardCount[Index] = cbDoubleLineCount;
				CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount++;
				ASSERT(CardTypeResult[CT_DOUBLE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);
				RemoveCard(cbDoubleLineCard, cbDoubleLineCount, cbTmpCardData, cbFirstCard + cbLeftCardCount);
				bFindDoubleLine = true;
				cbLeftCardCount -= cbDoubleLineCount;
			}
		}
	}
	//三连类型
	{
		//恢复扑克，防止分析时改变扑克
		CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount);
		//连牌判断
		uint8_t cbFirstCard = 0;
		//去除2和王
		for (uint8_t i = 0; i < cbHandCardCount; ++i)	if (GetCardLogicValue(cbTmpCardData[i]) < 15) { cbFirstCard = i; break; }
		uint8_t cbLeftCardCount = cbHandCardCount - cbFirstCard;
		bool bFindThreeLine = true;
		uint8_t cbThreeLineCount = 0;
		uint8_t cbThreeLineCard[20];
		//开始判断
		while (cbLeftCardCount >= 6 && bFindThreeLine)
		{
			uint8_t cbLastCard = cbTmpCardData[cbFirstCard];
			uint8_t cbSameCount = 1;
			cbThreeLineCount = 0;
			bFindThreeLine = false;
			for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount + cbFirstCard; ++i)
			{
				//搜索同牌
				while (GetCardLogicValue(cbLastCard) == GetCardLogicValue(cbTmpCardData[i]) && i < cbLeftCardCount + cbFirstCard)
				{
					++cbSameCount;
					++i;
				}
				uint8_t cbLastThreeCardValue;
				if (cbThreeLineCount > 0) cbLastThreeCardValue = GetCardLogicValue(cbThreeLineCard[cbThreeLineCount - 1]);
				//重新开始
				if ((cbSameCount < 3 || (cbThreeLineCount > 0 && (cbLastThreeCardValue - GetCardLogicValue(cbLastCard)) != 1)) && i <= cbLeftCardCount + cbFirstCard)
				{
					if (cbThreeLineCount >= 6) break;
					if (cbSameCount >= 3) i -= cbSameCount;
					cbLastCard = cbTmpCardData[i];
					cbThreeLineCount = 0;
				}
				//保存数据
				else if (cbSameCount >= 3)
				{
					cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i - cbSameCount];
					cbThreeLineCard[cbThreeLineCount + 1] = cbTmpCardData[i - cbSameCount + 1];
					cbThreeLineCard[cbThreeLineCount + 2] = cbTmpCardData[i - cbSameCount + 2];
					cbThreeLineCount += 3;
					//结尾判断
					if (i == (cbLeftCardCount + cbFirstCard - 3))
						if ((GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbTmpCardData[i])) == 1 && (GetCardLogicValue(cbTmpCardData[i]) == GetCardLogicValue(cbTmpCardData[i + 1])) && (GetCardLogicValue(cbTmpCardData[i]) == GetCardLogicValue(cbTmpCardData[i + 2])))
						{
							cbThreeLineCard[cbThreeLineCount] = cbTmpCardData[i];
							cbThreeLineCard[cbThreeLineCount + 1] = cbTmpCardData[i + 1];
							cbThreeLineCard[cbThreeLineCount + 2] = cbTmpCardData[i + 2];
							cbThreeLineCount += 3;
							break;
						}
				}

				cbLastCard = cbTmpCardData[i];
				cbSameCount = 1;
			}
			//保存数据
			if (cbThreeLineCount >= 6)
			{
				uint8_t Index;
				Index = CardTypeResult[CT_THREE_LINE].cbCardTypeCount;
				CardTypeResult[CT_THREE_LINE].cbCardType = CT_THREE_LINE;
				CopyMemory(CardTypeResult[CT_THREE_LINE].cbCardData[Index], cbThreeLineCard, sizeof(uint8_t)*cbThreeLineCount);
				CardTypeResult[CT_THREE_LINE].cbEachHandCardCount[Index] = cbThreeLineCount;
				CardTypeResult[CT_THREE_LINE].cbCardTypeCount++;
				ASSERT(CardTypeResult[CT_THREE_LINE].cbCardTypeCount < MAX_TYPE_COUNT);
				RemoveCard(cbThreeLineCard, cbThreeLineCount, cbTmpCardData, cbFirstCard + cbLeftCardCount);
				bFindThreeLine = true;
				cbLeftCardCount -= cbThreeLineCount;
			}
		}
	}
	//三带一单
	{
		//恢复扑克，防止分析时改变扑克
		CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount);
		uint8_t cbHandThreeCard[MAX_COUNT] = { 0 };
		uint8_t cbHandThreeCount = 0;
		//移除炸弹
		uint8_t cbAllBomCardData[MAX_COUNT] = { 0 };
		uint8_t cbAllBomCardCount = 0;
		GetAllBomCard(cbTmpCardData, cbHandCardCount, cbAllBomCardData, cbAllBomCardCount);
		RemoveCard(cbAllBomCardData, cbAllBomCardCount, cbTmpCardData, cbHandCardCount);
		GetAllThreeCard(cbTmpCardData, cbHandCardCount - cbAllBomCardCount, cbHandThreeCard, cbHandThreeCount);
		{
			uint8_t Index;
			//去掉三条
			uint8_t cbRemainCardData[MAX_COUNT] = { 0 };
			CopyMemory(cbRemainCardData, cbTmpCardData, cbHandCardCount - cbAllBomCardCount);
			uint8_t cbRemainCardCount = cbHandCardCount - cbAllBomCardCount - cbHandThreeCount;
			RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCardData, cbHandCardCount - cbAllBomCardCount);
			//三条带一张
			for (uint8_t i = 0; i < cbHandThreeCount; i += 3)
			{
				//三条带一张
				for (uint8_t j = 0; j < cbRemainCardCount; ++j)
				{
					Index = CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount;
					CardTypeResult[CT_THREE_TAKE_ONE].cbCardType = CT_THREE_TAKE_ONE;
					CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][0] = cbHandThreeCard[i];
					CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][1] = cbHandThreeCard[i + 1];
					CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][2] = cbHandThreeCard[i + 2];
					CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index][3] = cbRemainCardData[j];
					CardTypeResult[CT_THREE_TAKE_ONE].cbEachHandCardCount[Index] = 4;
					CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount++;
				}
			}
		}
		//三连带单
		uint8_t cbLeftThreeCardCount = cbHandThreeCount;
		bool bFindThreeLine = true;
		uint8_t cbLastIndex = 0;
		if (GetCardLogicValue(cbHandThreeCard[0]) == 15) cbLastIndex = 3;
		while (cbLeftThreeCardCount >= 6 && bFindThreeLine)
		{
			uint8_t cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[cbLastIndex]);
			uint8_t cbThreeLineCard[MAX_COUNT] = { 0 };
			uint8_t cbThreeLineCardCount = 3;
			cbThreeLineCard[0] = cbHandThreeCard[cbLastIndex];
			cbThreeLineCard[1] = cbHandThreeCard[cbLastIndex + 1];
			cbThreeLineCard[2] = cbHandThreeCard[cbLastIndex + 2];

			bFindThreeLine = false;
			for (uint8_t j = 3 + cbLastIndex; j < cbLeftThreeCardCount; j += 3)
			{
				//连续判断
				if (1 != (cbLastLogicCard - (GetCardLogicValue(cbHandThreeCard[j]))))
				{
					cbLastIndex = j;
					if (cbLeftThreeCardCount - j >= 6) bFindThreeLine = true;

					break;
				}

				cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[j]);
				cbThreeLineCard[cbThreeLineCardCount] = cbHandThreeCard[j];
				cbThreeLineCard[cbThreeLineCardCount + 1] = cbHandThreeCard[j + 1];
				cbThreeLineCard[cbThreeLineCardCount + 2] = cbHandThreeCard[j + 2];
				cbThreeLineCardCount += 3;
			}
			if (cbThreeLineCardCount > 3)
			{
				uint8_t Index;
				uint8_t cbRemainCard[MAX_COUNT] = { 0 };
				uint8_t cbRemainCardCount = cbHandCardCount - cbAllBomCardCount - cbHandThreeCount;
				//移除三条（还应该移除炸弹王等）
				CopyMemory(cbRemainCard, cbTmpCardData, (cbHandCardCount - cbAllBomCardCount) * sizeof(uint8_t));
				RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCard, cbHandCardCount - cbAllBomCardCount);
				for (uint8_t start = 0; start < cbThreeLineCardCount - 3; start += 3)
				{
					//本顺数目
					uint8_t cbThisTreeLineCardCount = cbThreeLineCardCount - start;
					//单牌个数
					uint8_t cbSingleCardCount = (cbThisTreeLineCardCount) / 3;
					//单牌不够
					if (cbRemainCardCount < cbSingleCardCount) continue;
					//单牌组合
					uint8_t cbComCard[5];
					uint8_t cbComResCard[254][5];
					uint8_t cbComResLen = 0;
					Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, cbSingleCardCount, cbRemainCardCount, cbSingleCardCount);
					for (uint8_t i = 0; i < cbComResLen; ++i)
					{
						Index = CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount;
						CardTypeResult[CT_THREE_TAKE_ONE].cbCardType = CT_THREE_TAKE_ONE;
						//保存三条
						CopyMemory(CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index], cbThreeLineCard + start, sizeof(uint8_t)*cbThisTreeLineCardCount);
						//保存单牌
						CopyMemory(CardTypeResult[CT_THREE_TAKE_ONE].cbCardData[Index] + cbThisTreeLineCardCount, cbComResCard[i], cbSingleCardCount);
						CardTypeResult[CT_THREE_TAKE_ONE].cbEachHandCardCount[Index] = cbThisTreeLineCardCount + cbSingleCardCount;
						CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount++;
						ASSERT(CardTypeResult[CT_THREE_TAKE_ONE].cbCardTypeCount < MAX_TYPE_COUNT);
					}
				}
				//移除三连
				bFindThreeLine = true;
				RemoveCard(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount);
				cbLeftThreeCardCount -= cbThreeLineCardCount;
			}
		}
	}
	//三带一对
	{
		//恢复扑克，防止分析时改变扑克
		CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount);
		uint8_t cbHandThreeCard[MAX_COUNT] = { 0 };
		uint8_t cbHandThreeCount = 0;
		uint8_t cbRemainCarData[MAX_COUNT] = { 0 };
		uint8_t cbRemainCardCount = 0;
		//抽取三条
		GetAllThreeCard(cbTmpCardData, cbHandCardCount, cbHandThreeCard, cbHandThreeCount);
		//移除三条（还应该移除炸弹王等）
		CopyMemory(cbRemainCarData, cbTmpCardData, cbHandCardCount);
		RemoveCard(cbHandThreeCard, cbHandThreeCount, cbRemainCarData, cbHandCardCount);
		cbRemainCardCount = cbHandCardCount - cbHandThreeCount;
		//抽取对牌
		uint8_t cbAllDoubleCardData[MAX_COUNT] = { 0 };
		uint8_t cbAllDoubleCardCount = 0;
		GetAllDoubleCard(cbRemainCarData, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount);
		//三条带一对
		for (uint8_t i = 0; i < cbHandThreeCount; i += 3)
		{
			uint8_t Index;
			//三条带一张
			for (uint8_t j = 0; j < cbAllDoubleCardCount; j += 2)
			{
				Index = CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount;
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardType = CT_THREE_TAKE_TWO;
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][0] = cbHandThreeCard[i];
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][1] = cbHandThreeCard[i + 1];
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][2] = cbHandThreeCard[i + 2];
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][3] = cbAllDoubleCardData[j];
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][4] = cbAllDoubleCardData[j + 1];
				CardTypeResult[CT_THREE_TAKE_TWO].cbEachHandCardCount[Index] = 5;
				CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount++;
			}
		}

		//三连带对
		uint8_t cbLeftThreeCardCount = cbHandThreeCount;
		bool bFindThreeLine = true;
		uint8_t cbLastIndex = 0;
		if (GetCardLogicValue(cbHandThreeCard[0]) == 15) cbLastIndex = 3;
		while (cbLeftThreeCardCount >= 6 && bFindThreeLine)
		{
			uint8_t cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[cbLastIndex]);
			uint8_t cbThreeLineCard[MAX_COUNT] = { 0 };
			uint8_t cbThreeLineCardCount = 3;
			cbThreeLineCard[0] = cbHandThreeCard[cbLastIndex];
			cbThreeLineCard[1] = cbHandThreeCard[cbLastIndex + 1];
			cbThreeLineCard[2] = cbHandThreeCard[cbLastIndex + 2];
			bFindThreeLine = false;
			for (uint8_t j = 3 + cbLastIndex; j < cbLeftThreeCardCount; j += 3)
			{
				//连续判断
				if (1 != (cbLastLogicCard - (GetCardLogicValue(cbHandThreeCard[j]))))
				{
					cbLastIndex = j;
					if (cbLeftThreeCardCount - j >= 6) bFindThreeLine = true;
					break;
				}
				cbLastLogicCard = GetCardLogicValue(cbHandThreeCard[j]);
				cbThreeLineCard[cbThreeLineCardCount] = cbHandThreeCard[j];
				cbThreeLineCard[cbThreeLineCardCount + 1] = cbHandThreeCard[j + 1];
				cbThreeLineCard[cbThreeLineCardCount + 2] = cbHandThreeCard[j + 2];
				cbThreeLineCardCount += 3;
			}
			if (cbThreeLineCardCount > 3)
			{
				uint8_t Index;
				for (uint8_t start = 0; start < cbThreeLineCardCount - 3; start += 3)
				{
					//本顺数目
					uint8_t cbThisTreeLineCardCount = cbThreeLineCardCount - start;
					//对牌张数
					uint8_t cbDoubleCardCount = ((cbThisTreeLineCardCount) / 3);
					//对牌不够
					if (cbRemainCardCount < cbDoubleCardCount) continue;
					uint8_t cbDoubleCardIndex[10]; //对牌下标
					for (uint8_t i = 0, j = 0; i < cbAllDoubleCardCount; i += 2, ++j)
						cbDoubleCardIndex[j] = i;
					//对牌组合
					uint8_t cbComCard[5];
					uint8_t cbComResCard[254][5];
					uint8_t cbComResLen = 0;
					//利用对牌的下标做组合，再根据下标提取出对牌
					Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, cbDoubleCardCount, cbAllDoubleCardCount / 2, cbDoubleCardCount);
					ASSERT(cbComResLen <= 254);
					for (uint8_t i = 0; i < cbComResLen; ++i)
					{
						Index = CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount;
						CardTypeResult[CT_THREE_TAKE_TWO].cbCardType = CT_THREE_TAKE_TWO;
						//保存三条
						CopyMemory(CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index], cbThreeLineCard + start, sizeof(uint8_t)*cbThisTreeLineCardCount);
						//保存对牌
						for (uint8_t j = 0, k = 0; j < cbDoubleCardCount; ++j, k += 2)
						{
							CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][cbThisTreeLineCardCount + k] = cbAllDoubleCardData[cbComResCard[i][j]];
							CardTypeResult[CT_THREE_TAKE_TWO].cbCardData[Index][cbThisTreeLineCardCount + k + 1] = cbAllDoubleCardData[cbComResCard[i][j] + 1];
						}
						CardTypeResult[CT_THREE_TAKE_TWO].cbEachHandCardCount[Index] = cbThisTreeLineCardCount + 2 * cbDoubleCardCount;
						CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount++;

						ASSERT(CardTypeResult[CT_THREE_TAKE_TWO].cbCardTypeCount < MAX_TYPE_COUNT);
					}
				}
				//移除三连
				bFindThreeLine = true;
				RemoveCard(cbThreeLineCard, cbThreeLineCardCount, cbHandThreeCard, cbLeftThreeCardCount);
				cbLeftThreeCardCount -= cbThreeLineCardCount;
			}
		}
	}
	//四带两单
	/*
	{
	//恢复扑克，防止分析时改变扑克
	CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	uint8_t cbFirstCard = 0 ;
	//去除王牌
	for(uint8_t i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

	uint8_t cbHandAllFourCardData[MAX_COUNT] ;
	uint8_t cbHandAllFourCardCount=0;
	//抽取四张
	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

	//移除四条
	uint8_t cbRemainCard[MAX_COUNT];
	uint8_t cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
	CopyMemory(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(uint8_t));
	RemoveCard(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

	for(uint8_t Start=0; Start<cbHandAllFourCardCount; Start += 4)
	{
	uint8_t Index ;
	//单牌组合
	uint8_t cbComCard[5];
	uint8_t cbComResCard[254][5] ;
	uint8_t cbComResLen=0 ;
	//单牌组合
	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbRemainCard, 2, cbRemainCardCount, 2);
	for(uint8_t i=0; i<cbComResLen; ++i)
	{
	//不能带对
	if(GetCardValue(cbComResCard[i][0])==GetCardValue(cbComResCard[i][1])) continue ;

	Index=CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount ;
	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardType = CT_FOUR_TAKE_ONE ;
	CopyMemory(CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;
	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index][4] = cbComResCard[i][0] ;
	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardData[Index][4+1] = cbComResCard[i][1] ;
	CardTypeResult[CT_FOUR_TAKE_ONE].cbEachHandCardCount[Index] = 6 ;
	CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount++ ;

	ASSERT(CardTypeResult[CT_FOUR_TAKE_ONE].cbCardTypeCount<MAX_TYPE_COUNT) ;
	}
	}
	}*/

	//四带两对
	/*
	{
	//恢复扑克，防止分析时改变扑克
	CopyMemory(cbTmpCardData, cbReserveCardData, cbHandCardCount) ;

	uint8_t cbFirstCard = 0 ;
	//去除王牌
	for(uint8_t i=0 ; i<cbHandCardCount ; ++i)	if(GetCardColor(cbTmpCardData[i])!=0x40)	{cbFirstCard = i ; break ;}

	uint8_t cbHandAllFourCardData[MAX_COUNT] ;
	uint8_t cbHandAllFourCardCount=0;

	//抽取四张
	GetAllBomCard(cbTmpCardData+cbFirstCard, cbHandCardCount-cbFirstCard, cbHandAllFourCardData, cbHandAllFourCardCount) ;

	//移除四条
	uint8_t cbRemainCard[MAX_COUNT];
	uint8_t cbRemainCardCount=cbHandCardCount-cbHandAllFourCardCount ;
	CopyMemory(cbRemainCard, cbTmpCardData, cbHandCardCount*sizeof(uint8_t));
	RemoveCard(cbHandAllFourCardData, cbHandAllFourCardCount, cbRemainCard, cbHandCardCount) ;

	for(uint8_t Start=0; Start<cbHandAllFourCardCount; Start += 4)
	{
	//抽取对牌
	uint8_t cbAllDoubleCardData[MAX_COUNT] ;
	uint8_t cbAllDoubleCardCount=0 ;
	GetAllDoubleCard(cbRemainCard, cbRemainCardCount, cbAllDoubleCardData, cbAllDoubleCardCount) ;

	uint8_t cbDoubleCardIndex[10]; //对牌下标
	for(uint8_t i=0, j=0; i<cbAllDoubleCardCount; i+=2, ++j)
	cbDoubleCardIndex[j]=i ;

	//对牌组合
	uint8_t cbComCard[5];
	uint8_t cbComResCard[255][5] ;
	uint8_t cbComResLen=0 ;

	//利用对牌的下标做组合，再根据下标提取出对牌
	Combination(cbComCard, 0, cbComResCard, cbComResLen, cbDoubleCardIndex, 2, cbAllDoubleCardCount/2, 2);
	for(uint8_t i=0; i<cbComResLen; ++i)
	{
	uint8_t Index = CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount ;
	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardType = CT_FOUR_TAKE_TWO ;
	CopyMemory(CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index], cbHandAllFourCardData+Start, 4) ;

	//保存对牌
	for(uint8_t j=0, k=0; j<4; ++j, k+=2)
	{
	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index][4+k] = cbAllDoubleCardData[cbComResCard[i][j]];
	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardData[Index][4+k+1] = cbAllDoubleCardData[cbComResCard[i][j]+1];
	}

	CardTypeResult[CT_FOUR_TAKE_TWO].cbEachHandCardCount[Index] = 8 ;
	CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount++ ;

	ASSERT(CardTypeResult[CT_FOUR_TAKE_TWO].cbCardTypeCount<MAX_TYPE_COUNT) ;
	}
	}
	}*/
}


//分析炸弹
VOID CGameLogic::GetAllBomCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbBomCardData[], uint8_t &cbBomCardCount)
{
	uint8_t cbTmpCardData[MAX_COUNT] = { 0 };
	CopyMemory(cbTmpCardData, cbHandCardData, cbHandCardCount);
	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);
	cbBomCardCount = 0;
	if (cbHandCardCount < 2) return;
	//双王炸弹
	if (0x4F == cbTmpCardData[0] && 0x4E == cbTmpCardData[1])
	{
		cbBomCardData[cbBomCardCount++] = cbTmpCardData[0];
		cbBomCardData[cbBomCardCount++] = cbTmpCardData[1];
	}
	//扑克分析
	for (uint8_t i = 0;i < cbHandCardCount;i++)
	{
		//变量定义
		uint8_t cbSameCount = 1;
		uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);
		//搜索同牌
		for (uint8_t j = i + 1;j < cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;
			//设置变量
			cbSameCount++;
		}
		if (4 == cbSameCount)
		{
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i];
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i + 1];
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i + 2];
			cbBomCardData[cbBomCardCount++] = cbTmpCardData[i + 3];
		}
		//设置索引
		i += cbSameCount - 1;
	}
}

//分析顺子
VOID CGameLogic::GetAllLineCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbLineCardData[], uint8_t &cbLineCardCount)
{
	uint8_t cbTmpCard[MAX_COUNT] = { 0 };
	CopyMemory(cbTmpCard, cbHandCardData, cbHandCardCount);
	//大小排序
	SortCardList(cbTmpCard, cbHandCardCount, ST_ORDER);

	cbLineCardCount = 0;

	//数据校验
	if (cbHandCardCount < 5) return;

	uint8_t cbFirstCard = 0;
	//去除2和王
	for (uint8_t i = 0; i < cbHandCardCount; ++i)
	{
		if (GetCardLogicValue(cbTmpCard[i]) < 15)
		{
			cbFirstCard = i;
			break;
		}
	}

	uint8_t cbSingleLineCard[12];
	uint8_t cbSingleLineCount = 0;
	uint8_t cbLeftCardCount = cbHandCardCount;
	bool bFindSingleLine = true;

	//连牌判断
	while (cbLeftCardCount >= 5 && bFindSingleLine)
	{
		cbSingleLineCount = 1;
		bFindSingleLine = false;
		uint8_t cbLastCard = cbTmpCard[cbFirstCard];
		cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCard[cbFirstCard];
		for (uint8_t i = cbFirstCard + 1; i < cbLeftCardCount; i++)
		{
			uint8_t cbCardData = cbTmpCard[i];

			//连续判断
			if (1 != (GetCardLogicValue(cbLastCard) - GetCardLogicValue(cbCardData)) && GetCardValue(cbLastCard) != GetCardValue(cbCardData))
			{
				cbLastCard = cbTmpCard[i];
				if (cbSingleLineCount < 5)
				{
					cbSingleLineCount = 1;
					cbSingleLineCard[cbSingleLineCount - 1] = cbTmpCard[i];
					continue;
				}
				else break;
			}
			//同牌判断
			else if (GetCardValue(cbLastCard) != GetCardValue(cbCardData))
			{
				cbLastCard = cbCardData;
				cbSingleLineCard[cbSingleLineCount] = cbCardData;
				++cbSingleLineCount;
			}
		}

		//保存数据
		if (cbSingleLineCount >= 5)
		{
			RemoveCard(cbSingleLineCard, cbSingleLineCount, cbTmpCard, cbLeftCardCount);
			memcpy(cbLineCardData + cbLineCardCount, cbSingleLineCard, sizeof(uint8_t)*cbSingleLineCount);
			cbLineCardCount += cbSingleLineCount;
			cbLeftCardCount -= cbSingleLineCount;
			bFindSingleLine = true;
		}
	}
}

//分析三条
VOID CGameLogic::GetAllThreeCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbThreeCardData[], uint8_t &cbThreeCardCount)
{
	uint8_t cbTmpCardData[MAX_COUNT] = { 0 };
	CopyMemory(cbTmpCardData, cbHandCardData, cbHandCardCount);

	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbThreeCardCount = 0;

	//扑克分析
	for (uint8_t i = 0;i < cbHandCardCount;i++)
	{
		//变量定义
		uint8_t cbSameCount = 1;
		uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);

		//搜索同牌
		for (uint8_t j = i + 1;j < cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		if (cbSameCount >= 3)
		{
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i];
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i + 1];
			cbThreeCardData[cbThreeCardCount++] = cbTmpCardData[i + 2];
		}

		//设置索引
		i += cbSameCount - 1;
	}
}

//分析对子
VOID CGameLogic::GetAllDoubleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbDoubleCardData[], uint8_t &cbDoubleCardCount)
{
	uint8_t cbTmpCardData[MAX_COUNT] = { 0 };
	CopyMemory(cbTmpCardData, cbHandCardData, cbHandCardCount);

	//大小排序
	SortCardList(cbTmpCardData, cbHandCardCount, ST_ORDER);

	cbDoubleCardCount = 0;

	//扑克分析
	for (uint8_t i = 0;i < cbHandCardCount;i++)
	{
		//变量定义
		uint8_t cbSameCount = 1;
		uint8_t cbLogicValue = GetCardLogicValue(cbTmpCardData[i]);

		//搜索同牌
		for (uint8_t j = i + 1;j < cbHandCardCount;j++)
		{
			//获取扑克
			if (GetCardLogicValue(cbTmpCardData[j]) != cbLogicValue) break;

			//设置变量
			cbSameCount++;
		}

		if (cbSameCount >= 2)
		{
			cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i];
			cbDoubleCardData[cbDoubleCardCount++] = cbTmpCardData[i + 1];
		}

		//设置索引
		i += cbSameCount - 1;
	}
}


//获取牌型对应的权值
int32_t CGameLogic::get_GroupData(uint8_t cgType, int MaxCard, int Count)
{
	int32_t nValue = 0;

	//单牌类型
	if (cgType == CT_SINGLE)
		nValue = MaxCard - 10;
	//对牌类型
	else if (cgType == CT_DOUBLE_CARD)
		nValue = MaxCard - 10;
	//三条类型
	else if (cgType == CT_THREE)
		nValue = MaxCard - 10;
	//单连类型
	else if (cgType == CT_SINGLE_LINE)
		nValue = MaxCard - 10 + 1;
	//对连类型
	else if (cgType == CT_DOUBLE_LINE)
		nValue = MaxCard - 10 + 1;
	//三连类型
	else if (cgType == CT_THREE_LINE)
		nValue = (MaxCard - 3 + 1) / 2;
	//三带一单
	else if (cgType == CT_THREE_TAKE_ONE)
	{
		if (Count > 4)//三带一单连
		{
			nValue = (MaxCard - 3 + 1) / 2;
		}
		else
		{
			nValue = MaxCard - 10;
		}
	}
	//三带一对
	else if (cgType == CT_THREE_TAKE_TWO)
	{
		if (Count > 5)//三带一单连
		{
			nValue = (MaxCard - 3 + 1) / 2;
		}
		else
		{
			nValue = MaxCard - 10;
		}
	}
	//四带两单
	else if (cgType == CT_FOUR_TAKE_ONE)
		nValue = (MaxCard - 3) / 2;
	//四带两对
	else if (cgType == CT_FOUR_TAKE_TWO)
		nValue = (MaxCard - 3) / 2;
	//炸弹类型 
	else if (cgType == CT_BOMB_CARD)
		nValue = MaxCard - 3 + 7;
	//王炸类型 
	else if (cgType == CT_MISSILE_CARD)
		nValue = 20;
	//错误牌型
	else
		nValue = 0;

	return nValue;
}

//获取手牌的权值
int32_t CGameLogic::get_HandCardValue(const uint8_t cbHandCardData[], uint8_t cbHandCardCount)
{
	tagOutCardTypeResult CardTypeResult[13];
	ZeroMemory(CardTypeResult, sizeof(CardTypeResult));
	AnalyseOutCardType(cbHandCardData, cbHandCardCount, CardTypeResult);

	int32_t cbHandCardValue = 0;
	for (uint8_t cbCardType = CT_SINGLE; cbCardType <= CT_MISSILE_CARD; ++cbCardType)
	{
		tagOutCardTypeResult const &tmpCardResult = CardTypeResult[cbCardType];
		if (tmpCardResult.cbCardTypeCount>0)
		{
			cbHandCardValue += get_GroupData(cbCardType, GetCardLogicValue(tmpCardResult.cbCardData[0][0]), tmpCardResult.cbEachHandCardCount[0]);
		}		
	}

	return cbHandCardValue;
}

//////////////////////////////////////////////////////////////////////////////////
