#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include "proto/QZXszSg.Message.pb.h"
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
#define SG_THREE_POKER              10                                  //三公
#define SG_BOMB						11									//炸弹
#define SG_BOMB_NINE                12                                  //爆玖

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
	//检验是否服务器的牌值
	bool checkSameCard(uint8_t *servercards, uint8_t *clientcards);
    //是否同样的牌值
    bool IsSameCard(uint8_t cbFirstCard[], uint8_t cbNextCard[]);
    //获取倍数
    uint8_t GetMultiple(uint8_t cbCardData[]);

public:
    //是否为爆玖
    bool IsBombNine(uint8_t cbCardData[MAX_COUNT]);
    //是否为炸弹
    bool IsBomb(uint8_t cbCardData[MAX_COUNT]);
    //是否为三公
    bool IsThreePoker(uint8_t cbCardData[MAX_COUNT]);
public:
    bool IsUsedCard(uint8_t cbCardData);
    void InsertUsedCard(uint8_t cbCardData);
    void ClearUsedCard();
    //根据花色和数值获取对应的牌值
    uint8_t GetCardData(uint8_t cbColor, uint8_t cbValue);
    //根据点数获取牌的数据
    bool GetMatchPointCard(int32_t iPoint,uint8_t cbCardData[MAX_SENDCOUNT], uint8_t byCardDataArray[MAX_CARD_TOTAL]);
    bool GetResolvePointCard(string& sValue,uint8_t cbCardData[MAX_SENDCOUNT], uint8_t byCardDataArray[MAX_CARD_TOTAL]);
    //添加一张牌得到相同牌型
    uint8_t GetSameTypeCard(uint8_t cbCardData[MAX_SENDCOUNT], uint8_t byCardDataArray[MAX_CARD_TOTAL]);

    //获取玩家最优牌型组合
    uint8_t GetBigTypeCard(uint8_t cbHandCardData[MAX_SENDCOUNT], uint8_t cbCardData[MAX_COUNT]);
    bool IsSameTypeCard(uint8_t cbCardData[MAX_SENDCOUNT],uint8_t cbData,int cardType);

private:
    std::vector<uint8_t> m_vecCardVal;
    random_device m_rd;
    std::mt19937 m_gen;
};

//////////////////////////////////////////////////////////////////////////

#endif		//GAME_LOGIC_HEAD_FILE


