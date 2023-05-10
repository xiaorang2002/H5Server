#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

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
//#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
//#include "pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/Qznn.Message.pb.h"

#include "CMD_Game.h"
#include "GameLogic.h"
#include "TestCase.h"

using namespace Qznn;

#include "TableFrameSink.h"

//////////////////////////////////////////////////////////////////////////
//定时器时间
#define TIME_READY						1					//准备定时器(2S)
#define TIME_ENTER_PLAYING              1
#define TIME_CALL_BANKER				5					//叫庄定时器(5S)
#define TIME_ADD_SCORE					5					//下注定时器(5S)
#define TIME_OPEN_CARD					5					//开牌定时器(5S)
#define TIME_GAME_END					1					//结束定时器(5S)
#define TIME_MATCH_GAME_END             2				//结束定时器(5S)
//frontend got a animation less then 5s,so after animation they can click the restart button

//////////////////////////////////////////////////////////////////////////////////////
uint8_t CTableFrameSink::m_cbBankerMultiple[MAX_BANKER_MUL] = { 1,2,4 };		//叫庄倍数
uint8_t CTableFrameSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 5,10,15,20 };	//下注倍数

int64_t CTableFrameSink::m_llStorageControl = 0;
int64_t CTableFrameSink::m_llTodayStorage = 0;
int64_t CTableFrameSink::m_llStorageLowerLimit = 0;
int64_t CTableFrameSink::m_llStorageHighLimit = 0;
bool  CTableFrameSink::m_isReadstorage = false;
bool  CTableFrameSink::m_isReadstorage1 = false;
bool  CTableFrameSink::m_isReadstorage2 = false;

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/qznn";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("qznn");
        // set std output special log content value.
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit;	// 声明全局变局,初始化库.


CTableFrameSink::CTableFrameSink(void) :  m_dwCellScore(1.0), m_pITableFrame(NULL), m_iTodayTime(time(NULL))
  ,m_isMatch(false)
{
    m_wGameStatus = GAME_STATUS_INIT;
    //清理游戏数据
    ClearGameData();
    //初始化数据
    InitGameData();

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        PaoMaDengCardType[i]=-1;
        PaoMaDengWinScore[i]=-1;
        m_bPlayerIsOperation[i] = false;
    }

    m_wGameStatus = GAME_STATUS_INIT;
    m_nLottery = 0;
    m_bContinueServer = true;
    if(ENABLE_TEST)
    {
        testCase = make_shared<CTestCase>();
    }
//    memset(m_SitTableFrameStatus, 0, sizeof(m_SitTableFrameStatus));
//    m_TimerLoopThread.startLoop();
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

	MaxGamePlayers_ = 0;
    stockWeak=0.0f;
}

CTableFrameSink::~CTableFrameSink(void)
{

}
void CTableFrameSink::InitLoadini()
{
    if(!boost::filesystem::exists("./conf/qznn_config.ini"))
    {
        LOG_ERROR<<"No config";
        return;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/qznn_config.ini",pt);

    stockWeak=pt.get<double>("GAME_CONF.STOCK_WEAK",1.0);
    ratioGamePlayerWeight_[0] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_0",10);
    ratioGamePlayerWeight_[1] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_1",10);
    ratioGamePlayerWeight_[2] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_2",10);
    ratioGamePlayerWeight_[3] = pt.get<int64_t>("GAME_CONF.PLAYER_CONF_3",10);
}
//设置指针
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    assert(NULL != pTableFrame);
    m_pITableFrame = pTableFrame;
    if(m_pITableFrame){
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();        
        uint8_t jsize = m_pGameRoomInfo->jettons.size();
        for(uint8_t i=0; i < jsize; i++)
        {
            m_cbJettonMultiple[i] = (uint8_t)m_pGameRoomInfo->jettons[i];
        }
//        memcpy(m_cbJettonMultiple, (vector<uint8_t>)m_pGameRoomInfo->jettons, sizeof(m_cbJettonMultiple));
        m_dwCellScore = m_pGameRoomInfo->floorScore;
        m_replay.cellscore = m_dwCellScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        if(m_pGameRoomInfo->roomId - m_pGameRoomInfo->gameId*10 > 20)
            m_isMatch=true;

    }
    //初始化游戏数据
    m_wGameStatus = GAME_STATUS_INIT;

    memset(ratioGamePlayerWeight_,0,sizeof(ratioGamePlayerWeight_));

    //更新游戏配置
    InitGameData();
    //更新游戏配置
    InitLoadini();

    //计算桌子要求标准游戏人数
    poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
    return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData()
{
    //庄家
    m_wBankerUser = INVALID_CHAIR;
    //当前筹码
//    memset(m_cbCurJetton, 0, sizeof(m_cbCurJetton));
}

//初始化游戏数据
void CTableFrameSink::InitGameData()
{
    //随机种子
    srand((unsigned)time(NULL));

//  //游戏状态
//    m_wGameStatus = GAME_STATUS_INIT;
    g_nRoundID = 0;
    //游戏中玩家个数
    m_wPlayCount = 0;
    //游戏中的机器的个数
    m_wAndroidCount = 0;
    m_bContinueServer = true;
    //游戏中玩家
    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
    //牌数据
    memset(m_cbCardData, 0, sizeof(m_cbCardData));
    //牛牛牌数据
    memset(m_cbOXCard, 0, sizeof(m_cbOXCard));

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_sCallBanker[i] = -1;								//玩家叫庄(-1:未叫, 0:不叫, 124:叫庄)
        m_sAddScore[i] = -1;								//玩家下注(-1:未下注, 大于-1:对应筹码索引)
        m_sCardType[i] = -1;								//玩家开牌(-1:未开牌, 大于-1:对应牌型点数)
        m_iMultiple[i] = 1;									//倍数

        m_bPlayerIsOperation[i] = false;
    }

    //时间初始化
    m_dwReadyTime = 0;										//ready时间
    m_dwEnterPlayingTime = 0;						        //EnterPlaying时间
    m_dwCallTime = 0;										//叫庄时间
    m_dwScoreTime = 0;										//下注时间
    m_dwOpenTime = 0;										//开牌时间
    m_dwEndTime = 0;										//结束时间
//    m_dwStartTime = 0;										//开始时间

    //基础积分
    if(m_pITableFrame)
    {
        m_dwCellScore = m_pGameRoomInfo->floorScore;//m_pITableFrame->GetGameCellScore();
    }

//    memcpy(m_cbCurJetton, m_cbJettonMultiple, sizeof(m_cbJettonMultiple));
    m_cbLeftCardCount = MAX_CARD_TOTAL;
    memset(m_cbRepertoryCard, 0, sizeof(m_cbRepertoryCard));
    //游戏记录
//    memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_bPlayerIsOperation[i]=false;
    }

    m_replay.clear();
    m_iBlacklistNum = 0;
}

void CTableFrameSink::PaoMaDeng()
{

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        if(m_cbPlayStatus[i] != 1)
            continue;
        if(PaoMaDengWinScore[i] > m_pITableFrame->GetGameRoomInfo()->broadcastScore)
        {
            int sgType = SMT_GLOBAL|SMT_SCROLL;
            m_pITableFrame->SendGameMessage(i, "", sgType, PaoMaDengWinScore[i]);
        }

    }
}

int CTableFrameSink::IsMaxCard(int wChairID)
{
    int isMaxCard = 1;

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i] || i == wChairID)
            continue;
        if(!m_GameLogic.CompareCard(m_cbCardData[wChairID], m_cbCardData[i]))
            isMaxCard = 0;
    }
    return isMaxCard;
}

void CTableFrameSink::OnTimerGameReadyOver()
{
	//assert(m_wGameStatus < GAME_STATUS_START);
    ////////////////////////////////////////
	//销毁当前旧定时器
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
	if (m_wGameStatus >= GAME_STATUS_START) {
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
			m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) {
			////////////////////////////////////////
			//桌子游戏人数不足，且机器人候补空位超时
			if (m_pITableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
				////////////////////////////////////////
				//达到最小游戏人数，开始游戏
//				printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
				GameTimerReadyOver();
			}
			else {
				////////////////////////////////////////
				//仍然没有达到最小游戏人数，继续等待
				m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			////////////////////////////////////////
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
//			printf("--- *** --------- 匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", totalMatchSeconds_, MaxGamePlayers_);
			////////////////////////////////////////
			//定时器检测机器人候补空位后是否达到游戏要求人数
			m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		}
	}
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    ////////////////////////////////////////
    //初始化或空闲准备阶段，可以进入桌子
    if(m_wGameStatus == GAME_STATUS_END){
        return false;
    }
    if (m_wGameStatus < GAME_STATUS_LOCK_PLAYING)
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
//                printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, pUser->GetUserId(), totalMatchSeconds_);
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
//                        printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, pUser->GetUserId(), totalMatchSeconds_);
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
    if(m_wGameStatus > GAME_STATUS_LOCK_PLAYING )
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
        return pIServerUserItem != NULL;
    }
    ////////////////////////////////////////
    //游戏状态为GAME_STATUS_LOCK_PLAYING(100)时，不会进入该函数
    assert(false);
    return false;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG(ERROR) << "OnUserLeft of userid:" << userId << ", get chair id failed, pIServerUserItem==NULL";
        return true;
    }
//    uint32_t chairId = pIServerUserItem->GetChairId();
    if( m_wGameStatus < GAME_STATUS_LOCK_PLAYING || m_wGameStatus == GAME_STATUS_END)// ||  !m_bJoinedGame[chairId]
        return true;
    else
        return false;
}


//发送场景
bool CTableFrameSink::OnUserEnter(int64_t userId, bool bisLookon)
{
	//muduo::MutexLockGuard lock(m_mutexLock);

	LOG(ERROR) << "---------------------------OnUserEnter---------------------------";

	uint32_t chairId = INVALID_CHAIR;

	shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);

	if (!pIServerUserItem)
	{
		LOG(ERROR) << "-----------------OnUserEnter--Fail----pIServerUserItem==NULL----------------";
		return false;
	}

	chairId = pIServerUserItem->GetChairId();
	LOG(ERROR) << "-------------OnUserEnter------user->GetChairID()=" << pIServerUserItem->GetChairId() << "---------------------";
	if (chairId >= GAME_PLAYER)
	{
		LOG(ERROR) << "---------------------------OnUserEnter--Fail----chairId=" << chairId << "---------------------";
		return false;
	}

	//if (m_wGameStatus == GAME_STATUS_INIT) {
	if (m_pITableFrame->GetPlayerCount(true) == 1) {
		////////////////////////////////////////
		//第一个进入房间的也必须是真人
        assert(m_isMatch||m_pITableFrame->GetPlayerCount(false) == 1);
		////////////////////////////////////////
		////////////////////////////////////////

        int sum = 0;
        int index=0;
        for (int i = 0; i < GAME_PLAYER; ++i) {
            sum += ratioGamePlayerWeight_[i];
        }
        if (sum <= 1) {
            index = 0;
        }
        int r=0;
        if(sum>0)
         r= 1+rand()%(sum); //m_random.betweenInt(1,sum).randInt_mt(true) ;
         int c = r;
        for (int i = 0; i < GAME_PLAYER; ++i) {
            c -= ratioGamePlayerWeight_[i];
            if (c <= 0) {
                index = i+1;
                break;
            }
        }
        if(index<2)
        {
            index=2;
        }
        MaxGamePlayers_=index;
		//重置累计时间
		totalMatchSeconds_ = 0;
//		printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 初始化游戏人数 MaxGamePlayers_：%d\n", MaxGamePlayers_);
	}
	if (m_wGameStatus < GAME_STATUS_START) {
		assert(MaxGamePlayers_ >= MIN_GAME_PLAYER);
		assert(MaxGamePlayers_ <= GAME_PLAYER);
	}
    m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
    m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
    m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);

    if(m_wGameStatus != GAME_STATUS_INIT )
    {
       OnEventGameScene(chairId,false);
    }
//    m_SitTableFrameStatus[dwChairID] = 2;

    if(m_wGameStatus < GAME_STATUS_READY)
    {
        int  wUserCount = 0;
        //用户状态
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            //获取用户
            shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(i);
            if(pIServerUser/* && m_SitTableFrameStatus[i] > 0*/)
            {
                ++wUserCount;
            }
        }
        LOG(ERROR)<<"---------Player iGAME_STATUS_INIT---------------------"<<m_pITableFrame->GetTableId()<<" wUserCount"<<wUserCount;
        if( wUserCount >= m_pGameRoomInfo->minPlayerNum)
        {
            ClearAllTimer();

            m_wGameStatus = GAME_STATUS_READY;
            LOG(ERROR)<<"-----------------m_wGameStatus == GAME_STATUS_READY && wUserCount >= "<< m_pGameRoomInfo->minPlayerNum << ",tableid:"<< m_pITableFrame->GetTableId();
            m_dwStartTime =  chrono::system_clock::now();//(uint32_t)time(NULL);
            m_dwReadyTime = (uint32_t)time(NULL);

            //m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::GameTimerReadyOver, this));
            LOG(ERROR)<<"---------------------------m_TimerIdReadyOver Start 1S---------------------------";
        }
    }

    if(m_wGameStatus == GAME_STATUS_LOCK_PLAYING)
    {
        pIServerUserItem->SetUserStatus(sReady);
    }

    LOG(ERROR)<<"---------------------------OnUserEnter-----m_wGameStatus----------------------"<<m_wGameStatus;
	if (m_wGameStatus < GAME_STATUS_START) {
		////////////////////////////////////////
		//达到桌子要求游戏人数，开始游戏
		if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			////////////////////////////////////////
			//不超过桌子要求最大游戏人数
			assert(m_pITableFrame->GetPlayerCount(true) == MaxGamePlayers_);
//			printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
			//GameTimereEnterPlayingOver();
			GameTimerReadyOver();
		}
		else {
			if (m_pITableFrame->GetPlayerCount(true) == 1) {
				////////////////////////////////////////
				//第一个进入房间的也必须是真人
                assert(m_isMatch||m_pITableFrame->GetPlayerCount(false) == 1);
				ClearAllTimer();
				////////////////////////////////////////
				//修改游戏准备状态
				//m_pITableFrame->SetGameStatus(GAME_STATUS_READY);
				//m_cbGameStatus = GAME_STATUS_READY;
				m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
			}
		}
	}

    return true;
}
/*
bool CTableFrameSink::RepayRecordGameScene(int64_t chairid, bool bisLookon)
{
    uint32_t dwChairID = chairid;
    shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(chairid);
    if(!pIServerUser)
    {
        return true;
    }

    dwChairID = pIServerUser->GetChairId();
    if(dwChairID >= GAME_PLAYER || dwChairID != chairid)
    {
        return true;
    }
    switch (m_wGameStatus)
    {
    case GAME_STATUS_INIT:
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_LOCK_PLAYING:
    {
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));//单元分


        uint32_t dwStartTime = 0;
        if(m_wGameStatus == GAME_STATUS_INIT)
            dwStartTime = m_dwInitTime;
        else if(m_wGameStatus == GAME_STATUS_READY)
            dwStartTime = m_dwReadyTime;
        else
            dwStartTime = m_dwEnterPlayingTime;

        uint32_t dwNowTime = (uint32_t)time(NULL);
        uint32_t cbReadyTime = 0;

        uint32_t dwCallTime = dwNowTime - dwStartTime;
        cbReadyTime = dwCallTime >= TIME_READY ? 0 : dwCallTime;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
            GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());


        //发送数据
        std::string data = GameFree.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID, SC_GAMESCENE_FREE, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
        NN_MSG_GS_CALL GameCall;
        GameCall.set_dcellscore(m_dwCellScore);
        uint32_t dwCallTime = (uint32_t)time(NULL) - m_dwCallTime;
        uint32_t cbTimeLeave = dwCallTime >= TIME_CALL_BANKER ? 0 : dwCallTime;		//剩余时间
        GameCall.set_cbtimeleave(cbTimeLeave);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_scallbanker(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            GameCall.set_scallbanker(i, m_sCallBanker[i]);//叫庄标志(-1:未叫, 0:不叫, 124:叫庄)

            //LOG(INFO)<<"Playe"<<i<<"    "<<"m_sCallBanker[i]"<<m_sCallBanker[i];
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
            GameCall.add_cbjettonmultiple(m_cbJettonMultiple[i]);

        for (int i = 0; i < MAX_BANKER_MUL; ++i)
            GameCall.add_cbcallbankermultiple(m_cbBankerMultiple[i]);


        GameCall.set_dwuserid(pIServerUser->GetUserId());
//        GameCall.set_cbgender(pIServerUser->GetGender());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameCall.set_cbviplevel(pIServerUser->GetVipLevel());
        GameCall.set_sznickname(pIServerUser->GetNickName());
//        GameCall.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameCall.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameCall.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_CALL, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID,SC_GAMESCENE_CALL, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
        NN_MSG_GS_SCORE GameScore;
        GameScore.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameScore.set_cbtimeleave(cbTimeLeave);
        GameScore.set_wbankeruser(m_wBankerUser);										//庄家用户
        GameScore.set_cbbankermultiple(m_sCallBanker[m_wBankerUser]);

        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbuserjettonmultiple(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            if (m_cbPlayStatus[i] == 0)
                continue;

            int cbUserJettonMultiple =((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;
            GameScore.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameScore.add_cbjettonmultiple(m_cbJettonMultiple[i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameScore.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
        }

        GameScore.set_dwuserid(pIServerUser->GetUserId());
//        GameScore.set_cbgender(pIServerUser->GetGender());
        GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameScore.set_cbviplevel(pIServerUser->GetVipLevel());
        GameScore.set_sznickname(pIServerUser->GetNickName());
//        GameScore.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameScore.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameScore.set_szlocation(pIServerUser->GetLocation());
        GameScore.set_usstatus(pIServerUser->GetUserStatus());
        GameScore.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameScore.SerializeAsString();
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_SCORE, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID,SC_GAMESCENE_SCORE, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_OPEN:		//开牌场景消息
    {
        NN_MSG_GS_OPEN GameOpen;
        GameOpen.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >= TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)m_wBankerUser);

        //LOG(INFO)<<"GameOpen.set_wbankeruser(m_wBankerUser)=="<<(int)m_wBankerUser;
        //庄家用户
        GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser]);		//庄家倍数

        //LOG(INFO)<<"GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser])=="<<(int)m_sCallBanker[m_wBankerUser];

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameOpen.add_cbjettonmultiple(0);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.add_cbcallbankermultiple(0);
        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            for(int j=0;j<MAX_COUNT;j++)
            {
                GameOpen.add_cbcarddata(0);
                GameOpen.add_cbhintcard(0);
            }
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)

            GameOpen.add_cbuserjettonmultiple(0);//游戏中玩家(1打牌玩家)

        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0)
                continue;

            if (m_sCardType[i] == (-1))
            {
                if (i == dwChairID)
                {
                    int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    GameOpen.set_cbcardtype(i,cardtype);

                    LOG(INFO)<<"---------------------------------open card"<<cardtype;

                    int startidx = i * MAX_COUNT;
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                        GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                    }
                }
                else
                {   int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    GameOpen.set_cbcardtype(i,cardtype);
                }
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
                int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                GameOpen.set_cbcardtype(i,cardtype);

                int startidx = i * MAX_COUNT;
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                    GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                }
            }

            int cbUserJettonMultiple = m_sAddScore[i] >= 0 ? m_cbJettonMultiple[m_sAddScore[i]] : 0;
            GameOpen.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameOpen.set_cbjettonmultiple(i,m_cbJettonMultiple[i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.set_cbcallbankermultiple(i,m_cbBankerMultiple[i]);
        }


        GameOpen.set_dwuserid(pIServerUser->GetUserId());
//        GameOpen.set_cbgender(pIServerUser->GetGender());
        GameOpen.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameOpen.set_cbviplevel(pIServerUser->GetVipLevel());
        GameOpen.set_sznickname(pIServerUser->GetNickName());
//        GameOpen.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameOpen.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameOpen.set_szlocation(pIServerUser->GetLocation());
        GameOpen.set_usstatus(pIServerUser->GetUserStatus());
        GameOpen.set_wchairid(pIServerUser->GetChairId());
        //LOG(INFO)<<"GAME_STATUS_OPEN-------------------------------User Enter"<<(int)dwChairID;

        //发送数据
        std::string data = GameOpen.SerializeAsString();

        const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartype = GameOpen.cbcardtype();
        int res[5];
        for(int i = 0; i < cartype.size(); ++i)
        {
            res[i]=cartype[i];
            //LOG(INFO)<<"---------------------------------open card"<<res[i];
        }
        //m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_OPEN, data.c_str(), data.size());

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID,SC_GAMESCENE_OPEN, data.c_str(),data.size());
//        }
        break;
    }
    case GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));
        GameFree.set_cbreadytime(TIME_READY);
        //memcpy(GameFree.cbAddJettonMultiple, m_cbJettonMultiple, sizeof(GameFree.cbAddJettonMultiple));
        //memcpy(GameFree.cbCallBankerMultiple, m_cbBankerMultiple, sizeof(GameFree.cbAddJettonMultiple));

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
        }


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        //LOG(INFO)<<"GAME_STATUS1_END-------------------------------User Enter"<<(int)dwChairID;

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecord(dwChairID, SC_GAMESCENE_FREE, data.c_str(),data.size());
//        }
        break;
    }
    default:
        break;
    }

    return true;
}

*/
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    //muduo::MutexLockGuard lock(m_mutexLock);

    LOG(ERROR)<<"---------------------------OnEventGameScene---------------------------";
    LOG(ERROR)<<"---------------------------OnEventGameScene---------------------------";
    LOG(ERROR)<<"---------------------------OnEventGameScene---------------------------";

    uint32_t dwChairID = chairId;
    shared_ptr<CServerUserItem> pIServerUser = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUser)
    {
        return false;
    }

    dwChairID = pIServerUser->GetChairId();
    if(dwChairID >= GAME_PLAYER || dwChairID != chairId)
    {
        return false;
    }
    switch (m_wGameStatus)
    {
    case GAME_STATUS_INIT:
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_LOCK_PLAYING:
    {
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));//单元分

//        int wUserCount = 0;
//        for (int i = 0; i < GAME_PLAYER; ++i)
//        {
//            if (m_pITableFrame->IsExistUser(i) && m_inPlayingStatus[i]>0)
//            {
//                ++wUserCount;
//            }
//        }

        uint32_t dwStartTime = 0;
        if(m_wGameStatus == GAME_STATUS_INIT)
            dwStartTime = m_dwInitTime;
        else if(m_wGameStatus == GAME_STATUS_READY)
            dwStartTime = m_dwReadyTime;
        else
            dwStartTime = m_dwEnterPlayingTime;

        uint32_t dwNowTime = (uint32_t)time(NULL);
        uint32_t cbReadyTime = 0;

        uint32_t dwCallTime = dwNowTime - dwStartTime;
        cbReadyTime = dwCallTime >= TIME_READY ? 0 : dwCallTime;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

//        for (int i = 0; i < MAX_JETTON_MUL; ++i)
//            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
//        for (int i = 0; i < MAX_BANKER_MUL; ++i)
//            GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());


        LOG(INFO)<<"GAME_STATUS1_FREE-------------------------------User Enter"<<(int)dwChairID;

        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
        NN_MSG_GS_CALL GameCall;
        GameCall.set_dcellscore(m_dwCellScore);
        uint32_t dwCallTime = (uint32_t)time(NULL) - m_dwCallTime;
        uint32_t cbTimeLeave = dwCallTime >= TIME_CALL_BANKER ? 0 : dwCallTime;		//剩余时间
        GameCall.set_cbtimeleave(cbTimeLeave);
        GameCall.set_roundid(mGameIds);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
             GameCall.add_scallbanker(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            GameCall.set_scallbanker(i, m_sCallBanker[i]);//叫庄标志(-1:未叫, 0:不叫, 124:叫庄)

            LOG(INFO)<<"Playe"<<i<<"    "<<"m_sCallBanker[i]"<<m_sCallBanker[i];
        }

//        for (int i = 0; i < MAX_JETTON_MUL; ++i)
//            GameCall.add_cbjettonmultiple(m_cbJettonMultiple[i]);

        for (int i = 0; i < MAX_BANKER_MUL; ++i){
             GameCall.add_cbcallbankermultiple(m_cbCallable[chairId][i]);
//             GameCall.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
        }



        GameCall.set_dwuserid(pIServerUser->GetUserId());
//        GameCall.set_cbgender(pIServerUser->GetGender());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameCall.set_cbviplevel(pIServerUser->GetVipLevel());
        GameCall.set_sznickname(pIServerUser->GetNickName());
//        GameCall.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameCall.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameCall.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_CALL, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
        NN_MSG_GS_SCORE GameScore;
        GameScore.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameScore.set_cbtimeleave(cbTimeLeave);
        GameScore.set_wbankeruser(m_wBankerUser);										//庄家用户
        GameScore.set_cbbankermultiple(m_sCallBanker[m_wBankerUser]);
        GameScore.set_roundid(mGameIds);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbuserjettonmultiple(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbplaystatus(m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)
            if (m_cbPlayStatus[i] == 0)
                continue;

            int cbUserJettonMultiple = ((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonable[i][m_sAddScore[i]] : 0) ;
                    //((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;
            GameScore.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameScore.add_cbjettonmultiple(m_cbJettonable[chairId][i]);//m_cbJettonMultiple[i]);
        }
//        for (int i = 0; i < MAX_BANKER_MUL; ++i)
//        {
//            GameScore.add_cbcallbankermultiple(m_cbCallable[chairId][i]);//m_cbBankerMultiple[i]);
//        }

        GameScore.set_dwuserid(pIServerUser->GetUserId());
//        GameScore.set_cbgender(pIServerUser->GetGender());
        GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameScore.set_cbviplevel(pIServerUser->GetVipLevel());
        GameScore.set_sznickname(pIServerUser->GetNickName());
//        GameScore.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameScore.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameScore.set_szlocation(pIServerUser->GetLocation());
        GameScore.set_usstatus(pIServerUser->GetUserStatus());
        GameScore.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameScore.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_SCORE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_OPEN:		//开牌场景消息
    {
        NN_MSG_GS_OPEN GameOpen;
        GameOpen.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >= TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)m_wBankerUser);
        GameOpen.set_roundid(mGameIds);
        LOG(INFO)<<"GameOpen.set_wbankeruser(m_wBankerUser)=="<<(int)m_wBankerUser;
        //庄家用户
        GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser]);		//庄家倍数

        LOG(INFO)<<"GameOpen.set_cbbankermutiple(m_sCallBanker[m_wBankerUser])=="<<(int)m_cbCallable[chairId][m_sCallBanker[m_wBankerUser]];

//        for (int i = 0; i < MAX_JETTON_MUL; ++i)
//        {
//            GameOpen.add_cbjettonmultiple(0);
//        }
//        for (int i = 0; i < MAX_BANKER_MUL; ++i)
//        {
//            GameOpen.add_cbcallbankermultiple(0);
//        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            for(int j=0;j<MAX_COUNT;j++)
            {
                GameOpen.add_cbcarddata(0);
                GameOpen.add_cbhintcard(0);
            }
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_cbPlayStatus[i]);//游戏中玩家(1打牌玩家)

            GameOpen.add_cbuserjettonmultiple(0);//游戏中玩家(1打牌玩家)

        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0)
                continue;

            if (m_sCardType[i] == (-1))
            {
                if (i == dwChairID)
                {
                    int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    GameOpen.set_cbcardtype(i,cardtype);

                   LOG(INFO)<<"---------------------------------open card"<<cardtype;

                    int startidx = i * MAX_COUNT;
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                        GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                    }
                }
                else
                {   int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                    GameOpen.set_cbcardtype(i,cardtype);
                }
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
                int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                GameOpen.set_cbcardtype(i,cardtype);

                int startidx = i * MAX_COUNT;
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.set_cbcarddata(startidx+j,(int) m_cbCardData[i][j]);
                    GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                }
            }

            int cbUserJettonMultiple = ((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonable[i][m_sAddScore[i]] : 0) ;
                    //((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;
            GameOpen.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
//            GameOpen.set_cbjettonmultiple(i,m_cbJettonMultiple[i]);
            GameOpen.add_cbjettonmultiple(m_cbJettonable[chairId][i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.add_cbcallbankermultiple(m_cbCallable[chairId][i]);//GameOpen.set_cbcallbankermultiple(i,m_cbBankerMultiple[i]);
        }


        GameOpen.set_dwuserid(pIServerUser->GetUserId());
//        GameOpen.set_cbgender(pIServerUser->GetGender());
        GameOpen.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameOpen.set_cbviplevel(pIServerUser->GetVipLevel());
        GameOpen.set_sznickname(pIServerUser->GetNickName());
//        GameOpen.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameOpen.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameOpen.set_szlocation(pIServerUser->GetLocation());
        GameOpen.set_usstatus(pIServerUser->GetUserStatus());
        GameOpen.set_wchairid(pIServerUser->GetChairId());
        LOG(INFO)<<"GAME_STATUS_OPEN-------------------------------User Enter"<<(int)dwChairID;

        //发送数据
        std::string data = GameOpen.SerializeAsString();

        const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartype = GameOpen.cbcardtype();
        int res[5];
        for(int i = 0; i < cartype.size(); ++i)
        {
            res[i]=cartype[i];
             LOG(INFO)<<"---------------------------------open card"<<res[i];
        }
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_OPEN, (uint8_t*)data.c_str(), data.size());

        break;
    }
#if 0
    case GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        NN_MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((m_dwCellScore));
        GameFree.set_cbreadytime(TIME_READY);
        //memcpy(GameFree.cbAddJettonMultiple, m_cbJettonMultiple, sizeof(GameFree.cbAddJettonMultiple));
        //memcpy(GameFree.cbCallBankerMultiple, m_cbBankerMultiple, sizeof(GameFree.cbAddJettonMultiple));

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
//            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
            GameFree.add_cbjettonmultiple(m_cbJettonable[chairId][i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
//            GameFree.add_cbcallbankermultiple(m_cbBankerMultiple[i]);
            GameFree.add_cbcallbankermultiple(m_cbCallable[chairId][i]);
        }


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pITableFrame->SendTableData(dwChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        LOG(INFO)<<"GAME_STATUS_END-------------------------------User Enter"<<(int)dwChairID;

        break;
    }
#endif
    default:
        break;
    }

    if(pIServerUser->GetUserStatus() == sOffline)  //left failt
    {
        pIServerUser->SetUserStatus(sPlaying);

        //tmp......
//        if(m_bReplayRecordStart)
//        {
//            RepayRecorduserinfoStatu(pIServerUser->GetChairId(), sPlaying);
//        }
    }

//    if(pIServerUser->GetUserStatus()==sOffline)
//    {
//        pIServerUser->SetUserStatus(sReady);
//    }

    m_bPlayerIsOperation[chairId] = true;
    return true;
}

bool CTableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    return true;
}

//用户离开
bool CTableFrameSink::OnUserLeft(int64_t userId, bool islookon)
{
    LOG(ERROR)<<"---------------------------OnUserLeft-----------m_wGameStatus----------------"<<m_wGameStatus;
    if(!CanLeftTable(userId))
    {
        return false;
    }

    bool ret = false;
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userId);
    if(!player)
    {
        LOG(ERROR)<<"---------------------------player == NULL---------------------------";
        return false;
    }
    int chairID = player->GetChairId();
    //if(m_wGameStatus==GAME_STATUS1_FREE||m_wGameStatus==GAME_STATUS_END||m_cbPlayStatus[chairID]==0)
    if(m_wGameStatus == GAME_STATUS_INIT || m_wGameStatus == GAME_STATUS_READY ||
            m_wGameStatus == GAME_STATUS_END || m_cbPlayStatus[chairID] == 0)
    {
        // if(player->GetUserStatus() != sPlaying)
        // {
            //tmp......
//            if(m_bReplayRecordStart)
//            {
//                RepayRecorduserinfoStatu(chairID,0);
//            }
            //            m_inPlayingStatus[chairID] = 0;
            LOG(ERROR)<<"---------------------------player->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << chairID;
//            openLog("7OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",player->GetUserId(),player->GetChairId());
            m_cbPlayStatus[chairID] = 0;
            player->SetUserStatus(sGetout);
            m_pITableFrame->BroadcastUserStatus(player, true);
            m_pITableFrame->ClearTableUser(chairID, true, true);
//            player->SetUserStatus(sGetout);

            ret = true;
        // }
    }

    if(GAME_STATUS_READY == m_wGameStatus)
    {
        int userNum = 0;
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i]>0*/)
            {
                userNum++;
            }
//            if(m_inPlayingStatus[i]>0)
//                usernum++;
        }

        if(userNum < m_pGameRoomInfo->minPlayerNum)
        {
            //m_pITableFrame->KillGameTimer(ID_TIME_READY);
//            m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
            ClearAllTimer();

            m_wGameStatus = GAME_STATUS_INIT;

            LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT;------------------------";
        }
    }

    // if(player && m_pITableFrame)
    // {
    //     if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    //     {
    //         player->SetUserStatus(sOffline);
    //     }
    //     else
    //     {
    //         player->SetUserStatus(sGetout);
    //     }

    //     m_pITableFrame->BroadcastUserStatus(player, true);
    // }
    LOG(ERROR)<<"---------------------------OnUserLeft1-----------m_wGameStatus----------------"<<m_wGameStatus;

//    openLog("  ------------------------------user left ");
    return ret;
}

void CTableFrameSink::GameTimerReadyOver()
{
    //muduo::MutexLockGuard lock(m_mutexLock);

	assert(m_wGameStatus < GAME_STATUS_START);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);

    LOG(ERROR)<<"---------------------------GameTimerReadyOver---------------------------";
    //获取玩家个数
    int wUserCount = 0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
//            openLog("  wanjia zhuang tai  renshu=%d",user->GetUserStatus());
            if(user->GetUserStatus() == sOffline)
            {
//                openLog(" 1ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(ERROR)<<"---------------------------ClearTableUse1r---------------------------";
            }else if(user->GetUserScore()< m_pGameRoomInfo->enterMinScore )//m_pITableFrame->GetUserEnterMinScore())
            {
//                openLog(" 2ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
                LOG(ERROR)<<"---------------------------ClearTableUser2---------------------------";
            }else{
                ++wUserCount;
            }
        }
    }



    //用户状态
//    for(int i = 0; i < GAME_PLAYER; ++i)
//    {
        //获取用户
//        shared_ptr<CServerUserItem>pIServerUser = m_pITableFrame->GetTableUserItem(i);
//        if(m_pITableFrame->IsExistUser(i)/* && m_inPlayingStatus[i] > 0*/)
//        {
//             ++wUserCount;
//        }
//        if(m_inPlayingStatus[i] > 0)
//            ++wUserCount;
//    }
//    openLog("  yijing diaoyong m_pITableFrame->StartGame()  renshu=%d",wUserCount);
    if(wUserCount >= m_pGameRoomInfo->minPlayerNum)
    {
        //清理游戏数据
        ClearGameData();
        //初始化数据
        InitGameData();
        InitLoadini();
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sPlaying);
            }
        }
        LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_PLAYING GameTimereEnterPlayingOver ---------------------------";
        m_wGameStatus = GAME_STATUS_LOCK_PLAYING;
        //m_TimerIdEnterPlaying = m_TimerLoopThread->getLoop()->runAfter(2.0, boost::bind(&CTableFrameSink::GameTimereEnterPlayingOver, this));
        GameTimereEnterPlayingOver();
    }else
    {
        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                LOG(ERROR)<<"---------------------------user->SetUserStatus(sReady) ---------------------------";
                user->SetUserStatus(sReady);
            }
            m_cbPlayStatus[i] = 0;
        }
        ClearAllTimer();
        m_wGameStatus = GAME_STATUS_INIT;
        LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT---------------------------";
    }
}

void CTableFrameSink::GameTimereEnterPlayingOver()
{
    {
        //muduo::MutexLockGuard lock(m_mutexLock);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);


        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
                user->SetUserStatus(sPlaying);
                m_cbPlayStatus[i] = 1;
                m_cbPlayStatusRecord[i] = 1;
                ++m_wPlayCount;
            }
            else
            {
                m_cbPlayStatus[i] = 0;
                m_cbPlayStatusRecord[i]=0;
            }

        }
        if(m_wPlayCount < m_pGameRoomInfo->minPlayerNum)
        {
            for(int i = 0; i < GAME_PLAYER; ++i)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(user)
                {
                    LOG(ERROR)<<"---------------------------user->SetUserStatus(sReady) ---------------------------";
                    user->SetUserStatus(sReady);
                }
                m_cbPlayStatus[i] = 0;
                m_cbPlayStatusRecord[i] = 0;
            }
            ClearAllTimer();
            m_wGameStatus = GAME_STATUS_INIT;
            LOG(ERROR)<<"---------------------------m_wGameStatus = GAME_STATUS_INIT---------------------------";
            return;
        }
    }

    {
        LOG(ERROR)<<"---------------------------GameTimereEnterPlayingOver----m_pITableFrame->StartGame()-----------------------";
//
//        m_pITableFrame->StartGame();
        OnGameStart();
//        openLog("  yijing diaoyong m_pITableFrame->StartGame() ");
    }
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    LOG(ERROR)<<"---------------------------OnGameStart---------------------------";
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);

    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;

    LOG(INFO)<<"FromGameServerm_llStorageControl"<<"="<<m_llStorageControl;
    LOG(INFO)<<"FromGameServerm_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
    LOG(INFO)<<"FromGameServerm_llStorageHighLimit"<<"="<<m_llStorageHighLimit;



    assert(NULL != m_pITableFrame);

    //清除所有定时器
    ClearAllTimer();
    m_dwReadyTime = (uint32_t)time(NULL);

    //发送开始消息
    NN_CMD_S_GameStart GameStart;
    GameStart.set_cbcallbankertime(TIME_CALL_BANKER);
    GameStart.set_roundid(mGameIds);

    //随机扑克发牌
    //每随机10次换一次随机种子
//    m_nLottery += 1;
//    m_nLottery = m_nLottery % 10;
    //同一房间多个同一秒开始游戏牌是一样的
//    if (0 == m_nLottery)
//    {
//        m_GameLogic.GenerateSeed(m_pITableFrame->GetTableId());
//    }
    int playersnum = 0;
    uint8_t cbTempArray[MAX_CARD_TOTAL] = { 0 };
    m_GameLogic.RandCardData(cbTempArray, sizeof(cbTempArray));
    memset(m_cbCardData, 0, sizeof(m_cbCardData));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        GameStart.add_cbplaystatus(m_cbPlayStatus[i]);

        if (0 == m_cbPlayStatus[i])
            continue;
        m_cbLeftCardCount -= MAX_COUNT;
        ++playersnum;
        //手牌
        memcpy(&m_cbCardData[i], &cbTempArray[m_cbLeftCardCount], sizeof(uint8_t)*MAX_COUNT);
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        m_replay.addPlayer(user->GetUserId(),user->GetAccount(),user->GetUserScore(),i);
        if(user->GetBlacklistInfo().status == 1 && user->GetBlacklistInfo().weight >= rand()%1000 )
        {
            m_iBlacklistNum++;
            m_bBlacklist[i]=true;
        }else
        {
            m_bBlacklist[i]=false;
        }

        if(ENABLE_TEST)
        {
            testCase->Check(user->GetAccount(),m_cbCardData[i]);
        }
    }
    memcpy(m_cbRepertoryCard, cbTempArray,sizeof(cbTempArray));

    CMD_S_GameStartAi  GameStartAi;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i])
            continue;

        if(!m_pITableFrame->GetTableUserItem(i))
            continue;

        if(!m_pITableFrame->GetTableUserItem(i)->IsAndroidUser())
            continue;

        int cardtype =(int) m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
        GameStartAi.set_cboxcarddata(cardtype);
        int isMax = IsMaxCard(i);
        GameStartAi.set_ismaxcard(isMax);
        std::string data = GameStartAi.SerializeAsString();
        m_pITableFrame->SendTableData(i, NN_SUB_S_GAME_START_AI, (uint8_t*)data.c_str(), data.size());
    }

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i]){
            for (int j = 0; j < MAX_BANKER_MUL; ++j)
            {
                m_cbCallable[i][j] = 0;
                GameStart.add_cbcallbankermultiple(m_cbCallable[i][j]);
            }
            continue;
        }


        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i);
        int64_t score = userItem->GetUserScore();
        int64_t maxCallTimes = score/(m_pGameRoomInfo->floorScore * 8 * (playersnum-1));

        m_cbCallable[i][0] = m_cbBankerMultiple[0];
        GameStart.add_cbcallbankermultiple(m_cbBankerMultiple[0]);

        for (int j = 1; j < MAX_BANKER_MUL; ++j)
        {
            if(maxCallTimes >= m_cbBankerMultiple[j]){
                m_cbCallable[i][j] = m_cbBankerMultiple[j];
                GameStart.add_cbcallbankermultiple(m_cbBankerMultiple[j]);
            }else{
                m_cbCallable[i][j] = -m_cbBankerMultiple[j];
                GameStart.add_cbcallbankermultiple(-m_cbBankerMultiple[j]);
            }
        }
    }
    std::string data = GameStart.SerializeAsString();
    //发送数据
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
    //设置游戏状态
    m_wGameStatus = GAME_STATUS_CALL;//zhuan tai gai bian
    m_dwCallTime = (uint32_t)time(NULL);

    LOG(ERROR)<<"------------------m_wGameStatus = GAME_STATUS_CALL---------IsTrustee---------------------------";
    //设置托管
    IsTrustee();
}

void CTableFrameSink::GameTimerCallBanker()
{
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
    m_dwCallTime = 0;

    //代替叫庄
    AutoUserCallBanker();
}

int CTableFrameSink::AutoUserCallBanker()
{
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if (m_sCallBanker[i] != -1)
        {
            continue;
        }

        OnUserCallBanker(i, 0);
    }
    return 0;
}

//玩家叫庄
int CTableFrameSink::OnUserCallBanker(int wChairID, int wCallFlag)
{
    //已叫过庄
    if (-1 != m_sCallBanker[wChairID])
    {
        return 1;
    }

    //叫庄标志错误
    if (wCallFlag < 0)
    {
        return 2;
    }

    //检测是否正确的倍数
    bool bCorrectMultiple = false;
    if (wCallFlag != 0)
    {
        for (int cbIndex = 0; cbIndex != MAX_BANKER_MUL; ++cbIndex)
        {
            if (m_cbCallable[wChairID][cbIndex] == wCallFlag)//(m_cbBankerMultiple[cbIndex] == wCallFlag)
            {
                bCorrectMultiple = true;
                break;
            }
        }

        if (bCorrectMultiple == false)
        {
            return 3;
        }
    }

    //记录叫庄
    m_sCallBanker[wChairID] = wCallFlag;
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(wCallFlag),-1,opBet,wChairID,wChairID);
    //处理叫庄
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有叫庄玩家
        if (-1 == m_sCallBanker[i])
            continue;

        //记录总人数
        ++wTotalCount;
    }

    //叫庄
    NN_CMD_S_CallBanker callBanker;
    callBanker.set_wcallbankeruser( wChairID);
    callBanker.set_cbcallmultiple(wCallFlag);
    std::string data = callBanker.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_CALL_BANKER, (uint8_t*)data.c_str(), data.size());

    //记录叫庄玩家
    int wBankerUser = INVALID_CHAIR;

     NN_CMD_S_CallBankerResult CallResult;
     for (int j = 0; j < GAME_PLAYER; ++j)
     {
         CallResult.add_cbcallbankeruser(0);
     }

    CallResult.set_cbrandbanker(0);   //是否随机抢庄
    if(wTotalCount == m_wPlayCount)  //all callBanker
    {
        //计算叫庄玩家
        int wCallBankerUser[MAX_BANKER_CALL][GAME_PLAYER];
        memset(wCallBankerUser, 0, sizeof(wCallBankerUser));
        int wCallBankerCount[MAX_BANKER_CALL];
        memset(wCallBankerCount, 0, sizeof(wCallBankerCount));  //0 1 2 4 count

        for(int i = 0; i < GAME_PLAYER; ++i)
        {
            CallResult.set_cbcallbankeruser(i, m_sCallBanker[i]);
            if(m_cbPlayStatus[i] != 1)
                continue;

            if(m_sCallBanker[i] != 0)
            {
                uint8_t cbCallScore = (uint8_t)m_sCallBanker[i];  //0,1,2,4
                uint8_t cbCallCount = (uint8_t)wCallBankerCount[cbCallScore];  //0,1,2,4 count

                wCallBankerUser[cbCallScore][cbCallCount] = i;
                ++wCallBankerCount[cbCallScore];
            }
        }

        //确定庄家
        for(int i = (MAX_BANKER_CALL - 1); i >= 0; --i)
        {
            if(wCallBankerCount[i] > 0)
            {
                int wCount = wCallBankerCount[i];
                if (wCount == 1)
                {
                    wBankerUser = wCallBankerUser[i][0];

                }else
                {
                    CallResult.set_cbrandbanker(1);
                    //todo need to be optimus
//                    wBankerUser = wCallBankerUser[i][rand() % wCount];
                    int64_t curMaxScore = 0;
                    for(int j = 0; j < wCount; j++)
                    {
                        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(wCallBankerUser[i][j]);
                        if(userItem->GetUserScore() > curMaxScore)
                        {
                            curMaxScore = userItem->GetUserScore();
                            wBankerUser = wCallBankerUser[i][j];
                        }
                    }
                }

                break;
            }
        }


        //没有人叫地主
        if (wBankerUser == INVALID_CHAIR)
        {
            wBankerUser = RandBanker();
//            for (int i = 0; i < GAME_PLAYER; ++i)
//            {
//                if (m_cbPlayStatus[i] == 0)
//                    continue;

//                CallResult.set_cbcallbankeruser(i, 1);
//            }
            CallResult.set_cbrandbanker(1);   //是否随机抢庄
            //如果人叫庄则随机到的庄家默认叫1倍
            m_sCallBanker[wBankerUser] = m_cbBankerMultiple[0];
        }

        m_wBankerUser = wBankerUser;
        //设置游戏状态
        m_wGameStatus = GAME_STATUS_SCORE;
        m_dwScoreTime = (uint32_t)time(NULL);
        //设置托管
        IsTrustee();
    }

    //发送叫庄结果
    if (wBankerUser != INVALID_CHAIR)
    {
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(wBankerUser),-1,opBanker,-1,-1);
        CallResult.set_dwbankeruser(wBankerUser);
        shared_ptr<CServerUserItem> bankerItem = m_pITableFrame->GetTableUserItem(wBankerUser);
        int64_t maxBankerCallTimes = bankerItem->GetUserScore()/(m_pGameRoomInfo->floorScore * (wTotalCount - 1) * m_sCallBanker[wBankerUser] );
        for(int j = 0; j < GAME_PLAYER; j++ )
        {
            if( m_cbPlayStatus[j] == 1 && j != wBankerUser)
            {
                shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(j);
                int64_t maxPlayerCallTimes = userItem->GetUserScore()/(m_pGameRoomInfo->floorScore*m_sCallBanker[wBankerUser]);
                int64_t callMaxTimes = maxPlayerCallTimes > maxBankerCallTimes ? maxBankerCallTimes : maxPlayerCallTimes;
                if( callMaxTimes >= 20){
                    for (int i = 0; i < MAX_JETTON_MUL; ++i)
                    {
                        m_cbJettonable[j][i]=m_cbJettonMultiple[i];
                        CallResult.add_cbjettonmultiple(m_cbJettonMultiple[i]);
                    }
                }else if(callMaxTimes >= 4){
                    for (int i = 0; i < MAX_JETTON_MUL; ++i)
                    {
                        m_cbJettonable[j][i]=callMaxTimes*(i+1)/4;
                        CallResult.add_cbjettonmultiple(m_cbJettonable[j][i]);
                    }
                }else{
                    for (int i = 1; i <= MAX_JETTON_MUL; ++i)
                    {
                        if(i <= callMaxTimes)
                        {
                            m_cbJettonable[j][i-1]=i;
                        }else{
                            m_cbJettonable[j][i-1]=0;
                        }
                        CallResult.add_cbjettonmultiple(m_cbJettonable[j][i-1]);
                    }
                }
            }else{
                for (int i = 0; i < MAX_JETTON_MUL; ++i)
                {
                    m_cbJettonable[j][i] = 0;
                    CallResult.add_cbjettonmultiple(m_cbJettonable[j][i]);
                }
            }
            CallResult.set_cbaddjettontime(TIME_ADD_SCORE);			//加注时间
            //LOG(WARNING) << "random banker ?" << (int)CallResult.cbRandBanker;
        }
        std::string data = CallResult.SerializeAsString();
        m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_CALL_BANKER_RESULT, (uint8_t*)data.c_str(), data.size());
    }
    return 4;
}

//随机庄家
int CTableFrameSink::RandBanker()
{
    int wTempBanker = 0;//rand() % m_wPlayCount;
    int64_t maxUserScore = 0;
    for (uint8_t i = 0; i < GAME_PLAYER; ++i)
    {
        if (1 == m_cbPlayStatus[i])
        {
            shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetTableUserItem(i);
            if(userItem->GetUserScore() > maxUserScore){
                maxUserScore = userItem->GetUserScore();
                wTempBanker = i;
            }
        }
    }

    return wTempBanker;
}


void CTableFrameSink::GameTimerAddScore()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_dwScoreTime = 0;
    //代替下注
    AutoUserAddScore();
}

int CTableFrameSink::AutoUserAddScore()
{
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if (m_sAddScore[i] != -1)
        {
            continue;
        }

        OnUserAddScore(i, 0);
    }
    return 0;
}

//玩家下注
int CTableFrameSink::OnUserAddScore(int wChairID, int wJettonIndex)
{
    //庄家不能下注
    if (m_wBankerUser == wChairID)
    {
        return 1;
    }

    //已下过注
    if (-1 != m_sAddScore[wChairID])
    {
        return 2;
    }

    //下注筹码错误
    if (wJettonIndex < 0 || wJettonIndex >= MAX_JETTON_MUL)  //5 10 15 20
    {
        return 3;
    }
    if(m_cbJettonable[wChairID][wJettonIndex] <= 0){
        return 4;
    }

    //记录下注
    m_sAddScore[wChairID] = wJettonIndex;
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(m_cbJettonable[wChairID][wJettonIndex]),-1,opAddBet,wChairID,wChairID);
    NN_CMD_S_AddScoreResult CallResult;
    CallResult.set_waddjettonuser(wChairID);
    CallResult.set_cbjettonmultiple(m_cbJettonable[wChairID][wJettonIndex]);//m_cbJettonMultiple[wJettonIndex]);

    //处理下注
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤庄家
        if (i == m_wBankerUser)
            continue;

        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有下注玩家
        if (-1 == m_sAddScore[i])
            continue;

        //记录总人数
        ++wTotalCount;
    }

    //是否发牌
    uint8_t cbIsSendCard = 0;
    int dwOpenTime = 0;
    if ((wTotalCount + 1) == m_wPlayCount)
    {
        //所有人都下注了
        //设置游戏状态
        m_wGameStatus = GAME_STATUS_OPEN;
        LOG(INFO)<<"m_wGameStatus = GAME_STATUS_OPEN";

        m_dwOpenTime = (uint32_t)time(NULL);
        //设置发牌标志
        cbIsSendCard = 1;
        dwOpenTime = TIME_OPEN_CARD;
        if(!ENABLE_TEST)
        {
            if(m_iBlacklistNum == 0 )//|| m_pGameRoomInfo->matchforbids[MTH_BLACKLIST])
            {
                //分析玩家牌
                AnalyseCardEx();
            }else
            {
                AnalyseForBlacklist();
            }
        }

        //设置托管
        IsTrustee();
    }


    //发送下注结果
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //if (0 == m_cbPlayStatus[i]) continue;
        std::string data = CallResult.SerializeAsString();
        m_pITableFrame->SendTableData(i, NN_SUB_S_ADD_SCORE_RESULT, (uint8_t*)data.c_str(), data.size());

        if (1 == cbIsSendCard)
        {
            //有发牌
            NN_CMD_S_SendCard sendCard;
            for(int j = 0; j < MAX_COUNT; ++j)
            {
                sendCard.add_cbsendcard(0);
                sendCard.add_cboxcard(0);
            }
            sendCard.set_cbcardtype(0);
            sendCard.set_cbopentime(dwOpenTime);

            if(m_cbPlayStatus[i] != 0 )
            {
                uint8_t cbCardType = m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                sendCard.set_cbcardtype(cbCardType);

                //提牌数据
                uint8_t cbOXCard[MAX_COUNT] = { 0 };
                memcpy(cbOXCard, &m_cbCardData[i], sizeof(uint8_t)*MAX_COUNT);
                m_GameLogic.GetOxCard(cbOXCard);

                for(int j = 0; j < MAX_COUNT; ++j)
                {
                    sendCard.set_cbsendcard(j, m_cbCardData[i][j]);
                    sendCard.set_cboxcard(j, cbOXCard[j]);
                }
            }
            std::string data = sendCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, NN_SUB_S_SEND_CARD, (uint8_t*)data.c_str(), data.size());
        }
    }
    return 4;
}

void CTableFrameSink::AnalyseForBlacklist()
{
    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbCardData, sizeof(cbUserCardData));

    //临时变量
    uint8_t cbTmpCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbTmpCardData, m_cbCardData, sizeof(cbTmpCardData));

    int androidPlayerNum = m_pITableFrame->GetAndroidPlayerCount();
//    int realPlayerNum = m_wPlayCount - androidPlayerNum;
    int strategy = 1;
    int winnerSeat = INVALID_CHAIR;
    int winnerWeight = 10000;
    if(androidPlayerNum == 0)//没有机器人找一个通杀率较低的玩家当赢家
    {
        strategy = 1;
    }else//机器人和玩家混用的时候找个机器人玩家赢
    {
        strategy = 2;
    }
    shared_ptr<CServerUserItem> tempUser;
    std::vector<int> vecCardVal;
    for (int j = 0; j < m_wPlayCount; j++)
    {
        //变量定义
        int wWinUser = INVALID_CHAIR;

        //查找数据
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            //用户过滤
            if (m_cbPlayStatus[i] == 0)
                continue;

            bool bExit = false;
            std::vector<int>::iterator  it = vecCardVal.begin();
            for (; it != vecCardVal.end(); ++it)
            {
                if (*it == i)
                {
                    bExit = true;
                    break;
                }
            }
            if(bExit)
            {
                continue;
            }

            //设置用户
            if (wWinUser == INVALID_CHAIR)
            {
                wWinUser = i;
                continue;
            }

            //对比扑克
            if (m_GameLogic.CompareCard(cbUserCardData[i], m_cbCardData[wWinUser]))
            {
                wWinUser = i;
            }
        }
        vecCardVal.push_back(wWinUser);
        if (m_cbPlayStatus[j] == 0)
            continue;
        if(strategy == 1)
        {
            if(!m_pITableFrame->IsAndroidUser(j))
            {
                tempUser = m_pITableFrame->GetTableUserItem(j);
                if(!m_bBlacklist[j])//& tempUser->GetBlacklistInfo().status == 0)
                {
                    if(winnerWeight == 0)
                    {
                        if(rand()%100 > 50)
                        {
                            winnerSeat = j;
                        }
                    }else
                    {
                        winnerWeight = 0;
                        winnerSeat = j;
                    }
                }else if( tempUser->GetBlacklistInfo().weight < winnerWeight )
                {
                    winnerSeat = j;
                    winnerWeight = tempUser->GetBlacklistInfo().weight;
                }
            }
        }else{
            if(m_pITableFrame->IsAndroidUser(j))
            {
                if(winnerSeat == INVALID_CHAIR || rand()%100 > 50)
                    winnerSeat = j;
            }
        }
    }    
    int cbTmpValue = 0;

//    int i = rand()%GAME_PLAYER;// i need to be random?
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if(m_pITableFrame->IsAndroidUser(i) || winnerSeat == i)
        {
            cbTmpValue = vecCardVal.front();
            vecCardVal.erase(vecCardVal.begin());
        }else
        {
            cbTmpValue = vecCardVal.back();
            vecCardVal.pop_back();
        }

        if (cbTmpValue != i)
        {
            memcpy(m_cbCardData[i], cbTmpCardData[cbTmpValue], MAX_COUNT);
        }
    }
}

void CTableFrameSink::AnalyseCardEx()
{
    if (m_pITableFrame->GetAndroidPlayerCount() == 0)
    {
        return;
    }

    int64_t llLowerLimitVal = m_llStorageLowerLimit;		//吸分值 下限值
    int64_t llHighLimitVal = m_llStorageHighLimit;		//吐分值 上限值

    //////////////////////////////////////////////////////////////////////////
    bool bSteal = true;
    LOG(INFO) << "-------------------m_llStorageControl = " << (int)m_llStorageControl;
    LOG(INFO) << "-------------------m_llStorageminis = " << (int)llLowerLimitVal;
    LOG(INFO) << "-------------------m_llStoragemax = " << (int)llHighLimitVal;
    if (m_llStorageControl > llLowerLimitVal && m_llStorageControl < llHighLimitVal)
    {
        return;
    }
    else if (m_llStorageControl > llHighLimitVal )
    {
        bSteal = false;
    }
    else if (m_llStorageControl < llLowerLimitVal )
    {
        bSteal = true;
    }

    if (m_llStorageControl > llHighLimitVal && rand() % 100 <= 30)
        return ;
    else if (m_llStorageControl < llLowerLimitVal && rand() % 100 <= 20)
        return ;
    if(bSteal)
    {
        LOG(ERROR) << "suan fa 吸分";
    }else
    {
        LOG(ERROR) << "suan fa 吐分";
    }
    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbCardData, sizeof(cbUserCardData));

    //临时变量
    uint8_t cbTmpCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbTmpCardData, m_cbCardData, sizeof(cbTmpCardData));

    std::vector<int> vecCardVal;
    for (int j = 0; j < m_wPlayCount; j++)
    {
        //变量定义
        int wWinUser = INVALID_CHAIR;

        //查找数据
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            //用户过滤
            if (m_cbPlayStatus[i] == 0)
                continue;

            bool bExit = false;
            std::vector<int>::iterator  it = vecCardVal.begin();
            for (; it != vecCardVal.end(); ++it)
            {
                if (*it == i)
                {
                    bExit = true;
                    break;
                }
            }
            if(bExit)
            {
                continue;
            }

            //设置用户
            if (wWinUser == INVALID_CHAIR)
            {
                wWinUser = i;
                continue;
            }

            //对比扑克
            if (m_GameLogic.CompareCard(cbUserCardData[i], m_cbCardData[wWinUser]) == bSteal)
            {
                wWinUser = i;
            }
        }
        vecCardVal.push_back(wWinUser);
    }

    int cbTmpValue = 0;

    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if (m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if(m_pITableFrame->IsAndroidUser(i))
        {
            cbTmpValue = vecCardVal.front();
            vecCardVal.erase(vecCardVal.begin());
        }else
        {
            cbTmpValue = vecCardVal.back();
            vecCardVal.pop_back();
        }

        if (cbTmpValue != i)
        {
            // LOG(WARNING) << "换牌 " << m_pITableFrame->GetEscortID(cbTmpValue) << " -> " << m_pITableFrame->GetEscortID(i);
            memcpy(m_cbCardData[i], cbTmpCardData[cbTmpValue], MAX_COUNT);
        }
    }
}


void CTableFrameSink::GameTimerOpenCard()
{
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
    m_dwOpenTime = 0;
    //代替开牌
    AutoUserOpenCard();
}

int CTableFrameSink::AutoUserOpenCard()
{
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        if(m_cbPlayStatus[i] == 0)
        {
            continue;
        }

        if(m_sCardType[i] != -1)
        {
            continue;
        }

        OnUserOpenCard(i);
        //	LOG_IF(INFO, PRINT_GAME_INFO) << "OnTimerMessage auto open card chairId = " << i;
    }
    return 0;
}

//玩家开牌
int CTableFrameSink::OnUserOpenCard(int wChairID)
{
    if (-1 != m_sCardType[wChairID])
    {
        //已开牌
        return 1;
    }
    //记录牌型
    m_sCardType[wChairID] =(int)m_GameLogic.GetCardType(m_cbCardData[wChairID], MAX_COUNT);
    //倍数
    m_iMultiple[wChairID] =(int)m_GameLogic.GetMultiple(m_cbCardData[wChairID]);

    //获得牛牛牌
    uint8_t cbOXCard[MAX_COUNT] = { 0 };
    memcpy(cbOXCard, &m_cbCardData[wChairID], sizeof(uint8_t)*MAX_COUNT);
    m_GameLogic.GetOxCard(cbOXCard);
    memcpy(&m_cbOXCard[wChairID], cbOXCard, sizeof(cbOXCard));

    //发送开牌结果
    NN_CMD_S_OpenCardResult OpenCardResult;
    OpenCardResult.set_wopencarduser( wChairID);
    OpenCardResult.set_cbcardtype(m_sCardType[wChairID]);
    for(int j = 0; j < MAX_COUNT; ++j)
    {
        OpenCardResult.add_cbcarddata(m_cbCardData[wChairID][j]);
        OpenCardResult.add_cboxcarddata(cbOXCard[j]);
    }

    std::string data = OpenCardResult.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_OPEN_CARD_RESULT, (uint8_t*)data.c_str(), data.size());

    //处理下注
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有开牌玩家
        if (-1 == m_sCardType[i])
            continue;

        //记录总人数
        ++wTotalCount;
    }

    //所有人都开牌了
    if ((wTotalCount) == m_wPlayCount)
    {
        //游戏结束
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
        return 10;
    }

    return 12;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t wChairID, uint8_t cbReason)
{
    //清除所有定时器
    ClearAllTimer();

    LOG(INFO)<<"m_wGameStatus = GAME_STATUS_END";

    m_dwEndTime = (uint32_t)time(NULL);

    switch (cbReason)
    {
        case GER_NORMAL:
        {//正常退出
            NormalGameEnd(wChairID);
            break;
        }
        // case GER_USER_LEFT:
        // {//玩家强制退出,则把该玩家托管不出。
        //     break;
        // }
        case GER_DISMISS:
        {//游戏解散
            break;
        }
        default:
            break;
    }
    //游戏结束
    m_bContinueServer = m_pITableFrame->ConcludeGame(GAME_STATUS_END);
    m_pITableFrame->SetGameStatus(GAME_STATUS_END);
    //设置游戏状态-空闲状态
    m_wGameStatus = GAME_STATUS_END;

    //设置托管
    IsTrustee();
}

//普通场游戏结算
void CTableFrameSink::NormalGameEnd(int dwChairID)
{
    NN_CMD_S_GameEnd GameEnd;
    /****
    庄家信息
    ****/
    //庄家本金
    shared_ptr<IServerUserItem> pBankerinfo = m_pITableFrame->GetTableUserItem(m_wBankerUser);
    int64_t llBankerBaseScore;
    if(pBankerinfo)
    {
        llBankerBaseScore= pBankerinfo->GetUserScore();
    }
    else
    {
        llBankerBaseScore = 0;
    }
    //庄家总赢的金币
    int64_t iBankerWinScore = 0;
    //庄家总输赢金币
    int64_t iBankerTotalWinScore = 0;
    //叫庄倍数
    int cbBankerMultiple = m_sCallBanker[m_wBankerUser];
    //庄家牌型倍数
    int cbBankerCardMultiple = m_GameLogic.GetMultiple(m_cbCardData[m_wBankerUser]);
    //庄家赢总倍数

    /****
    闲家信息
    ****/
    //闲家总赢的金币
    int64_t iUserTotalWinScore = 0;
    //player total lose score
    int64_t iUserTotalLoseScore = 0;
    //闲家本金
    int64_t llUserBaseScore[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //闲家各自赢分
    int64_t iUserWinScore[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //banker rwinscores
    int64_t rBankerWinScore=0;

    memset(m_iUserWinScore,0x00,sizeof(m_iUserWinScore));

    //下注倍数
    int64_t cbUserJettonMultiple[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //玩家牌型倍数
    int32_t cbUserCardMultiple[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //闲家输总倍数
    int64_t wWinUserTotalMultiple = 0;
    //闲家赢总倍数
    int64_t wLostUserTotalMultiple = 0;

    int32_t wLostUserCount = 0;

    stringstream iStr;
    //计算输赢
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        iStr << GlobalFunc::converToHex( m_cbCardData[i] ,5);

        if(0 == m_cbPlayStatus[i])
            continue;//没有参与游戏不参与结算

        if(i == m_wBankerUser)
            continue;// 庄家不参与闲家结算

        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
            continue;

        cbUserJettonMultiple[i] = m_cbJettonable[i][m_sAddScore[i]];//m_cbJettonMultiple[m_sAddScore[i]];
        cbUserCardMultiple[i] = m_GameLogic.GetMultiple(m_cbCardData[i]);

        LOG(INFO)<<"m_cbJettonMultiple[m_sAddScore[i]]"<<(int)cbUserJettonMultiple[i]<<"m_sAddScore[i]"<<(int)m_sAddScore[i];//m_cbJettonMultiple[m_sAddScore[i]]
        LOG(INFO)<<" cbUserCardMultiple[i]"<< (int)cbUserCardMultiple[i];
        LOG(INFO)<<" cbBankerMultiple======"<< (int)cbBankerMultiple<<"m_wBankerUser====="<<(int)m_wBankerUser;
        //闲家原始积分

        llUserBaseScore[i] = puserinfo->GetUserScore();
        int64_t iTempScore = 0;
        if(m_GameLogic.CompareCard(m_cbCardData[m_wBankerUser], m_cbCardData[i]))
        {
            //庄赢  底分 * 抢庄倍数 * 下注倍数 * 牌型倍数
            iTempScore = m_dwCellScore * cbBankerMultiple * cbUserJettonMultiple[i] * cbBankerCardMultiple;
            if (iTempScore > llUserBaseScore[i])
            {
                iTempScore = llUserBaseScore[i];
            }

            iBankerWinScore += iTempScore;
            iBankerTotalWinScore += iTempScore;
            rBankerWinScore += iTempScore;

            //玩家总输分
            iUserWinScore[i] = -iTempScore;
            iUserTotalLoseScore += -iTempScore;
            //玩家总赢倍数
            wLostUserTotalMultiple += cbUserJettonMultiple[i] * cbUserCardMultiple[i];

            //玩家输的人数
            ++wLostUserCount;
        }else
        {
            //闲赢
            iTempScore = m_dwCellScore * cbBankerMultiple * cbUserJettonMultiple[i] * cbUserCardMultiple[i];
            if (iTempScore > llUserBaseScore[i])
            {
                iTempScore = llUserBaseScore[i];
            }

            iUserTotalWinScore += iTempScore;
            iBankerTotalWinScore -= iTempScore;
            rBankerWinScore += iTempScore;

            //玩家赢分
            iUserWinScore[i] = iTempScore;
            //输家总倍数
            wWinUserTotalMultiple += cbUserJettonMultiple[i] * cbUserCardMultiple[i];
        }
    }

    iStr << m_wBankerUser;
    //庄家计算出来的赢分
    int64_t iBankerCalWinScore = 0;

    //如果庄家赢的钱大于庄家的基数
    if (iBankerTotalWinScore > llBankerBaseScore)
    {
        rBankerWinScore = 0;
        //庄家应该收入金币最多为 庄家本金+闲家总赢的金币
        iBankerCalWinScore = llBankerBaseScore + iUserTotalWinScore;

        //按照这个金额平摊给输家
//        int64_t iEachPartScore = iBankerCalWinScore * 100 / wLostUserTotalMultiple;
        int64_t bankerliss=0;
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0 || i == m_wBankerUser)
                continue;

            if (iUserWinScore[i] > 0){
                rBankerWinScore += iUserWinScore[i] ;
                continue;
            }

            //当前玩家真正要输的金币分摊一下
            double iUserNeedLostScore = iBankerCalWinScore*iUserWinScore[i]/iUserTotalLoseScore;//iEachPartScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i]/100;
            if (iUserNeedLostScore > llUserBaseScore[i])
            {
                iUserNeedLostScore = llUserBaseScore[i];
                bankerliss-=(iUserNeedLostScore-llUserBaseScore[i]);///////////yufang wanjia bu gou
            }
            rBankerWinScore += iUserNeedLostScore;
            iUserWinScore[i] = -iUserNeedLostScore;
        }

        //庄家赢的钱是庄家本金
        //iBankerRealWinScore = (int)llBankerBaseScore;
        iUserWinScore[m_wBankerUser] = llBankerBaseScore+bankerliss;

        //闲家赢钱的，再赢多少就赢多少，不需要有变动
    }
    else if (iBankerTotalWinScore + llBankerBaseScore < 0)   //如果庄家的钱不够赔偿
    {
        //庄家能赔付的钱总额
        int64_t iBankerMaxScore = llBankerBaseScore + iBankerWinScore;
        rBankerWinScore = iBankerMaxScore;
        //计算玩家每部分的钱
//        int64_t iEachPartScore = iBankerMaxScore * 100 / wWinUserTotalMultiple;

        //闲赢 (庄家本金 + 庄赢的总金币)*当前闲家赢的金币 /（闲家总赢）
        int64_t bankerscore=0.0;
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (0 == m_cbPlayStatus[i])
                continue;
            if (i == m_wBankerUser)
                continue;

            //输钱不算
            if (iUserWinScore[i] < 0)
                continue;

            double iUserRealWinScore = iBankerMaxScore*iUserWinScore[i]/iUserTotalWinScore;//iEachPartScore * cbUserJettonMultiple[i] * cbUserCardMultiple[i]/100;
            if (iUserRealWinScore > llUserBaseScore[i])
            {
                iUserRealWinScore = llUserBaseScore[i];
                bankerscore+=(iUserRealWinScore-llUserBaseScore[i]);
            }

            iUserWinScore[i] = iUserRealWinScore;
        }

        //庄家赢的钱
        //iBankerRealWinScore = -llBankerBaseScore;
        iUserWinScore[m_wBankerUser] -= llBankerBaseScore;
        iUserWinScore[m_wBankerUser] += bankerscore;
    }
    else
    {
        //其他情况，庄家输赢
        iUserWinScore[m_wBankerUser] = iBankerTotalWinScore;
    }

    //写入积分
    int64_t iRevenue[GAME_PLAYER] = {0};
//    int64_t CalculateAgentRevenue[GAME_PLAYER] = {0};
    int userCount=0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        GameEnd.add_dlwscore(0);
        GameEnd.add_dtotalscore(0);
        GameEnd.add_cboperate(0);
    }

    double res = 0.0;
//    int64_t revenue;
    vector<tagScoreInfo> scoreInfoVec;
    tagScoreInfo scoreInfo;
    // memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if (0 == m_cbPlayStatus[i])
        {
            continue;
        }
        if(!puserinfo)
        {
             continue;
        }

        if(!m_bPlayerIsOperation[i])//meiyou ca0 zuo
        {
//             GameEnd.set_cboperate(i, 0);
             m_cbPlayStatus[i] = 0;
        }
        GameEnd.set_cboperate(i, 0);

        //计算税收
        bool bAndroid = m_pITableFrame->IsAndroidUser(i);

        //统计库存
        if (bAndroid)
        {
            m_llStorageControl += iUserWinScore[i];
            res += iUserWinScore[i];
            //ResetTodayStorage();
        }else
        {

        }

        if (iUserWinScore[i] > 0)
        {
            iRevenue[i] = m_pITableFrame->CalculateRevenue(iUserWinScore[i]);
//            CalculateAgentRevenue[i] = m_pITableFrame->CalculateAgentRevenue(i,iUserWinScore[i]);
        }
//        else
//        {
//            iRevenue[i] = 0;
//            CalculateAgentRevenue[i] =m_pITableFrame->CalculateAgentRevenue(i,-iUserWinScore[i]);
//        }

        //LOG(WARNING) << "user  " << i << " win score: " << iUserWinScore[i];

        double iLWScore = iUserWinScore[i] - iRevenue[i];
        scoreInfo.clear();
        scoreInfo.chairId=i;

        scoreInfo.addScore=iLWScore;
        scoreInfo.startTime=m_dwStartTime;

        scoreInfo.revenue=iRevenue[i];

        uint8_t t[1];
        if( i != m_wBankerUser){
            t[0] = 0xff & cbUserJettonMultiple[i];
            scoreInfo.cardValue = iStr.str() + GlobalFunc::converToHex(t,1);
            scoreInfo.rWinScore = abs(iUserWinScore[i]);
        }else{
            t[0] = 0xff & cbBankerMultiple;
            scoreInfo.cardValue = iStr.str() + GlobalFunc::converToHex(t,1);
            scoreInfo.isBanker = 1;
            scoreInfo.rWinScore = rBankerWinScore;
        }
        scoreInfo.betScore = scoreInfo.rWinScore;
        m_replay.addResult(i,i,scoreInfo.betScore,scoreInfo.addScore,
                           to_string(m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT))+":"+GlobalFunc::converToHex( m_cbCardData[i] ,5),i == m_wBankerUser);
        scoreInfoVec.push_back(scoreInfo);

        userCount++;
        GameEnd.set_dlwscore(i, iLWScore);
        m_iUserWinScore[i] = iLWScore;
        GameEnd.set_dtotalscore(i,iLWScore+ puserinfo->GetUserScore());
        PaoMaDengCardType[i]=m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
        PaoMaDengWinScore[i]=iLWScore;
        LOG(INFO)<<"m_llStorageControl"<<"="<<m_llStorageControl;
        LOG(INFO)<<"m_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
        LOG(INFO)<<"m_llStorageHighLimit"<<"="<<m_llStorageHighLimit;

        LOG(INFO)<<"palyerCardtype="<<(int)puserinfo->GetUserId()<<"    "<<(int)PaoMaDengCardType[i];

        for(int j = 0; j < 5; ++j)
        {
            int inde = (int)m_cbCardData[i][j] % 16 + ((int)m_cbCardData[i][j] / 16) * 100;
            LOG(INFO)<<"player="<<(int)puserinfo->GetUserId()<<"      palyerCard="<<inde;
        }
    }

    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
//    ZeroMemory(&ceshi,sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
//    LOG(INFO)<<"update before-----------------"<<ceshi.lEndStorage;
//    LOG(INFO)<<"update Num-----------------"<<res;

    if( res > 0)
    {
        res = res*((1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0);// - m_pITableFrame->CalculateRevenue(res);
    }

    m_pITableFrame->UpdateStorageScore(res);
    m_pITableFrame->SaveReplay(m_replay);
    memset(&ceshi, 0, sizeof(ceshi));
//    ZeroMemory(&ceshi,sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
//    LOG(INFO)<<"update after -----------------"<<ceshi.lEndStorage;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
        {
            continue;
        }
        if (0 == m_cbPlayStatus[i])
        {
            continue;
        }
        double aaaa = puserinfo->GetUserScore();
        LOG(INFO)<<"userid="<<puserinfo->GetUserId()<<"    score="<<aaaa;
    }
    g_nRoundID++;
    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), mGameIds);

    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
        {
            continue;
        }
        if (0 == m_cbPlayStatus[i])
        {
            continue;
        }
        double aaa=puserinfo->GetUserScore();
        LOG(INFO)<<"userid="<<puserinfo->GetUserId()<<"    score="<<aaa;
    }
    //判断通杀通赔
    if (m_wPlayCount > 2)
    {
        if (wLostUserCount == 0)
        {
            GameEnd.set_cbendtype( 2);
        }
        else if (wLostUserCount == (m_wPlayCount - 1))
        {
            GameEnd.set_cbendtype( 1);
        }
        else
        {
            GameEnd.set_cbendtype( 0);
        }
    }
    else
    {
        GameEnd.set_cbendtype( -1);
    }

    //发送数据
    std::string data = GameEnd.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());

    PaoMaDeng();//發送跑馬燈
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(!user)
        {
            continue;
        }
        if(user->GetUserStatus() != sOffline)
        {
            user->SetUserStatus(sReady);
        }
        if((!m_bPlayerIsOperation[i]))
        {
             user->SetUserStatus(sOffline);
//             m_inPlayingStatus[i] = 0;

             LOG(INFO)<<"NO NO NO!!!!!!!!!!!!!!!!Operration and Setoffline";
        }
        LOG(INFO)<<"SET ALL PALYER  READY-------------------";
    }
}

void CTableFrameSink::GameTimerEnd()
{
    //muduo::MutexLockGuard lock(m_mutexLock);
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);

    // m_pITableFrame->SetGameStatus(201);//jie su

    LOG(INFO)<<"------------------------------------GameTimerEnd------------------------------------";

    m_dwEndTime = 0;

    if(!m_bContinueServer)
    {
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(user)
            {
//                openLog("3ConcludeGame=false  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true,ERROR_ENTERROOM_SERVERSTOP);
            }
        }
        return;
    }

    LOG(INFO)<<"------------------------------------GameTimerEnd1------------------------------------";

    // int AndroidNum = 0;
    // int realnum = 0;
    // for(int i = 0; i < GAME_PLAYER; ++i)
    // {
    //     shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
    //     if(user)
    //     {
    //         if(user->IsAndroidUser())
    //         {
    //             AndroidNum++;
    //         }
    //         realnum++;
    //     }
    // }
    // if(AndroidNum == realnum)
    // {
    //     for(int i = 0; i < GAME_PLAYER; ++i)
    //     {
    //         shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
    //         if(user)
    //             user->SetUserStatus(sOffline);
    //     }
    // }

    for(uint8_t i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if(user)
        {
            LOG(ERROR) << "Clean User, chairId" << (int)i;
            user->SetUserStatus(sOffline);
            m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_LONGTIME_NOOP);
            /*
            //openLog("  wanjia zhuang tai  renshu=%d",user->GetUserStatus());
            if(user->GetUserStatus()==sOffline)
            {
//                m_inPlayingStatus[i] = 0;
                openLog("4OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(INFO)<<"1Clear Player ClearTableUser userId:"<<user->GetUserId()<<" chairId:"<<user->GetChairId() ;
            }else if(user->GetUserScore()<m_pGameRoomInfo->enterMinScore)
            {
//                m_inPlayingStatus[i] = 0;
                openLog("5OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_SCORENOENOUGH);
                LOG(INFO)<<"2Clear Player ClearTableUser userId:"<<user->GetUserId()<<" chairId:"<<user->GetChairId() ;
            }
            */
        }
    }

    //设置游戏状态
//    m_wGameStatus = GAME_STATUS_READY;
    m_wGameStatus = GAME_STATUS_INIT;
    // LOG(INFO)<<"------------------------------------m_wGameStatus = GAME_STATUS_READY------------------------------------";

    //游戏结束
    //当前时间
    //std::string strTime = Utility::GetTimeNowString();
    //m_pITableFrame->ConcludeGame(GAME_STATUS1_FREE, strTime.c_str());
    m_pITableFrame->ClearTableUser(INVALID_CHAIR, true);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    // m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(TIME_READY, boost::bind(&CTableFrameSink::GameTimerReadyOver, this));
    // m_dwReadyTime = (uint32_t)time(NULL);
    //m_pITableFrame->SetGameTimer(ID_TIME_READY,TIME_READY*TIME_BASE);
    InitGameData();
}

//游戏消息
bool CTableFrameSink::OnGameMessage( uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    //muduo::MutexLockGuard lock(m_mutexLock);
    if( wChairID > 4)
    {
        return true;
    }
    int wRtn = 0;
    switch (wSubCmdID)
    {
        case NN_SUB_C_CALL_BANKER:					//叫庄消息
        {
            LOG(INFO)<<"***************************USER CALL BANK1";
            //游戏状态不对
            if (GAME_STATUS_CALL != m_wGameStatus)
            {
                LOG(INFO)<<"***************************GAME_STATUS_CALL != m_wGameStatus";
                return false;
            }

            //不是游戏玩家
            if (1 != m_cbPlayStatus[wChairID])
            {
                LOG(INFO)<<"***************************1 != m_cbPlayStatus[wChairID]";
                return false;
            }


            //变量定义
            NN_CMD_C_CallBanker  pCallBanker;
            pCallBanker.ParseFromArray(pData,wDataSize);

            LOG(INFO)<<"***************************1 pCallBanker"<<(int)pCallBanker.cbcallflag();

            //玩家叫庄
            wRtn = OnUserCallBanker(wChairID, pCallBanker.cbcallflag());
            m_bPlayerIsOperation[wChairID] = true;
            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserCallBanker wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn > 0;
        }
        case NN_SUB_C_ADD_SCORE:					//下注消息
        {
            //游戏状态不对
            if (GAME_STATUS_SCORE != m_wGameStatus)
            {
                return false;
            }

            //不是游戏玩家
            if (1 != m_cbPlayStatus[wChairID])
            {
                return false;
            }

            //变量定义
            NN_CMD_C_AddScore  pAddScore;
            pAddScore.ParseFromArray(pData,wDataSize);

            //玩家下注
            wRtn = OnUserAddScore(wChairID, pAddScore.cbjettonindex());
            m_bPlayerIsOperation[wChairID]=true;
            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserAddScore wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn > 0;
        }
        case NN_SUB_C_OPEN_CARD:					//开牌消息
        {
            m_bPlayerIsOperation[wChairID]=true;
            if (GAME_STATUS_OPEN != m_wGameStatus)
            {
                //游戏状态不对
                return false;
            }

            if (1 != m_cbPlayStatus[wChairID])
            {
                //不是游戏玩家
                return false;
            }

            //玩家下注
            wRtn = OnUserOpenCard(wChairID);

            //LOG_IF(INFO, PRINT_GAME_INFO) << "OnUserOpenCard wChairID=" << wChairID << ",wRtn=" << wRtn;
            return wRtn   > 0;
        }
    }
    return true;
}


////更新游戏配置
//void CTableFrameSink::UpdateGameConfig()
//{
//    if (NULL == m_pITableFrame)
//    {
//        return;
//    }
//}

//清除所有定时器
void CTableFrameSink::ClearAllTimer()
{
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);

}

void CTableFrameSink::ResetTodayStorage()
{
    uint32_t nTimeNow = time(NULL);
    // && isAcrossTheDay(m_iTodayTime, nTimeNow)
    if (nTimeNow - m_iTodayTime > 86400)
    {
        LOG(WARNING) << "AcrossTheDay 重置今日库存";
        m_llTodayStorage = 0;
        m_iTodayTime = nTimeNow;
    }
}

bool CTableFrameSink::IsTrustee(void)
{
    int dwShowTime = 0;
    if (m_wGameStatus == GAME_STATUS_CALL)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
        dwShowTime = 2; //发牌动画  1.5
        m_TimerIdCallBanker=m_TimerLoopThread->getLoop()->runAfter(TIME_CALL_BANKER + dwShowTime, boost::bind(&CTableFrameSink::GameTimerCallBanker, this));
    }
    else if (m_wGameStatus == GAME_STATUS_SCORE)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdCallBanker);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
        dwShowTime = 2; //抢庄动画  3.0
        m_TimerIdAddScore=m_TimerLoopThread->getLoop()->runAfter(TIME_ADD_SCORE + dwShowTime, boost::bind(&CTableFrameSink::GameTimerAddScore, this));
    }
    else if (m_wGameStatus == GAME_STATUS_OPEN)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        dwShowTime = 1; //开牌动画  2.0
        m_TimerIdOpenCard=m_TimerLoopThread->getLoop()->runAfter(TIME_OPEN_CARD + dwShowTime, boost::bind(&CTableFrameSink::GameTimerOpenCard, this));
    }
    else if (m_wGameStatus == GAME_STATUS_END)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);
        if(m_isMatch)
            m_TimerIdEnd=m_TimerLoopThread->getLoop()->runAfter(TIME_MATCH_GAME_END, boost::bind(&CTableFrameSink::GameTimerEnd, this));
        else
            m_TimerIdEnd=m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_END, boost::bind(&CTableFrameSink::GameTimerEnd, this));
        //LOG_IF(INFO, PRINT_GAME_INFO) << "OnEventGameEnd game end------";
    }
    return true;
}


void CTableFrameSink::RepositionSink()
{

}

//////////////////////////////////////////////////////////////////////////
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pSink)
{
    pSink.reset();
}
