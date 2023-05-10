#ifndef CMD_GAME_HEAD_FILE
#define CMD_GAME_HEAD_FILE


#include "types.h"
#include "public/Globals.h"
#include "public/gameDefine.h"

#define WPARAM int

//////////////////////////////////////////////////////////////////////////////////
//服务定义

//游戏属性
#define KIND_ID						620									//游戏 I D
#define GAME_NAME					TEXT("斗地主")						//游戏名字

//组件属性
#define GAME_PLAYER					3									//游戏人数
#define VERSION_SERVER				PROCESS_VERSION(6,0,3)				//程序版本
#define VERSION_CLIENT				PROCESS_VERSION(6,0,3)				//程序版本

//////////////////////////////////////////////////////////////////////////////////

#define LEN_NICKNAME 32

#define SCORE_LL					LONGLONG

//数目定义
#define MAX_COUNT					20									//最大数目
#define FULL_COUNT					54									//全牌数目
#define MAX_NO_CALL_BANKER			3									//最多只能三局不叫地主

#define NORMAL_COUNT				17									//常规数目
#define DISPATCH_COUNT				51									//派发数目
#define GOOD_CARD_COUTN				38									//好牌数目
#define MAX_CARD_VALUE				15									//牌最大逻辑数值
#define MAX_CARD_COLOR				4									//牌最大花色数值

//数值掩码
#define	MASK_COLOR					0xF0								//花色掩码
#define	MASK_VALUE					0x0F								//数值掩码

//数值掩码
#define	LOGIC_MASK_COLOR			0xF0								//花色掩码
#define	LOGIC_MASK_VALUE			0x0F								//数值掩码

#define GAME_SCENE_FREE				201									//等待开始
#define GAME_SCENE_CALL				202									//叫分状态
#define GAME_SCENE_CALLEnd          203									//叫分状态
#define GAME_SCENE_STAKE			204									//加倍状态
#define GAME_SCENE_STAKEEnd			205									//加倍状态end
#define GAME_SCENE_PLAY				206									//游戏进行


//控制状态
#define CONTROL_STATUS_NORMAL		0									//正常用户
#define CONTROL_STATUS_BLACK		1									//黑名单用户
#define CONTROL_STATUS_WHITE		2									//白名单用户

#define GER_NORMAL_					0x00								//常规结束
#define GER_DISMISS_				0x01								//游戏解散
#define GER_USER_LEAVE				0x02								//用户离开
#define GER_NETWORK_ERROR			0x03								//网络错误

#define US_NULL						0x00								//没有状态
#define US_FREE						0x01								//站立状态
#define US_SIT						0x02								//坐下状态
#define US_READY					0x03								//同意状态
#define US_PLAYING					0x04								//游戏状态
#define US_OFFLINE					0x05								//断线状态
#define US_LOOKON					0x06								//旁观状态
#define US_CLEAR_START				0x07								//清理开始
#define US_READYEX					0x08								//清理同意

#define MAX_TYPE1 15

//配置结构
struct tagCustomRule
{
	//其他定义
	WORD            wMaxScoreTimes;           //最大倍数
	WORD            wFleeScoreTimes;          //逃跑倍数
	uint8_t            cbFleeScorePatch;         //逃跑补偿

	//时间定义
	uint8_t            cbTimeOutCard;            //出牌时间
	uint8_t            cbTimeCallScore;          //叫分时间
	uint8_t            cbTimeStartGame;          //开始时间
	uint8_t            cbTimeHeadOutCard;        //首出时间
	DWORD           cbTimeDealCard;
	DWORD			cbTimeOutPass;
	DWORD			cbTimeShowBomb;
	DWORD			cbTimeShowPlane;
	DWORD			cbTimeOutRocket;
	DWORD			cbTimeStakeScore;
};

//逻辑类型
/*
#define CT_ERROR					0									//错误类型
#define CT_SINGLE					1									//单牌类型
#define CT_DOUBLE					2									//对牌类型
#define CT_THREE					3									//三条类型
#define CT_SINGLE_LINE				4									//单连类型
#define CT_DOUBLE_LINE				5									//对连类型
#define CT_THREE_LINE				6									//三连类型
#define CT_THREE_TAKE_ONE			7									//三带一单
#define CT_THREE_TAKE_TWO			8									//三带一对
#define CT_FOUR_TAKE_ONE			9									//四带两单
#define CT_FOUR_TAKE_TWO			10									//四带两对
#define CT_BOMB_CARD				11									//炸弹类型
#define CT_MISSILE_CARD				12									//火箭类型
*/
//////////////////////////////////////////////////////////////////////////////////
//状态定义

/*
sGetout     = 0,        // player get out.
sFree,                  // player is free.
sSit,                   // player is sitdown.
sReady,                 // player is ready.
sPlaying,               // player is playing.
sOffline,               // player is offline.
sLookon,                // player is lookon.
sGetoutAtplaying        // player is get out at playing.
*/
/*


struct tagCustomRule
{
    //其他定义
    WORD							wMaxScoreTimes;						//最大倍数
    WORD							wFleeScoreTimes;					//逃跑倍数
    uint8_t							cbFleeScorePatch;					//逃跑补偿

    //时间定义
    uint8_t							cbTimeOutCard;						//出牌时间(15s)
    uint8_t							cbTimeDealCard;						//发牌时间(2s)
    uint8_t							cbTimeCallScore;					//叫分时间(15s)
    uint8_t							cbTimeStakeScore;					//加倍时间(14s)
    uint8_t							cbTimeHeadOutCard;					//首出时间(25s)
    uint8_t							cbTimeOutBomb;						//上家炸弹出牌时间(18s)
    uint8_t							cbTimeOutPlane;						//上家飞机出牌时间(17s)
    uint8_t							cbTimeOutRocket;					//上家火箭出牌时间(17s)
    uint8_t							cbTimeOutPass;						//要不起出牌时间(5s)
    uint8_t							cbTimeShowBomb;						//炸弹表现时间(3s)
    uint8_t							cbTimeShowPlane;					//飞机表现时间(2s)
    uint8_t							cbTimeShowRocket;					//火箭表现时间(2s)
};


//空闲状态
struct CMD_S_StatusFree
{

    //游戏属性
    SCORE  lCellScore;
    //时间信息
    uint8_t                            cbTimeOutCard;
    uint8_t							cbTimeCallScore;					//叫分时间
    uint8_t							cbStakeTime;						//下注时间
    uint8_t							cbTimeHeadOutCard;					//首出时间
    uint8_t							cbTimeOutBomb;						//上家炸弹出牌时间
    uint8_t							cbTimeOutPlane;						//上家飞机出牌时间
    uint8_t							cbTimeOutRocket;					//上家火箭出牌时间
    uint8_t							cbTimeOutPass;						//要不起出牌时间

    bool							bTrustee[GAME_PLAYER];			    //是否托管
    //历史积分
    SCORE							lTurnScore[GAME_PLAYER];			//积分信息
    SCORE							lCollectScore[GAME_PLAYER];			//积分信息
};

//叫分状态
struct CMD_S_StatusCall
{
    //时间信息
    uint8_t							cbTimeOutCard;						//出牌时间
    uint8_t							cbTimeCallScore;					//叫分时间
    uint8_t							cbStakeTime;						//下注时间
    uint8_t							cbTimeHeadOutCard;					//首出时间
    uint8_t							cbTimeOutBomb;						//上家炸弹出牌时间
    uint8_t							cbTimeOutPlane;						//上家飞机出牌时间
    uint8_t							cbTimeOutRocket;					//上家火箭出牌时间
    uint8_t							cbTimeOutPass;						//要不起出牌时间

    //游戏信息
    SCORE							lCellScore;							//单元积分
    WORD							wCurrentUser;						//当前玩家
    uint8_t							cbBankerScore;						//庄家叫分
    uint8_t							cbScoreInfo[GAME_PLAYER];			//叫分信息(255:不叫分0：未操作1：1分2：2分3：3分)
    uint8_t							cbHandCardData[NORMAL_COUNT];		//手上扑克
    bool							bTrustee[GAME_PLAYER];				//是否托管
    //历史积分
    SCORE							lTurnScore[GAME_PLAYER];			//积分信息
    SCORE							lCollectScore[GAME_PLAYER];			//积分信息

    //定时器剩余时间
    uint8_t                           cbRemainTime;                       //定时器剩余时间
};

//下注状态
struct CMD_S_StatusStake
{
    //时间信息
    uint8_t							cbTimeOutCard;						//出牌时间
    uint8_t							cbTimeCallScore;					//叫分时间
    uint8_t							cbStakeTime;						//下注时间
    uint8_t							cbTimeHeadOutCard;					//首出时间
    uint8_t							cbTimeOutBomb;						//上家炸弹出牌时间
    uint8_t							cbTimeOutPlane;						//上家飞机出牌时间
    uint8_t							cbTimeOutRocket;					//上家火箭出牌时间
    uint8_t							cbTimeOutPass;						//要不起出牌时间

                                                                        //游戏信息
    SCORE							lCellScore;							//单元积分
    WORD							wCurrentUser;						//当前玩家
    WORD							wBankerUser;						//庄家用户
    uint8_t							cbBankerScore;						//庄家叫分
    uint8_t							cbStakeInfo[GAME_PLAYER];			//下注信息(0:没有操作1:不加倍2:加倍)
    uint8_t							cbHandCardData[NORMAL_COUNT];		//手上扑克
    bool							bTrustee[GAME_PLAYER];				//是否托管
    SCORE							lTurnScore[GAME_PLAYER];			//积分信息
    SCORE							lCollectScore[GAME_PLAYER];			//积分信息

    uint8_t							cbRemainTime;                       //定时器剩余时间
};

//游戏状态
struct CMD_S_StatusPlay
{
    //时间信息
    unsigned char							cbTimeOutCard;						//出牌时间
    unsigned char							cbTimeCallScore;					//叫分时间
    unsigned char							cbStakeTime;						//下注时间
    unsigned char							cbTimeHeadOutCard;					//首出时间
    unsigned char							cbTimeOutBomb;						//上家炸弹出牌时间
    unsigned char							cbTimeOutPlane;						//上家飞机出牌时间
    unsigned char							cbTimeOutRocket;					//上家火箭出牌时间
    unsigned char							cbTimeOutPass;						//要不起出牌时间

    //游戏变量
    SCORE							lCellScore;							//单元积分
    uint8_t							cbBombCount;						//炸弹次数
    WORD							wBankerUser;						//庄家用户
    WORD							wCurrentUser;						//当前玩家
    uint8_t							cbBankerScore;						//庄家叫分
    uint8_t							cbStakeInfo[GAME_PLAYER];			//下注信息(0:没有操作1:不加倍2:加倍)

    //出牌信息
    WORD							wTurnWiner;							//胜利玩家
    uint8_t							cbTurnCardCount;					//出牌数目
    uint8_t							cbTurnCardData[MAX_COUNT];			//出牌数据
    bool							bTrustee[GAME_PLAYER];				//是否托管

    //扑克信息
    uint8_t							cbBankerCard[BANK_COUNT];			//游戏底牌
    uint8_t							cbHandCardData[MAX_COUNT];			//手上扑克
    uint8_t							cbHandCardCount[GAME_PLAYER];		//扑克数目
    uint8_t							cbSurplusCardData[FULL_COUNT];		//剩余扑克
    uint8_t							cbSurplusCardCount;					//剩余扑克

    //历史积分
    SCORE							lTurnScore[GAME_PLAYER];			//积分信息
    SCORE							lCollectScore[GAME_PLAYER];			//积分信息

    //定时器剩余时间
    uint8_t                           cbRemainTime;                       //定时器剩余时间
};

//////////////////////////////////////////////////////////////////////////////////
//命令定义

#define SUB_S_GAME_START			100									//游戏开始
#define SUB_S_CALL_SCORE			101									//用户叫分
#define SUB_S_BANKER_INFO			102									//庄家信息
#define SUB_S_OUT_CARD				103									//用户出牌
#define SUB_S_PASS_CARD				104									//用户不出牌
#define SUB_S_GAME_CONCLUDE			105									//游戏结束
#define SUB_S_USER_EXIT				106									//用户离开
#define SUB_S_TRUSTEE				107									//用户托管
#define SUB_S_CHEAT_DATA			108									//超端数据
#define SUB_S_STAKE_SCORE			109									//用户加注
#define SUB_S_STAKE_END				110									//用户加注结束
#define SUB_S_REMAIN_CARD_NUM		111									//当前玩家剩余扑克数目(依次是A - K,小王，大王)


//发送扑克
struct CMD_S_GameStart
{
    WORD							wStartUser;							//开始玩家
    WORD				 			wCurrentUser;						//当前玩家
    uint8_t							cbValidCardData;					//明牌扑克
    uint8_t							cbValidCardIndex;					//明牌位置
    uint8_t							cbCardData[NORMAL_COUNT];			//扑克列表
    uint8_t							cbOtherCardData[GAME_PLAYER][MAX_COUNT];//其他玩家的扑克列表 供超端看到所有牌使用
    uint8_t							cbBankerCard[BANK_COUNT];			//庄家底牌
    uint8_t                            cbVersion;                          //文件版本号
};
//作弊数据
struct  CMD_S_CheatData
{
    uint8_t							cbOtherCardData[GAME_PLAYER][MAX_COUNT];	    //其他玩家的扑克列表 供超端看到所有牌使用
};

//机器人扑克
struct CMD_S_AndroidCard
{
    uint8_t							cbHandCard[GAME_PLAYER][NORMAL_COUNT];//手上扑克
    uint8_t							cbBackCard[3];
    WORD							wCurrentUser ;						//当前玩家
};

//用户叫分
struct CMD_S_CallScore
{
    WORD				 			wCurrentUser;						//当前玩家
    WORD							wCallScoreUser;						//叫分玩家
    uint8_t							cbCurrentScore;						//当前叫分
    uint8_t							cbUserCallScore;					//上次叫分
    WORD							wStakeUser;							//下注玩家
};

//用户加注
struct CMD_S_StakeScore
{
    WORD							wCallScoreUser;						//加注玩家
    bool							bIsAddScore;						//加注玩家是否加注
};

//庄家信息
struct CMD_S_BankerInfo
{
    WORD				 			wBankerUser;						//庄家玩家
    WORD				 			wCurrentUser;						//当前玩家
    uint8_t							cbBankerScore;						//庄家叫分
    uint8_t							cbBankerCard[GAME_PLAYER];			//庄家扑克
};


//用户出牌
struct CMD_S_OutCard
{
    uint8_t							cbCardCount;						//出牌数目
    WORD				 			wCurrentUser;						//当前玩家
    WORD							wOutCardUser;						//出牌玩家
    uint8_t							cbRecordCount;						//超时剩余
    uint8_t							cbCardData[MAX_COUNT];				//扑克列表
    bool							bPass;								//当前玩家是否能要的起
    bool							bOutAll;							//当前玩家是否可以出完最后一手牌
};

#define MAX_TYPE1 15
//用户剩余牌数目
struct CMD_S_RemainCardNum
{
    uint8_t							cbRemainCardNum[MAX_TYPE1];			//当前玩家剩余扑克数目(依次是A - K,小王，大王)
};

//用户不出牌
struct CMD_S_PassCard
{
    uint8_t							cbTurnOver;							//一轮结束
    WORD				 			wCurrentUser;						//当前玩家
    WORD				 			wPassCardUser;						//放弃玩家
    bool							bPass;								//当前玩家是否能要的起
    bool							bOutAll;							//当前玩家是否可以出完最后一手牌
};

//游戏结束
struct CMD_S_GameConclude
{
    //积分变量
    SCORE							lCellScore;							//单元积分
    SCORE							lGameScore[GAME_PLAYER];			//游戏积分

    //春天标志
    uint8_t							bChunTian;							//春天标志
    uint8_t							bFanChunTian;						//春天标志

    //炸弹信息
    uint8_t							cbBombCount;						//炸弹个数
    uint8_t							cbEachBombCount[GAME_PLAYER];		//炸弹个数

    //游戏信息
    uint8_t							cbBankerScore;						//叫分数目
    uint8_t							cbCardCount[GAME_PLAYER];			//扑克数目
    uint8_t							cbHandCardData[FULL_COUNT];			//扑克列表
    bool							bFlee;								//结束逃跑
    TCHAR							szNickName[GAME_PLAYER][LEN_NICKNAME];	//游戏昵称
};

//用户退出
struct CMD_S_USER_EXIT
{
    WORD							wChairID;							//用户椅子
};

//用户托管
struct CMD_S_Trustee
{
    bool							bTrustee;							//是否托管
    WORD							wChairID;							//托管用户
};

//////////////////////////////////////////////////////////////////////////////////
//命令定义

#define SUB_C_CALL_SCORE			1									//用户叫分
#define SUB_C_OUT_CARD				2									//用户出牌
#define SUB_C_PASS_CARD				3									//用户不出牌
#define SUB_C_TRUSTEE				4									//用户托管
#define SUB_C_STAKE_SCORE			5									//用户加注
#define SUB_C_REMAIN_CARD_NUM		6									//当前玩家剩余扑克数目(依次是A - K,小王，大王)
//用户叫分
struct CMD_C_CallScore
{
    uint8_t							cbCallScore;						//叫分数目
};

//用户下注
struct CMD_C_StakeScore
{
    uint8_t							cbOpId;						//操作ID （0:不加倍,1:加倍)
};

#define  C_STAKE_SCORE_ONE				0						//不加倍
#define  C_STAKE_SCORE_TWO				1						//加倍

//用户出牌
struct CMD_C_OutCard
{
public:
    uint8_t							cbCardCount;						//出牌数目
    uint8_t							cbCardData[MAX_COUNT];				//扑克数据
};

//用户托管
struct CMD_C_Trustee
{
    bool							bTrustee;							//是否托管
};
*/


//////////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif
