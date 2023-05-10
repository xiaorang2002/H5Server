#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <stdint.h>


//////////////////////////////////////////////////////////////////////////
//宏定义

#define MAX_COUNT					3									//最大数目
#define	DRAW						2									//和局类型

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码

//扑克类型
#define CT_SINGLE					1									//单牌类型
#define CT_DOUBLE					2									//对子类型
#define	CT_SHUN_ZI					3									//顺子类型
#define CT_JIN_HUA					4									//金花类型
#define	CT_SHUN_JIN					5									//顺金类型
#define	CT_BAO_ZI					6									//豹子类型
#define CT_SPECIAL					7									//特殊类型

//////////////////////////////////////////////////////////////////////////

//游戏逻辑类
class CGameLogic
{
	//变量定义
private:
    static uint8_t						m_cbCardListData[52*8];				//扑克定义

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
};

//////////////////////////////////////////////////////////////////////////

#endif
