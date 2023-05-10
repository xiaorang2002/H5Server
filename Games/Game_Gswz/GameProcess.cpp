#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string> 
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
//#include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/Gswz.Message.pb.h"

#include "MSG_GSWZ.h"
#include "GameLogic.h"

using namespace Gswz;

#include "GameProcess.h"




//打印日志
#define		PRINT_LOG_INFO_INFO						1
#define		PRINT_LOG_INFO_WARNING					1

////////////////////////////////////////////////////////////////////////


#define TIMER_START_READY               2                           //start to ready.
#define TIME_GAME_READY					2+4							//准备时间（真正准备是5秒,有3秒是客户端飘金币动画时间（不包括比牌时间））
#define TIME_GAME_LOCK_PLAYING          1

#define TIME_GAME_ADD_SCORE				15							//下注时间(second)
#define TIME_AUTO_FOLLOW				1							//自动加注
#define TIME_CLEAR_USER					3							//清理离线或者金币不足用户

#define END_TIME_ALLIN					0							//allin结束(4)
#define END_TIME_RUSH					0							//火拼结束(4)
#define END_TIME_RUSH_GIVEUP			0							//火拼最后一个玩家棋牌结束(2)
#define END_TIME_COMPARE				0							//比牌结束(4)


////////////////////////////////////////////////////////////////////////
//静态变量
//const int  CGameProcess::GAME_PLAYER     = GAME_PLAYER;			//游戏人数
int64_t      CGameProcess::m_lStockScore      = 0;
int64_t      CGameProcess::m_llTodayStorage   = 0;
int64_t      CGameProcess::m_lStockLowLimit   = 0;
int64_t      CGameProcess::m_llTodayHighLimit = 0;

CGameProcess::CGameProcess(void)
{
    //    LOG_INFO(INFO)<< __FILE__ << __FUNCTION__;
    stockWeak = 0.0;
    m_llMaxJettonScore = 0;  //最大下注筹码
    m_wPlayCount = 0;            //当局人数

    m_cbGameStatus = GAME_STATUS_INIT;       //游戏状态
    //    m_nRoundID = 0;
    m_nTableID = 0;

    m_dwStartTime = 0;
    m_HandCardCount = 0;
    m_lMarqueeMinScore = 1000;
    m_dAllInScore = 0;
    m_wCurrentChairID = INVALID_CHAIR;
    m_wWinnerUser = INVALID_CHAIR;

    for(int i = 0; i < GAME_PLAYER; i++)
    {
        m_bPlayStatus[i] = false;
        m_bJoinedGame[i] = false;
        m_wCurrentRoundScore[i]=-1;
        m_cbGiveUp[i] = 0;
        m_cbSystemGiveUp[i] = 0;
        m_dwPlayUserID[i] = -1;
        m_dwPlayFakeUserID[i] = -1;
        m_cbRealPlayer[i] = 0;
        m_cbAndroidStatus[i] = 0;
        m_bWaitEnterTable[i] = false;
        m_llUserSourceScore[i] = 0;
        m_lTableScore[i] = 0;
        m_iUserWinScore[i] = 0;
        m_bCompardChairID[i]=false;
    }

    m_UserInfo.clear();

    m_TurnsRound=0;
    m_lAllScoreInTable = 0;
    m_lCellScore = 0L;
    m_lCurrentJetton = 0L;

    m_dwOpEndTime = 0;
    m_wTotalOpTime = 0;
    m_mOutCardRecord.clear();

    m_bGameEnd = false;

    //扑克变量
    memset(m_cbHandCardData,  0, sizeof(m_cbHandCardData));
    memset(m_cbHandCardType,  0, sizeof(m_cbHandCardType));
    memset(m_cbRepertoryCard,  0, sizeof(m_cbRepertoryCard));
    memset(m_LastAction,  0, sizeof(m_LastAction));

    srand((unsigned)time(NULL));
    m_lCurrStockScore = 0;

    //m_wSystemChangeCardRatio = 620; // 系统换牌率.
    //m_wSystemAllKillRatio = 50;     // 系统通杀率.
	//放大倍数
	int scale = 1000;
	//系统输了换牌概率 80%
	m_wSystemLostChangeCardRatio = 80;
	int lw[MaxExchange] = {
		m_wSystemLostChangeCardRatio*scale,
		(100 - m_wSystemLostChangeCardRatio)*scale };
	m_poolSystemLostChangeCardRatio.init(lw, MaxExchange);
	//系统赢了换牌概率 70%
	m_wSystemWinChangeCardRatio = 70;
	int ww[MaxExchange] = {
		m_wSystemWinChangeCardRatio*scale,
		(100 - m_wSystemWinChangeCardRatio)*scale };
	m_poolSystemWinChangeCardRatio.init(ww, MaxExchange);

    m_bContinueServer = true;
	////////////////////////////////////////
	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//超时匹配时长(可配置)
	timeoutMatchSeconds_ = 0.8f;
	//机器人候补空位超时时长(可配置)
	timeoutAddAndroidSeconds_ = 0.4f;
	//放大倍数
	ratioGamePlayerWeightScale_ = 1000;
	//初始化桌子游戏人数概率权重
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (i == 4) {
			//5人局的概率15%
			ratioGamePlayerWeight_[i] = 15 * ratioGamePlayerWeightScale_;
		}
		else if (i == 3) {
			//4人局的概率35%
			ratioGamePlayerWeight_[i] = 35 * ratioGamePlayerWeightScale_;
		}
		else if (i == 2) {
			//3人局的概率40%
			ratioGamePlayerWeight_[i] = 40 * ratioGamePlayerWeightScale_;
		}
		else if (i == 1) {
			//2人局的概率10%
			ratioGamePlayerWeight_[i] = 10 * ratioGamePlayerWeightScale_;
		}
		else if (i == 0) {
			//单人局的概率0
			ratioGamePlayerWeight_[i] = 0;//5 * ratioGamePlayerWeightScale_;
		}
		else {
			ratioGamePlayerWeight_[i] = 0;
		}
	}
	//计算桌子要求标准游戏人数
	poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
	MaxGamePlayers_ = 0;
    m_IsMatch=false;
}

CGameProcess::~CGameProcess(void)
{

}

//设置指针
bool CGameProcess::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    m_pITableFrame = pTableFrame;
    if(m_pITableFrame)
    {
        m_pGameRoomKind = m_pITableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        m_replay.clear();
        m_replay.roomname = m_pGameRoomKind->roomName;
        m_replay.cellscore = m_pGameRoomKind->floorScore;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        if(m_pGameRoomKind->roomId - m_pGameRoomKind->gameId*10 > 20)
        {
            m_IsMatch= true;
        }

    }
    else
    {
        return false;
    }

    //初始化游戏数据
    InitGameData();

    //读取配置.
    ReadStorageInformation();

    m_nTableID = m_pITableFrame->GetTableId();

    return true;
}


//发送数据
bool CGameProcess::SendTableData(uint32_t chairId, uint8_t subId,const uint8_t* data, int len, bool isrecord)
{
    if(!m_pITableFrame)
    {
        LOG_INFO << "SendTableData: chairId = " << (int)chairId << ", subId ="<< (int)subId << " err";
        return false;
    }

    m_pITableFrame->SendTableData(chairId, subId, data, len, isrecord);

    return true;
}

//清理游戏数据
void CGameProcess::ClearGameData()
{

}

//初始化游戏数据
void CGameProcess::InitGameData()
{

    m_dwStartTime = 0;

    m_wCurrentChairID = INVALID_CHAIR;
    m_wWinnerUser = INVALID_CHAIR;
    m_HandCardCount = 0;
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        m_bPlayStatus[i] = false;
        m_bJoinedGame[i] = false;

        m_cbGiveUp[i] = 0;
        m_cbSystemGiveUp[i] = 0;
        m_wCurrentRoundScore[i]=-1;
        m_dwPlayUserID[i] = -1;
        m_dwPlayFakeUserID[i] = -1;
        m_cbRealPlayer[i] = 0;
        m_cbAndroidStatus[i] = 0;
        m_bWaitEnterTable[i] = false;
        m_llUserSourceScore[i] = 0.0;
        m_lTableScore[i] = 0;
        m_iUserWinScore[i] = 0;
        m_bCompardChairID[i]=false;
    }
    m_UserInfo.clear();
    m_lAllScoreInTable = 0;
    m_TurnsRound =0;
    m_lCellScore = 0L;
    m_lCurrentJetton = 0L;
    m_dAllInScore = 0;
    m_dwOpEndTime = 0;
    m_wTotalOpTime = 0;
    m_bGameEnd = false;
    m_MinAddScore=0;
    //扑克变量
    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_cbHandCardType, 0, sizeof(m_cbHandCardType));
    memset(m_LastAction,  0, sizeof(m_LastAction));

    m_bContinueServer = true;

    if(m_pITableFrame)
    {
        m_lCellScore =m_pGameRoomKind->floorScore;
        m_llMaxJettonScore = MAX_JETTON_MULTIPLE*m_lCellScore;
    }
}


//清除所有定时器
void CGameProcess::ClearAllTimer()
{
    //等待玩家出牌定时器和自动出牌定时器
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameLockPlaying);

    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);

    m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameEnd);
}


bool CGameProcess::OnUserEnter(int64_t userId, bool islookon)
{
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        return false;
    }
    if(m_cbGameStatus !=GAME_STATUS_INIT &&  m_cbGameStatus != GAME_STATUS_READY)
    {
        if( m_dwPlayUserID[pIServerUserItem->GetChairId()] != userId
                || m_cbGameStatus!=GAME_STATUS_PLAYING)
        {
            LOG_INFO<<"---------------------------OnUserEnter------CAN NOT JOIN"<<"---------------------"<<m_cbGameStatus;
            return false;
        }

        m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            if(userInfoItem->bLeave || userInfoItem->userId == userId)
            {
                tagUserInfo uInfo;
                uInfo.account = userInfoItem->nickName;
                uInfo.chairId = userInfoItem->chairId;
                uInfo.headId = userInfoItem->headerId;
                uInfo.location = userInfoItem->location;
                uInfo.nickName = userInfoItem->nickName;
                uInfo.score = pIServerUserItem->GetUserScore() - m_lTableScore[userInfoItem->chairId];
                uInfo.status = 0;
                uInfo.tableId = m_pITableFrame->GetTableId();
                uInfo.userId = userInfoItem->userId;
                m_pITableFrame->SendOtherUserInfoToUser(pIServerUserItem,uInfo);
            }
        }
        OnEventGameScene(pIServerUserItem->GetChairId(),false);
        return true;
    }

	if (m_pITableFrame->GetPlayerCount(true) == 1) {
		////////////////////////////////////////
		//第一个进入房间的也必须是真人
        assert(m_IsMatch||m_pITableFrame->GetPlayerCount(false) == 1);
		////////////////////////////////////////
		//权重随机乱排
		poolGamePlayerRatio_.shuffleSeq();
		////////////////////////////////////////
		//初始化桌子当前要求的游戏人数
		MaxGamePlayers_ = poolGamePlayerRatio_.getResultByWeight() + 1;
		////////////////////////////////////////
		//重置累计时间
		totalMatchSeconds_ = 0;
		printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ tableID[%d]初始化游戏人数 MaxGamePlayers_：%d\n", m_pITableFrame->GetTableId(), MaxGamePlayers_);
	}
	if (m_cbGameStatus < GAME_STATUS_START) {
		assert(MaxGamePlayers_ >= MIN_GAME_PLAYER);
		assert(MaxGamePlayers_ <= GAME_PLAYER);
	}
    uint32_t chairId = INVALID_CHAIR;


    chairId = pIServerUserItem->GetChairId();
    pIServerUserItem->SetUserStatus(sReady);

    if(chairId > 4)
    {
        return false;
    }


    //获取玩家个数
    int wUserCount = 0;
    //用户状态
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        //获取用户
        shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);
        if(pIServerUser)
        {
            ++wUserCount;
        }
    }

    if((m_cbGameStatus == GAME_STATUS_INIT ||m_cbGameStatus == GAME_STATUS_READY) && wUserCount == 2 )
    {
        ClearAllTimer();
        m_cbGameStatus = GAME_STATUS_READY;
        m_dwStartTime = (uint32_t)time(NULL);
        //m_TimerIdGameReadyOver =  m_TimerLoopThread->getLoop()->runAfter(2, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
    }
    else if((m_cbGameStatus == GAME_STATUS_INIT ||m_cbGameStatus == GAME_STATUS_READY) && wUserCount ==GAME_PLAYER)
    {
        //ClearAllTimer();
        m_cbGameStatus = GAME_STATUS_READY;
        m_dwStartTime = (uint32_t)time(NULL);
        //m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(0.1f, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
    }
    else if((m_cbGameStatus == GAME_STATUS_INIT ||m_cbGameStatus == GAME_STATUS_READY) && wUserCount <= 1 )
    {
        //ClearAllTimer();
    }
    if(m_cbGameStatus == GAME_STATUS_LOCK_PLAYING)
    {
        m_bJoinedGame[chairId] = true;
        pIServerUserItem->SetUserStatus(sPlaying);
    }
	if (m_cbGameStatus < GAME_STATUS_START) {
		////////////////////////////////////////
		//达到桌子要求游戏人数，开始游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			////////////////////////////////////////
			//不超过桌子要求最大游戏人数
			assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
			printf("--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetTableId(), m_pITableFrame->GetPlayerCount(true));
			OnTimerGameReadyOver();
		}
		else {
			if (m_pITableFrame->GetPlayerCount(true) == 1) {
				////////////////////////////////////////
				//第一个进入房间的也必须是真人
                assert(m_IsMatch||m_pITableFrame->GetPlayerCount(false) == 1);
				ClearAllTimer();
				////////////////////////////////////////
				//修改游戏准备状态
				//m_pITableFrame->SetGameStatus(GAME_STATUS_READY);
				//m_cbGameStatus = GAME_STATUS_READY;
				m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
				printf("--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d)，等待人数(%d)....................................................................\n", m_pITableFrame->GetTableId(), m_pITableFrame->GetPlayerCount(true), MaxGamePlayers_);
			}
		}
	}
    return true;
}


bool CGameProcess::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    ////////////////////////////////////////
	//初始化或空闲准备阶段，可以进入桌子
	if (m_cbGameStatus < GAME_STATUS_PLAYING)
	{
		////////////////////////////////////////
		//达到游戏人数要求禁入游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			return false;
		}
        if(m_IsMatch) return true;
		////////////////////////////////////////
		//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
        if (pUser->GetUserId() == -1) {
			if (totalMatchSeconds_ < timeoutMatchSeconds_) {
                printf("--- *** tableID[%d]%.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", m_pITableFrame->GetTableId(), timeoutMatchSeconds_, pUser->GetUserId(), totalMatchSeconds_);
				return false;
			}
		}
		else {
			//shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(userId); assert(!userItem);
			////////////////////////////////////////
			//真实玩家防止作弊检查
            printf("\n\n--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]真实玩家防止作弊检查 userId = %d userIp = %d\n", m_pITableFrame->GetTableId(), pUser->GetUserId(), pUser->GetUserBaseInfo().ip);
            for (int i = 0; i < GAME_PLAYER; ++i) {
				if (m_pITableFrame->IsExistUser(i)) {
					if (!m_pITableFrame->IsAndroidUser(i)) {

                        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i);
                        if(pUser->GetUserBaseInfo().userScore>m_lCellScore*m_joinRoomScoreMultipleLimit
                                && userItem->GetUserScore()>m_lCellScore*m_joinRoomScoreMultipleLimit)
                        {
                            return false;
                        }
					}
				}
			}
		}
		return true;
	}
    else if(m_cbGameStatus == GAME_STATUS_PLAYING) {
        ////////////////////////////////////////
		//游戏进行状态，处理断线重连，玩家信息必须存在
        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
		return userItem != NULL;
    }
	////////////////////////////////////////
	//游戏状态为GAME_STATUS_PLAYING(100)时，不会进入该函数
	//assert(false);
	return false;
}

bool CGameProcess::CanLeftTable(int64_t userId)
{
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_INFO << "OnUserLeft of userid:" << userId << ", get chair id failed, pIServerUserItem==NULL";
        return true;
    }
    uint32_t chairId = pIServerUserItem->GetChairId();
    if(m_cbGameStatus != GAME_STATUS_PLAYING || !m_bJoinedGame[chairId] || m_cbGiveUp[chairId] == 1 ||m_bPlayStatus[chairId] == false )
        return true;
    else
        return false;
}

bool CGameProcess::OnUserReady(int64_t userId, bool islookon)
{
    return true;
}


//用户离开
bool CGameProcess::OnUserLeft(int64_t userId, bool bIsLookUser)
{
    bool ret = false;
    // try to get the chair id value.
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_INFO << "OnUserLeft of userid:" << userId << ", get chair id failed, pIServerUserItem==NULL";
        return false;
    }
    int32_t chairId = pIServerUserItem->GetChairId();

    //玩家弃牌或者游戏结束的时候才能离开游戏
    if(m_cbGameStatus != GAME_STATUS_PLAYING || !m_bJoinedGame[chairId] || m_cbGiveUp[chairId] == 1 || m_bPlayStatus[chairId] == false)
    {
        m_bJoinedGame[chairId] = false;
        m_bPlayStatus[chairId] = false;

        pIServerUserItem->SetUserStatus(sOffline);
        m_pITableFrame->ClearTableUser(chairId, true, true);
        ret = true;
        if( m_cbGameStatus == GAME_STATUS_INIT || m_cbGameStatus == GAME_STATUS_READY )
        {
            m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
        }
    }

    if(GAME_STATUS_READY == m_cbGameStatus)
    {
        int userNum=0;
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i]>0*/)
            {
                userNum++;
            }
        }
        if(userNum < 2)
        {
            ClearAllTimer();
            m_cbGameStatus = GAME_STATUS_INIT;
        }
    }
    if(ret)
    {
        auto it = m_UserInfo.find(userId);
        if(it != m_UserInfo.end())
        {
            shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

            userInfo->bLeave = true;
        }
    }
    return ret;
}

void CGameProcess::GameTimerReadyOver() {
	//assert(m_cbGameStatus < GAME_STATUS_START);
	////////////////////////////////////////
	//销毁当前旧定时器
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);
	////////////////////////////////////////
	//计算累计匹配时间，当totalMatchSeconds_ >= timeoutMatchSeconds_时，
	//如果桌子游戏人数不足会自动开启 CanJoinTable 放行机器人进入桌子补齐人数
	totalMatchSeconds_ += sliceMatchSeconds_;
	////////////////////////////////////////
	//达到游戏人数要求开始游戏
	if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
		assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
	}
	else {
		////////////////////////////////////////
		//桌子游戏人数不足，且没有匹配超时，再次启动定时器
		if (totalMatchSeconds_ < timeoutMatchSeconds_) {
			m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
            printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]桌子游戏人数不足，且没有匹配超时，再次启动定时器\n", m_pITableFrame->GetTableId());
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) {
			////////////////////////////////////////
			//桌子游戏人数不足，且机器人候补空位超时
			if (m_pITableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
				////////////////////////////////////////
				//达到最小游戏人数，开始游戏
                printf("--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]达到最小游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetTableId(), m_pITableFrame->GetPlayerCount(true));
				OnTimerGameReadyOver();
			}
			else {
				////////////////////////////////////////
				//仍然没有达到最小游戏人数，继续等待
				m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
                printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]仍然没有达到最小游戏人数(%d)，继续等待...\n", m_pITableFrame->GetTableId(), MIN_GAME_PLAYER);
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			////////////////////////////////////////
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
            printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", m_pITableFrame->GetTableId(), totalMatchSeconds_, MaxGamePlayers_);
			////////////////////////////////////////
			//定时器检测机器人候补空位后是否达到游戏要求人数
			m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
		}
	}
}

//准备定时器
void CGameProcess::OnTimerGameReadyOver()
{
	//assert(m_cbGameStatus < GAME_STATUS_START);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);

    if(!m_IsMatch)
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                if(user->GetUserStatus() == sOffline)
                {
                    m_pITableFrame->ClearTableUser(i, true, true);
                }else if(user->GetUserScore()<m_pGameRoomKind->enterMinScore)
                {
                    user->SetUserStatus(sOffline);
                    m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
                }
            }
        }
    }

    int wUserCount = 0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_pITableFrame->IsExistUser(i))
        {
            ++wUserCount;
        }

    }
    if(wUserCount >= 2)
    {
        //清理游戏数据
        ClearGameData();
        //初始化数据
        InitGameData();

        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sPlaying);
            }
        }
        //        m_cbGameStatus = GAME_STATUS_LOCK_PLAYING;
        //        m_TimerIdGameLockPlaying = m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_LOCK_PLAYING, boost::bind(&CGameProcess::OnTimerLockPlayingOver, this));
        OnTimerLockPlayingOver();
    }else
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sReady);
            }
            m_bJoinedGame[i] = false;
            m_bPlayStatus[i] = false;
        }
        ClearAllTimer();
        m_cbGameStatus = GAME_STATUS_INIT;
    }
}

void CGameProcess::OnTimerLockPlayingOver()
{

    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameLockPlaying);


    m_wPlayCount = 0;
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
        if(pIServerUserItem)
        {

            pIServerUserItem->SetUserStatus(sPlaying);
            m_bJoinedGame[i] = true;
            m_bPlayStatus[i] = true;
            ++m_wPlayCount;

            if(pIServerUserItem->IsAndroidUser())
            {
                m_cbAndroidStatus[i] = 1;
            }else
            {
                m_cbRealPlayer[i] = 1;
            }

            m_llUserSourceScore[i]  = pIServerUserItem->GetUserScore();
            m_dwPlayUserID[i]       = pIServerUserItem->GetUserId();
            m_dwPlayFakeUserID[i]   = pIServerUserItem->GetUserId();
            pIServerUserItem->SetUserStatus(sPlaying);
            m_NoGiveUpTimeout[i]   = 0;


        }else
        {
            m_bJoinedGame[i] = false;
            m_bPlayStatus[i] = false;
        }
    }

    if(m_wPlayCount < 2)
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sReady);
            }
            m_bJoinedGame[i] = false;
            m_bPlayStatus[i] = false;
        }
        ClearAllTimer();
        m_cbGameStatus = GAME_STATUS_INIT;
        return;
    }

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
        if(pIServerUserItem)
        {

            m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
            m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
            m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
        }
    }
    //开始游戏
    //LOG_INFO << "pTableFrame:" << m_pITableFrame << ", OnTimerMessage IDI_GAME_READY, StartGame.";
    m_cbGameStatus = GAME_STATUS_PLAYING;
    m_pITableFrame->SetGameStatus(GAME_STATUS_PLAYING);
    OnGameStart();
}

//游戏开始
void CGameProcess::OnGameStart()
{
    m_UserInfo.clear();
    m_GameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = m_GameIds;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	m_replay.detailsData = "";
    m_startTime = chrono::system_clock::now();

    //清除所有定时器
    ClearAllTimer();
    m_dwStartTime = (uint32_t)time(NULL);
    //读取配置文件
    ReadStorageInformation();
    //读取库存配置
    tagStorageInfo storageInfo = {0};
    if (m_pITableFrame)
    {
        m_pITableFrame->GetStorageScore(storageInfo);
    }
    // initialize the storage value sitem now.
    m_lStockScore = storageInfo.lEndStorage;//系统初始库存
    m_lStockLowLimit = storageInfo.lLowlimit;//系统输了不得低于库存下限，否则玩家吐出来
    m_llTodayHighLimit = storageInfo.lUplimit;//系统赢了不得大于库存上限，否则系统吐出来

    LOG_ERROR << "StockScore:" << m_lStockScore <<",StockHighLimit:"<<m_llTodayHighLimit<<",StockLowLimit:"<<m_lStockLowLimit;


    m_bGameEnd = false;
    m_lCurrentJetton =0;
    m_lastMaxViewCardChairID=0;
    //m_lMaxCellScore = m_wCurJetton[MAX_JETTON_NUM - 1];

    //分发扑克 (洗牌)
    m_GameLogic.RandCardList();

    for(int i = 0 ; i <GAME_PLAYER ; i++)
    {
        if(m_bPlayStatus[i])
        {
            if(m_dAllInScore==0)
            {
                m_dAllInScore=m_llUserSourceScore[i];
            }else if(m_dAllInScore>m_llUserSourceScore[i])
            {
                m_dAllInScore=m_llUserSourceScore[i];
            }
        }
    }

    m_GameLogic.GetHandCards(m_cbHandCardBufer[0],sizeof(m_cbHandCardBufer)/sizeof(m_cbHandCardBufer[0][0]));
    if(m_listTestCardArray.empty())
        readJsonCard();

//    cout<<"m_listTestCardArray size="<<m_listTestCardArray.size()<<endl;
    if(!m_listTestCardArray.empty())
    {
        vector<uint8_t>&cards=m_listTestCardArray.front();
        memcpy(m_cbHandCardBufer[0],&cards[0],cards.size());
        m_listTestCardArray.pop_front();
    }else
        AnalyseStartCard();

    SendBufferCard2HandCard(2);
    //当前用户 max card
    m_wCurrentChairID = GetMaxShowCardChairID();
    assert(m_wCurrentChairID != INVALID_CHAIR);
    m_MinAddScore=m_lCellScore;
    //构造开始数据
    CMD_S_GameStart GameStart;
    GameStart.set_roundid(m_GameIds);
    GameStart.set_dcellscore(m_lCellScore);
    GameStart.set_dcurrentjetton(m_lCurrentJetton);
    GameStart.set_wallinscore(m_dAllInScore);

	m_strHandCardsList = "";
    //传给机器人库存，高出底线多少
    uint64_t current_StockScore=m_lStockScore-m_lStockLowLimit;
    //用户设置
    for(int i = 0; i < GAME_PLAYER; i++)
    {
		//座位有人
        if(m_bPlayStatus[i])
        {
            m_lTableScore[i] = m_lCellScore;
            //LOG_INFO << "tableid:" << m_nTableID << "round" << m_nRoundID << ", OnGameStart, m_lTableScore[" << i << "] = " << m_lTableScore[i];
            m_lAllScoreInTable +=m_lCellScore;

            auto it = m_UserInfo.find(GetUserIdByChairId(i));
            if(m_UserInfo.end() == it)
            {
                shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
                if(pIServerUserItem)
                {
                    shared_ptr<UserInfo> userInfo(new UserInfo());
                    userInfo->userId = GetUserIdByChairId(i);
                    userInfo->chairId = i;
                    userInfo->headerId = pIServerUserItem->GetHeaderId();
                    userInfo->nickName = pIServerUserItem->GetNickName();
                    userInfo->location = pIServerUserItem->GetLocation();
                    userInfo->account = pIServerUserItem->GetAccount();
                    userInfo->agentId = pIServerUserItem->GetUserBaseInfo().agentId;
                    userInfo->initScore =pIServerUserItem->GetUserScore();
                    m_UserInfo[GetUserIdByChairId(i)] = userInfo;
                }
                if(pIServerUserItem->IsAndroidUser())
                {
                    current_StockScore-=m_lCellScore;
                }
                m_replay.addPlayer(pIServerUserItem->GetUserId(),pIServerUserItem->GetAccount(),pIServerUserItem->GetUserScore(),i);


            }
			string strHandCards;
			for (int j = 0; j < MAX_COUNT; ++j) {

				char ch[10] = { 0 };
                sprintf(ch, "0x%02x", m_cbHandCardData[i][j]);
				strHandCards += ch;
			}
			m_strHandCardsList += strHandCards;
            m_replay.addStep(0,to_string(m_lCellScore),-1,opStart,i,i);
		}else{
			//座位没有人
			m_strHandCardsList += "000000";
		}
    }
    int64_t dTotalJetton = 0;
    // loop to check the special player status.
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_bPlayStatus[i])
        {
            dTotalJetton += m_lTableScore[i];
        }
    }

    //给机器人发送所有人的牌数据
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        //机器人数据
        if(!m_pITableFrame->IsExistUser(i))
            continue;

        if(m_pITableFrame->IsAndroidUser(i))
        {
            //机器人数据
            CMD_S_AndroidCard AndroidCard;

            //设置变量
            AndroidCard.set_lstockscore(current_StockScore);

            for(int m = 0; m < GAME_PLAYER; ++m)
            {
                if(!m_bPlayStatus[m]) continue;
                Gswz::AndroidPlayer* p = AndroidCard.add_players();
                p->set_cbchairid(m);
                p->set_cbrealplayer(m_cbRealPlayer[m]==1);
                p->set_cbuserid(GetUserIdByChairId(m));
                for(int j = 0; j < MAX_COUNT; ++j)
                {
                    p->add_cballhandcarddata(m_cbHandCardBufer[m][j]);
                }
            }

            // try to serial the special content as string.
            string content = AndroidCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
        }
    }

    // set the special total jetton score now.
    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    CMD_S_Next_StepInfo* nextStep = GameStart.mutable_nextstep();
    nextStep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));
    nextStep->set_wtimeleft(wTimeLeft);
    nextStep->set_dtotaljetton(m_lAllScoreInTable);
    nextStep->set_wturns(m_TurnsRound);
    nextStep->set_wneedjettonscore(0);
    nextStep->set_wmaxjettonscore(0);
    nextStep->set_wminaddscore(m_MinAddScore);
    m_lastMaxViewCardChairID=m_wCurrentChairID;
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;

    auto fillPlayerItem = [&](int crrentChair)
    {
        GameStart.clear_gameplayers();
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            if(m_bPlayStatus[i])
            {
                Gswz::PlayerItem* pItem=GameStart.add_gameplayers();
                pItem->set_cbchairid(i);
                pItem->set_wuserid(GetUserIdByChairId(i));
                pItem->set_cbplaystatus(m_bPlayStatus[i]?1:0);
                pItem->set_bgiveup(false);
                pItem->set_dtablejetton(m_lTableScore[i]);
                pItem->set_cblastaction(m_LastAction[i]);
                pItem->set_droundjetton(0);
                for(int j=0;j<m_HandCardCount;j++)
                {
                    pItem->add_cbcards((int32_t)m_cbHandCardData[i][j]);
                }
                if(i!=crrentChair)
                    pItem->set_cbcards(0,0);
                pItem->set_duserscore(GetCurrentUserScore(i));
            }
        }
    };
    //发送数据 第一张牌别人是不可见的，发送给自己的是可见的
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_bPlayStatus[i])
        {
            fillPlayerItem(i);
            string content = GameStart.SerializeAsString();
            m_pITableFrame->SendTableData(i, SUB_S_GAME_START, (uint8_t *)content.data(), content.size());
            LOG_INFO<<"Send Start Game------"<<i;
        }
    }


    //玩家下注定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
}

// try to get the special player current score value item.
int64_t CGameProcess::GetCurrentUserScore(int wChairID)
{
    int64_t lCurrentScore = 0;

    // check if the chair id is validate for later uer item now.
    if((wChairID < 0) || (wChairID >= GAME_PLAYER))
        return (lCurrentScore);
    //
    lCurrentScore = m_llUserSourceScore[wChairID] - m_lTableScore[wChairID];
    if(lCurrentScore < 0)
    {
        lCurrentScore = 0;
    }

    return (lCurrentScore);
}

//发送场景
bool CGameProcess::OnEventGameScene(uint32_t chairId, bool bIsLookUser)
{
    LOG_INFO << "OnEventGameScene called, dwChairID = " << chairId << ", nGameStatus:" << m_cbGameStatus;

    //    muduo::MutexLockGuard lock(m_mutexLock);

    if(chairId >= GAME_PLAYER)
    {
        LOG_INFO << "OnEventGameScene: dwChairID = " << (int)chairId << "  err";
        return false;
    }

    switch(m_cbGameStatus)
    {
    case GAME_STATUS_INIT:
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_LOCK_PLAYING:
    {
        //构造数据
        CMD_S_StatusFree StatusFree;
        //设置变量
        StatusFree.set_dcellscore(m_lCellScore);

        //发送场景
        string content = StatusFree.SerializeAsString();
        m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_FREE, (uint8_t *)content.data(), content.size());

        break;
    }
    case GAME_STATUS_PLAYING:	//游戏状态
    {
        m_bWaitEnterTable[chairId] = true;
        //构造数据
        CMD_S_StatusPlay StatusPlay;
        StatusPlay.set_roundid(m_GameIds);
        //加注信息
        StatusPlay.set_dcellscore(m_lCellScore);
        StatusPlay.set_dcurrentjetton((m_lCurrentJetton));
        //设置变量
        // set the special total jetton score now.
        StatusPlay.set_wtimeoutgiveupmask(m_NoGiveUpTimeout[chairId]);
        StatusPlay.set_wallinscore(m_dAllInScore);
        CMD_S_Next_StepInfo* nextStep = StatusPlay.mutable_nextstep();
        nextStep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));
        nextStep->set_wtimeleft(m_dwOpEndTime - (int32_t)time(NULL));
        nextStep->set_dtotaljetton(m_lAllScoreInTable);
        nextStep->set_wturns(m_TurnsRound);
        nextStep->set_wneedjettonscore(
                    m_lCurrentJetton-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID]:0));
        if(m_TurnsRound>2)
            nextStep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
        else
            nextStep->set_wmaxjettonscore(0);

        nextStep->set_wminaddscore(m_MinAddScore);

        //设置扑克

        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            if(m_bPlayStatus[i]||m_cbGiveUp[i]||chairId==i)
            {
                Gswz::PlayerItem* pItem=StatusPlay.add_gameplayers();
                pItem->set_cbchairid(i);
                pItem->set_wuserid(GetUserIdByChairId(i));
                pItem->set_cbplaystatus(m_bPlayStatus[i]?1:0);
                pItem->set_bgiveup(m_cbGiveUp[i]);
                pItem->set_dtablejetton(m_lTableScore[i]);
                pItem->set_duserscore(GetCurrentUserScore(i));
                pItem->set_cblastaction(m_LastAction[i]);
                pItem->set_droundjetton(m_wCurrentRoundScore[i]!=-1?m_wCurrentRoundScore[i]:0);
                for(int j=0;j<m_HandCardCount;j++)
                {
                    pItem->add_cbcards((int32_t)m_cbHandCardData[i][j]);
                }
                if(i!=chairId)
                    pItem->set_cbcards(0,0);
            }
        }
        //发送场景
        string content = StatusPlay.SerializeAsString();
        m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_PLAY, (uint8_t *)content.data(), content.size());
        break;
    }
    case GAME_STATUS_END:
    {
        CMD_S_StatusEnd statusEnd;
        statusEnd.set_wwaittime(m_dwOpEndTime - (uint32_t)time(NULL));
        string content = statusEnd.SerializeAsString();
        m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_END, (uint8_t *)content.data(), content.size());
    }
        break;
    default:
        break;
    }

    return true;
}

void CGameProcess::OnTimerAddScore()
{
    LOG_INFO << " >>> OnTimerAddScore arrived.";
    //现在需求就是直接弃牌
    OnUserGiveUp(m_wCurrentChairID, true);
    return;

    //删除时间
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);

    //当前玩家弃牌
    bool bExit = true;
    if(m_bWaitEnterTable[m_wCurrentChairID])
    {
        m_bWaitEnterTable[m_wCurrentChairID] = false;
        bExit = false;
    }
    if(m_NoGiveUpTimeout[m_wCurrentChairID]!=0)
    {
        if(m_lCurrentJetton==0)
        {
            OnUserPassScore(m_wCurrentChairID);
        }else
        {
            OnUserFollowScore(m_wCurrentChairID);
        }

    }else
    {
        if(m_lCurrentJetton==0)
        {
            OnUserPassScore(m_wCurrentChairID);
        }else
            OnUserGiveUp(m_wCurrentChairID, bExit);
    }
}


//放弃事件
bool CGameProcess::OnUserGiveUp(int wChairID, bool bExit)
{
    if((INVALID_CHAIR == wChairID) || m_bPlayStatus[wChairID] == false || m_cbGameStatus!=GAME_STATUS_PLAYING )//|| (m_wCurrentUser != wChairID))
    {
        return false;
    }

    //设置数据
    m_bPlayStatus[wChairID] = false;
    //谁弃牌，谁设为可离开状态
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    m_replay.addStep((uint32_t)time(NULL) - m_dwStartTime,"",-1,opQuitOrBCall,wChairID,wChairID);
    if(bExit)
    {
        //记录弃牌
        m_cbSystemGiveUp[wChairID] = 1;
        LOG_INFO << "GiveUp, set m_cbSystemGiveUp=1, wChairID:" << wChairID;
    }

    //人数统计
    int wPlayerCount = 0;
    for(int i = 0; i<GAME_PLAYER; ++i)
    {
        if(m_bPlayStatus[i])
        {
            wPlayerCount++;
        }
    }
    m_cbHandCardType[wChairID]=m_GameLogic.GetCardType(m_cbHandCardData[wChairID],m_HandCardCount);
    //记录弃牌
    WriteLoseUserScore(wChairID);
    int wNextPlayerChairID = GetNextPlayer();
    //
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    openLog("------>GameIds:%s,OnUserGiveUp, wChairID:%d,wNextPlayerChairID:%d"
            ,m_GameIds.c_str(),wChairID,wNextPlayerChairID);
    if(wPlayerCount == 1||(wNextPlayerChairID == INVALID_CHAIR&&m_HandCardCount == MAX_COUNT))
    {
        CMD_S_GiveUp GiveUp;
        GiveUp.set_wgiveupuser(GetUserIdByChairId(wChairID));//pIServerUserItem->GetUserId());
        GiveUp.set_biscurrentuser(m_wCurrentChairID == wChairID);
        GiveUp.set_bendgame(true);
        for(int i = 0; i < MAX_COUNT; ++i)
        {
            GiveUp.add_cbcarddata(m_cbHandCardData[wChairID][i]);
        }
        GiveUp.set_cbcardtype(m_cbHandCardType[wChairID]);
        GiveUp.set_wallinscore(m_dAllInScore);
        CMD_S_Next_StepInfo* nextStep = GiveUp.mutable_nextstep();
        nextStep->set_wnextuser(0);
        nextStep->set_wtimeleft(0);
        nextStep->set_wmaxjettonscore(0);
        nextStep->set_dtotaljetton(m_lAllScoreInTable);
        nextStep->set_wturns(0);
        nextStep->set_wneedjettonscore(0);
        nextStep->set_wmaxjettonscore(0);
        nextStep->set_wminaddscore(m_lCellScore);
        string content = GiveUp.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_GIVE_UP, (uint8_t *)content.data(), content.size());

        return HandleMorethanRoundEnd();
    }
    m_dAllInScore=0;
    for(int i = 0 ; i <GAME_PLAYER ; i++)
    {
        if(m_bPlayStatus[i])
        {
            if(m_dAllInScore==0)
            {
                m_dAllInScore=m_llUserSourceScore[i];
            }else if(m_dAllInScore>m_llUserSourceScore[i])
            {
                m_dAllInScore=m_llUserSourceScore[i];
            }
        }
    }
    m_LastAction[wChairID]=SUB_C_GIVE_UP;
    //判断结束
    if(wNextPlayerChairID!=INVALID_CHAIR)
    {
        //发送消息
        CMD_S_GiveUp GiveUp;
        GiveUp.set_wgiveupuser(GetUserIdByChairId(wChairID));//pIServerUserItem->GetUserId());
        GiveUp.set_bendgame(false);
        for(int i = 0; i < MAX_COUNT; ++i)
        {
            GiveUp.add_cbcarddata(m_cbHandCardData[wChairID][i]);
        }

        // try to set the special card tyep item value now.
        GiveUp.set_cbcardtype(m_cbHandCardType[wChairID]);
        GiveUp.set_bendgame(false);
        GiveUp.set_biscurrentuser(true);
        GiveUp.set_wallinscore(m_dAllInScore);

        LOG_INFO << "OnUseGivenUp, m_wCurrentUser:" << m_wCurrentChairID << ", 3wNextPlayer:" << wNextPlayerChairID;
        m_wCurrentChairID = wNextPlayerChairID;

        CMD_S_Next_StepInfo* nextStep = GiveUp.mutable_nextstep();
        nextStep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));//m_pITableFrame->GetTableUserItem(m_wCurrentUser)->GetUserId());

        uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
        nextStep->set_wtimeleft(wTimeLeft);
        m_dwOpEndTime = (uint32_t)time(NULL) + wTimeLeft;
        m_wTotalOpTime = wTimeLeft;
        nextStep->set_dtotaljetton(m_lAllScoreInTable);
        nextStep->set_wturns(m_TurnsRound);
        nextStep->set_wneedjettonscore(m_wCurrentRoundScore[wChairID]-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID] : 0));
        nextStep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
        nextStep->set_wminaddscore(m_MinAddScore);

        if(m_NoGiveUpTimeout[m_wCurrentChairID]==2) //auto jetton after seconds
            wTimeLeft=1;
        //玩家下注定时器
        m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
        LOG_INFO << " >>> OnUserGiveUp, delay:" << (wTimeLeft+1);

        string content = GiveUp.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_GIVE_UP, (uint8_t *)content.data(), content.size());
    }else
    {
        CMD_S_GiveUp GiveUp;
        GiveUp.set_wgiveupuser(GetUserIdByChairId(wChairID));//pIServerUserItem->GetUserId());
        GiveUp.set_biscurrentuser(m_wCurrentChairID == wChairID);
        GiveUp.set_bendgame(false);
        for(int i = 0; i < MAX_COUNT; ++i)
        {
            GiveUp.add_cbcarddata(m_cbHandCardData[wChairID][i]);
        }
        GiveUp.set_cbcardtype(m_cbHandCardType[wChairID]);
        GiveUp.set_wallinscore(m_dAllInScore);

        CMD_S_Next_StepInfo* nextStep = GiveUp.mutable_nextstep();
        nextStep->set_wnextuser(0);
        nextStep->set_wtimeleft(0);
        nextStep->set_dtotaljetton(m_lAllScoreInTable);
        nextStep->set_wturns(0);
        nextStep->set_wneedjettonscore(0);
        nextStep->set_wmaxjettonscore(0);
        nextStep->set_wminaddscore(m_lCellScore);

        string content = GiveUp.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_GIVE_UP, (uint8_t *)content.data(), content.size());

        SendBufferCard2HandCard(1);
        SendCard();
    }
    return true;
}


bool CGameProcess::HandleMorethanRoundEnd()
{
    int32_t winChairID=INVALID_CHAIR;
    for(int i = 0; i <GAME_PLAYER; i++)
    {
        //用户过滤
        if(!m_bPlayStatus[i])
            continue;
        //设置用户
        if (winChairID == INVALID_CHAIR)
        {
            winChairID = i;
            continue;
        }

        m_bCompardChairID[i]=true;
        m_bCompardChairID[winChairID]=true;
        //对比扑克
        if (m_GameLogic.CompareCard(m_cbHandCardData[i], m_cbHandCardData[winChairID], m_HandCardCount) >= 1)
        {
            winChairID = i;
        }
    }

    //找到最游戏
    m_wCurrentChairID = winChairID;
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(!m_bPlayStatus[i] || winChairID == i)
            continue;

        //设置数据
        WriteLoseUserScore(i);
    }

    OnEventGameConclude(INVALID_CHAIR, GER_COMPARECARD);
    return true;
}

//游戏消息
bool CGameProcess::OnGameMessage(uint32_t wChairID, uint8_t dwSubCmdID, const uint8_t* pDataBuffer, uint32_t dwDataSize)
{

    //    CTestDump dump("OnGameMessage", wChairID, dwSubCmdID);
    //    LOG_INFO << "OnGameMessage, wChairID:" << wChairID << ", dwSubCmdID:" << (int)dwSubCmdID;
    try
    {
        switch (dwSubCmdID)
        {
            case SUB_C_GIVE_UP:			//用户放弃
            {
                //状态判断
                if(!m_bPlayStatus[wChairID])
                    return false;
                if(m_cbGameStatus != GAME_STATUS_PLAYING)
                    return false;
                //no current user cannot give up ,it different zjh
                if(wChairID != m_wCurrentChairID)
                {
                    LOG_INFO << ">>> SUB_C_GIVE_UP wChairID != m_wCurrentUser wChairID=" << wChairID<<"CurrentUser="<<m_wCurrentChairID;
                    return false;
                }
                //消息处理
                return OnUserGiveUp(wChairID,true);
            }
            case SUB_C_PASS_SCORE:		//不加事件
            {
                //状态判断
                if(!m_bPlayStatus[wChairID])
                    return false;

                if(m_cbGameStatus != GAME_STATUS_PLAYING)
                    return false;
                //参数效验
                if(wChairID != m_wCurrentChairID)
                {
                    LOG_INFO << ">>> SUB_C_PASS_SCORE wChairID != m_wCurrentUser wChairID=" << wChairID<<"CurrentUser="<<m_wCurrentChairID;
                    return false;
                }
                //消息处理
                return OnUserPassScore(wChairID);

            }
            case SUB_C_FOLLOW_SCORE:	//用户跟注
            {
                if(m_cbGameStatus != GAME_STATUS_PLAYING)
                    return false;

                if(!m_bPlayStatus[wChairID])
                    return false;

                //参数效验
                if(wChairID != m_wCurrentChairID)
                {
                    LOG_INFO << ">>> SUB_C_FOLLOW_SCORE wChairID != m_wCurrentUser wChairID=" << wChairID<<"CurrentUser="<<m_wCurrentChairID;
                    return false;
                }

                //消息处理
              return OnUserFollowScore(wChairID);//wCompareUser);
            }
            case SUB_C_ADD_SCORE:		//用户加注
            {
                //效验数据
                if(m_cbGameStatus != GAME_STATUS_PLAYING)
                    return false;
                if(!m_bPlayStatus[wChairID])
                    return false;
                //变量定义
                CMD_C_AddScore AddScore;
                if(!AddScore.ParseFromArray(pDataBuffer, dwDataSize))
                {
                    return false;
                }

                //参数效验
                if(wChairID != m_wCurrentChairID)
                {
                    return false;
                }

                //状态判断
                if(!m_bPlayStatus[wChairID])
                    return false;

                //当前状态
                int64_t dScore = AddScore.dscore();
                //if (pAddScore->wState > 0)m_lCompareCount = pAddScore->lScore;
                int64_t llAddScore = (int64_t)(dScore);

                //消息处理
                bool isSucc = OnUserAddScore(wChairID, llAddScore);
                if(!isSucc)
                {
                    LOG_INFO << " >>> OnTimerAddScore, dScore:" << dScore;
                }
                return isSucc;
            }
            case SUB_C_ALL_IN:
            {
                //状态判断
                if(m_cbGameStatus != GAME_STATUS_PLAYING)
                    return false;
                if(!m_bPlayStatus[wChairID])
                    return false;
                //参数效验
                if(wChairID != m_wCurrentChairID)
                {
                    return false;
                }

                //消息处理
                return OnUserAllIn(wChairID);
            }
            case SUB_C_GIVEUP_TIMEOUT_OP:
            {
                CMD_C_GIVEUP_TIMEOUT_OP TimeoutSetting;
                if(!TimeoutSetting.ParseFromArray(pDataBuffer, dwDataSize))
                {
                    return false;
                }
                return OnUserTimeoutGiveUpSetting(wChairID,TimeoutSetting.mask());
            }
            case SUB_C_LOOK_CARD:
            {
                CMD_S_User_LookCard LookCard;
                LookCard.set_dchairid(wChairID);
                string content = LookCard.SerializeAsString();
                m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_USER_LOOK_CARD, (uint8_t *)content.data(), content.size());
                return true;
            }
        }
    }catch(exception & exp)
    {
        openLog("err dwSubCmdID:%d",dwSubCmdID);
    }

    return false;
}

//不加事件
bool CGameProcess::OnUserPassScore(int wChairID)
{
    //上一家有加注或者跟注，不可以不加
    if(m_lCurrentJetton==0)
        m_wCurrentRoundScore[wChairID]=0;
    else
    {
        LOG_INFO << " >>> UserPassScore err, m_lCurrentJetton:" <<m_lCurrentJetton;
        string str="有人加注，不可过牌";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_LastAction[wChairID]=SUB_C_PASS_SCORE;

    m_replay.addStep((int32_t)time(NULL)-m_dwStartTime,"",-1,opPass,wChairID,wChairID);

    //用户切换
    int32_t wNextPlayerChairID = GetNextPlayer();
    //构造数据
    CMD_S_PassScore passScore;
    passScore.set_wpassuser(GetUserIdByChairId(wChairID));

    openLog("------>GameIds:%s,OnUserPassScore, wChairID:%d,wNextPlayerChairID:%d"
            ,m_GameIds.c_str(),wChairID,wNextPlayerChairID);
    //start new round
    if (wNextPlayerChairID==INVALID_CHAIR) //all pass
    {
        CMD_S_Next_StepInfo*  nextstep =  passScore.mutable_nextstep();
        nextstep->set_wnextuser(0);
        nextstep->set_wtimeleft(0);
        nextstep->set_dtotaljetton(m_lAllScoreInTable);
        nextstep->set_wturns(m_TurnsRound);
        nextstep->set_wneedjettonscore(0);
        nextstep->set_wmaxjettonscore(0);
        nextstep->set_wminaddscore(m_lCellScore);

        string content = passScore.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_PASS_SCORE, (uint8_t *)content.data(), content.size());

        if(m_HandCardCount == 5) //arrive the last round ,game over soon
        {
            return HandleMorethanRoundEnd();
        }else
        {
            SendBufferCard2HandCard(1);
            SendCard();
        }

        return true;
    }

    m_wCurrentChairID = wNextPlayerChairID;

    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;

    CMD_S_Next_StepInfo*  nextStep =  passScore.mutable_nextstep();
    nextStep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));

    nextStep->set_wtimeleft(wTimeLeft);
    nextStep->set_dtotaljetton(m_lAllScoreInTable);
    nextStep->set_wturns(m_TurnsRound);
    nextStep->set_wneedjettonscore(m_wCurrentRoundScore[wChairID]-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID] : 0));
    if(m_TurnsRound>2)
        nextStep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
    else
        nextStep->set_wmaxjettonscore(0);
    nextStep->set_wminaddscore(m_MinAddScore);

    string content = passScore.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_PASS_SCORE, (uint8_t *)content.data(), content.size());

    if(m_NoGiveUpTimeout[m_wCurrentChairID]==2) //auto jetton after seconds
        wTimeLeft=1;
    //玩家下注定时器
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
    LOG_INFO << " >>> OnUserPassScore, delay:" << (wTimeLeft+2);


    return true;
}


//跟注事件
bool CGameProcess::OnUserFollowScore(int wChairID )
{
    assert(m_wCurrentRoundScore[wChairID] < m_lCurrentJetton);
    int64_t llNeedScore = m_lCurrentJetton-(m_wCurrentRoundScore[wChairID]==-1?0:m_wCurrentRoundScore[wChairID]);
    if(llNeedScore<1)
    {
        LOG_INFO << " >>>>>>>>>> OnUserFollowScore llNeedScore==0";
        string str="跟注金额错误";
        SendServerNotify2Client(wChairID,str);
        return false;
    }

    if (m_wCurrentRoundScore[wChairID]==-1)
        m_wCurrentRoundScore[wChairID]=0;

    if(llNeedScore==0)
    {
        LOG(ERROR) << " >>>>>>>>>> OnUserFollowScore llNeedScore==0";
    }


    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(pIServerUserItem)
    {
        int64_t llUserScore = GetCurrentUserScore(wChairID);
        if(llNeedScore > llUserScore)
        {
            LOG_INFO << " >>> no enough score llUserScore="<<llUserScore<<",llNeedScore ="<<llNeedScore;
            return false;
        }
    }
    m_LastAction[wChairID]=SUB_C_FOLLOW_SCORE;
    //stop timer first
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);

    m_lAllScoreInTable +=llNeedScore;
    m_lTableScore[wChairID] +=llNeedScore;
    m_wCurrentRoundScore[wChairID]+=llNeedScore;

    m_replay.addStep((int32_t)time(NULL)-m_dwStartTime,to_string(llNeedScore),-1,opFollow,wChairID,wChairID);

    int32_t wNextPlayerChairID = GetNextPlayer();
    LOG_INFO << "------>tableid:" << m_nTableID << /*",round" << m_nRoundID <<*/ ", OnUserFollowScore, wChairID:" << wChairID
               << ", m_lTableScore[" << wChairID << "]=" << m_lTableScore[wChairID]
                  << ", llNeedScore:" << llNeedScore<<"wNextPlayerChairID:"<<wNextPlayerChairID;

    openLog("------>GameIds:%s,OnUserFollowScore, wChairID:%d,llNeedScore:%d,wNextPlayerChairID:%d"
            ,m_GameIds.c_str(),wChairID,llNeedScore,wNextPlayerChairID);
    m_wCurrentChairID=wNextPlayerChairID;
    CMD_S_FollowScore followScore;
    followScore.set_wfollowuser(GetUserIdByChairId(wChairID));
    followScore.set_wfollowscore(llNeedScore);
    //start new round
    if (wNextPlayerChairID==INVALID_CHAIR) //all pass
    {
        CMD_S_Next_StepInfo*  nextstep =  followScore.mutable_nextstep();
        nextstep->set_wnextuser(0);
        nextstep->set_wtimeleft(0);
        nextstep->set_dtotaljetton(m_lAllScoreInTable);
        nextstep->set_wturns(m_TurnsRound);
        nextstep->set_wneedjettonscore(0);
        nextstep->set_wmaxjettonscore(0);
        nextstep->set_wminaddscore(m_lCellScore);

        string content = followScore.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_FOLLOW_SCORE, (uint8_t *)content.data(), content.size());

        if(m_HandCardCount == 5) //arrive the last round ,game over soon
        {
            return HandleMorethanRoundEnd();
        }else
        {

            SendBufferCard2HandCard(1);
            SendCard();
        }

        return true;
    }

    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;
    CMD_S_Next_StepInfo*  nextstep =  followScore.mutable_nextstep();
    nextstep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));
    nextstep->set_wtimeleft(wTimeLeft);
    nextstep->set_dtotaljetton(m_lAllScoreInTable);
    nextstep->set_wturns(m_TurnsRound);
    nextstep->set_wneedjettonscore(m_wCurrentRoundScore[wChairID]-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID] : 0));
    nextstep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
    nextstep->set_wminaddscore(m_MinAddScore);
    string content = followScore.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_FOLLOW_SCORE, (uint8_t *)content.data(), content.size());

    if(m_NoGiveUpTimeout[m_wCurrentChairID]==2) //auto jetton after seconds
        wTimeLeft=1;
    //玩家下注定时器
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
    LOG_INFO << " >>> OnUserFollowScore, delay:" << (wTimeLeft+2);

    return true;
}

//加注事件
bool CGameProcess::OnUserAddScore(int wChairID, int64_t lScore)
{
    //状态效验
    if(m_wCurrentChairID != wChairID)
    {
        LOG_INFO << " >>> OnTimerAddScore, m_wCurrentUser:" << m_wCurrentChairID<<",wChairID"<<wChairID;
        return false;
    }
    if(m_MinAddScore==0)
    {
        LOG_INFO << " >>> OnUserAddScore, m_MinAddScore :" << m_MinAddScore;
        string str="只能梭哈或跟牌";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    int64_t llCurrentJetton = lScore ;
    bool bInJetton = false;
    if(llCurrentJetton <m_pITableFrame->GetGameRoomInfo()->jettons[0] &&llCurrentJetton>m_pITableFrame->GetGameRoomInfo()->jettons[0])
    {
        LOG_INFO << " >>> OnUserAddScore, out of jettons :" << llCurrentJetton;
        string str="超过下注金额限制";
        SendServerNotify2Client(wChairID,str);
        return false;
    }


    if(llCurrentJetton  < m_MinAddScore)
    {
        LOG_INFO << " >>> OnUserAddScore, llCurrentJetton  <= llCurrentJetton:" << llCurrentJetton<<",m_MinAddScore"<<m_MinAddScore;
        string str="加注金额不足，请核对";
        SendServerNotify2Client(wChairID,str);
        return false;
    }


    m_LastAction[wChairID]=SUB_C_ADD_SCORE;
    int64_t lUserScore = GetCurrentUserScore(wChairID);
    //加注金币效验
    if (lScore < 0)
    {
        // try to add the special score content item value data for later user now.
        LOG_INFO << "add score: " << lScore << ", user score: " << lUserScore
                   << ", current jetton: "    << m_lCurrentJetton;

        string str="加注金额不正确";
        SendServerNotify2Client(wChairID,str);
        return false;
    }

    if(lUserScore < lScore)
    {
        LOG_INFO << "add score: " << lScore << ", but user score : " << lUserScore
                   << ", is not enough.";

        string str="金币不足";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    if(lScore>GetCanAddMaxScore(wChairID))
    {
        LOG_INFO << "add score more than CanAddMaxScore score:" <<lScore<<" CanAddMaxScore:"<<GetCanAddMaxScore(wChairID);
        string str="超过加注注额";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    if(m_wCurrentRoundScore[wChairID]==-1)
        m_wCurrentRoundScore[wChairID]=0;
    if(m_wCurrentRoundScore[wChairID]==-1)
        m_wCurrentRoundScore[wChairID]=0;
    //用户注金
    m_lTableScore[wChairID] += lScore;
    m_lAllScoreInTable +=lScore;
    m_wCurrentRoundScore[wChairID]+=lScore;
//    if(m_lCurrentJetton<m_wCurrentRoundScore[wChairID])
//    {
//        m_lCurrentJetton+=lScore;
//    }

    m_lCurrentJetton=m_wCurrentRoundScore[wChairID];

    m_replay.addStep((int32_t)time(NULL)-m_dwStartTime,to_string(lScore),-1,opAddBet,wChairID,wChairID);
    int32_t wNextPlayerChairID = GetNextPlayer();
    LOG_INFO << "------>tableid:" << m_nTableID << /*",round" << m_nRoundID <<*/ ", OnUserAddScore, wChairID:" << wChairID
               << ", m_lTableScore[" << wChairID << "]=" << m_lTableScore[wChairID]
                  << ", lScore:" << lScore<<"wNextPlayerChairID:"<<wNextPlayerChairID;

    openLog("------>GameIds:%s,OnUserAddScore, wChairID:%d,llCurrentJetton:%d,wNextPlayerChairID:%d"
            ,m_GameIds.c_str(),wChairID,m_lCurrentJetton,wNextPlayerChairID);
    //用户切换
    assert(wNextPlayerChairID !=INVALID_CHAIR && wNextPlayerChairID!=wChairID);

    m_wCurrentChairID = wNextPlayerChairID;
    //lScore+
    m_MinAddScore=m_lCellScore+(m_wCurrentRoundScore[wChairID]-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID] : 0));
    //构造数据
    CMD_S_AddScore AddScore;
    AddScore.set_cbstate(2);
    AddScore.set_wopuserid(GetUserIdByChairId(wChairID));//m_pITableFrame->GetTableUserItem(wChairID)->GetUserId());
    AddScore.set_wjetteonscore(lScore);
    AddScore.set_walljetton(m_lTableScore[wChairID]);

    int64_t dUserCurrentScore = GetCurrentUserScore(wChairID);
    AddScore.set_wuserscore(dUserCurrentScore);

    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;

    CMD_S_Next_StepInfo*  nextstep =  AddScore.mutable_nextstep();
    nextstep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));//m_pITableFrame->GetTableUserItem(m_wCurrentUser)->GetUserId());

    nextstep->set_wtimeleft(wTimeLeft);
    nextstep->set_dtotaljetton(m_lAllScoreInTable);
    nextstep->set_wturns(m_TurnsRound);

    nextstep->set_wneedjettonscore(m_MinAddScore-m_lCellScore);
    if(m_TurnsRound>2)
        nextstep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
    else
        nextstep->set_wmaxjettonscore(0);

    if(m_wCurrentRoundScore[m_wCurrentChairID]!=-1)
        nextstep->set_wminaddscore(0);
    else
        nextstep->set_wminaddscore(m_MinAddScore);

    string content = AddScore.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ADD_SCORE, (uint8_t *)content.data(), content.size());

    if(m_NoGiveUpTimeout[m_wCurrentChairID]==2) //auto jetton after seconds
        wTimeLeft=1;
    //玩家下注定时器
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
    LOG_INFO << " >>> OnTimerAddScore, delay:" << (wTimeLeft+2);

    return true;
}

//梭哈
bool CGameProcess::OnUserAllIn(int wChairID)
{
    //是否还有游戏状态
    if(!m_bPlayStatus[wChairID])
    {
        return false;
    }
    if(m_HandCardCount<4&&m_bLimitAllIn)
    {
        LOG_INFO<< "<<< OnUserAllIn    m_HandCardCount == "<<m_HandCardCount<<" cannot < 4";
        string str="小于4轮不可梭哈";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    uint64_t llCurrentJetton =GetCanAddMaxScore(wChairID);
    if(llCurrentJetton == 0)
    {
        LOG_INFO<< "<<< OnUserAllIn    llCurrentJetton == 0";
        string str="梭哈失败";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    //判断发起allIn的人是否钱够单注
    int64_t llUserScore = GetCurrentUserScore(wChairID);
    //够钱下注不能allIn
    if(llUserScore < llCurrentJetton)
    {
        LOG_INFO<< "<<< llUserScore < llCurrentJetton llUserScore="<<llUserScore <<", llCurrentJetton"<<llCurrentJetton;
        string str="梭哈失败,金额不足";
        SendServerNotify2Client(wChairID,str);
        return false;
    }
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    if(m_wCurrentRoundScore[wChairID]==-1)
        m_wCurrentRoundScore[wChairID]=0;
    //压注
    m_lTableScore[m_wCurrentChairID] += llCurrentJetton;
    m_lAllScoreInTable +=llCurrentJetton;
    m_wCurrentRoundScore[wChairID]+=llCurrentJetton;
    m_lCurrentJetton=m_wCurrentRoundScore[wChairID];

    m_replay.addStep((int32_t)time(NULL)-m_dwStartTime,to_string(llCurrentJetton),-1,opAddBet,wChairID,wChairID);

    m_LastAction[m_wCurrentChairID]=SUB_C_ALL_IN;
    CMD_S_AllIn allInResult;
    allInResult.set_wallinuser(GetUserIdByChairId(m_wCurrentChairID));//pIServerUserItem->GetUserId());
    allInResult.set_dallinscore(llCurrentJetton);
    //用户切换
    int32_t wNextPlayerChairID = GetNextPlayer();

    LOG_INFO << "------>tableid:" << m_nTableID << /*",round" << m_nRoundID <<*/ ", OnUserAllIn, wChairID:" << wChairID
               << ", m_lTableScore[" << wChairID << "]=" << m_lTableScore[wChairID]
                  << ", llCurrentJetton:" << llCurrentJetton<<"wNextPlayerChairID:"<<wNextPlayerChairID;

    openLog("------>GameIds:%s,OnUserAllIn, wChairID:%d,llCurrentJetton:%d,wNextPlayerChairID:%d"
            ,m_GameIds.c_str(),wChairID,llCurrentJetton,wNextPlayerChairID);
    if (wNextPlayerChairID==INVALID_CHAIR) //all pass
    {
        CMD_S_Next_StepInfo*  nextstep =  allInResult.mutable_nextstep();
        nextstep->set_wnextuser(0);
        nextstep->set_wtimeleft(0);
        nextstep->set_dtotaljetton(m_lAllScoreInTable);
        nextstep->set_wturns(m_TurnsRound);
        nextstep->set_wneedjettonscore(0);
        nextstep->set_wmaxjettonscore(0);
        nextstep->set_wminaddscore(m_lCellScore);

        string content = allInResult.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ALL_IN, (uint8_t *)content.data(), content.size());

        if(m_HandCardCount == 5) //arrive the last round ,game over soon
        {
            return HandleMorethanRoundEnd();
        }else
        {
            SendBufferCard2HandCard(1);
            SendCard();
        }

        return true;
    }
    assert(wNextPlayerChairID !=INVALID_CHAIR);

    m_wCurrentChairID=wNextPlayerChairID;
    m_MinAddScore=0;
    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;

    CMD_S_Next_StepInfo*  nextstep =  allInResult.mutable_nextstep();
    nextstep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));//m_pITableFrame->GetTableUserItem(m_wCurrentUser)->GetUserId());

    nextstep->set_wtimeleft(wTimeLeft);
    nextstep->set_dtotaljetton(m_lAllScoreInTable);
    nextstep->set_wturns(m_TurnsRound);
    nextstep->set_wneedjettonscore(m_wCurrentRoundScore[wChairID]-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID] : 0));
    nextstep->set_wmaxjettonscore(m_wCurrentRoundScore[wChairID]-(m_wCurrentRoundScore[m_wCurrentChairID]>0?m_wCurrentRoundScore[m_wCurrentChairID] : 0));
    nextstep->set_wminaddscore(m_MinAddScore);

    string content = allInResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ALL_IN, (uint8_t *)content.data(), content.size());

    if(m_NoGiveUpTimeout[m_wCurrentChairID]==2) //auto jetton after seconds
        wTimeLeft=1;

    //玩家下注定时器
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
    LOG_INFO << " >>> OnUserAllIn, delay:" << (wTimeLeft+2);

    return true;
}


//游戏结束
bool CGameProcess::OnEventGameConclude(uint32_t dwChairID, uint8_t GETag)
{
    //结束时清理所有定时器
    ClearAllTimer();
    //定义变量
    CMD_S_GameEnd GameEnd;

    //唯一玩家
    int wWinner = 0;
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_bPlayStatus[i])
        {
            wWinner = i;

        }
    }

    // update the winner now.
    m_wWinnerUser = wWinner;
    GameEnd.set_dwinneruserid(GetUserIdByChairId(wWinner));//m_pITableFrame->GetTableUserItem(wWinner)->GetUserId());
    //计算总注
    int64_t lWinnerScore = 0L;
    int64_t lGameScore[GAME_PLAYER] = {0};
    memset(m_iUserWinScore,0,sizeof(m_iUserWinScore));


    // loop to initialize the win lost score.
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if (i == wWinner)
            continue;

        lGameScore[i] = -m_lTableScore[i];
        m_iUserWinScore[i] = lGameScore[i];
        lWinnerScore +=  m_lTableScore[i];
    }


    lGameScore[wWinner] = lWinnerScore;
    m_iUserWinScore[wWinner] = lGameScore[wWinner];
    int64_t Revenue = m_pITableFrame->CalculateRevenue(lWinnerScore);
    if(((lWinnerScore-Revenue) >= m_pITableFrame->GetGameRoomInfo()->broadcastScore))
    {
        int msgType = SMT_GLOBAL|SMT_SCROLL;
        LOG_INFO  << " >>> marquee SendGameMessage, wWinnerChairID:" << wWinner << ", lWinnerScore:" << lWinnerScore ;
        m_pITableFrame->SendGameMessage(wWinner,"",msgType,(lWinnerScore-Revenue));
    }

    m_strHandCardsList.clear();
    string strHandCards;
    for (int i = 0; i <GAME_PLAYER ; ++i)
    {
        for (int j = 0; j < MAX_COUNT; ++j)
        {
            char ch[10] = { 0 };
            sprintf(ch, "%02X", m_cbHandCardData[i][j]);
            strHandCards += ch;
        }
    }
    m_strHandCardsList+=strHandCards;
    char ch[10] = { 0 };
    sprintf(ch, "%02d", m_wWinnerUser);
    m_strHandCardsList += ch;

//    m_replay.addStep((uint32_t)time(NULL)-m_dwStartTime,m_strHandCardsList,-1,opShowCard,-1,-1);

    int64_t tax =  m_pITableFrame->CalculateRevenue(lWinnerScore);

    uint8_t winnerchairId[1];
    winnerchairId[0]=wWinner;
    tagSpecialScoreInfo scoreInfo;
    if (m_IsMatch||!m_pITableFrame->IsAndroidUser(wWinner))///
	{
		shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wWinner);
		scoreInfo.userId = pIServerUserItem->GetUserId();
		scoreInfo.beforeScore = pIServerUserItem->GetUserScore();

		scoreInfo.chairId = wWinner;
		shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(scoreInfo.chairId);
		scoreInfo.userId = userItem->GetUserId();
		scoreInfo.account = userItem->GetAccount();
		scoreInfo.agentId = userItem->GetUserBaseInfo().agentId;
		scoreInfo.addScore = lWinnerScore - tax;
		scoreInfo.revenue = tax;
		scoreInfo.betScore = m_iUserWinScore[wWinner];
        scoreInfo.rWinScore = m_iUserWinScore[wWinner];

		scoreInfo.cellScore.push_back(m_lTableScore[wWinner]);

		scoreInfo.cardValue = m_strHandCardsList;//GlobalFunc::converToHex((uint8_t *)m_cbHandCardData, GAME_PLAYER*MAX_COUNT) + GlobalFunc::converToHex((uint8_t *)winnerchairId, 1);
		scoreInfo.startTime = m_startTime;
		scoreInfo.bWriteScore = true;           // 写分
		scoreInfo.bWriteRecord = true;          // 写记录
		m_pITableFrame->WriteSpecialUserScore(&scoreInfo, 1, m_GameIds);
	}


    vector<tagSpecialScoreInfo> scoreInfoVec;
    tagSpecialScoreInfo __scoreInfo;
    shared_ptr<UserInfo> userInfoItem;

    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        if(userInfoItem->chairId ==wWinner) continue;
        
        if (m_IsMatch||!m_pITableFrame->IsAndroidUser(userInfoItem->chairId)) {

			__scoreInfo.clear();

			__scoreInfo.beforeScore = userInfoItem->initScore;
			__scoreInfo.agentId = userInfoItem->agentId;
			__scoreInfo.account = userInfoItem->account;
			__scoreInfo.userId = userInfoItem->userId;
			__scoreInfo.chairId = userInfoItem->chairId;
			__scoreInfo.addScore = -m_lTableScore[userInfoItem->chairId];
			__scoreInfo.revenue = 0;
			__scoreInfo.betScore = m_lTableScore[userInfoItem->chairId];
            __scoreInfo.rWinScore = m_lTableScore[userInfoItem->chairId];


			__scoreInfo.cellScore.push_back(m_lTableScore[userInfoItem->chairId]);

			__scoreInfo.cardValue = m_strHandCardsList;//GlobalFunc::converToHex((uint8_t *)m_cbHandCardData, GAME_PLAYER*MAX_COUNT) + GlobalFunc::converToHex((uint8_t *)winnerchairId,1);
			__scoreInfo.startTime = m_startTime;
			__scoreInfo.bWriteScore = false;           // 写分
			__scoreInfo.bWriteRecord = true;          // 写记录

			scoreInfoVec.push_back(__scoreInfo);
		}

    }
    m_pITableFrame->WriteSpecialUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_GameIds);
    // 写入分值.
    //    LOG_INFO << "WriteUserScore called, count:" << idx;


    //不让别人看到弃牌玩家的牌
    auto fillPlayerItem=[&](int32_t chairID)
    {
        GameEnd.clear_penduserinfo();

        for(int i = 0 ; i< GAME_PLAYER;++i)
        {
            int64_t uId =GetUserIdByChairId(i);
            if(uId<=0) continue;
            CMD_S_GameEnd_Player* playerItem = GameEnd.add_penduserinfo();
            if(chairID==i||(m_bCompardChairID[i]&&m_bCompardChairID[chairID]))
            {
                for(int j = 0; j < 5; ++j)
                {
                    if(j==0||m_cbHandCardData[i][j])
                        playerItem->add_cbcarddata(m_cbHandCardData[i][j]);
                }
                m_cbHandCardType[i]=m_GameLogic.GetCardType(m_cbHandCardData[i],m_HandCardCount);
                playerItem->set_cbcardtype(m_cbHandCardType[i]);
            }else{
                for(int j = 0; j < 5; ++j)
                {
                    if(j>0&&m_cbHandCardData[i][j])
                        playerItem->add_cbcarddata(m_cbHandCardData[i][j]);
                    else
                        playerItem->add_cbcarddata(0);
                }
                playerItem->set_cbcardtype(0);
            }
            playerItem->set_dchairid(i);
            playerItem->set_duserid(uId);
            if(m_wWinnerUser == i)
            {
                playerItem->set_dgamescore(lWinnerScore - tax);
            }
            else
            {
                playerItem->set_dgamescore(-m_lTableScore[i]);
            }

//            shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
//            if (pIServerUserItem)
//            {
//                int64_t lUserScore = pIServerUserItem->GetUserScore();
//                playerItem->set_duserscore(lUserScore);
//            }
//            else
//            {
                if(m_wWinnerUser == i)
                {
                    playerItem->set_duserscore( m_llUserSourceScore[i] + lWinnerScore - m_pITableFrame->CalculateRevenue(lWinnerScore));//lWinnerScore*95 / 100);
                }
                else
                {
                    playerItem->set_duserscore(m_llUserSourceScore[i] - m_lTableScore[i]);
                }
//            }
        }

    };

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_bJoinedGame[i])
        {
            fillPlayerItem(i);
            if( m_wWinnerUser != i)
            {
                m_replay.addResult(i,i,m_lTableScore[i],m_iUserWinScore[i],
                                   to_string(m_cbHandCardType[i])+":"+GlobalFunc::converToHex(m_cbHandCardData[i],m_HandCardCount),false);
            }else
            {
                m_replay.addResult(i,i,m_lTableScore[i],lWinnerScore-tax,
                                   to_string(m_cbHandCardType[i])+":"+GlobalFunc::converToHex(m_cbHandCardData[i],m_HandCardCount),false);
            }

            string content = GameEnd.SerializeAsString();
            m_pITableFrame->SendTableData(i, SUB_S_GAME_END, (uint8_t *)content.data(), content.size());
        }
    }
	//设置当局游戏详情
	SetGameRecordDetail();
    // 写完分直接清零，这样OnUserLeft流程可以判断，直接走ClearTableUser流程。
    memset(m_lTableScore, 0, sizeof(m_lTableScore));

    //库存统计
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        //赢家的是机器人的话
        if (i == wWinner && m_cbAndroidStatus[wWinner])
        {
            m_lCurrStockScore += (lWinnerScore);
            //m_lStockScore     += (lWinnerScore-tax/5);
            //m_llTodayStorage  += (lWinnerScore-tax/5);
            continue;
        }

        //库存累计
        if (m_cbAndroidStatus[i])
        {
            m_lCurrStockScore += lGameScore[i];
            //m_lStockScore     += lGameScore[i];
            //m_llTodayStorage  += lGameScore[i];
            //ResetTodayStorage();
        }
    }
	
    // 写赢家积分.
    UpdateStorageScore();
    m_pITableFrame->SaveReplay(m_replay);
    m_replay.clear();
    int wTimeLeft = 0;
    uint8_t nReason = (uint8_t)GETag;
    if(nReason == GER_NO_PLAYER)
    {
        wTimeLeft = (TIME_GAME_READY);
    }
    else if(nReason == GER_COMPARECARD)
    {
        wTimeLeft = (TIME_GAME_READY + END_TIME_COMPARE);
    }
    else if(nReason == GER_ALLIN)
    {
        wTimeLeft = (TIME_GAME_READY + END_TIME_ALLIN);
    }
    else if(nReason == GER_RUSH)
    {
        wTimeLeft = (TIME_GAME_READY + END_TIME_RUSH);
    }
    else if(nReason == GER_RUSH_GIVEUP)
    {
        wTimeLeft = (TIME_GAME_READY + END_TIME_RUSH_GIVEUP);
    }
    else
    {
        wTimeLeft = (TIME_GAME_READY + END_TIME_COMPARE);
    }

    LOG_INFO << "OnEventGameConclude nReason:" << (int)nReason << ", wTimeLeft:" << wTimeLeft;

    wTimeLeft += 1; //1秒时间补偿
    m_dwOpEndTime = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;



    //m_pITableFrame->SetGameStatus(GAME_STATUS_END);
    m_pITableFrame->ConcludeGame(GAME_STATUS_END);
    m_cbGameStatus = GAME_STATUS_END;



    for(int index=0;index<GAME_PLAYER;index++)
    {
        m_dwPlayUserID[index] = -1;
    }

    m_UserInfo.clear();
    if(m_IsMatch)
        m_TimerIdGameEnd = m_TimerLoopThread->getLoop()->runAfter(1, boost::bind(&CGameProcess::OnTimerGameEnd,  this));
    else
        OnTimerGameEnd();
    //    m_pITableFrame->DismissGame();
    return true;
}

void CGameProcess::OnTimerGameEnd()
{
    //清理系统弃牌的玩家
    for(uint8_t i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
        if(pIServerUserItem)
        {

            pIServerUserItem->SetUserStatus(sOffline);
            m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
        }
    }

    ClearAllTimer();
    InitGameData();
    //游戏结束
    //m_bContinueServer = m_pITableFrame->ConcludeGame(GAME_STATUS_END);
    m_cbGameStatus = GAME_STATUS_INIT;
    m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
}


//扑克分析
void CGameProcess::AnalyseStartCard()
{
    //机器人数
    int wAiCount = 0;
    int wPlayerCount = 0;
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        //获取用户
        bool bExistUser = m_pITableFrame->IsExistUser(i);
        if(bExistUser)
        {
            if(!m_bPlayStatus[i])
                continue;

            if(m_pITableFrame->IsAndroidUser(i))
            {
                wAiCount++;
            }
            wPlayerCount++;
        }
    }

    //全部机器
    if(wPlayerCount == wAiCount || wAiCount == 0)
        return;

    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbHandCardBufer, sizeof(m_cbHandCardBufer));

    //排列扑克
//    for(int i = 0; i < GAME_PLAYER; ++i)
//    {
//        m_GameLogic.SortCardList(cbUserCardData[i], MAX_COUNT);
//    }

    //变量定义
    int wWinUser = INVALID_CHAIR;
    int wGoodPaiAndroid = INVALID_CHAIR;
     LOG_INFO << "当前库存----------------:"<<m_lStockScore;
    int blackChairId=-1;
     //查找数据
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if(!m_bPlayStatus[i])
            continue;

        // 扑克.
        string cards = StringCards(m_cbHandCardBufer[i], MAX_COUNT);
        LOG_INFO  << " >>> wChairID:" << i << ", 扑克:" << cards << ", isAndroid:" << m_pITableFrame->IsAndroidUser(i);

        if (!m_IsMatch && !m_pITableFrame->IsAndroidUser(i)
                && m_pITableFrame->GetTableUserItem(i)->GetBlacklistInfo().status == 1
                && m_pITableFrame->GetTableUserItem(i)->GetBlacklistInfo().weight >= rand()%1000 )
        {
            openLog("黑名单用户chairid:%d,userid %d",i,m_pITableFrame->GetTableUserItem(i)->GetUserId());
            blackChairId = i;
        }

        //设置用户
        if (wWinUser == INVALID_CHAIR)
        {
            wWinUser = i;
            continue;
        }

        //对比扑克
        if (m_GameLogic.CompareCard(cbUserCardData[i], cbUserCardData[wWinUser], MAX_COUNT) >= 1)
        {
            wWinUser = i;
        }

    }
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
        //玩家是最大牌，并且判断概率是否换牌
        if (!m_pITableFrame->IsAndroidUser(wWinUser) )
        {
            LOG_INFO << "根据几率 -> 换牌!";

            //随机赢家
            int wBeWinAndroid = INVALID_CHAIR;
            do
            {
                //随机机器人赢
                wBeWinAndroid = rand() % GAME_PLAYER;
                    if (m_pITableFrame->IsExistUser(wBeWinAndroid) && m_pITableFrame->IsAndroidUser(wBeWinAndroid))
                        break;
            } while (true);

            //交换数据
            uint8_t cbTempData[MAX_COUNT];
            memcpy(cbTempData, m_cbHandCardBufer[wBeWinAndroid], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbHandCardBufer[wBeWinAndroid], m_cbHandCardBufer[wWinUser], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbHandCardBufer[wWinUser], cbTempData, sizeof(uint8_t)*MAX_COUNT);
            return;
        }
        else
        {
            return;
        }
    }
    int blackListUser=0;
    if(blackChairId !=-1)
    {
        if(blackChairId !=wWinUser ) return;
        blackListUser = m_pITableFrame->GetTableUserItem(wWinUser)->GetUserId();
        LOG_INFO << "--- *** 黑名单用户!"<<blackListUser ;
        openLog("黑名单用户换牌:%d",blackListUser);
        int wBeWinAndroid = INVALID_CHAIR;
        do
        {
            //随机机器人赢
            wBeWinAndroid = rand() % GAME_PLAYER;
            if (m_pITableFrame->IsExistUser(wBeWinAndroid) && m_pITableFrame->IsAndroidUser(wBeWinAndroid))
                break;
        } while (true);

        //交换数据
        uint8_t cbTempData[MAX_COUNT];
        memcpy(cbTempData, m_cbHandCardBufer[wBeWinAndroid], sizeof(uint8_t)*MAX_COUNT);
        memcpy(m_cbHandCardBufer[wBeWinAndroid], m_cbHandCardBufer[wWinUser], sizeof(uint8_t)*MAX_COUNT);
        memcpy(m_cbHandCardBufer[wWinUser], cbTempData, sizeof(uint8_t)*MAX_COUNT);
        return;
    }

    //系统赢了不得大于库存上限，否则系统吐出来
    if ( m_lStockScore > m_llTodayHighLimit)		//放水
    {
        //机器人是最大牌，并且判断概率是否换牌
        if (m_pITableFrame->IsAndroidUser(wWinUser) && Exchange == CalcExchangeOrNot2(m_poolSystemWinChangeCardRatio)/*nRandNum < 500*/)
        {
            LOG_INFO << "--- *** 放水!";

            //随机赢家
            int wBeWin = INVALID_CHAIR;

            do
            {
                //随机玩家赢
                wBeWin = rand() % GAME_PLAYER;
                if (m_pITableFrame->IsExistUser(wBeWin) && !m_pITableFrame->IsAndroidUser(wBeWin))
                    break;
            } while (true);

            //交换数据
            uint8_t cbTempData[MAX_COUNT];
            memcpy(cbTempData, m_cbHandCardBufer[wBeWin], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbHandCardBufer[wBeWin], m_cbHandCardBufer[wWinUser], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbHandCardBufer[wWinUser], cbTempData, sizeof(uint8_t)*MAX_COUNT);
        }
    }
    //系统输了不得低于库存下限，否则玩家吐出来
    if (m_lStockScore < m_lStockLowLimit)			//杀玩家
    {
        //玩家是最大牌，并且判断概率是否换牌
        if (!m_pITableFrame->IsAndroidUser(wWinUser) && Exchange == CalcExchangeOrNot2(m_poolSystemLostChangeCardRatio)/*nRandNum < m_wSystemAllKillRatio*/)
        {
            LOG_INFO << "根据几率 -> 换牌!";

            //随机赢家
            int wBeWinAndroid = INVALID_CHAIR;
        do
        {
            //随机机器人赢
            wBeWinAndroid = rand() % GAME_PLAYER;
                if (m_pITableFrame->IsExistUser(wBeWinAndroid) && m_pITableFrame->IsAndroidUser(wBeWinAndroid))
                    break;
            } while (true);

            //交换数据
            uint8_t cbTempData[MAX_COUNT];
            memcpy(cbTempData, m_cbHandCardBufer[wBeWinAndroid], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbHandCardBufer[wBeWinAndroid], m_cbHandCardBufer[wWinUser], sizeof(uint8_t)*MAX_COUNT);
            memcpy(m_cbHandCardBufer[wWinUser], cbTempData, sizeof(uint8_t)*MAX_COUNT);
        }
    }

    return;
}

string CGameProcess::StringCards(uint8_t cbHandCardData[], uint8_t count)
{
    string s;
    for(int j = 0; j < count; j++)
    {
        uint8_t b = cbHandCardData[j];
        if(b == 0)
            return "";

        uint8_t v = m_GameLogic.GetCardValue(b);
        uint8_t c = m_GameLogic.GetCardColor(b);

        string color;
        if(c == 0x00)
        {
            color = "方";
        }
        else if(c == 0x10)
        {
            color = "梅";
        }
        else if(c == 0x20)
        {
            color = "红";
        }
        else if(c == 0x30)
        {
            color = "黑";
        }
        s.append(color);

        if(v == 0x01)
        {
            s.append("A");
        }
        else if(v == 0x02)
        {
            s.append("2");
        }
        else if(v == 0x03)
        {
            s.append("3");
        }
        else if(v == 0x04)
        {
            s.append("4");
        }
        else if(v == 0x05)
        {
            s.append("5");
        }
        else if(v == 0x06)
        {
            s.append("6");
        }
        else if(v == 0x07)
        {
            s.append("7");
        }
        else if(v == 0x08)
        {
            s.append("8");
        }
        else if(v == 0x09)
        {
            s.append("9");
        }
        else if(v == 0x0a)
        {
            s.append("10");
        }
        else if(v == 0x0b)
        {
            s.append("J");
        }
        else if(v == 0x0c)
        {
            s.append("Q");
        }
        else if(v == 0x0d)
        {
            s.append("K");
        }
        else if(v == 0x0e)
        {
            s.append("X");
        }
        else if(v == 0x0f)
        {
            s.append("Y");
        }
    }

    return s;
}



//更新REDIS库存.
void CGameProcess::UpdateStorageScore()
{
    if(m_pITableFrame)
    {
        double res = m_lCurrStockScore;
        if(res>0)
        {
            res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
        }
        m_pITableFrame->UpdateStorageScore(res);
        m_lCurrStockScore = 0;
    }
}

//读取配置
void CGameProcess::ReadStorageInformation()
{
    //读取配置ID.
    uint32_t nConfigId = 0;
    if(m_pGameRoomKind)
    {
        nConfigId = m_pGameRoomKind->roomId;

    }

    if(!boost::filesystem::exists(INI_FILENAME))
    {
        LOG_INFO << INI_FILENAME <<" not exists";
        return;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(INI_FILENAME, pt);

    m_bLimitAllIn= pt.get<int>("GAME_CONFIG.limitAllIn", 0);
    m_IsEnableOpenlog=pt.get<int>("GAME_CONFIG.openLog", 1);
    m_joinRoomScoreMultipleLimit=pt.get<double>("FLOOR_SCORE_MULTIPLE_LIMIT.room_"+to_string(nConfigId), 1);
    //读取库存配置
    stockWeak=pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    tagStorageInfo storageInfo = {0};
    if (m_pITableFrame)
    {
        m_pITableFrame->GetStorageScore(storageInfo);
    }
    // initialize the storage value sitem now.
    m_lStockScore = storageInfo.lEndStorage;//系统初始库存
    m_lStockLowLimit = storageInfo.lLowlimit;//系统输了不得低于库存下限，否则玩家吐出来
    m_llTodayHighLimit = storageInfo.lUplimit;//系统赢了不得大于库存上限，否则系统吐出来
	m_wSystemLostChangeCardRatio = 80;//系统输了换牌概率 80%
    m_wSystemWinChangeCardRatio = 70;//系统赢了换牌概率 70%

    // 当前系统库存
    LOG_ERROR << "StockScore:" << m_lStockScore <<",StockHighLimit:"<<m_llTodayHighLimit<<",StockLowLimit:"<<m_lStockLowLimit;

	//m_wSystemAllKillRatio = storageInfo.lSysAllKillRatio;//系统通杀概率
    //m_wSystemChangeCardRatio = storageInfo.lSysChangeCardRatio;//系统换牌概率
}

bool CGameProcess::readJsonCard()
{
    //return false;
    if(!boost::filesystem::exists("./log/gswz"))
    {
        boost::filesystem::create_directories("./log/gswz");
    }

    if (!boost::filesystem::exists("./conf/cards_gswz.json"))
        return false;
    boost::property_tree::ptree pt;
    try{
        boost::property_tree::read_json("./conf/cards_gswz.json",pt);
    }catch(...){
        LOG_INFO<<"cards.json firmat error !!!!!!!!!!!!!!!";
        return false;
    }

    m_listTestCardArray.clear();
    for (auto wave:pt)
    {
        vector<uint8_t> cards;
        //auto& wave=pt.back();
        for(auto&& card : wave.second)
        {
            cards.push_back(stod(card.second.data()));
            std::cout<<stod(card.second.data())<<endl;
        }
        std::cout<<"next"<<endl;
        m_listTestCardArray.push_back(cards);
        //pt.pop_back();
    }

    string str="./conf/cards";
    str+= time((time_t*)NULL);
    str+="_gswz.json";
    boost::filesystem::rename("./conf/cards_gswz.json",str);
}


int64_t CGameProcess::GetUserIdByChairId(int32_t chairId)
{
    return m_dwPlayUserID[chairId];
}


bool CGameProcess::OnUserTimeoutGiveUpSetting(int wChairID,  uint32_t mask)
{
    m_NoGiveUpTimeout[wChairID] = mask;
    if(!m_pITableFrame->IsExistUser(wChairID))
        return false;

    CMD_S_GIVEUP_TIMEOUT_OP TimeoutOp;
    TimeoutOp.set_mask(mask);
    string content = TimeoutOp.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SUB_S_GIVEUP_TIMEOUT_OP, (uint8_t *)content.data(), content.size());
    return true;
}

void CGameProcess::SendServerNotify2Client(int32_t chairID, string &str)
{
    Gswz::CMD_S_Operate_Notify  notify;
    notify.set_strnotify(str);
    string data=notify.SerializeAsString();
    m_pITableFrame->SendTableData(chairID,SUB_S_OPERATE_NOTIFY,(uint8_t *)data.data(),data.size());
}

int32_t CGameProcess::GetMaxShowCardChairID()
{
    int32_t ChairID=INVALID_CHAIR;
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if(!m_bPlayStatus[i])
            continue;
        //设置用户
        if (ChairID == INVALID_CHAIR)
        {
            ChairID = i;
            continue;
        }

        //对比扑克
        if (m_GameLogic.CompareCard(m_cbHandCardData[i]+1, m_cbHandCardData[ChairID]+1, m_HandCardCount-1) >= 1)
        {
            ChairID = i;
        }
    }

    return ChairID;
}


int32_t CGameProcess::GetNextPlayer()
{
    int32_t wNextPlayerChairID = INVALID_CHAIR;
    int32_t wTmpNextPlayer = INVALID_CHAIR;

    for(int i = 1; i<=GAME_PLAYER; i++)
    {
        //设置变量
        wTmpNextPlayer = (m_wCurrentChairID + i) % GAME_PLAYER;
        //继续判断
        if (m_bPlayStatus[wTmpNextPlayer]&&m_wCurrentRoundScore[wTmpNextPlayer] < m_lCurrentJetton)
        {
            wNextPlayerChairID=wTmpNextPlayer;
            break;
        }
    }
    return wNextPlayerChairID;
}

void CGameProcess::SendCard()
{
    m_wCurrentChairID=GetMaxShowCardChairID();
    assert(m_wCurrentChairID!=INVALID_CHAIR);
    CMD_S_SendCard sendCard;
    uint8_t chairId=m_lastMaxViewCardChairID;
    for(int i = 0 ; i <GAME_PLAYER ; ++i)
    {
        chairId = (m_lastMaxViewCardChairID + i) % GAME_PLAYER ;
        if(m_bPlayStatus[chairId])
        {
            Gswz::CardItem *pCardItem=sendCard.add_carditems();
            pCardItem->set_cbcarduser(GetUserIdByChairId(chairId));
            pCardItem->set_cbcard(m_cbHandCardData[chairId][m_HandCardCount-1]);
            pCardItem->set_cbcardtype(0); //no use
        }
    }
    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;

    CMD_S_Next_StepInfo*  nextstep =  sendCard.mutable_nextstep();
    if(GetCanAddMaxScore(m_wCurrentChairID)==0)
        nextstep->set_wnextuser(0);
    else
        nextstep->set_wnextuser(GetUserIdByChairId(m_wCurrentChairID));

    m_lastMaxViewCardChairID=m_wCurrentChairID;
    nextstep->set_wtimeleft(wTimeLeft);
    nextstep->set_dtotaljetton(m_lAllScoreInTable);
    nextstep->set_wturns(m_TurnsRound);
    nextstep->set_wneedjettonscore(0);
    LOG_INFO<<"m_TurnsRound   =================="<<m_TurnsRound;
    //nextstep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
    if(m_TurnsRound>2)
        nextstep->set_wmaxjettonscore(GetCanAddMaxScore(m_wCurrentChairID));
    else
        nextstep->set_wmaxjettonscore(0);
    nextstep->set_wminaddscore(m_lCellScore);
    openLog("------>GameIds:%s,SendCard,wNextPlayerChairID:%d"
            ,m_GameIds.c_str(),m_wCurrentChairID);
    string content = sendCard.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_SEND_CARD, (uint8_t *)content.data(), content.size());
    //已经到达最大下注 直接开牌
    if(GetCanAddMaxScore(m_wCurrentChairID)==0)
    {
        if(m_HandCardCount != 5)
        {
            SendBufferCard2HandCard(1);
            SendCard();
            return;
        }
        HandleMorethanRoundEnd();
        return ;
    }
    //玩家下注定时器
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 1),boost::bind(&CGameProcess::OnTimerAddScore, this));
    LOG_INFO << " >>> SendCard, delay:" << (wTimeLeft+2);
}

void CGameProcess::WriteLoseUserScore(int32_t chairId)
{
    if (m_IsMatch||!m_pITableFrame->IsAndroidUser(chairId)) {

        //TODO:扣玩家的金币
        //    m_pITableFrame->WriteUserScore()
        tagSpecialScoreInfo scoreInfo;
        //    scoreInfo.userId = pIServerUserItem->GetUserId();
        //    scoreInfo.beforeScore = pIServerUserItem->GetUserScore();
        scoreInfo.chairId = chairId;
        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(scoreInfo.chairId);
        scoreInfo.userId = userItem->GetUserId();
        scoreInfo.account = userItem->GetAccount();
        scoreInfo.agentId = userItem->GetUserBaseInfo().agentId;
        scoreInfo.addScore = -m_lTableScore[chairId];
        scoreInfo.betScore = m_lTableScore[chairId];
        scoreInfo.rWinScore = m_lTableScore[chairId];
        scoreInfo.revenue = 0;
        scoreInfo.cellScore.push_back(m_lTableScore[chairId]);
        scoreInfo.cardValue = "";
        scoreInfo.startTime = m_startTime;
        scoreInfo.bWriteScore = true;           // 写分
        scoreInfo.bWriteRecord = false;          // 写记录
        // 写入分值.
        m_pITableFrame->WriteSpecialUserScore(&scoreInfo, 1, m_GameIds);
    }
    m_bPlayStatus[chairId] = false;
}

int64_t CGameProcess::GetCanAddMaxScore(int32_t chairId)
{
    //
//    if(!m_bLimitAllIn)
//    {
//        return m_dAllInScore-m_lTableScore[chairId];
//    }
//    if(m_HandCardCount==2)
//    {
//        return m_dAllInScore/4-m_lTableScore[chairId];
//    }else if(m_HandCardCount==3)
//    {
//        return m_dAllInScore/2-m_lTableScore[chairId];
//    }else
    {
        return m_dAllInScore-m_lTableScore[chairId];
    }
}

void CGameProcess::openLog(const char *p, ...)
{
    if (m_IsEnableOpenlog)
    {

        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/gswz/gswz%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

void CGameProcess::SendBufferCard2HandCard(uint8_t count)
{
    m_HandCardCount+=count;
    for(int i=0 ;i<GAME_PLAYER ; i++)
    {
        if(m_bPlayStatus[i])
            memcpy(m_cbHandCardData[i],m_cbHandCardBufer[i],m_HandCardCount);

        m_wCurrentRoundScore[i]=-1;
    }
    memset(m_LastAction,0,sizeof(m_LastAction));
    m_lCurrentJetton=0;
    m_TurnsRound+=1;
    m_MinAddScore=m_lCellScore;
}

//设置当局游戏详情
void CGameProcess::SetGameRecordDetail()
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	//封顶值
	details.set_gamemaxscore(m_dAllInScore);
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		int chairID = userInfoItem->chairId;
		PlayerRecordDetail* detail = details.add_detail();
		//boost::property_tree::ptree item, itemscores, itemscorePure;
		//账号/昵称
		detail->set_account(std::to_string(userInfoItem->userId));
		//座椅ID
		detail->set_chairid(chairID);
		//手牌/牌型
		HandCards* pcards = detail->mutable_handcard();
		int count = 0;
		uint8_t handCards[MAX_COUNT] = { 0 };
		for (int id = 0; id < m_HandCardCount; id++)
		{
			handCards[count++] = m_cbHandCardData[chairID][id];
		}
		pcards->set_cards(&(handCards)[0], m_HandCardCount);
		pcards->set_ty(m_cbHandCardType[chairID]);
		//携带积分
		detail->set_userscore(userInfoItem->initScore);
		//房间底注
		detail->set_cellscore(m_lCellScore);
		//牌局轮数
		detail->set_dturnsround(m_TurnsRound);
		//投注积分
		detail->set_dbetjetton(m_lTableScore[chairID]);
		//输赢积分
		int32_t lWinnerScore = m_iUserWinScore[chairID];
		int32 dUserIsWin = 0;
		if (lWinnerScore>0)
		{
			int32_t Revenue = m_pITableFrame->CalculateRevenue(lWinnerScore);
			lWinnerScore -= Revenue;
			dUserIsWin = 1;
		}
		detail->set_winlostscore(lWinnerScore);
		//玩家是否赢[0:否;1:赢]
		detail->set_duseriswin(dUserIsWin);
		//玩家是否弃牌[0:不弃;1:弃牌]
		detail->set_isgiveup(m_cbSystemGiveUp[chairID]);
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		m_replay.detailsData = details.SerializeAsString();
	}
}

//得到桌子实例
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink()
{
    shared_ptr<CGameProcess> pGameProcess(new CGameProcess());
    shared_ptr<ITableFrameSink> pITableFrameSink = dynamic_pointer_cast<ITableFrameSink>(pGameProcess);
    return pITableFrameSink;
}

//删除桌子实例
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pITableFrameSink)
{
    pITableFrameSink.reset();
}

