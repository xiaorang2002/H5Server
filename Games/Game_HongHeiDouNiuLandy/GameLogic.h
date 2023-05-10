#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////
//宏定义

#define MAX_COUNT					5									//最大数目
#define	DRAW						2									//和局类型

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码

//扑克类型
// 0:无牛;1:牛一;2:牛二;3:牛三;4:牛四;5:牛五;6:牛六;7:牛七;8:牛八;9:牛九;10:牛牛;11:四花牛;12:五花牛;13:四炸;14:五小牛;
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
    CT_SIHUA_NIU        =        11,                                  //四花牛 
    CT_WUHUA_NIU        =        12,                                  //五花牛
    CT_BOMB_NIU         =        13,                                  //炸弹牛(四炸)
    CT_FIVESMALL_NIU    =        14,                                  //五小牛

    CT_MAXCOUNT         =        15,                                  //牌型的总类
};

//////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CGameLogic
{
	//变量定义
private:
    static uint8_t						m_cbCardListData[52];				//扑克定义

	//函数定义
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
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
	//获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return (cbCardData&LOGIC_MASK_COLOR)>>4; }
	//逻辑数值
	uint8_t GetLogicValue(uint8_t cbCardData);
	//控制函数
public:
	//排列扑克
    void SortCardList(uint8_t cbCardData[], uint8_t cbCardCount);
	//混乱扑克
    void RandCardList(uint8_t cbCardBuffer[], uint8_t cbBufferCount);

	//功能函数
public:
	//逻辑数值
    uint8_t GetCardLogicValue(uint8_t cbCardData);
	//对比扑克
    uint8_t CompareCard(uint8_t cbFirstData[], uint8_t cbNextData[], uint8_t cbCardCount);

    long long GetValue(uint8_t cbCardData[], uint8_t cbCardCount);
    uint8_t GetCardLevelValue(uint8_t cbCardData[], uint8_t cbCardCount, uint8_t bLevel);
    int64_t GetCardMaxValue(uint8_t cbCardData[], uint8_t cbCardCount);

	//获取倍数
	uint8_t GetMultiple(uint8_t cbCardData[]);
	//获取牛牛牌
	bool GetOxCard(uint8_t cbCardData[]);
	//是否为五小
	bool IsFiveSmall(uint8_t cbCardData[MAX_COUNT]);
	//是否为炸弹
	bool IsBomb(uint8_t cbCardData[MAX_COUNT]);
	//是否为金牛(五花牛)
	bool IsWuHuaBull(uint8_t cbCardData[MAX_COUNT]);
	//是否为银牛(四花牛)
	bool IsSiHuaBull(uint8_t cbCardData[MAX_COUNT]);
	//炸弹牛4+1、四花牛牌型序
	bool SortCardListBH(uint8_t cbCardData[MAX_COUNT], uint8_t cbCardType);
};

//////////////////////////////////////////////////////////////////////////

#endif
