#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#pragma once

//#include "Stdafx.h"
#include "./types.h"
#include "../Game_ddzLandy/CMD_Game.h"
//////////////////////////////////////////////////////////////////////////

//排序类型
#define ST_ORDER					1									//大小排序
#define ST_COUNT					2									//数目排序
#define ST_CUSTOM					3									//自定排序

//////////////////////////////////////////////////////////////////////////

//分析结构
struct tagAnalyseResult
{
    uint8_t 							cbBlockCount[4];					//扑克数目
    uint8_t							cbCardData[4][MAX_COUNT];			//扑克数据
};

//出牌结果
struct tagOutCardResult
{
    uint8_t							cbCardCount;						//扑克数目
    uint8_t							cbResultCard[MAX_COUNT];			//结果扑克
};

//分布信息
struct tagDistributing
{
    uint8_t							cbCardCount;						//扑克数目
    uint8_t							cbDistributing[15][6];				//分布信息
};

#define MAX_TYPE_COUNT 254
struct tagOutCardTypeResult
{
    uint8_t							cbCardType;							//扑克类型
    uint8_t							cbCardTypeCount;					//牌型数目
    uint8_t							cbEachHandCardCount[MAX_TYPE_COUNT];//每手个数
    uint8_t							cbCardData[MAX_TYPE_COUNT][MAX_COUNT];//扑克数据
};

//////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CGameLogic
{
    //变量定义
protected:
    static const uint8_t				m_cbCardData[FULL_COUNT];			//扑克数据

    //AI变量
public:
    static const uint8_t				m_cbGoodcardData[GOOD_CARD_COUTN];	//好牌数据
    uint8_t							m_cbAllCardData[GAME_PLAYER][MAX_COUNT];//所有扑克
    uint8_t							m_cbLandScoreCardData[MAX_COUNT];	//叫牌扑克
    uint8_t							m_cbUserCardCount[GAME_PLAYER];		//扑克数目
    WORD							m_wBankerUser;						//地主玩家

    //函数定义
public:
    //构造函数
    CGameLogic();
    //析构函数
    virtual ~CGameLogic();

    //类型函数
public:
    //获取类型
    uint8_t GetCardType(const uint8_t cbCardData[], uint8_t cbCardCount);
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return cbCardData&MASK_COLOR; }

    //控制函数
public:
    //混乱扑克
    VOID RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount);
    //排列扑克
    VOID SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t cbSortType);
    //删除扑克
    bool RemoveCard(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount);

    //逻辑函数
public:
    //有效判断
    bool IsValidCard(uint8_t cbCardData);
    //逻辑数值
    uint8_t GetCardLogicValue(uint8_t cbCardData);
    //对比扑克
    bool CompareCard(const uint8_t cbFirstCard[], const uint8_t cbNextCard[], uint8_t cbFirstCount, uint8_t cbNextCount);
    //出牌搜索
    bool SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & OutCardResult);

    //内部函数
public:
    //构造扑克
    uint8_t MakeCardData(uint8_t cbValueIndex, uint8_t cbColorIndex);
    //分析扑克
    VOID AnalysebCardData(const uint8_t cbCardData[], uint8_t cbCardCount, tagAnalyseResult & AnalyseResult);
    //分析分布
    VOID AnalysebDistributing(const uint8_t cbCardData[], uint8_t cbCardCount, tagDistributing & Distributing);

    //////////////////////////////////////////////////////////////////////////
    //AI函数
public:
    //设置扑克
    VOID SetUserCard(WORD wChairID, uint8_t cbCardData[], uint8_t cbCardCount) ;
    //设置底牌
    VOID SetBackCard(WORD wChairID, uint8_t cbBackCardData[], uint8_t cbCardCount) ;
    //设置庄家
    VOID SetBanker(WORD wBanker) ;
    //叫牌扑克
    VOID SetLandScoreCardData(uint8_t cbCardData[], uint8_t cbCardCount) ;
    //删除扑克
    VOID RemoveUserCardData(WORD wChairID, uint8_t cbRemoveCardData[], uint8_t cbRemoveCardCount) ;

    //辅助函数
public:
    //组合算法
    VOID Combination(uint8_t cbCombineCardData[], uint8_t cbResComLen,  uint8_t cbResultCardData[254][5], uint8_t &cbResCardLen,uint8_t cbSrcCardData[] , uint8_t cbCombineLen1, uint8_t cbSrcLen, const uint8_t cbCombineLen2);
    //排列算法
    VOID Permutation(uint8_t *list, INT m, INT n, uint8_t result[][4], uint8_t &len) ;
    //分析炸弹
    VOID GetAllBomCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbBomCardData[], uint8_t &cbBomCardCount);
    //分析顺子
    VOID GetAllLineCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbLineCardData[], uint8_t &cbLineCardCount);
    //分析三条
    VOID GetAllThreeCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbThreeCardData[], uint8_t &cbThreeCardCount);
    //分析对子
    VOID GetAllDoubleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbDoubleCardData[], uint8_t &cbDoubleCardCount);
    //分析单牌
    VOID GetAllSingleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbSingleCardData[], uint8_t &cbSingleCardCount);

    //主要函数
public:
    //分析牌型（后出牌调用）
    VOID AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t const cbTurnCardData[], uint8_t const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
    //分析牌牌（先出牌调用）
    VOID AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
    //单牌个数
    uint8_t AnalyseSinleCardCount(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t const cbWantOutCardData[], uint8_t const cbWantOutCardCount, uint8_t cbSingleCardData[]=NULL);

    //出牌函数
public:
    //是否是三个相同的
    bool IsThreeSame(const uint8_t cbCardData[], uint8_t	cbCardCount);
    //炸弹加单牌先出单牌
    bool IsBombWithSingle(const uint8_t cbHandCardData[], uint8_t cbHandCardCont, uint8_t& OutCard);
    //地主出牌（先出牌）
    VOID BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, tagOutCardResult & OutCardResult) ;
    //地主出牌（后出牌）
    VOID BankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, WORD wOutCardUser, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    //地主上家（先出牌）
    VOID UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, WORD wMeChairID,tagOutCardResult & OutCardResult) ;
    //地主上家（后出牌）
    VOID UpsideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, WORD wOutCardUser,  const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    //地主下家（先出牌）
    VOID UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, WORD wMeChairID,tagOutCardResult & OutCardResult) ;
    //地主下家（后出牌）
    VOID UndersideOfBankerOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, WORD wOutCardUser, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    //出牌搜索
    bool SearchOutCard(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, WORD wOutCardUser, WORD wMeChairID, tagOutCardResult & OutCardResult);

    //叫分函数
public:
    //叫分判断
    uint8_t LandScore(WORD wMeChairID, uint8_t cbCurrentLandScore) ;
    //加倍判断
    uint8_t DoubleScore(WORD wMeChairID, uint8_t cbCurrentLandScore) ;
	//分析能否出完剩下的牌
	bool IsCanOutAllCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13], WORD wCurrentUser, bool firstOutUser);
	//分析王炸加2手能否出完剩下的牌
	//bool IsCanOutAllCard_TwoKing(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13], WORD wCurrentUser, bool firstOutUser);
	//牌型组合是否两手牌型能出完
	bool IsTwoCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t &CardType1, uint8_t &TypeIndex1, uint8_t &CardType2, uint8_t &TypeIndex2, tagOutCardTypeResult CardTypeResult[13]);
	//分析炸弹加1手能否出完剩下的牌
	bool IsCanOutAllCard_BombAndOneCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13], WORD wCurrentUser, bool firstOutUser);
	//分析炸弹加2手能否出完剩下的牌
	bool IsCanOutAllCard_BombAndTwoCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13], WORD wCurrentUser, bool firstOutUser);
	//分析大牌加2手能否出完剩下的牌
	bool IsCanOutAllCard_BigCardAndTwoCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13], WORD wCurrentUser, bool firstOutUser);
	//分析2手牌能否出完剩下的牌
	bool IsCanOutAllCard_TwoCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13], WORD wCurrentUser, bool firstOutUser);
	//出牌玩家仅剩一张牌了,手上的炸弹是否需要炸
	bool AnalyseSinleOneCardIsNeedBomb(const uint8_t cbHandCardData[], uint8_t cbHandCardCount, WORD wOutCardUser, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount, tagOutCardResult & cbCanOutCardData, tagOutCardTypeResult CardTypeResult[13]);
};

//////////////////////////////////////////////////////////////////////////

#endif
