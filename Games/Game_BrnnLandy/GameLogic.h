#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <stdint.h>
#include "proto/Brnn.Message.pb.h"

//////////////////////////////////////////////////////////////////////////
//宏定义

#define MAX_COUNT					5									//最大数目
#define	DRAW						2									//和局类型

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码

//CardType Multiple
#define BOMB_MUL            3   //bomb

#define MAX_MUL             10   //bomb

#define SPECIAL_MUL         3   //other
#define NN_MUL              2   //n8-nn
#define NORMAL_MUL          1   //n0-n7

//////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CGameLogic
{
    //变量定义
private:
    static uint8_t						m_cbCardListData[52];				//扑克定义


    bool   isOpenTenMu;
    //函数定义
public:
    //构造函数
    CGameLogic();
    //析构函数
    virtual ~CGameLogic();

    //类型函数
public:

    void SetTenMuti(bool val){isOpenTenMu = val;}
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return (cbCardData&LOGIC_MASK_COLOR)>>4; }
    //判断是否是五小牛
    bool JudgeFiveSmallNiu(uint8_t arr[], uint8_t cbCardCount);
    //判断是否为炸弹
    bool JudgeBomb(uint8_t arr[], uint8_t cbCardCount);
    //判断是否为五花牛
    bool JudgeGoldenBull(uint8_t arr[], uint8_t cbCardCount);
    //判断是否为四花牛
    bool JudgeSilveryBull(uint8_t arr[], uint8_t cbCardCount);    
    //控制函数
public:
    bool IsUsedCard(uint8_t cbCardData);
    void InsertUsedCard(uint8_t cbCardData);
    void ClearUsedCard();
    //根据花色和数值获取对应的牌值
    uint8_t GetCardData(uint8_t cbColor, uint8_t cbValue);
    //根据点数获取牌的数据
    bool GetMatchPointCard(int32_t iPoint,uint8_t cbCardData[MAX_COUNT], uint8_t byCardDataArray[52]);
public:
    //排列扑克
    void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount);
    //混乱扑克
    void RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount);

    //功能函数
public:
    //逻辑数值
    uint8_t GetCardLogicValue(uint8_t cbCardData);

    //Add by guansheng
    uint8_t GetLogicValue(uint8_t cbCardData);
    //配牌
//    bool MatchCard();
    int32_t CalculateMultiples(uint8_t arr[],uint8_t brr[], uint8_t cbCardCount);

    int32_t CalculateMultiples10(uint8_t arr[],uint8_t brr[], uint8_t cbCardCount);
    int32_t CalculateMultiples3(uint8_t arr[],uint8_t brr[], uint8_t cbCardCount);

    int CalculatePoints(uint8_t arr[], uint8_t cbCardCount);

    int32_t SortNiuNiuCard(uint8_t bCardData[], uint8_t bCardCount);

    bool JudgeFiveHaveNiu(uint8_t arr[],uint8_t cbCardCount);
private:
    std::vector<uint8_t>    m_vecCardVal;
};

//////////////////////////////////////////////////////////////////////////

#endif
