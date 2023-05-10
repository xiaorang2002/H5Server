#ifndef		___MESSAGE_DDZ_H_DEF___
#define		___MESSAGE_DDZ_H_DEF___

#include "public/Globals.h"
#include "public/gameDefine.h"
//#include "GameLogic.h"
#include <string.h>


#pragma pack(1)


//////////////////////////////////////////////////////////////////////////////////
//公共宏定义

//组件属性
#define GAME_PLAYER					5										//游戏人数
#define MIN_GAME_PLAYER				2										//最小游戏人数
//#define MAX_JETTON_NUM				5										//最大筹码个数

//下注倍数和结束局数定义
//筹码最大倍数(相对于基础倍数)
//#define MAX_JETTON_MULTIPLE         10
//加注筹码的倍数
#define JETTON_MULTIPLE_1           2
#define JETTON_MULTIPLE_2           4
//可以进行火拼局数
#define CAN_RUSH_JETTON_COUNT       3
//最大局数
#define MAX_JETTON_ROUND            20
//可以看牌
#define CAN_LOOKCARD_JETTON_COUNT   2


//结束原因
#define GER_NO_PLAYER				1										//没有玩家
#define GER_COMPARECARD				2										//比牌结束
#define GER_ALLIN					3										//孤注一掷结束
#define GER_RUSH					4										//火拼(最后一个玩家有跟注)
#define GER_RUSH_GIVEUP				5										//火拼(最后一个玩家没有跟注)

//#define GAME_SCENE_FREE				(GAME_STATUS_FREE)						//等待开始
//#define GAME_SCENE_PLAY				(GAME_STATUS_START)						//游戏进行
//#define GAME_SCENE_END				102										//游戏结束

//游戏状态
//#define GAME_STATUS_INIT                        97                              //INIT status
//#define GAME_STATUS_READY						98								//空闲状态
//#define GAME_STATUS_LOCK_PLAYING                99                              //Playing
//#define GAME_STATUS_PLAYING                     100                             //Playing
//#define GAME_STATUS_END				    	    102								//游戏结束



//#define GIVE_UP						2                                       //放弃概率
//#define LOOK_CARD					4                       				//看牌概率
//#define COMPARE_CARD				5                           			//比牌概率


//扑克类型
// #define ZJH::Tysp					1									//单牌类型
// #define ZJH::Ty20				2									//对子类型
// #define	ZJH::Ty123					3									//顺子类型
// #define ZJH::Tysc					4									//金花类型
// #define	ZJH::Ty123sc					5									//顺金类型
// #define	ZJH::Ty30					6									//豹子类型
// #define ZJH::Tysp235					7	

/*
//////////////////////////////////////////////////////////////////////////
//服务器命令结构
//场景消息
#define SUB_SC_GAMESCENE_FREE			2001								//空闲场景消息
#define SUB_SC_GAMESCENE_PLAY			2002								//游戏中场景消息
#define SUB_SC_GAMESCENE_END			2003								//游戏结束

#define SUB_S_GAME_START				100									//游戏开始
#define SUB_S_ADD_SCORE					101									//加注结果
#define SUB_S_GIVE_UP					102									//放弃跟注
#define SUB_S_COMPARE_CARD				103									//比牌跟注
#define SUB_S_LOOK_CARD					104									//看牌跟注
#define SUB_S_GAME_END					105									//游戏结束
#define SUB_S_ALL_IN					106									//孤注一掷
#define SUB_S_ALL_IN_RESULT				107									//孤注一掷结果
#define SUB_S_OPEN_CARD					108									//开牌消息
#define	SUB_S_AUTO_FOLLOW				109									//跟得底【CMD_S_Auto_Follow】
#define	SUB_S_RUSH						110									//火拼【】
#define SUB_S_RUSH_RESULT				111									//火拼结果【CMD_S_Rush_Result】
#define SUB_S_OUT_currentTurn_END				112									//超过局数结束【CMD_S_OutcurrentTurn_Result】

#define SUB_S_ANDROID_CARD				113									//智能消息

//#define SUB_S_WAIT_COMPARE				107									//等待比牌
//#define SUB_S_CHEAT_CARD				110									//特殊命令
//#define SUB_S_ANDROID_CARD				111									//智能消息

//空闲状态
struct CMD_S_StatusFree
{
    SCORE                               dCellScore;							//基础积分
    BYTE								cbCanRushCount;						//可以火拼的局数
};

//游戏状态
struct CMD_S_StatusPlay
{
	//加注信息
    SCORE                               dCellScore;							//基础积分
    SCORE                               dCurrentJetton;						//当前筹码值

    DWORD                               wBankerUser;						//庄家信息
    DWORD                               wCurrentUser;						//当前玩家
    BYTE								cbPlayStatus[GAME_PLAYER];			//游戏状态
	bool								bMingZhu[GAME_PLAYER];				//看牌状态
	bool								bGiveUp[GAME_PLAYER];				//弃牌状态
	bool								bAutoFollow[GAME_PLAYER];			//跟到底

    SCORE                               dTableJetton[GAME_PLAYER];			//下注数目
    SCORE                               dTotalJetton;						//总注数目

	//扑克信息
    BYTE								cbHandCardData[3];					//扑克数据
    BYTE								cbHandCardType;						//牌型

	//火拼信息
    BYTE								cbCanRushCount;						//可以火拼的局数
    BYTE								cbRushState;						//火拼状态
    SCORE                               dRushJetton;						//火拼数目
	
	//状态信息
    WORD								wJettonCount;						//下注次数(大于2时可以看牌比牌)

    WORD								wTimeLeft;							//剩余时间
    WORD								wTotalTime;							//总时间
};

//空闲状态
struct CMD_S_StatusEnd
{
    WORD								wWaitTime;							//等待时间
    BYTE								cbCanRushCount;						//可以火拼的局数
};


//游戏开始
struct CMD_S_GameStart
{
	//下注信息
    SCORE               				dCellScore;							//基础积分
    SCORE                   			dCurrentJetton;						//当前筹码值

	//用户信息
    DWORD           					wBankerUser;						//庄家信息
    DWORD               	 			wCurrentUser;						//当前玩家
    BYTE								cbPlayStatus[GAME_PLAYER];			//用户状态
    SCORE               				dUserScore[GAME_PLAYER];			//每个玩家当前的金币
    SCORE                   			dTotalJetton;						//当前总注
    WORD								wTimeLeft;							//剩余时间
};

//用户下注
struct CMD_S_AddScore
{
	CMD_S_AddScore()
		:wCurrentUser(0)
		,wJettonCount(0)
		,wAddJettonUser(0)
		,dAddJettonCount(0.0f)
		,dUserTotalJetton(0.0f)
		,dTotalJetton(0.0f)
		,dUserCurrentScore(0.0f)
		,dCurrentJetton(0.0f)
		,dRustJetton(0.0f)
		,wTimeLeft(0)
		,cbState(1)
	{
	}

    DWORD                               wCurrentUser;						//当前用户
    WORD								wJettonCount;						//当前用户下注次数

    DWORD       						wAddJettonUser;						//加注用户
    SCORE           					dAddJettonCount;					//加注数目
    SCORE               				dUserTotalJetton;					//加注用户总注
    SCORE                   			dTotalJetton;						//总注
    SCORE                       		dUserCurrentScore;					//用户当前积分
    SCORE                           	dCurrentJetton;						//当前筹码值
    SCORE       						dRustJetton;						//火拼数目
    WORD								wTimeLeft;							//剩余时间
    BYTE								cbState;							//状态（1:跟注 2 加注）
	
};

//用户弃牌
struct CMD_S_GiveUp
{
    DWORD           					wGiveUpUser;						//放弃用户
    BYTE								cbSwitchUser;						//是否需要切换用户（1：切换 0：不切换）

    DWORD               				wCurrentUser;						//当前用户(如果收到65535表示游戏即将结束)
    WORD								wJettonCount;						//当前用户下注次数
    SCORE                   			dRustJetton;						//火拼数目
    WORD								wTimeLeft;							//剩余时间
};

//比牌数据包
struct CMD_S_sendCompareCardReq
{
	CMD_S_handlesendCompareCardReq()
	{
		memset(this, 0, sizeof(*this));
	}
    DWORD       						wCurrentUser;						//当前用户(如果收到65535表示游戏即将结束)
    DWORD           					wCompareUser[2];					//比牌用户
    DWORD               				wLostUser;							//输牌用户
    WORD                    			wJettonCount;						//当前用户下注次数

    DWORD   							wAddJettonUser;						//加注用户(比牌用户)
    SCORE       						dAddJettonCount;					//加注数目(比牌用户下的注)
    SCORE           					dUserTotalJetton;					//加注用户总注
    SCORE               				dTotalJetton;						//总注
    SCORE                   			dUserCurrentScore;					//用户当前积分
    SCORE                       		dCurrentJetton;						//单注
    SCORE   							dRustJetton;						//火拼数目
    WORD								wTimeLeft;							//剩余时间
};

//看牌数据包
struct CMD_S_LookCard
{
    DWORD   							wLookCardUser;						//看牌用户
    DWORD       						wCurrentUser;						//当前用户
    SCORE           					dCurrentJetton;						//单注
    BYTE								cbTimeLeft;							//剩余时间
    BYTE								cbTotalTime;						//总时间
    WORD								wJettonCount;						//当前用户下注次数
    SCORE           					dRustJetton;						//火拼数目
    BYTE								cbCardType;							//牌型
    BYTE								cbCardData[MAX_COUNT];				//用户扑克
};

//孤注一掷
struct CMD_S_AllIn
{
	CMD_S_AllIn()
	{
		memset(this, 0, sizeof(*this));
	}
    DWORD       						wCurrentUser;						//当前用户
    WORD								wJettonCount;						//当前用户下注次数
    SCORE           					dCurrentJetton;						//当前用户单注

    DWORD               				wAllInUser;							//allIn用户
    SCORE                   			dAllInJettonCount;					//allIn数目
    SCORE       						dAllInUserTotalJetton;				//allIn用户总注
    SCORE           					dTotalJetton;						//总注
    SCORE               				dAllInUserCurrentScore;				//allIn用户金币
    BYTE								cbAllInUserGiveUp;					//用户是否弃牌

    WORD								wTimeLeft;							//剩余时间
};

//孤注一掷比牌结果
struct CMD_S_AllIn_Result
{
    DWORD       						wCurrentUser;						//当前用户(65535表示游戏即将结束)
    WORD								wJettonCount;						//当前用户下注次数
    SCORE           					dCurrentJetton;						//当前用户单注
    SCORE               				dRustJetton;						//火拼数目

    DWORD       						wAllInUser;							//allIn用户
    SCORE           					dAllInJettonCount;					//allIn数目
    SCORE               				dAllInUserTotalJetton;				//allIn用户总注
    SCORE                   			dTotalJetton;						//总注
    SCORE                       		dAllInUserCurrentScore;				//allIn用户金币
    //BYTE								cbAllInUserGiveUp;					//用户是否弃牌

	//比牌结果
    DWORD                       		wCompareUser[GAME_PLAYER];			//比牌用户[不参与比牌用户为65535]
    //DWORD								wStartAllInUser;					//发起AllIn用户
    BYTE								cbStartAllInUserWin;				//发起AllIn用户是否赢（0:输 1:赢）

    WORD								wTimeLeft;							//剩余时间
};


//火拼
struct CMD_S_Rush
{
	CMD_S_Rush()
	{
		memset(this, 0, sizeof(*this));
	}
    DWORD           					wCurrentUser;						//当前用户
    WORD								wJettonCount;						//当前用户下注次数
    SCORE               				dCurrentJetton;						//当前用户单注

    DWORD                   			wRushUser;							//火拼用户
    SCORE                       		dRushJettonCount;					//火拼数目
    SCORE               				dRushUserTotalJetton;				//火拼用户总注
    SCORE                   			dTotalJetton;						//总注
    SCORE                   			dRushUserCurrentScore;				//火拼用户金币
    BYTE								cbRushUserGiveUp;					//用户是否弃牌

    WORD								wTimeLeft;							//剩余时间
};

struct CMD_S_Rush_Result
{
	CMD_S_Rush_Result()
	{
		memset(this, 0, sizeof(*this));
	}

    //DWORD             				wCurrentUser;						//当前用户(65535表示游戏即将结束)
    //WORD                  			wJettonCount;						//当前用户下注次数
    //SCORE                     		dCurrentJetton;						//当前用户单注

    DWORD               				wRushUser;							//火拼用户
    SCORE                   			dRushJettonCount;					//火拼金额
    SCORE               				dRushUserTotalJetton;				//火拼用户总注
    SCORE                   			dTotalJetton;						//总注
    SCORE                       		dRushUserCurrentScore;				//火拼用户金币
    BYTE								cbRushUserGiveUp;					//用户是否弃牌

	//比牌结果
    DWORD       						wCompareUser[GAME_PLAYER];			//参与比牌用户[不参与比牌用户为65535]
    DWORD           					wRushWinUser;						//火拼赢用户
    //DWORD             				wStartRushUser;						//发起火拼用户
    //BYTE								cbStartRushUserWin;					//发起火拼用户是否赢（0:输 1:赢）
};

struct CMD_S_OutcurrentTurn_Result
{
	CMD_S_OutcurrentTurn_Result()
	{
		for (int i = 0; i < GAME_PLAYER; ++i)
		{
			wCompareUser[i] = INVALID_CHAIR;
		}
		wRushWinUser = INVALID_CHAIR;
	}

    DWORD               				wCompareUser[GAME_PLAYER];			//参与比牌用户[不参与比牌用户为65535]
    DWORD                   			wRushWinUser;						//火拼赢用户
};



//开牌数据包
struct CMD_S_OpenCard
{
    DWORD               				wWinner;							//胜利用户
    BYTE								cbCardData[3];						//用户扑克
    BYTE								cbCardType;							//牌型
};

//跟到底
struct CMD_S_Auto_Follow
{
    DWORD               				wAutoFollowUser;					//跟到底用户
    DWORD                   			wCurrentUser;						//当前用户
    WORD								wJettonCount;						//当前用户下注次数
    BYTE								cbRushState;						//是否火拼
    BYTE								cbState;							//状态（0：取消跟到底 1：跟到底）
};

//游戏结束
struct CMD_S_GameEnd
{
    BYTE								cbGameState[GAME_PLAYER];			//游戏状态
    SCORE                   			dGameScore[GAME_PLAYER];			//游戏得分
    SCORE                       		dTotalScore[GAME_PLAYER];			//游戏总分
    BYTE								cbCardData[GAME_PLAYER][3];			//用户扑克
    BYTE								cbCardType[GAME_PLAYER];			//牌型

    BYTE								wCompareUser[GAME_PLAYER];			//比牌用户
    WORD								wTimeLeft;							//等待下一局开始时间
    BYTE								cbEndState;							//结束状态(0表示比牌结束 1表示弃牌结束)
};

//机器人扑克
struct CMD_S_AndroidCard
{
    BYTE								cbRealPlayer[GAME_PLAYER];				//真人玩家
    BYTE								cbAndroidStatus[GAME_PLAYER];			//机器数目
    BYTE								cbAllHandCardData[GAME_PLAYER][MAX_COUNT];//手上扑克
    SCORE								lStockScore;							//当前库存
};

struct CMD_S_PlayerCard
{
    BYTE								cbAllHandCardData[GAME_PLAYER][MAX_COUNT];//手上扑克
};

//客户端命令结构
#define SUB_C_ADD_SCORE					1									//用户加注
#define SUB_C_GIVE_UP					2									//放弃消息
#define SUB_C_COMPARE_CARD				3									//比牌消息
#define SUB_C_LOOK_CARD					4									//看牌消息
#define SUB_C_ALL_IN					5									//孤注一掷
#define SUB_C_OPEN_CARD					6									//开牌消息
#define SUB_C_AUTO_FOLLOW				7									//跟到底
#define SUB_C_CANCEL_AUTO_FOLLOW		8									//取消跟到底
#define SUB_C_RUSH						9									//火拼
#define SUB_C_CHEAT_LOOK_CARD			10									//看牌

// struct
struct MSG_GameMsgUpHead
{
    DWORD dwUserID;
};

//用户加注
struct CMD_C_AddScore : public MSG_GameMsgUpHead
{
    SCORE							dScore;                                 //加注数目
};

//比牌数据包
struct CMD_C_sendCompareCardReq : public MSG_GameMsgUpHead
{
    WORD								wCompareUser;						//比牌用户
};

//看牌数据包
struct CMD_C_CheatLookCard : public MSG_GameMsgUpHead
{
    DWORD							dwSec;								//密钥
};

//////////////////////////////////////////////////////////////////////////////////
*/

struct GameRecordPacket
{
    uint32_t chairId;
    uint32_t wMainCmdID;
    uint32_t wSubCmdID;
    uint32_t wSize;
    void*    pData;
};

struct ChairRecord
{
    uint32_t	chairId;			//椅子号
    uint32_t	dwUserId;		    //用户id
    char        nickName[100];	    //玩家昵称
    uint8_t     mHeaderID;
    uint8_t     mHeadBoxID;
    uint8_t     mVipLevel;
};

struct tagReplayRecordResp
{
    uint32_t		m_UUID;			//回放唯一ID
    uint32_t		m_tableID;		//公开桌子号
    uint32_t		roundStartTime_;	//游戏开始时间
    uint32_t		m_endTime;		//游戏结束时间
    uint32_t		m_UserId;		//用户ID

    ChairRecord     m_chairList[GAME_PLAYER];	//游戏椅子玩家信息
    uint32_t		m_recordCount;		//回放数据记录数
    char            pData[0];	//回放记录列表（尾拖数据）
};

struct RecordDrawInfo
{
    int64_t userScore[GAME_PLAYER];
    uint32_t bankerUserID;
};




#ifdef _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif


#endif		//___MESSAGE_H_DEF___

