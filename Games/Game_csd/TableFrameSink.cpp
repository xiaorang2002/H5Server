#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
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
#include "proto/csd.Message.pb.h"

#include "json/json.h"
#include "ConfigManager.h"
#include "CMD_Game.h"
#include "GameLogic.h"
#include "ConfigManager.h"

using namespace csd;
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

const double_t m_mutical[10][3]={{0.2,0.6,3},{0.2,0.6,3},{0.2,0.8,4},{0.2,1.2,4},{0.2,1.2,4},{0.6,2,8},{0.6,4,10},{0.6,4,10},{1,6,12},{0,0,0}};


//游戏开始

using namespace std;

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/csd";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if (!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("csd");
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
    if(m_isAndroid){
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

    m_kickTime++;
    if(m_kickTime >= 30)
    {
        int ret = cfgManager->CheckReadConfig();
        if(ret != 0){
            LOG(WARNING) << "配置文件读取失败 " << ret;
        }
        //获取配置文件
        memset(&m_AllConfig, 0, sizeof(m_AllConfig));
        ConfigManager::GetInstance()->GetConfig(m_AllConfig);
        m_kickTime = 0;
    }


    //
    if (false )
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
    //m_kickTime = 0;

    m_replay.clear(); 
    mGameIds = m_pITableFrame->GetNewRoundId();
    m_replay.gameinfoid = mGameIds;

    pUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    m_replay.addPlayer(pUserItem->GetUserId(), pUserItem->GetAccount(), pUserItem->GetUserScore(), wChairID);

    //记录原始积分
    m_llUserSourceScore[wChairID] = pUserItem->GetUserScore();

    switch (subid)
    {
        case SUB_C_START:
        {


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

                LOG_WARN << "玩家下注金额 "<<m_lUserBetScore[wChairID];
            }


            vector<int> vBetItemList = ConfigManager::GetInstance()->GetBetItemList();
            bool isRight =false;
            for(int betItem :vBetItemList)
            {
                if(betItem == m_lUserBetScore[wChairID] )
                {
                    isRight = true;
                    break;
                }
                LOG_WARN << "下注项分数 "<<betItem;

            }
            if(m_lUserBetScore[wChairID]<=0||!isRight)
            {
                LOG_WARN << "有狗逼玩家搞事情 ";
                return false ;
            }
            m_replay.cellscore = m_lUserBetScore[wChairID] ;

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
            if ((time(NULL) - m_tLastOperate[wChairID]) < 2&&!isFreeGame)
            {
                LOG(WARNING) << "拉动太频繁了 " << m_nPullInterval << " "<<wChairID <<" UserId:"<<pUserItem->GetUserId();
                return false;
            }


    
            //修改用户状态
            m_pITableFrame->SetUserReady(wChairID);
            //清除数据
            m_nMarrayNumLeft[wChairID] = 0;
            m_nMarryNum[wChairID] = 0;

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

    }
    return true;
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

void CTableFrameSink::CountIsWin(vector<resultStr> &resultArrVec, CMD_S_RoundResult_Tag &SceneOneEnd,int32_t allBet)
{

    resultArrVec.clear();

    vector<fruitWin> winFruit[10];
    vector<fruitWin> fiveLieVec[10][5];  //每种水果分成五列放
    bool isTrue[10][5];  //记录是不是连续的
    bool isWinFruit[10]; //水果是否中奖

    int32_t t_table[3][5];

    for(int i=0;i<3;i++)
    {
        for(int j=0;j<5;j++)
        {
            t_table[i][j] = SceneOneEnd.m_Table[i*5+j];
        }
    }
    //初始化结果数组
    for(int i=0;i<10;i++)
    {
        isWinFruit[i] =false;
        for(int j=0;j<5;j++)
        {
            isTrue[i][j] = false;
            fiveLieVec[i][j].clear();
        }
    }
    //应该是每种图标只能中将一次,那么就是每种图标判断一次就可以了
    for(int i=0;i<10;i++)
    {
        winFruit[i].clear();
        for(int k=0;k<3;k++)
        {
           for(int j=0;j<5;j++)
           {
               //财神到或者等于当前图标
                if(i!=9&&(t_table[k][j]==i||t_table[k][j]==10))
                {
                   isTrue[i][j] = true;
                   fruitWin fruit;
                   fruit.fruit = i;
                   fruit.heng = k;
                   fruit.lie = j;
                   fruit.index=j+k*5;
                   winFruit[i].push_back(fruit);
                }
                else if(i==9&&t_table[k][j]==i)
                {
                    isTrue[i][j] = true;
                    fruitWin fruit;
                    fruit.fruit = i;
                    fruit.heng = k;
                    fruit.lie = j;
                    fruit.index=j+k*5;
                    winFruit[i].push_back(fruit);
                }
           }
        }
    }

    int32_t count[10] = {0};
    for(int i=0;i<10;i++)
    {

        for(int j=0;j<5;j++)
        {
            if(isTrue[i][j])
            {
               count[i]++; //算出最多有多少列
            }
            else
            {
                break;
            }
        }
        //达不到中奖目标全部清空
        if(winFruit[i].size()<=2||count[i]<=2)
        {
            for(int j=0;j<5;j++)
            {
                isTrue[i][j] = false;
                fiveLieVec[i][j].clear();
            }
            isWinFruit[i] = false;
        }
        else
        {
            isWinFruit[i] = true;
            for(int k=0;k<count[i];k++)
            {
                for(int x=0;x<winFruit[i].size();x++)
                {
                    if(k==winFruit[i][x].lie)
                    {
                        fiveLieVec[i][k].push_back(winFruit[i][x]);
                    }
                }
            }
            //只有三种情况出现三连，四连，五连
            switch (count[i]) {
            case 3:
                {
                    for(int x=0;x<fiveLieVec[i][0].size();x++)
                    {

                        for(int c=0;c<fiveLieVec[i][1].size();c++)
                        {

                            for(int v=0;v<fiveLieVec[i][2].size();v++)
                            {
                                resultStr arr;
                                arr.count = 3;
                                arr.fruit = i;
                                arr.fruitVct.clear();
                                arr.fruitVct.push_back(fiveLieVec[i][0][x]);
                                arr.fruitVct.push_back(fiveLieVec[i][1][c]);
                                arr.fruitVct.push_back(fiveLieVec[i][2][v]);
                                if(i!=9)
                                {
                                   arr.isFree = false;
                                   arr.isWin =true;
                                }
                                else
                                {
                                    arr.isFree = true;
                                    arr.isWin =false;
                                    SceneOneEnd.nFreeRoundLeft=10;
                                    LOG(WARNING) << "本次中免费开奖3 " ;

                                }
                                arr.mutic =  m_mutical[i][0];
                                arr.winScore = arr.mutic*allBet;
                                resultArrVec.push_back(arr);
                                SceneOneEnd.Score+=arr.winScore;
                                Results res;
                                res.fruit = i;
                                res.score = arr.winScore;
                                res.indexs.clear();
                                res.indexs.push_back(fiveLieVec[i][0][x].index);
                                res.indexs.push_back(fiveLieVec[i][1][c].index);
                                res.indexs.push_back(fiveLieVec[i][2][v].index);
                                SceneOneEnd.vResults.push_back(res);
                            }
                        }
                    }
                }
                break;
            case 4:
                {
                    for(int x=0;x<fiveLieVec[i][0].size();x++)
                    {

                        for(int c=0;c<fiveLieVec[i][1].size();c++)
                        {

                            for(int v=0;v<fiveLieVec[i][2].size();v++)
                            {

                                for(int b=0;b<fiveLieVec[i][3].size();b++)
                                {

                                    resultStr arr;
                                    arr.count = 4;
                                    arr.fruit = i;
                                    arr.fruitVct.clear();
                                    arr.fruitVct.push_back(fiveLieVec[i][0][x]);
                                    arr.fruitVct.push_back(fiveLieVec[i][1][c]);
                                    arr.fruitVct.push_back(fiveLieVec[i][2][v]);
                                    arr.fruitVct.push_back(fiveLieVec[i][3][b]);
                                    if(i!=9)
                                    {
                                       arr.isFree = false;
                                       arr.isWin =true;

                                    }
                                    else
                                    {
                                        arr.isFree = true;
                                        arr.isWin =false;
                                        SceneOneEnd.nFreeRoundLeft=10;
                                        LOG(WARNING) << "本次中免费开奖4 " ;
                                    }
                                    arr.mutic =  m_mutical[i][1];
                                    arr.winScore = arr.mutic*allBet;
                                    resultArrVec.push_back(arr);
                                    SceneOneEnd.Score+=arr.winScore;
                                    Results res;
                                    res.fruit = i;
                                    res.score = arr.winScore;
                                    res.indexs.clear();
                                    res.indexs.push_back(fiveLieVec[i][0][x].index);
                                    res.indexs.push_back(fiveLieVec[i][1][c].index);
                                    res.indexs.push_back(fiveLieVec[i][2][v].index);
                                    res.indexs.push_back(fiveLieVec[i][3][b].index);
                                    SceneOneEnd.vResults.push_back(res);
                                }
                            }
                        }
                    }
                }
                break;
            case 5:
                {
                    for(int x=0;x<fiveLieVec[i][0].size();x++)
                    {
                        for(int c=0;c<fiveLieVec[i][1].size();c++)
                        {
                            for(int v=0;v<fiveLieVec[i][2].size();v++)
                            {
                                for(int b=0;b<fiveLieVec[i][3].size();b++)
                                {
                                    for(int n=0;n<fiveLieVec[i][4].size();n++)
                                    {

                                        resultStr arr;
                                        arr.count = 5;
                                        arr.fruit = i;
                                        arr.fruitVct.clear();
                                        arr.fruitVct.push_back(fiveLieVec[i][0][x]);
                                        arr.fruitVct.push_back(fiveLieVec[i][1][c]);
                                        arr.fruitVct.push_back(fiveLieVec[i][2][v]);
                                        arr.fruitVct.push_back(fiveLieVec[i][3][b]);
                                        arr.fruitVct.push_back(fiveLieVec[i][4][n]);
                                        if(i!=9)
                                        {
                                           arr.isFree = false;
                                           arr.isWin =true;

                                        }
                                        else
                                        {
                                            arr.isFree = true;
                                            arr.isWin =false;
                                            SceneOneEnd.nFreeRoundLeft=10;
                                            LOG(WARNING) << "本次中免费开奖5 " ;
                                        }
                                        arr.mutic =  m_mutical[i][2];
                                        arr.winScore = arr.mutic*allBet;
                                        resultArrVec.push_back(arr);
                                        SceneOneEnd.Score+=arr.winScore;
                                        Results res;
                                        res.fruit = i;
                                        res.score = arr.winScore;
                                        res.indexs.clear();
                                        res.indexs.push_back(fiveLieVec[i][0][x].index);
                                        res.indexs.push_back(fiveLieVec[i][1][c].index);
                                        res.indexs.push_back(fiveLieVec[i][2][v].index);
                                        res.indexs.push_back(fiveLieVec[i][3][b].index);
                                        res.indexs.push_back(fiveLieVec[i][4][n].index);
                                        SceneOneEnd.vResults.push_back(res);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }

        }

    }


    //到这里只要长度不为零的容器对应的水果就是在中将了

}

//创建旋转场景
int CTableFrameSink::BuildRoundScene(const int32_t wChairID, int64_t lbetScore, const bool IsFreeScene)
{
    //假如没在免费状态，就要判断分数
    if (IsFreeScene == false)
    {
        if (m_llUserSourceScore[wChairID] < lbetScore)// 分数太少
            return ERROR_SCORE_NOT_ENOUGH;
    }
    else
    {
        AssertReturn(m_nFreeRoundLeft[wChairID] > 0, return ERROR_STATUS);
    }

    //拉霸开奖结果命令 
    CMD_S_RoundResult_Tag CMDGameOneEnd; 

    int Count = 0;
    Json::Value root;
    Json::FastWriter write;
    do
    {
        memset(&CMDGameOneEnd, 0, sizeof(CMDGameOneEnd));
        //要是五百次都没有合适的结果给个不中奖的组合
        if ((++Count) > 500)
        {
            LOG(WARNING) << "Count > 500";
            m_GameLogic.RandNoneArray(CMDGameOneEnd.m_Table, ICON_MAX);
            break;
        }
 
        //产生一个基础的结果
        FillTable(CMDGameOneEnd,wChairID);
 
        //检查十五个图标的合法性
        if (CheckRule(CMDGameOneEnd, IsFreeScene) == false) {  continue;  }

        vector<resultStr> resultArrVec;
        //计算分数
        CountIsWin(resultArrVec,CMDGameOneEnd,lbetScore);
        //在这里写后台日志
        tmpWinLinesInfo="";
        root.clear();
        Json::Value friends;
        for(int i =0;i<CMDGameOneEnd.vResults.size();i++)
        {
            Json::Value value;
            value["fruit"] = CMDGameOneEnd.vResults[i].fruit;
            value["num"] = CMDGameOneEnd.vResults[i].indexs.size();
            value["score"] = CMDGameOneEnd.vResults[i].score;
            friends.append(value);
        }
        root["jiang"]=friends;
        //tmpWinLinesInfo = write.write(root);
		//免费旋转就不出免费了
        if (IsFreeScene == true && CMDGameOneEnd.nFreeRoundLeft > 0)
        {
            LOG(ERROR)<<" 免费旋转，又出免费中奖 ";
            continue;
        }
        
        //押注库存验证
        if (CMDGameOneEnd.Score > 0L)
        {
           if(!CheckStorageVal(CMDGameOneEnd.Score))
           {
                continue;
           }
        } 
        if(CMDGameOneEnd.Score > m_AllConfig.m_BestWinNum)
        {

            continue;
        }
        LOG(ERROR)<<"m_BestWinNum : "<<m_AllConfig.m_BestWinNum;
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
//	if (CMDGameOneEnd.nFreeRoundLeft > 0)
//    {
//		AssertReturn(CMDGameOneEnd.nFreeRoundLeft == 5 || CMDGameOneEnd.nFreeRoundLeft == 10 || CMDGameOneEnd.nFreeRoundLeft == 20, CMDGameOneEnd.nFreeRoundLeft = 5);
//	}


    //记录当前图标数据
    iconResStr = "";
    for (int i = 0; i < ICON_MAX; i++)
    {
        iconResStr += str(boost::format("%d")%CMDGameOneEnd.m_Table[i]);
        if(i < ICON_MAX - 1) iconResStr += ",";
    } 
     int FreeCount = IsFreeScene?m_nFreeRoundLeft[wChairID]:CMDGameOneEnd.nFreeRoundLeft;
    string tmpRewardInfo = str(boost::format("%d") % FreeCount );
    root["card"]=iconResStr;
    root["free"]=tmpRewardInfo;
    //记录
    cardValueStr= write.write(root);

    //cardValueStr =str(boost::format("%s|%s|%s") % tmpWinLinesInfo % tmpRewardInfo % iconResStr);
    
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

    LOG(WARNING) << "m_nFreeRoundLeft[wChairID]:" << m_nFreeRoundLeft[wChairID] << " CMDGameOneEnd.nFreeRoundLeft:"<< CMDGameOneEnd.nFreeRoundLeft;
    if (IsFreeScene == true)
    {
        m_nFreeRoundLeft[wChairID]--;
        m_lFreeRoundWinScore[wChairID] += (CMDGameOneEnd.Score - llGameTax);

        //统计使用
        m_lFreeWinScore += (CMDGameOneEnd.Score - llGameTax);

        //存数据
        WriteFreeGameRecord(wChairID,m_lUserBetScore[wChairID],m_nFreeRoundLeft[wChairID],m_nMarrayNumLeft[wChairID],m_nMarryNum[wChairID]); 
    }
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

    for (int i = 0; i < ICON_MAX; i++)
    {
        cmdData.add_m_table(CMDGameOneEnd.m_Table[i]);

    } 
    if(CMDGameOneEnd.vResults.size()>0)
    {
        for(int i=0;i<CMDGameOneEnd.vResults.size();i++)
        {
            CMD_S_Results * pResult = cmdData.add_line();
            pResult->set_fruit(CMDGameOneEnd.vResults[i].fruit);
            pResult->set_winscore(CMDGameOneEnd.vResults[i].score);
            for(int j=0;j<CMDGameOneEnd.vResults[i].indexs.size();j++)
            {
                pResult->add_index(CMDGameOneEnd.vResults[i].indexs[j]);
            }
        }
    }
    cmdData.set_roundid(mGameIds);

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

//写免费游戏次数记录
void CTableFrameSink::WriteFreeGameRecord(int32_t wChairID, int64_t betInfo,int32_t freeLeft,int32_t marryLeft,int32_t allMarry)
{
    return;//策划修改了方案
    tagSgjFreeGameRecord pSgjRecord;
    pSgjRecord.userId = m_pITableFrame->GetTableUserItem(wChairID)->GetUserId();
    pSgjRecord.betInfo = betInfo;
    pSgjRecord.freeLeft = freeLeft;
    pSgjRecord.marryLeft = marryLeft; 
    pSgjRecord.allMarry = allMarry;
    m_pITableFrame->WriteSgjFreeGameRecord(&pSgjRecord);
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
    if(m_nFreeRoundLeft[0]>0&&StoreScore >=GetLowerStock())
    {
        StoreIndex = 3;
    }
    return m_AllConfig.vProbability[StoreIndex];
}

//填充拉霸表格
bool CTableFrameSink::FillTable(CMD_S_RoundResult_Tag &SceneOneEnd,int32_t wChairID)
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
                     PersonConfig.nFruitPro[IMAGE_CaiShenDao]=PersonConfig.nFruitPro[IMAGE_CaiShenDao]*(1-gailv);
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
		//初始化
        m_FruitWeight.init(PersonConfig.nFruitPro, 11);
		//权值随机重排，可以不调用
		m_FruitWeight.shuffle();
		//返回随机索引，水果下标从1开始
        ret = m_FruitWeight.getResult();
		//LOG(DEBUG) << "************************ RandomFruitArea = " << ret;
        if (m_AllConfig.m_testLine == 1&&m_nFreeRoundLeft[wChairID] == 0)
        {
            ret = m_AllConfig.m_line[Index/5][Index%5];
            if (ret == 0)  ret = m_random.betweenInt(IMAGE_BingTang, IMAGE_CaiShenDao).randInt_mt(true);
        }
        else if (m_AllConfig.m_testLine == 2)
        {
            ret = m_random.betweenInt(IMAGE_BingTang,IMAGE_CaiShenDao).randInt_mt(true);
        }
         else if (m_AllConfig.m_testLine == 3)
        {
             ret = m_random.betweenInt(IMAGE_BingTang,IMAGE_CaiShenDao).randInt_mt(true);

        }  
        //范围断言
        assert(ret >=0 && ret < FRUIT_MAX);
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
        assert(SceneOneEnd.m_Table[Index] >= IMAGE_BingTang);
        assert(SceneOneEnd.m_Table[Index] <= IMAGE_CaiShenDao);
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


    // LOG(WARNING) <<"tmpWinLinesInfo:"<< tmpWinLinesInfo;


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
