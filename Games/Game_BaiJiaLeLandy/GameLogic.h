#ifndef GAME_LOGIC_HEAD_FILE
#define GAME_LOGIC_HEAD_FILE

#include <stdint.h>
#include <vector>
#include "stdlib.h"
#include "stdint.h"
#include "public/gameDefine.h"

using namespace std;
//////////////////////////////////////////////////////////////////////////
//宏定义

#define MAX_COUNT					6									//最大数目
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

//enum eGoodType
//{
//	NoHave					= 0,
//	ChangZhuang				= 1,	//长庄
//	ChangXian				= 2,	//长闲
//	DaLuDantiao				= 3,	//大路单跳
//	DaLuShuangtiao			= 4,	//大路双跳
//	FengZhuangtiao			= 5,	//逢庄跳
//	FengXiantiao			= 6,	//逢闲跳
//	YitingLiangfan_Zhuang   = 7,	//一厅两房庄
//	YitingLiangfan_Xian     = 8,	//一厅两房闲
//	FengZhuanglian			= 9,	//逢庄连
//	FengXianlian			= 10,	//逢闲连
//	PaipaiChi				= 11,	//拍拍黐
//	XiepoLu					= 12,	//斜坡路
//};
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
	//牌值是否有效
	bool IsCardValid(uint8_t cbCardData);
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

	//获取牌点
	uint8_t GetCardPip(uint8_t cbCardData);
	//获取牌点
	uint8_t GetCardListPip(const uint8_t cbCardData[], uint8_t cbCardCount);
	//是否牌对 index: 0和对 1庄对 2闲对
	uint8_t IsCardPair(uint8_t cbBankerData[], uint8_t cbPlayerData[], uint8_t cbCardCount, uint8_t index);
	uint8_t IsCardPair(uint8_t cbData[]);
	//获取好路单
	int getGoodRouteType(vector<int> vRecord);
	//获取好路单名称
	string getGoodRouteName(int typeId);
	//是否长庄index=1 是否长闲  index=2
	int isGoodType_ChangeZhuangXian(vector<int> vRecord, int index);
	//大路单跳 index=1 大路双跳 index=2
	int isGoodType_DaluTiao(vector<int> vRecord, int index);
	//是否逢庄闲跳
	int isGoodType_FengZXTiao(vector<int> vRecord);
	//是否一厅两房
	int isGoodType_YitingLiangfan(vector<int> vRecord);
	//是否逢庄闲连
	int isGoodType_FengZXLian(vector<int> vRecord);
	//是否拍拍黐
	int isGoodType_PaipaiChi(vector<int> vRecord);
	//是否斜坡路
	int isGoodType_XiepoLu(vector<int> vRecord);
};

//////////////////////////////////////////////////////////////////////////

#endif
