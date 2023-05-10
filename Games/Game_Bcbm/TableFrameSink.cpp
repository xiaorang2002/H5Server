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
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/BenCiBaoMa.Message.pb.h"

#include "BetArea.h" 
#include "BcbmAlgorithm.h"
#include "TableFrameSink.h"

//宏定义
#ifndef _UNICODE
#define myprintf    _snprintf
#else
#define myprintf    swprintf
#endif

#define RESULT_WIN (1)
#define RESULT_LOST (-1)

//initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;

#ifdef __HJSDS__
        google::InitGoogleLogging("benzbmw");
#else
        dir += "/benzbmw";
        google::InitGoogleLogging("benzbmw");
#endif //__HJSDS__

        FLAGS_log_dir = dir;
        // set std output special log content value.
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit; // 声明全局变局,初始化库.

int64_t TableFrameSink::m_llStorageControl = 0;
int64_t TableFrameSink::m_llStorageLowerLimit = 0;
int64_t TableFrameSink::m_llStorageHighLimit = 0;
int64_t TableFrameSink::m_llStorageAverLine = 0;
int64_t TableFrameSink::m_llGap = 0;

//库存加上
TableFrameSink::TableFrameSink()
{

    stockWeak = 0.0;
    m_bIsClearUser = false; 
    m_nReadCount = 0;
    m_MarqueeScore = 1000;
    m_bIsIncludeAndroid = 0;


    m_vOpenResultListCount.clear();
    for (int i = 0;i < BET_ITEM_COUNT;i++)
    {
        vector<shared_ptr<OpenResultInfo>> vResultList;
        m_vOpenResultListCount[i] = vResultList;
    }

    m_vResultRecored.clear();
    int32_t index = 0;
    for (int i = 0; i < OPEN_RESULT_LIST_COUNT; i++)
    {
        index = rand()%7;
        if (m_vResultRecored.size() >= 20 )
            m_vResultRecored.erase(m_vResultRecored.begin());
        m_vResultRecored.push_back(index);

        vector<shared_ptr<OpenResultInfo>> vResultList;
        for (int i = 0;i < BET_ITEM_COUNT;++i)
        {
            shared_ptr<OpenResultInfo> iResultIndex(new OpenResultInfo());
            shared_ptr<OpenResultInfo> iResultIndex_ls(new OpenResultInfo());
            vResultList = m_vOpenResultListCount[i];
            if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
            {
                vResultList.erase(vResultList.begin());
            }
            int resultCount = vResultList.size();
            iResultIndex->clear();
            if (resultCount > 0)
            {
                iResultIndex_ls = vResultList[resultCount - 1];
                iResultIndex->notOpenCount = (i == index) ? 0 : (iResultIndex_ls->notOpenCount + 1);
            }
            else
            {
                iResultIndex->notOpenCount = (i == index) ? 0 : (iResultIndex->notOpenCount + 1);
            }
            iResultIndex->iResultId = index;

            vResultList.push_back(iResultIndex);
            m_vOpenResultListCount[i] = vResultList;

        }

    }

    m_vOpenResultListCount_last.clear();
    for (auto &it : m_vOpenResultListCount)
    {
        m_vOpenResultListCount_last[it.first] = it.second;
    }
    m_lastRetId = -1;

    m_vPlayerList.clear(); 
    m_pPlayerInfoList.clear(); 
    m_bIsOpen = false; 
    m_pITableFrame = nullptr;
    m_EapseTime = (int32_t)time(nullptr);
    m_iGameState = GAME_STATUS_INIT;
    m_strRoundId = "";
    
    m_pBetArea.reset(new BetArea());
    
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        m_pPlayerList[i].clear(); 
        m_nPlayerScore[i] = 0;
    }
    m_iMul = 20;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        m_nMuilt[i] = 0;
        m_lOpenResultIndexCount[i] = 0;
    }

    // m_LoopThread.startLoop();

    m_bIsContinue = true;
    m_fTMJettonBroadCard = 0.5f;
    m_lOpenResultCount = 0;
    m_bTest = 0;
    m_nTestTimes = 0;
    m_bWritelog = false;
    m_nTestType = 0;
    m_bTestAgain = false;
    m_dControlkill=0;
    m_lLimitPoints=0;

    for(int i=0;i<MAX_PLAYER;i++)
    {
        m_currentchips[i][0]=100;
        m_currentchips[i][1]=500;
        m_currentchips[i][2]=1000;
        m_currentchips[i][3]=5000;
        m_currentchips[i][4]=10000;
        m_currentchips[i][5]=50000;
    }
    m_fResetPlayerTime = 5.0f;
}
TableFrameSink::~TableFrameSink()
{
    m_pITableFrame = nullptr;
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    m_LoopThread->getLoop()->cancel(m_TimerIdTest);
}

void TableFrameSink::OnGameStart()
{
    LOG(WARNING) << "On Game Start " << m_pGameRoomInfo->roomName << " " << m_pGameRoomInfo->roomId;
}

void TableFrameSink::OnEventGameEnd(int32_t chairid, uint8_t nEndTag)
{
    LOG_INFO << "On Event On Game End ";
}
bool TableFrameSink::OnEventGameScene(uint32_t chairid, bool bisLookon)
{
    LOG_INFO << "On Event On Event Game Scene ";
}

bool TableFrameSink::OnEventGameConclude(uint32_t chairid, uint8_t nEndTag)
{
    return false;
}

bool TableFrameSink::CanJoinTable(shared_ptr<CServerUserItem> &pUser)
{
    // 分数不够不能进
    // if(pUser->GetUserScore() < m_pGameRoomInfo->floorScore ){
    //     LOG(WARNING)<< "UserScore "<< pUser->GetUserScore() << " floorScore " << m_pGameRoomInfo->floorScore;

    //     return false;
    // }
    return true;
}

bool TableFrameSink::CanLeftTable(int64_t userId)
{
    LOG(WARNING) << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;

    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if (!pIServerUserItem)
    {
        LOG(WARNING) << " pIServerUserItem==nullptr Left UserId:" << userId;
        return true;
    }
    else
    {
        int32_t chairId = pIServerUserItem->GetChairId();
        strPlalyers player = m_pPlayerList[chairId];

        int64_t totalxz = 0;
        uint32_t status = m_pITableFrame->GetGameStatus();
        if (BenCiBaoMa::SUB_S_GameStart == status)
        {
            if (player.Isplaying)
                totalxz = player.dCurrentPlayerJetton;
        }

        if (totalxz > 0)
            canLeft = false;
        else
        {
            m_nPlayerScore[chairId] = 0;
        }
    }
    string strtttt = (canLeft?"可以":"不可以") ;
    LOG(WARNING) << "****玩家"<< strtttt <<"离开桌子***[" << pIServerUserItem->GetChairId() << "] " << pIServerUserItem->GetUserId() <<" "<< pIServerUserItem->GetAccount();

    return canLeft;
}

bool TableFrameSink::OnUserEnter(int64_t userid, bool islookon)
{ 
    shared_ptr<CServerUserItem> m_pPlayer = m_pITableFrame->GetUserItemByUserId(userid);
    if (!m_pPlayer)
    {
        LOG(ERROR) << "**********Get UserItem By UserId error**********" << userid;
        return false;
    }

    if(m_pPlayer->IsAndroidUser()){
        LOG(WARNING) << "*********机器人进来了**********" << userid;
    }

    int32_t wCharid = m_pPlayer->GetChairId();

    if (wCharid < 0 || wCharid >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a WrongchairID"  << "   " << wCharid;
        return false;
    }    
    // 
    m_pPlayer->setUserSit();

    LOG(WARNING) << "********** User Enter **********[" << wCharid << "] " << userid <<" "<< m_pPlayer->GetAccount();
    openSLog("===User Enter=== wCharid=%d,userid=%d,UserScore=%d;",wCharid,userid,m_pPlayer->GetUserScore());
    m_pPlayerList[wCharid].Isplaying = true;
    m_pPlayerList[wCharid].wCharid = wCharid;
    m_pPlayerList[wCharid].wUserid = userid;
    m_pPlayerList[wCharid].wheadid = m_pPlayer->GetHeaderId();
    m_nPlayerScore[wCharid] = m_pPlayer->GetUserScore();

    for(int i=0;i<6;i++)
    {
       if(m_pPlayer->GetUserScore()>= m_userScoreLevel[0]&&
               m_pPlayer->GetUserScore()< m_userScoreLevel[1])
       {
           m_currentchips[wCharid][i]=m_userChips[0][i];
       }
       else if(m_pPlayer->GetUserScore()>= m_userScoreLevel[1]&&
               m_pPlayer->GetUserScore()< m_userScoreLevel[2])
       {
           m_currentchips[wCharid][i]=m_userChips[1][i];
       }
       else if(m_pPlayer->GetUserScore()>= m_userScoreLevel[2]&&
               m_pPlayer->GetUserScore()< m_userScoreLevel[3])
       {
           m_currentchips[wCharid][i]=m_userChips[2][i];
       }
       else
       {
           m_currentchips[wCharid][i]=m_userChips[3][i];
       }
    }
    // 清楚这个座位的押分记录
	if (m_pPlayerList[wCharid].dCurrentPlayerJetton==0)
	{
		m_pPlayerList[wCharid].clearJetton();
	}

    // 启动游戏
    if(!m_bIsOpen)
    { 
        // mpBankerManager->UpdateBankerData(); //一开始设置系统庄家
        m_iShenSuanZiChairId = wCharid;
        m_iCurrBankerId = -1;  
        m_iShenSuanZiUserId = m_pPlayer->GetUserId();
        m_bIsOpen = true;
        m_EapseTime = (int32_t)time(nullptr);

        m_startTime = chrono::system_clock::now();
        m_dwReadyTime = (uint32_t)time(nullptr);
        m_strRoundId = m_pITableFrame->GetNewRoundId();
        m_replay.clear();
        m_replay.cellscore = m_pGameRoomInfo->floorScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
        m_replay.gameinfoid = m_strRoundId;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        // 修改miGameSceneState
        m_iGameState = BenCiBaoMa::SUB_S_GameStart; 
        // 启动定时器
        GamefirstInGame();

        // if (m_bTest>0)
        // {
        //     m_LoopThread->getLoop()->cancel(m_TimerIdTest);
        //     m_TimerIdTest=m_LoopThread->getLoop()->runEvery(0.1, boost::bind(&TableFrameSink::TestGameMessage, this,wCharid,m_nTestType));
        //     openSLog("本次测试%d开始;",m_nTestTimes);
        //     openSLog("开始库存:High Limit:%d,Lower Limit:%d,,Aver Line:%d",m_llStorageHighLimit,m_llStorageLowerLimit,m_llStorageControl);
        //     m_bTestAgain = true;
        // } 
    }

    // 发送场景消息
    switch (m_iGameState)
    {
    case BenCiBaoMa::SUB_S_GameStart:
    {
        SendGameSceneStart(userid, false);
        break;
    }
    case BenCiBaoMa::SUB_S_GameEnd:
    {
        SendGameSceneEnd(userid, false);
        break;
    }
    default:
    {
        LOG(ERROR) << "*********发送场景消息 状态错误**********" << m_iGameState;
    }
        break;
    }

    LOG(WARNING) << "********** User Enter 2 **********" << m_iGameState;

    return true;
}
bool TableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    return true;
}
bool TableFrameSink::OnUserLeft(int64_t userid, bool islookon)
{ 
    shared_ptr<CServerUserItem> user = m_pITableFrame->GetUserItemByUserId(userid);
    if (!user)
        return false;

    int32_t chairId = user->GetChairId();

    if (chairId < 0 || chairId >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a Wrong chair ID" << "   " << (int)chairId;
    }

    if (m_bIsClearUser)
    {
        m_pPlayerList[chairId].Allclear();
        m_pITableFrame->ClearTableUser(chairId, true, true);
    }
    else if (!user->IsAndroidUser() && m_pPlayerList[chairId].dCurrentPlayerJetton <= 0)
    {
        m_pPlayerList[chairId].Allclear();
        m_pITableFrame->ClearTableUser(chairId, true, true);
    }

    LOG(WARNING) << "**********玩家离开 3**********[" << chairId << "] " << userid;

    return true;
}

bool TableFrameSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{
    assert(nullptr != pTableFrame);
    if (pTableFrame == nullptr)
        return false;

    m_pITableFrame = pTableFrame;
    m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();

    // 获取EventLoopThread
    m_LoopThread = m_pITableFrame->GetLoopThread();

    m_cbJettonMultiple.clear();
    int jsize = m_pGameRoomInfo->jettons.size();
    for (int i = 0; i < jsize; i++)
    {
        m_cbJettonMultiple.push_back(m_pGameRoomInfo->jettons[i]);
        m_pBetArea->SetBetItem(i,m_pGameRoomInfo->jettons[i]);
        LOG(WARNING) << "jettons " << i <<" "<< m_pGameRoomInfo->jettons[i] <<" jsize:" << jsize;
    }
    m_replay.clear(); 
    m_replay.cellscore = m_pGameRoomInfo->floorScore; 
    m_replay.roomname = m_pGameRoomInfo->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    ReadStorageInfo();
    //初始化游戏数据
    // m_wGameStatus = GAME_STATUS_INIT;

    LOG(WARNING) << "Set Table Frame " << m_pGameRoomInfo->roomName << " " << m_pGameRoomInfo->roomId << " " << m_pGameRoomInfo->tableCount;

    OnGameStart(); // m_pITableFrame->onStartGame();

    //Algorithmc::Instence()->InitAlgor(m_pITableFrame->GetMaxPlayerCount(), m_pGameRoomInfo->tableCount, m_pGameRoomInfo->roomId);

    for (int i = 0; i < MAX_PLAYER; i++)
        m_pPlayerList[i].clear();

    // 读配置
    ReadInI();
    srand((int)time(nullptr));
    return true;
}

void TableFrameSink::GamefirstInGame()
{
    // 启动定时器
    m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(m_betCountDown - 3, boost::bind(&TableFrameSink::GameTimerEnd, this));

    m_timetickAll = m_betCountDown - 3;
    m_timetick = 0;
    // 测试
    // m_TimerIdTest = m_LoopThread->getLoop()->runEvery(1, [&]() {
    //     // LOG(WARNING) << "Tick---> " << (m_timetickAll - (m_timetick++)) ;
    // });

    LOG(WARNING) << "--- 启动桌子---" << __FUNCTION__ << " " << m_iGameState;
}
void TableFrameSink::GameRefreshData()
{
    shared_ptr<IServerUserItem> pServerUserItem;
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        if (!m_pPlayerList[i].Isplaying)
        {
            continue;
        }
        pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerList[i].wCharid);
        if (!pServerUserItem)
            continue;
        m_pPlayerList[i].RefreshGameRusult(true);
    }
}
void TableFrameSink::GameTimerStart()
{
    LOG(WARNING) << "--- 开始游戏 1---" << __FUNCTION__;

    // string Huode = "发送房间消息,大保时捷";
    // int sgType = SMT_GLOBAL | SMT_SCROLL;
    // m_pITableFrame->SendGameMessage(0, (uint8_t *)Huode.c_str(), sgType, 100000);

    // LOG(ERROR) << "---- 发送房间消息 ----" << __FUNCTION__;

    //连续5局未下注踢出房间
    //CheckKickOutUser();

    // 设置游戏状态
    m_iGameState = BenCiBaoMa::SUB_S_GameStart;
    m_pITableFrame->SetGameStatus(BenCiBaoMa::SUB_S_GameStart);
    m_timetick = 0;
    m_timetickAll = m_betCountDown;

    m_startTime = chrono::system_clock::now();
    m_dwReadyTime = (uint32_t)time(nullptr);
    m_strRoundId = m_pITableFrame->GetNewRoundId();

    m_replay.clear();
    m_replay.cellscore = m_pGameRoomInfo->floorScore;
    m_replay.roomname = m_pGameRoomInfo->roomName;
    m_replay.gameinfoid = m_strRoundId;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    // muduo::MutexLockGuard lock(m_mutexLock); 
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        // 清空玩家数据
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
        if (!user)
        {
            m_pPlayerList[i].Allclear();
            continue;
        }

        LOG(WARNING) << "当前玩家分 "<< i << "  " <<  user->GetUserScore();


        // 踢出离线玩家
        if (user->GetUserStatus() == sOffline)
        {
            m_pITableFrame->ClearTableUser(i, true, true);
            m_pPlayerList[i].Allclear();
        }
    }
    /*****************打印数据 start******************/
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> it = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == it)
            continue;
        if (!m_pPlayerList[i].Isplaying)
        {
            continue;
        }
        if (it->IsAndroidUser())
        {
            continue;
        }

        LOG(WARNING) << "user=" << it->GetUserId() << " bet1=" << m_pPlayerList[i].dAreaJetton[0] << " bet2=" << m_pPlayerList[i].dAreaJetton[1] << " bet3=" << m_pPlayerList[i].dAreaJetton[2]
                     << " bet4=" << m_pPlayerList[i].dAreaJetton[3] << " bet5=" << m_pPlayerList[i].dAreaJetton[5] << " bet6=" << m_pPlayerList[i].dAreaJetton[5] << " bet7=" << m_pPlayerList[i].dAreaJetton[6]
                     << " bet8=" << m_pPlayerList[i].dAreaJetton[7];
    }

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> it = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == it)
            continue;
        if (!m_pPlayerList[i].Isplaying)
        {
            continue;
        }

        // LOG(WARNING) << "user=" << it->GetUserId() << " playereWin=" << m_pPlayerList[i].m_winAfterTax;
    }
    /*****************打印数据 end*******************/

    // 记录当前时间
    m_EapseTime = time(nullptr);

    // 跑马灯显示
    BulletinBoard();

    //clear Table
    ResetTableDate();
    // 服务器没启动踢出玩家
    if (m_pGameRoomInfo->serverStatus !=SERVER_RUNNING)
    {
        for (int i = 0; i < MAX_PLAYER; i++)
        {
            shared_ptr<CServerUserItem> it = this->m_pITableFrame->GetTableUserItem(i);
            if (nullptr == it)
                continue;
            it->setOffline(); 
            m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SERVERSTOP);
            m_pPlayerList[i].Allclear();
        }
        m_bIsOpen = false;
        m_bIsContinue = true;
        m_LoopThread->getLoop()->cancel(m_TimerIdTest);
        m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
        m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
        LOG(WARNING) << "m_pGameRoomInfo->serverStatus !=SERVER_RUNNING === " << m_pGameRoomInfo->serverStatus << "SERVER_RUNNING="<<SERVER_RUNNING;
        return;
    }


    //更新开奖结果
    m_vOpenResultListCount_last.clear();
    for (auto &it : m_vOpenResultListCount)
    {
        m_vOpenResultListCount_last[it.first] = it.second;
    }
    PlayerRichSorting();


    // 向客户端发送游戏开始命令
    NormalGameStart();
    //开始启动一段时间内的相对自己的其他所有玩家的押注总额,发送给客户端
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    m_TimerOtherBetJetton = m_LoopThread->getLoop()->runEvery(m_fTMJettonBroadCard,boost::bind(&TableFrameSink::OnTimerSendOtherBetJetton, this));

    // 清除玩家标志
    m_bIsClearUser = false;

    // 服务器状态判断
    if (m_bIsContinue)
    {
        LOG(WARNING) << "--- 启动定时器 开始押分 CallBack GameTimerEnd---";
        m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
        // if (m_bTest>0)
        //     m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(0.125, boost::bind(&TableFrameSink::GameTimerEnd, this));
        // else
            m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(m_betCountDown, boost::bind(&TableFrameSink::GameTimerEnd, this));
    }
    else
    {
        for (int i = 0; i < MAX_PLAYER; i++)
        {
            shared_ptr<CServerUserItem> it = this->m_pITableFrame->GetTableUserItem(i);
            if (!it)
            {
                m_pPlayerList[i].Allclear();
                continue;
            }

            if (!m_pPlayerList[i].Isplaying)
                continue;

            it->setOffline();

            // 对不起,当前游戏服务器正在维护,请稍后重试.
            m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SERVERSTOP);
        }
    }
    m_bIsContinue = false;
}
void TableFrameSink::GameTimerEnd()
{
    LOG(WARNING) << "--- 游戏结束---" << __FUNCTION__; 
    // 更新状态
    m_iGameState = BenCiBaoMa::SUB_S_GameEnd;
    m_pITableFrame->SetGameStatus(BenCiBaoMa::SUB_S_GameEnd);
    m_timetick = 0;
    m_timetickAll = m_endCountDown;
    m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    OnTimerSendOtherBetJetton();


    // muduo::MutexLockGuard lock(m_mutexLock);
    m_EapseTime = (int32_t)time(nullptr);

    // 结算计算
    NormalCalculate();
    m_bIsClearUser = true;
    // 跑马灯显示
    BulletinBoard();
    m_bIsContinue = m_pITableFrame->ConcludeGame(m_iGameState);
    LOG(WARNING) << "--- 游戏结束 m_bIsContinue is True?---" << m_bIsContinue;
    LOG(WARNING) << "--- 启动结算定时器---"; 
    // if (m_bTest>0)
    //     m_TimerIdStart = m_LoopThread->getLoop()->runAfter(0.125,CALLBACK_0(TableFrameSink::GameTimerStart, this));
    // else
        m_TimerIdStart = m_LoopThread->getLoop()->runAfter(m_endCountDown,CALLBACK_0(TableFrameSink::GameTimerStart, this));

        m_TimerRefresh = m_LoopThread->getLoop()->runAfter(m_fResetPlayerTime,CALLBACK_0(TableFrameSink::GameRefreshData, this));
    // 更新配置的频率
    // if (m_bTest==0)
    {
        m_nReadCount++;
        if (m_nReadCount >= 5)
        {
            ReadInI();
            m_nReadCount = 0;
        }
    }
    
    //统计本局的库存累加值
    int64_t res = 0.0;
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(i);
        if (!player || !m_pPlayerList[i].Isplaying)
            continue;
        if (player->IsAndroidUser())
            continue;
        res -= m_pPlayerList[i].m_winAfterTax;        
    }
    LOG(INFO)<<"update Num----------------- "<<res;
    if( res > 0)
    {
        res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0; //- m_pITableFrame->CalculateRevenue(res);
    }
    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update before----------------- "<<ceshi.lEndStorage;
    LOG(INFO)<<"update Num----------------- "<<res;
    m_pITableFrame->UpdateStorageScore(res);
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update after ----------------- "<<ceshi.lEndStorage;    
    openSLog("库存值:High Limit:%d,Lower Limit:%d,Stock:%d",m_llStorageHighLimit,m_llStorageLowerLimit,m_llStorageControl);
    openSLog("本局游戏结束;\n");

    ReadStorageInfo();
        
}
void TableFrameSink::CheckKickOutUser()
{
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> pUserItem = m_pITableFrame->GetTableUserItem(i);
        if (!pUserItem || !m_pPlayerList[i].Isplaying)
            continue;
        if (pUserItem->IsAndroidUser())
            continue;      
        if (m_pPlayerList[i].NotBetJettonCount>=5)
        {
            pUserItem->SetUserStatus(sGetoutAtplaying);
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);
            m_pITableFrame->ClearTableUser(i,true,true);
            m_pPlayerList[i].Allclear();
            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<m_pPlayerList[i].wCharid<<" userId="<<m_pPlayerList[i].wUserid;
        }       
    }
}

void TableFrameSink::ReadStorageInfo()
{
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);

    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;
    m_llGap = (m_llStorageHighLimit-m_llStorageLowerLimit) >> 1;
    m_llStorageAverLine = (m_llStorageHighLimit+m_llStorageLowerLimit) >> 1;
    if(m_llGap < 0)
    {
        LOG(ERROR) << "Error Limit...";
    }else{
        LOG(INFO) << "High Limit" << m_llStorageHighLimit << ",Lower Limit:" << m_llStorageLowerLimit << ",Aver Line"<<m_llStorageAverLine
                  << ", Gap" << m_llGap << ", Stock:" << m_llStorageControl;
    }

    LOG(ERROR)<<    "llStorageControl"  << "=" <<  m_llStorageControl;
    LOG(ERROR)<<    "llStorageLowerLimit"   << "="  <<    m_llStorageLowerLimit;
    LOG(ERROR)<<    "llStorageHighLimit"    <<  "="   << m_llStorageHighLimit;
}
//测试游戏数据
void TableFrameSink::TestGameMessage(uint32_t wChairID,int type)
{
    // 游戏状态判断
    if (BenCiBaoMa::SUB_S_GameStart != m_iGameState)
    {
       return;
    }
    int betTimes = 0;
    // 测试1-放分; 测试2-收分; 测试3-正常; 测试4 - 保险1;  测试4 - 保险2;
    int betAllScore[6] = {0,5000,100,100,900,1500};
    int tmpbetAllScore = betAllScore[m_nTestType]*COIN_RATE;
    //每次下注到一个5倍的区域   
    int32_t bJettonArea = RandomInt(0, 3);
    do
    {                
        if (m_nTestType<4)
        {
            int randn = 0;
            for (size_t i = 0; i < BET_ITEM_COUNT; i++)
            {
                randn = RandomInt(1, 100);
                if (randn <= 30)
                {
                    bJettonArea = i;
                    break;
                }
            }
            if(bJettonArea = -1)
                bJettonArea = RandomInt(0, BET_ITEM_COUNT-1);
        }
        //随机可下注金额
        int64_t JettonScore=0,canGold = 0,TotalWeightGold = 0;
        int randn = 0;
        for (int i = 4; i >= 0; --i)
        {
            if(m_pGameRoomInfo->jettons[i]>tmpbetAllScore) 
                continue;
            randn = RandomInt(0, i);
            canGold = m_pGameRoomInfo->jettons[randn];
            tmpbetAllScore -= canGold;
            break; 
        }
        //随机下注筹码
        JettonScore = canGold;

        LOG(WARNING) << "----随机下注金额---" << JettonScore << " " << bJettonArea ;

        BenCiBaoMa::CMD_C_PlaceJet mPlaceJet;
        mPlaceJet.set_cbjettonarea(bJettonArea);
        mPlaceJet.set_ljettonscore(JettonScore);

        string content = mPlaceJet.SerializeAsString();
        OnGameMessage(wChairID, BenCiBaoMa::SUB_C_USER_JETTON, (uint8_t *)content.data(), content.size());
    }
    while(tmpbetAllScore>0);
    
    

    GameTimerEnd();
}

bool TableFrameSink::OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{ 
    if (chairid < 0 || chairid >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a WrongchairID"  << "   " << (int)chairid;
        return false;
    }
    switch (subid)
    {
        case BenCiBaoMa::SUB_C_USER_JETTON: //玩家押注
        {
            GamePlayerJetton(chairid, subid, data, datasize);
            break;
        }
        case BenCiBaoMa::SUB_C_USER_ASKLIST:
        {
            SendPlayerList(chairid, subid, data, datasize); //排行榜
            break;
        }
        case BenCiBaoMa::SUB_C_USER_REPEAT_JETTON:
        {
            Rejetton(chairid, subid, data, datasize); //续押
            break;
        }
        case BenCiBaoMa::SUB_C_QUERY_PLAYERLIST: //查询玩家列表
        {
            if(BenCiBaoMa::SUB_S_GameStart == m_iGameState)
            {
                GamePlayerQueryPlayerList(chairid, subid, data, datasize,true); //wan jia lie biao
            }
            else
            {
                GamePlayerQueryPlayerList(chairid, subid, data, datasize,false); //wan jia lie biao
            }

            break;
        }
        case BenCiBaoMa::SUB_C_QUERY_CUR_STATE: //获取当前状态
        {
            QueryCurState(chairid, subid, data, datasize);
            break;
        }
        case BenCiBaoMa::SUB_C_QUERY_GAMERECORD:
        {
            QueryGameRecord(chairid, subid, data, datasize);
            break;
        }
    }
    return true;
}

// 查询当前游戏状态
void TableFrameSink::QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{ 
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(chairid);
    if (!player)
        return;
    // if (m_bTest>0)
    // {
    //     return;
    // }
    BenCiBaoMa::CMD_C_CurState curStateReq;
    curStateReq.ParseFromArray(data, datasize);

    int32_t userid = curStateReq.dwuserid();

    int32_t elsTimeTest = ((int32_t)time(nullptr) - m_EapseTime);
    LOG(WARNING) << "**********查询当前游戏状态**********" << userid <<" "<<elsTimeTest;

    BenCiBaoMa::CMD_S_CurState curStateRsp;

    int32_t iEapseTime = m_betCountDown;

    if(BenCiBaoMa::SUB_S_GameStart != m_iGameState){
        iEapseTime = m_endCountDown;
    }
    curStateRsp.set_ncurretidx(m_iStopIdx); 
    curStateRsp.set_ncurstate((int)(BenCiBaoMa::SUB_S_GameStart != m_iGameState)); //当前状态
    curStateRsp.set_cbtimeleave(iEapseTime - elsTimeTest);
    LOG(WARNING) << "****" <<iEapseTime;

    int betCount = 0;
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> usr = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == usr )
            continue; 

        // 增加玩家押分筹码值
        if (m_pPlayerList[i].dCurrentPlayerJetton > 0)
        { 
            for (tagBetInfo betVal : m_pPlayerList[i].JettonValues)
            {
                BenCiBaoMa::RepeatInfo *RepeatInfo = curStateRsp.add_trepeat();
                RepeatInfo->set_luserid(usr->GetUserId());
                RepeatInfo->set_tljettonscore(betVal.betScore);
                RepeatInfo->set_tjetidx(betVal.betIdx);
            }
        }

        if (i != chairid || !m_pPlayerList[i].Isplaying)
            continue; 

        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            betCount += m_pPlayerList[i].dAreaJetton[j];
            curStateRsp.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[j]);
            curStateRsp.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
            curStateRsp.add_mutical(m_nMuilt[j]);
        } 
    }
    curStateRsp.set_nlastretid(m_lastRetId);

    //记录
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--){
        curStateRsp.add_cartype(m_vResultRecored[i]);
        // LOG(WARNING) << "Recored =>" << i << " "<< m_vResultRecored[i];
    }

    LOG(ERROR) << "查询当前游戏状态 =>UserScore=" << player->GetUserScore() << ",betCount= "<< betCount;
    if(BenCiBaoMa::SUB_S_GameStart == m_iGameState)
    {
        curStateRsp.set_luserscore(player->GetUserScore() );
    }
    else
    {
        curStateRsp.set_luserscore(player->GetUserScore()); 
    }
    curStateRsp.set_userwinscore(m_pPlayerList[chairid].RealMoney);

    std::string sendData = curStateRsp.SerializeAsString();
    // if (m_bTest==0)
        m_pITableFrame->SendTableData(chairid, BenCiBaoMa::SUB_S_QUERY_CUR_STATE, (uint8_t *)sendData.c_str(), sendData.size());
}

/*
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> it = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == it)
            continue;
        if (!m_pPlayerList[i].Isplaying)
        {
            continue;
        }

        LOG(WARNING) << "user=" << it->GetUserId() << " playereWin=" << m_pPlayerList[i].m_winAfterTax;
    }
*/

// 写玩家分
void TableFrameSink::WriteUserScore()
{ 
    vector<tagScoreInfo> infoVec;
    infoVec.clear();

    string strRet = str(boost::format("%d-%d") % m_iResIdx % m_iMul);
    LOG(WARNING) << "写玩家分 =>" << strRet; 
    m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime,strRet,-1,opShowCard,-1,-1);

    bool isNeedRecord = false;

	int64_t allBetScore = 0;
	int64_t userid = 0;
	m_replay.detailsData = "";
    for (int idx = 0; idx < MAX_PLAYER; idx++)
    {
        shared_ptr<CServerUserItem> User = m_pITableFrame->GetTableUserItem(idx);
        if (!User ) continue;  
        if (User->IsAndroidUser()) continue;  
        if (!m_pPlayerList[idx].Isplaying ) continue;  
        if ( m_pPlayerList[idx].dCurrentPlayerJetton == 0) continue;

		allBetScore = 0;
		userid = User->GetUserId();
        tagScoreInfo scoreInfo; 
        for(int betIdx = 0;betIdx < BET_ITEM_COUNT; betIdx++){  

            // 结算时记录
            int64_t addScore = m_pPlayerList[idx].dAreaWin[betIdx];
            int64_t betScore = m_pPlayerList[idx].dAreaJetton[betIdx]; 
            // 押分情况
            scoreInfo.cellScore.push_back(betScore);
            //牌型
            if(betScore > 0){
                m_replay.addResult(idx, betIdx, betScore, addScore, strRet, false);
            }
			allBetScore += betScore;
        }
		if (allBetScore>0)
		{
			//对局记录详情
            SetGameRecordDetail(userid, idx, User->GetUserScore(), m_pPlayerList[idx].RealMoney);
		}
		
        LOG(WARNING) << "结算时记录 =>";

        scoreInfo.chairId = idx;
        scoreInfo.addScore = m_pPlayerList[idx].m_winAfterTax;
        scoreInfo.revenue = m_pPlayerList[idx].m_userTax;
        scoreInfo.betScore = m_pPlayerList[idx].dCurrentPlayerJetton;
        scoreInfo.rWinScore = m_pPlayerList[idx].dCurrentPlayerJetton; //
        //牌型
        scoreInfo.cardValue = strRet;
        scoreInfo.startTime = m_startTime;
        infoVec.push_back(scoreInfo);

        isNeedRecord = true;
    }
	
    //保存对局结果
    if(isNeedRecord)
        m_pITableFrame->SaveReplay(m_replay);

    //写分
    m_pITableFrame->WriteUserScore(&infoVec[0], infoVec.size(), m_strRoundId);
}
/*
     * 位置 2-7-11-16-21 是雷克萨斯，0
     * 位置 3-8-13-17-22 是大众，1
     * 位置 4-9-14-19-23 是奔驰，2
     * 位置 1-5-10-15-20 是宝马，3
     * 位置 18 是保时捷，4
     * 位置 6 是玛莎拉蒂，5
     * 位置 0 是兰博基尼，6
     * 位置 12 是法拉利，7
*/
void TableFrameSink::ReadInI()
{
    CINIHelp *pini = new CINIHelp("conf/bcbm_config.ini");




    int32_t odds[BET_ITEM_COUNT] = {5,5,5,5,10,20,30,40};
    int32_t weight[BET_ITEM_COUNT] = {1984,1984,1984,1984,992,495,330,247};
    int32_t nAreaMaxJetton[BET_ITEM_COUNT];
    int32_t retWeight[BET_ITEM_COUNT];
    int32_t nMuilt[BET_ITEM_COUNT];// 倍率表
    memcpy(nAreaMaxJetton, m_nAreaMaxJetton, sizeof(nAreaMaxJetton));    
    memcpy(retWeight, m_retWeight, sizeof(retWeight));
    memcpy(nMuilt, m_nMuilt, sizeof(nMuilt));

    char buf[32] = { 0 };
    for(int i = 0;i<BET_ITEM_COUNT;i++){
        // 赔率
        sprintf(buf, "AREA_%d",i);
        m_nMuilt[i] = pini->GetLong("GAME_MULTIPLIER", buf, odds[i]);
        if(m_nMuilt[i]==0)
            m_nMuilt[i] = nMuilt[i];
        // 押分区最大押分值
        sprintf(buf, "MaxJetton%d",i);
        m_nAreaMaxJetton[i] = pini->GetLong("GAME_AREA_MAX_JETTON", buf, 10000);
        if(m_nAreaMaxJetton[i]==0)
            m_nAreaMaxJetton[i] = nAreaMaxJetton[i];
        // 开奖权重 
        sprintf(buf, "Ret%d",i);
        m_retWeight[i] = pini->GetLong("GAME_RET", buf, weight[i]);
        if(m_retWeight[i]==0)
            m_retWeight[i] = retWeight[i];

        LOG(WARNING) << "--- ReadInI---" << m_nMuilt[i] <<" "<< m_nAreaMaxJetton[i] <<" "<<m_retWeight[i];
    }

    m_dControlkill = pini->GetFloat("GAME_CONFIG", "Controlkill", 0.3);

    m_lLimitPoints = pini->GetLong("GAME_CONFIG", "LimitPoint", 500000);
    // 倒计时时间
    m_betCountDown = pini->GetLong("GAME_STATE_TIME", "TM_GAME_START", 15);
    m_endCountDown = pini->GetLong("GAME_STATE_TIME", "TM_GAME_END", 21);
    // 控制概率
    m_ctrlRate         =(pini->GetLong("GAME_STATE_TIME", "CTRL_RATE", 50) * 1.0)/100.0;
    m_stockPercentRate =(pini->GetLong("GAME_STATE_TIME", "STOCK_CTRL_RATE", 50) * 1.0)/100.0;

    m_bIsIncludeAndroid = pini->GetLong("GAME_CONFIG", "USER_ANDROID_DATA_TEST_AISC", 0);
    m_MarqueeScore = pini->GetLong("GAME_CONFIG", "PAOMADENG_CONDITION_SET", 1000);
    m_fTaxRate = pini->GetFloat("GAME_CONFIG", "REVENUE", 0.025);
    m_fTMJettonBroadCard = pini->GetFloat("GAME_CONFIG", "TM_JETTON_BROADCAST", 0.5);

    stockWeak = pini->GetFloat("GAME_CONFIG", "STOCK_WEAK", 1.0);
    // m_bTest = pini->GetLong("GAME_CONFIG", "TestGame", 0);
    // m_nTestTimes = pini->GetLong("GAME_CONFIG", "TestGameTimes", 0);
    // m_nTestType = pini->GetLong("GAME_CONFIG", "nTestType", 3);
    // 是否写日记
    m_bWritelog = false;
    if (pini->GetLong("GAME_CONFIG", "nWritelog", 0) != 0)
    {
        m_bWritelog = true;
    }
    // TCHAR szPath[MAX_PATH] = TEXT("");
    // TCHAR szConfigFileName[MAX_PATH] = TEXT("");
    // TCHAR OutBuf[255] = TEXT("");
    // CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
    // myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/bcbm_config.ini"), szPath);
    // TCHAR INI_SECTION_CONFIG[30] = TEXT("");
    // myprintf(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG), _T("ANDROID_NEWCONFIG_%d"), m_pTableFrame->GetGameRoomInfo()->roomId);
    // CINIHelp::WritePrivateProfileString(_T"GAME_CONFIG", _T"TestGame","0" ,szConfigFileName);
    // CINIHelp::WritePrivateProfileString(_T"GAME_CONFIG", _T"TestGameTimes", "1",szConfigFileName);
    m_useIntelligentChips=pini->GetLong("CHIP_CONFIGURATION", "useintelligentchips", 1) ;

    if(m_useIntelligentChips)
    {
        for(int i=0;i<6;i++)
        {
            char buf[100];
            sprintf(buf, "chipgear1_%d",i+1);
            m_userChips[0][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;

            sprintf(buf, "chipgear2_%d",i+1);
            m_userChips[1][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;

            sprintf(buf, "chipgear3_%d",i+1);
            m_userChips[2][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;

            sprintf(buf, "chipgear4_%d",i+1);
            m_userChips[3][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;
        }

        for(int i=0;i<4;i++)
        {
            char buf[100];
            sprintf(buf, "scorelevel%d",i+1);
            m_userScoreLevel[i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;
        }
    }
    else
    {
        for(int i=0;i<6;i++)
        {
            char buf[100];
            sprintf(buf, "chipgear4_%d",i+1);
            m_userChips[0][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;
            m_userChips[1][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;
            m_userChips[2][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;
            m_userChips[3][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 10) ;
        }
    }


    delete pini;
}

// 发送开始游戏命令
void TableFrameSink::NormalGameStart()
{
    LOG(WARNING) << "--- 开始游戏 2---" << __FUNCTION__;
    openSLog("本局游戏开始;");
    // if (m_bTest>0)
    // {
    //     return;
    // }

    int64_t xzScore = 0;
    int64_t nowScore = 0;
    shared_ptr<IServerUserItem> pServerUserItem;

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> usr = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == usr || !m_pPlayerList[i].Isplaying)
            continue;
        
        BenCiBaoMa::CMD_S_GameStart gameStart;
        gameStart.set_cbplacetime(m_betCountDown);
        gameStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            gameStart.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[j]);
            gameStart.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
            gameStart.add_mutical(m_nMuilt[j]);
        }
        if (!usr->IsAndroidUser())
        {
            LOG(WARNING) << "--------------------GetUserScore()==" <<usr->GetUserId() <<" "<< usr->GetUserScore();
        }
        // 
        int32_t index = 0;
        for (auto &uInfoItem : m_pPlayerInfoList)
        {
            xzScore = 0;
            pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem.wCharid);
            if (!pServerUserItem)
                nowScore = 0;
            else
            {
                for (int j = 0; j < BET_ITEM_COUNT; j++)
                {
                    xzScore += m_pPlayerList[uInfoItem.wCharid].dAreaJetton[j];
                }
                nowScore = pServerUserItem->GetUserScore() ;
            }
            /*if (userInfoItem->chairId == m_wBankerUser)
            {
                continue;
            }*/
            if (uInfoItem.wheadid > 12)
            {
                LOG_INFO << " ========== 5 wUserid " << uInfoItem.wUserid << " == headerId:" << uInfoItem.wheadid;
            }
            BenCiBaoMa::PlayerListItem* palyer = gameStart.add_players();
            palyer->set_dwuserid(uInfoItem.wUserid);
            palyer->set_headerid(uInfoItem.wheadid);
            palyer->set_nviplevel(0);
            palyer->set_nickname(" ");

            palyer->set_luserscore(nowScore);

            palyer->set_ltwentywinscore(uInfoItem.GetTwentyJetton());
            palyer->set_ltwentywincount(uInfoItem.GetTwentyWin());
            palyer->set_luserwinscore(uInfoItem.RealMoney);
            if(index==0)
            {
                palyer->set_isdivinemath(1);

            }
            else
            {
                palyer->set_isdivinemath(0);
            }
            palyer->set_index(index);


            palyer->set_isbanker(0);
            //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
            //{
               // palyer->set_isbanker(1);
            //}
            palyer->set_gender(0);
            palyer->set_isapplybanker(0);
            palyer->set_applybankermulti(0);
            palyer->set_ljetton(0);
            palyer->set_szlocation(" ");
            palyer->set_headboxid(0);          
            if (++index >= 6)
                break;
        }
        gameStart.set_cbroundid(m_strRoundId);
        gameStart.set_luserscore(usr->GetUserScore());
        gameStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
        std::string endData = gameStart.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(usr->GetChairId(), BenCiBaoMa::SUB_S_GameStart, (uint8_t *)endData.c_str(), endData.size());
        if(!usr->IsAndroidUser())
        {
            sendGameRecord(usr->GetChairId());
        }
    }
}

// 跑马灯显示
void TableFrameSink::BulletinBoard()
{
    int32_t winscore = 0;
    int idex = 0;
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> playe = m_pITableFrame->GetTableUserItem(i);
        if (!playe || !m_pPlayerList[i].Isplaying)
        {
            continue;
        }
        if (m_pPlayerList[i].RealMoney >= m_pGameRoomInfo->broadcastScore)
        {
            int sgType = SMT_GLOBAL | SMT_SCROLL;
            m_pITableFrame->SendGameMessage(i, "", sgType, m_pPlayerList[i].RealMoney);
        }
    }
    
}

// 重置桌子数据(记录重押数据)
void TableFrameSink::ResetTableDate()
{
    m_iMul = 0;
    m_iResIdx = 0;
    m_iStopIdx = 0;
    m_BestPlayerUserId = 0;
    m_BestPlayerChairId = 0;
    m_BestPlayerWinScore = 0.0;

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        // 玩家没有押分
        if (m_pPlayerList[i].dCurrentPlayerJetton == 0)
        {
            m_pPlayerList[i].clear();
            continue;
        }
        // 否则把重押分数记录
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            m_pPlayerList[i].dLastJetton[j] = m_pPlayerList[i].dAreaJetton[j];
        }

        m_pPlayerList[i].clear();
    }
    m_pBetArea->ThisTurnClear();
}

/*
     * 位置 2-7-11-16-21 是雷克萨斯， 0
     * 位置 3-8-13-17-22 是大众，1
     * 位置 4-9-14-19-23 是奔驰，2
     * 位置 1-5-10-15-20 是宝马，3
     * 位置 18 是保时捷，4
     * 位置 6 是玛莎拉蒂，5
     * 位置 0 是兰博基尼，6
     * 位置 12 是法拉利，7
*/
// 获取开奖结果
void TableFrameSink::Algorithm()
{
    int  isHaveBlackPlayer=0;


    float cotrolWeight[8]={0};
    for(int i=0;i<8;i++)
        cotrolWeight[i]=m_retWeight[i];

    for(int i=0;i<MAX_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton=0;
        for(int i=0;i<8;i++)
        {
            allJetton+=m_pPlayerList[user->GetChairId()].dAreaJetton[i];
        }
        if(allJetton<=0) continue;
        if(user->GetBlacklistInfo().listRoom.size()>0)
        {
            auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
            if(it != user->GetBlacklistInfo().listRoom.end())
            {
                if(it->second>0)
                {
                    isHaveBlackPlayer=it->second;//
                }
                //openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                //openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
            }
        }

    }

    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        //必杀调用
        bcbmAlgorithm.SetMustKill(-1);
        for(int i=0;i<8;i++)
        bcbmAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
        bcbmAlgorithm.GetOpenCard(m_iResIdx, cotrolWeight);
        openSLog("正常算法最终出结果: %d",m_iResIdx);
    }
    else
    {
        if(isHaveBlackPlayer)
        {
            float probilityAll[8]={0.0f};
            //user probilty count
            for(int i=0;i<MAX_PLAYER;i++)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(!user||user->IsAndroidUser())
                {
                    continue;
                }
                int64_t allJetton=0;
                for(int j=0;j<8;j++)
                {
                    allJetton+=m_pPlayerList[user->GetChairId()].dAreaJetton[j];
                }
                if(allJetton==0) continue;

                if(user->GetBlacklistInfo().status == 1 )
                {
                    int shuzhi=0;
                    auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
                    if(it != user->GetBlacklistInfo().listRoom.end())
                    {
                        shuzhi=it->second;
                    }
                    else
                    {
                        shuzhi=0;
                    }
                    float gailv=(float)shuzhi/1000;
                    int64_t allWinscore=0;
                    for(int j=0;j<MAX_JETTON_AREA;j++)
                    {
                        allWinscore+=m_nMuilt[j]*m_pBetArea->AndroidmifourAreajetton[j];
                    }
                   for(int j=0;j<MAX_JETTON_AREA;j++)
                   {
                       if(allWinscore>0)
                       {
                           probilityAll[j]+=(gailv*(float)m_pPlayerList[user->GetChairId()].dAreaJetton[j]*m_nMuilt[j])/(float)allWinscore;
                           openSLog("黑名单玩家 :%d 确定调用黑名单算法 每门传的黑名单控制概率是[%d]：%f",user->GetUserId(),j,(gailv*(float)m_pPlayerList[user->GetChairId()].dAreaJetton[j]*m_nMuilt[j])/(float)allWinscore);
                       }

                   }

                }
            }
            for(int i=0;i<8;i++)
            {
                if(probilityAll[i]>0.0001)
                {
                    float weight=0.0;
                    weight=m_retWeight[i]*(1-probilityAll[i]);
                    cotrolWeight[i]=weight;
                }
                else
                {
                    cotrolWeight[i]=m_retWeight[i];
                }

            }

            bcbmAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill,m_nMuilt, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<8;i++)
             bcbmAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            bcbmAlgorithm.BlackGetOpenCard(m_iResIdx,cotrolWeight);
            openSLog("黑名单算法最终出结果: %d",m_iResIdx);
        }
        else
        {
            bcbmAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill,m_nMuilt, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<8;i++)
             bcbmAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            bcbmAlgorithm.GetOpenCard(m_iResIdx, cotrolWeight);
            openSLog("正常算法最终出结果: %d",m_iResIdx);
        }
    }




    // 位置定位
    static int iconPos[4][5] = {{2,7,11,16,21} , {3,8,13,17,22} , {4,9,14,19,23} ,{1,5,10,15,20}};
    //转换
    string strm = "";
    switch (m_iResIdx)
    {
    case (int)CardType::Lexus : m_iStopIdx = iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "雷克萨斯";strm = "雷克萨斯";break;
    case (int)CardType::Daz : m_iStopIdx =  iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "大众"; strm = "大众"; break;
    case (int)CardType::Benz : m_iStopIdx =  iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "奔驰"; strm = "奔驰"; break;
    case (int)CardType::Bmw : m_iStopIdx =  iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "宝马"; strm = "宝马"; break;
    case (int)CardType::Bsj : m_iStopIdx = 18;LOG(WARNING) << "保时捷"; strm = "保时捷"; break;
    case (int)CardType::Marati : m_iStopIdx = 6;LOG(WARNING) << "玛莎拉蒂"; strm = "玛莎拉蒂"; break;
    case (int)CardType::Lambo : m_iStopIdx = 0;LOG(WARNING) << "兰博基尼"; strm = "兰博基尼"; break;
    case (int)CardType::Ferrari : m_iStopIdx = 12;LOG(WARNING) << "法拉利"; strm = "法拉利"; break;
    default:
        break;
    }
    // 倍数
    m_iMul = m_nMuilt[m_iResIdx];
    // if (m_bTest>0)
        m_lOpenResultCount++;
    m_lOpenResultIndexCount[m_iResIdx]++;
    LOG(WARNING)<< "车型 " << m_iResIdx << " 倍率 " << m_iMul << " 开奖下标 " << m_iResIdx << " 累计开奖次数 " << m_lOpenResultCount;
    openSLog("本局开奖结果:车型:%d,%s,累计开奖次数:%d;",m_iResIdx,strm.c_str(),m_lOpenResultCount);
    string openStr = "开奖车标累计:";
    for (int i = 0; i < BET_ITEM_COUNT; ++i)
    {
        openStr += to_string(i) + "_" + to_string(m_lOpenResultIndexCount[i])+ " ";
    }
    LOG(WARNING) << openStr ;
    openSLog("%s;",openStr.c_str());
    if (m_lOpenResultCount>100000 || m_lOpenResultIndexCount[m_iResIdx]>10000000)
    {
        m_lOpenResultCount = 0;
        memset(m_lOpenResultIndexCount,0,sizeof(m_lOpenResultIndexCount));
    }
}
void TableFrameSink::ArgorithResult()
{
    // 最终开奖结果
    int32_t finalResultIdx = -1;
    // 结果数组
    vector<int32_t> tempWinRetIdx;
    vector<int32_t> tempLoseRetIdx;
    vector<int32_t> tempAllRetIdx;
    // 控制类型
    int32_t isNeedCtrlType = 0; 
    //干涉比率*min（2*（当前库存-上线）/（上线-下线），1）
    //当前库存
    int64_t fcurStor = m_llStorageControl;
    //库存下限
    int64_t flowStor = m_llStorageLowerLimit;
    //库存上限
    int64_t heightStor = m_llStorageHighLimit; 

    // 筛选出结果

    // 允许最大输分值
    int64_t maxScore = (fcurStor - flowStor) * m_stockPercentRate;
    // if (maxScore<500000)
    // {
    //     maxScore = 500000 ;
    // }
    LOG(WARNING) << " 允许最大输分值 maxScore " << maxScore  << " m_stockPercentRate "<< m_stockPercentRate << " curStor " << fcurStor << " lowStor " << flowStor;
    // 统计总押分
    int32_t tempAllBet = 0;
    for (int index = 0; index < BET_ITEM_COUNT; index++)
        tempAllBet += m_pBetArea->AndroidmifourAreajetton[index];

    // 筛选出结果
    string winStr=" 系统可赢选项:",loseStr=" 系统输选项:";
    for (int index = 0; index < BET_ITEM_COUNT; index++)
    {
        // 押分 *  倍数 < 允许最大输分值
        int32_t tempItemWin = m_pBetArea->AndroidmifourAreajetton[index] * (m_nMuilt[index]);
        // if (tempItemWin < maxScore)
        {
            // 系统赢分
            if (tempItemWin <= tempAllBet)
            {
                tempWinRetIdx.push_back(index);
                winStr += to_string(index);
                // 所有可选择结果
                tempAllRetIdx.push_back(index);
            }
            else
            {
                // 系统输分                
                if ((tempItemWin - tempAllBet ) < std::max(maxScore,(int64_t)5000*COIN_RATE) )
                {
                    tempLoseRetIdx.push_back(index);
                    loseStr += to_string(index);
                    // 所有可选择结果
                    tempAllRetIdx.push_back(index);
                }          
            }            
        }
    }
    LOG(WARNING) << winStr << loseStr;
    openSLog("%s,%s;",winStr.c_str(),loseStr.c_str());
    // 达到控制条件
    if( tempAllBet > 0 && (fcurStor >= heightStor || fcurStor <= flowStor) )
    {
        float minfloat = 1.0;//2 * (fcurStor - flowStor)/(heightStor - flowStor);
        if (fcurStor >= heightStor) // 放分
        {
            minfloat = 2.0 * (fcurStor - heightStor)/(heightStor - flowStor);
        }
        else if (fcurStor <= flowStor) // 收分
        {
            minfloat = 2.0 * (flowStor - fcurStor)/(heightStor - flowStor);
        }
        if(minfloat >  1) minfloat = 1.0;
        float fTemp = m_ctrlRate * minfloat;
        float tempRnd = (RandomInt(0, 1000) * 1.0) / 1000;
        LOG(ERROR) << "达到控制条件 min 取值是否正确 " << fTemp << " "<< tempRnd <<"  ctrlRate "<< m_ctrlRate;
        
        if( tempRnd < fTemp ){
            // 1为超出上限，要放分；2为小于下限，要收分
            isNeedCtrlType = (fcurStor > heightStor) ? 1 : 2;
            LOG(ERROR) << "干涉控制[常规] " << isNeedCtrlType << " "<< tempRnd <<"  ctrlRate "<< m_ctrlRate;
        }
        // else if (fcurStor < flowStor)
        // {
        //     // 2为小于下限，要收分
        //     isNeedCtrlType = 2;
        //     LOG(ERROR) << "干涉控制[小于下线要收分] " << isNeedCtrlType << " "<< tempRnd <<"  ctrlRate "<< m_ctrlRate;
        // }

        int32_t loseSize = tempLoseRetIdx.size(); 
        int32_t winSize = tempWinRetIdx.size(); 

        //要控制放分
        if( isNeedCtrlType == 1 ){ 
            if(loseSize > 0){
                finalResultIdx =tempLoseRetIdx[RandomInt(0, loseSize-1)];
            }
            else if(winSize > 0){ 
                finalResultIdx =tempWinRetIdx[RandomInt(0, winSize-1)];
            }
            else{ 
                LOG(ERROR) << "要控制放分,没有可开奖的结果 loseSize" << loseSize << " winSize "<< winSize <<" CtrlType "<<isNeedCtrlType;
            } 
        } 
        //要控制收分
        else if( isNeedCtrlType == 2 ){
            if(winSize > 0){ 
                finalResultIdx = tempWinRetIdx[RandomInt(0, winSize-1)]; 
            }
            else{ 
                LOG(ERROR) << "要控制收分，没有可开奖的结果 loseSize" << loseSize << " winSize "<< winSize <<" CtrlType "<<isNeedCtrlType;
            } 
        }
    } 
    else if (tempAllBet > 0)
    {
        // if (RandomInt(1, 1000) < 350)
        // {
        //     // 1为超出上限，要放分；2为小于下限，要收分
        //     isNeedCtrlType = (RandomInt(1, 100) < 48) ? 1 : 2;
        //     LOG(ERROR) << "干涉控制[随机开] " << isNeedCtrlType <<"  ctrlRate "<< m_ctrlRate;
        // }

        int32_t loseSize = tempLoseRetIdx.size(); 
        int32_t winSize = tempWinRetIdx.size(); 

        //要控制放分
        if( isNeedCtrlType == 1 ){ 
            if(loseSize > 0){
                finalResultIdx =tempLoseRetIdx[RandomInt(0, loseSize-1)];
            }
            else if(winSize > 0){ 
                finalResultIdx =tempWinRetIdx[RandomInt(0, winSize-1)];
            }
            else{ 
                LOG(ERROR) << "要控制放分,没有可开奖的结果 loseSize" << loseSize << " winSize "<< winSize <<" CtrlType "<<isNeedCtrlType;
            } 
        } 
        //要控制收分
        else if( isNeedCtrlType == 2 ){
            if(winSize > 0){ 
                finalResultIdx = tempWinRetIdx[RandomInt(0, winSize-1)]; 
            }
            else{ 
                LOG(ERROR) << "要控制收分，没有可开奖的结果 loseSize" << loseSize << " winSize "<< winSize <<" CtrlType "<<isNeedCtrlType;
            } 
        }
    }
    // 可选择结果
    int allSize = tempAllRetIdx.size();
    // 备份
    m_iResIdx = finalResultIdx;
    // 没有控制结果,则随机，但要符合合理结果
    if (tempAllBet > 0 && allSize > 0 && finalResultIdx == -1)
    {
        int32_t TotalWeight = 0;
        for (int i = 0; i < BET_ITEM_COUNT; ++i)
            TotalWeight += m_retWeight[i];

        assert(TotalWeight > 0);

        // 最多随机500局
        int32_t loopCount = 500;
        bool loopEnd = false;
        int index[8] = {0,1,2,3,4,5,6,7};
        // std::shuffle(&index[0], &index[BET_ITEM_COUNT], STD::Generator::instance().get_mt());
        // string indexStr="随机取权重顺序:";
        // for (int i = 0; i < BET_ITEM_COUNT; ++i)
        // {
        //     indexStr += to_string(index[i]);
        // }
        // LOG(WARNING) << indexStr;
        do
        {
            int32_t tempResIdx = -1;
            /* code */
            if(--loopCount <= 0){
                m_iResIdx = tempAllRetIdx[RandomInt(0, allSize-1)]; 
                LOG(ERROR) << "随机500局没有找到结果 allSize" << allSize << " m_iResIdx "<< m_iResIdx;
                break;
            }

            /* code */
            int tempweightsize = RandomInt(0, TotalWeight - 1);
            int tempweightsize2 = tempweightsize;         
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                // tempweightsize -= m_retWeight[i];
                tempweightsize -= m_retWeight[index[i]];
                if (tempweightsize < 0)
                {
                    tempResIdx = index[i];
                    LOG(WARNING) << "随机开奖权重 " << tempweightsize2 << " 总权重 "<< TotalWeight << " 开奖下标 " << tempResIdx;
                    break;
                }
            } 

            // 有随机出结果
            if(tempResIdx != -1){ 
                for (auto it = tempAllRetIdx.begin(); it != tempAllRetIdx.end(); ++it)
                {
                    // 随机出结果在合理结果中
                    if (tempResIdx == (*it))
                    { 
                        m_iResIdx = tempResIdx;
                        loopEnd = true;
                        LOG(WARNING) << "随机中找到结果 " << tempResIdx << " allSize "<< tempAllRetIdx.size();
                        break;
                    }
                } 
            } 

        } while (!loopEnd); 
    }

    //  没有真人押分则走概率结果
    if (tempAllBet == 0 || allSize == 0 ){

        LOG(WARNING) << "没有真人押分则走概率结果 " << allSize  << " tempAllBet "<< tempAllBet;

        int32_t TotalWeight = 0;
        for (int i = 0; i < BET_ITEM_COUNT; ++i)
            TotalWeight += m_retWeight[i];

        assert(TotalWeight > 0);

        int tempweightsize = RandomInt(0, TotalWeight - 1);
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            tempweightsize -= m_retWeight[i];
            if (tempweightsize < 0)
            {
                m_iResIdx = i;
                break;
            }
        }
    }

    // 位置定位
    static int iconPos[4][5] = {{2,7,11,16,21} , {3,8,13,17,22} , {4,9,14,19,23} ,{1,5,10,15,20}};
    //转换
    string strm = "";
    switch (m_iResIdx)
    {
    case (int)CardType::Lexus : m_iStopIdx = iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "雷克萨斯";strm = "雷克萨斯";break;
    case (int)CardType::Daz : m_iStopIdx =  iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "大众"; strm = "大众"; break;
    case (int)CardType::Benz : m_iStopIdx =  iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "奔驰"; strm = "奔驰"; break;
    case (int)CardType::Bmw : m_iStopIdx =  iconPos[m_iResIdx][RandomInt(0, 4)];LOG(WARNING) << "宝马"; strm = "宝马"; break;
    case (int)CardType::Bsj : m_iStopIdx = 18;LOG(WARNING) << "保时捷"; strm = "保时捷"; break;
    case (int)CardType::Marati : m_iStopIdx = 6;LOG(WARNING) << "玛莎拉蒂"; strm = "玛莎拉蒂"; break;
    case (int)CardType::Lambo : m_iStopIdx = 0;LOG(WARNING) << "兰博基尼"; strm = "兰博基尼"; break;
    case (int)CardType::Ferrari : m_iStopIdx = 12;LOG(WARNING) << "法拉利"; strm = "法拉利"; break;
    default:
        break;
    } 
    // 倍数
    m_iMul = m_nMuilt[m_iResIdx]; 
    // if (m_bTest>0)
        m_lOpenResultCount++;
    m_lOpenResultIndexCount[m_iResIdx]++;
    LOG(WARNING)<< "车型 " << m_iResIdx << " 倍率 " << m_iMul << " 开奖下标 " << m_iResIdx << " 累计开奖次数 " << m_lOpenResultCount;
    openSLog("本局开奖结果:车型:%d,%s,累计开奖次数:%d;",m_iResIdx,strm.c_str(),m_lOpenResultCount);
    string openStr = "开奖车标累计:";
    for (int i = 0; i < BET_ITEM_COUNT; ++i)
    {
        openStr += to_string(i) + "_" + to_string(m_lOpenResultIndexCount[i])+ " ";
    }
    LOG(WARNING) << openStr ;
    openSLog("%s;",openStr.c_str());
    if (m_lOpenResultCount>100000 || m_lOpenResultIndexCount[m_iResIdx]>10000000)
    {
        m_lOpenResultCount = 0;
        memset(m_lOpenResultIndexCount,0,sizeof(m_lOpenResultIndexCount));
    }

}
// 100局游戏记录
void TableFrameSink::QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(chairid);
    if (!player)
        return;
    //BenCiBaoMa::CMD_C_GameRecord gameRecordReq;
    //gameRecordReq.ParseFromArray(data, datasize);

    sendGameRecord(chairid);

}
void TableFrameSink::sendGameRecord(int32_t chairid)
{
    //记录
    BenCiBaoMa::CMD_S_GameRecord gameRecordRsp;

    vector<shared_ptr<OpenResultInfo>> vResultList;
    vResultList = m_vOpenResultListCount_last[0];
    int32_t openCount = vResultList.size();

    gameRecordRsp.set_opencount(openCount);

    for (int i = 0;i < BET_ITEM_COUNT; ++i)
    {
        vResultList = m_vOpenResultListCount_last[i];
        BenCiBaoMa::GameRecordList *gamerecordList = gameRecordRsp.add_recordlists();
        for (int j = 0; j < openCount; j++)
        {
            if (i == 0)
                gameRecordRsp.add_iresultids(vResultList[j]->iResultId);
            BenCiBaoMa::GameRecordListItem *listItem = gamerecordList->add_records();
            listItem->set_notopencount(vResultList[j]->notOpenCount);
        }
    }

    std::string sendData = gameRecordRsp.SerializeAsString();
    LOG(ERROR)<<" send user ask gamerecord length = "<<sendData.size() <<" openCount="<<openCount;
    m_pITableFrame->SendTableData(chairid, BenCiBaoMa::SUB_S_QUERY_GAMERECORD, (uint8_t *)sendData.c_str(), sendData.size());
}

//更新100局开奖结果信息
void TableFrameSink::updateResultList(int resultId)
{
    vector<shared_ptr<OpenResultInfo>> vResultList;
    for (int i = 0;i < BET_ITEM_COUNT;++i)
    {
        shared_ptr<OpenResultInfo> iResultIndex(new OpenResultInfo());
        shared_ptr<OpenResultInfo> iResultIndex_ls(new OpenResultInfo());
        vResultList = m_vOpenResultListCount[i];
        if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
        {
            vResultList.erase(vResultList.begin());
        }
        int resultCount = vResultList.size();
        iResultIndex->clear();
        if (resultCount > 0)
        {
            iResultIndex_ls = vResultList[resultCount - 1];
            iResultIndex->notOpenCount = (i == resultId) ? 0 : (iResultIndex_ls->notOpenCount + 1);
        }
        else
        {
            iResultIndex->notOpenCount = (i == resultId) ? 0 : (iResultIndex->notOpenCount + 1);
        }
        iResultIndex->iResultId = resultId;

        vResultList.push_back(iResultIndex);
        m_vOpenResultListCount[i] = vResultList;

    }

}

// 结算
void TableFrameSink::NormalCalculate()
{
    //ArgorithResult();//老算法

    Algorithm();//新算法
    // 选择出本轮最大赢分玩家
    int32_t winscore = 0;
    int bestChairID = 0;
    bool bFirstOnlineUser = true;
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(i);
        if (!player || !m_pPlayerList[i].Isplaying)
            continue;
        if (bFirstOnlineUser) // 预防在线的玩家都输钱的情况
        {
            bFirstOnlineUser = false;
            winscore = m_pPlayerList[i].ReturnMoney;
            bestChairID = i;
        }
        m_pPlayerList[i].SetPlayerMuticl(m_iResIdx, m_iMul); //she zhi wanjia bei lv
        m_pPlayerList[i].Culculate(m_pITableFrame, m_fTaxRate);//税收比例
        if (winscore < m_pPlayerList[i].ReturnMoney)
        {
            winscore = m_pPlayerList[i].ReturnMoney;
            bestChairID = i;
        }
    }

    // 把最大赢分玩家添加到列表(20秒)
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(bestChairID);
    if (player && m_pPlayerList[bestChairID].ReturnMoney > 0)
    {
        BestPlayerList pla;
        pla.NikenName = player->GetNickName();
        pla.bestID = player->GetChairId();
        pla.WinScore = m_pPlayerList[bestChairID].ReturnMoney;
        time_t t = time(nullptr);

        char ch[64] = {0};

        strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t)); //年-月-日 时-分-秒
        pla.onTime = ch;
        //cout<<p->tm_year+1900<<"  "<<p->tm_mon+1<<"  "<<p->tm_mday<<"  "<<p->tm_hour<<

        if (m_vPlayerList.size() >= 20){
            m_vPlayerList.erase(m_vPlayerList.begin());
        }

        m_vPlayerList.push_back(pla);
    }
	
    //写玩家分(注意顺序)
    WriteUserScore();



    int64_t xzScore = 0;
    int64_t nowScore = 0;


    for(int i=0;i<6;i++)
    {
        if(i>=m_pPlayerInfoList.size()) continue;
        if(!m_pPlayerInfoList[i].Isplaying)
        {
            continue;
        }
        for (int j = 0; j < MAX_PLAYER; j++)
        {
            if (!m_pPlayerList[j].Isplaying)
            {
                continue;
            }
            if(m_pPlayerInfoList[i].wUserid==m_pPlayerList[j].wUserid)
            m_pPlayerInfoList[i]=m_pPlayerList[j];
        }
    }

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> it = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == it)
            continue;
        if (m_pPlayerList[i].dCurrentPlayerJetton<=0)
        {
            m_pPlayerList[i].NotBetJettonCount++;
        }
        else //if (m_pPlayerList[i].NotBetJettonCount < 5)
        {            
            m_pPlayerList[i].NotBetJettonCount = 0;
        }
        BenCiBaoMa::CMD_S_GameEnd gameEnd;
        gameEnd.set_cbplacetime(m_endCountDown);//12); //总时间
        gameEnd.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            gameEnd.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[j]);
            gameEnd.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
        }
        gameEnd.set_lusermaxscore(0);
        gameEnd.set_winindex(m_iStopIdx);
        gameEnd.set_cartype(m_iResIdx);
        gameEnd.set_cbroundid(m_strRoundId);
        LOG(ERROR) << "结算状态 =>UserScore=" << it->GetUserScore() << ",i= "<< i;
        gameEnd.set_luserscore(it->GetUserScore()); // //m_nPlayerScore[wCharid]
        gameEnd.set_mostwinscore(m_pPlayerList[bestChairID].ReturnMoney);

        shared_ptr<CServerUserItem> bestplayer = m_pITableFrame->GetTableUserItem(bestChairID);
        gameEnd.set_headid(bestplayer->GetHeaderId());
        gameEnd.set_headboxid(0); //bestplayer->GetHeadBoxID());
        gameEnd.set_gender(0);    //bestplayer->GetGender());
        m_BestPlayerUserId = bestplayer->GetUserId();
        m_BestPlayerChairId = bestChairID;
        m_BestPlayerWinScore = m_pPlayerList[bestChairID].ReturnMoney;
        gameEnd.set_bestuserid(bestplayer->GetUserId());
        gameEnd.set_bestusernikename(bestplayer->GetNickName());
        gameEnd.set_userwinscore(m_pPlayerList[i].RealMoney);
        gameEnd.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
        int32_t index = 0;
        for (auto &uInfoItem : m_pPlayerInfoList)
        {
            xzScore = 0;
            shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem.wCharid);
            if (!pServerUserItem)
                nowScore = 0;
            else
            {
                for (int j = 0; j < BET_ITEM_COUNT; j++)
                {
                    xzScore += m_pPlayerList[uInfoItem.wCharid].dAreaJetton[j];
                }
                nowScore = pServerUserItem->GetUserScore();
            }
            /*if (userInfoItem->chairId == m_wBankerUser)
            {
                continue;
            }*/
            if (uInfoItem.wheadid > 12)
            {
                LOG_INFO << " ========== 5 wUserid " << uInfoItem.wUserid << " == headerId:" << uInfoItem.wheadid;
            }
            BenCiBaoMa::PlayerListItem* palyer = gameEnd.add_players();
            palyer->set_dwuserid(uInfoItem.wUserid);
            palyer->set_headerid(uInfoItem.wheadid);
            palyer->set_nviplevel(0);
            palyer->set_nickname(" ");

            palyer->set_luserscore(nowScore);

            palyer->set_ltwentywinscore(uInfoItem.GetTwentyJetton());
            palyer->set_ltwentywincount(uInfoItem.GetTwentyWin());
            if(uInfoItem.m_winAfterTax>0)
            {
                int x = uInfoItem.ReturnMoney;
            }
            palyer->set_luserwinscore(uInfoItem.m_winAfterTax);
            if(index==0)
            {
                palyer->set_isdivinemath(1);

            }
            else
            {
                palyer->set_isdivinemath(0);
            }
            palyer->set_index(index);


            palyer->set_isbanker(0);
            //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
            //{
               // palyer->set_isbanker(1);
            //}
            palyer->set_gender(0);
            palyer->set_isapplybanker(0);
            palyer->set_applybankermulti(0);
            palyer->set_ljetton(0);
            palyer->set_szlocation(" ");
            palyer->set_headboxid(0);
            if (++index >= 6)
                break;
        }



        std::string endData = gameEnd.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(it->GetChairId(), BenCiBaoMa::SUB_S_GameEnd, (uint8_t *)endData.c_str(), endData.size()); 

        openSLog("本局结算: wCharid=%d,userid=%d,UserScore=%d,输赢=%d;",it->GetChairId(),it->GetUserId(),it->GetUserScore(),m_pPlayerList[i].RealMoney);



    }
    // 填充路单(长度9)
    if (m_vResultRecored.size() >= 20 )//< 9)
        m_lastRetId = m_vResultRecored[0];
        m_vResultRecored.erase(m_vResultRecored.begin());
    m_vResultRecored.push_back(m_iResIdx);
    updateResultList(m_iResIdx);

}

// 玩家财富排序
void TableFrameSink::PlayerRichSorting()
{
    m_pPlayerInfoList.clear();

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        if (!m_pPlayerList[i].Isplaying)
        {
            continue;
        }
        m_pPlayerInfoList.push_back(m_pPlayerList[i]);
    }
    if (m_pPlayerInfoList.size() > 1)
    {
        sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions);
        sort(++m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
    }
    if(m_pPlayerInfoList.size() > 0)
    {
        m_iShenSuanZiUserId = m_pPlayerInfoList[0].wUserid;
        m_iShenSuanZiChairId = m_pPlayerInfoList[0].wCharid;
    }

}

void TableFrameSink::SendPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{

    if (m_iGameState == BenCiBaoMa::SUB_S_GameEnd)
    {
        if (m_vPlayerList.size() > 1)
        {
            BenCiBaoMa::CMD_S_UserWinList PlayerList;
            for (int i = m_vPlayerList.size() - 2; i > -1; i--)
            {
                shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(m_vPlayerList[i].bestID);
                if (!player)
                {
                    continue;
                }
                BenCiBaoMa::PlayerInfo *playerinfo = PlayerList.add_player();
                playerinfo->set_winscore(m_vPlayerList[i].WinScore);
                playerinfo->set_wintime(m_vPlayerList[i].onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_gender(0);    //player->GetGender());
                playerinfo->set_headboxid(0); //player->GetHeadBoxID());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_nviplevel(0); //player->GetVipLevel());
                playerinfo->set_szlocation(player->GetLocation()); 
            }
            string date = PlayerList.SerializeAsString();
            m_pITableFrame->SendTableData(chairid, BenCiBaoMa::SUB_S_PLAYERLIST, (uint8_t *)date.c_str(), date.size());
        }
    }
    else
    {
        if (m_vPlayerList.size() > 0)
        {
            BenCiBaoMa::CMD_S_UserWinList PlayerList;
            for (int i = m_vPlayerList.size() - 1; i > -1; i--)
            {
                shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(m_vPlayerList[i].bestID);
                if (!player)
                {
                    continue;
                }
                BenCiBaoMa::PlayerInfo *playerinfo = PlayerList.add_player();
                playerinfo->set_winscore(m_vPlayerList[i].WinScore);
                playerinfo->set_wintime(m_vPlayerList[i].onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_gender(0);    //player->GetGender());
                playerinfo->set_headboxid(0); //player->GetHeadBoxID());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_nviplevel(0); //player->GetVipLevel());
                playerinfo->set_szlocation(player->GetLocation()); 
            }
            string date = PlayerList.SerializeAsString();
            m_pITableFrame->SendTableData(chairid, BenCiBaoMa::SUB_S_PLAYERLIST, (uint8_t *)date.c_str(), date.size());
        }
    }

    LOG(WARNING) << "----发送排行榜数据------";
}
bool TableFrameSink::GamePlayerJetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> pSelf = m_pITableFrame->GetTableUserItem(chairid);
    if (!pSelf)
        return false;

    BenCiBaoMa::CMD_C_PlaceJet placeJet;
    placeJet.ParseFromArray(data, datasize);

    int cbJetArea = placeJet.cbjettonarea();
    int32_t lJetScore = placeJet.ljettonscore(); 
    
    LOG(WARNING) << "Jet ="<< " JetArea " << cbJetArea << " JetScore " << lJetScore << " " << chairid << " " << (int)subid;
    // 有效性检查
    if (lJetScore <= 0 || cbJetArea < 0 || cbJetArea >= BET_ITEM_COUNT)
    {
        LOG(ERROR) << "---------------押分有效性检查出错---------->" << cbJetArea <<" "<<lJetScore;
        return false;
    }

    // 筹码合法性检查
    //if (lJetScore != 1 && lJetScore != 10 && lJetScore != 50 && lJetScore != 100 && lJetScore != 500)
    //    return false;

    // 游戏状态判断
    // if (BenCiBaoMa::SUB_S_GameStart != m_iGameState)
    // {
    //     LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_iGameState << " " << BenCiBaoMa::SUB_S_GameStart;
    //     return true;
    // }

    bool bPlaceJetScuccess = false;
    bool nomoney = false;
    int errorAsy = 0;

    do
    {
        if (BenCiBaoMa::SUB_S_GameStart != m_iGameState)
        {
            errorAsy = -1;
            break;
        }

        if (pSelf)
        {
            // 玩家押分判断
            errorAsy = PlayerJettonPanDuan(cbJetArea, lJetScore, chairid, pSelf);
            if (errorAsy != 0)
            {
                break;
            }
        }
        bPlaceJetScuccess = true;
    } while (0);

    // 玩家下注无效
    if (!bPlaceJetScuccess || BenCiBaoMa::SUB_S_GameStart != m_iGameState)
    {
        // 非机器人才返回
        if(!pSelf->IsAndroidUser()){
            BenCiBaoMa::CMD_S_PlaceJettonFail placeJetFail;
            int64_t userid = pSelf->GetUserId();
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(cbJetArea);
            placeJetFail.set_lplacescore(lJetScore);
            placeJetFail.set_cbandroid(pSelf->IsAndroidUser());
            placeJetFail.set_returncode(errorAsy);

            std::string data = placeJetFail.SerializeAsString();
            // if (m_bTest==0)
                m_pITableFrame->SendTableData(pSelf->GetChairId(), BenCiBaoMa::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());
        }

        LOG(WARNING) << "----下注无效-----"<< errorAsy <<" PlaceJet state "<<bPlaceJetScuccess;
        return false;
    }

    int pmayernum = 0; 
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> player = this->m_pITableFrame->GetTableUserItem(i);
        if (!player)
            continue;
        BenCiBaoMa::CMD_S_PlaceJetSuccess placeJetSucc;
        placeJetSucc.Clear();

        int64_t userid = pSelf->GetUserId();
        placeJetSucc.set_dwuserid(userid);
        placeJetSucc.set_cbjettonarea(cbJetArea);
        placeJetSucc.set_ljettonscore(lJetScore);
        placeJetSucc.set_bisandroid(pSelf->IsAndroidUser());

        for (int x = 1; x <= MAX_JETTON_AREA; x++)
        {
            placeJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[x - 1]);
        }

        int32_t alljetton = 0;
        alljetton = m_pPlayerList[i].dCurrentPlayerJetton;
        placeJetSucc.set_luserscore(player->GetUserScore() - alljetton);
        for (int k = 1; k <= MAX_JETTON_AREA; k++)
        {
            placeJetSucc.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[k - 1]);
        } 

        std::string data = placeJetSucc.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(i, BenCiBaoMa::SUB_S_JETTON_SUCCESS, (uint8_t *)data.c_str(), data.size());
        pmayernum++;
    } 

    // LOG(WARNING) << "Jet Scuccess Num="<< pmayernum << " JetArea " << cbJetArea << " JetScore " << lJetScore << " " << chairid << " " << (int)subid;

    return true;
}
bool TableFrameSink::GamePlayerJetton1(int32_t score, int index, int chairid, shared_ptr<CServerUserItem> &pSelf)
{
    int32_t lJetScore = score;
    int cbJetArea = index; 

    bool bPlaceJetScuccess = false; 
    int errorAsy = 0;
    do
    {
        if (BenCiBaoMa::SUB_S_GameStart != m_iGameState)
        {
            errorAsy = -1;
            break;
        }
        if (pSelf)
        { 
            errorAsy = PlayerJettonPanDuan(cbJetArea, lJetScore, chairid, pSelf);
            if (errorAsy != 0)
            {
                break;
            }
        }
        bPlaceJetScuccess = true;
    } while (0);

    // 下注失败处理
    if (!bPlaceJetScuccess)
    {
        BenCiBaoMa::CMD_S_PlaceJettonFail placeJetFail;
        int64_t userid = pSelf->GetUserId();
        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(cbJetArea); //-1=shijian guo   1==wanjia shibai 2=qu yu man 3=wanjia fen buzu
        placeJetFail.set_returncode(errorAsy);
       
        placeJetFail.set_lplacescore(lJetScore);
        placeJetFail.set_cbandroid(pSelf->IsAndroidUser());

        std::string data = placeJetFail.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(pSelf->GetChairId(), BenCiBaoMa::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());

        LOG(WARNING) << "-------------------续押玩家下注无效---------------" << errorAsy;
    }

    return bPlaceJetScuccess;
}
// 重押
void TableFrameSink::Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
    if (!Usr)
        return;

    if (!m_pPlayerList[chairid].Isplaying)
    {
        LOG(ERROR) << "-------------续押失败(玩家状态不对)-------------";
        return;
    }

     // 游戏状态判断
    if (BenCiBaoMa::SUB_S_GameStart != m_iGameState)
    {
        LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_iGameState << " " << BenCiBaoMa::SUB_S_GameStart;
        return;
    }

    int64_t userid = Usr->GetUserId();

    int alljetton = 0;
    int areajetton[BET_ITEM_COUNT] = {0};
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        alljetton += m_pPlayerList[chairid].dLastJetton[i];
        areajetton[i] = m_pPlayerList[chairid].dLastJetton[i];
    }

    // 续押失败
    if (alljetton == 0 || Usr->GetUserScore() < alljetton)
    {
        BenCiBaoMa::CMD_S_PlaceJettonFail placeJetFail;

        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(3);
        placeJetFail.set_lplacescore(0);
        placeJetFail.set_returncode(1);
        placeJetFail.set_cbandroid(Usr->IsAndroidUser());

        std::string data = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(Usr->GetChairId(), BenCiBaoMa::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());
        LOG(ERROR) << "------------------续押失败------------" << alljetton << " " << Usr->GetUserScore();
        return;
    }

    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        // 
        // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值 m_pBetArea->AndroidmifourAreajetton[i]
        if (m_nAreaMaxJetton[i] * COIN_RATE <m_pPlayerList[chairid].dAreaJetton[i]  + m_pPlayerList[chairid].dLastJetton[i])
        {
            BenCiBaoMa::CMD_S_PlaceJettonFail placeJetFail;
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(i);
            placeJetFail.set_lplacescore(m_pPlayerList[chairid].dLastJetton[i]);
            placeJetFail.set_returncode(2);
            placeJetFail.set_cbandroid(Usr->IsAndroidUser());

            std::string data = placeJetFail.SerializeAsString();
            // if (m_bTest==0)
                m_pITableFrame->SendTableData(Usr->GetChairId(), BenCiBaoMa::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());
            LOG(ERROR) << "---------续押失败(超过区域最大押分)---------" <<m_nAreaMaxJetton[i] <<" "<< m_pPlayerList[chairid].dAreaJetton[i] <<" "<<m_pPlayerList[chairid].dLastJetton[i];
            return;
        }
    }

    //返回续押数组
    BenCiBaoMa::CMD_S_RepeatJetSuccess RepeatJet;
    RepeatJet.set_dwuserid(userid);
    RepeatJet.set_bisandroid(Usr->IsAndroidUser());
    RepeatJet.set_luserscore(Usr->GetUserScore() - alljetton);
   
    bool bIsJetOk = false;
    //RepeatJet
    for (tagBetInfo betVal : m_pPlayerList[chairid].JettonValues)
    {
        BenCiBaoMa::RepeatInfo *RepeatInfo = RepeatJet.add_trepeat();
        RepeatInfo->set_luserid(userid);
        RepeatInfo->set_tljettonscore(betVal.betScore);
        RepeatInfo->set_tjetidx(betVal.betIdx);
        bIsJetOk = GamePlayerJetton1(betVal.betScore, betVal.betIdx, chairid, Usr);
        areajetton[betVal.betIdx] -= betVal.betScore;
    }

    // 重押失败
    if(!bIsJetOk) {
        LOG(ERROR) << "---------重押失败 重押失败 重押失败--------" << m_pPlayerList[chairid].JettonValues.size();
        return;
    }

    //每个区域的总下分
    for (int i = 0; i < MAX_JETTON_AREA; i++){
        RepeatJet.add_alljettonscore(m_pBetArea->mifourAreajetton[i]);
    }

    //单个玩家每个区域的总下分
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> player = this->m_pITableFrame->GetTableUserItem(i);
        if (!player) continue;

        RepeatJet.clear_selfjettonscore();
        for (int k = 0; k < MAX_JETTON_AREA; k++){
            RepeatJet.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[k]);
        }

        std::string data = RepeatJet.SerializeAsString();
        m_pITableFrame->SendTableData(i, BenCiBaoMa::SUB_S_REPEAT_JETTON_OK, (uint8_t *)data.c_str(), data.size());
    }

    LOG(ERROR) << "---------重押成功--------";
}
void TableFrameSink::GamePlayerQueryPlayerList(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd)
{
    shared_ptr<CServerUserItem> pIIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if (!pIIServerUserItem)
        return;
    vector<strPlalyers> playerInfoList;
    playerInfoList.clear();
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        if (!m_pPlayerList[i].Isplaying)
        {
            continue;
        }
        playerInfoList.push_back(m_pPlayerList[i]);
    }
    if (playerInfoList.size() > 1)
    {
        sort(playerInfoList.begin(), playerInfoList.end(), Comparingconditions2);
    }
    BenCiBaoMa::CMD_C_PlayerList querylist;
    querylist.ParseFromArray(data, datasize);
    int32_t limitcount = querylist.nlimitcount();
    int32_t beginindex = querylist.nbeginindex();
    int32_t resultcount = 0;
    int32_t endindex = 0;
    BenCiBaoMa::CMD_S_PlayerList playerlist;
    int32_t wMaxPlayer = m_pITableFrame->GetMaxPlayerCount();
    if (!limitcount)
        limitcount = wMaxPlayer;
    int index = 0;
    //PlayerRichSorting();

    for (vector<strPlalyers>::iterator it = playerInfoList.begin(); it != playerInfoList.end(); it++)
    {
        shared_ptr<CServerUserItem> pUser = this->m_pITableFrame->GetTableUserItem((*it).wCharid);
        if (!pUser)
            continue;
        if (!(*it).Isplaying)
            continue;
        BenCiBaoMa::PlayerListItem *item = playerlist.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser->GetHeaderId());
        item->set_nviplevel(0); //pUser->GetVipLevel());
        item->set_nickname(pUser->GetNickName());
        item->set_headboxid(0); //pUser->GetHeadBoxID());
        if (IsEnd)
        {
            item->set_luserscore(pUser->GetUserScore() - (*it).dCurrentPlayerJetton);
        }
        else
        {
            item->set_luserscore(pUser->GetUserScore());
        }
        item->set_isdivinemath(0);
        if (pUser->GetUserId() == m_iShenSuanZiUserId)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(index);
        item->set_ltwentywinscore((*it).GetTwentyWinScore());
        item->set_ltwentywincount((*it).GetTwentyWin());

        item->set_gender(0); //pUser->GetGender());

        endindex = index + 1;
        index++;
        resultcount++;
        if (resultcount >= 20)
        {
            break;
        }
    }
    playerlist.set_nendindex(endindex);
    playerlist.set_nresultcount(resultcount);
    std::string lstData = playerlist.SerializeAsString();
    m_pITableFrame->SendTableData(pIIServerUserItem->GetChairId(), BenCiBaoMa::SUB_S_QUERY_PLAYLIST, (uint8_t *)lstData.c_str(), lstData.size());
}
// 发送场景消息(押分阶段)
void TableFrameSink::SendGameSceneStart(int64_t userid, bool lookon)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userid);

    if (!player)
        return;

    LOG(WARNING) << "********** Send GameScene Start **********" << userid;

    BenCiBaoMa::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime(m_betCountDown);
    scenceStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
    //筹码数量
    int jsize = 6;
    int32_t dAlljetton = 0;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        BenCiBaoMa::CMD_AeraInfo *areainfo = scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_pPlayerList[player->GetChairId()].dAreaJetton[i]);
        areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]);
        // areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]-m_pPlayerList[player->GetChairId()].dsliceSecondsAllAreaJetton[i]);
        areainfo->set_mutical(m_nMuilt[i]);
        dAlljetton += m_pPlayerList[player->GetChairId()].dAreaJetton[i];
      
        //押分筹码
        if(i < jsize)
            scenceStart.add_betitems(m_currentchips[player->GetChairId()][i]);
    }
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {
        scenceStart.add_cartype(m_vResultRecored[i]);
    }

    scenceStart.set_nlastretid(m_lastRetId);
    

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> usr = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == usr)  continue;

        if (m_pPlayerList[i].dCurrentPlayerJetton > 0)
        {
            // 增加玩家押分筹码值
            for (tagBetInfo betVal : m_pPlayerList[i].JettonValues)
            {
                BenCiBaoMa::RepeatInfo *RepeatInfo = scenceStart.add_trepeat();
                RepeatInfo->set_luserid(usr->GetUserId());
                RepeatInfo->set_tljettonscore(betVal.betScore);
                RepeatInfo->set_tjetidx(betVal.betIdx);
            }
         }
    } 

    int64_t xzScore = 0;
    int64_t nowScore = 0;
    int32_t index = 0;
    for (auto &uInfoItem : m_pPlayerInfoList)
    {
        xzScore = 0;
        shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem.wCharid);
        if (!pServerUserItem)
            nowScore = 0;
        else
        {
            for (int j = 0; j < BET_ITEM_COUNT; j++)
            {
                xzScore += m_pPlayerList[uInfoItem.wCharid].dAreaJetton[j];
            }
            nowScore = pServerUserItem->GetUserScore();
        }
        if (uInfoItem.wheadid > 12)
        {
            LOG_INFO << " ========== 5 wUserid " << uInfoItem.wUserid << " == headerId:" << uInfoItem.wheadid;
        }
        BenCiBaoMa::PlayerListItem* palyer = scenceStart.add_players();
        palyer->set_dwuserid(uInfoItem.wUserid);
        palyer->set_headerid(uInfoItem.wheadid);
        palyer->set_nviplevel(0);
        palyer->set_nickname(" ");

        palyer->set_luserscore(nowScore);

        palyer->set_ltwentywinscore(uInfoItem.GetTwentyJetton());
        palyer->set_ltwentywincount(uInfoItem.GetTwentyWin());
        palyer->set_luserwinscore(uInfoItem.RealMoney);
        if(index==0)
        {
            palyer->set_isdivinemath(1);

        }
        else
        {
            palyer->set_isdivinemath(0);
        }
        palyer->set_index(index);


        palyer->set_isbanker(0);
        //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
        //{
           // palyer->set_isbanker(1);
        //}
        palyer->set_gender(0);
        palyer->set_isapplybanker(0);
        palyer->set_applybankermulti(0);
        palyer->set_ljetton(0);
        palyer->set_szlocation(" ");
        palyer->set_headboxid(0);
        if (++index >= 6)
            break;
    }


    scenceStart.set_ncurstate(0); 
    scenceStart.set_ncurretidx(m_iStopIdx);

    scenceStart.set_luserscore(player->GetUserScore() );
    scenceStart.set_dwuserid(player->GetUserId());
    scenceStart.set_gender(0);    //player->GetGender());
    scenceStart.set_headboxid(0); //player->GetHeadBoxID());
    scenceStart.set_headerid(player->GetHeaderId());
    scenceStart.set_nickname(player->GetNickName());
    scenceStart.set_nviplevel(0); //player->GetVipLevel());
    //scenceStart.set_szlocation(player->GetLocation());
    scenceStart.set_cbroundid(m_strRoundId);
    scenceStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
    std::string data = scenceStart.SerializeAsString();
    m_pITableFrame->SendTableData(player->GetChairId(), BenCiBaoMa::SUB_S_SCENE_START, (uint8_t *)data.c_str(), data.size());
}

// 发送场景消息(结算阶段)
void TableFrameSink::SendGameSceneEnd(int64_t userid, bool lookon)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userid);

    if (!player)
        return;

    BenCiBaoMa::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime(m_endCountDown);
    scenceStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);

  //筹码数量
    int jsize = 6;

    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        BenCiBaoMa::CMD_AeraInfo *areainfo = scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_pPlayerList[player->GetChairId()].dAreaJetton[i]);
        areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]);
        areainfo->set_mutical(m_nMuilt[i]);

        //押分筹码
        if(i < jsize)
            scenceStart.add_betitems(m_currentchips[player->GetChairId()][i]);
    }

    scenceStart.set_nlastretid(m_lastRetId);

    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {
        scenceStart.add_cartype(m_vResultRecored[i]);
    }

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        shared_ptr<CServerUserItem> usr = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == usr)  continue;

        if (m_pPlayerList[i].dCurrentPlayerJetton > 0)
        {
            // 增加玩家押分筹码值
            for (tagBetInfo betVal : m_pPlayerList[i].JettonValues)
            {
                BenCiBaoMa::RepeatInfo *RepeatInfo = scenceStart.add_trepeat();
                RepeatInfo->set_luserid(usr->GetUserId());
                RepeatInfo->set_tljettonscore(betVal.betScore);
                RepeatInfo->set_tjetidx(betVal.betIdx);
            }
         }
    }
    int64_t xzScore = 0;
    int64_t nowScore = 0;
    int32_t index = 0;
    for (auto &uInfoItem : m_pPlayerInfoList)
    {
        xzScore = 0;
        shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem.wCharid);
        if (!pServerUserItem)
            nowScore = 0;
        else
        {
            for (int j = 0; j < BET_ITEM_COUNT; j++)
            {
                xzScore += m_pPlayerList[uInfoItem.wCharid].dAreaJetton[j];
            }
            nowScore = pServerUserItem->GetUserScore();
        }
        if (uInfoItem.wheadid > 12)
        {
            LOG_INFO << " ========== 5 wUserid " << uInfoItem.wUserid << " == headerId:" << uInfoItem.wheadid;
        }
        BenCiBaoMa::PlayerListItem* palyer = scenceStart.add_players();
        palyer->set_dwuserid(uInfoItem.wUserid);
        palyer->set_headerid(uInfoItem.wheadid);
        palyer->set_nviplevel(0);
        palyer->set_nickname(" ");

        palyer->set_luserscore(nowScore);

        palyer->set_ltwentywinscore(uInfoItem.GetTwentyJetton());
        palyer->set_ltwentywincount(uInfoItem.GetTwentyWin());
        palyer->set_luserwinscore(uInfoItem.RealMoney);
        if(index==0)
        {
            palyer->set_isdivinemath(1);

        }
        else
        {
            palyer->set_isdivinemath(0);
        }
        palyer->set_index(index);


        palyer->set_isbanker(0);
        //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
        //{
           // palyer->set_isbanker(1);
        //}
        palyer->set_gender(0);
        palyer->set_isapplybanker(0);
        palyer->set_applybankermulti(0);
        palyer->set_ljetton(0);
        palyer->set_szlocation(" ");
        palyer->set_headboxid(0);
        if (++index >= 6)
            break;
    }
    scenceStart.set_ncurstate(1); 
    scenceStart.set_ncurretidx(m_iStopIdx);
    scenceStart.set_luserscore(player->GetUserScore());
    scenceStart.set_dwuserid(player->GetUserId());
    scenceStart.set_gender(0);    //player->GetGender());
    scenceStart.set_headboxid(0); //player->GetHeadBoxID());
    scenceStart.set_headerid(player->GetHeaderId());
    scenceStart.set_nickname(player->GetNickName());
    scenceStart.set_nviplevel(0); //player->GetVipLevel());
    //scenceStart.set_szlocation(player->GetLocation());
    scenceStart.set_cbroundid(m_strRoundId);
    scenceStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));

    std::string data = scenceStart.SerializeAsString();
    m_pITableFrame->SendTableData(player->GetChairId(), BenCiBaoMa::SUB_S_SCENE_START, (uint8_t *)data.c_str(), data.size());

    LOG(WARNING) << "********** 发送场景消息(结算阶段) **********" << userid;
}

// 押分判断
int TableFrameSink::PlayerJettonPanDuan(int index, int32_t score, int32_t chairid, shared_ptr<CServerUserItem> &pSelf)
{
    if (index < 0 || index >= BET_ITEM_COUNT || score <= 0)
    {
        LOG(ERROR) << "---------押分判断:失败 --------index=" << index << ",score=" << score;
        return 1;
    }

    int32_t allBetScore = 0;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        allBetScore += m_pPlayerList[chairid].dAreaJetton[i];
    }

    // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
    /*
    if (m_nAreaMaxJetton[index] * COIN_RATE < m_pBetArea->mifourAreajetton[index] + score)
    {
        LOG(ERROR) << "---------超过区域最大押分--------" << m_nAreaMaxJetton[index];
        return 2;
    }
    */
    
    // 判断是否超过区域最大押分   < 玩家每个区域的总下分  + 本次玩家押分值
    if (m_nAreaMaxJetton[index] * COIN_RATE < m_pPlayerList[chairid].dAreaJetton[index] + score)
    {
        LOG(ERROR) << "---------超过区域最大押分--------" << m_nAreaMaxJetton[index];
        return 2;
    }

    // 判断是否小于准许进入分
    if (pSelf->GetUserScore() < m_pGameRoomInfo->enterMinScore)
    {
        LOG(WARNING) << "---------小于准许进入分--------";
        return 3;
    }

     bool bFind = false;

     for(int i=0;i<6;i++)
     {
         if(m_currentchips[chairid][i]==score)
         {
             bFind = true;
             break;
         }
     }
//     for(auto it : m_pGameRoomInfo->jettons)
//     {
//         if(it == score)
//         {
//             bFind = true;
//             break;
//         }
//     }
     if(!bFind)
     {
         LOG_ERROR << " Jettons Error:"<<score;
         return 4;       // 下注的筹码值无效
     }

     //已经下的总注
     if(pSelf->GetUserScore() < allBetScore + score || pSelf->GetUserScore() - allBetScore < 50.0)
     {
         LOG_ERROR << " Real User Score is not enough ";
         return 5;      // 押注已达最大的下注金额
     }

    // 清缓存筹码
    if (m_pPlayerList[chairid].dCurrentPlayerJetton == 0){
        m_pPlayerList[chairid].clearJetton();
    }

    // 记录真人押分,有玩家押分则添加
    if(!pSelf->IsAndroidUser()) {

        // 记录押分步骤
        m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime,to_string(score),-1,opBet,chairid,index);

        // 记录真人押分(有玩家押分则添加)
        if(m_pPlayerList[chairid].dCurrentPlayerJetton == 0)
        {
            LOG(WARNING) << "********** 记录真人押分（有玩家押分则添加）**********" << pSelf->GetUserId();
            m_replay.addPlayer(pSelf->GetUserId(),pSelf->GetAccount(),pSelf->GetUserScore(),chairid);
        } 
        // LOG(WARNING) << "******JettonValues******["<<chairid << "]" << m_pPlayerList[chairid].JettonValues.size();
    }

    tagBetInfo betinfo = {0};
    betinfo.betIdx = index;
    betinfo.betScore = score;
    // 记录玩家押分筹码值
    m_pPlayerList[chairid].JettonValues.push_back(betinfo);

    // 允许下注(分数累加)
    m_pPlayerList[chairid].dAreaJetton[index] += score;
    // 当前玩家押分
    m_pPlayerList[chairid].dCurrentPlayerJetton = allBetScore + score;
    // 押分区增加押分
    m_pBetArea->SetJettonScore(index, score, /*mpBankerManager->GetBankerScore()*/0, pSelf->IsAndroidUser());
    //一段时间内各区域其他玩家的总下注
    for (int i = 0; i < MAX_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> UserItem = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == UserItem || i==chairid)
            continue;
        m_pPlayerList[i].dsliceSecondsAllJetton += score;
        m_pPlayerList[i].dsliceSecondsAllAreaJetton[index] += score;
    }
    return 0;
}

//发送一段时间内相对自己的其他所有玩家的总押注数据
void TableFrameSink::OnTimerSendOtherBetJetton()
{
    for (int i = 0; i < MAX_PLAYER; ++i)
    {
        shared_ptr<CServerUserItem> pUserItem = this->m_pITableFrame->GetTableUserItem(i);
        if (nullptr == pUserItem)
            continue;
        // if (!m_pPlayerList[i].Isplaying)
        // {
        //     continue;
        // }
        // if (pUserItem->IsAndroidUser())
        // {
        //     continue;
        // }
        if (m_pPlayerList[i].dsliceSecondsAllJetton <= 0)
        {
            continue;
        }
        // 押分区增加押分
        // int32_t score = 0;
        // for (int index = 0; index < MAX_JETTON_AREA; index++)
        // {
        //     score = m_pPlayerList[i].dsliceSecondsAllAreaJetton[index];
        //     if ( score <= 0 ) continue;
        //     m_pBetArea->SetJettonScore(index, score, 0, pUserItem->IsAndroidUser());
        // }

        BenCiBaoMa::CMD_S_OtherPlaceJetSuccess otherPlaceJetSucc;
        otherPlaceJetSucc.Clear();

        int64_t userid = -1;
        otherPlaceJetSucc.set_dwuserid(userid);
        for (int x = 1; x <= MAX_JETTON_AREA; x++)
        {
            otherPlaceJetSucc.add_alljettonscore(m_pPlayerList[i].dsliceSecondsAllAreaJetton[x-1]);
            otherPlaceJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[x - 1]);
        }

        std::string data = otherPlaceJetSucc.SerializeAsString();
        // m_pITableFrame->SendTableData(i, BenCiBaoMa::SUB_S_JETTON_SUCCESS_OTHER, (uint8_t *)data.c_str(), data.size());

        m_pPlayerList[i].clearOtherJetton();
    }
}

void TableFrameSink::openSLog(const char *p, ...)
{
    if(!m_bWritelog) 
        return;
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//benzbmw//bcbm_table_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
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
}
//设置当局游戏详情
void TableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId,int64_t userScore, int64_t userWinScorePure)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail="";
	BenCiBaoMa::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_residx(m_iResIdx);				//开奖结果
	details.set_idxmul(m_iMul);					//结果倍数
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userWinScorePure); //总输赢积分

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		for (int betIdx = 0;betIdx < BET_ITEM_COUNT; betIdx++) 
		{
			// 结算时记录
			int64_t winScore = m_pPlayerList[chairId].dAreaWin[betIdx];
			int64_t betScore = m_pPlayerList[chairId].dAreaJetton[betIdx];
			int32_t	nMuilt = m_nMuilt[betIdx];// 倍率表m_nMuilt
			//if (betScore > 0) 
			{
				BenCiBaoMa::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//区域倍数
				detail->set_dareaidmul(nMuilt);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
			}
		}		
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
}

extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<TableFrameSink> pTableFrameSink(new TableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink> &pSink)
{
    pSink.reset();
}
