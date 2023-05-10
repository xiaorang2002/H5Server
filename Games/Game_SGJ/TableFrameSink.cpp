#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TimerId.h"
#include "muduo/base/Logging.h"

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
#include "public/StdRandom.h"
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/Sgj.Message.pb.h"

#include "json/json.h"
#include "ConfigManager.h"
#include "CMD_Game.h"
#include "GameLogic.h"
#include "ConfigManager.h"

using namespace Sgj;
#include "TableFrameSink.h"

//定时器
#define ID_TIME_BET_SCORE 102 //下注定时器
#define ID_TIME_GAME_END 103  //结束定时器

//定时器时间
#define TIME_BET 15      //下注时间
#define TIME_GAME_END 10 //结束时间 (有人下注有大赢家)

#define ONCELOCK \
    for (static bool o_b_; !o_b_; o_b_ = true)

int64_t CTableFrameSink::m_llStorageControl = 0;
int64_t CTableFrameSink::m_llTodayStorage = 0;
int64_t CTableFrameSink::m_llStorageLowerLimit = 0;
int64_t CTableFrameSink::m_llStorageHighLimit = 0;
int32_t CTableFrameSink::m_nAndroidCount = 0; //机器人数量


using namespace std;

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/sgj";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if (!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("sgj");
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit; // 声明全局变局,初始化库.

CTableFrameSink::CTableFrameSink()
{
    m_wGameStatus = GAME_STATUS_INIT;
    stockWeak = 0.0;
    InitGameData();
}

CTableFrameSink::~CTableFrameSink()
{
}

void CTableFrameSink::InitGameData()
{
    //随机种子
    srand((unsigned)time(NULL));
    //游戏中玩家
    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));

    //游戏记录
    //    memset(m_RecordScoreInfo, 0, sizeof(m_RecordScoreInfo));

    //玩家原始分
    memset(&m_llUserSourceScore, 0, sizeof(m_llUserSourceScore));

    //结算数据
    //    memset(&m_GameEnd, 0, sizeof(m_GameEnd));
    m_MinEnterScore = 0;

    m_Round = 0;
    memset(m_tLastOperate, 0, sizeof(m_tLastOperate));
    memset(m_nMarrayNumLeft, 0, sizeof(m_nMarrayNumLeft));
    memset(m_nFreeRoundLeft, 0, sizeof(m_nFreeRoundLeft));
    memset(m_lMarryTotalWin, 0, sizeof(m_lMarryTotalWin));
    memset(m_lUserBetScore, 0, sizeof(m_lUserBetScore));
    memset(m_lUserWinScore, 0, sizeof(m_lUserWinScore));
    memset(m_nMarryNum, 0, sizeof(m_nMarryNum));
    memset(m_lFreeRoundWinScore, 0, sizeof(m_lFreeRoundWinScore));

    m_FreeRound = 0;
    m_MarryRound = 0;
    m_nPullInterval = 0;

    m_kickTime = 0;
    m_kickOutTime = 0;

    m_userId = 0;
    m_bUserIsLeft = true;
    m_lFreeWinScore = 0;
    m_lMarryWinScore = 0;
    m_lastJackpotScore = 0;
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
    m_replay.clear();
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);
    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;

    
    LOG(WARNING)<<"+++++++++库存值 Storage+++++++:";
    LOG(WARNING)<<"Control:"<<m_llStorageControl;
    LOG(WARNING)<<"TodayStorage:"<<m_llTodayStorage;
    LOG(WARNING)<<"LowerLimit:"<<m_llStorageLowerLimit;
    LOG(WARNING)<<"HighLimit:"<<m_llStorageHighLimit;
    
    // LOG(WARNING) << "游戏开始 OnGameStart:" ;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t nEndTag)
{
    LOG(WARNING) << "游戏结束 OnEventGameConclude:" <<chairId;

    return true;
}

//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    if (chairId >= GAME_PLAYER) { return false;  }
  
    CMD_S_GameFree StatusFree; 

    StatusFree.set_maxbet(m_AllConfig.m_MaxBet);
    StatusFree.set_minbet(m_AllConfig.m_MinBet);

    StatusFree.set_lcurrentbet(0);
    StatusFree.set_myscoreinfo(m_pITableFrame->GetTableUserItem(chairId)->GetUserScore());
    StatusFree.set_nleftnum(m_nFreeRoundLeft[chairId]);
    StatusFree.set_nsmallmarynum(m_nMarrayNumLeft[chairId]);
    StatusFree.set_totalwin(m_lMarryTotalWin[chairId]);

    //填充RewardList数据
    // vector<int> vRewardList = ConfigManager::GetInstance()->GetRewardList();
    // for(int rewardItem :vRewardList){
    //     LOG(WARNING) << "vRewardList:" << rewardItem;
    //     StatusFree.add_rewardlists(rewardItem);
    // }

    vector<int> vBetItemList = ConfigManager::GetInstance()->GetBetItemList();
    int betIndex = 0;
    int Index = 0;
    //填充数据
    for(int betItem :vBetItemList){
        LOG(WARNING) << "vBetItemList:" << betItem;
        if(betItem == 100) {
            betIndex = Index;
        }
        StatusFree.add_betlists(betItem);
        Index++;
    }

    if(vBetItemList.size() > 0){
        StatusFree.set_lcurrentbet(betIndex);//客户端要索引
        m_lUserBetScore[chairId] = vBetItemList[betIndex] * LINE_MAX;
    }
    else
    {
        assert(vBetItemList.size() > 0);
    }
    

    LOG(WARNING) << "m_lMarryTotalWin:" << m_lMarryTotalWin[chairId];
    LOG(WARNING) << "m_nMarrayNumLeft:" << m_nMarrayNumLeft[chairId];
    LOG(WARNING) << "m_nFreeRoundLeft:" << m_nFreeRoundLeft[chairId];
    LOG(WARNING) << "m_lUserBetScore:" << StatusFree.lcurrentbet() <<" " << m_lUserBetScore[chairId];
    LOG(WARNING) << "m_MaxBet:" <<  StatusFree.maxbet();
    LOG(WARNING) << "m_MinBet:" << StatusFree.minbet();

    std::string data = StatusFree.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, SUB_SC_GAMESCENE_FREE, (uint8_t *)data.c_str(), data.size());

    LOG(WARNING) << "发送场景 OnEventGameScene:" <<chairId;

    return true;
}

bool CTableFrameSink::OnUserEnter(int64_t userId, bool islookon)
{
    LOG(WARNING) << "OnUserEnter:" <<userId << " "<< m_pITableFrame->GetTableId();

    m_userId = userId;

    if (islookon)  return false; 

    //用户是否离开
    m_bUserIsLeft = false;
    m_lastJackpotScore = 0; 

    auto dwChairID = m_pITableFrame->GetUserItemByUserId(userId)->GetChairId();
    //只有一个玩家一个桌子的时候才可以这么设置
    m_ChairId = dwChairID;
    //清数据
    ClearGameData(dwChairID);
        
    m_lUserScore = m_pITableFrame->GetTableUserItem(dwChairID)->GetUserScore();
    m_llUserSourceScore[dwChairID] = m_lUserScore;
 
    // 是否机器人
    m_isAndroid = m_pITableFrame->GetUserItemByUserId(userId)->IsAndroidUser();
    if(m_isAndroid)
    {
        int64_t sum = 0;
        for(int i=0;i<10;i++)
        {
             sum+= m_AllConfig.m_AndroidWeight[i];
        }
        int ranNum = m_random.betweenInt(0,sum).randInt_mt(true);
        int64_t valu = 0;
        for(int i=0;i<10;i++)
        {
            valu+=m_AllConfig.m_AndroidWeight[i];
            if(valu>ranNum)
            {
                m_userId = m_AllConfig.m_AndroidUserId[i];
                break;
            }
        }
        if(m_userId==0)
        {
            m_userId = m_AllConfig.m_AndroidUserId[0];
        }
        //机器人关键一步，把机器人UserId设置成为配置文件的Userid
        m_pITableFrame->GetTableUserItem(dwChairID)->SetAgentId(m_AllConfig.m_AgentId);
        m_pITableFrame->GetTableUserItem(dwChairID)->SetUserId(m_userId);
        m_nAndroidCount++;
        LOG(ERROR) << "机器人进入: " << m_AllConfig.m_MaxAndroidCount<< " " << m_nAndroidCount <<" "<<userId;
    }


    LOG(WARNING) << m_AllConfig.m_testCount <<" "<<m_AllConfig.m_testWeidNum<<" "<<m_AllConfig.m_runAlg;
 
    //踢出时间
    m_kickOutTime = m_AllConfig.nKickOutTime;

    //踢出检测
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdKickOut);
    m_TimerIdKickOut=m_TimerLoopThread->getLoop()->runEvery(1, boost::bind(&CTableFrameSink::checkKickOut, this));

    m_TimerLoopThread->getLoop()->cancel(m_TimerIdJackpot);
    // 是否机器人
    if (!m_isAndroid)
    {
        // 彩金
        m_TimerIdJackpot = m_TimerLoopThread->getLoop()->runEvery(2, boost::bind(&CTableFrameSink::SendJackPot, this));
    }
    else
    {
        m_TimerIdJackpot = m_TimerLoopThread->getLoop()->runEvery(2, boost::bind(&CTableFrameSink::CheckKickAndroid, this));
    }
    

    m_TimerTestCount = 0;

    if(m_AllConfig.m_runAlg == 1){
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
        m_TimerIdTest=m_TimerLoopThread->getLoop()->runEvery(0.02, boost::bind(&CTableFrameSink::LeftReward, this,userId,dwChairID));
    }

    //用户进入发送场景消息
    OnEventGameScene(dwChairID,islookon);
    LOG(ERROR) << "TotalStock:" << GetTotalStock() << ",LowerStock:" << GetLowerStock()<<",HighStock:"<<GetHighStock();

    return true;
}

// 机器人离线踢出检测
void CTableFrameSink::CheckKickAndroid()
{
    shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(m_ChairId);
    if (!user)
    {
        LOG(ERROR) << "用户不存在:" << m_ChairId << " " << __FUNCTION__;
        return;
    }

    if (user->IsAndroidUser() && (user->GetUserStatus() == sOffline))
    {
        user->SetUserStatus(sGetout);
        //修改桌子状态
        m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
        //踢玩家
        OnUserLeft(m_userId, false);
        //清理桌子
        m_pITableFrame->ClearTableUser(m_ChairId, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
        LOG(WARNING) << "机器人离线被踢出 " << m_userId << " ChairId " << m_ChairId;
    }
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool islookon)
{
    auto dwChairID = m_pITableFrame->GetUserItemByUserId(userId)->GetChairId();
    if (dwChairID >= GAME_PLAYER)
    {
        LOG(WARNING) << "用户起立 ERROR: "<< dwChairID;
        return false;
    }
    //用户离开
    m_bUserIsLeft = true;

    // 机器人离开要减
    if(m_isAndroid){
       if(m_nAndroidCount > 0) m_nAndroidCount--;
    }

    LOG(WARNING) << "剩小玛丽次数 1: " << m_nMarrayNumLeft[dwChairID]<< " " << m_nFreeRoundLeft[dwChairID];

    if (m_nFreeRoundLeft[dwChairID] > 0)
    {
        int nFreeCnt = m_nFreeRoundLeft[dwChairID];
        for (size_t i = 0; i < nFreeCnt; i++)
        {
            LeftReward(userId,dwChairID);
        }
        
        m_nFreeRoundLeft[dwChairID] = 0;
    }
    else if (m_nMarrayNumLeft[dwChairID] > 0)
    {
        int nMarryCnt = m_nMarrayNumLeft[dwChairID];
        for (size_t i = 0; i < nMarryCnt; i++)
        {
            LeftReward(userId,dwChairID);
        }

        m_nMarrayNumLeft[dwChairID] = 0;
    }
  
    LOG(WARNING) << "剩小玛丽次数 2: " << m_nMarrayNumLeft[dwChairID]<< " " << m_nFreeRoundLeft[dwChairID];

    //保存数据
    WriteFreeGameRecord(dwChairID,m_lUserBetScore[dwChairID],m_nFreeRoundLeft[dwChairID],m_nMarrayNumLeft[dwChairID],m_nMarryNum[dwChairID]);
    
    //清数据
    ClearGameData(dwChairID);

    //起立
    m_pITableFrame->ClearTableUser(dwChairID,true);//INVALID_CHAIR, true);
    
    //销毁定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdKickOut);

    return true;
}

//用户同意
bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    return true;
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    // 机器人数量限制
    if(pUser && pUser->IsAndroidUser()){
       if(m_nAndroidCount >= m_AllConfig.m_MaxAndroidCount) {
            // LOG(ERROR) << "机器人数量已经到达上限: " << m_AllConfig.m_MaxAndroidCount<< " " << m_nAndroidCount;
            return false;
       }
    }
    return true;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    return true;
} 
  
// 保存对局结果
void CTableFrameSink::checkKickOut()
{
    //m_kickTime++;
    if ( m_kickTime >= m_kickOutTime )
    { 
        uint32_t wChairID = m_ChairId;
        //发送超时提示
        // 
        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(wChairID);
        if(!user)
        {
	        LOG(ERROR) << "用户不存在:" << wChairID<<" "<< m_userId;
            return;
        }

        if(!user->IsAndroidUser()){
            // 
            user->SetUserStatus(sGetout);
            // 发送踢出通知
            SendErrorCode(0, ERROR_OPERATE_TIMEOUT);

            //修改桌子状态
            m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
            //踢玩家
            OnUserLeft(m_userId, false);
            //清理桌子
            m_pITableFrame->ClearTableUser(wChairID, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
	        LOG(WARNING) << "超时被踢出 "<< m_kickTime << " " << m_kickOutTime;
        }

        m_kickTime = 0;
    }

    // 判断服务器是否在维护
    if(m_pITableFrame->GetGameRoomInfo()->serverStatus != SERVER_RUNNING){
        //修改桌子状态
        m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
        OnTimerClearUser();
        OnUserLeft(m_userId,false);
    }

    //保存对局结果
	// LOG(WARNING) << "kickTime "<< m_kickTime << " " << m_kickOutTime;
}
// 
void CTableFrameSink::LeftReward(int64_t userId,uint32_t wChairID)
{
	// LOG(WARNING) << "LeftReward " << userId << " " << wChairID;

    if(m_nMarryNum[wChairID] > 0 && m_nMarrayNumLeft[wChairID] > 0)
    {
        m_MarryRound ++;
	    LOG(WARNING) << "余下玛丽次数 " << m_nMarrayNumLeft[wChairID]<< " 总开次数 "<<m_MarryRound;

        //发送开始消息
        CMD_C_GameStart GameStart;
        GameStart.set_llbetscore(0);

        std::string data = GameStart.SerializeAsString();
        //发送数据
        OnGameMessage(wChairID,SUB_C_MARRY,(uint8_t*)data.c_str(), data.size());

    }
    else
    {
        if(m_nFreeRoundLeft[wChairID] > 0)
        {
            m_FreeRound++;
	        LOG(WARNING) << "余下免费次数 " << m_nFreeRoundLeft[wChairID] << " 总开次数 "<<m_FreeRound;
        }
 

        //发送开始消息
        CMD_C_GameStart GameStart;
        GameStart.set_llbetscore(m_lUserBetScore[wChairID]);

        std::string data = GameStart.SerializeAsString();
        //发送数据
        OnGameMessage(wChairID,SUB_C_START,(uint8_t*)data.c_str(), data.size());
    }

    if (m_AllConfig.m_runAlg == 1)
    {
        if (m_TimerTestCount++ > m_AllConfig.m_testCount)
        {
            int64_t affterScore = m_pITableFrame->GetTableUserItem(wChairID)->GetUserScore();
            LOG(WARNING) << "测试跑次数 " << m_TimerTestCount << " 跑前玩家分 " << m_lUserScore << "  跑后玩家分 " << affterScore << " 差 " << m_lUserScore - affterScore;
	        LOG(WARNING) << "玛丽总开次数 "<<m_MarryRound << " 得分 "<< m_lMarryWinScore << " 免费总开次数 "<<m_FreeRound <<" 得分 "<<m_lFreeWinScore;
            m_TimerTestCount = 0;

            if(m_AllConfig.m_testWeidNum==0)  LOG(WARNING) << " 跑收分 ";
            else if(m_AllConfig.m_testWeidNum==2)  LOG(WARNING) << " 跑放分 ";
            else  LOG(WARNING) << " 跑正常 ";
            m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
        }
    }
}

//游戏消息处理
bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t subid, const uint8_t *data, uint32_t dwDataSize)
{
    //如果当前处理维护状态
    if(m_pITableFrame->GetGameRoomInfo()->serverStatus != SERVER_RUNNING){
         SendErrorCode(wChairID,ERROR_STATUS); 
         return true;
    }
    //清理
    cardValueStr = "";
    //开始时间
	m_startTime = chrono::system_clock::now();//(uint32_t)time(NULL);//开始时间
	groupStartTime = (uint32_t)time(NULL);

    //清零
    m_kickTime = 0;

    m_replay.clear(); 
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;

    pUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    m_replay.addPlayer(pUserItem->GetUserId(), pUserItem->GetAccount(), pUserItem->GetUserScore(), wChairID);

    //记录原始积分
    m_llUserSourceScore[wChairID] = pUserItem->GetUserScore();
    // 是否机器人
    bool m_Android = pUserItem->IsAndroidUser();
    if(m_Android)
    {
        int64_t sum = 0;
        for(int i=0;i<10;i++)
        {
             sum+= m_AllConfig.m_AndroidWeight[i];
        }
        int ranNum = m_random.betweenInt(0,sum).randInt_mt(true);
        int64_t valu = 0;
        for(int i=0;i<10;i++)
        {
            valu+=m_AllConfig.m_AndroidWeight[i];
            if(valu>ranNum)
            {
                m_userId = m_AllConfig.m_AndroidUserId[i];
                break;
            }
        }
        if(m_userId==0)
        {
            m_userId = m_AllConfig.m_AndroidUserId[0];
        }
        //机器人关键一步，把机器人UserId设置成为配置文件的Userid
        pUserItem->SetAgentId(m_AllConfig.m_AgentId);
        pUserItem->SetUserId(m_userId);

    }
    switch (subid)
    {
        case SUB_C_START:
        {

          //玛丽还没结束前不能操作
          if (m_nMarrayNumLeft[wChairID] > 0){
              LOG(WARNING) << "玛丽还没结束前不能操作 ";
              return false;
          }

           if(pUserItem->IsAndroidUser())
           {
               int ret = cfgManager->CheckReadConfig();
               if(ret != 0){
                   LOG(WARNING) << "配置文件读取失败 " << ret;
                   return false;
               }

               //获取配置文件
               memset(&m_AllConfig, 0, sizeof(m_AllConfig));
               ConfigManager::GetInstance()->GetConfig(m_AllConfig);

           }



            // 是否免费游戏
            bool isFreeGame = m_nFreeRoundLeft[wChairID] > 0 ? true : false;

            if(m_nPullInterval == 0){ 
                m_nPullInterval = m_AllConfig.nPullInterval;
            }

            CMD_C_GameStart pGamePulling;
            pGamePulling.ParseFromArray(data, dwDataSize);
            

            if(!isFreeGame)
            {
                //玩家下注金额
                m_lUserBetScore[wChairID] =  pGamePulling.llbetscore();
            }

            vector<int> vBetItemList = ConfigManager::GetInstance()->GetBetItemList();
            bool isRight =false;
            for(int betItem :vBetItemList)
            {
                if(betItem* LINE_MAX == m_lUserBetScore[wChairID] )
                {
                    isRight = true;
                    break;
                }

            }
            if(m_lUserBetScore[wChairID]<=0||!isRight)
            {
                LOG_WARN << "有狗逼玩家搞事情 ";
                return false ;
            }
            m_replay.cellscore = m_lUserBetScore[wChairID] / LINE_MAX;

            //押分分数太小
            if ( !isFreeGame && m_lUserBetScore[wChairID] > pUserItem->GetUserScore())
            {
                LOG_WARN << "玩家分数不足 " << m_lUserBetScore[wChairID] <<" UserId:"<< pUserItem->GetUserId();
                 SendErrorCode(wChairID,(enErrorCode)ERROR_SCORE_NOT_ENOUGH); 
                return false;
            }

            //免费转状态下不限制
            if(m_bUserIsLeft)
                m_nPullInterval = 0;

            if (m_AllConfig.m_runAlg==1)
            {
                m_nPullInterval = 0;
            }
        

    #if DEBUG_INFO
            LOG(WARNING) << " SUB_C_START "<<  m_lUserBetScore[wChairID] ;
    #endif
            //拉动频率限制
            if ((time(NULL) - m_tLastOperate[wChairID]) < m_nPullInterval)
            {
                LOG(WARNING) << "拉动太频繁了 " << m_nPullInterval << " "<<wChairID <<" UserId:"<<pUserItem->GetUserId();
                return false;
            }


    
            //修改用户状态
            m_pITableFrame->SetUserReady(wChairID);
            //清除数据
            m_nMarrayNumLeft[wChairID] = 0;
            m_nMarryNum[wChairID] = 0;

            // 更新彩金信息
            if ( !isFreeGame ){
                // 押注的1%,押注值已经放大100倍但最小值为0.9，要存储整数，所以还要放大10倍存储
                int64_t incBet=0;
                if(!m_pITableFrame->GetTableUserItem(wChairID)->IsAndroidUser())
                {
                   incBet = m_lUserBetScore[wChairID] * m_AllConfig.m_BetRateVal * 0.1;
                }
                else
                {
                    incBet = m_lUserBetScore[wChairID] * m_AllConfig.m_BetRateVal * 0.1*m_AllConfig.m_AndroidRatio;
                }
                LOG(WARNING) << "更新彩金" << m_AllConfig.m_BetRateVal <<" "<<incBet;
                m_pITableFrame->UpdateJackpot((int32_t)eOpJackPotType::op_inc,incBet,(int32_t)eJackPotPoolId::jp_sgj,&m_newJackPot); 
            }

            //发送开奖结果
            int bRet = BuildRoundScene(wChairID, m_lUserBetScore[wChairID], isFreeGame);
            if(bRet > 0){
                LOG(WARNING) << "创建开奖结果失败!BuildRoundScene:" << bRet;
                SendErrorCode(wChairID,(enErrorCode)bRet); 
            }  

            //修改桌子状态
            m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);

            m_tLastOperate[wChairID] = time(NULL);

            m_Round++; 

            // m_pITableFrame->ClearTableUser(INVALID_CHAIR, true);
            return true;
        }
        case SUB_C_MARRY:
        {
            if (m_nMarrayNumLeft[wChairID] <= 0)
            {
                LOG(WARNING) << "玛丽开奖次数已经为0";
                return true;
            }

            //修改用户状态
            m_pITableFrame->SetUserReady(wChairID);
            //发送开奖结果
            int bRet = BuildMarryScene(wChairID);
            if(bRet > 0){
                LOG(WARNING) << "创建玛丽开奖结果失败!";
                SendErrorCode(wChairID,(enErrorCode)bRet);
            }
            
            //修改桌子状态
            m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
            return true;
        } 
        case SUB_C_JACKPOT_REC:
        { 
            CMD_C_ClientReq pClientReq;
            pClientReq.ParseFromArray(data, dwDataSize);
            int32_t nReq = pClientReq.nreqtype();   

            if (nReq == 0)
            {
                // 频率控制
                if (time(nullptr) - m_lastTick >= m_AllConfig.m_PullJackpotRecInterval)
                {
                    m_lastTick = (int64_t)time(nullptr);

                    Json::FastWriter writer;
                    // 整包
                    Json::Value root;
                    root["msgCount"] = m_AllConfig.m_MsgItemCount;
                    root["msgType"] = 1;//模式1为读,0为写
                    string jsonMsg = writer.write(root);
                    m_pITableFrame->CommonFunc(eCommFuncId::fn_sgj_jackpot_rec, jsonMsg, m_JackPotRecMsg);
                }

                //命令组装
                CMD_S_JackPot_Rec JackPotRec;
                JackPotRec.set_jackpotrecord(m_JackPotRecMsg);

                //序列化数据发送
                string data = JackPotRec.SerializeAsString();
                if (!m_bUserIsLeft){
                    m_pITableFrame->SendTableData(wChairID, SUB_S_JackPot_Rec, (uint8_t *)data.c_str(), data.size());
                }

                // LOG(WARNING) << "发送奖池记录:" << m_JackPotRecMsg;
            }

            return true;
        } 
    }
    return true;
}
// 发送彩金数据
void CTableFrameSink::SendJackPot()
{
    //读取0号彩金
    int64_t jackpotScore = m_pITableFrame->ReadJackpot((int32_t)eJackPotPoolId::jp_sgj);
    // LOG(WARNING) << "读取0号彩金："<< jackpotScore << " "<< m_lastJackpotScore;

    if (jackpotScore >= 0 && m_lastJackpotScore != jackpotScore){
        // LOG(WARNING) << "彩金有变动，刷新当前库存彩金 "<< m_lastJackpotScore << " "<< jackpotScore;
        //sync save
        m_lastJackpotScore = jackpotScore;
        // send data
        CMD_S_JackPot cmd_jackpot;
        cmd_jackpot.set_totaljackpot(jackpotScore);
        std::string data_jackpot = cmd_jackpot.SerializeAsString();
        if (!m_bUserIsLeft)
            m_pITableFrame->SendTableData(m_ChairId, SUB_S_JackPot, (uint8_t *)data_jackpot.c_str(), data_jackpot.size());
    }
}

//发送
bool CTableFrameSink::SendErrorCode(int32_t wChairID, enErrorCode enCodeNum)
{
    CMD_S_ErrorCode errorCode;
    //memset(&errorCode, 0, sizeof(errorCode));
    errorCode.set_cblosereason(enCodeNum);
    string data = errorCode.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SUB_S_ERROR_INFO, (uint8_t *)data.c_str(), data.size());
    return true;
}


//设置桌子
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{ 
    //更新游戏配置 

    assert(NULL != pTableFrame);
    m_pITableFrame = pTableFrame;
    if (m_pITableFrame)
    {
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        uint8_t jsize = m_pGameRoomInfo->jettons.size();
        for (uint8_t i = 0; i < jsize; i++)
        {
            m_cbJettonMultiple.push_back((uint8_t)m_pGameRoomInfo->jettons[i]);
        }

        m_dwCellScore = m_pGameRoomInfo->floorScore;
        m_replay.cellscore = m_dwCellScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
        m_TimerLoopThread = m_pITableFrame->GetLoopThread(); 
        //准入分数
        m_MinEnterScore = m_pGameRoomInfo->enterMinScore;
    }
    //初始化游戏数据
    m_wGameStatus = GAME_STATUS_INIT;


    //加载库存
    if(m_llStorageControl == 0){
        OnGameStart();
    }

    //获取游戏配置
    cfgManager = ConfigManager::GetInstance();
	if (cfgManager == NULL){
		LOG(WARNING) << "ConfigManager失败 ";
		return false;
    } 

	int ret = cfgManager->CheckReadConfig();
    if(ret != 0){
		LOG(WARNING) << "配置文件读取失败 " << ret;
		return false;
	}

    //获取配置文件
    memset(&m_AllConfig, 0, sizeof(m_AllConfig));
    ConfigManager::GetInstance()->GetConfig(m_AllConfig); 

    // LOG(WARNING) << "配置文件读取完成 " << ret;

    return true;
}


//创建旋转场景
int CTableFrameSink::BuildRoundScene(const int32_t wChairID, int64_t lbetScore, const bool IsFreeScene)
{
    if (IsFreeScene == false) {
        if (m_llUserSourceScore[wChairID] < lbetScore)// 分数太少
            return ERROR_SCORE_NOT_ENOUGH;
    }
    else{
        AssertReturn(m_nFreeRoundLeft[wChairID] > 0, return ERROR_STATUS);
    }

    //拉霸开奖结果命令 
    CMD_S_RoundResult_Tag CMDGameOneEnd; 

    int Count = 0;
    do
    {
        memset(&CMDGameOneEnd, 0, sizeof(CMDGameOneEnd));
        
        if ((++Count) > 500){
            LOG(WARNING) << "Count > 500";
            m_GameLogic.RandNoneArray(CMDGameOneEnd.m_Table, ICON_MAX);
            break;
        }
 
        //填充命令参数
        FillTable(CMDGameOneEnd);
 
        //检查行Scatter和 Bonus
        if (CheckRule(CMDGameOneEnd, IsFreeScene) == false) {  continue;  }
 
        //计算分数
        if (CalculateScore(CMDGameOneEnd, lbetScore / LINE_MAX) == false) { LOG(WARNING) << "Scatter和Bouns只能出一个"; continue; }

        //检查 Scatter 和 Bouns只能出一个
		if (CMDGameOneEnd.nFreeRoundLeft > 0 && CMDGameOneEnd.nMarryNum > 0) { LOG(WARNING) << "Scatter和Bouns只能出一个"; continue; }

		//免费旋转就不出免费了
		if (IsFreeScene == true && CMDGameOneEnd.nFreeRoundLeft > 0) { continue; }
        
        //押注库存验证
        if (CMDGameOneEnd.Score > 0L) {
           if(!CheckStorageVal(CMDGameOneEnd.Score)) {
                continue;
           }
        } 

        //免费没跑完不能出玛丽
        if (CMDGameOneEnd.nMarryNum > 0 && m_nFreeRoundLeft[wChairID] > 0 ) { 
            continue;
        } 
        //难度干涉值
        //m_difficulty
        m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        //难度值命中概率,全局难度,命中概率系统必须赚钱
        if(randomRet<m_difficulty)
        {
            if(CMDGameOneEnd.Score>0)
            {
                continue;
            }
        }
        //退出
        break;

    } while (true);

	if (CMDGameOneEnd.nFreeRoundLeft > 0) { 
		AssertReturn(CMDGameOneEnd.nFreeRoundLeft == 5 || CMDGameOneEnd.nFreeRoundLeft == 10 || CMDGameOneEnd.nFreeRoundLeft == 20, CMDGameOneEnd.nFreeRoundLeft = 5);
	}

	if (CMDGameOneEnd.nMarryNum >0) {
		AssertReturn(CMDGameOneEnd.nMarryNum == 3 || CMDGameOneEnd.nMarryNum == 5 || CMDGameOneEnd.nMarryNum == 10, CMDGameOneEnd.nMarryNum = 5);
	}

    // 计算出最后发给玩家的彩金
    int64_t curScatterJackPot = 0;//派奖前金额
    int64_t ScatterJackPot = 0;
    int64_t ScatterJackPotScore = 0;
    //scatter 数量
    if(CMDGameOneEnd.nFreeRoundLeft >= m_AllConfig.m_MiniJackPotId){
        int32_t idx = CMDGameOneEnd.nFreeRoundLeft == 10 ? 1 : 2;
        // 中奖奖励=奖池金额*20%*玩家单条线下注金额/100
        int64_t curJackPot = m_pITableFrame->ReadJackpot((int32_t)eJackPotPoolId::jp_sgj);
        if (curJackPot > 0)
        {
            curScatterJackPot = curJackPot;
            LOG(WARNING) << "放前彩金: " << curJackPot << " "<<m_AllConfig.m_MiniJackPotId<<" "<<m_AllConfig.m_ScatterJackPot[idx];
            // 1，取奖池金额的20%
            int64_t rewardJackpot = curJackPot * (m_AllConfig.m_ScatterJackPot[idx] * 0.01f);
            LOG(WARNING) << "抽取: " << m_AllConfig.m_ScatterJackPot[idx] <<" % "<< rewardJackpot;
            // 2，*玩家单条线下注金额/100


            rewardJackpot = rewardJackpot * ((lbetScore * 0.01) / (LINE_MAX * 100));
            LOG(WARNING) << "计算结果: " << rewardJackpot;
            // 余下彩金分数
            int64_t endJackPot = curJackPot - rewardJackpot;
            if(endJackPot < 0){
                LOG(ERROR) << "放后彩金小于0，有问题: " << endJackPot <<" "<< lbetScore<<" "<<curJackPot <<" "<<rewardJackpot;
                rewardJackpot = curJackPot;
                endJackPot = 0;
            }
            // 更新彩金分数
            int64_t endJackPotTemp = 0;
            m_pITableFrame->UpdateJackpot((int32_t)eOpJackPotType::op_inc, -rewardJackpot, (int32_t)eJackPotPoolId::jp_sgj, &endJackPotTemp);
            LOG(WARNING) << "放后彩金: " << endJackPot <<" "<<endJackPotTemp <<" 减少:"<< -rewardJackpot;
            // 计算出最后发给玩家的彩金(钱放大了1000倍,真正金额则*0.1即是获奖的分数)
            ScatterJackPot = rewardJackpot;
            // 真正金额分值
            ScatterJackPotScore = rewardJackpot/10; //*0.1
            LOG(WARNING) << "玩家获得彩金: " << ScatterJackPot << " "<< ScatterJackPotScore;
            // 把获奖分数也加到玩家分上
            CMDGameOneEnd.Score += ScatterJackPotScore;
            // 写奖池记录到redis
            int64_t userId = m_pITableFrame->GetTableUserItem(wChairID)->GetUserId();
            int64_t timestamp = (int64_t)time(nullptr);
            int32_t linebet = lbetScore / LINE_MAX;
            int32_t jackpot = ScatterJackPotScore;

            Json::FastWriter writer;
            Json::Value JsonReward;
            JsonReward["userid"] = (uint32_t)userId;
            JsonReward["timestamp"] = (uint32_t)timestamp;
            JsonReward["linebet"] = linebet;
            JsonReward["jackpot"] = jackpot;
            string jackpotMsg = writer.write(JsonReward);

            // 整包
            Json::Value root;
            root["msgCount"] = m_AllConfig.m_MsgItemCount;
            root["msgstr"] = jackpotMsg;
            root["msgType"] = 0;
            string jsonMsg = writer.write(root);
            string jsonOutMsg = "";
            m_pITableFrame->CommonFunc(eCommFuncId::fn_sgj_jackpot_rec, jsonMsg,jsonOutMsg);
            LOG(WARNING) << "写奖池记录到redis:" << jsonMsg;
        }
    }

    //记录当前图标数据
    iconResStr = "";
    int lineCount = 0;
    for (int i = 0; i < ICON_MAX; i++)
    {
        iconResStr += str(boost::format("%d")%CMDGameOneEnd.m_Table[i]);
        if(i < ICON_MAX - 1) iconResStr += ",";

        if(i < LINE_MAX && CMDGameOneEnd.Line[i] > 0) 
            lineCount++; 
    } 

    // 防止强制不开奖时的注单不正确
    if( lineCount == 0 ) tmpWinLinesInfo = "";

    string tmp97 = ScatterJackPotScore > 0 ? "---97":"";
    //记录
    int FreeCount = IsFreeScene?m_nFreeRoundLeft[wChairID]:CMDGameOneEnd.nFreeRoundLeft;
    string tmpRewardInfo = str(boost::format("%d,%d") % FreeCount % CMDGameOneEnd.nMarryNum);
    string tmpScatterWin = str(boost::format("%d-%d") % ScatterJackPotScore % curScatterJackPot);
    cardValueStr =str(boost::format("%s%d|%s|%s|%s|%s") % tmp97 % lineCount % tmpWinLinesInfo % tmpRewardInfo % iconResStr % tmpScatterWin);
    
    // LOG(WARNING) << cardValueStr;
    if(m_AllConfig.m_runAlg==1){
        string tempStr = str(boost::format("%5d|%s") % m_Round % cardValueStr);
        openLog(cardValueStr.c_str());
    }

    //写分
    int64_t llGameTax = 0;
    if (RoundSceneWriteScore(wChairID, IsFreeScene, lbetScore, CMDGameOneEnd.Score, llGameTax,cardValueStr) == false)
    {
        return ERROR_SIT_CHAIR_ID;
    }

    //保存数据
	m_lUserBetScore[wChairID] = lbetScore;
	m_lUserWinScore[wChairID] = CMDGameOneEnd.Score;
	m_nFreeRoundLeft[wChairID] += CMDGameOneEnd.nFreeRoundLeft;//免费不出，所以没有累加的影响
    if (IsFreeScene == true)
    {
        m_nFreeRoundLeft[wChairID]--;
        m_lFreeRoundWinScore[wChairID] += (CMDGameOneEnd.Score - llGameTax);
        if (m_nFreeRoundLeft[wChairID] <= 0)
        {
            m_nMarrayNumLeft[wChairID] = 0;
            m_nMarryNum[wChairID] = 0;
        }

        //统计使用
        m_lFreeWinScore += (CMDGameOneEnd.Score - llGameTax);

        //存数据
        WriteFreeGameRecord(wChairID,m_lUserBetScore[wChairID],m_nFreeRoundLeft[wChairID],m_nMarrayNumLeft[wChairID],m_nMarryNum[wChairID]); 
    }
    m_nMarryNum[wChairID] = CMDGameOneEnd.nMarryNum;
	m_nMarrayNumLeft[wChairID] = CMDGameOneEnd.nMarryNum;

	CMDGameOneEnd.Score -= llGameTax;
	CMDGameOneEnd.lCurrentScore = m_llUserSourceScore[wChairID];
	CMDGameOneEnd.nFreeRoundLeft = m_nFreeRoundLeft[wChairID];
	CMDGameOneEnd.lFreeRoundWinScore = m_lFreeRoundWinScore[wChairID];

    //免费转结束时清空分数
    if (m_nFreeRoundLeft[wChairID] <= 0)  m_lFreeRoundWinScore[wChairID] = 0;

#if DEBUG_INFO
    LOG(WARNING) << "lCurrentScore: " << CMDGameOneEnd.lCurrentScore;
    LOG(WARNING) << "Score: " << CMDGameOneEnd.Score;
    LOG(WARNING) << "nFreeRoundLeft: " << CMDGameOneEnd.nFreeRoundLeft;
    LOG(WARNING) << "lFreeRoundWinScore: " << CMDGameOneEnd.lFreeRoundWinScore;
    LOG(WARNING) << "nMarryNum: " << CMDGameOneEnd.nMarryNum;
#endif
    //
    if((m_lUserBetScore[wChairID]>0 && CMDGameOneEnd.Score/(m_lUserBetScore[wChairID]/9) >= m_AllConfig.m_MinBoardCastOdds)
    ||CMDGameOneEnd.Score > m_pGameRoomInfo->broadcastScore)
    {
        string muticle="";
        muticle = to_string(CMDGameOneEnd.Score/(m_lUserBetScore[wChairID]/9));
        m_pITableFrame->SendGameMessage(wChairID, muticle,SMT_GLOBAL|SMT_SCROLL, CMDGameOneEnd.Score);
    }

    //复制数据
    CMD_S_RoundResult cmdData;
    cmdData.set_score(CMDGameOneEnd.Score);
    cmdData.set_lcurrentscore(CMDGameOneEnd.lCurrentScore);
    cmdData.set_nfreeroundleft(CMDGameOneEnd.nFreeRoundLeft);
    cmdData.set_lfreeroundwinscore(CMDGameOneEnd.lFreeRoundWinScore);
    cmdData.set_nmarrynum(CMDGameOneEnd.nMarryNum);
    for (int i = 0; i < ICON_MAX; i++)
    {
        cmdData.add_m_table(CMDGameOneEnd.m_Table[i]);
        cmdData.add_tablelight(CMDGameOneEnd.TableLight[i]);

        if(i < LINE_MAX && CMDGameOneEnd.Line[i] > 0) {
            cmdData.add_line(i+1); 
        }
        if(i < 5)  
            cmdData.add_bspeedup(CMDGameOneEnd.bSpeedUp[i]);
    } 
    cmdData.set_roundid(mGameIds);
    cmdData.set_njackpot(ScatterJackPot); 

    std::string data = cmdData.SerializeAsString(); 
    if (!m_bUserIsLeft)
        m_pITableFrame->SendTableData(wChairID, SUB_S_NORMAL, (uint8_t *)data.c_str(), data.size());
     
    return 0;
}

bool CTableFrameSink::CheckStorageVal(int64_t winScore)
{
    int64_t temp = GetTotalStock() - GetLowerStock();
    if (winScore > max(m_AllConfig.m_CtrlVal * 100, temp / 3)){

        LOG(WARNING) << "winScore:" << winScore << " max:"<< temp;
        return false;
    } 
    return true;
} 

//构造玛丽开奖结果
int CTableFrameSink::BuildMarryScene(const int32_t wChairID)
{
    UserProbabilityItem Item;
    memset(&Item, 0, sizeof(Item));
    Item = ScoreGetProbability(GetTotalStock());
    //////////////////////////////////
    ///
    ///
    ///
    int  isHaveBlackPlayer=0;

    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton= m_lUserBetScore[user->GetChairId()];
        if(allJetton<=0) continue;
        if(user->GetBlacklistInfo().listRoom.size()>0)
        {
            auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
            if(it != user->GetBlacklistInfo().listRoom.end())
            {
                isHaveBlackPlayer=it->second;//
                //openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                //openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
            }
        }

    }
    if(isHaveBlackPlayer)
    {
        //user probilty count
        for(int i=0;i<GAME_PLAYER;i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(!user||user->IsAndroidUser())
            {
                continue;
            }
            int64_t allJetton=m_lUserBetScore[user->GetChairId()];;

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

                 {
                     Item.nBounsPro[IMAGE_WILD-1]=Item.nBounsPro[IMAGE_WILD-1]*(1-gailv);
                     //openSLog("黑名单玩家 :%d 确定调用黑名单算法 每门传的黑名单控制概率是[wild]：%d",user->GetUserId(),PersonConfig.nFruitPro[8]);
                 }
            }
        }
     }
    //////////////////////////////////
    int count = 0;
    int FruitIndex = 0;
    int64_t WinScore = 0;
    int32_t FruitOdd = 0L;


#if DEBUG_INFO
    int32_t tsum = 0L;
    for (int i = 0; i < 9; i++)
    {
        tsum += Item.nBounsPro[i];
        LOG(WARNING) <<  Item.nBounsPro[i];
    }
    LOG(WARNING) << "nACTotal: " <<tsum;
#endif

    do
    {  
		//取结果
		//FruitIndex = m_GameLogic.RandomFruitArea(9, Item.nBounsPro);
		//初始化
		m_MarryWeight.init(Item.nBounsPro, 9);
		//权值随机重排，可以不调用
		m_MarryWeight.shuffle();
		//返回随机索引，水果下标从1开始
		FruitIndex = m_MarryWeight.getResult() + 1;
		//LOG(DEBUG) << "************************ RandomFruitArea = " << FruitIndex;
        assert(FruitIndex >= IMAGE_CHERRY);
        assert(FruitIndex <= IMAGE_SEVEN);
    
        switch (FruitIndex)
        {
        case IMAGE_CHERRY:{     FruitOdd = 10;break;}
        case IMAGE_STRAWBERRY:{ FruitOdd =  15;break;}
        case IMAGE_ORANGE:{     FruitOdd =  20;break;}
        case IMAGE_LEMON:{      FruitOdd =  30;break;}
        case IMAGE_GRAPE:{      FruitOdd =  40;break;}
        case IMAGE_WATERMELON:{ FruitOdd =  50;break;}
        case IMAGE_BELL: {      FruitOdd =  60;break;}
        case IMAGE_STAR: {      FruitOdd =  80;break;} 
        case IMAGE_SEVEN:{      FruitOdd =  100;break;}
        default:
            return ERROR_GET_FRUIT;
        }
        //
        WinScore = (m_lUserBetScore[wChairID] * FruitOdd ) / LINE_MAX;

        //
        count++;
        if (count > 100)
        {
            FruitIndex = IMAGE_CHERRY;
            WinScore = (m_lUserBetScore[wChairID] * 10) / LINE_MAX ;
            LOG(WARNING) << "玛丽检查超时强开 "<< WinScore;
            break;
        }

        //超分检查
        if(CheckStorageVal(WinScore)) {
           break;
        }
        else
        {
           continue;
        }
        

    } while (true);
    
    //记录
    string tmpRewardInfo = str(boost::format("%d,%d") % m_nFreeRoundLeft[wChairID] % m_nMarrayNumLeft[wChairID]);
    cardValueStr = str(boost::format("0|%s|%s|%s") % "" % tmpRewardInfo % str(boost::format("%d,%d") % FruitIndex % FruitOdd));
    LOG(WARNING) << cardValueStr;

    //玛丽游戏写分
    int64_t llGameTax = 0;
    if (MarrySceneWriteScore(wChairID, WinScore, llGameTax,cardValueStr) == false) {
        return ERROR_OPERATE;
    }

    //玛丽减次数
    m_nMarrayNumLeft[wChairID]--;
    m_lMarryTotalWin[wChairID] += (WinScore- llGameTax);
     
    //命令组装
    CMD_S_MarryResult MarryEnd;
    MarryEnd.set_currentwin(WinScore - llGameTax);
    MarryEnd.set_totalwin(m_lMarryTotalWin[wChairID]);
    MarryEnd.set_fruit(FruitIndex);
    MarryEnd.set_userscore(m_llUserSourceScore[wChairID]);
    MarryEnd.set_nmarryleft(m_nMarrayNumLeft[wChairID]);
    MarryEnd.set_nmarrynum(m_nMarryNum[wChairID]);
    MarryEnd.set_roundid(mGameIds);
    
    //序列化数据发送
    string data = MarryEnd.SerializeAsString();
    if (!m_bUserIsLeft) {
        m_pITableFrame->SendTableData(wChairID, SUB_S_MARRY, (uint8_t *)data.c_str(), data.size());
    }

    //玛丽结束判断
    if (m_nMarrayNumLeft[wChairID] <= 0)  {
        m_nMarryNum[wChairID] = 0;
        m_lMarryTotalWin[wChairID] = 0L; 
    } 

    //实时保存数据
    WriteFreeGameRecord(wChairID,m_lUserBetScore[wChairID],m_nFreeRoundLeft[wChairID],m_nMarrayNumLeft[wChairID],m_nMarryNum[wChairID]); 
    return 0;
}

//写免费游戏次数记录
void CTableFrameSink::WriteFreeGameRecord(int32_t wChairID, int64_t betInfo,int32_t freeLeft,int32_t marryLeft,int32_t allMarry)
{
    return;//策划修改了方案

}

//分数获得概率
//@ StoreScore 库存值
//@ UserPro 个人概率（开奖概率）
UserProbabilityItem CTableFrameSink::ScoreGetProbability(int64_t StoreScore)//, UserProbabilityItem &UserPro)
{
    //权重线
    int StoreIndex = 1;
    if(StoreScore >= GetHighStock()) StoreIndex = 2; //放分
    else if(StoreScore < GetLowerStock()) StoreIndex = 0;//收分
    
    //测试算法
    if(m_AllConfig.m_runAlg==1)
    {
        StoreIndex = m_AllConfig.m_testWeidNum;
    }

    return m_AllConfig.vProbability[StoreIndex];
}

//填充拉霸表格
bool CTableFrameSink::FillTable(CMD_S_RoundResult_Tag &SceneOneEnd)
{ 
    UserProbabilityItem PersonConfig;
    memset(&PersonConfig, 0, sizeof(PersonConfig));
    PersonConfig =  ScoreGetProbability(GetTotalStock());
 
    
    //////////////////////////////////
    ///
    ///
    ///
    int  isHaveBlackPlayer=0;

    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton=m_lUserBetScore[user->GetChairId()];;
        if(allJetton<=0) continue;
        if(user->GetBlacklistInfo().listRoom.size()>0)
        {
            auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
            if(it != user->GetBlacklistInfo().listRoom.end())
            {
                isHaveBlackPlayer=it->second;//
                //openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                //openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
            }
        }

    }
    if(isHaveBlackPlayer)
    {
        //user probilty count
        for(int i=0;i<GAME_PLAYER;i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(!user||user->IsAndroidUser())
            {
                continue;
            }
            int64_t allJetton=m_lUserBetScore[user->GetChairId()];;

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

                 {
                     PersonConfig.nFruitPro[IMAGE_WILD-1]=PersonConfig.nFruitPro[IMAGE_WILD-1]*(1-gailv);
                     //openSLog("黑名单玩家 :%d 确定调用黑名单算法 每门传的黑名单控制概率是[wild]：%d",user->GetUserId(),PersonConfig.nFruitPro[8]);
                 }
            }
        }
     }
    //////////////////////////////////
    /// //////////////////////////////

    int IsHaveSpecil[5] = {};
    memset(IsHaveSpecil, 0, sizeof(IsHaveSpecil));

    //测试打印
    char szFile[256] = { 0 };
    char szTemp[64] = { 0 };

#if DEBUG_INFO
    int32_t tsum = 0L;
    for (int i = 0; i < 13; i++)
    {
        tsum += PersonConfig.nFruitPro[i];
        LOG(WARNING) <<  PersonConfig.nFruitPro[i];
    }
    LOG(WARNING) << " Fruit Total: " <<tsum;
#endif
 
    int ret = 0;
    for (int Index = 0; Index < ICON_MAX; Index++)
    {
        int ProbabilityIndex = IsHaveSpecil[Index % 5] >= 1 ? 10 : 12;
        //ret = m_GameLogic.RandomFruitArea(ProbabilityIndex, PersonConfig.nFruitPro);
		//初始化
		m_FruitWeight.init(PersonConfig.nFruitPro, ProbabilityIndex);
		//权值随机重排，可以不调用
		m_FruitWeight.shuffle();
		//返回随机索引，水果下标从1开始
		ret = m_FruitWeight.getResult() + 1;
		//LOG(DEBUG) << "************************ RandomFruitArea = " << ret;
        if (m_AllConfig.m_testLine == 1) {
            ret = m_AllConfig.m_line[Index/5][Index%5];
            if (ret == 0)  ret = m_random.betweenInt(IMAGE_CHERRY, IMAGE_SEVEN).randInt_mt(true);
        }
        else if (m_AllConfig.m_testLine == 2){ 
            if( IsHaveSpecil[Index % 5] == 0){
                if(m_random.betweenInt(0,3).randInt_mt(true) ==0)
                   ret=IMAGE_SCATTER;
                else
                   ret = m_random.betweenInt(IMAGE_CHERRY,IMAGE_SEVEN).randInt_mt(true);
            } 
        }
         else if (m_AllConfig.m_testLine == 3){  
             if( IsHaveSpecil[Index % 5] == 0){
                if(m_random.betweenInt(0,3).randInt_mt(true) ==0)
                   ret=IMAGE_BONUS;
                else
                   ret = m_random.betweenInt(IMAGE_CHERRY,IMAGE_SEVEN).randInt_mt(true);
            } 
        }
       
        
          
        //范围断言
        assert(ret > 0 && ret < FRUIT_MAX);

        if (ret == IMAGE_SCATTER || ret == IMAGE_BONUS){
            IsHaveSpecil[Index % 5] = 1;
        } 

        SceneOneEnd.m_Table[Index] = ret;
    } 

    #if DEBUG_INFO
    //测试打印
    for (int Index = 0; Index < ICON_MAX; Index++)
    {
        snprintf(szTemp, sizeof(szTemp), "%d ", SceneOneEnd.m_Table[Index]);
        strcat(szFile,szTemp);  
        if(Index % 5 == 4){
            LOG(WARNING) <<szFile;
            strcpy(szFile,"");
        }
    } 
    #endif

    return true;
}

//scatter和bonus规则检查
bool CTableFrameSink::CheckRule(CMD_S_RoundResult_Tag &SceneOneEnd, bool IsFreeScene)
{
    int Specil[5] = {0}; 
    for (int Index = 0; Index < ICON_MAX; Index++)
    {
        //统计总共有多少个 SCATTER  BONUS
        if (SceneOneEnd.m_Table[Index] == IMAGE_SCATTER || SceneOneEnd.m_Table[Index] == IMAGE_BONUS)
        {
            Specil[Index % 5]++;
        }

        assert(SceneOneEnd.m_Table[Index] >= IMAGE_CHERRY);
		assert(SceneOneEnd.m_Table[Index] <= IMAGE_BONUS);
    }
    for (int Index = 0; Index < 5; Index++)
    {
        if (Specil[Index] > 1)
        {
            LOG(WARNING) << "scatter和bonus规则检查 错误"<<Specil[Index];
            return false;
        }
    }
    return true;
}
//计算分数
bool CTableFrameSink::CalculateScore(CMD_S_RoundResult_Tag &SceneOneEnd, int64_t SingleLine)
{
    for (size_t i = 0; i < ICON_MAX; i++)
    {
        AssertReturn(SceneOneEnd.m_Table[i] > 0 && SceneOneEnd.m_Table[i] < FRUIT_MAX, return false);
    }

    tmpWinLinesInfo = "";

    //计算分数
    for (int LineIndex = 0; LineIndex < LINE_MAX; LineIndex++)
    {
        int Multi = 0;
        int FruitIndex = 0;
        int FruitNum = 0;
        Multi = m_GameLogic.Multi_Line(FruitIndex, FruitNum, SceneOneEnd.m_Table, SceneOneEnd.TableLight, ICON_MAX, LineIndex);
        
        SceneOneEnd.Score += Multi * SingleLine;
       
        if (Multi > 0)
        {
#if DEBUG_INFO
             LOG(WARNING) << " 中奖线: "<< LineIndex << " " << SceneOneEnd.Score << " = " << Multi << " * "<< SingleLine;
#endif
            //增加
            if(!tmpWinLinesInfo.empty()) tmpWinLinesInfo += ",";

            tmpWinLinesInfo += str(boost::format("%d-%d") % LineIndex % Multi);

            //设置中奖的线
            SceneOneEnd.Line[LineIndex] = 1;
        }
        if (FruitNum >= 3)
        {
           SceneOneEnd.bSpeedUp[4] = true;
        }
    }

    // LOG(WARNING) <<"tmpWinLinesInfo:"<< tmpWinLinesInfo;

    //免费次数
    {
        int IconScatterNum = 0;
        int IconScatter[5] = {0};

        for (int Index = 0; Index < ICON_MAX; Index++)
        {
           if (SceneOneEnd.m_Table[Index] == IMAGE_SCATTER)
			{
                AssertReturn(IconScatter[Index % 5] == 0, return false);
                IconScatter[Index % 5] = IMAGE_SCATTER;
                IconScatterNum++;
            }
        }
        if (IconScatterNum >= 2)
        {
            //紧张感
            int num = 0;
            for (int Index = 0; Index < 5; Index++)
            {
                if (num >= 2)
                {
                   SceneOneEnd.bSpeedUp[Index] = true;
                }
                if (IconScatter[Index] == IMAGE_SCATTER)
                {
                    num++;
                }
            }
            //免费次数
            if (IconScatterNum > 2)
            {
                if (IconScatterNum == 3)
                {
                    SceneOneEnd.nFreeRoundLeft = 5;
                }
                else if (IconScatterNum == 4)
                {
                    SceneOneEnd.nFreeRoundLeft = 10;
                }
                else if (IconScatterNum == 5)
                {
                    SceneOneEnd.nFreeRoundLeft = 20;
                }
                else
                {
                    LOG(ERROR) << "免费次数错误,IconScatterNum " << IconScatterNum;
                    assert(false);
                }
               
                //亮灯
                for (int Index = 0; Index < ICON_MAX; Index++)
                {
                   if (SceneOneEnd.m_Table[Index] == IMAGE_SCATTER)
					{
						SceneOneEnd.TableLight[Index] = 1;
					}
                }
            }
        }
    }
    
    //小游戏
    {
        int IconBonusNum = 0;
        int IconBouns[5] = {0};
       
        for (int Index = 0; Index < ICON_MAX; Index++)
        {
            if (SceneOneEnd.m_Table[Index] == IMAGE_BONUS)
            {
                AssertReturn(IconBouns[Index % 5] == 0, return false);
                IconBouns[Index % 5] = IMAGE_BONUS;
                IconBonusNum++;
            }
        }
        if (IconBonusNum >= 2)
        {
            //紧张感
            int num = 0;
            for (int Index = 0; Index < 5; Index++)
            {
                if (num >= 2)
                {
                    SceneOneEnd.bSpeedUp[Index] = true;
                }
                if (IconBouns[Index] == IMAGE_BONUS)
                {
                    num++;
                }
            }
            //免费次数
            if (IconBonusNum > 2)
            {
                if (IconBonusNum == 3)
				{
					SceneOneEnd.nMarryNum = 3;
				}
				else if (IconBonusNum == 4)
				{
					SceneOneEnd.nMarryNum = 5;
				}
				else if (IconBonusNum == 5)
				{
					SceneOneEnd.nMarryNum = 10;
				}
				else {
                    LOG(ERROR) << "玛丽次数错误,IconBonusNum " << IconBonusNum;
					assert(false);
				}

                //亮灯
                for (int Index = 0; Index < ICON_MAX; Index++)
				{
					if (SceneOneEnd.m_Table[Index] == IMAGE_BONUS)
					{
						SceneOneEnd.TableLight[Index] = 1;
					}
				}
            }
        }
        
    }
    return true;
}
//玛丽游戏写分
bool CTableFrameSink::MarrySceneWriteScore(const int32_t wChairID, const int64_t lWinScore, int64_t &lGameTax,string cardValueStr)
{
    AssertReturn(wChairID < GAME_PLAYER, return false);
    AssertReturn(lWinScore > 0L, return false);

    int64_t lplayerWinScore = lWinScore;

    m_pGameRoomInfo->totalStock -= lplayerWinScore;
    m_llTodayStorage -= lplayerWinScore;

    //计算税收
    lGameTax = 0;// m_pITableFrame->CalculateRevenue(lplayerWinScore);

    //写积分
    tagScoreInfo ScoreData;
    ScoreData.chairId = wChairID;
    ScoreData.addScore = lplayerWinScore - lGameTax;         // 当局输赢分数 
    ScoreData.betScore = 0;//m_lUserBetScore[wChairID]; // 总压注
    ScoreData.revenue = lGameTax;                   // 当局税收
    ScoreData.rWinScore = 0;//lWinScore;                //有效投注额：税前输赢
    //牌型
    ScoreData.cardValue = cardValueStr;
    ScoreData.startTime = m_startTime;

    //统计用
    m_lMarryWinScore += lplayerWinScore;

    m_llUserSourceScore[wChairID] += ScoreData.addScore;
 

#if 1//!RUN_ALGORITHM
    // string m_strRoundId = m_pITableFrame->GetNewRoundId();
    m_pITableFrame->WriteUserScore(&ScoreData,1,mGameIds);

    if (!m_isAndroid){
        //更新系统库存
        m_pITableFrame->UpdateStorageScore((-lplayerWinScore));

        m_replay.addResult(wChairID, wChairID, m_lUserBetScore[wChairID]/LINE_MAX, ScoreData.addScore, "", false); 
        m_replay.addStep((uint32_t)time(NULL) - groupStartTime, "", -1, 0, wChairID, wChairID);

        //保存对局结果
        m_pITableFrame->SaveReplay(m_replay);
    }
#endif

#if DEBUG_INFO
    LOG(WARNING) << "总库存: " << GetTotalStock() * TO_DOUBLE << ", 最低:" << GetLowerStock() * TO_DOUBLE << ", 今日库存: "\
        << m_llTodayStorage * TO_DOUBLE << ", 最高:" << GetHighStock() * TO_DOUBLE;
#endif
 
    return true;
}
//拉霸写分
bool CTableFrameSink::RoundSceneWriteScore(const int32_t wChairID, const bool IsFreeScene,
                                           const int64_t lBetScore, const int64_t lWinScore, int64_t &lGameTax,string cardValueStr)
{
    AssertReturn(wChairID < GAME_PLAYER, return false);
 
    int64_t lplayerWinScore = 0;
    if (IsFreeScene)
    {
        lplayerWinScore = lWinScore;
    }
    else
    {
        lplayerWinScore = lWinScore - lBetScore;//0 - 900
    }

    m_pGameRoomInfo->totalStock -= lplayerWinScore;
    m_llTodayStorage -= lplayerWinScore;

    //算税收
    if (lWinScore > 0)  lGameTax = 0;//m_pITableFrame->CalculateRevenue(lWinScore);
       
    //写积分
    tagScoreInfo ScoreData;
    ScoreData.chairId = wChairID;
    ScoreData.addScore = lplayerWinScore - lGameTax; // 当局输赢分数 
    ScoreData.betScore = IsFreeScene?0:lBetScore;         // 总压注
    ScoreData.revenue = lGameTax;           // 当局税收
    ScoreData.rWinScore = ScoreData.betScore;//lBetScore;//lWinScore; //有效投注额：税前输赢
    ScoreData.cardValue = cardValueStr;
    ScoreData.startTime = m_startTime;

    m_llUserSourceScore[wChairID] += ScoreData.addScore;

#if 1//!RUN_ALGORITHM

    m_pITableFrame->WriteUserScore(&ScoreData,1,mGameIds); 

    if (!m_isAndroid)
    {
        //更新系统库存
        double res = (-lplayerWinScore);
        if(res>0)
        {
            res=res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
        }
        m_pITableFrame->UpdateStorageScore(res);

        m_replay.addResult(wChairID, wChairID, ScoreData.betScore, ScoreData.addScore, "", false);
        m_replay.addStep((uint32_t)time(NULL) - groupStartTime, "", -1, 0, wChairID, wChairID);
        //保存对局结果
        m_pITableFrame->SaveReplay(m_replay);
    }

#endif

#if DEBUG_INFO 
    LOG(WARNING) << "总库存: " << GetTotalStock() * TO_DOUBLE << ", 最低:" << GetLowerStock() * TO_DOUBLE << ", 今日库存: "\
        << m_llTodayStorage * TO_DOUBLE << ", 最高:" << GetHighStock() * TO_DOUBLE <<" 总次数 "<< m_Round << " 免费次数 "<< m_FreeRound << " 玛丽次数 " << m_MarryRound;
#endif
 
    return true;
}

// 踢出数据
void CTableFrameSink::OnTimerClearUser()
{
    shared_ptr<IServerUserItem> pUserItem;
    for(int i=0;i < GAME_PLAYER;i++)
    {
        pUserItem = m_pITableFrame->GetTableUserItem(i);
        if( pUserItem )
        {
            pUserItem->setOffline(); 
        }
    }
 
    LOG(WARNING) << "踢出!";
}

//检查是否跨天
void CTableFrameSink::ResetTodayStorage()
{
	//检查是否跨天
	uint32_t dwTimeNow = (uint32_t)time(NULL);
    bool bAcrossDay = isAcrossTheDay(m_dwTodayTime, dwTimeNow);
	if (bAcrossDay)
	{
		m_llTodayStorage = 0;
		m_dwTodayTime = dwTimeNow;
	}
}

bool CTableFrameSink::isAcrossTheDay(uint32_t curTime,uint32_t oldTime)
{

    return  false;
}

//清理游戏数据
void CTableFrameSink::ClearGameData(int dwChairID)
{
    m_lMarryTotalWin[dwChairID] = 0L;
    m_lUserWinScore[dwChairID] = 0L;
    m_lUserBetScore[dwChairID] = 0L;
    m_nMarrayNumLeft[dwChairID] = 0;
    m_nFreeRoundLeft[dwChairID] = 0;
    m_nMarryNum[dwChairID] = 0;
    m_lFreeRoundWinScore[dwChairID] = 0L;
    m_bIsSpecilUser[dwChairID] = false;

    m_tLastOperate[dwChairID] = 0L;
}
//复位桌子
void CTableFrameSink::RepositionSink()
{
#if DEBUG_INFO
    LOG(WARNING) << "复位桌子 round:"<<m_Round;
#endif
}


void  CTableFrameSink::openLog(const char *p, ...)
{
    // if (m_IsEnableOpenlog)
    {

        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        // sprintf(Filename, "./log/Sgj/Sgj_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        sprintf(Filename, "./log/Sgj/Sgj_%d.log",m_AllConfig.m_testWeidNum);
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        // fprintf(fp, "%s", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink> &pSink)
{
    pSink.reset();
}
