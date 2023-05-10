#include <glog/logging.h>

#include <boost/filesystem.hpp>
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
#include "proto/Jsys.Message.pb.h"

#include "betarea.h" 
// #include "Algorithmc.h" 
#include "JsysAlgorithm.h"
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
        google::InitGoogleLogging("Jsys");
#else
        dir += "/Jsys";
        google::InitGoogleLogging("Jsys");
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
    stockWeak = 1.0;
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
	// 初始化20个随机结果
	int iResultId = 0;
	int32_t wholeweight = 0;
	int32_t weight[BET_ITEM_COUNT] = { 1598,1598,1199,1199,1199,1199,796,796,294,122,0,0 };
	for (int i = 0;i < BET_ITEM_COUNT - 2;i++)
	{
		wholeweight += weight[i];
	}
	int pribility = 0;
	int res = 0;	
	for (int i = 0; i < RES_RECORD_COUNT; i++)
	{
        res = 0;
		pribility = m_random.betweenInt(0, wholeweight).randInt_mt(true);
		for (int j = 0;j < BET_ITEM_COUNT - 2;j++)
		{
			res += weight[j];
			if (pribility - res <= 0)
			{
				iResultId = j;
				break;
			}
		}
		m_vResultRecored.push_back(iResultId);
		updateResultList(iResultId);
	}

	m_vOpenResultListCount_last.clear();
	for (auto &it : m_vOpenResultListCount)
	{
		m_vOpenResultListCount_last[it.first] = it.second;
	}
    m_lastRetId = -1;

    m_vPlayerList.clear(); 
    m_pPlayerInfoList.clear(); 
	m_pPlayerInfoList_20.clear();
    m_bIsOpen = false; 
    m_pITableFrame = nullptr;
    m_EapseTime = (int32_t)time(nullptr);
    m_iGameState = GAME_STATUS_INIT;
    m_strRoundId = "";
    
    m_pBetArea.reset(new BetArea());
    
    m_UserInfo.clear();
    m_iRetMul = 20;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        m_nMuilt[i] = 0;
        m_lOpenResultIndexCount[i] = 0;
    }

    // m_LoopThread.startLoop();

    m_bIsContinue = true;
    m_fTMJettonBroadCard = 0.5f;
    m_testInex=-1;
    m_testResultInex = -1;
	m_testInex_sd = -1;
	m_testResultInex_sd = -1;
    m_lOpenResultCount = 0;
    m_bTest = 0;
    m_nTestTimes = 0;
    m_bWritelog = false;
    m_nTestType = 0;
    m_bTestAgain = false;
    m_dControlkill=0;
    m_lLimitPoints=0;
    m_iStopIndex = 0;
	m_iStopIndex_sd = 0;
    for(int i=0;i<MAX_PLAYER;i++)
    {
        m_currentchips[i][0]=1;
        m_currentchips[i][1]=5;
        m_currentchips[i][2]=10;
        m_currentchips[i][3]=50;
        m_currentchips[i][4]=100;
        m_currentchips[i][5]=500;
    }
	m_ShenSuanZiId = 0;
	memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));
	memset(m_retWeight, 0, sizeof(m_retWeight));
	m_addCountDown_sd = 1.0;
	m_addCountDown_cj = 1.0;
	m_fGameStartAniTime = 1.0;
	m_fGameEndAniTime = 1.0;
	m_iResultId_sd = 0;
	m_iResultMul_js = 24;
	m_iResultMul_cj = 0;
	m_fResetPlayerTime = 5.0f;
	m_bGameEnd = false;
	m_ctrlGradePercentRate[0] = 600;
	m_ctrlGradePercentRate[1] = 900;
	m_ctrlGradeMul[0] = 50;
	m_ctrlGradeMul[1] = 25;
}
TableFrameSink::~TableFrameSink()
{
    m_pITableFrame = nullptr;
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    m_LoopThread->getLoop()->cancel(m_TimerIdTest);
	m_LoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
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
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->wCharid;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(GAME_STATUS_START == status)
        {
            if (userInfo->Isplaying)
                totalxz = userInfo->dCurrentPlayerJetton;
        }

        if(totalxz > 0)
            canLeft = false;

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

    auto it = m_UserInfo.find(userid);
    if(m_UserInfo.end() == it)
    {
        shared_ptr<UserInfo> userInfo(new UserInfo());
        userInfo->Isplaying = true;
        userInfo->wCharid = wCharid;
        userInfo->wUserid = userid;
		userInfo->headerId = m_pPlayer->GetHeaderId();

        m_UserInfo[userid] = userInfo;
    }

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

	for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
	{
        if ((*it)->wUserid == userid)
		{
			m_pPlayerInfoList.erase(it);
			m_pPlayerInfoList.push_back(m_UserInfo[userid]);
			break;
		}
	}
    for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
	{
        if ((*it).wUserid == userid)
		{
			m_pPlayerInfoList_20.erase(it);
            m_pPlayerInfoList_20.push_back((*m_UserInfo[userid]));
			break;
		}
	}
	
    // 启动游戏
    if(!m_bIsOpen)
    { 
        // mpBankerManager->UpdateBankerData(); //一开始设置系统庄家
		m_ShenSuanZiId = m_pPlayer->GetUserId();
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

        // 修改miGameSceneState
        m_iGameState = Jsys::SUB_S_GameStart; 
        // 启动定时器
        GamefirstInGame();

        // if (m_bTest>0)
        // {
        //     m_LoopThread->getLoop()->cancel(m_TimerIdTest);
        //     m_TimerIdTest=m_LoopThread->getLoop()->runEvery(0.04, boost::bind(&TableFrameSink::TestGameMessage, this,wCharid,m_nTestType));
        //     openSLog("本次测试%d开始;",m_nTestTimes);
        //     openSLog("开始库存:High Limit:%d,Lower Limit:%d,,Aver Line:%d",m_llStorageHighLimit,m_llStorageLowerLimit,m_llStorageControl);
        //     m_bTestAgain = true;
        // } 
    }

    // 发送场景消息
    switch (m_iGameState)
    {
    case Jsys::SUB_S_GameStart:
    {
        SendGameSceneStart(userid, false);
        break;
    }
    case Jsys::SUB_S_GameEnd:
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

    bool bClear = false;
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userid];
    uint32_t chairId = userInfo->wCharid;
    if (chairId < 0 || chairId >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a Wrong chair ID" << "   " << (int)chairId;
    }
    if (m_bIsClearUser)
    {
        m_UserInfo.erase(userid);
        m_pITableFrame->ClearTableUser(chairId);
        bClear = true;
    }
    else if (!user->IsAndroidUser() && userInfo->dCurrentPlayerJetton <= 0)
    {
        m_UserInfo.erase(userid);
        m_pITableFrame->ClearTableUser(chairId);
        bClear = true;
    }

    LOG(WARNING) << "**********玩家离开 3**********[" << chairId << "] " << userid;
    if (userid == m_ShenSuanZiId)
	{
		m_ShenSuanZiId = 0;
	}
	if (bClear)
	{
		for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
		{
			if ((*it)->wUserid == userid)
			{
                m_pPlayerInfoList.erase(it);
				break;
			}
		}
        for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
		{
            if ((*it).wUserid == userid)
			{
				m_pPlayerInfoList_20.erase(it);
				break;
			}
		}
	}
    return bClear;
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
    m_replay.clear(); 
    m_replay.cellscore = m_pGameRoomInfo->floorScore; 
    m_replay.roomname = m_pGameRoomInfo->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    ReadStorageInfo();
    //初始化游戏数据
    // m_wGameStatus = GAME_STATUS_INIT;

    LOG(WARNING) << "Set Table Frame " << m_pGameRoomInfo->roomName << " " << m_pGameRoomInfo->roomId << " " << m_pGameRoomInfo->tableCount;

    OnGameStart(); // m_pITableFrame->onStartGame();

    // Algorithmc::Instence()->InitAlgor(m_pITableFrame->GetMaxPlayerCount(), m_pGameRoomInfo->tableCount, m_pGameRoomInfo->roomId);

    m_UserInfo.clear();
    // 读配置
    ReadInI();
    srand((int)time(nullptr));
	/*string str = "";
	int textNum = 0;
	for (int i=0;i<200;i++)
	{
		textNum = m_random.betweenInt(0, 2).randInt_mt(true);
		str += to_string(textNum) + "  ";
	}
	LOG(INFO) << "m_random.betweenInt(0, 2):" << str;
	openSLog("m_random.betweenInt(0, 2):：%s", str.c_str());*/
    return true;
}

void TableFrameSink::GamefirstInGame()
{
	getJSodds();
    // 启动定时器
    float betCountDown = m_betCountDown + m_fGameStartAniTime; //1s,预留前端的开始动画
    m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(m_betCountDown, boost::bind(&TableFrameSink::GameTimerEnd, this));

    m_timetickAll = betCountDown;
    m_timetick = 0;
    // 测试
    // m_TimerIdTest = m_LoopThread->getLoop()->runEvery(1, [&](){
    //     // LOG(WARNING) << "Tick---> " << (m_timetickAll - (m_timetick++)) ;
    // });

    LOG(WARNING) << "--- 启动桌子---" << __FUNCTION__ << " " << m_iGameState;
}

void TableFrameSink::GameTimerStart()
{
    LOG(WARNING) << "--- 开始游戏 1---" << __FUNCTION__;
    //连续5局未下注踢出房间
    //CheckKickOutUser();
    // 设置游戏状态
    m_iGameState = Jsys::SUB_S_GameStart;
    m_pITableFrame->SetGameStatus(Jsys::SUB_S_GameStart);
	m_bGameEnd = false;
    m_timetick = 0;
    float betCountDown = m_betCountDown + m_fGameStartAniTime; //1s,预留前端的开始动画
    m_timetickAll = betCountDown;

    m_startTime = chrono::system_clock::now();
    m_dwReadyTime = (uint32_t)time(nullptr);
    m_strRoundId = m_pITableFrame->GetNewRoundId();

    m_replay.clear();
    m_replay.cellscore = m_pGameRoomInfo->floorScore;
    m_replay.roomname = m_pGameRoomInfo->roomName;
    m_replay.gameinfoid = m_strRoundId;

    vector<uint64_t> clearUserIdVec;
    vector<uint32_t> clearChairIdVec;

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if (!pUserItem)
            continue;
        // 踢出离线玩家
        if (pUserItem->GetUserStatus() == sOffline)
        {
            clearUserIdVec.push_back(userInfoItem->wUserid);
            clearChairIdVec.push_back(userInfoItem->wCharid);
            LOG(INFO) << "踢出离线玩家 "<< userInfoItem->wCharid << "  " <<  pUserItem->GetUserId();
            continue;
        }
        if (!userInfoItem->Isplaying || pUserItem->IsAndroidUser())
        {
            continue;
        }
        LOG(WARNING) << "user=" << pUserItem->GetUserId() << " bet1=" << userInfoItem->dAreaJetton[0] << " bet2=" << userInfoItem->dAreaJetton[1] << " bet3=" << userInfoItem->dAreaJetton[2]
                     << " bet4=" << userInfoItem->dAreaJetton[3] << " bet5=" << userInfoItem->dAreaJetton[5] << " bet6=" << userInfoItem->dAreaJetton[5] << " bet7=" << userInfoItem->dAreaJetton[6]
                     << " bet8=" << userInfoItem->dAreaJetton[7];

        LOG(WARNING) << "user=" << pUserItem->GetUserId() << " playereWin=" << userInfoItem->m_winAfterTax;
    }
    uint32_t size = clearUserIdVec.size();
    for (int i = 0; i < size; ++i)
    {
        m_pITableFrame->ClearTableUser(clearChairIdVec[i]);
        m_UserInfo.erase(clearUserIdVec[i]);
		//for (auto it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); ++it)
		for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
		{
            if (clearUserIdVec[i] == (*it)->wUserid)
			{
				m_pPlayerInfoList.erase(it);
				break;
			}
		}
        for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
		{
            if (clearUserIdVec[i] == (*it).wUserid)
			{
				m_pPlayerInfoList_20.erase(it);
				break;
			}
		}
    }

    // 记录当前时间
    m_EapseTime = time(nullptr);

    // 跑马灯显示
    // BulletinBoard();
    LOG(ERROR) << "游戏开始1111111111111111111111111111111111111111111111111111111111111";
	RefreshPlayerList();
    //clear Table
    ResetTableDate();
    // 服务器没启动踢出玩家
    if (m_pGameRoomInfo->serverStatus !=SERVER_RUNNING)
    {
        clearUserIdVec.clear();
        clearChairIdVec.clear();
        for (auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
            if(!pUserItem)
                continue;
            pUserItem->setOffline();
            clearUserIdVec.push_back(userInfoItem->wUserid);
            clearChairIdVec.push_back(userInfoItem->wCharid);
        }
        uint32_t size = clearUserIdVec.size();
        for (int i = 0; i < size; ++i)
        {
            m_pITableFrame->ClearTableUser(clearChairIdVec[i],true, true, ERROR_ENTERROOM_SERVERSTOP);
            m_UserInfo.erase(clearUserIdVec[i]);
        }

        m_bIsOpen = false;
        m_bIsContinue = true;
        m_LoopThread->getLoop()->cancel(m_TimerIdTest);
        m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
        m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
        LOG(WARNING) << "m_pGameRoomInfo->serverStatus !=SERVER_RUNNING === " << m_pGameRoomInfo->serverStatus << "SERVER_RUNNING="<<SERVER_RUNNING;
        return;
    }
    // 向客户端发送游戏开始命令
    NormalGameStart();
    //开始启动一段时间内的相对自己的其他所有玩家的押注总额,发送给客户端
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    m_TimerOtherBetJetton = m_LoopThread->getLoop()->runEvery(m_fTMJettonBroadCard,boost::bind(&TableFrameSink::OnTimerSendOtherBetJetton, this));
    LOG(WARNING)<< "----------当前房间总人数  " << m_pITableFrame->GetPlayerCount(true);
    // 清除玩家标志
    m_bIsClearUser = false;

    // 服务器状态判断
    if (m_bIsContinue)
    {
        LOG(WARNING) << "--- 启动定时器 开始押分 CallBack GameTimerEnd---";
        m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
        // if (m_bTest>0)
        //     m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(0.05, boost::bind(&TableFrameSink::GameTimerEnd, this));
        // else
            m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(betCountDown, boost::bind(&TableFrameSink::GameTimerEnd, this));
    }
    else
    {
        clearUserIdVec.clear();
        clearChairIdVec.clear();
        for (auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
            if(!pUserItem || !userInfoItem->Isplaying)
                continue;
            pUserItem->setOffline();
            clearUserIdVec.push_back(userInfoItem->wUserid);
            clearChairIdVec.push_back(userInfoItem->wCharid);
        }
        uint32_t size = clearUserIdVec.size();
        for (int i = 0; i < size; ++i)
        {
            m_pITableFrame->ClearTableUser(clearChairIdVec[i], true, true, ERROR_ENTERROOM_SERVERSTOP);
            m_UserInfo.erase(clearUserIdVec[i]);
        }
    }
    m_bIsContinue = false;
}
void TableFrameSink::GameTimerEnd()
{
    LOG(WARNING) << "--- 游戏结束---" << __FUNCTION__;
    // 更新状态
    m_iGameState = Jsys::SUB_S_GameEnd;
    m_pITableFrame->SetGameStatus(Jsys::SUB_S_GameEnd);
    m_timetick = 0;
    float endCountDown = m_endCountDown + m_fGameEndAniTime; //1s,预留前端播放动画	
    m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    OnTimerSendOtherBetJetton();


    // muduo::MutexLockGuard lock(m_mutexLock);
    m_EapseTime = time(nullptr);

    // 结算计算
    NormalCalculate();
    m_bIsClearUser = true;
	float addTime = 0;
	if (m_iResultId == (int)AnimType::eSilverSharp)
		addTime = m_addCountDown_sd;
	else if (m_iResultId == (int)AnimType::eGoldSharp)
		addTime = m_addCountDown_sd + m_addCountDown_cj;

	endCountDown += addTime;
    // 跑马灯显示
    BulletinBoard();
    m_bIsContinue = m_pITableFrame->ConcludeGame(m_iGameState);
    LOG(WARNING) << "--- 游戏结束 m_bIsContinue is True?---" << m_bIsContinue;
    LOG(WARNING) << "--- 启动结算定时器---"; 
    // if (m_bTest>0)
    //     m_TimerIdStart = m_LoopThread->getLoop()->runAfter(0.05,CALLBACK_0(TableFrameSink::GameTimerStart, this));
    // else
		m_TimerIdStart = m_LoopThread->getLoop()->runAfter(endCountDown, CALLBACK_0(TableFrameSink::GameTimerStart, this));
        m_TimerIdResetPlayerList = m_LoopThread->getLoop()->runAfter(m_fResetPlayerTime, CALLBACK_0(TableFrameSink::RefreshPlayerList, this,true));
    // 更新配置的频率
    // if (m_bTest==0)
    {
        m_nReadCount++;
        //if (m_nReadCount >= 10)
        {
            ReadInI();
            m_nReadCount = 0;
        }
    }
    
    //统计本局的库存累加值
    double res = 0.0;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if(pUserItem->IsAndroidUser())
            continue;
        res -= userInfoItem->m_winAfterTax; 
    }
    LOG(INFO)<<"update Num----------------- "<<res;
    // if( res > 0)
    // {
    //     res = res*99/100; //- m_pITableFrame->CalculateRevenue(res);
    // }
    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update before----------------- "<<ceshi.lEndStorage;
    LOG(INFO)<<"update Num----------------- "<<res;
    if(res>0)
    {
        res=res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    m_pITableFrame->UpdateStorageScore(res);
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update after ----------------- "<<ceshi.lEndStorage;    
    
    // 重新更新库存
    ReadStorageInfo();

	openSLog("库存值:High Limit:%d,Lower Limit:%d,Stock:%d", m_llStorageHighLimit, m_llStorageLowerLimit, m_llStorageControl);
	openSLog("本局游戏结束;\n");
	// if (m_bTest > 0 && m_bTestAgain > 0)
	// {
	// 	if ((m_nTestType == 1 || m_nTestType == 2) && m_llStorageControl <= 5500000 * COIN_RATE) //放分
	// 	{
	// 		m_bTestAgain = false;
	// 		openSLog("=============库存下降为550W后再测试500局开始;");
	// 		m_nTestTimes = 500;
	// 		m_lOpenResultCount = 0;
	// 		for (int i = 0; i < BET_ITEM_COUNT; ++i)
	// 		{
	// 			m_lOpenResultIndexCount[i] = 0;
	// 		}
	// 	}
	// 	else if ((m_nTestType == 3 || m_nTestType == 4) && m_llStorageControl >= 5000000 * COIN_RATE) //收分
	// 	{
	// 		m_bTestAgain = false;
	// 		openSLog("=============库存上升到500W后再测试500局开始;");
	// 		m_nTestTimes = 500;
	// 		m_lOpenResultCount = 0;
	// 		for (int i = 0; i < BET_ITEM_COUNT; ++i)
	// 		{
	// 			m_lOpenResultIndexCount[i] = 0;
	// 		}
	// 	}
	// }
}
void TableFrameSink::CheckKickOutUser()
{
    vector<int> clearUserIdVec;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo){
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem)
            continue;
        if (pUserItem->IsAndroidUser())
            continue;      
        if (userInfoItem->NotBetJettonCount>=5)
        {
            pUserItem->SetUserStatus(sGetoutAtplaying);
            clearUserIdVec.push_back(userInfoItem->wUserid);            
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);            
            m_pITableFrame->ClearTableUser(userInfoItem->wCharid,true,true);
            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<userInfoItem->wCharid<<" userId="<<userInfoItem->wUserid;          
        } 
    }
    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_UserInfo.erase(clearUserIdVec[i]);
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
}
//测试游戏数据
void TableFrameSink::TestGameMessage(uint32_t wChairID,int type)
{
    // 游戏状态判断
    if (Jsys::SUB_S_GameStart != m_iGameState)
    {
       return;
    }
    int betTimes = 0;
    // 测试1-放分(单点下注); 测试2-放分(随机飞禽或走兽下注); 测试3-收分(单点下注); 测试4-收分(随机飞禽或走兽下注); 测试5-正常(单点下注); 测试6-正常(随机飞禽或走兽下注);
    // 测试7-保险1(随机飞禽或走兽下注);  测试8-保险1(随机飞禽或走兽下注);
    // 测试9-保险1(随机飞禽或走兽下注);  测试10-保险2(随机飞禽或走兽下注);
    int betAllScore[11] = {0,100,100,100,100,100,100,4500,5500,100,5000};
    int tmpbetAllScore = betAllScore[m_nTestType]*COIN_RATE;
    //每次下注到一个5倍的区域   
    int32_t bJettonArea = 0;//RandomInt(0, 3);
    if (m_nTestType>=7)
    {
        bJettonArea = RandomInt(10, 11);
    }
    do
    {                
        if (m_nTestType<7)
        {
            if (m_nTestType%2==1)
            {
                bJettonArea = RandomInt(0, 9);
            }
            else
            {
                bJettonArea = RandomInt(10, 11);
            }
        }
        
        //随机可下注金额
        int64_t JettonScore=0,canGold = 0,TotalWeightGold = 0;
        int randn = 0;
        int jsize = m_pGameRoomInfo->jettons.size();
        for (int i = jsize-1; i >= 0; --i)
        {
            if(m_pGameRoomInfo->jettons[i]>tmpbetAllScore) 
                continue;
            randn = i;//RandomInt(0, i);
            canGold = m_pGameRoomInfo->jettons[randn];
            tmpbetAllScore -= canGold;
            break; 
        }
        //随机下注筹码
        JettonScore = canGold;

        LOG(WARNING) << "----随机下注金额---" << JettonScore << " " << bJettonArea ;

        Jsys::CMD_C_PlaceJet mPlaceJet;
        mPlaceJet.set_cbjettonarea(bJettonArea);
        mPlaceJet.set_ljettonscore(JettonScore);

        string content = mPlaceJet.SerializeAsString();
        OnGameMessage(wChairID, Jsys::SUB_C_USER_JETTON, (uint8_t *)content.data(), content.size());
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
        case Jsys::SUB_C_USER_JETTON: //玩家押注
        {
            GamePlayerJetton(chairid, subid, data, datasize);
            break;
        }
        case Jsys::SUB_C_USER_ASKLIST:
        {
            SendPlayerList(chairid, subid, data, datasize); //排行榜
            break;
        }
        case Jsys::SUB_C_USER_REPEAT_JETTON:
        {
            Rejetton(chairid, subid, data, datasize); //续押
            break;
        }
        case Jsys::SUB_C_QUERY_PLAYERLIST: //查询玩家列表
        {
            GamePlayerQueryPlayerList(chairid, subid, data, datasize); //wan jia lie biao
            break;
        }
        case Jsys::SUB_C_QUERY_CUR_STATE: //获取当前状态
        {
            QueryCurState(chairid, subid, data, datasize);
            break;
        }
		case Jsys::SUB_C_QUERY_GAMERECORD: //100局获取开奖结果
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
    Jsys::CMD_C_CurState curStateReq;
    curStateReq.ParseFromArray(data, datasize);

    int32_t userid = curStateReq.dwuserid();

    int32_t elsTimeTest = ((int32_t)time(nullptr) - m_EapseTime);
    LOG(WARNING) << "**********查询当前游戏状态**********" << userid <<" "<<elsTimeTest;

    Jsys::CMD_S_CurState curStateRsp;

    int32_t iEapseTime = m_betCountDown + m_fGameStartAniTime;

    if(Jsys::SUB_S_GameStart != m_iGameState){
        iEapseTime = m_endCountDown;
		float addTime = 0;
		if (m_iResultId == (int)AnimType::eSilverSharp)
			addTime = m_addCountDown_sd;
		else if (m_iResultId == (int)AnimType::eGoldSharp)
			addTime = m_addCountDown_sd + m_addCountDown_cj;
		iEapseTime += addTime;
    }
    curStateRsp.set_ncurretidx(m_iStopIndex); 
    curStateRsp.set_ncurstate((int)(Jsys::SUB_S_GameStart != m_iGameState)); //当前状态
    curStateRsp.set_cbtimeleave(iEapseTime - elsTimeTest);
    LOG(WARNING) << "****" <<iEapseTime;

    int betCount = 0;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || userInfoItem->wCharid != chairid || !userInfoItem->Isplaying)
            continue;
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            betCount += userInfoItem->dAreaJetton[j];
            curStateRsp.add_selfjettonscore(userInfoItem->dAreaJetton[j]);
            curStateRsp.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
            curStateRsp.add_mutical(m_nMuilt[j]);
        } 
    }
    curStateRsp.set_nlastretid(m_lastRetId);

    //记录
	int fqWin = 0;
	int zsWin = 0;
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--){
        curStateRsp.add_cartype(m_vResultRecored[i]);
        // LOG(WARNING) << "Recored =>" << i << " "<< m_vResultRecored[i];
        if (m_vResultRecored[i] < (int)AnimType::eSilverSharp)
		{
			if (m_vResultRecored[i] % 2 == 1)
				fqWin++;
			else
				zsWin++;
		}
    }
	curStateRsp.set_lfeiqincount(fqWin);
	curStateRsp.set_lzoushoucount(zsWin);

    LOG(ERROR) << "查询当前游戏状态 =>UserScore=" << player->GetUserScore() << ",betCount= "<< betCount;
    if(Jsys::SUB_S_GameStart == m_iGameState)
    {
        curStateRsp.set_luserscore(player->GetUserScore() - betCount); 
    }
    else
    {
        curStateRsp.set_luserscore(player->GetUserScore()); 
		if (m_iResultId >= (int)AnimType::eSilverSharp)
		{
			curStateRsp.set_cartype_sd(m_iResultId_sd);
			curStateRsp.set_winindex_sd(m_iStopIndex_sd);
			if (m_iResultId >= (int)AnimType::eGoldSharp)
			{
				int32_t dAreaJetton = 1;//m_UserInfo[userid]->dAreaJetton[8];
				curStateRsp.set_caijinscore(m_iResultMul_cj*dAreaJetton);
			}
		}
    }
    curStateRsp.set_userwinscore(m_UserInfo[player->GetUserId()]->RealMoney);

	vector<shared_ptr<OpenResultInfo>> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();
	int notOpenSilverSharp = 0;
	int notOpenGoldSharp = 0;
	for (int i = 0;i < BET_ITEM_COUNT; ++i)
	{
		vResultList = m_vOpenResultListCount_last[i];
		if (openCount == 0)
			curStateRsp.add_notopencount(0);
		else
		{
			curStateRsp.add_notopencount(vResultList[openCount - 1]->notOpenCount);
			if (i == (int)AnimType::eSilverSharp)
			{
				notOpenSilverSharp = vResultList[openCount - 1]->notOpenCount;
			}
			else if (i == (int)AnimType::eGoldSharp)
			{
				notOpenGoldSharp = vResultList[openCount - 1]->notOpenCount;
				if (notOpenGoldSharp >= notOpenSilverSharp)
				{
					curStateRsp.set_notopencount(i, notOpenSilverSharp);
				}
				else
				{
					curStateRsp.set_notopencount(i - 1, notOpenGoldSharp);
				}
			}
		}
	}

    std::string sendData = curStateRsp.SerializeAsString();
    m_pITableFrame->SendTableData(chairid, Jsys::SUB_S_QUERY_CUR_STATE, (uint8_t *)sendData.c_str(), sendData.size());
}

// 100局游戏记录
void TableFrameSink::QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(chairid);
	if (!player)
		return;
	// if (m_bTest>0)
	// {
	//     return;
	// }
	Jsys::CMD_C_GameRecord gameRecordReq;
	gameRecordReq.ParseFromArray(data, datasize);

	int32_t userid = gameRecordReq.dwuserid();

	/*int32_t elsTimeTest = ((int32_t)time(nullptr) - m_EapseTime);
	LOG(WARNING) << "**********查询当前游戏状态**********" << userid << " " << elsTimeTest;

	int32_t iEapseTime = m_betCountDown;

	if (Jsys::SUB_S_GameStart != m_iGameState) {
		iEapseTime = m_endCountDown;
	}*/
	
	sendGameRecord(chairid);
	
}

// 写玩家分
void TableFrameSink::WriteUserScore()
{ 
    vector<tagScoreInfo> infoVec;
    infoVec.clear(); 

    string strRet = str(boost::format("%d-%d") % m_iResultId % m_iRetMul);
	int winAllMul = m_iRetMul;
    LOG(WARNING) << "写玩家分 =>" << strRet; 
    m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime,strRet,-1,opShowCard,-1,-1);

	if (m_iResultId >= (int)AnimType::eSilverSharp)
	{		
		//开鲨鱼赠送的动物
		string strRet2 = str(boost::format("%d-%d") % m_iResultId_sd % m_nMuilt[m_iResultId_sd]);
		winAllMul += m_nMuilt[m_iResultId_sd];
		m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime, strRet2, -1, opUnkonw0, -1, -1);
		if (m_iResultId == (int)AnimType::eGoldSharp)
		{
			//开金鲨赠送的彩金倍数
			strRet2 = str(boost::format("9-%d") % m_iResultMul_cj);
			winAllMul += m_iResultMul_cj;
			m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime, strRet2, -1, opUnkonw1, -1, -1);
		}
		strRet = str(boost::format("%d-%d") % m_iResultId % winAllMul);
	}

    bool isNeedRecord = false;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

	int64_t allBetScore = 0;
	int64_t userid = 0;
	m_replay.detailsData = "";
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if (pUserItem->IsAndroidUser()) 
            continue; 
        if ( userInfoItem->dCurrentPlayerJetton == 0) 
            continue;

		allBetScore = 0;
		userid = pUserItem->GetUserId();
        tagScoreInfo scoreInfo; 
        for(int betIdx = 0;betIdx < BET_ITEM_COUNT; betIdx++){  
			if (betIdx == (int)AnimType::eGoldSharp) 
				continue;
            // 结算时记录
            int64_t addScore = userInfoItem->dAreaWin[betIdx];
            int64_t betScore = userInfoItem->dAreaJetton[betIdx]; 
            // 押分情况
            scoreInfo.cellScore.push_back(betScore);
            //牌型
            if(betScore > 0){
                m_replay.addResult(userInfoItem->wCharid, betIdx, betScore, addScore, strRet, false);
            }
			allBetScore += betScore;
        }
		if (allBetScore > 0)
		{
			//对局记录详情
			SetGameRecordDetail(userid, userInfoItem->wCharid, pUserItem->GetUserScore(), userInfoItem);
		}
        LOG(WARNING) << "结算时记录 =>";

        scoreInfo.chairId = userInfoItem->wCharid;
        scoreInfo.addScore = userInfoItem->m_winAfterTax; 
        scoreInfo.revenue = userInfoItem->m_userTax; 
        scoreInfo.betScore = userInfoItem->dCurrentPlayerJetton;
        scoreInfo.rWinScore = userInfoItem->dCurrentPlayerJetton; //
        //牌型
        scoreInfo.cardValue = strRet;// str(boost::format("%d-%d;") % m_iResultId % m_iRetMul);
        scoreInfo.startTime = m_startTime;
        infoVec.push_back(scoreInfo);

        isNeedRecord = true;
        // 结算时记录 
        // m_replay.addResult(userInfoItem->wCharid, userInfoItem->wCharid, scoreInfo.betScore, scoreInfo.addScore, scoreInfo.cardValue, false);
    }
    //保存对局结果
    if(isNeedRecord)
        m_pITableFrame->SaveReplay(m_replay);

    //先关掉写分
    m_pITableFrame->WriteUserScore(&infoVec[0], infoVec.size(), m_strRoundId);
}
/*
* 位置是15-16-17是兔子 - 雷克萨斯， 0
* 位置是19-20-21是燕子 -  大众 ，1
* 位置是12-13是猴子 - 奔驰，2
* 位置是23-24是鸽子 - 宝马，3
* 位置是9-10是熊猫 - 保时捷，4
* 位置是26-27是孔雀 - 马萨拉蒂，5
* 位置5-6-7是狮子 - 兰博基尼，6
* 位置1-2-3是老鹰 - 法拉利，7
* 位置0-8-14-22是银鲨，8
* 位置4-11-18-25是金鲨，9
*/
void TableFrameSink::ReadInI()
{
    CINIHelp *pini = new CINIHelp("conf/jsys_config.ini");

    int32_t odds[BET_ITEM_COUNT] = {6,8,8,12,12,8,8,6,24,99,2,2};//{6,8,8,12,12,8,8,6,25,50,2,2};
    int32_t weight[BET_ITEM_COUNT] = { 1625,1625,1218,1218,1218,1218,812,812,170,84,0,0 }; //{1615,1615,1211,1211,1076,1076,807,807,388,194,0,0};

    int32_t nAreaMaxJetton[BET_ITEM_COUNT] = { 0 };
	int32_t retWeight[BET_ITEM_COUNT] = {0};
    int32_t nMuilt[BET_ITEM_COUNT] = { 0 };// 倍率表
    memcpy(nAreaMaxJetton, m_nAreaMaxJetton, sizeof(nAreaMaxJetton));    
    memcpy(retWeight, m_retWeight, sizeof(retWeight));
    memcpy(nMuilt, m_nMuilt, sizeof(nMuilt));

    m_dControlkill = pini->GetFloat("GAME_CONFIG", "Controlkill", 0.3);

    m_lLimitPoints = pini->GetLong("GAME_CONFIG", "LimitPoint", 500000);
    char buf[32] = { 0 };
    for(int i = 0;i<BET_ITEM_COUNT;i++){
        // 赔率
        sprintf(buf, "AREA_%d",i);
        m_nMuilt[i] = pini->GetLong("GAME_MULTIPLIER", buf, odds[i]);
        assert(m_nMuilt[i]>0);
        if(m_nMuilt[i]==0)
            m_nMuilt[i] = nMuilt[i];
        // 押分区最大押分值
        sprintf(buf, "MaxJetton%d",i);
        m_nAreaMaxJetton[i] =  pini->GetLong("GAME_AREA_MAX_JETTON", buf, 10000);
        LOG(WARNING) << "m_nAreaMaxJetton[i] "<<m_nAreaMaxJetton[i];
        assert(m_nAreaMaxJetton[i]>0);
        if(m_nAreaMaxJetton[i]==0)
            m_nAreaMaxJetton[i] = nAreaMaxJetton[i];
        // 开奖权重 
        sprintf(buf, "Ret%d",i);
        m_retWeight[i] = pini->GetLong("GAME_RET", buf, weight[i]);
        if(m_retWeight[i]==0)
            m_retWeight[i] = retWeight[i];

        LOG(WARNING) << "--- ReadInI--- " << odds[i]<<" "<< m_nMuilt[i] <<" "<< m_nAreaMaxJetton[i] <<" "<< weight[i]<<" "<<m_retWeight[i];
    }

    stockWeak = pini->GetFloat("GAME_CONFIG", "STOCK_WEAK", 1.0);
    // 倒计时时间
    m_betCountDown = pini->GetLong("GAME_STATE_TIME", "TM_GAME_START", 20);
    m_endCountDown = pini->GetLong("GAME_STATE_TIME", "TM_GAME_END", 10);
	m_addCountDown_sd = pini->GetFloat("GAME_STATE_TIME", "TM_GAME_ADD_SD", 2.0);
	m_addCountDown_cj = pini->GetFloat("GAME_STATE_TIME", "TM_GAME_ADD_CJ", 2.0);

	m_fGameStartAniTime = pini->GetFloat("GAME_STATE_TIME", "TM_GAME_START_ANI", 1.0);
	m_fGameEndAniTime = pini->GetFloat("GAME_STATE_TIME", "TM_GAME_END_ANI", 1.0);

	m_fResetPlayerTime = pini->GetFloat("GAME_STATE_TIME", "TM_RESET_PLAYER_LIST", 5.0);

    // 控制概率
    m_ctrlRate         =(pini->GetLong("GAME_STATE_TIME", "CTRL_RATE", 50) * 1.0)/100.0;
    m_stockPercentRate =(pini->GetLong("GAME_STATE_TIME", "STOCK_CTRL_RATE", 50) * 1.0)/100.0;

    m_bIsIncludeAndroid = pini->GetLong("GAME_CONFIG", "USER_ANDROID_DATA_TEST_AISC", 0);
    m_MarqueeScore = pini->GetLong("GAME_CONFIG", "PAOMADENG_CONDITION_SET", 1000);
    m_fTaxRate = pini->GetFloat("GAME_CONFIG", "REVENUE", 0.025);
    m_fTMJettonBroadCard = pini->GetFloat("GAME_CONFIG", "TM_JETTON_BROADCAST", 0.5);
	// 
    m_testRet = pini->GetFloat("TEST", "TestRet", 0);
    m_testResultInex = pini->GetFloat("TEST", "TestIndex", -1);
	m_testResultInex_sd = pini->GetFloat("TEST", "TestIndex_SD", -1);

	// m_bTest = pini->GetLong("GAME_CONFIG", "TestGame", 0);
 //    m_nTestTimes = pini->GetLong("GAME_CONFIG", "TestGameTimes", 0);
 //    m_nTestType = pini->GetLong("GAME_CONFIG", "nTestType", 3);
    // 是否写日记
    m_bWritelog = false;
    if (pini->GetLong("GAME_CONFIG", "nWritelog", 0) != 0)
    {
        m_bWritelog = true;
    }
	//开金鲨是的倍数和彩金倍数
	m_ctrlGradePercentRate[0] = pini->GetLong("GAME_CONFIG", "GradePercentRate0", 600);
	m_ctrlGradePercentRate[1] = pini->GetLong("GAME_CONFIG", "GradePercentRate1", 900);
	m_ctrlGradeMul[0] = pini->GetLong("GAME_CONFIG", "GradeMul_JS", 50);
	m_ctrlGradeMul[1] = pini->GetLong("GAME_CONFIG", "GradeMul_CJ", 25);

	LOG(WARNING) << "--- ReadInI--- 控制比例：" << m_ctrlGradePercentRate[0] << " " << m_ctrlGradePercentRate[1] << "，金鲨低倍: " << m_ctrlGradeMul[0] << "，彩金低倍:  " << m_ctrlGradeMul[1];
    openSLog("ReadInI,控制比例：%d,%d,金鲨低倍:%d,彩金低倍:%d", m_ctrlGradePercentRate[0], m_ctrlGradePercentRate[1], m_ctrlGradeMul[0], m_ctrlGradeMul[1]);

    m_useIntelligentChips=pini->GetLong("CHIP_CONFIGURATION", "useintelligentchips", 1) ;

    if(m_useIntelligentChips)
    {
        for(int i=0;i<6;i++)
        {
            char buf[100];
            sprintf(buf, "chipgear1_%d",i+1);
            m_userChips[0][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 100) ;

            sprintf(buf, "chipgear2_%d",i+1);
            m_userChips[1][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 100) ;

            sprintf(buf, "chipgear3_%d",i+1);
            m_userChips[2][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 100) ;

            sprintf(buf, "chipgear4_%d",i+1);
            m_userChips[3][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 100) ;
        }

        for(int i=0;i<4;i++)
        {
            char buf[100];
            sprintf(buf, "scorelevel%d",i+1);
            m_userScoreLevel[i]=pini->GetLong("CHIP_CONFIGURATION", buf, 100) ;
        }
    }
    else
    {


        for(int i=0;i<6;i++)
        {
            char buf[100];
            sprintf(buf, "chipgear4_%d",i+1);
            m_userChips[0][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 1000) ;
            m_userChips[1][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 1000) ;
            m_userChips[2][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 1000) ;
            m_userChips[3][i]=pini->GetLong("CHIP_CONFIGURATION", buf, 1000) ;
        }
    }

    m_cbJettonMultiple.clear();
    for(int i=0;i<6;i++)
    {
        m_cbJettonMultiple.push_back(m_userChips[0][i]);
    }
    for(int i=0;i<6;i++)
    {
        vector<int>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),m_userChips[1][i]);
        if(it!=m_cbJettonMultiple.end())
        {

        }
        else
        {
            m_cbJettonMultiple.push_back(m_userChips[1][i]);
        }
    }
    for(int i=0;i<6;i++)
    {
        vector<int>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),m_userChips[2][i]);
        if(it!=m_cbJettonMultiple.end())
        {

        }
        else
        {
            m_cbJettonMultiple.push_back(m_userChips[2][i]);
        }
    }
    for(int i=0;i<6;i++)
    {
        vector<int>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),m_userChips[3][i]);
        if(it!=m_cbJettonMultiple.end())
        {

        }
        else
        {
            m_cbJettonMultiple.push_back(m_userChips[3][i]);
        }
    }
	sort(m_cbJettonMultiple.begin(), m_cbJettonMultiple.end());
    delete pini;
}

// 发送开始游戏命令
void TableFrameSink::NormalGameStart()
{
    LOG(WARNING) << "--- 开始游戏 2---" << __FUNCTION__;
    openSLog("本局游戏开始;");
	getJSodds();
    // if (m_bTest>0)
    // {
    //     return;
    // }	
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<IServerUserItem> pServerUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;

        Jsys::CMD_S_GameStart gameStart;
        gameStart.set_cbplacetime(m_betCountDown);
        gameStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            gameStart.add_selfjettonscore(userInfoItem->dAreaJetton[j]);
            gameStart.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
            gameStart.add_mutical(m_nMuilt[j]);
        }
        if (!pUserItem->IsAndroidUser())
        {
            LOG(WARNING) << "--------------------GetUserScore()==" <<pUserItem->GetUserId() <<" "<< pUserItem->GetUserScore();
        }
        // 
        gameStart.set_cbroundid(m_strRoundId);
        gameStart.set_luserscore(pUserItem->GetUserScore());
        gameStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
		
		/*gameStart.set_lfeiqincount(fqWin);
		gameStart.set_lzoushoucount(zsWin);*/
		i = 0;
		for (auto &uInfoItem : m_pPlayerInfoList)
		{
			xzScore = 0;
			pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
			if (!pServerUserItem)
				nowScore = 0;
			else
			{
                for (int j = 0; j < BET_ITEM_COUNT; j++)
				{
					xzScore += uInfoItem->dAreaJetton[j];
				}
				nowScore = pServerUserItem->GetUserScore() - xzScore;
			}
			/*if (userInfoItem->chairId == m_wBankerUser)
			{
				continue;
			}*/
			if (uInfoItem->headerId > 12)
			{
				LOG_INFO << " ========== 5 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
			}
            Jsys::PlayerListItem* palyer = gameStart.add_players();
            palyer->set_dwuserid(uInfoItem->wUserid);
			palyer->set_headerid(uInfoItem->headerId);
			palyer->set_nviplevel(0);
			palyer->set_nickname(uInfoItem->nickName);

			palyer->set_luserscore(nowScore);

			palyer->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
			palyer->set_ltwentywincount(uInfoItem->lTwentyWinCount);
            int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
			palyer->set_isdivinemath(shensuanzi);
			palyer->set_index(i + 1);
			if (shensuanzi == 1)
				palyer->set_index(0);

			palyer->set_isbanker(0);
			//if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
			//{
			   // palyer->set_isbanker(1);
			//}
			palyer->set_gender(0);
			palyer->set_isapplybanker(0);
			palyer->set_applybankermulti(0);
			palyer->set_ljetton(0);
			palyer->set_szlocation(uInfoItem->location);
			palyer->set_headboxid(0);
			palyer->set_luserwinscore(uInfoItem->RealMoney);

			if (++i >= size)
				break;
		}
		
		vector<shared_ptr<OpenResultInfo>> vResultList;
		vResultList = m_vOpenResultListCount_last[0];
		int32_t openCount = vResultList.size();
		int notOpenSilverSharp = 0;
		int notOpenGoldSharp = 0;
		for (int i = 0;i < BET_ITEM_COUNT; ++i)
		{
			vResultList = m_vOpenResultListCount_last[i];
			if (openCount == 0)
				gameStart.add_notopencount(0);
			else
			{
				gameStart.add_notopencount(vResultList[openCount - 1]->notOpenCount);
				if (i == (int)AnimType::eSilverSharp)
				{
					notOpenSilverSharp = vResultList[openCount - 1]->notOpenCount;
				} 
				else if(i == (int)AnimType::eGoldSharp)
				{
					notOpenGoldSharp = vResultList[openCount - 1]->notOpenCount;
					if (notOpenGoldSharp >= notOpenSilverSharp)
					{
						gameStart.set_notopencount(i, notOpenSilverSharp);
					}
					else
					{
						gameStart.set_notopencount(i-1, notOpenGoldSharp);
					}
				}
			}				
		}

        std::string endData = gameStart.SerializeAsString();
        m_pITableFrame->SendTableData(pUserItem->GetChairId(), Jsys::SUB_S_GameStart, (uint8_t *)endData.c_str(), endData.size());
    }
}

// 跑马灯显示
void TableFrameSink::BulletinBoard()
{
    int32_t winscore = 0;
    int idex = 0;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        // 查找到最大得分玩家获奖分数
        // if (userInfoItem->RealMoney > winscore)
        // {
        //     winscore = userInfoItem->RealMoney;
        //     idex = userInfoItem->wCharid;
        // }
        if (userInfoItem->RealMoney >= m_pGameRoomInfo->broadcastScore)
        {
            int sgType = SMT_GLOBAL | SMT_SCROLL;
            m_pITableFrame->SendGameMessage(userInfoItem->wCharid, "", sgType, userInfoItem->RealMoney);
            LOG(INFO) << "广播玩家 "<< userInfoItem->wCharid << " 获奖分数 " <<  userInfoItem->RealMoney;
        }
    }
    // m_MarqueeScore = m_pGameRoomInfo->broadcastScore;
    // string Huode = "";
    // if (winscore >= m_MarqueeScore)
    // {
    //     int sgType = SMT_GLOBAL | SMT_SCROLL;
    //     m_pITableFrame->SendGameMessage(idex, Huode, sgType, winscore);
    // }
}

// 重置桌子数据(记录重押数据)
void TableFrameSink::ResetTableDate()
{
    LOG(WARNING) << "重置桌子数据" << __FUNCTION__;

    m_iRetMul = 0;
    // m_iResultId = 0;
    // m_iStopIndex = 0;
    m_BestPlayerUserId = 0;
    m_BestPlayerChairId = 0;
    m_BestPlayerWinScore = 0.0;

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem)
            continue;
        if(userInfoItem->dCurrentPlayerJetton == 0)
        {
            userInfoItem->clear();
            continue;
        }
        // 否则把重押分数记录
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            userInfoItem->dLastJetton[j] = userInfoItem->dAreaJetton[j];
        }

        userInfoItem->clear();
    }

    m_pBetArea->ThisTurnClear();
}
void TableFrameSink::Algorithm()
{
    int  isHaveBlackPlayer=0;

	
    float cotrolWeight[BET_ITEM_COUNT]={0};
    for(int i=0;i<BET_ITEM_COUNT;i++)
        cotrolWeight[i]=m_retWeight[i];

    for(int i=0;i<MAX_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton=0;
        for(int i=0;i<BET_ITEM_COUNT;i++)
        {
            allJetton+=m_UserInfo[user->GetUserId()]->dAreaJetton[i];
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
                    openSLog("黑名单:%d房间控制系数读到 :%d",user->GetUserId(),isHaveBlackPlayer);
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
	jsysAlgorithm.SetSendOdds(m_iResultId_sd, m_iResultMul_cj);
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {

        jsysAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill,m_nMuilt, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
        jsysAlgorithm.SetMustKill(-1);
        for(int i=0;i<MAX_JETTON_AREA;i++)
         jsysAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
        jsysAlgorithm.GetOpenCard(m_iResultId, cotrolWeight);
        openSLog("全局难度,正常算法最终出结果: %d",m_iResultId);
    }
    else
    {
        if(isHaveBlackPlayer)
        {
            float probilityAll[10]={0.0f};
            //user probilty count
            for(int i=0;i<MAX_PLAYER;i++)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(!user||user->IsAndroidUser())
                {
                    continue;
                }
                int64_t allJetton=0;
                for(int j=0;j<BET_ITEM_COUNT;j++)
                {
                    allJetton+=m_UserInfo[user->GetUserId()]->dAreaJetton[j];
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
                   for(int j=0;j<MAX_JETTON_AREA-2;j++)
                   {
                       if(allWinscore>0)
                       {
                           probilityAll[j]+=(gailv*(float)m_UserInfo[user->GetUserId()]->dAreaJetton[j]*m_nMuilt[j])/(float)allWinscore;
                           openSLog("黑名单玩家 :%d 确定调用黑名单算法 每门传的黑名单控制概率是[%d]：%f",user->GetUserId(),j,(gailv*(float)m_UserInfo[user->GetUserId()]->dAreaJetton[j]*m_nMuilt[j])/(float)allWinscore);
                       }

                   }

                }
            }

            int64_t allWin=0;
            for(int j=0;j<MAX_JETTON_AREA;j++)
            {
                allWin+=m_nMuilt[j]*m_pBetArea->AndroidmifourAreajetton[j];
            }
            float feijipro=m_pBetArea->AndroidmifourAreajetton[10]/4.0/allWin;
            float zoushoupro=m_pBetArea->AndroidmifourAreajetton[11]/4.0/allWin;
            for(int i=0;i<MAX_JETTON_AREA-4;i++)
            {
                if(i%2==0)
                {
                    probilityAll[i]+=feijipro;
                }
                else
                {
                     probilityAll[i]+=zoushoupro;
                }
            }
            for(int i=0;i<MAX_JETTON_AREA;i++)
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

            jsysAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill,m_nMuilt, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<MAX_JETTON_AREA;i++)
             jsysAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            jsysAlgorithm.BlackGetOpenCard(m_iResultId,cotrolWeight);
            openSLog("黑名单算法最终出结果: %d",m_iResultId);
        }
        else
        {
            jsysAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill,m_nMuilt, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<MAX_JETTON_AREA;i++)
             jsysAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            jsysAlgorithm.GetOpenCard(m_iResultId, cotrolWeight);
            openSLog("正常算法最终出结果: %d",m_iResultId);
        }

    }

	//测试开奖结果
	if (rand() % 100 < m_testRet)
	{
		if (m_testResultInex >= 0 && m_testResultInex <= (int)AnimType::eGoldSharp)
		{
			m_testInex = m_testResultInex;
		}
		else
		{
			m_testInex++;
			if (m_testInex > (int)AnimType::eGoldSharp)
			{
				m_testInex = 0;
			}
		}
		m_iResultId = m_testInex;
		if (m_iResultId >= (int)AnimType::eSilverSharp && m_iResultId <= (int)AnimType::eGoldSharp)
		{
			if (m_testResultInex_sd >= 0 && m_testResultInex_sd <= (int)AnimType::eGoldSharp)
			{
				m_testInex_sd = m_testResultInex_sd;
			}
			else
			{
				m_testInex_sd++;
				if (m_testInex_sd >= (int)AnimType::eSilverSharp)
				{
					m_testInex_sd = 0;
				}				
			}
			m_iResultId_sd = m_testInex_sd;
		}
	}

    //转换
    string strm = "";
    switch (m_iResultId)
    {
        case (int)AnimType::eRabbit:  { m_iStopIndex = 15 + RandomInt(0, 2) ; LOG(WARNING) << "兔子"; strm = "兔子";}break;
        case (int)AnimType::eSwallow:  { m_iStopIndex = 19 + RandomInt(0, 2) ;LOG(WARNING) << "燕子"; strm = "燕子"; }break;
        case (int)AnimType::eMonkey:  { m_iStopIndex = 12 + RandomInt(0, 1) ; LOG(WARNING) << "猴子"; strm = "猴子";}break;
        case (int)AnimType::ePigeon:  { m_iStopIndex = 23 + RandomInt(0, 1) ; LOG(WARNING) << "鸽子"; strm = "鸽子";}break;
        case (int)AnimType::ePanda:  { m_iStopIndex = 9 + RandomInt(0, 1) ; LOG(WARNING) << "熊猫"; strm = "熊猫";}break;
        case (int)AnimType::ePeacock:  { m_iStopIndex = 26 + RandomInt(0, 1) ; LOG(WARNING) << "孔雀"; strm = "孔雀";}break;
        case (int)AnimType::eLion:  { m_iStopIndex = 5 + RandomInt(0, 2) ; LOG(WARNING) << "狮子"; strm = "狮子";}break;
        case (int)AnimType::eEagle:  { m_iStopIndex = 1 + RandomInt(0, 2) ; LOG(WARNING) << "老鹰"; strm = "老鹰";}break;
        case (int)AnimType::eSilverSharp:  { //位置0-8-14-22是银鲨
            int pos[4] = {0,8,14,22};
            m_iStopIndex = pos[RandomInt(0, 3)]; LOG(WARNING) << "银鲨"; strm = "银鲨"; }break;
        case (int)AnimType::eGoldSharp:  { //位置4-11-18-25是金鲨
            int pos[4] = {4,11,18,25};
            m_iStopIndex =  pos[RandomInt(0, 3)]; LOG(WARNING) << "金鲨"; strm = "金鲨"; }break;
    default:
        break;
    }

    // 倍率
    m_iRetMul = m_nMuilt[m_iResultId];
    // if (m_bTest>0)
        m_lOpenResultCount++;
    m_lOpenResultIndexCount[m_iResultId]++;
    if (m_iResultId<8)
    {
        if (m_iResultId%2==0)
        {
            m_lOpenResultIndexCount[10]++; //走兽
        }
        else
        {
            m_lOpenResultIndexCount[11]++; //飞禽
        }
    }

    LOG(WARNING) << "开奖倍率 " << m_iRetMul << " 开奖结果 " << m_iResultId << " 开奖下标 " << m_iStopIndex << " 累计开奖次数 " << m_lOpenResultCount;
    if (m_iResultId<8)
    {
        openSLog("本局开奖结果:类型:%d,%s,%s,累计开奖次数:%d;",m_iResultId, m_iResultId%2==0 ? "走兽":"飞禽" ,strm.c_str(),m_lOpenResultCount);
    }
    else
    {
        openSLog("本局开奖结果:类型:%d,%s,累计开奖次数:%d;",m_iResultId,strm.c_str(),m_lOpenResultCount);
    }

    string openStr = "开奖动物累计:";
    for (int i = 0; i < BET_ITEM_COUNT; ++i)
    {
        openStr += to_string(i) + "_" + to_string(m_lOpenResultIndexCount[i])+ "  ";
    }
    LOG(WARNING) << openStr ;
    openSLog("%s;",openStr.c_str());
    if (m_lOpenResultCount>100000 || m_lOpenResultIndexCount[m_iResultId]>10000000)
    {
        m_lOpenResultCount = 0;
        memset(m_lOpenResultIndexCount,0,sizeof(m_lOpenResultIndexCount));
    }
}
// 获取开奖结果
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
    LOG(WARNING) << "允许最大输分值 maxScore " << maxScore  << " m_stockPercentRate "<< m_stockPercentRate <<" curStor " << fcurStor << " lowStor " << flowStor;
    // 统计总押分
    int32_t tempAllBet = 0;
    for (int index = 0; index < BET_ITEM_COUNT; index++)
        tempAllBet += m_pBetArea->AndroidmifourAreajetton[index];

    // 筛选出结果
    string winStr=" 系统可赢选项:",loseStr=" 系统输选项:";
    for (int index = 0; index < BET_ITEM_COUNT-2; index++)
    {
        // 押分 *  倍数 < 允许最大输分值
        int32_t tempItemWin = m_pBetArea->AndroidmifourAreajetton[index] * (m_nMuilt[index]);
        if (index < (int)AnimType::eSilverSharp)
        {
            if (index%2==0) //走兽
            {
                tempItemWin += m_pBetArea->AndroidmifourAreajetton[10]*(m_nMuilt[10]);
                tempItemWin -= m_pBetArea->AndroidmifourAreajetton[11];
            }
            else
            {
                tempItemWin += m_pBetArea->AndroidmifourAreajetton[11]*(m_nMuilt[11]);
                tempItemWin -= m_pBetArea->AndroidmifourAreajetton[10];
            }
        }
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
        if(minfloat >  1.0) minfloat = 1.0;
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
    m_iResultId = finalResultIdx;
    // 没有控制结果,则随机，但要符合合理结果
    if (tempAllBet > 0 && allSize > 0 && finalResultIdx == -1)
    {
        int32_t TotalWeight = 0;
        for (int i = 0; i < BET_ITEM_COUNT-2; ++i)
            TotalWeight += m_retWeight[i];

        assert(TotalWeight > 0);

        // 最多随机500局
        int32_t loopCount = 500;
        bool loopEnd = false;
        int index[10] = {0,1,2,3,4,5,6,7,8,9};
        // std::shuffle(&index[0], &index[BET_ITEM_COUNT-2], STD::Generator::instance().get_mt());
        // string indexStr="随机取权重顺序:";
        // for (int i = 0; i < BET_ITEM_COUNT-2; ++i)
        // {
        //     indexStr += to_string(index[i]);
        // }
        // LOG(WARNING) << indexStr;
        do
        {
            int32_t tempResIdx = -1;
            /* code */
            if(--loopCount <= 0){
                m_iResultId = tempAllRetIdx[RandomInt(0, allSize-1)]; 
                LOG(ERROR) << "随机500局没有找到结果 allSize" << allSize << " m_iResultId "<< m_iResultId;
                break;
            }

            /* code */
            int tempweightsize = RandomInt(0, TotalWeight - 1);
            int tempweightsize2 = tempweightsize;
            for (int i = 0; i < BET_ITEM_COUNT-2; i++)
            {
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
                        m_iResultId = tempResIdx;
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
        for (int i = 0; i < BET_ITEM_COUNT-2; ++i)
            TotalWeight += m_retWeight[i];

        assert(TotalWeight > 0);

        int tempweightsize = RandomInt(0, TotalWeight - 1);
        for (int i = 0; i < BET_ITEM_COUNT-2; i++)
        {
            tempweightsize -= m_retWeight[i];
            if (tempweightsize < 0)
            {
                m_iResultId = i;
                break;
            }
        }
    }
    //测试开奖结果
    if (rand()%100 < m_testRet)
    {
        if (m_testResultInex>=0 && m_testResultInex<=(int)AnimType::eGoldSharp)
        {
            m_testInex = m_testResultInex;
        }
        else
        {
            m_testInex++;
            if (m_testInex>(int)AnimType::eGoldSharp)
            {
                m_testInex=0;
            }
        }        
        m_iResultId = m_testInex;
    }
    //转换
    string strm = "";
    switch (m_iResultId)
    {
        case (int)AnimType::eRabbit:  { m_iStopIndex = 15 + RandomInt(0, 2) ; LOG(WARNING) << "兔子"; strm = "兔子";}break;
        case (int)AnimType::eSwallow:  { m_iStopIndex = 19 + RandomInt(0, 2) ;LOG(WARNING) << "燕子"; strm = "燕子"; }break;
        case (int)AnimType::eMonkey:  { m_iStopIndex = 12 + RandomInt(0, 1) ; LOG(WARNING) << "猴子"; strm = "猴子";}break;
        case (int)AnimType::ePigeon:  { m_iStopIndex = 23 + RandomInt(0, 1) ; LOG(WARNING) << "鸽子"; strm = "鸽子";}break;
        case (int)AnimType::ePanda:  { m_iStopIndex = 9 + RandomInt(0, 1) ; LOG(WARNING) << "熊猫"; strm = "熊猫";}break;
        case (int)AnimType::ePeacock:  { m_iStopIndex = 26 + RandomInt(0, 1) ; LOG(WARNING) << "孔雀"; strm = "孔雀";}break;
        case (int)AnimType::eLion:  { m_iStopIndex = 5 + RandomInt(0, 2) ; LOG(WARNING) << "狮子"; strm = "狮子";}break;
        case (int)AnimType::eEagle:  { m_iStopIndex = 1 + RandomInt(0, 2) ; LOG(WARNING) << "老鹰"; strm = "老鹰";}break;
        case (int)AnimType::eSilverSharp:  { //位置0-8-14-22是银鲨
            int pos[4] = {0,8,14,22};
            m_iStopIndex = pos[RandomInt(0, 3)]; LOG(WARNING) << "银鲨"; strm = "银鲨"; }break;
        case (int)AnimType::eGoldSharp:  { //位置4-11-18-25是金鲨
            int pos[4] = {4,11,18,25};
            m_iStopIndex =  pos[RandomInt(0, 3)]; LOG(WARNING) << "金鲨"; strm = "金鲨"; }break;
    default:
        break;
    }

    // 倍率
    m_iRetMul = m_nMuilt[m_iResultId];
    // if (m_bTest>0)
        m_lOpenResultCount++;
    m_lOpenResultIndexCount[m_iResultId]++;
    if (m_iResultId<8)
    {
        if (m_iResultId%2==0)
        {
            m_lOpenResultIndexCount[10]++; //走兽
        }
        else
        {
            m_lOpenResultIndexCount[11]++; //飞禽
        }
    }
    
    LOG(WARNING) << "开奖倍率 " << m_iRetMul << " 开奖结果 " << m_iResultId << " 开奖下标 " << m_iStopIndex << " 累计开奖次数 " << m_lOpenResultCount;
    if (m_iResultId<8)
    {
        openSLog("本局开奖结果:类型:%d,%s,%s,累计开奖次数:%d;",m_iResultId, m_iResultId%2==0 ? "走兽":"飞禽" ,strm.c_str(),m_lOpenResultCount);
    }
    else
    {
        openSLog("本局开奖结果:类型:%d,%s,累计开奖次数:%d;",m_iResultId,strm.c_str(),m_lOpenResultCount);
    }
	
    string openStr = "开奖动物累计:";
    for (int i = 0; i < BET_ITEM_COUNT; ++i)
    {
        openStr += to_string(i) + "_" + to_string(m_lOpenResultIndexCount[i])+ "  ";
    }
    LOG(WARNING) << openStr ;
    openSLog("%s;",openStr.c_str());
    // if (m_bTest>0 && m_nTestTimes>0)
    // {
    //     m_nTestTimes--;
    //     LOG(WARNING)<< "==========剩余测试次数=========== " << m_nTestTimes;
    //     openSLog("剩余测试次数: %d;",m_nTestTimes);
    //     if (m_nTestTimes<=0)
    //     {
    //         m_nTestTimes = 0;
    //         m_bTest = 0;
    //         m_lOpenResultCount = 0;
    //         LOG(WARNING)<< "==========本次测试结束===========" ;
    //         openSLog("==========本次测试结束=========== ");
    //         m_LoopThread->getLoop()->cancel(m_TimerIdTest);
    //         TCHAR szPath[MAX_PATH] = TEXT("");
    //         TCHAR szConfigFileName[MAX_PATH] = TEXT("");
    //         TCHAR OutBuf[255] = TEXT("");
    //         CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
    //         myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/jsys_config.ini"), szPath);
    //         // TCHAR INI_SECTION_CONFIG[30] = TEXT("");
    //         // myprintf(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG), _T("ANDROID_NEWCONFIG_%d"), m_pTableFrame->GetGameRoomInfo()->roomId);
    //         CINIHelp::WritePrivateProfileString(_T"GAME_CONFIG", _T"TestGame","0",szConfigFileName);
    //         CINIHelp::WritePrivateProfileString(_T"GAME_CONFIG", _T"TestGameTimes","0",szConfigFileName);
    //         ReadInI();
    //     }        
    // }
    if (m_lOpenResultCount>100000 || m_lOpenResultIndexCount[m_iResultId]>10000000)
    {
        m_lOpenResultCount = 0;
        memset(m_lOpenResultIndexCount,0,sizeof(m_lOpenResultIndexCount));
    }

}
/*
* 位置是15-16-17是兔子， 0
* 位置是19-20-21是燕子，1
* 位置是12-13是猴子，2
* 位置是23-24是鸽子，3
* 位置是9-10是熊猫，4
* 位置是26-27是孔雀，5
* 位置5-6-7是狮子，6
* 位置1-2-3是老鹰，7
* 位置0-8-14-22是银鲨，8
* 位置4-11-18-25是金鲨，9
*/
 
// 结算
void TableFrameSink::NormalCalculate()
{
  //   if (m_bTest)
  //   {
  //       string str1,str2;
  //       int32_t allBet1,allBet2;

		// shared_ptr<IServerUserItem> pUserItem;
		// shared_ptr<UserInfo> userInfoItem;
		// for (auto &it : m_UserInfo)
		// {
		// 	userInfoItem = it.second;
		// 	pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
		// 	if (!pUserItem)
		// 		continue;

		// 	str1 = "";
		// 	str2 = "";
		// 	allBet1 = 0;
		// 	allBet2 = 0;
		// 	for (int j = 0; j < BET_ITEM_COUNT; j++)
		// 	{
		// 		allBet1 += userInfoItem->dAreaJetton[j];
		// 		allBet2 += m_pBetArea->mifourAreajetton[j];
		// 		str1 += str(boost::format("%d-%d ") % j % userInfoItem->dAreaJetton[j]);
		// 		str2 += str(boost::format("%d-%d ") % j % m_pBetArea->mifourAreajetton[j]);
		// 	}
		// 	openSLog("自己各区域的押注:%s,总押注:%d;", str1.c_str(), allBet1);
		// 	// openSLog("所有各区域的押注:%s,总押注:%d;", str2.c_str(), allBet2);
		// }

  //   }
    // 算法获取结果
    //ArgorithResult();

    Algorithm();

    // 选择出本轮最大赢分玩家
    int32_t winscore = 0;
    int bestChairID = 0;
    int bestUserid = 0;
    bool bFirstOnlineUser = true;
	//m_pPlayerInfoList.clear();
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    shared_ptr<UserInfo> userInfoItemMax;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if (bFirstOnlineUser) // 预防在线的玩家都输钱的情况
        {
            bFirstOnlineUser = false;
            winscore = userInfoItem->ReturnMoney;
            bestChairID = userInfoItem->wCharid;
            bestUserid = userInfoItem->wUserid;
        }
        //设置玩家倍率
        userInfoItem->SetPlayerMuticl(m_iResultId, m_iRetMul, m_iResultId_sd, m_nMuilt[m_iResultId_sd], m_iResultMul_cj);
        //税收比例
        userInfoItem->Culculate(m_fTaxRate,m_iResultId); //m_pITableFrame, 
        if (winscore <= userInfoItem->ReturnMoney)
        {
            winscore = userInfoItem->ReturnMoney;
            bestChairID = userInfoItem->wCharid;
            bestUserid = userInfoItem->wUserid;
        }
		//m_pPlayerInfoList.push_back(userInfoItem);
    }
    // 把最大赢分玩家添加到列表(20秒)
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(bestChairID);
    if (player && m_UserInfo[bestUserid]->ReturnMoney > 0)
    {
        BestPlayerList pla;
        pla.NikenName = player->GetNickName();
        pla.bestID = player->GetChairId();
        pla.WinScore = m_UserInfo[bestUserid]->ReturnMoney;
        time_t t = time(nullptr);

        char ch[64] = {0};

        strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t)); //年-月-日 时-分-秒
        pla.onTime = ch;
        //cout<<p->tm_year+1900<<"  "<<p->tm_mon+1<<"  "<<p->tm_mday<<"  "<<p->tm_hour<<

        if (m_vPlayerList.size() >= BEST_PLAYER_LIST_COUNT){
            m_vPlayerList.erase(m_vPlayerList.begin());
        }

        m_vPlayerList.push_back(pla);
    }
	//PlayerRichSorting();

	//写玩家分(注意顺序)
	WriteUserScore();
	m_bGameEnd = true;
	// 填充路单(长度9)
	if (m_vResultRecored.size() >= RES_RECORD_COUNT)//< 9)
	{
		m_lastRetId = m_vResultRecored[0];
		m_vResultRecored.erase(m_vResultRecored.begin());
		m_vResultRecored.push_back(m_iResultId);
		if (m_iResultId >= (int)AnimType::eSilverSharp)
		{
			m_lastRetId = m_vResultRecored[0];
			m_vResultRecored.erase(m_vResultRecored.begin());
			m_vResultRecored.push_back(m_iResultId_sd);
		}
	}
	else
	{
		m_vResultRecored.push_back(m_iResultId);
		if (m_iResultId >= (int)AnimType::eSilverSharp)
		{
			if (m_vResultRecored.size() >= RES_RECORD_COUNT)//< 9)
			{
				m_lastRetId = m_vResultRecored[0];
				m_vResultRecored.erase(m_vResultRecored.begin());
				m_vResultRecored.push_back(m_iResultId_sd);
			}
			else
			{
				m_vResultRecored.push_back(m_iResultId_sd);
			}
		}
	}

	float addTime = 0;
	if (m_iResultId == (int)AnimType::eSilverSharp)
		addTime = m_addCountDown_sd;
	else if (m_iResultId == (int)AnimType::eGoldSharp)
		addTime = m_addCountDown_sd + m_addCountDown_cj;

    // udpate state
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
	shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem)
            continue;
        if (userInfoItem->dCurrentPlayerJetton<=0)
        {
            userInfoItem->NotBetJettonCount++;
        }
        else //if (m_pPlayerList[i].NotBetJettonCount < 5)
        {            
            userInfoItem->NotBetJettonCount = 0;
        }
        Jsys::CMD_S_GameEnd gameEnd;
        gameEnd.set_cbplacetime((int32_t)(m_endCountDown + addTime));//12); //总时间
        gameEnd.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            gameEnd.add_selfjettonscore(userInfoItem->dAreaJetton[j]);
            gameEnd.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
        }
        gameEnd.set_lusermaxscore(0);
        gameEnd.set_winindex(m_iStopIndex);
        gameEnd.set_cartype(m_iResultId);
        gameEnd.set_cbroundid(m_strRoundId);

        LOG(WARNING) << "下发玩家分数 "<< userInfoItem->wCharid << " RealMoney=" << userInfoItem->RealMoney << " " << pUserItem->GetUserScore();

        gameEnd.set_luserscore(pUserItem->GetUserScore()); // //m_nPlayerScore[wCharid]
        gameEnd.set_mostwinscore(m_UserInfo[bestUserid]->ReturnMoney);

        shared_ptr<CServerUserItem> bestplayer = m_pITableFrame->GetTableUserItem(bestChairID);
        gameEnd.set_headid(bestplayer->GetHeaderId());
        gameEnd.set_headboxid(0); //bestplayer->GetHeadBoxID());
        gameEnd.set_gender(0);    //bestplayer->GetGender());
        m_BestPlayerUserId = bestplayer->GetUserId();
        m_BestPlayerChairId = bestChairID;
        m_BestPlayerWinScore = m_UserInfo[bestUserid]->ReturnMoney;
        gameEnd.set_bestuserid(bestplayer->GetUserId());
        gameEnd.set_bestusernikename(bestplayer->GetNickName());
        gameEnd.set_userwinscore(userInfoItem->RealMoney);
        gameEnd.set_onlinenum(m_pITableFrame->GetPlayerCount(true));

		int fqWin = 0;
		int zsWin = 0;
		for (int j = m_vResultRecored.size() - 1; j >= 0; j--)
		{
			if (m_vResultRecored[j] < (int)AnimType::eSilverSharp)
			{
				if (m_vResultRecored[j] % 2 == 1)
					fqWin++;
				else
					zsWin++;
			}
		}
		gameEnd.set_lfeiqincount(fqWin);
		gameEnd.set_lzoushoucount(zsWin);
		i = 0;
        for (auto &uInfoItem : m_pPlayerInfoList)
		{
			xzScore = 0;
			pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
			if (!pServerUserItem)
				nowScore = 0;
			else
			{
                /*for (int j = 0; j < BET_ITEM_COUNT; j++)
				{
                    xzScore += uInfoItem->dAreaJetton[j];
				}*/
				nowScore = pServerUserItem->GetUserScore();
			}
			/*if (userInfoItem->chairId == m_wBankerUser)
			{
				continue;
			}*/
			if (uInfoItem->headerId > 12)
			{
				LOG_INFO << " ========== 1 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
			}
            Jsys::PlayerListItem *palyer = gameEnd.add_players();
            palyer->set_dwuserid(uInfoItem->wUserid);
            palyer->set_headerid(uInfoItem->headerId);
			palyer->set_nviplevel(0);
            palyer->set_nickname(uInfoItem->nickName);

			palyer->set_luserscore(nowScore);

            palyer->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
            palyer->set_ltwentywincount(uInfoItem->lTwentyWinCount);
            int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
			palyer->set_isdivinemath(shensuanzi);
			palyer->set_index(i + 1);
			if (shensuanzi == 1)
				palyer->set_index(0);

			palyer->set_isbanker(0);
			//if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
			//{
			   // palyer->set_isbanker(1);
			//}
			palyer->set_gender(0);
			palyer->set_isapplybanker(0);
			palyer->set_applybankermulti(0);
			palyer->set_ljetton(0);
            palyer->set_szlocation(uInfoItem->location);
			palyer->set_headboxid(0);
			palyer->set_luserwinscore(uInfoItem->RealMoney);
			if (++i >= size)
				break;
		}

		if (m_iResultId >= (int)AnimType::eSilverSharp)
		{
			gameEnd.set_cartype_sd(m_iResultId_sd);
			gameEnd.set_winindex_sd(m_iStopIndex_sd);
			if (m_iResultId >= (int)AnimType::eGoldSharp)
			{
				int32_t dAreaJetton = 1;//m_UserInfo[pUserItem->GetUserId()]->dAreaJetton[8];
				gameEnd.set_caijinscore(m_iResultMul_cj*dAreaJetton);
			}
		}

        std::string endData = gameEnd.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(pUserItem->GetChairId(), Jsys::SUB_S_GameEnd, (uint8_t *)endData.c_str(), endData.size());

        openSLog("本局结算: wCharid=%d,userid=%d,UserScore=%d,输赢=%d;",pUserItem->GetChairId(),pUserItem->GetUserId(),pUserItem->GetUserScore(),userInfoItem->RealMoney);
    }

	m_pPlayerInfoList.clear();
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
		if (!pUserItem || !userInfoItem->Isplaying)
			continue;
		m_pPlayerInfoList.push_back(userInfoItem);
	}
	PlayerRichSorting();
    //100局记录
	updateResultList(m_iResultId);	
	/*for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
		if (!pUserItem)
			continue;
		sendGameRecord(userInfoItem->wCharid);
	}*/
}

// 玩家财富排序
void TableFrameSink::PlayerRichSorting(bool iChooseShenSuanZi)
{
    if (m_pPlayerInfoList.size() > 1)
    {
		if (iChooseShenSuanZi)
		{
			sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions);
			sort(++m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
			m_iShenSuanZiUserId = m_pPlayerInfoList[0]->wUserid;
			m_iShenSuanZiChairId = m_pPlayerInfoList[0]->wCharid;
			m_ShenSuanZiId = m_pPlayerInfoList[0]->wUserid;
		} 
		else
		{
			sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
		}
    }   

	SortPlayerList();
    
}

void TableFrameSink::SendPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{

    if (m_iGameState == Jsys::SUB_S_GameEnd)
    {
        if (m_vPlayerList.size() > 1)
        {
            Jsys::CMD_S_UserWinList PlayerList;
            for (int i = m_vPlayerList.size() - 2; i > -1; i--)
            {
                shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(m_vPlayerList[i].bestID);
                if (!player)
                {
                    continue;
                }
                Jsys::PlayerInfo *playerinfo = PlayerList.add_player();
                playerinfo->set_winscore(m_vPlayerList[i].WinScore);
                playerinfo->set_wintime(m_vPlayerList[i].onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_gender(0);    //player->GetGender());
                playerinfo->set_headboxid(0); //player->GetHeadBoxID());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_nviplevel(0); //player->GetVipLevel());
                playerinfo->set_szlocation(player->GetLocation()); 
				playerinfo->set_luserscore(player->GetUserScore());
            }
            string sendData = PlayerList.SerializeAsString();
            m_pITableFrame->SendTableData(chairid, Jsys::SUB_S_PLAYERLIST, (uint8_t *)sendData.c_str(), sendData.size());
        }
    }
    else
    {
        if (m_vPlayerList.size() > 0)
        {
            Jsys::CMD_S_UserWinList PlayerList;
            for (int i = m_vPlayerList.size() - 1; i > -1; i--)
            {
                shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(m_vPlayerList[i].bestID);
                if (!player)
                {
                    continue;
                }
                Jsys::PlayerInfo *playerinfo = PlayerList.add_player();
                playerinfo->set_winscore(m_vPlayerList[i].WinScore);
                playerinfo->set_wintime(m_vPlayerList[i].onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_gender(0);    //player->GetGender());
                playerinfo->set_headboxid(0); //player->GetHeadBoxID());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_nviplevel(0); //player->GetVipLevel());
                playerinfo->set_szlocation(player->GetLocation()); 
				playerinfo->set_luserscore(player->GetUserScore());
            }
            string sendData = PlayerList.SerializeAsString();
            m_pITableFrame->SendTableData(chairid, Jsys::SUB_S_PLAYERLIST, (uint8_t *)sendData.c_str(), sendData.size());
        }
    }

    LOG(WARNING) << "----发送排行榜数据------";
}
bool TableFrameSink::GamePlayerJetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> pSelf = m_pITableFrame->GetTableUserItem(chairid);
    if (!pSelf)
        return false;

    Jsys::CMD_C_PlaceJet placeJet;
    placeJet.ParseFromArray(data, datasize);

    int cbJetArea = placeJet.cbjettonarea();
    int32_t lJetScore = placeJet.ljettonscore();
    
    //LOG(WARNING) << " Area " << cbJetArea << " Score " << lJetScore << " " << (int)subid;
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
    // if (Jsys::SUB_S_GameStart != m_iGameState)
    // {
    //     LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_iGameState << " " << Jsys::SUB_S_GameStart;
    //     return true;
    // }

    bool bPlaceJetScuccess = false;
    bool nomoney = false;
    int errorAsy = 0;

    do
    {
        if (Jsys::SUB_S_GameStart != m_iGameState)
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
    if (!bPlaceJetScuccess || Jsys::SUB_S_GameStart != m_iGameState)
    {
        // 非机器人才返回
        if(!pSelf->IsAndroidUser()){
            Jsys::CMD_S_PlaceJettonFail placeJetFail;
            int64_t userid = pSelf->GetUserId();
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(cbJetArea);
            placeJetFail.set_lplacescore(lJetScore);
            placeJetFail.set_cbandroid(pSelf->IsAndroidUser());
            placeJetFail.set_returncode(errorAsy);

        	std::string sendData = placeJetFail.SerializeAsString();
            // if (m_bTest==0)
       		   m_pITableFrame->SendTableData(pSelf->GetChairId(), Jsys::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
        }
        LOG(WARNING) << "-------------------玩家下注无效---------------" << errorAsy;
        return false;
    }
	
	uint32_t userId = pSelf->GetUserId();
	if (m_ShenSuanZiId == userId && m_ShenSuanZiId != 0)
	{
		m_lShenSuanZiJettonScore[cbJetArea] += lJetScore;
	}

	//LOG(WARNING) << "------- Place Jet Scuccess-------- UserBet: userid:" << pSelf->GetUserId() << " cbJetArea:" << cbJetArea << " lJetScore:" << lJetScore;
    int pmayernum = 0; 
    //shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
	shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
		pServerUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pServerUserItem)
            continue;
        Jsys::CMD_S_PlaceJetSuccess placeJetSucc;
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
        alljetton = userInfoItem->dCurrentPlayerJetton;
		//int UserScore = pServerUserItem->GetUserScore() - alljetton;
        placeJetSucc.set_luserscore(pServerUserItem->GetUserScore() - alljetton);
        for (int k = 1; k <= MAX_JETTON_AREA; k++)
        {
            placeJetSucc.add_selfjettonscore(userInfoItem->dAreaJetton[k - 1]);
        } 
		//LOG(WARNING) << "------- Place Jet Scuccess-------- sendUser userid:" << pServerUserItem->GetUserId() << " GetUserScore():" << pServerUserItem->GetUserScore()  << " alljetton:" << alljetton <<  " leftUserScore:" << UserScore;
        std::string sendData = placeJetSucc.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(userInfoItem->wCharid, Jsys::SUB_S_JETTON_SUCCESS, (uint8_t *)sendData.c_str(), sendData.size());
        pmayernum++;
    }
    // LOG(WARNING) << "-------------------- Place Jet Scuccess------------ Num=" << pmayernum;

    return true;
}
void TableFrameSink::GamePlayerJetton1(int32_t score, int index, int chairid, shared_ptr<CServerUserItem> &pSelf)
{
    int32_t lJetScore = score;
    int cbJetArea = index;

    // if (lJetScore <= 0 || cbJetArea < 0 || cbJetArea >= BET_ITEM_COUNT)
    // {
    //     LOG(ERROR) << "---------------押分有效性检查出错----------" << cbJetArea << " "<< lJetScore;
    //     return;
    // }

    // LOG(WARNING) << "2 Area " << cbJetArea << " Score " << lJetScore;
    // 筹码合法性检查
    //if (lJetScore != 1 && lJetScore != 10 && lJetScore != 50 && lJetScore != 100 && lJetScore != 500)
    //    return false;

    bool bPlaceJetScuccess = false;
    bool nomoney = false;
    int errorAsy = 0;
    do
    {
        if (Jsys::SUB_S_GameStart != m_iGameState)
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

    // 
    if (!bPlaceJetScuccess)
    {
        Jsys::CMD_S_PlaceJettonFail placeJetFail;
        int64_t userid = pSelf->GetUserId();
        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(errorAsy); //-1=shijian guo   1==wanjia shibai 2=qu yu man 3=wanjia fen buzu
        placeJetFail.set_returncode(errorAsy);

        placeJetFail.set_lplacescore(lJetScore);
        placeJetFail.set_cbandroid(pSelf->IsAndroidUser());

        std::string sendData = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(pSelf->GetChairId(), Jsys::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());

        LOG(WARNING) << "-------------------续押玩家下注无效---------------" << errorAsy;
        return;
    }
    
    Jsys::CMD_S_PlaceJetSuccess placeJetSucc;
    int64_t userid = pSelf->GetUserId();
    placeJetSucc.set_dwuserid(userid);
    placeJetSucc.set_cbjettonarea(cbJetArea);
    placeJetSucc.set_ljettonscore(lJetScore);
    placeJetSucc.set_bisandroid(pSelf->IsAndroidUser());
    int32_t alljetton = 0;
    // alljetton = m_pPlayerList[chairid].dCurrentPlayerJetton;
    alljetton = m_UserInfo[userid]->dCurrentPlayerJetton;

    placeJetSucc.set_luserscore(pSelf->GetUserScore() - alljetton);
    for (int i = 0; i < MAX_JETTON_AREA; i++)
    {
        placeJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[i]);

    }

	if (m_ShenSuanZiId == userid && m_ShenSuanZiId != 0)
	{
		m_lShenSuanZiJettonScore[cbJetArea] += lJetScore;
	}

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem )
            continue;
        placeJetSucc.clear_selfjettonscore();
        for (int k = 0; k < MAX_JETTON_AREA; k++)
        {
            placeJetSucc.add_selfjettonscore(userInfoItem->dAreaJetton[k]);

        }
        std::string sendData = placeJetSucc.SerializeAsString();
        m_pITableFrame->SendTableData(userInfoItem->wCharid, Jsys::SUB_S_JETTON_SUCCESS, (uint8_t *)sendData.c_str(), sendData.size());
    }
}
// 重押
void TableFrameSink::Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
    if (!Usr){ LOG(WARNING) << "----查找玩家不在----"; return;}

    if (!m_UserInfo[Usr->GetUserId()]->Isplaying)
    {
        LOG(WARNING) << "-------------续押失败(玩家状态不对)-------------";
        return;
    }

    LOG(WARNING) << "-----------Rejetton续押-------------";
 	// 游戏状态判断
    if (Jsys::SUB_S_GameStart != m_iGameState)
    {
        LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_iGameState << " " << Jsys::SUB_S_GameStart;
        return;
    }

    int64_t userid = Usr->GetUserId();

    int alljetton = 0;
    int areajetton[BET_ITEM_COUNT] = {0};
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        alljetton += m_UserInfo[userid]->dLastJetton[i];
        areajetton[i] = m_UserInfo[userid]->dLastJetton[i];
        LOG(WARNING) << "--dLastJetton-" << areajetton[i] ;
    }

    // 续押失败
    if (alljetton == 0 || Usr->GetUserScore() < alljetton)
    {
        Jsys::CMD_S_PlaceJettonFail placeJetFail;

        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(3);
        placeJetFail.set_lplacescore(0);
        placeJetFail.set_returncode(3);
        placeJetFail.set_cbandroid(Usr->IsAndroidUser());

        std::string sendData = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(Usr->GetChairId(), Jsys::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
        LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
        return;
    }

    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
        if (m_nAreaMaxJetton[i] < m_UserInfo[userid]->dAreaJetton[i] + m_UserInfo[userid]->dLastJetton[i])
        {
            Jsys::CMD_S_PlaceJettonFail placeJetFail;
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(i);
            placeJetFail.set_lplacescore(m_UserInfo[userid]->dLastJetton[i]);
            placeJetFail.set_returncode(2);
            placeJetFail.set_cbandroid(Usr->IsAndroidUser());

            std::string data = placeJetFail.SerializeAsString();
			// if (m_bTest==0)
            	m_pITableFrame->SendTableData(Usr->GetChairId(), Jsys::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());
            LOG(WARNING) << "---------续押失败(超过区域最大押分)---------" << m_nAreaMaxJetton[i] <<" < "<<  m_pBetArea->mifourAreajetton[i] <<"+"<< m_UserInfo[userid]->dLastJetton[i];
            return;
        }
    }

    //返回续押数组
    // Jsys::CMD_S_RepeatJetSuccess RepeatJet;
    // RepeatJet.set_dwuserid(userid);
    // RepeatJet.set_bisandroid(Usr->IsAndroidUser());
    // RepeatJet.set_luserscore(Usr->GetUserScore() - alljetton);
   
    // bool bIsJetOk = false;
    // //RepeatJet
    // for (tagBetInfo betVal : m_pPlayerList[chairid].JettonValues)
    // {
    //     Jsys::RepeatInfo *RepeatInfo = RepeatJet.add_trepeat();
    //     RepeatInfo->set_luserid(userid);
    //     RepeatInfo->set_tljettonscore(betVal.betScore);
    //     RepeatInfo->set_tjetidx(betVal.betIdx);
    //     bIsJetOk = GamePlayerJetton1(betVal.betScore, betVal.betIdx, chairid, Usr);
    //     areajetton[betVal.betIdx] -= betVal.betScore;
    // }

    // // 重押失败
    // if(!bIsJetOk) {
    //     LOG(ERROR) << "---------重押失败 重押失败 重押失败--------" << m_pPlayerList[chairid].JettonValues.size();
    //     return;
    // }

    // //每个区域的总下分
    // for (int i = 0; i < MAX_JETTON_AREA; i++){
    //     RepeatJet.add_alljettonscore(m_pBetArea->mifourAreajetton[i]);
    // }

    // //单个玩家每个区域的总下分
    // for (int i = 0; i < MAX_PLAYER; i++)
    // {
    //     shared_ptr<CServerUserItem> player = this->m_pITableFrame->GetTableUserItem(i);
    //     if (!player) continue;

    //     RepeatJet.clear_selfjettonscore();
    //     for (int k = 0; k < MAX_JETTON_AREA; k++){
    //         RepeatJet.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[k]);
    //     }

    //     std::string data = RepeatJet.SerializeAsString();
    //     m_pITableFrame->SendTableData(i, Jsys::SUB_S_REPEAT_JETTON_OK, (uint8_t *)data.c_str(), data.size());
    // }

    // LOG(ERROR) << "---------重押成功--------";
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        if (areajetton[i] == 0)
        {
            continue;
        }

        while (areajetton[i] > 0)
        {
            // if (areajetton[i] >= m_pBetArea->GetBetItem(4))//500
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(4);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else if (areajetton[i] >= m_pBetArea->GetBetItem(3))//100)
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(3);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else if (areajetton[i] >= m_pBetArea->GetBetItem(2))//50)
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(2); 
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else if (areajetton[i] >= m_pBetArea->GetBetItem(1))//10)
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(1); 
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(0); 
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            int jSize = m_cbJettonMultiple.size();
            for (int j=jSize-1;j>=0;j--)
            {
                if (areajetton[i] >= m_cbJettonMultiple[j])
                {
                    int32_t tmpVar = m_cbJettonMultiple[j];
                    GamePlayerJetton1(tmpVar, i, chairid, Usr);
                    areajetton[i] -= tmpVar;
                    break;
                }
                if(areajetton[i]<m_cbJettonMultiple[0])
                {
                    break;
                }
            }
        }
    }
}
void TableFrameSink::GamePlayerQueryPlayerList(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd)
{
    shared_ptr<CServerUserItem> pIIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if (!pIIServerUserItem)
        return;

	m_pPlayerInfoList.clear();

	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
		if (!pUserItem)
			continue;
		if (!userInfoItem->Isplaying)
		{
			continue;
		}
		m_pPlayerInfoList.push_back(userInfoItem);
	}

    Jsys::CMD_C_PlayerList querylist;
    querylist.ParseFromArray(data, datasize);
    int32_t limitcount = querylist.nlimitcount();
    int32_t beginindex = querylist.nbeginindex();
    int32_t resultcount = 0;
    int32_t endindex = 0;
    Jsys::CMD_S_PlayerList playerlist;
    int32_t wMaxPlayer = m_pITableFrame->GetMaxPlayerCount();
    if (!limitcount)
        limitcount = wMaxPlayer;
	limitcount = 20;
    int index = 0;
    PlayerRichSorting(false);
	SortPlayerList();
    for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
    {
        shared_ptr<CServerUserItem> pUser = this->m_pITableFrame->GetTableUserItem((*it).wCharid);
        if (!pUser)
            continue;
        if (!(*it).Isplaying)
            continue;
		if (pUser->GetHeaderId() > 12)
		{
            LOG_INFO << " ========== 2 wUserid " << pUser->GetUserId() << " == headerId:" << pUser->GetHeaderId();
		}
        Jsys::PlayerListItem *item = playerlist.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser->GetHeaderId());
        item->set_nviplevel(0); //pUser->GetVipLevel());
        item->set_nickname(pUser->GetNickName());
        item->set_headboxid(0); //pUser->GetHeadBoxID());
        if (!m_bGameEnd)
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
        item->set_luserwinscore((*it).RealMoney);
        endindex = index + 1;
        index++;
        resultcount++;
        if (resultcount >= limitcount)
        {
            break;
        }
    }
    playerlist.set_nendindex(endindex);
    playerlist.set_nresultcount(resultcount);
    std::string lstData = playerlist.SerializeAsString();
    m_pITableFrame->SendTableData(pIIServerUserItem->GetChairId(), Jsys::SUB_S_QUERY_PLAYLIST, (uint8_t *)lstData.c_str(), lstData.size());
	PlayerRichSorting();
}
// 发送场景消息(押分阶段)
void TableFrameSink::SendGameSceneStart(int64_t userid, bool lookon)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userid);

    if (!player)
        return;

    LOG(WARNING) << "********** Send GameScene Start **********" << userid;

    Jsys::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime(m_betCountDown);
    scenceStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
    //筹码数量
    int jsize = 6;
    int32_t dAlljetton = 0;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        Jsys::CMD_AeraInfo *areainfo = scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_UserInfo[player->GetUserId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]);
        areainfo->set_mutical(m_nMuilt[i]);
        dAlljetton += m_UserInfo[player->GetUserId()]->dAreaJetton[i];
      
        //押分筹码
       if(i < jsize)
           scenceStart.add_betitems(m_currentchips[player->GetChairId()][i]);
    }
	int fqWin = 0;
	int zsWin = 0;
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {
        scenceStart.add_cartype(m_vResultRecored[i]);
        // LOG(WARNING) << "Recored =>" << i << " "<< m_vResultRecored[i];
        if (m_vResultRecored[i] < (int)AnimType::eSilverSharp)
		{
			if (m_vResultRecored[i] % 2 == 1)
				fqWin++;
			else
				zsWin++;
		}
    }
	scenceStart.set_lfeiqincount(fqWin);
	scenceStart.set_lzoushoucount(zsWin);

    scenceStart.set_nlastretid(m_lastRetId);
    

    // for (int i = 0; i < MAX_PLAYER; i++)
    // {
    //     shared_ptr<CServerUserItem> usr = this->m_pITableFrame->GetTableUserItem(i);
    //     if (nullptr == usr)  continue;

    //     if (m_pPlayerList[i].dCurrentPlayerJetton > 0)
    //     {
    //         // 增加玩家押分筹码值
    //         for (tagBetInfo betVal : m_pPlayerList[i].JettonValues)
    //         {
    //             Jsys::RepeatInfo *RepeatInfo = scenceStart.add_trepeat();
    //             RepeatInfo->set_luserid(usr->GetUserId());
    //             RepeatInfo->set_tljettonscore(betVal.betScore);
    //             RepeatInfo->set_tjetidx(betVal.betIdx);
    //         }
    //      }
    // } 

    scenceStart.set_ncurstate(0); 
    scenceStart.set_ncurretidx(m_iStopIndex); 

    scenceStart.set_luserscore(player->GetUserScore() - dAlljetton);
    scenceStart.set_dwuserid(player->GetUserId());
    scenceStart.set_gender(0);    //player->GetGender());
    scenceStart.set_headboxid(0); //player->GetHeadBoxID());
    scenceStart.set_headerid(player->GetHeaderId());
    scenceStart.set_nickname(player->GetNickName());
    scenceStart.set_nviplevel(0); //player->GetVipLevel());
    //scenceStart.set_szlocation(player->GetLocation());
    scenceStart.set_cbroundid(m_strRoundId);
	scenceStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pServerUserItem;
	for (auto &uInfoItem : m_pPlayerInfoList)
	{
		xzScore = 0;
		pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
		if (!pServerUserItem)
			nowScore = 0;
		else
		{
			for (int j = 0; j < BET_ITEM_COUNT; j++)
			{
				xzScore += uInfoItem->dAreaJetton[j];
			}
			nowScore = pServerUserItem->GetUserScore() - xzScore;
		}
		/*if (userInfoItem->chairId == m_wBankerUser)
		{
			continue;
		}*/
		if (uInfoItem->headerId > 12)
		{
			LOG_INFO << " ========== 3 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
		}
        Jsys::PlayerListItem *palyerItem = scenceStart.add_players();
		palyerItem->set_dwuserid(uInfoItem->wUserid);
		palyerItem->set_headerid(uInfoItem->headerId);
		palyerItem->set_nviplevel(0);
		palyerItem->set_nickname(uInfoItem->nickName);

		palyerItem->set_luserscore(nowScore);

		palyerItem->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
		palyerItem->set_ltwentywincount(uInfoItem->lTwentyWinCount);
        int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
		palyerItem->set_isdivinemath(shensuanzi);
		palyerItem->set_index(i + 1);
		if (shensuanzi == 1)
			palyerItem->set_index(0);

		palyerItem->set_isbanker(0);
		//if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
		//{
		   // palyer->set_isbanker(1);
		//}
		palyerItem->set_gender(0);
		palyerItem->set_isapplybanker(0);
		palyerItem->set_applybankermulti(0);
		palyerItem->set_ljetton(0);
		palyerItem->set_szlocation(uInfoItem->location);
		palyerItem->set_headboxid(0);
        palyerItem->set_luserwinscore(uInfoItem->RealMoney);
		if (++i >= size)
			break;
	}

	for (int j = 0; j < BET_ITEM_COUNT; j++)
	{
		scenceStart.add_mutical(m_nMuilt[j]);
	}

	vector<shared_ptr<OpenResultInfo>> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();
	int notOpenSilverSharp = 0;
	int notOpenGoldSharp = 0;
	for (int i = 0;i < BET_ITEM_COUNT; ++i)
	{
		vResultList = m_vOpenResultListCount_last[i];
		if (openCount == 0)
			scenceStart.add_notopencount(0);
		else
		{
			scenceStart.add_notopencount(vResultList[openCount - 1]->notOpenCount);
			if (i == (int)AnimType::eSilverSharp)
			{
				notOpenSilverSharp = vResultList[openCount - 1]->notOpenCount;
			}
			else if (i == (int)AnimType::eGoldSharp)
			{
				notOpenGoldSharp = vResultList[openCount - 1]->notOpenCount;
				if (notOpenGoldSharp >= notOpenSilverSharp)
				{
					scenceStart.set_notopencount(i, notOpenSilverSharp);
				}
				else
				{
					scenceStart.set_notopencount(i - 1, notOpenGoldSharp);
				}
			}
		}
	}

    std::string sendData = scenceStart.SerializeAsString();
    m_pITableFrame->SendTableData(player->GetChairId(), Jsys::SUB_S_SCENE_START, (uint8_t *)sendData.c_str(), sendData.size());
}

// 发送场景消息(结算阶段)
void TableFrameSink::SendGameSceneEnd(int64_t userid, bool lookon)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userid);

    if (!player)
        return;

	float addTime = 0;
	if (m_iResultId == (int)AnimType::eSilverSharp)
		addTime = m_addCountDown_sd;
	else if (m_iResultId == (int)AnimType::eGoldSharp)
		addTime = m_addCountDown_sd + m_addCountDown_cj;

    Jsys::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime((int32_t)(m_endCountDown + addTime));
    scenceStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);

  //筹码数量
    int jsize = 6;

    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        Jsys::CMD_AeraInfo *areainfo = scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_UserInfo[player->GetUserId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]);
        areainfo->set_mutical(m_nMuilt[i]);

        //押分筹码
        if(i < jsize)
            scenceStart.add_betitems(m_currentchips[player->GetChairId()][i]);
    }

    scenceStart.set_nlastretid(m_lastRetId);
	int fqWin = 0;
	int zsWin = 0;
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {
        scenceStart.add_cartype(m_vResultRecored[i]);
        if (m_vResultRecored[i] < (int)AnimType::eSilverSharp)
		{
			if (m_vResultRecored[i] % 2 == 1)
				fqWin++;
			else
				zsWin++;
		}
    }
	scenceStart.set_lfeiqincount(fqWin);
	scenceStart.set_lzoushoucount(zsWin);
    // for (int i = 0; i < MAX_PLAYER; i++)
    // {
    //     shared_ptr<CServerUserItem> usr = this->m_pITableFrame->GetTableUserItem(i);
    //     if (nullptr == usr)  continue;

    //     if (m_pPlayerList[i].dCurrentPlayerJetton > 0)
    //     {
    //         // 增加玩家押分筹码值
    //         for (tagBetInfo betVal : m_pPlayerList[i].JettonValues)
    //         {
    //             Jsys::RepeatInfo *RepeatInfo = scenceStart.add_trepeat();
    //             RepeatInfo->set_luserid(usr->GetUserId());
    //             RepeatInfo->set_tljettonscore(betVal.betScore);
    //             RepeatInfo->set_tjetidx(betVal.betIdx);
    //         }
    //      }
    // }
    //scenceStart.set_ncurretidx(m_iStopIndex);  
    scenceStart.set_ncurstate(1); //游戏状态 0,押注状态；1,结算状态
    scenceStart.set_ncurretidx(m_iStopIndex);
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
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
	shared_ptr<IServerUserItem> pServerUserItem;
	for (auto &uInfoItem : m_pPlayerInfoList)
	{
		xzScore = 0;
		pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
		if (!pServerUserItem)
			nowScore = 0;
		else
		{
			/*for (int j = 0; j < BET_ITEM_COUNT; j++)
			{
				xzScore += uInfoItem->dAreaJetton[j];
			}*/
			nowScore = pServerUserItem->GetUserScore();
		}
		/*if (userInfoItem->chairId == m_wBankerUser)
		{
			continue;
		}*/
		if (uInfoItem->headerId>12)
		{
			LOG_INFO << " ========== 4 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
		}
        Jsys::PlayerListItem *palyerItem = scenceStart.add_players();
		palyerItem->set_dwuserid(uInfoItem->wUserid);
		palyerItem->set_headerid(uInfoItem->headerId);
		palyerItem->set_nviplevel(0);
		palyerItem->set_nickname(uInfoItem->nickName);

		palyerItem->set_luserscore(nowScore);

		palyerItem->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
		palyerItem->set_ltwentywincount(uInfoItem->lTwentyWinCount);
        int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
		palyerItem->set_isdivinemath(shensuanzi);
		palyerItem->set_index(i + 1);
		if (shensuanzi == 1)
			palyerItem->set_index(0);

		palyerItem->set_isbanker(0);
		//if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
		//{
		   // palyer->set_isbanker(1);
		//}
		palyerItem->set_gender(0);
		palyerItem->set_isapplybanker(0);
		palyerItem->set_applybankermulti(0);
		palyerItem->set_ljetton(0);
		palyerItem->set_szlocation(uInfoItem->location);
		palyerItem->set_headboxid(0);
        palyerItem->set_luserwinscore(uInfoItem->RealMoney);
		if (++i >= size)
			break;
	}

	if (m_iResultId >= (int)AnimType::eSilverSharp)
    {
        scenceStart.set_ncurretidx_sd(m_iStopIndex_sd);
	}

	for (int j = 0; j < BET_ITEM_COUNT; j++)
	{
		scenceStart.add_mutical(m_nMuilt[j]);
	}

	vector<shared_ptr<OpenResultInfo>> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();
	int notOpenSilverSharp = 0;
	int notOpenGoldSharp = 0;
	for (int i = 0;i < BET_ITEM_COUNT; ++i)
	{
		vResultList = m_vOpenResultListCount_last[i];
		if (openCount == 0)
			scenceStart.add_notopencount(0);
		else
		{
			scenceStart.add_notopencount(vResultList[openCount - 1]->notOpenCount);
			if (i == (int)AnimType::eSilverSharp)
			{
				notOpenSilverSharp = vResultList[openCount - 1]->notOpenCount;
			}
			else if (i == (int)AnimType::eGoldSharp)
			{
				notOpenGoldSharp = vResultList[openCount - 1]->notOpenCount;
				if (notOpenGoldSharp >= notOpenSilverSharp)
				{
					scenceStart.set_notopencount(i, notOpenSilverSharp);
				}
				else
				{
					scenceStart.set_notopencount(i - 1, notOpenGoldSharp);
				}
			}
		}
	}

    std::string sendData = scenceStart.SerializeAsString();
    m_pITableFrame->SendTableData(player->GetChairId(), Jsys::SUB_S_SCENE_START, (uint8_t *)sendData.c_str(), sendData.size());

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
        // allBetScore += m_pPlayerList[chairid].dAreaJetton[i];
        allBetScore += m_UserInfo[pSelf->GetUserId()]->dAreaJetton[i];
    }

    // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
    /*
    if (m_nAreaMaxJetton[index] < m_pBetArea->mifourAreajetton[index] + score)
    {
        LOG(ERROR) << "---------超过区域最大押分--------" << m_nAreaMaxJetton[index];
        return 2;
    }
    */
    
    // 判断是否超过区域最大押分   < 玩家每个区域的总下分  + 本次玩家押分值
    if (m_nAreaMaxJetton[index] < m_UserInfo[pSelf->GetUserId()]->dAreaJetton[index] + score)
    {
        LOG(ERROR) << "---------超过区域最大押分--------" << m_nAreaMaxJetton[index];
        return 2;
    }

    // 判断是否小于准许进入分
    int32_t enterMinScore = m_pGameRoomInfo->enterMinScore; // m_pITableFrame->GetUserEnterMinScore();
    if (pSelf->GetUserScore() - allBetScore - score < enterMinScore)
    {
        LOG(WARNING) << "---------小于准许进入分--------" << " UserId:" << pSelf->GetUserId();
        return 3;
    }
	// 押注项超出限制范围
    // if (m_nAreaMaxJetton[index] < m_pBetArea->mifourAreajetton[index] + score)
    // {
    //     LOG(ERROR) << "------押注项超出限制范围-----" << m_nAreaMaxJetton[index] <<" < "<<score << " + "<< m_pBetArea->mifourAreajetton[index];
    //     return 4;
    // }
    bool bFind = false;
    int leng =m_cbJettonMultiple.size();
    for(int i=0;i<leng;i++)
    {
        if(m_cbJettonMultiple[i]==score)
        {
            bFind = true;
            break;
        }
    }
    if(!bFind)
    {
        LOG_ERROR << " Jettons Error:"<<score;
        return 4;       // 下注的筹码值无效
    }

    //已经下的总注
    if(pSelf->GetUserScore() < allBetScore + score)
    {
        LOG_ERROR << " Real User Score is not enough ";        
        return 5;      // 押注已达最大的下注金额
    }
    
    // 清缓存筹码
    if (m_UserInfo[pSelf->GetUserId()]->dCurrentPlayerJetton == 0){
        m_UserInfo[pSelf->GetUserId()]->clearJetton();
    }

    // 记录真人押分,有玩家押分则添加
    if(!pSelf->IsAndroidUser()) {

        // 记录押分步骤
        m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime,to_string(score),-1,opBet,chairid,index);

        // 记录真人押分(有玩家押分则添加)
        if(m_UserInfo[pSelf->GetUserId()]->dCurrentPlayerJetton == 0)
        {
            LOG(WARNING) << "********** 记录真人押分（有玩家押分则添加）**********" << pSelf->GetUserId();
            m_replay.addPlayer(pSelf->GetUserId(),pSelf->GetAccount(),pSelf->GetUserScore(),chairid);
        }
    }

    tagBetInfo betinfo = {0};
    betinfo.betIdx = index;
    betinfo.betScore = score;
    // 记录玩家押分筹码值
    m_UserInfo[pSelf->GetUserId()]->JettonValues.push_back(betinfo);

    // 允许下注(分数累加)
    m_UserInfo[pSelf->GetUserId()]->dAreaJetton[index] += score;
    // 当前玩家押分
    m_UserInfo[pSelf->GetUserId()]->dCurrentPlayerJetton = allBetScore + score;
    // 押分区增加押分
    m_pBetArea->SetJettonScore(index, score, /*mpBankerManager->GetBankerScore()*/0, pSelf->IsAndroidUser());
    //一段时间内各区域其他玩家的总下注
    // for (int i = 0; i < MAX_PLAYER; ++i)
    // {
    //     shared_ptr<CServerUserItem> UserItem = this->m_pITableFrame->GetTableUserItem(i);
    //     if (nullptr == UserItem || i==chairid)
    //         continue;
    //     m_pPlayerList[i].dsliceSecondsAllJetton += score;
    //     m_pPlayerList[i].dsliceSecondsAllAreaJetton[index] += score;
    // }
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if (nullptr == pUserItem || userInfoItem->wCharid==chairid)
            continue;
        userInfoItem->dsliceSecondsAllJetton += score;
        userInfoItem->dsliceSecondsAllAreaJetton[index] += score;
    }
    return 0;
}

//发送一段时间内相对自己的其他所有玩家的总押注数据
void TableFrameSink::OnTimerSendOtherBetJetton()
{
    // for (int i = 0; i < MAX_PLAYER; ++i)
    // {
    //     shared_ptr<CServerUserItem> pUserItem = this->m_pITableFrame->GetTableUserItem(i);
    //     if (nullptr == pUserItem)
    //         continue;
    //     // if (!m_pPlayerList[i].Isplaying)
    //     // {
    //     //     continue;
    //     // }
    //     // if (pUserItem->IsAndroidUser())
    //     // {
    //     //     continue;
    //     // }
    //     if (m_pPlayerList[i].dsliceSecondsAllJetton <= 0)
    //     {
    //         continue;
    //     }
    //     // 押分区增加押分
    //     // int32_t score = 0;
    //     // for (int index = 0; index < MAX_JETTON_AREA; index++)
    //     // {
    //     //     score = m_pPlayerList[i].dsliceSecondsAllAreaJetton[index];
    //     //     if ( score <= 0 ) continue;
    //     //     m_pBetArea->SetJettonScore(index, score, 0, pUserItem->IsAndroidUser());
    //     // }

    //     Jsys::CMD_S_OtherPlaceJetSuccess otherPlaceJetSucc;
    //     otherPlaceJetSucc.Clear();

    //     int64_t userid = -1;
    //     otherPlaceJetSucc.set_dwuserid(userid);
    //     for (int x = 1; x <= MAX_JETTON_AREA; x++)
    //     {
    //         otherPlaceJetSucc.add_alljettonscore(m_pPlayerList[i].dsliceSecondsAllAreaJetton[x-1]);
    //         otherPlaceJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[x - 1]);
    //     }

    //     std::string data = otherPlaceJetSucc.SerializeAsString();
    //     // m_pITableFrame->SendTableData(i, Jsys::SUB_S_JETTON_SUCCESS_OTHER, (uint8_t *)data.c_str(), data.size());

    //     m_pPlayerList[i].clearOtherJetton();
    // }
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

    sprintf(Filename, "log//Jsys//jsys_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
        return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}

//设置当局游戏详情
void TableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail = "";
	Jsys::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_residx(m_iResultId);				//开奖结果
	details.set_idxmul(m_iRetMul);					//结果倍数
	//开奖类型[0:金鲨银鲨;1:走兽;2:飞禽]
	int32 resType = 0;
	int32 resTypeMul = 0;
	if (m_iResultId < (int)AnimType::eSilverSharp)
	{
		resType = userInfo->IsFeiQin(m_iResultId) ? 1 : 2;
		resTypeMul = userInfo->IsFeiQin(m_iResultId) ? m_nMuilt[BET_IDX_BIRD] : m_nMuilt[BET_IDX_BEAST];
	} 
	details.set_restype(resType);
	//类型倍数
	details.set_restypemul(resTypeMul);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->RealMoney); //总输赢积分

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		for (int betIdx = 0;betIdx < BET_ITEM_COUNT; betIdx++)
		{
			// 结算时记录
			if (betIdx == (int)AnimType::eGoldSharp)
			{
				continue;
			}
			int64_t winScore = userInfo->dAreaWin[betIdx];
			int64_t betScore = userInfo->dAreaJetton[betIdx];
			int32_t	nMuilt = m_nMuilt[betIdx];// 倍率表m_nMuilt
			if (m_iResultId == (int)AnimType::eGoldSharp && betIdx == 8)
			{
				nMuilt = m_nMuilt[9];
			} 
			//if (betScore > 0)
			{
				Jsys::BetAreaRecordDetail* detail = details.add_detail();
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

	if (m_iResultId >= (int)AnimType::eSilverSharp)
	{
		details.set_residxsd(m_iResultId_sd);
		details.set_idxmulsd(m_nMuilt[m_iResultId_sd]);
		if (m_iResultId == (int)AnimType::eGoldSharp)
			details.set_mulcj(m_iResultMul_cj);
	}
	

	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
}

//更新100局开奖结果信息
void TableFrameSink::updateResultList(int resultId)
{
	vector<shared_ptr<OpenResultInfo>> vResultList;
	for (int i = 0;i < BET_ITEM_COUNT - 2;++i)
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
		if (resultId < (int)AnimType::eSilverSharp && resultId == i)
		{
			bool bFeiqin = iResultIndex->IsFeiQin(resultId);
			//是走兽
			if (bFeiqin)
			{
				shared_ptr<OpenResultInfo> iResultIndex_fq(new OpenResultInfo());
				vResultList = m_vOpenResultListCount[BET_IDX_BIRD];
				if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
				{
					vResultList.erase(vResultList.begin());
				}
				resultCount = vResultList.size();
				iResultIndex_fq->iResultId = resultId;
				iResultIndex_fq->notOpenCount = 0;
				vResultList.push_back(iResultIndex_fq);
				m_vOpenResultListCount[BET_IDX_BIRD] = vResultList;

				shared_ptr<OpenResultInfo> iResultIndex_zs(new OpenResultInfo());
				shared_ptr<OpenResultInfo> iResultIndex_zsls(new OpenResultInfo());
				vResultList = m_vOpenResultListCount[BET_IDX_BEAST];
				if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
				{
					vResultList.erase(vResultList.begin());
				}
				iResultIndex->clear();
				resultCount = vResultList.size();
				if (resultCount > 0)
				{
					iResultIndex_zsls = vResultList[resultCount - 1];
					iResultIndex_zs->notOpenCount = iResultIndex_zsls->notOpenCount + 1;
				}
				else
				{
					iResultIndex_zs->notOpenCount++;
				}
				iResultIndex_zs->iResultId = resultId;
				
				vResultList.push_back(iResultIndex_zs);
				m_vOpenResultListCount[BET_IDX_BEAST] = vResultList;
			}
			else
			{
				shared_ptr<OpenResultInfo> iResultIndex_fq(new OpenResultInfo());
				vResultList = m_vOpenResultListCount[BET_IDX_BEAST];
				if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
				{
					vResultList.erase(vResultList.begin());
				}
				resultCount = vResultList.size();
				if (resultCount > 0)
					iResultIndex = vResultList[resultCount - 1];				
				iResultIndex_fq->iResultId = resultId;
				iResultIndex_fq->notOpenCount = 0;
				vResultList.push_back(iResultIndex_fq);
				m_vOpenResultListCount[BET_IDX_BEAST] = vResultList;

				shared_ptr<OpenResultInfo> iResultIndex_zs(new OpenResultInfo());
				shared_ptr<OpenResultInfo> iResultIndex_zsls(new OpenResultInfo());
				vResultList = m_vOpenResultListCount[BET_IDX_BIRD];
				if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
				{
					vResultList.erase(vResultList.begin());
				}
				resultCount = vResultList.size();
				if (resultCount > 0)
				{
					iResultIndex_zsls = vResultList[resultCount - 1];
					iResultIndex_zs->notOpenCount = iResultIndex_zsls->notOpenCount + 1;
				}
				else
				{
					iResultIndex_zs->notOpenCount++;
				}
				iResultIndex_zs->iResultId = resultId;
				
				vResultList.push_back(iResultIndex_zs);
				m_vOpenResultListCount[BET_IDX_BIRD] = vResultList;
			}
		}
		else if (resultId == i)
		{
			shared_ptr<OpenResultInfo> iResultIndex_fq(new OpenResultInfo());
			shared_ptr<OpenResultInfo> iResultIndex_fqls(new OpenResultInfo());
			vResultList = m_vOpenResultListCount[BET_IDX_BIRD];
			if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
			{
				vResultList.erase(vResultList.begin());
			}
			resultCount = vResultList.size();
			iResultIndex_fq->iResultId = resultId;
			if (resultCount > 0)
			{
				iResultIndex_fqls = vResultList[resultCount - 1];
				iResultIndex_fq->notOpenCount = iResultIndex_fqls->notOpenCount + 1;
			}
			else
			{
				iResultIndex_fq->notOpenCount++;
			}
			vResultList.push_back(iResultIndex_fq);
			m_vOpenResultListCount[BET_IDX_BIRD] = vResultList;

			shared_ptr<OpenResultInfo> iResultIndex_zs(new OpenResultInfo());
			shared_ptr<OpenResultInfo> iResultIndex_zsls(new OpenResultInfo());
			vResultList = m_vOpenResultListCount[BET_IDX_BEAST];
			if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
			{
				vResultList.erase(vResultList.begin());
			}
			iResultIndex->clear();
			resultCount = vResultList.size();
			if (resultCount > 0)
			{
				iResultIndex_zsls = vResultList[resultCount - 1];
				iResultIndex_zs->notOpenCount = iResultIndex_zsls->notOpenCount + 1;
			}
			else
			{
				iResultIndex_zs->notOpenCount++;
			}
			iResultIndex_zs->iResultId = resultId;

			vResultList.push_back(iResultIndex_zs);
			m_vOpenResultListCount[BET_IDX_BEAST] = vResultList;
		}
	}

}


void TableFrameSink::sendGameRecord(int32_t chairid)
{
	//记录
	Jsys::CMD_S_GameRecord gameRecordRsp;

	vector<shared_ptr<OpenResultInfo>> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();
	gameRecordRsp.set_opencount(openCount);

	for (int i = 0;i < BET_ITEM_COUNT; ++i)
	{
		vResultList = m_vOpenResultListCount_last[i];
		Jsys::GameRecordList *gamerecordList = gameRecordRsp.add_recordlists();
		for (int j = 0; j < openCount; j++)
		{
			if (i == 0)
				gameRecordRsp.add_iresultids(vResultList[j]->iResultId);
			Jsys::GameRecordListItem *listItem = gamerecordList->add_records();
			listItem->set_notopencount(vResultList[j]->notOpenCount);
		}
	}

	std::string sendData = gameRecordRsp.SerializeAsString();
	m_pITableFrame->SendTableData(chairid, Jsys::SUB_S_QUERY_GAMERECORD, (uint8_t *)sendData.c_str(), sendData.size());
}

// 获取本局金鲨开奖的倍数
void TableFrameSink::getJSodds()
{
	m_iResultId_sd = 0;
	m_iResultMul_js = 24;
	m_iResultMul_cj = 6;

	int32_t  wholeweight = 0;
	int32_t res = 0;
	//float cotrolWeight[BET_ITEM_COUNT-4] = { 0 };
	//for (int i = 0;i < BET_ITEM_COUNT - 4;i++)
	//{
	//	cotrolWeight[i] = m_retWeight[i];
	//	wholeweight += m_retWeight[i];
	//}
	//// 取赠送的动物
	//int randNum = m_random.betweenInt(0, wholeweight).randInt_mt(true);
	//for (int i = 0;i < BET_ITEM_COUNT - 4;i++)
	//{
	//	res += cotrolWeight[i];
	//	if (randNum - res <= 0)
	//	{
	//		m_iResultId_sd = i;
	//		break;
	//	}
	//}
	m_iResultId_sd = m_random.betweenInt(0, 7).randInt_mt(true);
	// 获取金鲨的倍数(24-99倍),赠送彩金倍数(6-99倍)
	int randNum = m_random.betweenInt(0, 1000).randInt_mt(true);
	int addOdds = 0;
	LOG(WARNING) << " getJSodds 控制比例：" << m_ctrlGradePercentRate[0] << " " << m_ctrlGradePercentRate[1] << "，金鲨低倍: " << m_ctrlGradeMul[0] << "，彩金低倍:  " << m_ctrlGradeMul[1];
	openSLog("getJSodds,控制比例：%d,%d,金鲨低倍:%d,彩金低倍:%d", m_ctrlGradePercentRate[0], m_ctrlGradePercentRate[1], m_ctrlGradeMul[0], m_ctrlGradeMul[1]);

	LOG(WARNING) << " getJSodds 获取金鲨的比例 randNum：" << randNum;
	openSLog("getJSodds,获取金鲨的比例 randNum：%d", randNum);
	if (randNum < m_ctrlGradePercentRate[0])
	{
		m_iResultMul_js = m_random.betweenInt(24, m_ctrlGradeMul[0]).randInt_mt(true);
	} 
	else if (randNum < m_ctrlGradePercentRate[1])
	{
		m_iResultMul_js = m_random.betweenInt(m_ctrlGradeMul[0] + 1, 75).randInt_mt(true);
	}
	else
	{
		m_iResultMul_js = m_random.betweenInt(76, 99).randInt_mt(true);
	}
	//彩金
	randNum = m_random.betweenInt(0, 1000).randInt_mt(true);
	LOG(WARNING) << " getJSodds 获取彩金的比例 randNum：" << randNum;
	openSLog("getJSodds,获取彩金的比例 randNum：%d", randNum);
	if (randNum < m_ctrlGradePercentRate[0])
	{
		m_iResultMul_cj = m_random.betweenInt(6, m_ctrlGradeMul[1]).randInt_mt(true);
	}
	else if (randNum < m_ctrlGradePercentRate[1])
	{
		m_iResultMul_cj = m_random.betweenInt(m_ctrlGradeMul[1] + 1, 50).randInt_mt(true);
	}
	else
	{
		m_iResultMul_cj = m_random.betweenInt(51, 99).randInt_mt(true);
	}	

	openSLog("本局金鲨的倍数:%d,若开金鲨赠送彩金的倍数:%d ,若开金鲨银鲨将要赠送的动物id:%d,", m_iResultMul_js, m_iResultMul_cj, m_iResultId_sd);
	LOG(INFO) << "本局金鲨的倍数:" << m_iResultMul_js << " ,若开金鲨赠送彩金的倍数:" << m_iResultMul_cj << " ,若开金鲨银鲨将要赠送的动物id: " << m_iResultId_sd;
	int jsIndex = (int)AnimType::eGoldSharp;
	m_nMuilt[jsIndex] = m_iResultMul_js;
	//重新计算金鲨银鲨的权重
	//int ysIndex  = (int)AnimType::eSilverSharp;
 //   float weight = 0.95f * 10000 / (m_nMuilt[ysIndex] + m_nMuilt[m_iResultId_sd]);
	//m_retWeight[ysIndex] = (int)weight;
	//// 金鲨权重
 //   weight = 0.95f * 10000 / (m_nMuilt[jsIndex] + m_nMuilt[m_iResultId_sd] + m_iResultMul_cj);
	//m_retWeight[jsIndex] = (int)weight;
	string str = "";
	for (int i = 0;i < BET_ITEM_COUNT - 2;i++)
	{
		str += to_string(i) + "_" + to_string(m_retWeight[i]) + "  ";
	}
	openSLog("本局刷新后的权重：%s", str.c_str());
}

void TableFrameSink::RefreshPlayerList(bool isGameEnd)
{
	m_pPlayerInfoList_20.clear();
	shared_ptr<IServerUserItem> pServerUserItem;
    for(int i=0;i<m_pPlayerInfoList.size();i++)
	{
        if (!m_pPlayerInfoList[i])
            continue;
        pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->wCharid);
		if (!pServerUserItem)
			continue;
        m_pPlayerInfoList[i]->GetTwentyWinScore();
        m_pPlayerInfoList[i]->RefreshGameRusult(isGameEnd);
	}

    for(int i=0;i<m_pPlayerInfoList.size();i++)
     {
        if (!m_pPlayerInfoList[i])
           continue;
        pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->wCharid);
        if (!pServerUserItem)
            continue;
        m_pPlayerInfoList_20.push_back((*m_pPlayerInfoList[i]));
        LOG(ERROR) << "old二十局赢分:"<< (m_pPlayerInfoList[i]->GetTwentyWinScore());
        LOG(ERROR) << "二十局赢分:"<< (m_pPlayerInfoList_20[i].GetTwentyWinScore());
     }
	//m_pPlayerInfoList_20.assign(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end());
	m_LoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
	PlayerRichSorting();
	//更新开奖结果
	m_vOpenResultListCount_last.clear();
	for (auto &it : m_vOpenResultListCount)
	{
		m_vOpenResultListCount_last[it.first] = it.second;
	}
	if (isGameEnd)
	{
		shared_ptr<IServerUserItem> pUserItem;
		shared_ptr<UserInfo> userInfoItem;
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
			if (!pUserItem)
				continue;
			sendGameRecord(userInfoItem->wCharid);
		}
	}	
}

void TableFrameSink::SortPlayerList()
{
	for (int i = 0;i < m_pPlayerInfoList_20.size();i++)
	{
		for (int j = i + 1;j < m_pPlayerInfoList_20.size();j++)
		{
			UserInfo userinfo;
			if (m_pPlayerInfoList_20[i].GetTwentyWinScore() < m_pPlayerInfoList_20[j].GetTwentyWinScore())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
			else if (m_pPlayerInfoList_20[i].GetTwentyWinScore() == m_pPlayerInfoList_20[j].GetTwentyWinScore()
				&& m_pPlayerInfoList_20[i].GetTwentyJetton() < m_pPlayerInfoList_20[j].GetTwentyJetton())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
			else if (m_pPlayerInfoList_20[i].GetTwentyJetton() == m_pPlayerInfoList_20[j].GetTwentyJetton()
				&& m_pPlayerInfoList_20[i].GetTwentyWin() < m_pPlayerInfoList_20[j].GetTwentyWin())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
		}

	}
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
