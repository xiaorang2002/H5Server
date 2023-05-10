#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

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
#include <functional>
#include <chrono>

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
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/Blackjack.Message.pb.h"

#include "MSG_HJK.h"
#include "GameLogic.h"
#include "hand.h"
#include "errorcode.h"

//#include "TestCase.h"

using namespace Blackjack;

#include "GameProcess.h"

//打印日志
#define		PRINT_LOG_INFO						1
#define		PRINT_LOG_WARNING					1


// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/hjk/";
        // initialize the glog content.
        FLAGS_colorlogtostderr = true;
        FLAGS_minloglevel = google::GLOG_INFO;
        FLAGS_log_dir     = dir;
        FLAGS_logbufsecs  = 3;

        if(!boost::filesystem::exists(dir))
        {
            boost::filesystem::create_directories(dir);
        }

        // set std output special log content value.
        google::SetStderrLogging(FLAGS_minloglevel);
        // initialize the log prefix name now.
        google::InitGoogleLogging("hjk");

        //设置文件名
        //=====config=====
        if(!boost::filesystem::exists(INI_FILENAME))
        {
            LOG_INFO << INI_FILENAME <<" not exists";
            return;
        }

        boost::property_tree::ptree pt;
        boost::property_tree::read_ini(INI_FILENAME, pt);
        int errorlevel = pt.get<int>("Global.errorlevel", -1);
        if(errorlevel >= 0)
        {
            FLAGS_minloglevel = errorlevel;
        }

        int printlevel = pt.get<int>("Global.printlevel", -1);
        if(printlevel >= 0)
        {
            // set std output special log content.
            google::SetStderrLogging(printlevel);
        }
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit glog_init;


class CTestDump
{
public:
    CTestDump(string func, int32_t dwChairID, uint8_t dwSubCmdID, uint8_t tableID)
    {
        m_dwChairID  = dwChairID;
        m_dwSubCmdID = dwSubCmdID;
        m_strFunc    = func;
        m_tableID =tableID;
    }

    virtual ~CTestDump()
    {
        LOG(ERROR) << "Table:" << (int)m_tableID << ", End of " << m_strFunc << ", wChairID:" << m_dwChairID << ", dwSubCmdID:" << (int)m_dwSubCmdID;
    }

protected:
    string m_strFunc;
    int32_t m_dwChairID;
    uint8_t m_dwSubCmdID;
    uint8_t m_tableID;
};



////////////////////////////////////////////////////////////////////////
//WORD CGameProcess::m_wLargeJetton[MAX_JETTON_NUM] = { 2000,3000,5000,8000,10000 };	//大筹码
//WORD CGameProcess::m_wMediumJetton[MAX_JETTON_NUM] ={ 1000,1500,2500,4000,5000 };	//中筹码
//WORD CGameProcess::m_wSmallJetton[MAX_JETTON_NUM] = { 200,300,500,800,1000 };		//小筹码

////定时器
//#define IDI_GAME_ADD_SCORE				1							//下注定时器
//#define IDI_AUTO_FOLLOW					2							//自动加注
//#define IDI_CLEAR_OFFLINE_USER			3							//清理离线玩家
//#define IDI_GAME_READY					4							//准备定时器

#define TIMER_START_READY               1                           //start to ready.
#define TIME_GAME_READY					2+4							//准备时间（真正准备是5秒,有3秒是客户端飘金币动画时间（不包括比牌时间））
#define TIME_GAME_LOCK_PLAYING          2
#define TIME_GAME_ADD_SCORE				20							//下注时间(second)
#define TIME_GAME_INSURE                15                          //insure time
#define TIME_GAME_OPERATE               15                          //operate time
#define TIME_CLEAR_USER					3							//清理离线或者金币不足用户




////////////////////////////////////////////////////////////////////////
//静态变量
//const int  CGameProcess::GAME_PLAYER     = GAME_PLAYER;			//游戏人数
//int64_t      CGameProcess::m_lStockScore      = 0;
int64_t      CGameProcess::m_totalstock   = 0;
int64_t      CGameProcess::m_totalstocklowerlimit   = 0;
int64_t      CGameProcess::m_totalstockhighlimit = 0;
int32_t     CGameProcess::m_poorTimes = 0;
int32_t     CGameProcess::m_richTimes = 0;

CGameProcess::CGameProcess(void)
{
    LOG(INFO)<< __FILE__ << __FUNCTION__;

    stockWeak = 0.0;
    m_llMaxJettonScore = 0;  //最大下注筹码
    m_wPlayCount = 0;            //当局人数

    m_cbGameStatus = GAME_STATUS_INIT;       //游戏状态
    m_nRoundID = 0;
    m_nTableID = 0;

//    m_dwStartTime = NULL;

    m_lMarqueeMinScore = 1000;

    m_wBankerUser = GAME_PLAYER-1;
    m_wCurrentUser = INVALID_CHAIR;
//    m_wWinnerUser = INVALID_CHAIR;
    shared_ptr<Hand> lastHand;
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        m_bPlayStatus[i] = false;
        m_bJoinedGame[i] = false;

        m_dwPlayUserID[i] = -1;
        m_dwPlayFakeUserID[i] = -1;

        m_cbRealPlayer[i] = 0;
        m_cbAndroidStatus[i] = 0;

        m_bWaitEnterTable[i] = false;

        m_lTableScore[i] = 0;
//        m_iUserWinScore[i] = 0;

        shared_ptr<Hand> hand0(new Hand(i));
        m_Hands[i] = hand0;
        if(i != m_wBankerUser)
        {
            if(lastHand)
            {
                lastHand->next = hand0;
            }
            shared_ptr<Hand> hand1(new Hand(i+GAME_PLAYER));
            m_Hands[i+GAME_PLAYER] = hand1;
            hand0->next = hand1;
            lastHand = hand1;
//            LOG(ERROR) << "Hand:" << hand1->index << ", issecond:" << hand1->issecond;
        }

//        m_wCompardUser[i].clear();
        //        m_mChairIdUserId[i] = -1;
    }
    m_llUserSourceScore.clear();
    m_lCellScore = 0L;

    m_dwOpEndTime = 0;
    m_wTotalOpTime = 0;
//    m_mOutCardRecord.clear();
    //    memset(m_RecordOutCard, 0, sizeof(m_RecordOutCard));
    m_iUserWinScore.clear();
    // m_bGameEnd = false;

    //扑克变量
    memset(m_cbHandCardData,  0, sizeof(m_cbHandCardData));
    memset(m_cbRepertoryCard,  0, sizeof(m_cbRepertoryCard));


//    m_lCurrStockScore = 0;

    m_wSystemChangeCardRatio = 620; // 系统换牌率.
    m_wSystemAllKillRatio = 50;     // 系统通杀率.
    m_bContinueServer = true;
//    m_wCurrentCards.reserve(MAX_COUNT);

//   testCase = make_shared<CTestCase>();

	////////////////////////////////////////
	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//超时匹配时长(可配置)
	timeoutMatchSeconds_ = 3.6f;
	//机器人候补空位超时时长(可配置)
	timeoutAddAndroidSeconds_ = 1.4f;
	//放大倍数
	ratioGamePlayerWeightScale_ = 1000;
	//初始化桌子游戏人数概率权重
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (i == 5) {
			//6人局的概率0
			ratioGamePlayerWeight_[i] = 0;//15 * ratioGamePlayerWeightScale_;
		}
		else if (i == 4) {
			//5人局的概率15%
			ratioGamePlayerWeight_[i] = 15 * ratioGamePlayerWeightScale_;
		}
		else if (i == 3) {
			//4人局的概率25%
			ratioGamePlayerWeight_[i] = 25 * ratioGamePlayerWeightScale_;
		}
		else if (i == 2) {
			//3人局的概率40%
			ratioGamePlayerWeight_[i] = 40 * ratioGamePlayerWeightScale_;
		}
		else if (i == 1) {
			//2人局的概率15%
			ratioGamePlayerWeight_[i] = 15 * ratioGamePlayerWeightScale_;
		}
		else if (i == 0) {
			//单人局的概率5%
			ratioGamePlayerWeight_[i] = 5 * ratioGamePlayerWeightScale_;
		}
		else {
			ratioGamePlayerWeight_[i] = 0;
		}
	}
	//计算桌子要求标准游戏人数
	poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
	MaxGamePlayers_ = 0;
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

        m_replay.cellscore = m_pGameRoomKind->floorScore;
        m_replay.roomname = m_pGameRoomKind->roomName;
    }

    //初始化游戏数据
    InitGameData();

    //    //更新游戏配置
    //    UpdateGameConfig();

    //读取配置.
    ReadStorageInformation();
    LOG(ERROR) << "init ReadStorageInformation, m_totalstock:" << m_totalstock;

    m_nTableID = m_pITableFrame->GetTableId();

    return true;
}


//发送数据
bool CGameProcess::SendTableData(uint32_t chairId, uint8_t subId,const uint8_t* data, int len, bool isrecord)
{
    if(!m_pITableFrame)
    {
        LOG(WARNING) << "SendTableData: chairId = " << (int)chairId << ", subId ="<< (int)subId << " err";
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
    m_bInsured = false;
//    m_dwStartTime = 0;

    m_wCurrentUser = 0;
    m_currentHand = m_Hands[0];

    for(int i = 0; i < GAME_PLAYER; i++)
    {
        m_bPlayStatus[i] = false;
        m_bJoinedGame[i] = false;

        m_dwPlayUserID[i] = -1;
        m_dwPlayFakeUserID[i] = -1;

        m_cbRealPlayer[i] = 0;
        m_cbAndroidStatus[i] = 0;

        m_bWaitEnterTable[i] = false;
        m_bSettled[i]=false;
    }
    memset(m_lTableScore,0,sizeof(m_lTableScore));
//    memset(m_iUserWinScore,0.0,sizeof(m_iUserWinScore));
    m_iUserWinScore.clear();
    m_SeatToUser.clear();
    // m_mChairIdUserId.clear();
    m_lCellScore = 0L;
    m_llUserSourceScore.clear();
    m_dwOpEndTime = 0;
    m_wTotalOpTime = 0;
    m_wUserBets.clear();
//    m_mOutCardRecord.clear();
    shared_ptr<Hand> hand;
    for(auto &it : m_Hands)
    {
        hand = it.second;
        hand->clear();
    }

    //扑克变量
    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_lUserInsure,0, sizeof(m_lUserInsure));
    m_wDroppedCards.clear();
    m_bContinueServer = true;

    if(m_pITableFrame)
    {
        m_lCellScore = m_pGameRoomKind->floorScore;
        m_llMaxJettonScore = MAX_JETTON_MULTIPLE*m_lCellScore;
    }

    reallyreallypoor = false;

    m_iBlacklistNum = 0;
    ReadStorageInformation();
}


//清除所有定时器
void CGameProcess::ClearAllTimer()
{
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameLockPlaying);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdInsure);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOperate);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameEnd);
}


bool CGameProcess::OnUserEnter(int64_t userId, bool islookon)
{
    LOG(ERROR)<<"---------------------------OnUserEnter---------------------userId:"<<userId;

    shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!userItem)
    {
        LOG(ERROR) << "User not found, userid:" << userId;
        return false;
    }
    uint32_t chairId = userItem->GetChairId();
    if(chairId > m_wBankerUser-1)
    {
        LOG(ERROR)<<"---------------------------OnUserEnter------pIServerUserItem==NULL"<<"---------------------";
        return false;
    }

	if (m_pITableFrame->GetPlayerCount(true) == 1) {
		////////////////////////////////////////
		//第一个进入房间的也必须是真人
		assert(m_pITableFrame->GetPlayerCount(false) == 1);
		////////////////////////////////////////
		//权重随机乱排
		poolGamePlayerRatio_.shuffleSeq();
		////////////////////////////////////////
		//初始化桌子当前要求的游戏人数
		MaxGamePlayers_ = poolGamePlayerRatio_.getResultByWeight() + 1;
		totalMatchSeconds_ = 0;
		printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 初始化游戏人数 MaxGamePlayers_：%d\n", MaxGamePlayers_);
	}
	if (m_cbGameStatus < GAME_STATUS_START) {
		assert(MaxGamePlayers_ >= MIN_GAME_PLAYER);
		assert(MaxGamePlayers_ <= GAME_PLAYER);
	}

    if(m_cbGameStatus > GAME_STATUS_LOCK_PLAYING )
    {
        if( m_dwPlayUserID[chairId] == userItem->GetUserId())
        {
            userItem->SetUserStatus(sPlaying);
            m_pITableFrame->BroadcastUserInfoToOther(userItem);
            m_pITableFrame->SendAllOtherUserInfoToUser(userItem);
            m_pITableFrame->BroadcastUserStatus(userItem, true);
            // OnEventGameScene(chairId,false);
            LOG(ERROR)<<"---------------------------OnUserEnter------user->GetChairId()= " << userItem->GetChairId()<<"---------------------";
//            if(m_cbGameStatus != GAME_STATUS_INIT )
//            {
               OnEventGameScene(chairId,false);
//            }
            return true;
        }
        return false;
    }else{
        userItem->SetUserStatus(sReady);
        LOG(ERROR)<<"---------------------------OnUserEnter------user->GetChairId()= " << userItem->GetChairId()<<"---------------------";

        //获取玩家个数
       if (m_cbGameStatus == GAME_STATUS_INIT)
       {
           int wUserCount = 0;
           //用户状态
           for(int i = 0; i < m_wBankerUser; ++i)
           {
               //获取用户
               shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);
               if(pIServerUser)
               {
                   ++wUserCount;
               }
           }

           if(wUserCount >= m_pGameRoomKind->minPlayerNum)
           {
               ClearAllTimer();
               m_cbGameStatus = GAME_STATUS_READY;
               LOG(ERROR)<<"--------------------------m_cbGameStatus == GAME_STATUS_INIT && wUserCount >= 2--------------------------"<<m_pITableFrame->GetTableId();

               m_dwStartTime = chrono::system_clock::now();

               //m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(TIMER_START_READY, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
           }
       }

       if(m_cbGameStatus == GAME_STATUS_LOCK_PLAYING || m_cbGameStatus == GAME_STATUS_READY )
       {
           m_bJoinedGame[chairId] = true;
           userItem->SetUserStatus(sPlaying);
       }

       m_pITableFrame->BroadcastUserInfoToOther(userItem);
       m_pITableFrame->SendAllOtherUserInfoToUser(userItem);
       m_pITableFrame->BroadcastUserStatus(userItem, true);
//       if(m_cbGameStatus != GAME_STATUS_INIT )
//       {
          OnEventGameScene(chairId,false);
//       }
    }

    LOG(ERROR)<<"---------------------------OnUserEnter-----m_wGameStatus----------------------"<< (int)m_cbGameStatus;
	if (m_cbGameStatus < GAME_STATUS_START) {
		////////////////////////////////////////
		//达到桌子要求游戏人数，开始游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			////////////////////////////////////////
			//不超过桌子要求最大游戏人数
			assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
			printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
			OnTimerGameReadyOver();
		}
		else {
			if (m_pITableFrame->GetPlayerCount(true) == 1) {
				////////////////////////////////////////
				//第一个进入房间的也必须是真人
				assert(m_pITableFrame->GetPlayerCount(false) == 1);
				ClearAllTimer();
				////////////////////////////////////////
				//修改游戏准备状态
				//m_pITableFrame->SetGameStatus(GAME_STATUS_READY);
				//m_cbGameStatus = GAME_STATUS_READY;
				m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
			}
		}
	}
	return true;
}

void CGameProcess::RepositionSink()
{

}

bool CGameProcess::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
	////////////////////////////////////////
	//初始化或空闲准备阶段，可以进入桌子
	if (m_cbGameStatus < GAME_STATUS_START)
	{
		////////////////////////////////////////
		//达到游戏人数要求禁入游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			return false;
		}
		////////////////////////////////////////
		//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
        if (pUser->GetUserId() == -1) {
			if (totalMatchSeconds_ < timeoutMatchSeconds_) {
				//printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, userId, totalMatchSeconds_);
				return false;
			}
		}
		else {
			////////////////////////////////////////
			//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
            shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
            if (userItem) {
                if (userItem->IsAndroidUser()) {
					if (totalMatchSeconds_ < timeoutMatchSeconds_) {
						//printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, userId, totalMatchSeconds_);
						return false;
					}
				}
				else {
					//真实玩家底分不足，不能进入房间
				}
            }
		}
		return true;
	}
    if(m_cbGameStatus >= GAME_STATUS_BET )
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
        return pIServerUserItem != NULL;
    }
	////////////////////////////////////////
	//游戏状态为GAME_STATUS_BET(100)时，不会进入该函数
	assert(false);
    return false;
}

bool CGameProcess::CanLeftTable(int64_t userId)
{
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG(ERROR) << "OnUserLeft of userid:" << userId << ", get chair id failed, pIServerUserItem==NULL";
        return true;
    }
//    uint32_t chairId = pIServerUserItem->GetChairId();
    if( m_cbGameStatus < GAME_STATUS_LOCK_PLAYING || m_cbGameStatus == GAME_STATUS_END)
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
    LOG(ERROR)<<"---------------------------OnUserLeft-----------m_wGameStatus----------------"<<(int)m_cbGameStatus;

    if(!CanLeftTable(userId))
    {
        return false;
    }
    bool ret = false;
    // try to get the chair id value.
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG(ERROR) << "OnUserLeft of userid:" << userId << ", get chair id failed, pIServerUserItem==NULL";
        return false;
    }
    int32_t chairId = pIServerUserItem->GetChairId();
//    pIServerUserItem->SetUserStatus(sOffline);
//    m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
    LOG(ERROR) << "OnUserLeft, chairId:" << chairId << ", bJoinGame:" << m_bJoinedGame[chairId]
                  << ", cbPlayStatus:" << m_bPlayStatus[chairId]
                     << ", cbGameStatus:" << m_cbGameStatus;


    if( (m_cbGameStatus != GAME_STATUS_END && m_cbGameStatus < GAME_STATUS_LOCK_PLAYING) || !m_bJoinedGame[chairId] )
    {
        LOG(ERROR)<<"---------------------------player->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << chairId;
        LOG(ERROR)<<"7OnUserLeft  ClearTableUser(i,true,true) userId="<<pIServerUserItem->GetUserId()<<"and chairID="<<pIServerUserItem->GetChairId();
        m_bJoinedGame[chairId] = false;
        m_bPlayStatus[chairId] = false;

        pIServerUserItem->SetUserStatus(sGetout);
        if( m_cbGameStatus < GAME_STATUS_LOCK_PLAYING )
        {
           m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);
           LOG(ERROR) << "ClearTableUser no join game, dwChairID:" << chairId;
        }
        m_pITableFrame->ClearTableUser(chairId, true, true);


        ret = true;
    }

    if(GAME_STATUS_READY == m_cbGameStatus)
    {
        int userNum=0;
        for(int i = 0; i < m_wBankerUser; ++i)
        {
            if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i]>0*/)
            {
                userNum++;
            }
        }

        if(userNum < (int)m_pGameRoomKind->minPlayerNum)
        {
            ClearAllTimer();

            m_cbGameStatus = GAME_STATUS_INIT;

            LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT;------------------------";
        }
    }
    LOG(ERROR)<<"---------------------------OnUserLeft1-----------m_wGameStatus----------------"<<(int)m_cbGameStatus;

    return ret;
}

void CGameProcess::GameTimerReadyOver() {
	//assert(m_cbGameStatus < GAME_STATUS_START);
	////////////////////////////////////////
	//销毁当前旧定时器
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);
	if (m_cbGameStatus >= GAME_STATUS_START) {
		return;
	}
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
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) {
			////////////////////////////////////////
			//桌子游戏人数不足，且机器人候补空位超时
			if (m_pITableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
				////////////////////////////////////////
				//达到最小游戏人数，开始游戏
				printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
				OnTimerGameReadyOver();
			}
			else {
				////////////////////////////////////////
				//仍然没有达到最小游戏人数，继续等待
				m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			////////////////////////////////////////
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
			//printf("--- *** --------- 匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", totalMatchSeconds_, MaxGamePlayers_);
			////////////////////////////////////////
			//定时器检测机器人候补空位后是否达到游戏要求人数
			m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::GameTimerReadyOver, this));
		}
	}
}

//准备定时器
void CGameProcess::OnTimerGameReadyOver()
{
	assert(m_cbGameStatus < GAME_STATUS_START);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);

    //    LOG(ERROR)<<"---------------------------GameTimerReadyOver---------------------------";

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            if(user->GetUserStatus() == sOffline)
            {
                //                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                m_pITableFrame->ClearTableUser(i, true, true);
                //                LOG(ERROR)<<"---------------------------ClearTableUse1r---------------------------";
            }else if(user->GetUserScore()<m_pGameRoomKind->enterMinScore)
            {
                //                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
                //                LOG(ERROR)<<"---------------------------ClearTableUser2---------------------------";
            }
        }
    }

    //获取玩家个数
    int wUserCount = 0;

    //用户状态
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        //获取用户
        if(m_pITableFrame->IsExistUser(i))
        {
            ++wUserCount;
        }

    }
    //    LOG(INFO)<<"  yijing diaoyong m_pITableFrame->StartGame()  PlayingCount:"<<wUserCount;
    if(wUserCount >= (int)(m_pGameRoomKind->minPlayerNum))
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
        //        LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_PLAYING GameTimereEnterPlayingOver ---------------------------";
        m_cbGameStatus = GAME_STATUS_LOCK_PLAYING;
        //m_TimerIdGameLockPlaying = m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_LOCK_PLAYING, boost::bind(&CGameProcess::OnTimerLockPlayingOver, this));
		OnTimerLockPlayingOver();
    }else
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                //                LOG(ERROR)<<"---------------------------user->SetUserStatus(sReady) ---------------------------";
                user->SetUserStatus(sReady);
            }
            m_bJoinedGame[i] = false;
            m_bPlayStatus[i] = false;
        }
        ClearAllTimer();
        m_cbGameStatus = GAME_STATUS_INIT;
        //        LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT---------------------------";
    }
}

void CGameProcess::OnTimerLockPlayingOver()
{
    LOG(ERROR) << "CGameProcess::OnTimerGameReady called.";

    {
        //        muduo::MutexLockGuard lock(m_mutexLock);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameLockPlaying);

        //所有玩家准备
        // OnAllUserReady();

        m_wPlayCount = 0;
        for(int i = 0; i < m_wBankerUser; ++i)
        {
            shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
            if(pIServerUserItem)
            {
                m_bJoinedGame[i] = true;
                m_bPlayStatus[i] = true;
                ++m_wPlayCount;

                if(pIServerUserItem->IsAndroidUser())
                {
                    m_cbAndroidStatus[i] = true;
                }else
                {
                    m_cbRealPlayer[i] = true;
                }
                shared_ptr<Hand> &hand = m_Hands[i];
                hand->userId = pIServerUserItem->GetUserId();

                m_llUserSourceScore[pIServerUserItem->GetUserId()]  = pIServerUserItem->GetUserScore();
                m_wUserBets[pIServerUserItem->GetUserId()] = 0;
                m_dwPlayUserID[i]       = pIServerUserItem->GetUserId();
                m_SeatToUser[m_dwPlayUserID[i]] = i;
                m_dwPlayFakeUserID[i]   = pIServerUserItem->GetUserId();
                pIServerUserItem->SetUserStatus(sPlaying);

            }else
            {
                m_bJoinedGame[i] = false;
                m_bPlayStatus[i] = false;
            }
        }

        if(m_wPlayCount < m_pGameRoomKind->minPlayerNum)
        {
            for(int i = 0; i < m_wBankerUser; ++i)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(user)
                {
                    LOG(ERROR)<<"---------------------------user->SetUserStatus(sReady) ---------------------------";
                    user->SetUserStatus(sReady);
                }
                m_bJoinedGame[i] = false;
                m_bPlayStatus[i] = false;
            }
            ClearAllTimer();
            m_cbGameStatus = GAME_STATUS_INIT;
            LOG(ERROR)<<"---------------------------m_cbGameStatus = GAME_STATUS_INIT---------------------------";
            return;
        }
    }

    {
        LOG(ERROR)<<"---------------------------GameTimereEnterPlayingOver----m_pITableFrame->StartGame()-----------------------";

        //开始游戏
        LOG(ERROR) << "pTableFrame:" << m_pITableFrame << ", OnTimerMessage IDI_GAME_READY, StartGame.";
        m_cbGameStatus = GAME_STATUS_PLAY;
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
        //        m_pITableFrame->OnGameStart();
        OnGameStart();
    }
}

//游戏开始
void CGameProcess::OnGameStart()
{
    m_GameIds = m_pITableFrame->GetNewRoundId();
    m_replay.clear();
    m_replay.gameinfoid = m_GameIds;
    m_nRoundID++;

    //清除所有定时器
    ClearAllTimer();
    m_dwReadyTime = (uint32_t)time(NULL);

    //读取配置文件
    ReadStorageInformation();
    //构造数据
    CMD_S_GameStart GameStart;

    GameStart.set_roundid(m_GameIds);
    GameStart.set_dcellscore(m_lCellScore);

    for(int i = 0; i < m_wBankerUser; ++i)
    {
        GameStart.add_cbplaystatus(m_bPlayStatus[i]?1:0);
        int64_t dUserScore = 0;
        if(m_bPlayStatus[i])
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            // get current user score item value.
            dUserScore = user->GetUserScore();//GetCurrentUserScore(i);
            m_replay.addPlayer(user->GetUserId(),user->GetAccount(),dUserScore,i);

            if(user->GetBlacklistInfo().status == 1 && user->GetBlacklistInfo().weight >= rand()%1000)
            {
                m_iBlacklistNum++;
                m_bBlackList[i] = true;
            }else
            {
                m_bBlackList[i] = false;
            }
        }

        // try to add the special user score.
        GameStart.add_duserscore(dUserScore);
    }

    uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
    GameStart.set_wtimeleft(wTimeLeft);
    m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
    m_wTotalOpTime = wTimeLeft;

    string content = GameStart.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_GAME_START, (uint8_t *)content.data(), content.size());

    m_replay.addStep(0,"",-1,opStart,-1,-1);
    //todo shuffle here
    //玩家下注定时器
    m_cbGameStatus = GAME_STATUS_BET;
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_TimerIdAddScore = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 2),boost::bind(&CGameProcess::OnTimerAddScore, this));
//    LOG(ERROR) << " >>> OnTimerAddScore, delay:" << (wTimeLeft+2);
}

// try to get the special player current score value item.
int64_t CGameProcess::GetCurrentUserScore(int wChairID)
{

//    LOG(INFO) << "GetCurrentUserScore, chairId:" << wChairID <<", m_nTableID" << m_nTableID;
    int64_t lCurrentScore = 0;
    // check if the chair id is validate for later uer item now.
    if(wChairID >= 0 && wChairID != m_wBankerUser && wChairID <= GAME_PLAYER*2)
    {
        shared_ptr<Hand> &hand = m_Hands[wChairID];
        LOG(INFO) << "GetCurrentUserScore, userid:" << hand->userId ;
        if(hand->userId == 0 )
        {
            LOG(ERROR) << "HEHE";
        }
        if( hand )
        {
            uint64_t usedscore = m_wUserBets[hand->userId];
            lCurrentScore = m_llUserSourceScore[hand->userId] - usedscore;//m_lTableScore[wChairID];
            if(lCurrentScore < 0)
            {
                lCurrentScore = 0;
            }
        }
       
    }

    //Cleanup:
    return (lCurrentScore);
}

//发送场景
bool CGameProcess::OnEventGameScene(uint32_t chairId, bool bIsLookUser)
{
    LOG(ERROR) << "OnEventGameScene called, dwChairID = " << chairId << ", nGameStatus:" << (int)m_cbGameStatus;

    //    muduo::MutexLockGuard lock(m_mutexLock);

    if(chairId >= GAME_PLAYER)
    {
        LOG(WARNING) << "OnEventGameScene: dwChairID = " << (int)chairId << "  err";
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

        for(int i = 0; i < GAME_PLAYER-1; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                ::Blackjack::HJK_PlayerItem* player = StatusFree.add_gameplayers();
                player->set_cbchairid(i);
                player->set_cbplaystatus(m_bPlayStatus[i]);
                player->set_szlocation(user->GetLocation());
                player->set_sznickname(user->GetNickName());
                player->set_llscore(user->GetUserScore());
                player->set_cbheadindex(user->GetHeaderId());
                player->set_wuserid(user->GetUserId());
                player->set_bshadow(false);
            }

        }

        //发送场景
        string content = StatusFree.SerializeAsString();
        m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_FREE, (uint8_t *)content.data(), content.size());

        break;
    }
    case GAME_STATUS_BET:
    {
        CMD_S_StatusScore StatusScore;
        StatusScore.set_roundid(m_GameIds);
        StatusScore.set_dcellscore(m_lCellScore);
        for(int i = 0; i < m_wBankerUser; ++i)
        {
            StatusScore.add_dbetscores(m_lTableScore[i]);
            StatusScore.add_bendscore(m_bSettled[i]);
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            shared_ptr<Hand> hand = m_Hands[i];
            if(hand->isactivated || user)
            {
                ::Blackjack::HJK_PlayerItem* player = StatusScore.add_gameplayers();
                player->set_cbchairid(i);
                if( hand->masterChair != -1)
                {
                    user = m_pITableFrame->GetTableUserItem(hand->masterChair);
                    player->set_bshadow(true);
                    player->set_cbplaystatus(m_bPlayStatus[hand->masterChair]);
                    player->set_wmasterchair(hand->masterChair);
                }else
                {
                    player->set_cbplaystatus(m_bPlayStatus[i]);
                    player->set_llscore(GetCurrentUserScore(i));
                    player->set_cbheadindex(user->GetHeaderId());
                    player->set_szlocation(user->GetLocation());
                    player->set_bshadow(false);
                }

                player->set_sznickname(user->GetNickName());
                player->set_wuserid(user->GetUserId());
            }
        }
        StatusScore.set_wtimeleft(m_dwOpEndTime - (int32_t)time(NULL));
        StatusScore.set_wtotaltime(m_wTotalOpTime);
        string content = StatusScore.SerializeAsString();
        m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_SCORE, (uint8_t *)content.data(), content.size());
        break;
    }
    case GAME_STATUS_INSURE:
    {
        CMD_S_StatusInsure StatusInsure;
        StatusInsure.set_roundid(m_GameIds);
        StatusInsure.set_dcellscore(m_lCellScore);
        for(int i = 0; i < m_wBankerUser; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            shared_ptr<Hand> hand = m_Hands[i];
            StatusInsure.add_cbinsurestatus(m_lUserInsure[i]);
            if(hand->isactivated)
            {
                ::Blackjack::HJK_PlayerItem* player = StatusInsure.add_gameplayers();
                player->set_cbchairid(i);
                if( hand->masterChair != -1)
                {
                    user = m_pITableFrame->GetTableUserItem(hand->masterChair);
                    player->set_cbplaystatus(m_bPlayStatus[hand->masterChair]);
                    player->set_bshadow(true);
                    player->set_wmasterchair(hand->masterChair);
                }else
                {
                    player->set_cbplaystatus(m_bPlayStatus[i]);
                    player->set_szlocation(user->GetLocation());
                    player->set_cbheadindex(user->GetHeaderId());
                    player->set_llscore(GetCurrentUserScore(i));
                    player->set_bshadow(false);
                }
                player->set_binsured(m_lUserInsure[i] == 1);
                player->set_sznickname(user->GetNickName());
                player->set_wuserid(user->GetUserId());

                ::Blackjack::HandItem* firstHand = player->mutable_cbfirsthand();
                firstHand->set_dtablejetton(m_lTableScore[i]);
                firstHand->add_cbhandcarddata(m_cbHandCardData[i][0]);
                firstHand->add_cbhandcarddata(m_cbHandCardData[i][1]);
            }
        }
        HandCard* bankerHand = StatusInsure.mutable_wbankerhand();
        bankerHand->set_wchairid(m_wBankerUser);
        bankerHand->add_cbhandcarddata(m_cbHandCardData[m_wBankerUser][0]);

        StatusInsure.set_wtimeleft(m_dwOpEndTime - (int32_t)time(NULL));
        StatusInsure.set_wtotaltime(m_wTotalOpTime);
        string content = StatusInsure.SerializeAsString();
        m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_INSURE, (uint8_t *)content.data(), content.size());
        break;
    }
    case GAME_STATUS_PLAY:	//游戏状态
    {
        m_bWaitEnterTable[chairId] = true;
        //构造数据
        CMD_S_StatusPlay StatusPlay;
        StatusPlay.set_roundid(m_GameIds);
        StatusPlay.set_dcellscore(m_lCellScore);
        
        StatusPlay.set_wcurrentuser(m_wCurrentUser);
        StatusPlay.set_wcurrentindex(m_currentHand->index);
        StatusPlay.set_wopercode(m_currentHand->opercode);
        for(int i = 0; i < m_wBankerUser; i++)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            shared_ptr<Hand> hand = m_Hands[i];
            if(hand->isactivated)
            {
                ::Blackjack::HJK_PlayerItem* player = StatusPlay.add_gameplayers();
                player->set_cbchairid(i);
                if( hand->masterChair != -1)
                {
                    user = m_pITableFrame->GetTableUserItem(hand->masterChair);
                    player->set_cbplaystatus(m_bPlayStatus[hand->masterChair]);
                    player->set_bshadow(true);
                    player->set_wmasterchair(hand->masterChair);
                }else
                {
                    player->set_cbplaystatus(m_bPlayStatus[i]);
                    player->set_szlocation(user->GetLocation());
                    player->set_cbheadindex(user->GetHeaderId());
                    player->set_llscore(GetCurrentUserScore(i));
                    player->set_wuserid(user->GetUserId());
                    player->set_bshadow(false);
                }
                player->set_binsured(m_lUserInsure[i] == 1);
                player->set_sznickname(user->GetNickName());
                
                ::Blackjack::HandItem* firstHand = player->mutable_cbfirsthand();
                firstHand->set_dtablejetton(hand->betscore);
                firstHand->set_cbhandcardponit(hand->cardpoint);
                firstHand->set_cbhandcardtype(hand->cardtype);
                firstHand->add_cbhandcarddata(m_cbHandCardData[i][0]);
                firstHand->add_cbhandcarddata(m_cbHandCardData[i][1]);
                uint8_t index = 2;
                while(index < hand->cardcount) {
                    firstHand->add_cbhandcarddata(m_cbHandCardData[i][index]);
                    index++;
                }

                hand = m_Hands[i+GAME_PLAYER];
                if(hand->isactivated)
                {
                    ::Blackjack::HandItem* secondHandItem = player->mutable_cbsecondhand();
                    secondHandItem->set_dtablejetton(hand->betscore);
                    secondHandItem->set_cbhandcardponit(hand->cardpoint);
                    secondHandItem->set_cbhandcardtype(hand->cardtype);
                    index = 0;
                    int handIndex = hand->index;
                    while(index < hand->cardcount) {
                        secondHandItem->add_cbhandcarddata(m_cbHandCardData[handIndex][index]);
                        index++;
                    }
                }
            }
        }

        //当前状态
        StatusPlay.set_wtimeleft(m_dwOpEndTime - (int32_t)time(NULL));
        StatusPlay.set_wtotaltime(m_wTotalOpTime);
        HandCard* bankerHand = StatusPlay.mutable_wbankerhand();
        bankerHand->set_wchairid(m_wBankerUser);
        bankerHand->add_cbhandcarddata(m_cbHandCardData[m_wBankerUser][0]);

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
        break;
    }

    default:
        break;
    }

    return true;
}

void CGameProcess::updateNext()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOperate);
    m_currentHand = m_currentHand->next;
    while( m_currentHand && (!m_currentHand->isactivated || m_currentHand->stopped))
    {
        m_currentHand = m_currentHand->next;
        if(m_currentHand)
        {
            LOG(INFO) << "Search next(index:"<<(int)(m_currentHand->index)<<",cardtype:"<<(int)(m_currentHand->cardtype)
                <<",isactive:"<<m_currentHand->isactivated<<",stoped:"<<m_currentHand->stopped<<");";
        }
    }
    if(m_currentHand != NULL)
    {
        LOG(INFO) << "Update next m_wCurrentUser:" << (int)m_currentHand->chairId<<" m_currenIndex" << (int)m_currentHand->index;
        m_wCurrentUser = m_currentHand->masterChair == -1 ? m_currentHand->chairId : m_currentHand->masterChair;
        uint32_t wTimeLeft = TIME_GAME_OPERATE;
        CMD_S_Operate OperInfo;
        m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
        m_wTotalOpTime = wTimeLeft;
        OperInfo.set_woperuser(m_wCurrentUser);
        OperInfo.set_wseat(m_currentHand->index);
        OperInfo.set_wopercode(m_currentHand->opercode);
        OperInfo.set_wtimeleft(wTimeLeft);
        string content = OperInfo.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPERATE, (uint8_t *)content.data(), content.size());
        m_TimerIdOperate = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1,boost::bind(&CGameProcess::OnTimerOperate, this));
    }else
    {
        BankerDeal();
    }
}

void CGameProcess::BankerDeal()
{
    LOG(INFO) << "Banker Deals : "<<reallyreallypoor;
    shared_ptr<Hand> &bankerHand = m_Hands[m_wBankerUser];
#if 0
    if(reallyreallypoor)
    {
        //todo
        vector<CompareDef> CompareInfos;
        CompareDef compareInfo;

        uint8_t targetCardType = 0;
        uint8_t targetCardPoint = 0;

        shared_ptr<Hand> hand = m_Hands[0];
        int64_t userWins = 0;
        LOG(WARNING) << "Check User Win";
        while(hand)
        {
            if(hand->isactivated && !m_cbAndroidStatus[hand->index])//m_cbRealPlayer & masterChair
            {
                if( hand->cardtype == CT_BUST)
                {
                    userWins -= hand->betscore;
                }else
                {
                    compareInfo.BetScore = hand->betscore;
                    compareInfo.CardPoint = hand->cardpoint;
                    compareInfo.CardType = hand->cardtype;
                    compareInfo.WinScore = m_GameLogic.GetCardTypeMultiple(hand->cardtype)*hand->betscore;
                    userWins += compareInfo.WinScore;
                    CompareInfos.push_back(compareInfo);
                }
            }
            hand = hand->next;
        }
        LOG(WARNING) << "User Win : " << (int)userWins;
        if(userWins > 0)
        {
            sort(CompareInfos.begin(),CompareInfos.end(),CompareByDef);
            for(vector<CompareDef>::iterator it = CompareInfos.begin(); it != CompareInfos.end(); ++it)
            {
                if(userWins - it->WinScore < 0)
                {
                    targetCardType = it->CardType;
                    targetCardPoint = it->CardPoint;
                    break;
                }
            }
            LOG(WARNING) << "TargetPoint " << (int)targetCardPoint << " ,TargetCardType:" <<(int)targetCardType;
            //only support CT_POINT: not avalible temprary
            if(targetCardPoint != 0 && targetCardType != 0)
            {
//                {
//                    if(bankerHand->stopped)
//                    {
//                        uint8_t ncard = m_cbHandCardData[m_wBankerUser][1];
//                        uint8_t ncardValue = m_GameLogic.GetCardLogicValue(ncard);
//                    }
//                }
            }
        }

    }
#endif

    while(bankerHand->cardpoint < 17 && !bankerHand->stopped)//cardpoint problem
    {
        m_cbHandCardData[m_wBankerUser][bankerHand->cardcount] = GetOneCard();
        bankerHand->cardcount++;
        m_GameLogic.UpdateAddOne(bankerHand,m_cbHandCardData[m_wBankerUser][bankerHand->cardcount-1]);
    }

    stringstream s;
    s << to_string(bankerHand->cardtype) << ":";
    s << to_string(bankerHand->cardpoint) << ":";
    s << GlobalFunc::converToHex(m_cbHandCardData[m_wBankerUser],bankerHand->cardcount);

    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,s.str(),-1,opShowCard,-1,-1);
    CMD_S_Banker_Deal DealInfo;
    DealInfo.set_cbhandcardtype(bankerHand->cardtype);
    uint8_t index = 1;
    while(index < bankerHand->cardcount)
    {
        DealInfo.add_cbhandcarddata((uint8_t)(m_cbHandCardData[m_wBankerUser][index]));
        index++;
    }
    string content = DealInfo.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_BANKER_DEAL, (uint8_t *)content.data(), content.size());
    OnEventGameConclude(INVALID_CHAIR,1);
}

void CGameProcess::OnTimerOperate()
{
    OnUserOperate(OPER_STOP);
}

void CGameProcess::OnTimerInsure()
{
    LOG(ERROR) << "OnTimerInsure................";
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdInsure);
    for(uint8_t i = 0; i < m_wBankerUser; i++)
    {
        //todo player have more than 1 hand
        if(m_cbGameStatus != GAME_STATUS_INSURE) break;
        shared_ptr<Hand> hand = m_Hands[i];
        LOG(ERROR) << "check insure status chairid: " << (int)i << ", isplaying: " << hand->isactivated << ", insurestatus: " << (int)(m_lUserInsure[i]);
        if( hand->isactivated && m_lUserInsure[i] == 0)
        {
            if(hand->masterChair == -1)
            {
                OnUserInsure(i,false,-1);
            }else
            {
                OnUserInsure(hand->masterChair,false,i);
            }
        }
    }
}

void CGameProcess::OnTimerAddScore()
{
//    LOG(ERROR) << " >>> OnTimerAddScore arrived.";
    
    // bool allBeted = true;
    //删除时间：下注失败怎么办
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    for(int i = 0; i < GAME_PLAYER - 1; i++)
    {
        LOG(ERROR) << "OnTimerAddScore chairid:"<< i << ", tablescore:" << m_lTableScore[i] << ", playstatus:" << m_bPlayStatus[i];
        if(m_lTableScore[i] == 0 && m_bPlayStatus[i])
        {
            //由于多了个下注完成的步骤，下面这个很有可能会进入不到
            // allBeted = false;
            OnUserAddScore(i, m_lCellScore,-1);
            // OnUserEndScore(i);
        }
    }
    // if ( allBeted )//玩家对应的位置都下注了，但是还有空位置没有下注，还有玩家没有点下注完成
    // {
        InitDealCards();
    // }
}


//游戏消息
bool CGameProcess::OnGameMessage(uint32_t wChairID, uint8_t dwSubCmdID, const uint8_t* pDataBuffer, uint32_t dwDataSize)
{

    CTestDump dump("OnGameMessage", wChairID, dwSubCmdID, m_nTableID);
    LOG(ERROR) << "OnGameMessage, wChairID:" << wChairID << ", dwSubCmdID:" << (int)dwSubCmdID << ",gameStatus:"<<(int)m_cbGameStatus;
    switch (dwSubCmdID)
    {
    case SUB_C_INSURE:
    {
        if(m_cbGameStatus != GAME_STATUS_INSURE)
        {
            sendErrorCode(wChairID,ERR_CODE_G_STATUS);
            return false;
        }

        //状态判断
        if(!m_bPlayStatus[wChairID])
        {
            sendErrorCode(wChairID,ERR_CODE_P_STATUS);
            return false;
        }
            

        CMD_C_Insure InsureInfo;
        if(!InsureInfo.ParseFromArray(pDataBuffer, dwDataSize))
        {
            sendErrorCode(wChairID,ERR_CODE_DATA);
            return false;
        }
        bool bought = InsureInfo.wbuy();
        int seat = -1;
        if(InsureInfo.has_dseat())
        {
            seat = InsureInfo.dseat();
        }
        //消息处理
        return OnUserInsure(wChairID,bought,seat);
    }
    case SUB_C_OPERATE:
    {
        //状态判断
        if(!m_bPlayStatus[wChairID])
        {
            sendErrorCode(wChairID,ERR_CODE_P_STATUS);
            return false;
        }
        if(m_cbGameStatus != GAME_STATUS_PLAY)
        {
            sendErrorCode(wChairID,ERR_CODE_G_STATUS);
            return false;
        }
        if(m_wCurrentUser != wChairID)
        {
            sendErrorCode(wChairID,ERR_CODE_TURN);
            return false;
        }
        CMD_C_Operate OperInfo;
        if(!OperInfo.ParseFromArray(pDataBuffer, dwDataSize))
        {
            sendErrorCode(wChairID,ERR_CODE_DATA);
            return false;
        }
        int32_t opercode = OperInfo.wopercode();
        if( (opercode & m_currentHand->opercode) == 0)
        {
            sendErrorCode(wChairID,ERR_CODE_OPER);
            return false;
        }
        //消息处理
        return OnUserOperate(opercode);

    }
    case SUB_C_ADD_SCORE:
    {
        if(m_cbGameStatus != GAME_STATUS_BET)
        {
            sendErrorCode(wChairID,ERR_CODE_G_STATUS);
            return false;
        }
        //状态判断
        if(!m_bPlayStatus[wChairID])
        {
            sendErrorCode(wChairID,ERR_CODE_P_STATUS);
            return false;
        }

        CMD_C_AddScore AddScore;
        if(!AddScore.ParseFromArray(pDataBuffer, dwDataSize))
        {
            sendErrorCode(wChairID,ERR_CODE_DATA);
            return false;
        }

        //当前状态
        int64_t dScore = AddScore.dscore();
        //if (pAddScore->wState > 0)m_lCompareCount = pAddScore->lScore;
        int betSeat = -1;
        if(AddScore.has_dseat())
        {
            betSeat = AddScore.dseat();
        }

        //消息处理
        return OnUserAddScore(wChairID, dScore, betSeat);
    }
    case SUB_C_END_SCORE:
    {
        //状态判断
        if(!m_bPlayStatus[wChairID])
        {
            sendErrorCode(wChairID,ERR_CODE_P_STATUS);
            return false;
        }
        return OnUserEndScore(wChairID);
    }
    break;
    default:
        LOG(ERROR) << "Unknown Commandid : " << (int)dwSubCmdID;
        break;

    }
    return false;
}

bool CGameProcess::OnUserInsure(int wChairID, bool buy,int betSeat )
{
    LOG(INFO) << "OnUserInsure chairID:" << wChairID << ", betSeat:" << betSeat;
    if(wChairID < 0 || wChairID >= m_wBankerUser)
    {
        sendErrorCode(wChairID,ERR_CODE_INSURE_SEAT);
        LOG(ERROR) << "Illegal insure seat chaird:" << wChairID ;
        return false;
    }
    if(betSeat != -1 &&  (betSeat < 0 || betSeat > m_wBankerUser))
    {
        sendErrorCode(wChairID,ERR_CODE_INSURE_SEAT);
        LOG(ERROR) << "Illegal insure  betseat:" << betSeat;
        return false;
    }
    if(betSeat != -1 && m_lUserInsure[betSeat] != 0)
    {
        sendErrorCode(wChairID,ERR_CODE_INSURED);
        LOG(ERROR) << "Duplicated Insure on seat : " << betSeat;
        return false;
    }
    if(betSeat == -1 && m_lUserInsure[wChairID] != 0)
    {
        sendErrorCode(wChairID,ERR_CODE_INSURED);
        LOG(ERROR) << "Duplicated Insure on seat : " << wChairID;
        return false;
    }

    shared_ptr<Hand> &hand = betSeat == -1 ? m_Hands[wChairID] : m_Hands[betSeat];
    int64_t userRestScore = GetCurrentUserScore(wChairID);
    if( buy )
    {
        if(userRestScore < hand->betscore/2)
        {
            sendErrorCode(wChairID,ERR_CODE_INSRE_LACK_MONEY);
            m_lUserInsure[hand->index] = 2;
            buy = false;
        }else
        {
            m_lUserInsure[hand->index] = 1;
            hand->insurescore = hand->betscore/2;
            m_wUserBets[hand->userId] =  m_wUserBets[hand->userId]+hand->insurescore;
        }
    }else{
        m_lUserInsure[hand->index] =  2;
    }
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(m_lUserInsure[hand->index]%2),-1,opBuyInsure,wChairID,hand->chairId);
    bool allInsured = true;
    bool someInsured = false;
    uint32_t wTimeLeft;
    for(int i = 0; i < m_wBankerUser; i++)
    {
        shared_ptr<Hand> hand = m_Hands[i];
        if(hand->isactivated){
            if( m_lUserInsure[i] == 0)
            {
                allInsured = false;
                
            }else if(m_lUserInsure[i] == 1)
            {
                someInsured = true;
            }
        } 
    }
    
    string content;

    CMD_S_Insure InsureInfo;
    InsureInfo.set_winsureuser(wChairID);//GetUserIdByChairId
    InsureInfo.set_bbought(buy);
    InsureInfo.set_wuserscore(GetCurrentUserScore(wChairID));
    if( betSeat != -1)
    {
        InsureInfo.set_wseat(betSeat);
    }else
    {
        InsureInfo.set_wseat(wChairID);
    }
    content = InsureInfo.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_INSURE, (uint8_t *)content.data(), content.size());

    //全部操作了保险
    if( allInsured )
    {

        CMD_S_Insure_Result InsureResult;
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdInsure);
        if (someInsured)
        {
            int secondValue = m_GameLogic.GetCardLogicValue(m_cbHandCardData[m_wBankerUser][1]);
            if(secondValue == 10)//庄家blackjack
            {
                InsureResult.set_bbankerbj(2);
                InsureResult.set_cbsecondcard(m_cbHandCardData[m_wBankerUser][1]);
                for(uint8_t i = 0; i < m_wBankerUser; i++)
                {
                    shared_ptr<Hand> hand = m_Hands[i];
                    if( hand->isactivated )
                    {
                        InsureResult.add_binsurescore( hand->insurescore != 0 ? hand->betscore : 0);
                    }else
                    {
                        InsureResult.add_binsurescore(0);
                    }
                }
                content = InsureResult.SerializeAsString();
                m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_INSURE_RESULT, (uint8_t *)content.data(), content.size());
                OnEventGameConclude(INVALID_CHAIR,2);
                return true;
            }else
            {
                m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,"",-1,opLkOrCall,-1,-1);
                InsureResult.set_bbankerbj(3);
                for(uint8_t i = 0; i < m_wBankerUser; i++)
                {
                    shared_ptr<Hand> hand = m_Hands[i];
                    if( hand->isactivated )
                    {
                        InsureResult.add_binsurescore( hand->insurescore != 0 ? -hand->insurescore : 0);
                    }
                    else
                    {
                        InsureResult.add_binsurescore(0);
                    }
                }
                content = InsureResult.SerializeAsString();
                m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_INSURE_RESULT, (uint8_t *)content.data(), content.size());
            }
        }else
        {
            InsureResult.set_bbankerbj(1);
            for(uint8_t i = 0; i < m_wBankerUser; i++)
            {
               InsureResult.add_binsurescore(0);
            }
            content = InsureResult.SerializeAsString();
            m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_INSURE_RESULT, (uint8_t *)content.data(), content.size());
        }
        //庄家不是blackjack或者没人购买保险   
        LOG(ERROR) << "No body insured or someone insured but banker is not blackjack";
        m_cbGameStatus = GAME_STATUS_PLAY;
        if(!m_currentHand->stopped)
        {
            //发送玩家操作信息
            wTimeLeft = TIME_GAME_OPERATE;
            m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
            m_wTotalOpTime = wTimeLeft;
            CMD_S_Operate OperInfo;
            OperInfo.set_woperuser(m_wCurrentUser);
            OperInfo.set_wopercode(m_currentHand->opercode);
            OperInfo.set_wseat(m_currentHand->index);
            OperInfo.set_wtimeleft(wTimeLeft);
            content = OperInfo.SerializeAsString();
            m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPERATE, (uint8_t *)content.data(), content.size());

            m_TimerIdOperate = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::OnTimerOperate, this));
        }else
        {
            LOG(ERROR) << "first hand is stopped.";
            updateNext();
        }
    }
    return true;
}

bool CGameProcess::OnUserOperate(uint8_t wOperCode)
{
//    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOperate);
    switch (wOperCode) {
    case OPER_CALL:
        return HandleCall();
        break;
    case OPER_DOUBLE:
        return HandleDouble();
        break;
    case OPER_SPLIT:
        return HandleSplit();
        break;
    case OPER_STOP:
        return HandleStop();
        break;
    default:
        return false;
    }
}

uint8_t CGameProcess::GetOneCard()
{
    uint8_t card;
    if(m_wCurrentCards.size() > 0)
    {
        card = m_wCurrentCards.back();
        m_wCurrentCards.pop_back();
        return card;
    }

    if(m_wDroppedCards.size() > 0)
    {
        card = m_wDroppedCards.back();
        m_wDroppedCards.pop_back();
        return card;
    }
    LOG(ERROR) << "OUT OF CONTROL....";
    return card;
}

void CGameProcess::dealOne()
{
    int32_t masterId = m_currentHand->masterChair != -1 ? m_currentHand->masterChair : m_currentHand->index;
    if(!reallyreallypoor || !m_cbRealPlayer[masterId] || m_bBlackList[masterId])
    {
        uint8_t curIndex = m_currentHand->cardcount;
        m_cbHandCardData[m_currentHand->index][curIndex] = GetOneCard(); //m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        m_currentHand->cardcount = curIndex+1;
        m_GameLogic.UpdateAddOne(m_currentHand,m_cbHandCardData[m_currentHand->index][curIndex]);
    }else
    {
        SpecialDealOne();
    }
}

void CGameProcess::SpecialDealOne()
{
    //if cardpoint > 17 bust
    uint8_t nextCard,nextCardValue,times;
    times = 0;
    LOG(WARNING) << "Trigger Special Deal";
    if(m_currentHand->cardpoint >= 17)
    {
        LOG(ERROR) << "Condition 0";
        nextCard = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        nextCardValue = m_GameLogic.GetCardLogicValue(nextCard);
        while( m_currentHand->cardpoint + nextCardValue <= 21)
        {
            m_wDroppedCards.push_back(nextCard);
            nextCard = GetOneCard();//m_wCurrentCards.back();
            nextCardValue = m_GameLogic.GetCardLogicValue(nextCard);
//            m_wCurrentCards.pop_back();
            times++;
            if(times >= 6) break;
        }
//        m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount] = nextCard;
//        m_currentHand->cardcount++;
//        m_GameLogic.UpdateAddOne(m_currentHand,m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount-1]);
    }else//else control cardpoint less than 18 or bust
    {
        LOG(ERROR) << "Condition 1";
        nextCard = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        nextCardValue = m_GameLogic.GetCardLogicValue(nextCard);
        while(true)
        {
            if(m_currentHand->cardcount < 4)
            {
                if(m_currentHand->hasA || nextCardValue == 1)//consider hasA
                {
                    if(m_currentHand->cardpoint + nextCardValue > 11 )//means +10 >= 22
                    {
                        if((m_currentHand->cardpoint + nextCardValue <= 18 || m_currentHand->cardpoint + nextCardValue > 21))
                        {
                            LOG(ERROR) << "Triggered 1.0 =====" << (int)nextCard <<" , value : " << (int)nextCardValue;
                            break;
                        }
                    }else
                    {
                        if(m_currentHand->cardpoint + nextCardValue <= 8)
                        {
                            LOG(ERROR) << "Triggered 1.1 =====" << (int)nextCard <<" , value : " << (int)nextCardValue;
                            break;
                        }
                    }
                }else
                {
                    if(m_currentHand->cardpoint + nextCardValue > 21 || m_currentHand->cardpoint + nextCardValue <= 18)
                    {
                        LOG(ERROR) << "Triggered 1.2 =====" << (int)nextCard <<" , value : " << (int)nextCardValue;
                        break;
                    }
                }
            }else//to exclude five dragon
            {
                if(m_currentHand->hasA || nextCardValue == 1)//consider hasA
                {
                    if(m_currentHand->cardpoint + nextCardValue > 11 )//means +10 >= 22
                    {
                        if(m_currentHand->cardpoint + nextCardValue > 21)
                        {
                            LOG(ERROR) << "Triggered 1.3 =====" << (int)nextCard <<" , value : " << (int)nextCardValue;
                            break;
                        }
                    }else
                    {
                        if(m_currentHand->cardpoint + nextCardValue >= 12)
                        {
                            LOG(ERROR) << "Triggered 1.4 =====" << (int)nextCard <<" , value : " << (int)nextCardValue;
                            break;
                        }
                    }
                }else
                {
                    if(m_currentHand->cardpoint + nextCardValue > 21)
                    {
                        LOG(ERROR) << "Triggered 1.5 =====" << (int)nextCard <<" , value : " << (int)nextCardValue;
                        break;
                    }
                }
            }
            m_wDroppedCards.push_back(nextCard);
            nextCard = GetOneCard();//m_wCurrentCards.back();
//            m_wCurrentCards.pop_back();
            nextCardValue = m_GameLogic.GetCardLogicValue(nextCard);
            times++;
            if(times >= 6)//avoid dead loop
            {
                break;
            }
        }

    }
    m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount] = nextCard;
    m_currentHand->cardcount++;
    m_GameLogic.UpdateAddOne(m_currentHand,m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount-1]);
}

bool CGameProcess::HandleDouble()
{
    int8_t realChair = m_wCurrentUser;
    if(m_currentHand->masterChair != -1 )
    {
        realChair = m_currentHand->masterChair;
    }
    int64_t lUserScore = GetCurrentUserScore(realChair);
    if(lUserScore < m_currentHand->betscore)
    {
        sendErrorCode(m_wCurrentUser,ERR_CODE_LACK_MONEY);
        return false;
    }
    m_wUserBets[m_currentHand->userId] = m_wUserBets[m_currentHand->userId] +  m_currentHand->betscore;
    m_currentHand->stopped = true;
    m_currentHand->doubled = true;
    m_currentHand->betscore = m_currentHand->betscore * 2;
    dealOne();

    if(m_currentHand->hasA && m_currentHand->cardpoint+10 <= 21)
    {
        m_currentHand->cardpoint += 10;
    }

    CMD_S_Oper_Result OperResult;
    OperResult.set_woperuser(m_wCurrentUser);
    OperResult.set_wopertype(OPER_DOUBLE);
    OperResult.set_woperseat(m_currentHand->index);
    OperResult.set_cbfirsthandcarddata(m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount-1]);
    OperResult.set_cbfirsthandcardtype(m_currentHand->cardtype);
    OperResult.set_cbscore(m_currentHand->betscore);
    OperResult.set_wuserscore(GetCurrentUserScore(realChair));
    string content = OperResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPER_RESULT, (uint8_t *)content.data(), content.size());
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,
                     GlobalFunc::converToHex(m_cbHandCardData[m_currentHand->index],1,m_currentHand->cardcount-1),-1,opDouble,realChair,m_currentHand->index);
    updateNext();
    return true;
}

bool CGameProcess::HandleSplit()
{
//    LOG(ERROR) << "Handle Split";
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOperate);
    int32_t realChair = m_wCurrentUser;

    uint8_t cardvalue0 = m_currentHand->cardpoint >> 1;
    uint8_t newCard,newCardValue,newPoints;
    if(m_currentHand->masterChair != -1 )
    {
        realChair = m_currentHand->masterChair;
    }
    int64_t lUserScore = GetCurrentUserScore(realChair);
    if(lUserScore < m_currentHand->betscore)
    {
        sendErrorCode(m_wCurrentUser,ERR_CODE_LACK_MONEY);
        return false;
    }
    m_wUserBets[m_currentHand->userId] = m_wUserBets[m_currentHand->userId] +  m_currentHand->betscore;
    
    m_currentHand->issplited = true;
    shared_ptr<Hand> &secondHand = m_Hands[m_currentHand->index + GAME_PLAYER];
    secondHand->isactivated = true;
    secondHand->betscore = m_currentHand->betscore;
    secondHand->userId = m_currentHand->userId;
    secondHand->masterChair = realChair;
    m_cbHandCardData[secondHand->index][0] = m_cbHandCardData[m_currentHand->index][1];

    newCard = GetOneCard();//m_wCurrentCards.back();
//    m_wCurrentCards.pop_back();
    if(reallyreallypoor)
    {
        LOG(ERROR) << "Trigger Poor Mode On Split, cardPoolLen:" << m_wCurrentCards.size();
        uint8_t times = 0;
        newCardValue = m_GameLogic.GetCardLogicValue(newCard);
        newPoints = cardvalue0 + newCardValue;
        while ( ((cardvalue0 != 1 && newCardValue != 1) && (newPoints >=18 && newPoints <=21)) || ( (cardvalue0 == 1 || newCardValue ==1) && (newPoints <= 11 || newPoints >= 8) )) {
            m_wDroppedCards.push_back(newCard);
            newCard = GetOneCard(); //m_wCurrentCards.back();
//            m_wCurrentCards.pop_back();
            times++;
            if(times >= 8)
            {
                break;
            }
        }

        m_cbHandCardData[m_currentHand->index][1] = newCard;
        LOG(ERROR) << "Trigger Poor Mode On Split, cardPoolLen:" << m_wCurrentCards.size() ;
        times = 0;
        newCard = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        newPoints = cardvalue0 + m_GameLogic.GetCardLogicValue(newCard);
        while ( ((cardvalue0 != 1 && newCardValue != 1) && (newPoints >=18 && newPoints <=21)) || ( (cardvalue0 == 1 || newCardValue ==1) && (newPoints <= 11 || newPoints >= 8) )) {
//            m_wCurrentCards.push_back(newCard);
            newCard = GetOneCard();//m_wCurrentCards.back();
//            m_wCurrentCards.pop_back();
            times++;
            if(times >= 8)
            {
                break;
            }
        }
        m_cbHandCardData[secondHand->index][1] = newCard;

    }else
    {
        m_cbHandCardData[m_currentHand->index][1] = newCard;
//        m_wCurrentCards.pop_back();
        m_cbHandCardData[secondHand->index][1] = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
    }

    m_GameLogic.UpdateInit(m_currentHand,m_cbHandCardData[m_currentHand->index]);
    m_GameLogic.UpdateInit(secondHand,m_cbHandCardData[secondHand->index]);

    CMD_S_Oper_Result OperResult;
    OperResult.set_woperuser(realChair);
    OperResult.set_woperseat(m_currentHand->index);
    OperResult.set_cbfirsthandcarddata( m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount-1]);
    OperResult.set_cbfirsthandcardtype(m_currentHand->cardtype);
    OperResult.set_cbsecondhandcarddata( m_cbHandCardData[secondHand->index][1]);
    OperResult.set_cbsecondhandcardtype(m_currentHand->cardtype);
    OperResult.set_wopertype(OPER_SPLIT);
    OperResult.set_wuserscore(GetCurrentUserScore(realChair));

//    if(!m_currentHand->stopped)//21dian
//    {
        OperResult.set_wopercode(m_currentHand->opercode);
        OperResult.set_wtimeleft(TIME_GAME_OPERATE);
         m_TimerIdOperate = m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_OPERATE+1,boost::bind(&CGameProcess::OnTimerOperate, this));
//    }
         m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,GlobalFunc::converToHex(m_cbHandCardData[m_currentHand->index],2),-1,opLeave,realChair,m_currentHand->index);
         m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,GlobalFunc::converToHex(m_cbHandCardData[secondHand->index],2),-1,opLeave,realChair,secondHand->index);

    string content = OperResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPER_RESULT, (uint8_t *)content.data(), content.size());
    if(m_currentHand->stopped)
    {
        updateNext();
    }
    return true;
}

bool CGameProcess::HandleCall()
{
//    LOG(ERROR) << "Handle Call";
    dealOne();
    CMD_S_Oper_Result OperResult;
    OperResult.set_woperuser(m_wCurrentUser);
    OperResult.set_woperseat(m_currentHand->index);
    OperResult.set_cbfirsthandcarddata( m_cbHandCardData[m_currentHand->index][m_currentHand->cardcount-1]);
    OperResult.set_cbfirsthandcardtype(m_currentHand->cardtype);
    OperResult.set_wopertype(OPER_CALL);
    OperResult.set_wopercode(m_currentHand->opercode);
    string content = OperResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPER_RESULT, (uint8_t *)content.data(), content.size());
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,
                     GlobalFunc::converToHex(m_cbHandCardData[m_currentHand->index],1,m_currentHand->cardcount-1),-1,opCall,m_wCurrentUser,m_currentHand->index);
    if(m_currentHand->stopped)
    {
        updateNext();
    }
    return true;
}

bool CGameProcess::HandleStop()
{
//     LOG(ERROR) << "Handle Stop";
    m_currentHand->stopped = true;
    if(m_currentHand->hasA && m_currentHand->cardpoint+10 <= 21)
    {
        m_currentHand->cardpoint += 10;
    }
    CMD_S_Oper_Result OperResult;
    OperResult.set_woperuser(m_wCurrentUser);
    OperResult.set_wopertype(OPER_STOP);
    OperResult.set_woperseat(m_currentHand->index);
    string content = OperResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPER_RESULT, (uint8_t *)content.data(), content.size());
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,"",-1,opStop,m_wCurrentUser,m_currentHand->index);
    updateNext();
    return true;
}

void CGameProcess::InitDealCards()
{
    //删除时间
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    if(m_cbGameStatus != GAME_STATUS_BET)
    {
        LOG(WARNING) << "Already started";
        return;
    }
    uint32_t wTimeLeft;

    //分发扑克 (洗牌)
    //todo need to be optimus
    string content;
    CMD_S_DealCard DealCard;
    shared_ptr<Hand> hand;
    m_GameLogic.RandCardList(m_wCurrentCards);
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        hand = m_Hands[i];

        if(!hand->isactivated && i != m_wBankerUser) continue;
//        hand->cardcount = 2;

        m_cbHandCardData[i][0] = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        m_cbHandCardData[i][1] = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        m_GameLogic.UpdateInit(hand,m_cbHandCardData[i] );
    }

    AnalyseStartCard();

    for(int i = 0; i < GAME_PLAYER; i++)
    {
        hand = m_Hands[i];
        if(i != m_wBankerUser)
        {
            DealCard.add_wrealchair(0);
        }

        if(!hand->isactivated && i != m_wBankerUser) continue;

        ::Blackjack::HandCard* handCard = DealCard.add_cbhandcards();
        handCard->set_wchairid(i);
        handCard->add_cbhandcarddata(m_cbHandCardData[i][0]);

        if(i != m_wBankerUser ) //庄家只显示一张牌，所以只给一个
        {
            handCard->add_cbhandcarddata(m_cbHandCardData[i][1]);
            handCard->set_cbhandcardtype(hand->cardtype);
            if(hand->masterChair != -1)
            {
                DealCard.set_wrealchair(i,hand->masterChair);
            }else
            {
                DealCard.set_wrealchair(i,hand->chairId);
            }
            m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,GlobalFunc::converToHex(m_cbHandCardData[i],2),-1,opFollow,i,i);
        }else{
            m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,GlobalFunc::converToHex(m_cbHandCardData[i],1),-1,opFollow,i,0);
        }
    }

    //当前用户
    m_wCurrentUser = 0;
    m_currentHand = m_Hands[m_wCurrentUser];
    while(!m_currentHand->isactivated )// m_currentHand->stopped 是否stopped在下面处理
    {
        m_wCurrentUser = (m_wCurrentUser + 1) % GAME_PLAYER;
        m_currentHand = m_Hands[m_wCurrentUser];
    }
   
    if( m_currentHand->masterChair != -1)
    {
        m_wCurrentUser = m_currentHand->masterChair;
    }


    uint8_t firstCard = m_GameLogic.GetCardLogicValue(m_cbHandCardData[m_wBankerUser][0]);
    if( firstCard == 1)
    {
        m_bInsured = true;
        DealCard.set_wneedinsure(true);
        wTimeLeft = TIME_GAME_INSURE;
        DealCard.set_wtimeleft(wTimeLeft);
        m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
        m_wTotalOpTime = wTimeLeft;
        m_cbGameStatus = GAME_STATUS_INSURE;
        content = DealCard.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_DEAL_CARD, (uint8_t *)content.data(), content.size());
        m_TimerIdInsure = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft,boost::bind(&CGameProcess::OnTimerInsure, this));
    }else
    {
        DealCard.set_wneedinsure(false);
        // if( firstCard == 10 )
        // {
        //     shared_ptr<Hand> bankerHand = m_Hands[m_wBankerUser];
        //     if(bankerHand->cardtype == CT_BLACKJACK)
        //     {
        //         DealCard.set_wtimeleft(0);
        //         content = DealCard.SerializeAsString();
        //         m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_DEAL_CARD, (uint8_t *)content.data(), content.size());
        //         OnEventGameConclude(INVALID_CHAIR,3);
        //         return;
        //     }
        // }

        m_cbGameStatus = GAME_STATUS_PLAY;
        content = DealCard.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_DEAL_CARD, (uint8_t *)content.data(), content.size());
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,"",-1,opLkOrCall,-1,-1);
        if(!m_currentHand->stopped)
        {
//            DealCard.set_wcurrentuser(m_wCurrentUser);
//            DealCard.set_wopercode(m_currentHand->opercode);
//            DealCard.set_wseat(m_currentHand->index);
            wTimeLeft = TIME_GAME_OPERATE ;
//            m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
//            m_wTotalOpTime = wTimeLeft;
//            DealCard.set_wtimeleft(wTimeLeft);
//            m_TimerIdOperate = m_TimerLoopThread->getLoop()->runAfter((wTimeLeft + 3),boost::bind(&CGameProcess::OnTimerOperate, this));
//            content = DealCard.SerializeAsString();
//            m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_DEAL_CARD, (uint8_t *)content.data(), content.size());

            CMD_S_Operate OperInfo;
            m_dwOpEndTime  = (uint32_t)time(NULL) + wTimeLeft;
            m_wTotalOpTime = wTimeLeft;
            OperInfo.set_woperuser(m_wCurrentUser);
            OperInfo.set_wseat(m_currentHand->index);
            OperInfo.set_wopercode(m_currentHand->opercode);
            OperInfo.set_wtimeleft(wTimeLeft);
            string content = OperInfo.SerializeAsString();
            m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OPERATE, (uint8_t *)content.data(), content.size());
            m_TimerIdOperate = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 3,boost::bind(&CGameProcess::OnTimerOperate, this));
        }else{//the first one blackjack
            LOG(ERROR) << "第一个玩家是blackjack";
//            DealCard.set_wtimeleft(0);
//            content = DealCard.SerializeAsString();
//            m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_DEAL_CARD, (uint8_t *)content.data(), content.size());
            updateNext();
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
            AndroidCard.set_lstockscore(0);//m_lStockScore);

            for(int i = 0; i < GAME_PLAYER; ++i)
            {
                AndroidCard.add_cbrealplayer(m_cbRealPlayer[i]);
                AndroidCard.add_cbandroidstatus(m_cbAndroidStatus[i]);
            }


            for(int i = 0; i < GAME_PLAYER; ++i)
            {
                for(int j = 0; j < MAX_COUNT; ++j)
                {
                    AndroidCard.add_cballhandcarddata(m_cbHandCardData[i][j]);
                }
            }

            // try to serial the special content as string.
            string content = AndroidCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
        }
    }
}

//如果所有在玩位置都下注完成就开始发牌
bool CGameProcess::OnUserEndScore(int wChairID )
{
    if(m_cbGameStatus != GAME_STATUS_BET)
    {
        //因为下注有可能导致游戏开始，所以这个判断移到这里来
//        sendErrorCode(wChairID,ERR_CODE_G_STATUS);
        LOG(INFO) << "Status Error, Current:" << (int)m_cbGameStatus;
        return false;
    }
    if (m_bSettled[wChairID])
    {
        sendErrorCode(wChairID,ERR_CODE_ALREADY_END_BET);
        return false;
    }
    if(m_lTableScore[wChairID] == 0 )
    {//自己的位置是优先下注的，自己的位置都没有下注就不能下注完成
        sendErrorCode(wChairID,ERR_CODE_NO_BETS);
        return false;
    }

    m_bSettled[wChairID] = true;

    CMD_S_EndScore EndScore;
    EndScore.set_woperuser(wChairID);
    string content = EndScore.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_END_SCORE, (uint8_t *)content.data(), content.size());

    bool allBeted = true;
    //发送数据
    for(int i = 0; i < m_wBankerUser; ++i)
    {
        if( m_bPlayStatus[i] &&  !m_bSettled[i])
        {
            allBeted = false;
            break;
        }
    }

    //if all beted then deal card
    if(allBeted)
    {
        InitDealCards();
    }
}

//加注事件:如果所有位置都下注了就开始发牌
bool CGameProcess::OnUserAddScore(int wChairID, int64_t lScore,int betSeat )
{
    LOG(INFO) << "OnUserAddScore chairId:" << wChairID << ", betSeat: " << betSeat;
    if(m_bSettled[wChairID])
    {
        sendErrorCode(wChairID,ERR_CODE_ALREADY_END_BET);
        return false;
    }
    if(wChairID < 0 || wChairID > m_wBankerUser)
    {
        LOG(ERROR) << "OnUserAddScore Error wChairID : " << (int)wChairID;
        sendErrorCode(wChairID,ERR_CODE_SCORE_SEAT);
        return false;
    }
    if(betSeat != -1 &&  (betSeat < 0 || betSeat > m_wBankerUser))
    {
        LOG(ERROR) << "OnUserAddScore Error wChairID : " << (int)wChairID << ", betSeat" << betSeat;
        sendErrorCode(wChairID,ERR_CODE_SCORE_SEAT);
        return false;
    }
    if(betSeat != -1 && (betSeat != wChairID ) )
    {
        //有人的位置或者别人已经下注的位置不给下
        if(m_pITableFrame->GetTableUserItem(betSeat) || m_Hands[betSeat]->isactivated)
        {
            sendErrorCode(wChairID,ERR_CODE_SCORE_SEAT);
            LOG(ERROR) << "Seat has been taken by others. betSeat: " << betSeat <<" ,betUser:" << (int)wChairID;
            return false;
        }

        if(m_lTableScore[wChairID] == 0)
        {
            sendErrorCode(wChairID,ERR_CODE_BETSELF);
            LOG(ERROR) << "Bet yourself seat. betSeat: " << betSeat <<" ,betUser:" << (int)wChairID;
            return false;
        }
    }

    //加注金币效验
    if ( lScore <= 0 )
    {
        sendErrorCode(wChairID,ERR_CODE_PARAM);
        LOG(ERROR) << "Error add score: " << lScore ;
        return false;
    }

    if(lScore < m_lCellScore)
    {
        sendErrorCode(wChairID,ERR_CODE_LACK_MONEY);
        LOG(ERROR) << "U R SO CHEEP," << betSeat;
        return false;
    }

    if(lScore > m_pGameRoomKind->ceilScore)
    {
        lScore = m_pGameRoomKind->ceilScore;
        sendErrorCode(wChairID,ERR_CODE_OVER_BET);
        LOG(ERROR) << "Over Betted";
//        return false;
    }

    int64_t lUserScore = GetCurrentUserScore(wChairID);
    
    if( lUserScore < lScore)
    {
        sendErrorCode(wChairID,ERR_CODE_LACK_MONEY);
        // try to add the special score content item value data for later user now.
        LOG(ERROR) << "Error user score: " << lUserScore;
        return false;
    }

    shared_ptr<Hand> hand = betSeat == -1 ? m_Hands[wChairID] : m_Hands[betSeat];

    if(hand->betscore != 0 )
    {
        sendErrorCode(wChairID,ERR_CODE_BETED);
        return false;
    }

    if(hand->userId == 0)
    {
        hand->userId = GetUserIdByChairId(wChairID);
    }
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lScore),-1,opBet,wChairID,hand->chairId);
    hand->betscore = lScore;
    hand->isactivated = true;
    m_wUserBets[hand->userId] =  m_wUserBets[hand->userId]+lScore;

    string log = to_string(lScore);
//    m_mOutCardRecord[wChairID].append(log.c_str());
    //用户注金
   
    m_lTableScore[hand->index] = lScore;
    if( betSeat != -1 && betSeat != wChairID)
    {
        hand->masterChair = wChairID;
    }

    LOG(ERROR) << "OnUserAddScore tableid:" << m_nTableID << ",userid:" << hand->userId
                << ",betSeat:" << betSeat << ", wChairID:" << wChairID
               << ", m_lTableScore[" << wChairID << "]=" << m_lTableScore[wChairID]
                  << ", lScore:" << lScore;
    //构造数据
    CMD_S_AddScore AddScore;
    AddScore.set_woperuser(wChairID);//GetUserIdByChairId
    AddScore.set_wjettonscore(lScore);
    AddScore.set_wuserscore(GetCurrentUserScore(wChairID));
    if(betSeat != -1)
    {
        AddScore.set_wseat(betSeat);
    }else
    {
        AddScore.set_wseat(wChairID);
    }
    string content = AddScore.SerializeAsString();
    //deal card has bug, because seat is not continous
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_ADD_SCORE, (uint8_t *)content.data(), content.size());


    bool allBeted = true;
    //发送数据
    for(int i = 0; i < m_wBankerUser; ++i)
    {
        if( m_lTableScore[i] == 0 )
        {
            allBeted = false;
            break;
        }
    }

    //if all beted then deal card
    if(allBeted)
    {
        InitDealCards();
    }

    return true;
}

//游戏结束GETage:1 正常结束, 2 保险结束, 3 庄家10A结束【已取消】
bool CGameProcess::OnEventGameConclude( uint32_t chairId,uint8_t GETag)
{
    m_cbGameStatus = GAME_STATUS_END;
    //结束时清理所有定时器
    ClearAllTimer();
    //定义变量
    CMD_S_GameEnd GameEnd;
    GameEnd.set_cbstate(GETag);
    string strTableScore = "tableid:" + to_string(m_nTableID) + ", round" + to_string(m_nRoundID) + ", OnEventGameConclude:";
    //计算总注
    int64_t lWinnerScore = 0;
    int64_t realWinScore = 0;
    int64_t SystemScore  = 0;
    //玩家输赢
    int64_t lGameScore[GAME_PLAYER*2] = {0};
    m_iUserWinScore.clear();

    shared_ptr<Hand> &bankerHand = m_Hands[m_wBankerUser];
    shared_ptr<Hand> playerHand ;
    float bankerMultiple;
    vector<tagScoreInfo> scoreInfoVec;
    
    map<uint8_t,uint8_t> chairToScore;
    map<uint8_t,uint8_t>::iterator it;
    int scoreIndex;
    tagScoreInfo scoreInfo;
    uint8_t opercode = 0;
    // uint64_t revenue = 0;
    string iStrs[GAME_PLAYER];
    switch(GETag)
    {
        case 1:
        {
            for(int i = 0; i < m_wBankerUser; ++i)
            {
                playerHand = m_Hands[i];
                if(!playerHand->isactivated)
                {
                    continue;
                }
                // revenue = 0;
                opercode = 0;
                UpdateScoreCode(playerHand,opercode);
                if(playerHand->masterChair == -1)
                {
                    it = chairToScore.find(playerHand->index);
                    iStrs[i].append(to_string(i)).append(to_string(opercode)).append(GlobalFunc::converToHex( m_cbHandCardData[i] ,playerHand->cardcount));
                }else
                {
                    it = chairToScore.find(playerHand->masterChair);
                    iStrs[i].append("|").append(to_string(i)).append(to_string(opercode)).append(GlobalFunc::converToHex( m_cbHandCardData[i] ,playerHand->cardcount)) ;
                }

                if( it == chairToScore.end())
                {
                    scoreIndex = -1;
                }else
                {
                    scoreIndex = (int)(it->second);
                }

//                LOG(INFO) << "Score Index:" << (int)scoreIndex;

                if(scoreIndex == -1)
                {
                    scoreInfo.clear();
                    scoreInfo.cellScore = {0,0,0,0,0};
                    scoreInfoVec.push_back(scoreInfo);
                    if(playerHand->masterChair == -1)
                    {
                        scoreIndex = chairToScore[playerHand->index] = scoreInfoVec.size()-1;
                        scoreInfoVec[scoreIndex].chairId = i;
                    }else
                    {
                        scoreIndex = chairToScore[playerHand->masterChair] = scoreInfoVec.size()-1;
                        scoreInfoVec[scoreIndex].chairId = playerHand->masterChair;
                    }
                    scoreInfoVec[scoreIndex].startTime = m_dwStartTime;
                }
                scoreInfoVec[scoreIndex].cellScore[i] = playerHand->betscore+playerHand->insurescore;
                scoreInfoVec[scoreIndex].betScore += playerHand->betscore+playerHand->insurescore;

                bankerMultiple = m_GameLogic.CompareCard(bankerHand, playerHand);
                lGameScore[i] = -bankerMultiple*playerHand->betscore - playerHand->insurescore;

                stringstream cardtype;
                cardtype << to_string(opercode)
                         << ":" << to_string(playerHand->cardtype)
                         << ":" << to_string(playerHand->cardpoint)
                         << ":" << GlobalFunc::converToHex( m_cbHandCardData[i] ,playerHand->cardcount);

                m_replay.addResult(scoreInfoVec[scoreIndex].chairId,playerHand->index,playerHand->betscore,lGameScore[i],cardtype.str(),false);

                scoreInfoVec[scoreIndex].addScore += lGameScore[i];

                strTableScore += "lGameScore[" + to_string(i) + "]=" + to_string(lGameScore[i]) + ", ";
                CMD_S_EndHand* hand = GameEnd.add_pendhands();

                hand->set_dchairid(i);
                
                
                if(playerHand->next->isactivated)
                {
                    opercode = 0;
                    playerHand = playerHand->next;
                    UpdateScoreCode(playerHand,opercode);

                    bankerMultiple = m_GameLogic.CompareCard(bankerHand, playerHand);
                    lGameScore[i+GAME_PLAYER] = -bankerMultiple*playerHand->betscore;

                    stringstream cardtype1;
                    cardtype1 << to_string(opercode)
                              << ":" << to_string(playerHand->cardtype)
                              << ":" << to_string(playerHand->cardpoint)
                              << ":" << GlobalFunc::converToHex( m_cbHandCardData[i+GAME_PLAYER] ,playerHand->cardcount);

                    m_replay.addResult(scoreInfoVec[scoreIndex].chairId,playerHand->index,playerHand->betscore,lGameScore[i+GAME_PLAYER],cardtype1.str(),false);

                    scoreInfoVec[scoreIndex].cellScore[i] += playerHand->betscore;
                    scoreInfoVec[scoreIndex].addScore += lGameScore[i+GAME_PLAYER];
                    scoreInfoVec[scoreIndex].betScore += playerHand->betscore+playerHand->insurescore;

                    hand->set_dscore( lGameScore[i]+lGameScore[i+GAME_PLAYER] );
                    iStrs[i].append("-").append(to_string(opercode)).append(GlobalFunc::converToHex( m_cbHandCardData[i+GAME_PLAYER] ,playerHand->cardcount));
                    
                    strTableScore += "lGameScore[" + to_string(i+GAME_PLAYER) + "]=" + to_string(lGameScore[i+GAME_PLAYER]) + ", ";
                }else
                {
                    hand->set_dscore( lGameScore[i] );
                }
            }
        }
        break;
        case 2:
        {
            for(int i = 0; i < m_wBankerUser; ++i)
            {
                playerHand = m_Hands[i];
                if(!playerHand->isactivated) continue;
                opercode = 0;
                UpdateScoreCode(playerHand,opercode);
                 if(playerHand->masterChair == -1)
                {
                    it = chairToScore.find(playerHand->index);
                    iStrs[i].append(to_string(i)).append(to_string(opercode)).append(GlobalFunc::converToHex( m_cbHandCardData[i] ,playerHand->cardcount));
                }else
                {
                    it = chairToScore.find(playerHand->masterChair);
                    iStrs[i].append("|").append(to_string(i)).append(to_string(opercode)).append(GlobalFunc::converToHex( m_cbHandCardData[i] ,playerHand->cardcount)) ;
                }
    
                if( it == chairToScore.end())
                {
                    scoreIndex = -1;
                }else
                {
                    scoreIndex = (int)(it->second);
                }

                if(scoreIndex == -1)
                {
                    scoreInfo.clear();
                    scoreInfo.cellScore = {0,0,0,0,0};
                    scoreInfoVec.push_back(scoreInfo);
                    if(playerHand->masterChair == -1)
                    {
                        scoreIndex = chairToScore[playerHand->index] = scoreInfoVec.size()-1;
                        scoreInfoVec[scoreIndex].chairId = i;
                    }else
                    {
                        scoreIndex = chairToScore[playerHand->masterChair] = scoreInfoVec.size()-1;
                        scoreInfoVec[scoreIndex].chairId = playerHand->masterChair;
                    }
                    scoreInfoVec[scoreIndex].startTime = m_dwStartTime;
                }
                scoreInfoVec[scoreIndex].cellScore[i] = playerHand->betscore+playerHand->insurescore;
                scoreInfoVec[scoreIndex].betScore += playerHand->betscore + playerHand->insurescore;
               
                CMD_S_EndHand* hand = GameEnd.add_pendhands();
                hand->set_dchairid(i);
                if(playerHand->insurescore != 0)
                {
                    if( playerHand->cardtype == CT_BLACKJACK)
                    {
                        scoreInfoVec[scoreIndex].addScore += playerHand->betscore;
                        hand->set_dscore(playerHand->betscore);
                    }else
                    {
                        hand->set_dscore(0);
                    }
                }else
                {
                    if( playerHand->cardtype != CT_BLACKJACK)
                    {
                        hand->set_dscore(-playerHand->betscore);
                        scoreInfoVec[scoreIndex].addScore -= playerHand->betscore;
                    }else
                    {
                        hand->set_dscore(0);
                    }

                }

                stringstream cardtype;
                cardtype << to_string(opercode)
                         << ":" << to_string(playerHand->cardtype)
                         << ":" << to_string(playerHand->cardpoint)
                         << ":" << GlobalFunc::converToHex( m_cbHandCardData[i] ,playerHand->cardcount);

                m_replay.addResult(scoreInfoVec[scoreIndex].chairId,playerHand->index,playerHand->betscore,lGameScore[i],cardtype.str(),false);

                strTableScore += "lGameScore[" + to_string(i) + "]=" + to_string(-scoreInfoVec[scoreIndex].addScore) + ", ";
            }
        }
        break;
    }

    iStrs[m_wBankerUser].append( to_string(m_wBankerUser)+"0" ).append( GlobalFunc::converToHex( m_cbHandCardData[m_wBankerUser] ,bankerHand->cardcount));
               
    for(int i = 0; i < m_wBankerUser; ++i)
    {
        playerHand = m_Hands[i];
        if(playerHand->masterChair != -1)
        {
            iStrs[playerHand->masterChair].append(iStrs[i]);
            iStrs[i] = "";
        }
    }
    string cardValue = "";

    for(int i = 0; i < m_wBankerUser; ++i)
    {
        if(iStrs[i] != "")
        {
            cardValue.append(iStrs[i]).append(",");
        }
    }
    cardValue.append(iStrs[m_wBankerUser]);

    int64_t userId,revenue;
    for(vector<tagScoreInfo>::iterator it0 = scoreInfoVec.begin(); it0 != scoreInfoVec.end(); it0++)
    {
        it0->cardValue = cardValue;
        it0->rWinScore = abs(it0->addScore);
        if(m_cbRealPlayer[it0->chairId])
        {
            if( it0->addScore > 0)
            {
                SystemScore  += -it0->addScore;
            }else{

                SystemScore = -it0->addScore;
            }
        }
        if(it0->addScore > 0)
        {
            //发送跑马灯信息
            if(it0->addScore>=m_pITableFrame->GetGameRoomInfo()->broadcastScore)
            {
                m_pITableFrame->SendGameMessage(it0->chairId,"",SMT_GLOBAL|SMT_SCROLL,it0->addScore);
            }
            it0->revenue = m_pITableFrame->CalculateRevenue(it0->addScore);
            it0->addScore -= it0->revenue;
        }
        userId = m_dwPlayUserID[it0->chairId];
        ::Blackjack::CMD_S_EndPlayer* player = GameEnd.add_pendplayers();
        player->set_duserid(userId);
        player->set_duserscore(it0->addScore + m_llUserSourceScore[userId]);
        player->set_dchairid(m_SeatToUser[userId]);
        player->set_dscore(it0->addScore);
        if(m_cbRealPlayer[it0->chairId])
        {
            if( it0->addScore > 0)
            {
                realWinScore += -it0->addScore-it0->revenue;

            }else{
//                revenue = m_pITableFrame->CalculateRevenue(-it0->addScore);
                realWinScore = -it0->addScore*99/100; //- revenue);

            }
        }
        m_replay.addResult(it0->chairId,-1,it0->betScore,it0->addScore,"",false);
        lWinnerScore += -it0->addScore;
    }

    GameEnd.set_dbankerwinscore(lWinnerScore);

    LOG(ERROR)    << strTableScore; // score.

    LOG(ERROR) << " ===================== end of tableid:" << (int)m_nTableID << ",round" << (int)m_nRoundID << "======================= ";

    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_GameIds);
    m_pITableFrame->SaveReplay(m_replay);
    // 写完分直接清零，这样OnUserLeft流程可以判断，直接走ClearTableUser流程。
    memset(m_lTableScore, 0, sizeof(m_lTableScore));

    if(m_pITableFrame)
    {
        double res=SystemScore;
        if(res>0)
        {
            res=res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
        }
        m_pITableFrame->UpdateStorageScore(res);
    }

    // 写赢家积分.
//    UpdateStorageScore();

    // int wTimeLeft = 2;

    LOG(ERROR) << "OnEventGameConclude nReason:" << (int)GETag;// << ", wTimeLeft:" << wTimeLeft;
    // m_dwOpEndTime = (uint32_t)time(NULL) + wTimeLeft;
    // m_wTotalOpTime = wTimeLeft;


    //    for(int i = 0; i < GAME_PLAYER; ++i)
    //    {
    string content = GameEnd.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_GAME_END, (uint8_t *)content.data(), content.size());
    //    }


    //游戏结束
    m_bContinueServer = m_pITableFrame->ConcludeGame(GAME_STATUS_END);
    m_pITableFrame->SetGameStatus(GAME_STATUS_END);


    //    LOG(ERROR) << "OnGameConclude after ConcludeGame, GameStatus:" << (int)m_cbGameStatus;

    //    m_mChairIdUserId.clear();
    for(int index=0;index<m_wBankerUser;index++)
    {
        m_dwPlayUserID[index] = -1;
    }

    OnTimerGameEnd();
//     m_TimerIdGameEnd = m_TimerLoopThread->getLoop()->runAfter(1, boost::bind(&CGameProcess::OnTimerGameEnd,  this));
    //    m_pITableFrame->DismissGame();
    return true;
}

void CGameProcess::OnTimerGameEnd()
{
    //    muduo::MutexLockGuard lock(m_mutexLock);
    //销毁定时器
     m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameEnd);

    LOG(INFO)<<"------------------------------------GameTimerEnd1------------------------------------";


    for(int i = 0; i < m_wBankerUser; i++)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            LOG(INFO)<<"3ConcludeGame=false  ClearTableUser(i,true,true) userId="<<user->GetUserId()<<" and chairID="<<user->GetChairId();
            user->SetUserStatus(sOffline);
            m_pITableFrame->ClearTableUser(i, true, true,ERROR_ENTERROOM_SERVERSTOP);
        }
    }


    //设置游戏状态
//    m_cbGameStatus = GAME_STATUS_READY;
        m_cbGameStatus = GAME_STATUS_INIT;

    //    LOG(INFO)<<"------------------------------------m_wGameStatus = GAME_STATUS_READY------------------------------------";

    //游戏结束
    //当前时间
    //    std::string strTime = Utility::GetTimeNowString();
    //    m_pITableFrame->ConcludeGame(GAME_STATUS1_FREE, strTime.c_str());
    // m_pITableFrame->ClearTableUser(INVALID_CHAIR, true);
    // m_TimerLoopThread->getLoop()->cancel(m_TimerIdGameReadyOver);
    //    m_TimerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_READY, boost::bind(&CGameProcess::OnTimerGameReadyOver,  this));
    ClearAllTimer();
    InitGameData();
    m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
}

//扑克分析
void CGameProcess::AnalyseStartCard()
{
    if(m_totalstockhighlimit <= 0 || m_totalstocklowerlimit <= 0)
    {
        LOG(ERROR) << "Error Limit";
        return;
    }
    shared_ptr<Hand> hand;
    shared_ptr<CServerUserItem> user;
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
        for(uint8_t i = 0; i < m_wBankerUser; i++)
        {
            hand = m_Hands[i];
            user = m_pITableFrame->GetTableUserItem(i);
            if(hand->isactivated && (m_cbRealPlayer[i] || (hand->masterChair != -1 && m_cbRealPlayer[hand->masterChair])) && IsGoodPai(hand))
            {
                ChangeGoodPai(hand);
            }
        }
    }
    else
    {
        int64_t gap = (m_totalstockhighlimit - m_totalstocklowerlimit) >> 1;
        int64_t aver = (m_totalstockhighlimit + m_totalstocklowerlimit) >> 1;
        LOG(WARNING) << "Gap:" << (int)gap << ", aver:" << (int)aver << ", current:" << (int)m_totalstock;
        int64_t rate = (m_totalstock-aver)*100/gap;
        int64_t randNum = GlobalFunc::RandomInt64(0,100);
        reallyreallypoor = rate < 0 &&  -rate >= randNum;
        reallyreallyrich = rate > 0 && rate >= randNum;
        LOG(INFO) << "Rate : " << (int)rate << ", rand:" << (int)randNum << ", poor:" << reallyreallypoor << ", rich:" << reallyreallyrich;

        if(reallyreallypoor || m_iBlacklistNum != 0)
        {
            m_poorTimes++;
            for(uint8_t i = 0; i < m_wBankerUser; i++)
            {
                hand = m_Hands[i];
                user = m_pITableFrame->GetTableUserItem(i);
                if(hand->isactivated && (m_cbRealPlayer[i] || (hand->masterChair != -1 && m_cbRealPlayer[hand->masterChair])) && IsGoodPai(hand))
                {
                    if(reallyreallypoor || m_bBlackList[i])//user->GetBlacklistInfo().status == 1)
                    {
                        ChangeGoodPai(hand);
                    }

                }
            }
        }

        if(reallyreallyrich)
        {
            m_richTimes++;
            ChangeGoodPai(m_Hands[m_wBankerUser]);
        }
        LOG(INFO) << "Rich Times:" << m_richTimes <<", Poor Times:" << m_poorTimes;
    }

}


void CGameProcess::ChangeGoodPai(shared_ptr<Hand> &hand)
{
    LOG(WARNING) << "Before : " << (int)hand->cardtype << ",point" << (int)hand->cardpoint;
    uint8_t cardIndex = hand->index;
    uint8_t card0, card1,card2;

    // >= 18: A+7 ~ 9+9 Change The Bigger one
    card1 = GetOneCard();//m_wCurrentCards.back();
    uint8_t times = 0;
//    m_wCurrentCards.pop_back();
    card2 = m_GameLogic.GetCardLogicValue(card1);
    while(card2 == 1 || card2 >= 7)
    {
//        m_wCurrentCards.push_back(card1);//todo  bug..
        card1 = GetOneCard();//m_wCurrentCards.back();
//        m_wCurrentCards.pop_back();
        card2 = m_GameLogic.GetCardLogicValue(card1);
        times++;
        if(times >= 5) break;
    }

    card0 = m_GameLogic.GetCardLogicValue(m_cbHandCardData[cardIndex][0]);
    if(card0 == 1 || card0 >= 8)
    {
        LOG(WARNING) << "FROM : " << int(card0);
        m_cbHandCardData[cardIndex][0] = card1;
    }else
    {
        LOG(WARNING) << "FROM : " << int(m_cbHandCardData[cardIndex][1]);
        m_cbHandCardData[cardIndex][1] = card1;
    }
    LOG(WARNING) << "TO : " << int(card1);
    m_GameLogic.UpdateInit(hand,m_cbHandCardData[cardIndex]);
    LOG(WARNING) << "After : " << (int)hand->cardtype << ",point" << (int)hand->cardpoint;
}

// 对子及以上为好牌.
bool CGameProcess::IsGoodPai(shared_ptr<Hand> &hand)
{
    return hand->cardtype > CT_POINTS || hand->cardpoint >= 17 || (hand->hasA && hand->cardpoint >= 8);
}

//更新REDIS库存.
//void CGameProcess::UpdateStorageScore()
//{
//    if(m_pITableFrame)
//    {
//        m_pITableFrame->UpdateStorageScore(m_lCurrStockScore);
//        m_lCurrStockScore = 0;
//    }
//}

//读取配置
void CGameProcess::ReadStorageInformation()
{
    //读取配置ID.
    uint32_t nConfigId = 0;
    if(m_pGameRoomKind)
    {
        nConfigId = m_pGameRoomKind->roomId;
    }

    //设置文件名
    string strRoomName = "GameServer_" + to_string(nConfigId);
    //=====config=====
    if(!boost::filesystem::exists(INI_FILENAME))
    {
        LOG_INFO << INI_FILENAME <<" not exists";
        return;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(INI_FILENAME, pt);

    //    m_lAgentRevenueRatio = pt.get<double>("Global.AgentRevenueRatio", 0.025);
    m_lMarqueeMinScore = pt.get<double>(strRoomName+".MarqueeMinScore", 1000);
    //m_recudeRatio27 = pt.get<int>(strRoomName+".ReduceRatio", 0);
    stockWeak=pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    //读取库存配置
    tagStorageInfo storageInfo = {0};
    if(m_pITableFrame)
    {
        m_pITableFrame->GetStorageScore(storageInfo);
    }

    // initialize the storage value item now.
    m_totalstock = storageInfo.lEndStorage;
    m_totalstocklowerlimit = storageInfo.lLowlimit;
    m_totalstockhighlimit = storageInfo.lUplimit;
    m_wSystemAllKillRatio = storageInfo.lSysAllKillRatio;
    m_wSystemChangeCardRatio = storageInfo.lSysChangeCardRatio;

}


int64_t CGameProcess::GetUserIdByChairId(int32_t chairId)
{
    return m_dwPlayUserID[chairId];
    //   shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    //    if(pIServerUserItem)
    //    {
    //        return pIServerUserItem->GetUserId();
    //    }
    //    return -1;
    //     m_mChairIdUserId
    //        map<int32_t, int64_t>::iterator iter =m_mChairIdUserId.find(chairId);
    //        if(iter!=m_mChairIdUserId.end()) return m_mChairIdUserId[chairId];
    //        return -1;
}


void CGameProcess::sendErrorCode(uint8_t seat, int errorCode)
{
    CMD_S_ErrorInfo ErrorInfo;
    ErrorInfo.set_wcode(errorCode);
    string content = ErrorInfo.SerializeAsString();
    m_pITableFrame->SendTableData(seat, SUB_S_ERROR_INFO, (uint8_t *)content.data(), content.size());
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

