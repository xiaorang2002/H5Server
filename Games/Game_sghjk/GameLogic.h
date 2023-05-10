#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <memory.h>
#include <vector>
#include "hand.h"
//////////////////////////////////////////////////////////////////////////////////
//宏定义
//#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))

#ifndef isZero
#define isZero(a)        ((a>-0.000001) && (a<0.000001))
#endif//isZero


#define MAX_COUNT					5									//最大数目

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码

//扑克类型
#define CT_BUST					1									//爆牌
#define CT_POINTS				2									//点数
#define	CT_FIVE_DRAGON			3									//五小龙
#define CT_BLACKJACK			4									//黑杰克

//operate code
#define OPER_STOP           1
#define OPER_CALL           2
#define OPER_DOUBLE         4
#define OPER_SPLIT          8

//Card type multiple
#define MUL_BJ           1.5
#define MUL_FIVE_DRAGON  1.5

#define MAX_CARD_TOTAL				52									//牌总个数
#define USE_COUNT                   8               //用的几副牌

using namespace std;

//////////////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CGameLogic
{
    //变量定义
private:
    static uint8_t						m_cbCardListData[MAX_CARD_TOTAL];	//扑克定义

                                                                        //函数定义
public:
    //构造函数
    CGameLogic();
    //析构函数
    virtual ~CGameLogic();

    //类型函数
public:
    //获取类型
//    uint8_t GetCardType(uint8_t cbCardData[], uint8_t cbCardCount);
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData & LOGIC_MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return cbCardData & LOGIC_MASK_COLOR; }
    //控制函数
public:
    //排列扑克
    void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount);
    //混乱扑克
    void RandCardList(vector<uint8_t> &cbCardData);

    void UpdateInit(shared_ptr<Hand> &hand,uint8_t carddata[]);

    bool UpdateAddOne(shared_ptr<Hand>  &hand, uint8_t carddata);
    //功能函数
public:
    //逻辑数值
    uint8_t GetCardLogicValue(uint8_t cbCardData);
    //庄家和闲家对比，返回庄家输赢的倍数:正数 庄赢，负数 庄输，0 平局
    float CompareCard(shared_ptr<Hand> &banker, shared_ptr<Hand> &player);
    //牌型倍数
    float GetCardTypeMultiple(uint8_t cardtype)
    {
        switch(cardtype){
            case CT_BLACKJACK:
            {
                return MUL_BJ;
            }
            case CT_FIVE_DRAGON:
            {
                return MUL_FIVE_DRAGON;
            }
            default:
            {
                return 1;
            }
        }
    }
protected:
//    bool RandomCardData(std::vector<uint8_t>& cbCardData);
    //
    //bool recude27Cards(vector<uint8_t>& cbCardData, int reduceRatio);
};

//////////////////////////////////////////////////////////////////////////
#endif


