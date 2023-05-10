#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <memory.h>
#include <vector>

//////////////////////////////////////////////////////////////////////////////////
//宏定义
//#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))

#ifndef isZero
#define isZero(a)        ((a>-0.000001) && (a<0.000001))
#endif//isZero


#define MAX_COUNT					5									//最大数目
#define	DRAW						2									//和局类型

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码


#define MAX_CARD_TOTAL				28									//牌总个数
using namespace std;

typedef vector<uint8_t>::iterator SortedBuffer;
//扑克类型

enum CARD_TYPE {
    CT_SINGLE = 1,
    CT_DOUBLE = 2,
    CT_TWO_DOUBLE =3,
    CT_THIRD = 4,
    CT_LINK = 5,
    CT_SAME_FLOWER = 6,
    CT_CALABASH = 7,
    CT_FOUR = 8,
    CT_SAME_FLOWER_LINK = 9,
    CT_MAX_SAME_FLOWER_LINK =10
};
//////////////////////////////////////////////////////////////////////////////////
//游戏逻辑类
class CGameLogic
{

public:
    //构造函数
    CGameLogic();
    //析构函数
    virtual ~CGameLogic();

    //类型函数
public:
    //获取类型
    uint8_t GetCardType(uint8_t cbCardData[], uint8_t cbCardCount);
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData & LOGIC_MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return cbCardData & LOGIC_MASK_COLOR; }

    //控制函数
public:
    //排列扑克
    void SortCardList(vector<uint8_t> & cards);
    //混乱扑克
    void RandCardList();
    void GetHandCards(uint8_t cbHandCards[],uint8_t userCount);
    //功能函数
public:
    //逻辑数值
    uint8_t GetCardLogicValue(uint8_t cbCardData);
    //对比扑克
    uint8_t CompareCard(uint8_t cbFirstData[], uint8_t cbNextData[], uint8_t cbCardCount, uint8_t bCompareColor = 1);
    bool CompareSingleCard(uint8_t first,uint8_t second);
    void GetMaxCards(uint8_t cbHandCards[],uint8_t cardBuffer[],uint8_t cardCount,uint8_t cards[]);
protected:
//    bool RandomCardData(std::vector<uint8_t>& cbCardData);
public:
    void GetSingle(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetCalabash(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetSameFlowerLink(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetSameFlower(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetLink(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetDouble(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetThird(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetTwoDouble(uint8_t cbCardBuffer[],uint8_t cbCard[]);
    void GetFour(uint8_t cbCardBuffer[],uint8_t cbCard[]);
public:
    bool compareSingle(SortedBuffer sortedFirstCards,SortedBuffer sortedSecondCards, uint8_t cardCount);
    bool compareRepeat(SortedBuffer sortedFirstCards,SortedBuffer sortedSecondCards,uint8_t repeatCount,uint8_t cardCount);
    bool compareLink(SortedBuffer sortedFirstCards,SortedBuffer sortedSecondCards, uint8_t cardCount);
    bool compareTwoDouble(SortedBuffer sortedFirstCards,SortedBuffer sortedSecondCards, uint8_t cardCount);
public:
    //对子
    uint8_t isDouble(SortedBuffer sortedCards, uint8_t cardCount);
    //三张
    uint8_t isThird(SortedBuffer sortedCards, uint8_t cardCount);
    //四张
    uint8_t isFour(SortedBuffer sortedCards, uint8_t cardCount);
    //葫芦
    bool isCalabash(SortedBuffer sortedCards, uint8_t cardCount);
    //顺子
    bool isLink(SortedBuffer sortedCards, uint8_t cardCount);
    //两对
    bool isTwoDouble(SortedBuffer sortedCards, uint8_t cardCount);
    //同花
    bool isSameFlower(SortedBuffer sortedCards, uint8_t cardCount);
    //同花顺
    bool isSameFlowerLink(SortedBuffer sortedCards, uint8_t cardCount);
public:
    void printCardType(uint8_t type);
    //变量定义
    static uint8_t						m_cbCardListData[MAX_CARD_TOTAL];	//扑克定义
    vector<uint8_t>                     m_vecCardBuffer;                                                                       //函数定义
};

//////////////////////////////////////////////////////////////////////////
#endif


