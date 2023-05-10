#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include "BBQznn.Message.pb.h"
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
#define MAX_CARD_TOTAL_KING         54                                  //牌总个数包括大小王

//扑克类型
#define OX_VALUE0					 0									//混合牌型 meiniu 1-10 niu1-niuniu
#define OX_SILVERY_BULL				11									//银牛 sihua
#define OX_GOLDEN_BULL				12									//五花牛（金牛）
#define OX_BOMB						13									//炸弹（四梅）
#define OX_FIVE_SMALL				14									//五小牛
// 0:无牛;1:牛一;2:牛二;3:牛三;4:牛四;5:牛五;6:牛六;7:牛七;8:牛八;9:牛九;10:牛牛;11:顺子牛;12:五花牛;13:同花牛;14:葫芦牛;15:炸弹牛;16:五小牛;17:同花顺牛
enum emOXType
{
    CT_NIU_NULL         =         0,                                  //无牛
    CT_NIU_ONE          =         1,                                  //牛一
    CT_NIU_TWO          =         2,                                  //牛二
    CT_NIU_THREE        =         3,                                  //牛三
    CT_NIU_FOUR         =         4,                                  //牛四
    CT_NIU_FIVE         =         5,                                  //牛五
    CT_NIU_SIX          =         6,                                  //牛六
    CT_NIU_SEVEN        =         7,                                  //牛七
    CT_NIU_EIGHT        =         8,                                  //牛八
    CT_NIU_NINE         =         9,                                  //牛九
    CT_NIUNIU           =        10,                                  //普通牛 
    CT_SHUNZI_NIU       =        11,                                  //顺子牛 
    CT_WUHUA_NIU        =        12,                                  //五花牛
    CT_TONGHUA_NIU      =        13,                                  //同花牛
    CT_HULU_NIU         =        14,                                  //葫芦牛
    CT_BOMB_NIU         =        15,                                  //炸弹牛
    CT_FIVESMALL_NIU    =        16,                                  //五小牛
    CT_TONGHUASHUN_NIU  =        17,                                  //同花顺牛

    CT_MAXCOUNT         =        18,                                  //牌型的总类
};


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
    static const uint8_t        m_byHaveKingCardDataArray[MAX_CARD_TOTAL_KING];  //扑克数据包括大小王

    static int								m_iBigDivisorSeed[10];				//种子除数
    static int								m_iSmallDivsorSeed[10];				//种子除数
    static int								m_iMultiSeed[10];					//种子除数

    bool                                    m_bHaveKing;                        //是否有大小王,百变牛玩法
    bool                                    m_bPlaySixChooseFive;               //是否6选5玩法

public:
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return (cbCardData&MASK_COLOR)>>4; }
    //逻辑数值
    uint8_t GetLogicValue(uint8_t cbCardData);
    //是否用百变牛
    void SetIsHaveKing(bool bHaveking) { m_bHaveKing = bHaveking;}
    //是否用百变牛
    void SetSixChooseFive(bool bSixChooseFive) { m_bPlaySixChooseFive = bSixChooseFive;}

public:
    //获得当前时间
    unsigned int GetSeed();
    //产生随机种子
    void GenerateSeed(int TableId);
    //混乱扑克
    void RandCardData(uint8_t byCardBuffer[], uint8_t byBufferCount);
    //排列扑克
    void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t bOrder=false);
    void SortCardListSix(uint8_t cbCardData[], uint8_t cbCardCount);
    //炸弹牛4+1、葫芦牛3+2牌型序
    bool SortCardListBH(uint8_t cbCardData[],int cbCardType);
    //获取类型
    uint8_t GetCardType(uint8_t cbCardData[], uint8_t cbCardCount,bool bKingChangCard = false);
    //对比扑克
    bool CompareCard(uint8_t cbFirstCard[], uint8_t cbNextCard[],uint8_t cbFirstKingChangCard[2]={0},uint8_t cbNextKingChangCard[2]={0},bool bSelfSixChooseFive = true, bool bFirstBanker = true);
    //获取倍数
    uint8_t GetMultiple(uint8_t cbCardData[],uint8_t playGameType);
    //获取牛牛牌
    bool GetOxCard(uint8_t cbCardData[],uint8_t cbKingChangCard[2]={0});

public:
    //是否为五小
    bool IsFiveSmall(uint8_t cbCardData[MAX_COUNT]);
    //是否为炸弹
    bool IsBomb(uint8_t cbCardData[MAX_COUNT]);
    //是否为金牛(五花牛)
    bool IsGoldenBull(uint8_t cbCardData[MAX_COUNT]);
    //是否为银牛
    bool IsSilveryBull(uint8_t cbCardData[MAX_COUNT]);

    //判断是否为同花顺
    bool IsTierce(uint8_t cbCardData[MAX_COUNT]);
    //判断是否为葫芦
    bool IsCalabash(uint8_t cbCardData[MAX_COUNT]);
    //判断是否为同花
    bool IsFlush(uint8_t cbCardData[MAX_COUNT]);
    //判断是否为顺子
    bool IsStraight(uint8_t cbCardData[MAX_COUNT]);
    //判断其他牛(无牛、牛1-10)
    uint8_t IsOtherNiu(uint8_t cbCardData[MAX_COUNT]);
    //获取玩家最优牌型组合
    uint8_t GetBigTypeCard(uint8_t cbHandCardData[], uint8_t cbCardData[MAX_COUNT]);
    //带百变王时手牌的牌型判断
    uint8_t GetCardTypeHaveKing(uint8_t cbHandCardData[], uint8_t cbCardData[MAX_COUNT],uint8_t cbHandCardcount, uint8_t haveKingCount, uint8_t kingChangCard[]);
    //带百变王时六张手牌的牌型判断
    uint8_t GetHaveKingBigCardType(uint8_t cbHandCardData[], uint8_t cbCardData[MAX_COUNT],uint8_t cbHandCardcount,uint8_t kingChangCard[]);
    //带1张百变王时的牌型判断
    uint8_t GetCardTypeOneKing(uint8_t cbHandCardData[], uint8_t cbCardData[], uint8_t cbHandCardcount,uint8_t kingChangCard[]);
    //带2张百变王时的牌型判断
    uint8_t GetCardTypeTwoKing(uint8_t cbHandCardData[], uint8_t cbCardData[], uint8_t cbHandCardcount,uint8_t kingChangCard[]);
	//检验手牌牌值是否有效
	bool CheckHanCardIsOK(uint8_t cbHandCardData[], uint8_t cardcount);

private:
    random_device m_rd;
    std::mt19937 m_gen;
};

//////////////////////////////////////////////////////////////////////////

#endif		//GAME_LOGIC_HEAD_FILE


