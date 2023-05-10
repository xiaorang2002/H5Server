#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include "Tbnn.Message.pb.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <vector>
#include <stdint.h>

#include "CMD_Game.h"

//宏定义
//////////////////////////////////////////////////////////////////////////
//逻辑掩码
#define MASK_COLOR					0xF0								//花色掩码
#define MASK_VALUE					0x0F								//数值掩码
#define MAX_CARD_TOTAL				52									//牌总个数

//扑克类型
#define OX_VALUE0					0									//混合牌型 meiniu 1-10 niu1-niuniu
#define OX_SILVERY_BULL				11									//银牛 sihua
#define OX_GOLDEN_BULL				12									//五花牛（金牛）
#define OX_BOMB						13									//炸弹（四梅）
#define OX_FIVE_SMALL				14									//五小牛

//数组维数
#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))

//游戏逻辑类
class CGameLogic
{
    //函数定义
public:
    //构造函数
    CGameLogic();
    //析构函数
    virtual ~CGameLogic();

    //变量定义
protected:
    static const uint8_t		m_byCardDataArray[MAX_CARD_TOTAL];	//扑克数据

    static int								m_iBigDivisorSeed[10];				//种子除数
    static int								m_iSmallDivsorSeed[10];				//种子除数
    static int								m_iMultiSeed[10];					//种子除数

public:
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return (cbCardData&MASK_COLOR)>>4; }
    //逻辑数值
    uint8_t GetLogicValue(uint8_t cbCardData);

public:
    //获得当前时间
    unsigned int GetSeed();
    //产生随机种子
    void GenerateSeed(int TableId);
    //混乱扑克
    void RandCardData(uint8_t byCardBuffer[], uint8_t byBufferCount);
    //排列扑克
    void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount);
    //获取类型
    uint8_t GetCardType(uint8_t cbCardData[], uint8_t cbCardCount);
    //对比扑克
    bool CompareCard(uint8_t cbFirstCard[], uint8_t cbNextCard[]);
    //获取倍数
    uint8_t GetMultiple(uint8_t cbCardData[]);
    //获取牛牛牌
    bool GetOxCard(uint8_t cbCardData[]);

public:
    //是否为五小
    bool IsFiveSmall(uint8_t cbCardData[MAX_COUNT]);
    //是否为炸弹
    bool IsBomb(uint8_t cbCardData[MAX_COUNT]);
    //是否为金牛(五花牛)
    bool IsGoldenBull(uint8_t cbCardData[MAX_COUNT]);
    //是否为银牛
    bool IsSilveryBull(uint8_t cbCardData[MAX_COUNT]);
private:
    random_device m_rd;
    std::mt19937 m_gen;
};

//////////////////////////////////////////////////////////////////////////

#endif		//GAME_LOGIC_HEAD_FILE


