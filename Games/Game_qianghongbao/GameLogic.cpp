#include "GameLogic.h"

#include <random>
#include <assert.h>
#include <cstring>

//////////////////////////////////////////////////////////////////////////

//扑克数据
uint8_t CGameLogic::m_cbCardListData[52*8]=
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K

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

//获取类型
uint8_t CGameLogic::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount)
{
    assert(cbCardCount==MAX_COUNT);

	if (cbCardCount==MAX_COUNT)
	{
		//变量定义
		bool cbSameColor=true,bLineCard=true;
        uint8_t cbFirstColor=GetCardColor(cbCardData[0]);
        uint8_t cbFirstValue=GetCardLogicValue(cbCardData[0]);

		//牌形分析
        for (uint8_t i=1;i<cbCardCount;i++)
		{
			//数据分析
			if (GetCardColor(cbCardData[i])!=cbFirstColor) cbSameColor=false;
			if (cbFirstValue!=(GetCardLogicValue(cbCardData[i])+i)) bLineCard=false;

			//结束判断
			if ((cbSameColor==false)&&(bLineCard==false)) break;
		}

		//特殊A32
		if(!bLineCard)
		{
			bool bOne=false,bTwo=false,bThree=false;
            for(uint8_t i=0;i<MAX_COUNT;i++)
			{
				if(GetCardValue(cbCardData[i])==1)		bOne=true;
				else if(GetCardValue(cbCardData[i])==2)	bTwo=true;
				else if(GetCardValue(cbCardData[i])==3)	bThree=true;
			}
			if(bOne && bTwo && bThree)bLineCard=true;
		}

		//顺金类型
		if ((cbSameColor)&&(bLineCard)) return CT_SHUN_JIN;

		//顺子类型
		if ((!cbSameColor)&&(bLineCard)) return CT_SHUN_ZI;

		//金花类型
		if((cbSameColor)&&(!bLineCard)) return CT_JIN_HUA;

		//牌形分析
		bool bDouble=false,bPanther=true;

		//对牌分析
        for (uint8_t i=0;i<cbCardCount-1;i++)
		{
            for (uint8_t j=i+1;j<cbCardCount;j++)
			{
				if (GetCardLogicValue(cbCardData[i])==GetCardLogicValue(cbCardData[j])) 
				{
					bDouble=true;
					break;
				}
			}
			if(bDouble)break;
		}

		//三条(豹子)分析
        for (uint8_t i=1;i<cbCardCount;i++)
		{
			if (bPanther && cbFirstValue!=GetCardLogicValue(cbCardData[i])) bPanther=false;
		}

		//对子和豹子判断
		if (bDouble==true) return (bPanther)?CT_BAO_ZI:CT_DOUBLE;

		//特殊235
		bool bTwo=false,bThree=false,bFive=false;
        for (uint8_t i=0;i<cbCardCount;i++)
		{
			if(GetCardValue(cbCardData[i])==2)	bTwo=true;
			else if(GetCardValue(cbCardData[i])==3)bThree=true;
			else if(GetCardValue(cbCardData[i])==5)bFive=true;			
		}	
		if (bTwo && bThree && bFive) return CT_SPECIAL;
	}

	return CT_SINGLE;
}

//排列扑克
void CGameLogic::SortCardList(uint8_t cbCardData[], uint8_t cbCardCount)
{
	//转换数值
    uint8_t cbLogicValue[MAX_COUNT];
    for (uint8_t i=0;i<cbCardCount;i++)
        cbLogicValue[i]=GetCardLogicValue(cbCardData[i]);

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

	//特殊情况分析
    if((cbNextType+cbFirstType)==(CT_SPECIAL+CT_BAO_ZI))return (uint8_t)(cbFirstType>cbNextType);

	//还原单牌类型
    if(cbFirstType==CT_SPECIAL)cbFirstType=CT_SINGLE;
    if(cbNextType==CT_SPECIAL)cbNextType=CT_SINGLE;

	//类型判断
	if (cbFirstType!=cbNextType) return (cbFirstType>cbNextType)?1:0;

	//简单类型
	switch(cbFirstType)
	{
	case CT_BAO_ZI:			//豹子
	case CT_SINGLE:			//单牌
	case CT_JIN_HUA:		//金花
		{
			//对比数值
            for (uint8_t i=0;i<cbCardCount;i++)
			{
                uint8_t cbNextValue=GetCardLogicValue(NextData[i]);
                uint8_t cbFirstValue=GetCardLogicValue(FirstData[i]);
				if (cbFirstValue!=cbNextValue) return (cbFirstValue>cbNextValue)?1:0;
			}
			return DRAW;
		}
	case CT_SHUN_ZI:		//顺子
	case CT_SHUN_JIN:		//顺金 432>A32
		{		
            uint8_t cbNextValue=GetCardLogicValue(NextData[0]);
            uint8_t cbFirstValue=GetCardLogicValue(FirstData[0]);

			//特殊A32
			if(cbNextValue==14 && GetCardLogicValue(NextData[cbCardCount-1])==2)
			{
				cbNextValue=3;
			}
			if(cbFirstValue==14 && GetCardLogicValue(FirstData[cbCardCount-1])==2)
			{
				cbFirstValue=3;
			}

			//对比数值
			if (cbFirstValue!=cbNextValue) return (cbFirstValue>cbNextValue)?1:0;;
			return DRAW;
		}
	case CT_DOUBLE:			//对子
		{
            uint8_t cbNextValue=GetCardLogicValue(NextData[0]);
            uint8_t cbFirstValue=GetCardLogicValue(FirstData[0]);

			//查找对子/单牌
            uint8_t bNextDouble=0,bNextSingle=0;
            uint8_t bFirstDouble=0,bFirstSingle=0;
			if(cbNextValue==GetCardLogicValue(NextData[1]))
			{
				bNextDouble=cbNextValue;
				bNextSingle=GetCardLogicValue(NextData[cbCardCount-1]);
			}
			else
			{
				bNextDouble=GetCardLogicValue(NextData[cbCardCount-1]);
				bNextSingle=cbNextValue;
			}
			if(cbFirstValue==GetCardLogicValue(FirstData[1]))
			{
				bFirstDouble=cbFirstValue;
				bFirstSingle=GetCardLogicValue(FirstData[cbCardCount-1]);
			}
			else 
			{
				bFirstDouble=GetCardLogicValue(FirstData[cbCardCount-1]);
				bFirstSingle=cbFirstValue;
			}

			if (bNextDouble!=bFirstDouble)return (bFirstDouble>bNextDouble)?1:0;
			if (bNextSingle!=bFirstSingle)return (bFirstSingle>bNextSingle)?1:0;
			return DRAW;
		}
	}

	return DRAW;
}

//////////////////////////////////////////////////////////////////////////
