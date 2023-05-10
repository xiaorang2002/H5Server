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
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/Tbnn.Message.pb.h"

#include "CMD_Game.h"
#include "GameLogic.h"
#include "TestCase.h"

using namespace Tbnn;
#include "TableFrameSink.h"

#define _OPENLOG_ false

//打印日志
#define PRINT_GAME_INFO					1
#define PRINT_GAME_WARNING				1


//////////////////////////////////////////////////////////////////////////
////定时器
//定时器时间
//#define TIME_BASE						1000
#define TIME_READY						1					//准备定时器(2S)
#define TIME_ENTER_PLAYING              1
#define TIME_ADD_SCORE					5					//下注定时器(5S)
#define TIME_OPEN_CARD					5					//开牌定时器(5S)
#define TIME_GAME_END					1					//结束定时器(5S)
//frontend got a animation less then 5s,so after animation they can click the restart button

//////////////////////////////////////////////////////////////////////////////////////
uint32_t CTableFrameSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 1,2,3,4,5 };	//下注倍数

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
        string dir = "./log/tbnn";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("tbnn");


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
{
    stockWeak = 0.0;
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
	//初始化桌子游戏人数概率权重
	for (int i = 0; i < GAME_PLAYER; ++i) {
        if (i == 5) {
            //6人局的概率0
            ratioGamePlayerWeight_[i] = 100;//25 * ratioGamePlayerWeightScale_;
        }
        else if (i == 4) {
			//5人局的概率0
			ratioGamePlayerWeight_[i] = 0;//25 * ratioGamePlayerWeightScale_;
		}
		else if (i == 3) {
			//4人局的概率85%
			ratioGamePlayerWeight_[i] = 0 * ratioGamePlayerWeightScale_;
		}
		else if (i == 2) {
			//3人局的概率10%
			ratioGamePlayerWeight_[i] = 0 * ratioGamePlayerWeightScale_;
		}
		else if (i == 1) {
			//2人局的概率5%
			ratioGamePlayerWeight_[i] = 0 * ratioGamePlayerWeightScale_;
		}
		else if (i == 0) {
			//单人局的概率5%
			ratioGamePlayerWeight_[i] = 0;//5 * ratioGamePlayerWeightScale_;
		}
		else {
			ratioGamePlayerWeight_[i] = 0;
		}
	}
	//计算桌子要求标准游戏人数
	poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
	MaxGamePlayers_ = 0;
}

CTableFrameSink::~CTableFrameSink(void)
{

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
            m_cbJettonMultiple[i] = m_pGameRoomInfo->jettons[i];
        }
//        memcpy(m_cbJettonMultiple, (vector<uint8_t>)m_pGameRoomInfo->jettons, sizeof(m_cbJettonMultiple));
        m_dwCellScore = m_pGameRoomInfo->floorScore;
        m_replay.cellscore = m_dwCellScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
		//m_replay.gameid = ThisGameId;//游戏类型
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
    }
    //初始化游戏数据
    m_wGameStatus = GAME_STATUS_INIT;
    //更新游戏配置
    InitGameData();
    return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData()
{
    //庄家
    m_wWinner = INVALID_CHAIR;
    //当前筹码
//    memset(m_cbCurJetton, 0, sizeof(m_cbCurJetton));
}
void CTableFrameSink::ReadConfig()
{
    if(!boost::filesystem::exists("./conf/tbnn_config.ini"))
    {
        return ;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini("./conf/tbnn_config.ini", pt);

    stockWeak=pt.get<double>("GAME_CONFIG.STOCK_WEAK",1.0);
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

    memset(m_RecordOutCard, 0, sizeof(m_RecordOutCard));
    m_mOutCardRecord.clear();

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        m_sAddScore[i] = -1;								//玩家下注(-1:未下注, 大于-1:对应筹码索引)
        m_sCardType[i] = -1;								//玩家开牌(-1:未开牌, 大于-1:对应牌型点数)
        m_iMultiple[i] = 1;									//倍数

        m_bPlayerIsOperation[i] = false;
    }

    //if (NULL != m_pITableFrame)
    //{//获取私人房信息
    //	m_pITableFrame->GetPrivateTableInfo(m_PrivateTableInfo);
    //}
    //时间初始化

    m_dwReadyTime = 0;										//ready时间
    m_dwEnterPlayingTime = 0;						        //EnterPlaying时间
    m_dwScoreTime = 0;										//下注时间
    m_dwOpenTime = 0;										//开牌时间
    m_dwEndTime = 0;										//结束时间
//    m_dwStartTime = 0;										//开始时间

    m_iBlacklistNum = 0;

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

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)//int64_t userId, uint32_t userIp)
{
    if(m_wGameStatus == GAME_STATUS_END)
    {
          return false;
    }

    //获取用户Id
    int64_t userId = pUser->GetUserId();
    ////////////////////////////////////////
    //初始化或空闲准备阶段，可以进入桌子
    if (m_wGameStatus < GAME_STATUS_LOCK_PLAYING)
    {
        ////////////////////////////////////////
        //达到游戏人数要求禁入游戏
        if (m_pITableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
            return false;
        }
        ////////////////////////////////////////
        //timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
        if (userId == -1) {
            if (totalMatchSeconds_ < timeoutMatchSeconds_) {
//                printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, userId, totalMatchSeconds_);
                return false;
            }
        }
        else {
            ////////////////////////////////////////
            //timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
            shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(userId);
            if (userItem) {
                if (userItem->IsAndroidUser()) {
                    if (totalMatchSeconds_ < timeoutMatchSeconds_) {
//                        printf("--- *** %.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", timeoutMatchSeconds_, userId, totalMatchSeconds_);
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
        shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
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

    LOG(ERROR)<<"---------------------------OnUserEnter---------------------------";

    uint32_t chairId = INVALID_CHAIR;
    
	shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);

    if(!pIServerUserItem)
    {
        LOG(ERROR)<<"-----------------OnUserEnter--Fail----pIServerUserItem==NULL----------------";
        return false;
    }
    
    chairId = pIServerUserItem->GetChairId();
    LOG(ERROR)<<"-------------OnUserEnter------user->GetChairID()="<<pIServerUserItem->GetChairId()<<"---------------------";
    if(chairId >= GAME_PLAYER )
    {
        LOG(ERROR)<<"---------------------------OnUserEnter--Fail----chairId="<<chairId <<"---------------------";
        return false;
    }

	//if (m_wGameStatus == GAME_STATUS_INIT) {
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
		////////////////////////////////////////
		//重置累计时间
		totalMatchSeconds_ = 0;
		printf("-- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 初始化游戏人数 MaxGamePlayers_：%d\n", MaxGamePlayers_);
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
			printf("--- *** &&&&&&&&&&&&&&&&&&& 当前游戏人数(%d)，开始游戏....................................................................\n", m_pITableFrame->GetPlayerCount(true));
			//GameTimereEnterPlayingOver();
			GameTimerReadyOver();
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
				m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
			}
		}
	}

    return true;
}

bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    //muduo::MutexLockGuard lock(m_mutexLock);
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

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);

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

    case GAME_STATUS_SCORE:		//下注状态
    {
        NN_MSG_GS_SCORE GameScore;
        GameScore.set_dcellscore((m_dwCellScore));
        uint32_t dwScoreTime = (uint32_t)time(NULL) - m_dwScoreTime;
        uint32_t cbTimeLeave = dwScoreTime >=TIME_ADD_SCORE ? 0 : dwScoreTime;		//剩余时间
        GameScore.set_cbtimeleave(cbTimeLeave);
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

            int cbUserJettonMultiple =((int)m_sAddScore[i] >= 0 ? (int)m_cbJettonMultiple[m_sAddScore[i]] : 0) ;

            GameScore.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameScore.add_cbjettonmultiple(m_cbJettonMultiple[i]);
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
        GameOpen.set_wwinuser((int)m_wWinner);
        GameOpen.set_roundid(mGameIds);
        LOG(INFO)<<"GameOpen.set_wbankeruser(m_wBankerUser)=="<<(int)m_wWinner;
        //庄家用户
//        GameOpen.set_cbwinnermutiple(m_sCallBanker[m_wWinner]);		//庄家倍数


        for (int i = 0; i < MAX_JETTON_MUL; ++i)
        {
            GameOpen.add_cbjettonmultiple(0);
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
            GameFree.add_cbjettonmultiple(m_cbJettonMultiple[i]);
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
            openLog("7OnUserLeft  ClearTableUser(i,true,true) user=%d   and chairID=%d",player->GetUserId(),player->GetChairId());
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

    LOG(ERROR)<<"---------------------------OnUserLeft1-----------m_wGameStatus----------------"<<(int)m_wGameStatus;

    openLog("  ------------------------------user left ");
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
            openLog("  wanjia zhuang tai  renshu=%d",user->GetUserStatus());
            if(user->GetUserStatus() == sOffline)
            {
                openLog(" 1ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                LOG(ERROR)<<"---------------------------user->GetUserStatus() != sPlaying ---ClearTableUser-----------charId-------------" << user->GetChairId();
                m_pITableFrame->ClearTableUser(i,true,true);
                LOG(ERROR)<<"---------------------------ClearTableUse1r---------------------------";
            }else if(user->GetUserScore()< m_pGameRoomInfo->enterMinScore )//m_pITableFrame->GetUserEnterMinScore())
            {
                openLog(" 2ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
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
    openLog("  yijing diaoyong m_pITableFrame->StartGame()  renshu=%d",wUserCount);
    if(wUserCount >= m_pGameRoomInfo->minPlayerNum)
    {
        //清理游戏数据
        ClearGameData();
        //初始化数据
        InitGameData();

        ReadConfig();
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
        openLog("  yijing diaoyong m_pITableFrame->StartGame() ");
    }
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    LOG(ERROR)<<"---------------------------OnGameStart---------------------------";
    m_replay.clear();
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;
    openLog("  kaishi OnGameStart()");
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
    GameStart.set_cbaddjettontime(TIME_ADD_SCORE);
    GameStart.set_roundid(mGameIds);
    for (int i = 0; i < MAX_JETTON_MUL; ++i)
        GameStart.add_cbjettonmultiple(m_cbJettonMultiple[i]);
    //随机扑克发牌
    //每随机10次换一次随机种子
//    m_nLottery += 1;
//    m_nLottery = m_nLottery % 10;

//    if (0 == m_nLottery)
//    {
//        m_GameLogic.GenerateSeed(m_pITableFrame->GetTableId());
//    }
    uint8_t cbTempArray[MAX_CARD_TOTAL] = { 0 };
    m_GameLogic.RandCardData(cbTempArray, sizeof(cbTempArray));
    memset(m_cbCardData, 0, sizeof(m_cbCardData));
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        GameStart.add_cbplaystatus(m_cbPlayStatus[i]);
        if (0 == m_cbPlayStatus[i])
            continue;
        m_cbLeftCardCount -= MAX_COUNT;
        //手牌
        memcpy(&m_cbCardData[i], &cbTempArray[m_cbLeftCardCount], sizeof(uint8_t)*MAX_COUNT);
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        m_replay.addPlayer(user->GetUserId(),user->GetAccount(),user->GetUserScore(),i);

        // 增加黑名单处理
        if(user->GetBlacklistInfo().status == 1 && user->GetBlacklistInfo().weight >= rand()%1000 )
        {
            m_iBlacklistNum++;
            m_bBlacklist[i]=true;

            LOG(WARNING) << "找到控制目标[" << i << "] 数量 "<< m_iBlacklistNum;
        }
        else
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
        openLog("111111111111 i=%d cardtype=%d isMax=%d",i, cardtype, isMax);
        std::string data = GameStartAi.SerializeAsString();
        m_pITableFrame->SendTableData(i, NN_SUB_S_GAME_START_AI, (uint8_t*)data.c_str(), data.size());
    }
 
    std::string data = GameStart.SerializeAsString();
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        if (0 == m_cbPlayStatus[i])
            continue;

        //发送数据
        m_pITableFrame->SendTableData(i, NN_SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
        //m_mOutCardRecord[i].append(" fetch " + StringCards(m_cbCardData[i], MAX_COUNT));
    }

    //设置游戏状态
    m_wGameStatus = GAME_STATUS_SCORE;//zhuan tai gai bian
    m_dwScoreTime = (uint32_t)time(NULL);

    LOG(ERROR)<<"------------------m_wGameStatus = GAME_STATUS_CALL---------IsTrustee---------------------------";

    //tmp......
//    RepayRecordStart();
//    if(m_bReplayRecordStart)
//    {
//        std::string data = GameStart.SerializeAsString();

//        openLog("NN_SUB_S_GAME_START msg size:%d",data.size());
//        RepayRecord(INVALID_CHAIR, NN_SUB_S_GAME_START, data.data(),data.size());
//    }
    //设置托管
    IsTrustee();

    return;
}

void CTableFrameSink::GameTimerAddScore()
{
    //muduo::MutexLockGuard lock(m_mutexLock);
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

        int wJettonIndex = 0;
        OnUserAddScore(i, wJettonIndex);
        //LOG_IF(INFO, PRINT_GAME_INFO) << "OnTimerMessage auto add int64_t chairId = " << i;
    }
    return 0;
}

//玩家下注
int CTableFrameSink::OnUserAddScore(int wChairID, int wJettonIndex)
{
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

    //记录下注
    m_sAddScore[wChairID] = wJettonIndex;
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(m_cbJettonMultiple[wJettonIndex]),-1,opAddBet,wChairID,wChairID);
    openLog("OnUserAddScore userid=%d wJettonIndex=%d",m_pITableFrame->GetTableUserItem(wChairID)->GetUserId(),wJettonIndex);
    NN_CMD_S_AddScoreResult CallResult;
    CallResult.set_waddjettonuser(wChairID);
    CallResult.set_cbjettonmultiple(m_cbJettonMultiple[wJettonIndex]);

    //处理下注
    int wTotalCount = 0;
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
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
    if (wTotalCount  == m_wPlayCount)
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
            if(m_iBlacklistNum == 0 )
            {
                LOG(ERROR) << "-------------不控制-------------";
                //分析玩家牌
                AnalyseCardEx();
            }
            else
            {
                //0，先找到要控制玩家 1，找出小牌 2，再交换两的牌
                for (int i = 0; i < GAME_PLAYER; ++i)
                {
                    //过滤非游戏玩家
                    if (1 != m_cbPlayStatus[i])
                        continue;

                    //过滤没有下注玩家
                    if (-1 == m_sAddScore[i])
                        continue;

                    //只交换机器人牌
                    shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
                    if (!user || user->IsAndroidUser()){
                        continue;
                    }
                                    
                   if(m_bBlacklist[i]){
                        // break;
                        AnalyseCardExPlus(i);
                        // LOG(ERROR) << "----处理数量-------"<<i;
                   }
                }
               
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
            //开牌时间
            sendCard.set_cbopentime(dwOpenTime);

            if(m_cbPlayStatus[i] != 0 )
            {
                //牌型（是否为炸弹）
                uint8_t cbCardType = m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
                sendCard.set_cbcardtype(cbCardType);

                //提牌数据(牛牌)
                uint8_t cbOXCard[MAX_COUNT] = { 0 };
                memcpy(cbOXCard, &m_cbCardData[i], sizeof(uint8_t)*MAX_COUNT);
                m_GameLogic.GetOxCard(cbOXCard);

                // 整理玩家牌
                for(int j = 0; j < MAX_COUNT; ++j)
                {
                    sendCard.set_cbsendcard(j, m_cbCardData[i][j]);
                    sendCard.set_cboxcard(j, cbOXCard[j]);
                }
            }
            std::string data = sendCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, NN_SUB_S_SEND_CARD, (uint8_t*)data.c_str(), data.size());

            //tmp......
//            if(m_bReplayRecordStart && 1 == m_cbPlayStatus[i])
//                //if(m_bReplayRecordStart )
//            {
//                RepayRecord(i, NN_SUB_S_SEND_CARD, data.data(),data.size());
//            }
        }
    }
    return 4;
}

void CTableFrameSink::AnalyseCardExPlus(int iWinSeat)
{
    LOG(ERROR) << "-------------控制-------------" << iWinSeat;

    //扑克变量
    uint8_t cbUserCardData[GAME_PLAYER][MAX_COUNT];
    memcpy(cbUserCardData, m_cbCardData, sizeof(cbUserCardData));

    //变量定义
    int iTargetIdx = iWinSeat;
    //查找数据
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //过滤非游戏玩家
        if (1 != m_cbPlayStatus[i])
            continue;

        //过滤没有下注玩家
        if (-1 == m_sAddScore[i])
            continue;

        if (i == iWinSeat)
        {
            LOG(ERROR) << "--------相同桌不判断------" << i << " " << iWinSeat;
            continue;
        }

        //只交换机器人牌
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if (!user || user->IsAndroidUser() == false)
        {
            LOG(ERROR) << "--------不交换真人------" << i;
            continue;
        }

        //对比扑克
        if (m_GameLogic.CompareCard(cbUserCardData[i], m_cbCardData[iWinSeat]) == false)
        {
            LOG(WARNING) << "找到合适牌下标[" << i << "],控制目标[" << iWinSeat << "]";
            //找到控制玩家的牌小于当前牌
            iTargetIdx = i;
            break;
        }
    }

    if (iTargetIdx != iWinSeat)
    {
        LOG(ERROR) << "换牌前";
        for (int i = 0; i < MAX_COUNT; i++)
            LOG(WARNING) << "[" << (int)m_cbCardData[iWinSeat][i] << "]<==>" << (int)m_cbCardData[iTargetIdx][i];

        //换牌
        std::swap(m_cbCardData[iWinSeat], m_cbCardData[iTargetIdx]);
        LOG(WARNING) << "换牌[" << iTargetIdx << "],控制目标[" << iWinSeat << "]";

        LOG(ERROR) << "换牌后";
        for (int i = 0; i < MAX_COUNT; i++)
            LOG(WARNING) << "[" << (int)m_cbCardData[iWinSeat][i] << "]<==>" << (int)m_cbCardData[iTargetIdx][i];
    }
    else
    {
        LOG(ERROR) << "-------已经是最小，无需换牌-------";
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
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
         bSteal = true;
    }
    else
    {
        if (m_llStorageControl > llLowerLimitVal && m_llStorageControl < llHighLimitVal)
        {
            openLog("-------------------m_llStorageControl=%.2f",m_llStorageControl);
            openLog("-------------------m_llStorageminis=%.2f",llLowerLimitVal);
            openLog("-------------------m_llStoragemax=%.2f",llHighLimitVal);
            return;
        }
        else if (m_llStorageControl > llHighLimitVal )
        {
            openLog("-------------------m_llStorageControl=%.2f",m_llStorageControl);
            openLog("-------------------m_llStorageminis=%.2f",llLowerLimitVal);
            openLog("-------------------m_llStoragemax=%.2f",llHighLimitVal);
            openLog("suan fa 吐分");
            bSteal = false;

        }
        else if (m_llStorageControl < llLowerLimitVal )
        {
            bSteal = true;
            openLog("-------------------m_llStorageControl=%.2f",m_llStorageControl);
            openLog("-------------------m_llStorageminis=%.2f",llLowerLimitVal);
            openLog("-------------------m_llStoragemax=%.2f",llHighLimitVal);
            openLog("suan fa 吸分");
        }

        if (m_llStorageControl > llHighLimitVal && rand() % 100 <= 30)
            return ;
        else if (m_llStorageControl < llLowerLimitVal && rand() % 100 <= 20)
            return ;
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
    //muduo::MutexLockGuard lock(m_mutexLock);
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

    //	m_mOutCardRecord[wChairID].append(" " + StringCards(m_cbCardData[wChairID], MAX_COUNT));

    //倍数
    m_iMultiple[wChairID] =(int)m_GameLogic.GetMultiple(m_cbCardData[wChairID]);

    //获得牛牛牌
    uint8_t cbOXCard[MAX_COUNT] = { 0 };
    memcpy(cbOXCard, &m_cbCardData[wChairID], sizeof(uint8_t)*MAX_COUNT);
    m_GameLogic.GetOxCard(cbOXCard);
    memcpy(&m_cbOXCard[wChairID], cbOXCard, sizeof(cbOXCard));

//    char tmp[20] = { 0 };
//    GetValue(wChairID, tmp);
//    string cardName = GetCardValueName(wChairID);
//    openLog("111111 wChairID=%d NickName=%s [%s]", wChairID, m_pITableFrame->GetTableUserItem(wChairID)->GetNickName(), cardName.c_str());
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
    //SendTableData(INVALID_CHAIR, NN_SUB_S_OPEN_CARD_RESULT, &OpenCardResult, sizeof(NN_CMD_S_OpenCardResult));

    //tmp......
//    if(m_bReplayRecordStart)
//    {
//        RepayRecord(INVALID_CHAIR,NN_SUB_S_OPEN_CARD_RESULT, data.c_str(),data.size());
//    }
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

    //LOG_IF(INFO, PRINT_GAME_INFO) << "OnEventGameEnd";

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

    //tmp......
//    if(m_bReplayRecordStart)
//    {
//        RepayRecordEnd();
//    }
    //设置托管
    IsTrustee();
}

//普通场游戏结算
void CTableFrameSink::NormalGameEnd(int dwChairID)
{
    NN_CMD_S_GameEnd GameEnd;
	//记录当局游戏详情binary
	Tbnn::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	//牌局倍数 ///
	int32_t calcMulti[GAME_PLAYER] = { 0 };
    //下注倍数
    int64_t cbUserJettonMultiple[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //玩家牌型倍数
    int32_t cbUserCardMultiple[GAME_PLAYER]={0};// = { 0,0,0,0 };
    m_wWinner = 0;
    stringstream iStr;
    for(int i = 0; i < GAME_PLAYER;i++)
    {
        iStr << GlobalFunc::converToHex( m_cbCardData[i] ,5);

        if(0 == m_cbPlayStatus[i])
            continue;//没有参与游戏不参与结算

        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo)
            continue;

        // 玩家下注倍率
        cbUserJettonMultiple[i] = m_cbJettonMultiple[m_sAddScore[i]];
        // 玩家牌倍
        cbUserCardMultiple[i] = m_GameLogic.GetMultiple(m_cbCardData[i]);
        if( i != 0){
            if(m_GameLogic.CompareCard(m_cbCardData[i], m_cbCardData[m_wWinner])){
                m_wWinner = i;
            }
        }else{
             m_wWinner = 0;
        }
    }
    LOG(INFO) << "WInner : " <<(int)m_wWinner;
    /****
    庄家信息
    ****/
    //庄家本金
    shared_ptr<IServerUserItem> pBankerinfo = m_pITableFrame->GetTableUserItem(m_wWinner);
    int64_t llBankerBaseScore;
    if(pBankerinfo)
    {
        llBankerBaseScore= pBankerinfo->GetUserScore();
    }
    else
    {
        llBankerBaseScore = 0;
    }

    //赢家总输赢金币
    // int64_t iBankerTotalWinScore = 0;
    //赢家牌型倍数
    int cbBankerCardMultiple = cbUserCardMultiple[m_wWinner];

    //闲家本金
    int64_t llUserBaseScore[GAME_PLAYER]={0};// = { 0,0,0,0 };
    //闲家各自赢分
    int64_t iUserWinScore[GAME_PLAYER]={0};// = { 0,0,0,0 };
    memset(m_iUserWinScore,0x00,sizeof(m_iUserWinScore));

    //闲家赢总倍数
    // int64_t wLostUserTotalMultiple = 0;


    //计算输赢
    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        shared_ptr<IServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(i);
        if(!puserinfo || i == m_wWinner )
            continue;
//        LOG(INFO)<<"m_cbJettonMultiple[m_sAddScore[i]]"<<(int)m_cbJettonMultiple[m_sAddScore[i]]<<"m_sAddScore[i]"<<(int)m_sAddScore[i];
//        LOG(INFO)<<" cbUserCardMultiple[i]"<< (int)cbUserCardMultiple[i];

        //闲家原始积分
        llUserBaseScore[i] = puserinfo->GetUserScore();

        //玩家输分 =  底分 * 下注倍数 * 抢庄倍数 *  牌型倍数
        int64_t iTempScore = m_dwCellScore *  cbUserJettonMultiple[i] * cbUserJettonMultiple[m_wWinner] * cbBankerCardMultiple;
        if (iTempScore > llUserBaseScore[i])
        {
            iTempScore = llUserBaseScore[i];
        }
        // 
        if (iTempScore > llBankerBaseScore)
        {
            iTempScore = llBankerBaseScore;
        }

        //玩家总输分
        iUserWinScore[i] = -iTempScore;
        iUserWinScore[m_wWinner] += iTempScore;

		//牌局倍数
        calcMulti[i] = cbUserJettonMultiple[i] * cbUserJettonMultiple[m_wWinner] * cbBankerCardMultiple;//m_iMultiple[i] * m_sCallBanker[m_wBankerUser] * cbUserJettonMultiple[i];
        calcMulti[m_wWinner] += cbUserJettonMultiple[i] * cbUserJettonMultiple[m_wWinner] * cbBankerCardMultiple;//m_iMultiple[m_wBankerUser] * m_sCallBanker[m_wBankerUser] * cbUserJettonMultiple[i];
    }

    iStr << m_wWinner;
    //庄家计算出来的赢分
    //    int64_t iBankerCalWinScore = 0;
    /*
    //如果庄家赢的钱大于庄家的基数 20，10
    if (iBankerTotalWinScore > llBankerBaseScore)
    {
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_cbPlayStatus[i] == 0 || i == m_wWinner)
                continue;
            // 玩家当前分
            int64_t curScore = llUserBaseScore[i];
            // 当前玩家真正要输的金币
            int64_t iUserNeedLostScore =  curScore < llBankerBaseScore ? curScore : llBankerBaseScore; 
         
           
            //当前玩家真正要输的金币分摊一下
            // double iUserNeedLostScore = llBankerBaseScore * iUserWinScore[i]/iBankerTotalWinScore;
            // if (-iUserNeedLostScore > llUserBaseScore[i])
            // {
            //     iUserNeedLostScore = -llUserBaseScore[i];
            // }
            
            //-95 ，-18 
            if(iUserWinScore[i] < -iUserNeedLostScore)
                iUserWinScore[i] = -iUserNeedLostScore;
            iUserWinScore[m_wWinner] += (-iUserWinScore[i]);
        }

    } 
    else
    {
        //其他情况，庄家输赢
        iUserWinScore[m_wWinner] = iBankerTotalWinScore;
    }
    */

    //写入积分
    int64_t iRevenue[GAME_PLAYER] = {0};// { 0,0,0,0 };
    int64_t CalculateAgentRevenue[GAME_PLAYER] = {0};//{ 0,0,0,0 };
    bool bNeedRecord = false;
    int userCount=0;

    for(int i = 0; i < GAME_PLAYER; ++i)
    {
        GameEnd.add_dlwscore(0);
        GameEnd.add_dtotalscore(0);
    }

    double res = 0.0;
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

        //计算税收
        bool bAndroid = m_pITableFrame->IsAndroidUser(i);

        //统计库存
        if (bAndroid)
        {
            m_llStorageControl += iUserWinScore[i];
            res += iUserWinScore[i];
        }else
        {
            bNeedRecord = true;
        }

        if (iUserWinScore[i] > 0)
        {
            iRevenue[i] = m_pITableFrame->CalculateRevenue(iUserWinScore[i]);
        }


        int64_t iLWScore = iUserWinScore[i] - iRevenue[i];
        scoreInfo.clear();
        scoreInfo.chairId=i;

        scoreInfo.addScore=iLWScore;
        scoreInfo.startTime=m_dwStartTime;
        scoreInfo.revenue=iRevenue[i];
        uint8_t t[1];
        t[0] = 0xff & cbUserJettonMultiple[i];
        scoreInfo.cardValue = iStr.str() + GlobalFunc::converToHex(t,1);
        scoreInfo.rWinScore = abs(iUserWinScore[i]);
        scoreInfo.betScore = scoreInfo.rWinScore;
        m_replay.addResult(i,i,scoreInfo.betScore,scoreInfo.addScore,
                           to_string(m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT))+":"+GlobalFunc::converToHex( m_cbCardData[i] ,5),false);
         scoreInfoVec.push_back(scoreInfo);


        userCount++;
        GameEnd.set_dlwscore(i, iLWScore);
        m_iUserWinScore[i] = iLWScore;
        GameEnd.set_dtotalscore(i,iLWScore+ puserinfo->GetUserScore());
        PaoMaDengCardType[i]=m_GameLogic.GetCardType(m_cbCardData[i], MAX_COUNT);
        PaoMaDengWinScore[i]=iLWScore;
//        LOG(INFO)<<"m_llStorageControl"<<"="<<m_llStorageControl;
//        LOG(INFO)<<"m_llStorageLowerLimit"<<"="<<m_llStorageLowerLimit;
//        LOG(INFO)<<"m_llStorageHighLimit"<<"="<<m_llStorageHighLimit;

//        LOG(INFO)<<"palyerCardtype="<<(int)puserinfo->GetUserId()<<"    "<<(int)PaoMaDengCardType[i];

        for(int j = 0; j < 5; ++j)
        {
            int inde = (int)m_cbCardData[i][j] % 16 + ((int)m_cbCardData[i][j] / 16) * 100;
//            LOG(INFO)<<"player="<<(int)puserinfo->GetUserId()<<"      palyerCard="<<inde;
        }

		Tbnn::PlayerRecordDetail* detail = details.add_detail();
		//账号/昵称
		detail->set_account(std::to_string(m_pITableFrame->GetTableUserItem(i)->GetUserId()));
		//座椅ID
		detail->set_chairid(i);
		//是否庄家
		detail->set_isbanker(m_wWinner == i ? true : false);
		//手牌/牌型
		Tbnn::HandCards* pcards = detail->mutable_handcards();
		pcards->set_cards(&(m_cbOXCard[i])[0], MAX_COUNT);
		pcards->set_ty(m_sCardType[i]);
		//牌型倍数
		pcards->set_multi(m_iMultiple[i]);
		//携带积分
		detail->set_userscore(i == m_wWinner ? llBankerBaseScore : llUserBaseScore[i]);
		//房间底注
		detail->set_cellscore(m_pITableFrame->GetGameRoomInfo()->floorScore);
		//下注/抢庄倍数
		detail->set_multi(/*i == m_wWinner ? cbUserJettonMultiple[i] : */cbUserJettonMultiple[i]);
		//牌局倍数
		detail->set_calcmulti(calcMulti[i]);
		//输赢积分
		detail->set_winlostscore(iLWScore);
    }
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		m_replay.detailsData = details.SerializeAsString();
	}
    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
//    ZeroMemory(&ceshi,sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    m_pITableFrame->SaveReplay(m_replay);
//    LOG(INFO)<<"update before-----------------"<<ceshi.lEndStorage;
//    LOG(INFO)<<"update Num-----------------"<<res;

    if( res > 0)
    {
        //CalculateRevenue 是抽水调用抽5%，里面实现简单，改成机器人抽1%
        //res = res - m_pITableFrame->CalculateRevenue(res);
        res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;// res - m_pITableFrame->CalculateRevenue(res);
    }

    m_pITableFrame->UpdateStorageScore(res);

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

    //发送数据
    std::string data = GameEnd.SerializeAsString();
    m_pITableFrame->SendTableData(INVALID_CHAIR, NN_SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
     PaoMaDeng();//發送跑馬燈
    //tmp......
//    if(m_bReplayRecordStart)
//    {
//        RepayRecord(INVALID_CHAIR,NN_SUB_S_GAME_END, data.c_str(),data.size());
//    }

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
                openLog("3ConcludeGame=false  ClearTableUser(i,true,true) user=%d   and chairID=%d",user->GetUserId(),user->GetChairId());
                user->SetUserStatus(sOffline);
                m_pITableFrame->ClearTableUser(i, true, true,ERROR_ENTERROOM_SERVERSTOP);
            }
        }
        return;
    }

    LOG(INFO)<<"------------------------------------GameTimerEnd1------------------------------------";
    
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
    if( wChairID >= GAME_PLAYER || wChairID < 0)
    {
        return true;
    }
    int wRtn = 0;
    switch (wSubCmdID)
    {
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



//清除所有定时器
void CTableFrameSink::ClearAllTimer()
{
    //m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnterPlaying);
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
    if (m_wGameStatus == GAME_STATUS_SCORE)
    {
        //return true;
        //删除超时定时器
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
        dwShowTime = 2; //动画  3.0
        openLog("  m_TimerLoopThread->getLoop()->runAfter(TIME_ADD_SCORE)");
        LOG(INFO) << "SCORE TIMER:" << (int)(TIME_ADD_SCORE+dwShowTime);
        m_TimerIdAddScore=m_TimerLoopThread->getLoop()->runAfter(TIME_ADD_SCORE + dwShowTime, boost::bind(&CTableFrameSink::GameTimerAddScore, this));
    }
    else if (m_wGameStatus == GAME_STATUS_OPEN)
    {
        //return true;
        //删除超时定时器
        //m_pITableFrame->KillGameTimer(ID_TIME_ADD_SCORE);
        //m_pITableFrame->KillGameTimer(ID_TIME_OPEN_CARD);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdAddScore);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        dwShowTime = 1; //开牌动画  2.0
        openLog("  m_TimerLoopThread->getLoop()->runAfter(TIME_OPEN_CARD)");
        //m_pITableFrame->SetGameTimer(ID_TIME_OPEN_CARD, TIME_BASE * (TIME_OPEN_CARD + dwShowTime));
        m_TimerIdOpenCard=m_TimerLoopThread->getLoop()->runAfter(TIME_OPEN_CARD + dwShowTime, boost::bind(&CTableFrameSink::GameTimerOpenCard, this));
    }
    else if (m_wGameStatus == GAME_STATUS_END)
    {
        //删除超时定时器
        //m_pITableFrame->KillGameTimer(ID_TIME_OPEN_CARD);
        //m_pITableFrame->KillGameTimer(ID_TIME_GAME_END);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdOpenCard);
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdEnd);
//        dwShowTime =1000;//结束动画  5.0
        //m_pITableFrame->SetGameTimer(ID_TIME_GAME_END,TIME_BASE * TIME_GAME_END);
        openLog("  m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_END)");
        m_TimerIdEnd=m_TimerLoopThread->getLoop()->runAfter(TIME_GAME_END, boost::bind(&CTableFrameSink::GameTimerEnd, this));
        //LOG_IF(INFO, PRINT_GAME_INFO) << "OnEventGameEnd game end------";
    }
    return true;
}

uint8_t CTableFrameSink::GetCardTypeStorgOne()
{
    int dwRandNum = rand() % 10000;
    if (dwRandNum < 1040)
    {
        return 0;//10.4%
    }
    else if (dwRandNum >= 1040 && dwRandNum < 1936)
    {
        return 1;//8.96%
    }
    else if (dwRandNum >= 1936 && dwRandNum < 2682)
    {
        return 2;//7.46%
    }
    else if (dwRandNum >= 2682 && dwRandNum < 3578)
    {
        return 3;//8.96%
    }
    else if (dwRandNum >= 3578 && dwRandNum < 3877)
    {
        return 4;//2.99%
    }
    else if (dwRandNum >= 3877 && dwRandNum < 4328)
    {
        return 5;//4.51%
    }
    else if (dwRandNum >= 4328 && dwRandNum < 5522)
    {
        return 6;//11.94%
    }
    else if (dwRandNum >= 5522 && dwRandNum < 5970)
    {
        return 7;//4.48%
    }
    else if (dwRandNum >= 5970 && dwRandNum < 7015)
    {
        return 8;//10.45%
    }
    else if (dwRandNum >= 7015 && dwRandNum < 8060)
    {
        return 9;//10.45%
    }
    else if (dwRandNum >= 8060 && dwRandNum < 10000)
    {
        return 10;//19.4%
    }

    return 10;
}

uint8_t CTableFrameSink::GetCardTypeStorgTwo()
{
    int dwRandNum = rand() % 10000;
    if (dwRandNum < 1659)
    {
        return 0;//16.59%
    }
    else if (dwRandNum >= 1659 && dwRandNum < 2300)
    {
        return 1;//6.41%
    }
    else if (dwRandNum >= 2300 && dwRandNum < 2947)
    {
        return 2;//6.47%
    }
    else if (dwRandNum >= 2947 && dwRandNum < 3688)
    {
        return 3;//7.41%
    }
    else if (dwRandNum >= 3688 && dwRandNum < 4435)
    {
        return 4;//7.47%
    }
    else if (dwRandNum >= 4435 && dwRandNum < 5291)
    {
        return 5;//8.56%
    }
    else if (dwRandNum >= 5291 && dwRandNum < 6138)
    {
        return 6;//8.47%
    }
    else if (dwRandNum >= 6138 && dwRandNum < 7079)
    {
        return 7;//9.41%
    }
    else if (dwRandNum >= 7079 && dwRandNum < 8026)
    {
        return 8;//9.47%
    }
    else if (dwRandNum >= 8026 && dwRandNum < 8967)
    {
        return 9;//9.41%
    }
    else if (dwRandNum >= 8967 && dwRandNum < 10000)
    {
        return 10;//10.33%
    }

    return 10;
}
/*
string CTableFrameSink::GetCardValueName(int index)
{
    string strName;

    for(int i = 0 ; i < MAX_COUNT; ++i)
    {
        //获取花色
        uint8_t cbColor = m_GameLogic.GetCardColor(m_cbCardData[index][i]);
        uint8_t cbValue = m_GameLogic.GetCardValue(m_cbCardData[index][i]);
        switch(cbColor)
        {
            case 0:
                strName += "方";
                break;
            case 1:
                strName += "梅";
                break;
            case 2:
                strName += "红";
                break;
            case 3:
                strName += "黑";
                break;
            default:
                break;
        }
        switch(cbValue)
        {
            case 1:
                strName += "A";
                break;
            case 2:
                strName += "2";
                break;
            case 3:
                strName += "3";
                break;
            case 4:
                strName += "4";
                break;
            case 5:
                strName += "5";
                break;
            case 6:
                strName += "6";
                break;
            case 7:
                strName += "7";
                break;
            case 8:
                strName += "8";
                break;
            case 9:
                strName += "9";
                break;
            case 10:
                strName += "10";
                break;
            case 11:
                strName += "J";
                break;
            case 12:
                strName += "Q";
                break;
            case 13:
                strName += "K";
                break;
            default:
                break;
        }
    }
    return strName;
}
*/
void  CTableFrameSink::openLog(const char *p, ...)
{
 #if _OPENLOG_
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//tbnn//tbnn_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pGameRoomInfo->roomId);//m_pITableFrame->GetGameKindInfo()->nConfigId);
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
        return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] [TABLEID:%d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,m_pITableFrame->GetTableId());
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
#endif
}

bool CTableFrameSink::RepayRecorduserinfo(int wChairID)
{
//    shared_ptr<CServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(wChairID);
//    if(!puserinfo)
//        return true;
//    ::GameServer::Message::MSG_SC_UserBaseInfo userinfo;

//    ::Game::Common::Header* pheader = userinfo.mutable_header();
//    pheader->set_sign(1);
//    pheader->set_crc(1);
//    pheader->set_mainid(Game::Common::MSGGS_MAIN_FRAME);
//    pheader->set_subid(::GameServer::Message::SUB_S2C_USER_ENTER);

//    userinfo.set_userid(puserinfo->GetUserID());
//    userinfo.set_nickname(puserinfo->GetNickName(),strlen(puserinfo->GetNickName()));
//    userinfo.set_clientip("");
//    userinfo.set_dlongitude(0);
//    userinfo.set_szlocation(puserinfo->GetLocation(),strlen(puserinfo->GetLocation()));
//    userinfo.set_cbheadindex(puserinfo->GetHeaderID());
//    userinfo.set_cbgender(puserinfo->GetGender());
//    userinfo.set_cbviplevel(puserinfo->GetVipLevel());
//    userinfo.set_cbviplevel2(0);
//    userinfo.set_dwgem(0);
//    userinfo.set_llscore(puserinfo->GetUserScore());
//    userinfo.set_tableid(puserinfo->GetTableID());
//    userinfo.set_chairid(puserinfo->GetChairID());
//    userinfo.set_usstatus(puserinfo->GetUserStatus());
//    userinfo.set_cbheadboxid(puserinfo->GetHeadBoxID());

//    std::string data = userinfo.SerializeAsString();
//    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
//    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_FRAME);
//    pcmddate->set_wsubcmdid(::GameServer::Message::SUB_S2C_USER_ENTER);
//    pcmddate->set_msgdata(data.data(),(dword)(data.size()));
//    pcmddate->set_userid(0);
//    pcmddate->set_chairid(6);
    return true;
}


bool CTableFrameSink::RepayRecorduserinfoStatu(int wChairID,int status)
{
//    shared_ptr<CServerUserItem> puserinfo = m_pITableFrame->GetTableUserItem(wChairID);
//    if(!puserinfo)
//        return true;

//    ::GameServer::Message::MSG_SC_GameUserStatus userstatu;

//    ::Game::Common::Header* puserstatu = userstatu.mutable_header();
//    puserstatu->set_sign(1);
//    puserstatu->set_crc(1);
//    puserstatu->set_mainid(Game::Common::MSGGS_MAIN_FRAME);
//    puserstatu->set_subid(::GameServer::Message::SUB_S2C_USER_STATUS);


//    userstatu.set_userid(puserinfo->GetUserID());
//    userstatu.set_tableid(puserinfo->GetTableID());
//    userstatu.set_chairid(puserinfo->GetChairID());
//    userstatu.set_usstatus(status);

//    std::string data = userstatu.SerializeAsString();
//    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
//    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_FRAME);
//    pcmddate->set_wsubcmdid(::GameServer::Message::SUB_S2C_USER_STATUS);
//    pcmddate->set_msgdata(data.data(),(dword)(data.size()));
//    pcmddate->set_userid(0);
//    pcmddate->set_chairid(6);
    return true;
}

bool CTableFrameSink::RepayRecordStart()
{
//    m_bReplayRecordStart = true;
//    m_UserReplayRecord.Clear();

//    ::GameServer::Message::MSG_SC_EnterRoomSucc EnterRoomSucc;
//    ::Game::Common::Header* pEnterRoomSucc = EnterRoomSucc.mutable_header();
//    pEnterRoomSucc->set_sign(1);
//    pEnterRoomSucc->set_crc(1);
//    pEnterRoomSucc->set_mainid(Game::Common::MSGGS_MAIN_FRAME);
//    pEnterRoomSucc->set_subid(::GameServer::Message::SUB_S2C_ENTER_ROOM_SUCC);

//    EnterRoomSucc.set_gameid(m_pITableFrame->GetGameKindInfo()->nGameId);
//    EnterRoomSucc.set_kindid(m_pITableFrame->GetGameKindInfo()->nKindId);
//    EnterRoomSucc.set_roomkindid(m_pITableFrame->GetGameKindInfo()->nRoomKindId);
//    EnterRoomSucc.set_cellscore(m_pITableFrame->GetGameKindInfo()->nCellScore);
//    EnterRoomSucc.set_configid(m_pITableFrame->GetGameKindInfo()->nConfigId);

//    std::string data = EnterRoomSucc.SerializeAsString();
//    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
//    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_FRAME);
//    pcmddate->set_wsubcmdid(::GameServer::Message::SUB_S2C_ENTER_ROOM_SUCC);
//    pcmddate->set_msgdata(data.data(),(dword)(data.size()));
//    pcmddate->set_userid(0);
//    pcmddate->set_chairid(6);
//    for (int i = 0; i < GAME_PLAYER; ++i)
//    {
//        if (0 == m_cbPlayStatus[i])
//            continue;

//        RepayRecordGameScene(i,false);
//        RepayRecorduserinfo(i);
//    }
    return true;
}
bool CTableFrameSink::RepayRecordEnd()
{
//    if (!m_bReplayRecordStart)
//        return false;

//    int usercount = m_pITableFrame->GetPlayerCount(false);
//    if(usercount==0)
//        return true;

//    //发送数据
//    std::string data = m_UserReplayRecord.SerializeAsString();
//    ISaveReplayRecord* pSaveReplayRecord = nullptr;
//    m_pITableFrame->QueryInterface(guid_ISaveReplay,(void **)&pSaveReplayRecord);

//    {
//        tagGameRecPlayback  mGameRecPlayback;

//        //mGameRecPlayback.configid = m_pITableFrame->GetGameKindInfo()->nConfigId;
//        mGameRecPlayback.rec_roundid = g_nRoundID;
//        shared_ptr<CServerUserItem> pBankuserItem =m_pITableFrame->GetTableUserItem(m_wBankerUser);
//        if(!pBankuserItem)
//        {
//            return true;
//        }
//        string mbankAccount = pBankuserItem->getAccount();
//        mGameRecPlayback.banker_userid = atoi(mbankAccount.c_str());
//        for (int i = 0; i < GAME_PLAYER; ++i)
//        {
//            if (0 == m_cbPlayStatusRecord[i])
//                continue;

//            shared_ptr<CServerUserItem> pUserItem =m_pITableFrame->GetTableUserItem(i);
//            if(!pUserItem)
//            {
//                return true;
//            }
//            string mAccount = pUserItem->getAccount();
//            mGameRecPlayback.player[i].account = atoi(mAccount.c_str());
//            mGameRecPlayback.player[i].userid = m_pITableFrame->GetTableUserItem(i)->GetUserID();
//            mGameRecPlayback.player[i].changed_int64_t = m_iUserWinScore[i];
//        }
//        mGameRecPlayback.content = data;

//        pSaveReplayRecord->SaveReplayRecord(mGameRecPlayback);
//    }

//    m_UserReplayRecord.Clear();
//    m_bReplayRecordStart = false;
    //delete replyRecordData;不能删除，已托管给SaveReplayRecord函数
    return true;
}

bool CTableFrameSink::RepayRecord(int wChairID,uint8_t wSubCmdId, const char* data, int64_t wSize)
{
    //    if(wSize<=0 || data==nullptr)
    //        return true;

//    ::Game::Common::GamePlaybackRecordItem* pcmddate = m_UserReplayRecord.add_recitem();
//    pcmddate->set_wmaincmdid(Game::Common::MSGGS_MAIN_GAME);
//    pcmddate->set_wsubcmdid(wSubCmdId);
//    pcmddate->set_msgdata(data, wSize);
//    if(INVALID_CHAIR != wChairID)
//    {
//        shared_ptr<CServerUserItem> player= m_pITableFrame->GetTableUserItem(wChairID);
//        if(!player)
//        {
//            return true;
//        }
//        pcmddate->set_userid(player->GetUserID());
//        pcmddate->set_chairid(wChairID);
//    }
//    else
//    {
//        pcmddate->set_userid(0);
//        pcmddate->set_chairid(6);
//    }
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
