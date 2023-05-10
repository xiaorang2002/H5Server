#ifndef		___MESSAGE_DDZ_H_DEF___
#define		___MESSAGE_DDZ_H_DEF___

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "GameLogic.h"
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
#define MAX_JETTON_MULTIPLE         10
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
#define GAME_STATUS_INIT                        97                              //INIT status
#define GAME_STATUS_READY						98								//空闲状态
#define GAME_STATUS_LOCK_PLAYING                99                              //Playing
#define GAME_STATUS_PLAYING                     100                             //Playing
#define GAME_STATUS_END				    	    102								//游戏结束



#define GIVE_UP						2                                       //放弃概率
#define LOOK_CARD					4                       				//看牌概率
#define COMPARE_CARD				5                           			//比牌概率

struct GameRecordPacket
{
    uint32_t wChairID;
    uint32_t wMainCmdID;
    uint32_t wSubCmdID;
    uint32_t wSize;
    void*    pData;
};

struct ChairRecord
{
    uint32_t	wChairId;			//椅子号
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
    uint32_t		m_startTime;	//游戏开始时间
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

