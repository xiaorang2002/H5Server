#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include "ddz.Message.pb.h"
#include "CMD_Game.h"

using namespace ddz;

//////////////////////////////////////////////////////////////////////////////////

//排序类型
#define ST_ORDER					1									//大小排序
#define ST_COUNT					2									//数目排序
#define ST_CUSTOM					3									//自定排序


//////////////////////////////////////////////////////////////////////////////////

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

//搜索结果
struct tagSearchCardResult
{
    uint8_t							cbSearchCount;						//结果数目
    uint8_t							cbCardCount[MAX_COUNT];				//扑克数目
    uint8_t							cbResultCard[MAX_COUNT][MAX_COUNT];	//结果扑克
};

#define MAX_TYPE_COUNT 254
struct tagOutCardTypeResult
{
	uint8_t							cbCardType;							//扑克类型
	uint8_t							cbCardTypeCount;					//牌型数目
	uint8_t							cbEachHandCardCount[MAX_TYPE_COUNT];//每手个数
	uint8_t							cbCardData[MAX_TYPE_COUNT][MAX_COUNT];//扑克数据
};

//////////////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CGameLogic
{
    //变量定义
protected:
    static const uint8_t				m_cbCardData[FULL_COUNT];			//扑克数据
    static const uint8_t				m_cbGoodcardData[GOOD_CARD_COUTN];	//好牌数据//2015.4.28

	uint8_t								m_cbAllCardData[GAME_PLAYER][MAX_COUNT];//所有扑克
	uint8_t								m_cbLandScoreCardData[MAX_COUNT];		//叫牌扑克
	uint8_t								m_cbUserCardCount[GAME_PLAYER];			//扑克数目
	WORD								m_wBankerUser;							//地主玩家

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

    //获取数值
    //uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
    //获取花色
    //uint8_t GetCardColor(uint8_t cbCardData)
    //{
    //    uint8_t color = cbCardData&LOGIC_MASK_COLOR;
    //   color = color>>4;
    //    return color;
    //}

    //控制函数
public:
    //混乱扑克
    VOID RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount);
    //混乱扑克
    VOID RandCardListEX(uint8_t cbCardBuffer[], uint8_t cbBufferCount);
    //排列扑克
    VOID SortCardList(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t cbSortType);
    //删除扑克
    bool RemoveCardList(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount);
    //排列扑克
    VOID SortOutCardList(uint8_t cbCardData[], uint8_t cbCardCount);
    //删除扑克
    bool RemoveCard(const uint8_t cbRemoveCard[], uint8_t cbRemoveCount, uint8_t cbCardData[], uint8_t cbCardCount);
    //逻辑函数
public:
    //逻辑数值
    uint8_t GetCardLogicValue(uint8_t cbCardData);
    //对比扑克
    bool CompareCard(const uint8_t cbFirstCard[], const uint8_t cbNextCard[], uint8_t cbFirstCount, uint8_t cbNextCount);

    //内部函数
public:
    //构造扑克
    uint8_t MakeCardData(uint8_t cbValueIndex, uint8_t cbColorIndex);
    //分析扑克
    VOID AnalysebCardData(const uint8_t cbCardData[], uint8_t cbCardCount, tagAnalyseResult & AnalyseResult);
    //分析分布
    VOID AnalysebDistributing(const uint8_t cbCardData[], uint8_t cbCardCount, tagDistributing & Distributing);

    uint8_t SearchOutCard( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, const uint8_t cbTurnCardData[], uint8_t cbTurnCardCount,
                               tagSearchCardResult *pSearchCardResult );

    uint8_t SearchTakeCardType( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint8_t cbReferCard, uint8_t cbSameCount, uint8_t cbTakeCardCount,
                                    tagSearchCardResult *pSearchCardResult );
                                    //同牌搜索
    uint8_t SearchSameCard( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint8_t cbReferCard, uint8_t cbSameCardCount,
                                    tagSearchCardResult *pSearchCardResult );
        //连牌搜索
    uint8_t SearchLineCardType( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, uint8_t cbReferCard, uint8_t cbBlockCount, uint8_t cbLineCount,
        tagSearchCardResult *pSearchCardResult );
    //搜索飞机
    uint8_t SearchThreeTwoLine( const uint8_t cbHandCardData[], uint8_t cbHandCardCount, tagSearchCardResult *pSearchCardResult );
    void GetGoodCardData(uint8_t cbGoodCardData[NORMAL_COUNT]);
    bool RemoveGoodCardData(uint8_t cbGoodcardData[NORMAL_COUNT], uint8_t cbGoodCardCount, uint8_t cbCardData[FULL_COUNT], uint8_t cbCardCount) ;

	//辅助函数
public:
	//组合算法
	VOID Combination(uint8_t cbCombineCardData[], uint8_t cbResComLen, uint8_t cbResultCardData[254][5], uint8_t &cbResCardLen, uint8_t cbSrcCardData[], uint8_t cbCombineLen1, uint8_t cbSrcLen, const uint8_t cbCombineLen2);
	//排列算法
	//VOID Permutation(uint8_t *list, INT m, INT n, uint8_t result[][4], uint8_t &len);
	//分析炸弹
	VOID GetAllBomCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbBomCardData[], uint8_t &cbBomCardCount);
	//分析顺子
	VOID GetAllLineCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbLineCardData[], uint8_t &cbLineCardCount);
	//分析三条
	VOID GetAllThreeCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbThreeCardData[], uint8_t &cbThreeCardCount);
	//分析对子
	VOID GetAllDoubleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbDoubleCardData[], uint8_t &cbDoubleCardCount);
	//分析单牌
	//VOID GetAllSingleCard(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, uint8_t cbSingleCardData[], uint8_t &cbSingleCardCount);
	//分析手牌
	VOID AnalyseOutCardType(uint8_t const cbHandCardData[], uint8_t const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12 + 1]);
	//获取牌型对应的权值
	int32_t get_GroupData(uint8_t cgType, int MaxCard, int Count);
	//获取手牌的权值
	int32_t get_HandCardValue(const uint8_t cbHandCardData[], uint8_t cbHandCardCount);

private:
	random_device m_rd;
	std::mt19937 m_gen;

};

//////////////////////////////////////////////////////////////////////////////////

#endif
