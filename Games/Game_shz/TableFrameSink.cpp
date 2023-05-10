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
#include "proto/shz.Message.pb.h"

#include "json/json.h"
#include "ConfigManager.h"
#include "CMD_Game.h"
#include "GameLogic.h"
#include "ConfigManager.h"

using namespace shz;
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
        string dir = "./log/shz";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if (!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("shz");
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
    memset(m_lUserBiBeiBet, 0, sizeof(m_lUserBiBeiBet));
    memset(m_lFreeRoundWinScore, 0, sizeof(m_lFreeRoundWinScore));
    memset(m_iUserBeControlAgent, 0, sizeof(m_iUserBeControlAgent));

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
    StatusFree.set_headid(m_pITableFrame->GetTableUserItem(chairId)->GetHeaderId());
    //填充RewardList数据
    // vector<int> vRewardList = ConfigManager::GetInstance()->GetRewardList();
    // for(int rewardItem :vRewardList){
    //     LOG(WARNING) << "vRewardList:" << rewardItem;
    //     StatusFree.add_rewardlists(rewardItem);
    // }

    //下注每根线下注分数设置
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



    //用户进入发送场景消息
    OnEventGameScene(dwChairID,islookon);
    LOG(ERROR) << "TotalStock:" << GetTotalStock() << ",LowerStock:" << GetLowerStock()<<",HighStock:"<<GetHighStock();
    if(m_AllConfig.m_runAlg == 1){
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);
        m_lUserBetScore[dwChairID]=90000;
        m_TimerIdTest=m_TimerLoopThread->getLoop()->runEvery(0.02, boost::bind(&CTableFrameSink::LeftReward, this,userId,dwChairID));
    }
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
//    if(m_isAndroid){
//       if(m_nAndroidCount > 0) m_nAndroidCount--;
//    }

    LOG(WARNING) << "剩小玛丽次数 1: " << m_nMarrayNumLeft[dwChairID]<< " " << m_nFreeRoundLeft[dwChairID];

    //假如没有搞完，就一次性发送很多次玛利给前端
    if (m_nMarrayNumLeft[dwChairID] > 0&&vtMarryResult.size()>0)
    {
        int nMarryCnt =(int) vtMarryResult.size();
        for (size_t i = 0; i < nMarryCnt; i++)
        {
            LeftReward(userId,dwChairID);
        }

        m_nMarrayNumLeft[dwChairID] = 0;
        vtMarryResult.clear();
    }
  
    LOG(WARNING) << "剩小玛丽次数 2: " << m_nMarrayNumLeft[dwChairID]<< " " << m_nFreeRoundLeft[dwChairID];

    //保存数据
    //WriteFreeGameRecord(dwChairID,m_lUserBetScore[dwChairID],m_nFreeRoundLeft[dwChairID],m_nMarrayNumLeft[dwChairID],m_nMarryNum[dwChairID]);
    
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
//    m_kickTime++;
//    if ( m_kickTime >= m_kickOutTime )
//    {
//        uint32_t wChairID = m_ChairId;
//        //发送超时提示
//        //
//        shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(wChairID);
//        if(!user)
//        {
//	        LOG(ERROR) << "用户不存在:" << wChairID<<" "<< m_userId;
//            return;
//        }

//        if(!user->IsAndroidUser()){
//            //
//            user->SetUserStatus(sGetout);
//            // 发送踢出通知
//            SendErrorCode(0, ERROR_OPERATE_TIMEOUT);

//            //修改桌子状态
//            m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
//            //踢玩家
//            OnUserLeft(m_userId, false);
//            //清理桌子
//            m_pITableFrame->ClearTableUser(wChairID, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
//	        LOG(WARNING) << "超时被踢出 "<< m_kickTime << " " << m_kickOutTime;
//        }

//        m_kickTime = 0;
//    }

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
        GameStart.set_llbetscore(m_lUserBetScore[wChairID]);

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
    //记录原始积分,有信息来就记录一次玩家积分吗????????????
    m_llUserSourceScore[wChairID] = pUserItem->GetUserScore();

    switch (subid)
    {
        case SUB_C_START://水浒传没有免费玩法
        {

            //拉动频率限制
            if ((time(NULL) - m_tLastOperate[wChairID]) < 1)
            {
                LOG(WARNING) << "拉动太频繁了 " << m_nPullInterval << " "<<wChairID <<" UserId:"<<pUserItem->GetUserId();
                return false;
            }

            //玛丽还没结束前不能操作
            if (m_nMarrayNumLeft[wChairID] > 0){
                LOG(WARNING) << "玛丽还没结束前不能操作 ";
                return false;
            }
            //频繁拉取控制变量
            if(m_nPullInterval == 0){ 
                m_nPullInterval = m_AllConfig.nPullInterval;
            }
            CMD_C_GameStart pGamePulling;
            pGamePulling.ParseFromArray(data, dwDataSize);
            
            //玩家下注金额
            m_lUserBetScore[wChairID] =  pGamePulling.llbetscore();


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

            openSLog("本局玩家： %d          押分分数是：%d ",pUserItem->GetUserId(),m_lUserBiBeiBet[wChairID]);
            //押分分数太大，超过玩家分
            if ( m_lUserBetScore[wChairID] > pUserItem->GetUserScore())
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

    
            //修改用户状态
            m_pITableFrame->SetUserReady(wChairID);
            //清除数据
            m_nMarrayNumLeft[wChairID] = 0;
            m_nMarryNum[wChairID] = 0;
            m_lBibeiAllWin[wChairID] = 0;

            //发送开奖结果
            int bRet = BuildRoundScene(wChairID, m_lUserBetScore[wChairID]);

            if(bRet > 0)
            {
                LOG(WARNING) << "创建开奖结果失败!BuildRoundScene:" << bRet;
                SendErrorCode(wChairID,(enErrorCode)bRet); 
            }  

            //修改桌子状态,发完结果进入空闲状态
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
                openSLog("本局玩家：%d 拉取超级玛利失败，因为玛利次数为零",pUserItem->GetUserId());
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
        case SUB_C_BIBEI_BET:
        {
            if(m_nMarrayNumLeft[wChairID]>0)
            {
                LOG(WARNING) << "状态不对玛利机!";
                //玛利机状态下不能操作比倍
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }
            if ((time(NULL) - m_tLastOperate[wChairID]) < 1)
            {
                LOG(WARNING) << "时间太短!";
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }

            m_tLastOperate[wChairID]= time(NULL);
            if(m_lUserBiBeiBet[wChairID]>pUserItem->GetUserScore())
            {
                LOG(WARNING) << "分数不够!";
                SendErrorCode(wChairID,(enErrorCode)1);
                return false;
            }

             if(m_lUserBiBeiBet[wChairID]<=0)
             {
                LOG(WARNING) << "比倍下注分数没有了!";
                openSLog("没有可以比倍的分数 m_lUserBiBeiBet[wChairID]=%d",m_lUserBiBeiBet[wChairID]);
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
             }

             CMD_C_BIBEI_BET pGameBetting;
             pGameBetting.ParseFromArray(data, dwDataSize);
             int32_t bettingPlace = pGameBetting.cbbetplace();
             if(bettingPlace<0||bettingPlace>2)
             {
                  LOG(WARNING) << "比倍失败下注失败!";
                  openSLog("比倍失败下注失败 bettingPlace=%d",bettingPlace);
                  return false;
             }
             openSLog("比倍下注门 bettingPlace=%d",bettingPlace);
             int bRet = BuildBibeiScene(wChairID,bettingPlace);
             if(bRet > 0){
                 LOG(WARNING) << "比倍失败开奖结果失败!";
                 SendErrorCode(wChairID,(enErrorCode)bRet);
             }
             return true;
        }
        case SUB_C_JACKPOT_REC:
        { 
            return true;
        } 
    }
    return true;
}
// 发送彩金数据
void CTableFrameSink::SendJackPot()
{

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
    ConfigManager * cfgManager = ConfigManager::GetInstance();
	if (cfgManager == NULL){
		LOG(WARNING) << "ConfigManager失败 ";
		return false;
    } 

    int ret = ConfigManager::GetInstance()->CheckReadConfig();
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
int CTableFrameSink::BuildRoundScene(const int32_t wChairID, int64_t lbetScore)
{
    if (m_llUserSourceScore[wChairID] < lbetScore)// 分数太少
        return ERROR_SCORE_NOT_ENOUGH;

    int ret = ConfigManager::GetInstance()->CheckReadConfig();
    if(ret != 0){
        LOG(WARNING) << "配置文件读取失败 " << ret;
        return false;
    }

    //获取配置文件
    memset(&m_AllConfig, 0, sizeof(m_AllConfig));
    ConfigManager::GetInstance()->GetConfig(m_AllConfig);
    //拉霸开奖结果命令 
    CMD_S_RoundResult_Tag CMDGameOneEnd; 

    int Count = 0;
    do
    {
        //随机一个没有奖的结果
        memset(&CMDGameOneEnd, 0, sizeof(CMDGameOneEnd));      
        //超过五百次就出一个没有奖项的结果
        if ((++Count) > 500)
        {
            LOG(WARNING) << "Count > 500";
            //没有奖项的结果
            m_GameLogic.RandNoneArray(CMDGameOneEnd.m_Table, ICON_MAX);
            openSLog("开奖循环超过了500次，所以出一个没奖的结果 ");
            string Kaijiang="";
            for(int i=0;i<ICON_MAX;i++)
            {
                Kaijiang+="    "+to_string(CMDGameOneEnd.m_Table[i]);
            }
            openSLog(Kaijiang.c_str());
            break;
        }
 
        //填充命令参数,并且按照水浒图标的权重随机出结果
        FillTable(wChairID,CMDGameOneEnd);
 
        //计算分数
        if (CalculateScore(CMDGameOneEnd, lbetScore / LINE_MAX) == false) { LOG(WARNING) << "Scatter和Bouns只能出一个"; continue; }

        //检查 Scatter 和 Bouns只能出一个
        //if (CMDGameOneEnd.nFreeRoundLeft > 0 && CMDGameOneEnd.nMarryNum > 0) { LOG(WARNING) << "Scatter和Bouns只能出一个"; continue; }

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
        //string tmp97 = "";
        //记录
        int FreeCount = 0;
        string tmpRewardInfo = str(boost::format("%d")  % CMDGameOneEnd.nMarryNum);
        //string tmpScatterWin = str(boost::format("%d-%d") % 0 % 0);
        if(CMDGameOneEnd.isAll)
         {
             tmpWinLinesInfo = str(boost::format("99-%d") % CMDGameOneEnd.Score);
         }
        cardValueStr =str(boost::format("%d|%s|%s|%s") % lineCount % tmpWinLinesInfo % tmpRewardInfo % iconResStr );

        int64_t AllMarryWinScore=0;
        //填充超级玛利的结果列表
        GetAllResult(wChairID ,CMDGameOneEnd.nMarryNum,AllMarryWinScore);
        //押注库存验证,然后重新出结果
        //超过二十万的结果暂时不开放，以后人数多了在考虑
        if (CMDGameOneEnd.Score > 0L||AllMarryWinScore>0) {
           if(!CheckStorageVal(CMDGameOneEnd.Score+AllMarryWinScore)||AllMarryWinScore>m_AllConfig.m_BestWinNum)
           {
                LOG(ERROR)<<" 出现大奖重新出结果 倍数是:"<<CMDGameOneEnd.Score+AllMarryWinScore<<" 玛丽倍数是："<<AllMarryWinScore;
                continue;
           }
        } 
        LOG(ERROR)<<" 最大可以赢取的分数是: "<<m_AllConfig.m_BestWinNum;
        //难度干涉值
        //m_difficulty
        m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        //难度值命中概率,全局难度,命中概率系统必须赚钱
        if(randomRet<=m_difficulty)
        {
            if(CMDGameOneEnd.Score > 0L||AllMarryWinScore>0)
            {
               continue;
            }
        }

        break;

    } while (true);


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

    if(m_AllConfig.m_runAlg==1){
        string tempStr = str(boost::format("%5d|%s") % m_Round % cardValueStr);

    }
    //写分
    int64_t llGameTax = 0;
    if (RoundSceneWriteScore(wChairID, lbetScore, CMDGameOneEnd.Score, llGameTax,cardValueStr) == false)
    {
        return ERROR_SIT_CHAIR_ID;
    }

    //保存数据
    m_lUserBiBeiBet[wChairID] = CMDGameOneEnd.Score;
	m_lUserBetScore[wChairID] = lbetScore;
	m_lUserWinScore[wChairID] = CMDGameOneEnd.Score;
    m_nMarryNum[wChairID] = CMDGameOneEnd.nMarryNum;
	m_nMarrayNumLeft[wChairID] = CMDGameOneEnd.nMarryNum;


	CMDGameOneEnd.Score -= llGameTax;
	CMDGameOneEnd.lCurrentScore = m_llUserSourceScore[wChairID];


#if DEBUG_INFO
    LOG(WARNING) << "lCurrentScore: " << CMDGameOneEnd.lCurrentScore;
    LOG(WARNING) << "Score: " << CMDGameOneEnd.Score;
    LOG(WARNING) << "nFreeRoundLeft: " << CMDGameOneEnd.nFreeRoundLeft;
    LOG(WARNING) << "lFreeRoundWinScore: " << CMDGameOneEnd.lFreeRoundWinScore;
    LOG(WARNING) << "nMarryNum: " << CMDGameOneEnd.nMarryNum;
#endif
    //跑马灯信息
    if((m_lUserBetScore[wChairID]>0 && CMDGameOneEnd.Score/(m_lUserBetScore[wChairID]/9) >= m_AllConfig.m_MinBoardCastOdds)
    ||CMDGameOneEnd.Score > m_pGameRoomInfo->broadcastScore)
    {
        string muticle="";
        muticle = to_string(CMDGameOneEnd.Score/(m_lUserBetScore[wChairID]/9));
        m_pITableFrame->SendGameMessage(wChairID, muticle,SMT_GLOBAL|SMT_SCROLL, CMDGameOneEnd.Score);
    }

    //复制数据
    CMD_S_RoundResult cmdData;

    for(int i=0;i<(int)CMDGameOneEnd.vtResultList.size();i++)
    {
        CMD_S_ResultList * resList =  cmdData.add_resultlist();
        resList->set_fruitindex(CMDGameOneEnd.vtResultList[i].fruitIndex);
        resList->set_fruitnum(CMDGameOneEnd.vtResultList[i].fruitNum);
        resList->set_multic(CMDGameOneEnd.vtResultList[i].mutical);
    }
    cmdData.set_score(CMDGameOneEnd.Score);
    cmdData.set_lcurrentscore(CMDGameOneEnd.lCurrentScore);
    cmdData.set_nmarrynum(CMDGameOneEnd.nMarryNum);//玛丽次数
    cmdData.set_isall(CMDGameOneEnd.isAll);
    for (int i = 0; i < ICON_MAX; i++)
    {
        cmdData.add_m_table(CMDGameOneEnd.m_Table[i]); //所有水果图标
        cmdData.add_tablelight(CMDGameOneEnd.TableLight[i]);//中奖闪烁的图标

        //openSLog("屏幕所有图标 = %d   ",CMDGameOneEnd.m_Table[i]);

        if(i < LINE_MAX && CMDGameOneEnd.Line[i] > 0) {
            cmdData.add_line(i+1); //中奖的线
        }
    } 

    cmdData.set_roundid(mGameIds);//牌局id
    openSLog("玩家赢分 = %d   ",CMDGameOneEnd.Score);
    openSLog("玛利容器数量 = %d  剩余玛利次数  = %d ",(int)vtMarryResult.size(),CMDGameOneEnd.nMarryNum);
    std::string data = cmdData.SerializeAsString(); 
    if (!m_bUserIsLeft)
        m_pITableFrame->SendTableData(wChairID, SUB_S_NORMAL, (uint8_t *)data.c_str(), data.size());
     
    return 0;
}
//bibei write score
bool CTableFrameSink::WriteBiBeiGameRecord(const int32_t wChairID, const int64_t lWinScore, const int64_t lBetScore ,int64_t &lGameTax,string cardValueStr)
{

    AssertReturn(wChairID < GAME_PLAYER, return false);

    int64_t lplayerWinScore = lWinScore - lBetScore;

    m_pGameRoomInfo->totalStock -= lplayerWinScore;
    m_llTodayStorage -= lplayerWinScore;

    //计算税收
    lGameTax = 0;// m_pITableFrame->CalculateRevenue(lplayerWinScore);

    //写积分
    tagScoreInfo ScoreData;
    ScoreData.chairId = wChairID;
    ScoreData.addScore = lplayerWinScore - lGameTax;         // 当局输赢分数
    ScoreData.betScore = lBetScore;//m_lUserBetScore[wChairID]; // 总压注
    ScoreData.revenue = lGameTax;                   // 当局税收
    ScoreData.rWinScore = lBetScore;//lWinScore;                //有效投注额：税前输赢
    //牌型
    ScoreData.cardValue = cardValueStr;
    ScoreData.startTime = m_startTime;

    //统计用
    //m_lMarryWinScore += lplayerWinScore;

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
//bibei scene build
int CTableFrameSink::BuildBibeiScene(const int32_t wChairID,const int32_t bettingPlace)
{
    //读配置
    UserProbabilityItem Item;
    memset(&Item, 0, sizeof(Item));
    Item = ScoreGetProbability(wChairID,GetTotalStock());
    //////////////////////////////////
    int count = 0;
    int64_t winScore = 0;
    int64_t betScore = m_lUserBiBeiBet[wChairID];
    int FruitIndex[2] = {0};
    int32_t FruitOdd = 0L;
    int PlayerbetPlace = bettingPlace;//玩家下注位置
    int openIndex=0;
    int winOrLose  = 0;
    do
    {
        // chongxin  tongji
        winScore = 0;
        FruitIndex[0] = m_random.betweenInt(1,6).randInt_mt(true);
        FruitIndex[1] = m_random.betweenInt(1,6).randInt_mt(true);

        if(m_AllConfig.m_testBibei)
        {
            FruitIndex[0] =  m_AllConfig.m_bibiList[0];
            FruitIndex[1] =  m_AllConfig.m_bibiList[1];
        }
        assert( FruitIndex[0] >= 1);
        assert( FruitIndex[0] <= 6);
        assert( FruitIndex[1] >= 1);
        assert( FruitIndex[1] <= 6);
        //0 = da 1 = xiao 2 = he

        if(FruitIndex[0]+FruitIndex[1]>7)
        {
           openIndex=0;
        }
        else if(FruitIndex[0]+FruitIndex[1]<7)
        {
            openIndex=1;
        }
        else
        {
            openIndex=2;
        }
        if(PlayerbetPlace==0)//大
        {
           if(FruitIndex[0]+FruitIndex[1]>7)
           {
               if(FruitIndex[0]==FruitIndex[1])
               {
                    winScore=betScore*4;// 多出3倍
               }
               else
               {
                    winScore=betScore*2;// 多出1倍
               }
           }
           else
           {
                    winScore=0; //从写分的角度那这次 他是输的
           }
        }
        else if(PlayerbetPlace==1)// 小
        {
            if(FruitIndex[0]+FruitIndex[1]<7)
            {
                if(FruitIndex[0]==FruitIndex[1])
                {
                     winScore=betScore*4;// 多出3倍
                }
                else
                {
                     winScore=betScore*2;// 多出1倍
                }
            }
            else
            {
                winScore=0; //从写分的角度那这次 他是输的
            }
        }
        else //等于7 和
        {
            if(FruitIndex[0]+FruitIndex[1]==7)
            {
                 winScore=betScore*6; //从写分的角度那这次 他是输的
            }
            else
            {
                winScore=0; //从写分的角度那这次 他是输的
            }
        }
        //超分检查
        if(CheckStorageVal(winScore)) {
           break;
        }
        else
        {
           continue;
        }


    } while (true);

    //记录
    //string tmpRewardInfo = str(boost::format("%d,%d") % m_nFreeRoundLeft[wChairID] % m_nMarrayNumLeft[wChairID]);
    //cardValueStr = str(boost::format("0|%s|%s|%s") % "" % tmpRewardInfo % str(boost::format("%d,%d") % FruitIndex % FruitOdd));

    cardValueStr = str(boost::format("%d|%d|%d|%d") % FruitIndex[0] % FruitIndex[1] % openIndex%PlayerbetPlace);
    LOG(WARNING) << cardValueStr;

    //玛丽游戏写分
    int64_t llGameTax = 0;
    if (WriteBiBeiGameRecord(wChairID, winScore, betScore,llGameTax,cardValueStr) == false) {
        return ERROR_OPERATE;
    }

    if(winScore>0)
    {
        m_lBibeiAllWin[wChairID] += winScore;
        m_lUserBiBeiBet[wChairID] = winScore;
    }
    else
    {
        m_lBibeiAllWin[wChairID] = 0;//全部清零
        m_lUserBiBeiBet[wChairID] = 0;//
    }
    if(winScore>0)
        winOrLose = 1;
    else
        winOrLose = 0;
    //命令组装
    CMD_S_BiBeiResult BibeiEnd;
    BibeiEnd.set_currentwin(winScore);
    BibeiEnd.set_totalwin(m_lBibeiAllWin[wChairID]);
    BibeiEnd.set_userscore(m_llUserSourceScore[wChairID]);
    BibeiEnd.set_betmoney(winScore);
    BibeiEnd.set_isover(winOrLose);
    BibeiEnd.add_saizi(FruitIndex[0]);
    BibeiEnd.add_saizi(FruitIndex[1]);
    BibeiEnd.set_openindex(openIndex);
    openSLog("本次比倍赢分 winScore=%d   m_lBibeiAllWin[wChairID]=%d",winScore,m_lBibeiAllWin[wChairID]);
    openSLog("结算后玩家分数 m_llUserSourceScore[wChairID]=%d  ",m_llUserSourceScore[wChairID]);
    openSLog("玩家下注分数 betscore=%d  ",betScore);
    openSLog("是否结束 isover=%d  ",winOrLose);
    openSLog("骰子一 =%d  骰子二  =%d",FruitIndex[0],FruitIndex[1]);

    openSLog("----------------------------------------------");
    string data = BibeiEnd.SerializeAsString();
    if (!m_bUserIsLeft) {
        m_pITableFrame->SendTableData(wChairID, SUB_S_Bibei_Rec, (uint8_t *)data.c_str(), data.size());
    }


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


void CTableFrameSink::GetAllResult(int chairid ,int  marrytimes,int64_t &allMarryWin)
{
    //int count = 0;
    if(marrytimes<=0) return;
    int FruitIndex = 0;
    int64_t WinScore = 0;
    int32_t FruitOdd = 0L;
    int fruitList[4] = {0};
    int AllmarryTimes = marrytimes;
    vtMarryResult.clear();
    allMarryWin = 0;
    int testTimes=0;
    do
    {
        WinScore=0;
        FruitOdd=0;
        MarryInfo marInfo;

        //八个图标和一个exit的九种格子的权重
        //exit—斧头—红缨枪—砍刀—鲁智深—林冲—宋江—替天行道—忠义堂
        int marryWeight[9]={4,4,3,3,3,2,2,2,1};

        int mutilcIcon[9]={0,2,5,10,20,50,70,100,200};

        int allWeight=0;
        for(int i=0;i<9;i++)
            allWeight+=marryWeight[i];

        int ran=m_random.betweenInt(0,allWeight).randInt_mt(true);

        int res=0;
        for(int i=0;i<9;i++)
        {
            res+=marryWeight[i];
            if(res>=ran)
            {
                FruitIndex = i;
                marInfo.waiIndex = FruitIndex;
                break;
            }
        }

        //LOG(DEBUG) << "************************ RandomFruitArea = " << FruitIndex;
        {   //当外圈是某个水果的时候，内圈产生某些水果的概率出牌
            switch(FruitIndex)
            {
                case IMAGE_NULL:
                {
                    //外面是退出图标的时候，里面随便随机
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }

                    break;
                }
                case IMAGE_AX:
                {
                    //外面是退出图标的时候，里面随便随机
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_TASSEL:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_DAGGER:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_ZHISHENLU:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_CHONGLIN:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_JIANGSONG:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_KILLFORGOD:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_LOYALTYHAUSE:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
                case IMAGE_LEGEND:
                {
                    for(int i=0;i<4;i++)
                    {
                       fruitList[i] = m_random.betweenInt(1,8).randInt_mt(true);
                       marInfo.neiIconList[i]=fruitList[i] ;
                    }
                    break;
                }
            }
        }

        if(m_AllConfig.m_testMarry)
        {
            if(testTimes==0)
            {
                FruitIndex=m_AllConfig.m_marryIndex;
                for(int i=0;i<4;i++)
                {
                    fruitList[i]=m_AllConfig.m_marryList[i];
                    marInfo.neiIconList[i]=fruitList[i] ;
                }
                marInfo.waiIndex=FruitIndex;
                testTimes++;
            }
            else
            {
                FruitIndex=0;
                for(int i=0;i<4;i++)
                {
                    fruitList[i]=m_AllConfig.m_marryList[i];
                    marInfo.neiIconList[i]=fruitList[i] ;
                }
                marInfo.waiIndex=FruitIndex;
            }
        }

        assert(FruitIndex >= IMAGE_NULL);
        assert(FruitIndex <= IMAGE_LOYALTYHAUSE);
        if((FruitIndex==fruitList[0]&&fruitList[0]==fruitList[1]&&fruitList[1]==fruitList[2]&&fruitList[2]!=fruitList[3])||
                (FruitIndex==fruitList[3]&&fruitList[3]==fruitList[2]&&fruitList[2]==fruitList[1]&&fruitList[1]!=fruitList[0]))
        {
            //中三个20倍
            switch (FruitIndex)
            {
                case IMAGE_NULL:{FruitOdd = mutilcIcon[FruitIndex];break;}
                case IMAGE_AX:{     FruitOdd = mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_TASSEL:{ FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_DAGGER:{     FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_ZHISHENLU:{      FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_CHONGLIN:{      FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_JIANGSONG:{ FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_KILLFORGOD: {      FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                case IMAGE_LOYALTYHAUSE: {      FruitOdd =  mutilcIcon[FruitIndex]*3+20;break;}
                default:
                        break;
            }
        }
        else if((FruitIndex==fruitList[0]&&fruitList[0]==fruitList[1]&&fruitList[1]==fruitList[2]&&fruitList[2]==fruitList[3]))
        {
            //中四个500倍
            switch (FruitIndex)
            {
                case IMAGE_NULL:{FruitOdd = mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_AX:{     FruitOdd = mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_TASSEL:{ FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_DAGGER:{     FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_ZHISHENLU:{      FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_CHONGLIN:{      FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_JIANGSONG:{ FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_KILLFORGOD: {      FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                case IMAGE_LOYALTYHAUSE: {      FruitOdd =  mutilcIcon[FruitIndex]*4+500;break;}
                default:
                     break;
            }
        }
        else
        {
            int sameNum=0;
            //无连线的时候，统计个数
            for(int i=0;i<4;i++)
            {
                if(FruitIndex==fruitList[i])  sameNum++;
            }
            switch (FruitIndex)
            {
                case IMAGE_NULL:{FruitOdd = mutilcIcon[FruitIndex];break;}
                case IMAGE_AX:{     FruitOdd = mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_TASSEL:{ FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_DAGGER:{     FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_ZHISHENLU:{      FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_CHONGLIN:{      FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_JIANGSONG:{ FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_KILLFORGOD: {      FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                case IMAGE_LOYALTYHAUSE: {      FruitOdd =  mutilcIcon[FruitIndex]*sameNum;break;}
                default:
                 break;
            }
        }
        WinScore = (m_lUserBetScore[chairid] * FruitOdd );

        //填充结果容器
        marInfo.currentWin = WinScore;
        vtMarryResult.push_back(marInfo);
        allMarryWin+=WinScore;
        //每次都是EXIT的时候算是一次
        if(FruitIndex==IMAGE_NULL)
        {
             AllmarryTimes--;
        }
    } while (AllmarryTimes>0);
}
//构造玛丽开奖结果
int CTableFrameSink::BuildMarryScene(const int32_t wChairID)
{


    if(vtMarryResult.size()<=0)
    {
        openSLog("玛利容器为0");
        return 1;
    }    //////////////////////////////////
    /// \brief FruitIndex
    ///
    ///
    //玛丽减次数
    int FruitIndex = 0;
    int64_t WinScore = 0;
    int fruitList[4] = {0};


    //先减后写


    FruitIndex=vtMarryResult[0].waiIndex;
    for(int i=0;i<4;i++) fruitList[i] = vtMarryResult[0].neiIconList[i];
    WinScore = vtMarryResult[0].currentWin;
    vtMarryResult.erase(vtMarryResult.begin());
    openSLog("剩余玛利次数  = %d ",(int)vtMarryResult.size());
    //记录
    int iswin=((WinScore==0)?0:1);
    if(FruitIndex==IMAGE_NULL) m_nMarrayNumLeft[wChairID]--;
    string tmpRewardInfo = str(boost::format("%d")  % m_nMarrayNumLeft[wChairID]);
    cardValueStr = str(boost::format("%d|%s|%s")%iswin % tmpRewardInfo % str(boost::format("%d,%d") % FruitIndex%WinScore));
    LOG(WARNING) << cardValueStr;

    //玛丽游戏写分
    int64_t llGameTax = 0;

    if (MarrySceneWriteScore(wChairID, WinScore, llGameTax,cardValueStr) == false) {
        return ERROR_OPERATE;
    }


    //叠加玩家小玛利得分
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
    for(int i=0;i<4;i++) MarryEnd.add_listfruit(fruitList[i]);
    MarryEnd.set_betscore(m_lUserBetScore[wChairID]);
    openSLog("发送玛利数据 currentwin = %d    totalwin=%d   FruitIndex=%d   marryleft=%d   marrynum=%d",WinScore - llGameTax, m_lMarryTotalWin[wChairID] , FruitIndex,m_nMarrayNumLeft[wChairID],m_nMarryNum[wChairID]);
    //序列化数据发送
    string data = MarryEnd.SerializeAsString();
    if (!m_bUserIsLeft) {
        m_pITableFrame->SendTableData(wChairID, SUB_S_MARRY, (uint8_t *)data.c_str(), data.size());
    }

    //玛丽结束判断
    if (m_nMarrayNumLeft[wChairID] <= 0)  {
        m_nMarryNum[wChairID] = 0;
        m_lMarryTotalWin[wChairID] = 0L; 
        vtMarryResult.clear();
    } 

    //实时保存数据
    ///WriteFreeGameRecord(wChairID,m_lUserBetScore[wChairID],m_nFreeRoundLeft[wChairID],m_nMarrayNumLeft[wChairID],m_nMarryNum[wChairID]);
    return 0;
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
UserProbabilityItem CTableFrameSink::ScoreGetProbability(const int32_t wChairID,int64_t StoreScore)//, UserProbabilityItem &UserPro)
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
    memset(m_iUserBeControlAgent, 0, sizeof(m_iUserBeControlAgent));
    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||user->IsAndroidUser())
        {
            continue;
        }
        for(int j=0;j<m_AllConfig.m_vControlAgentid.size();j++)
        {
            if(user->GetUserBaseInfo().agentId==m_AllConfig.m_vControlAgentid[j])
            {
                m_iUserBeControlAgent[j]=true;

                break;
            }

        }

    }
    //假如玩家是需要控制的代理线的玩家，则选择代理控制概率
    if(m_iUserBeControlAgent[wChairID])
    {
        StoreIndex = 3;

        LOG(WARNING) << "这个玩家是需要控制的代理线的玩家";
        LOG(WARNING) << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[0]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[1]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[2]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[3]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[4]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[5]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[6]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[7]
                     << "    "<<m_AllConfig.vProbability[StoreIndex].nFruitPro[8];
        return m_AllConfig.vProbability[StoreIndex];

    }
    return m_AllConfig.vProbability[StoreIndex];
}

//填充拉霸表格
bool CTableFrameSink::FillTable(const int32_t wChairID,CMD_S_RoundResult_Tag &SceneOneEnd)
{ 
    ///////////////////////////////
    ///
    ///



    UserProbabilityItem PersonConfig;
    memset(&PersonConfig, 0, sizeof(PersonConfig));
    PersonConfig =  ScoreGetProbability(wChairID,GetTotalStock());
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
                     PersonConfig.nFruitPro[IMAGE_LEGEND-1]=PersonConfig.nFruitPro[IMAGE_LEGEND-1]*(1-gailv);
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
        int ProbabilityIndex = IMAGE_LEGEND;
        //ret = m_GameLogic.RandomFruitArea(ProbabilityIndex, PersonConfig.nFruitPro);
		//初始化
		m_FruitWeight.init(PersonConfig.nFruitPro, ProbabilityIndex);
		//权值随机重排，可以不调用
		m_FruitWeight.shuffle();
		//返回随机索引，水果下标从1开始
		ret = m_FruitWeight.getResult() + 1;
		//LOG(DEBUG) << "************************ RandomFruitArea = " << ret;
        ///测试用例
        if (m_AllConfig.m_testLine == 1) {
            ret = m_AllConfig.m_line[Index/5][Index%5];
            if (ret == 0)  ret = m_random.betweenInt(IMAGE_AX, IMAGE_LOYALTYHAUSE).randInt_mt(true);
        }
        else if (m_AllConfig.m_testLine == 2){ 
            if( IsHaveSpecil[Index % 5] == 0){
                if(m_random.betweenInt(0,3).randInt_mt(true) ==0)
                   ret=IMAGE_LEGEND;
                else
                   ret = m_random.betweenInt(IMAGE_AX,IMAGE_LOYALTYHAUSE).randInt_mt(true);
            } 
        }
         else if (m_AllConfig.m_testLine == 3){  
             if( IsHaveSpecil[Index % 5] == 0){
                if(m_random.betweenInt(0,3).randInt_mt(true) ==0)
                   ret=IMAGE_LEGEND;
                else
                   ret = m_random.betweenInt(IMAGE_AX,IMAGE_LOYALTYHAUSE).randInt_mt(true);
            } 
        }         
        //范围断言
        assert(ret > 0 && ret < FRUIT_MAX);

//        if (ret == IMAGE_SCATTER || ret == IMAGE_BONUS){
//            IsHaveSpecil[Index % 5] = 1;
//        }

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
//        if (SceneOneEnd.m_Table[Index] == IMAGE_SCATTER || SceneOneEnd.m_Table[Index] == IMAGE_BONUS)
//        {
//            Specil[Index % 5]++;
//        }

        assert(SceneOneEnd.m_Table[Index] >= IMAGE_AX);
        assert(SceneOneEnd.m_Table[Index] <= IMAGE_LEGEND);
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
        int FruitIndexRight = 0;
        int FruitNumRight = 0;
        int MultiRight = 0;
        int FruitIndexLeft = 0;
        int FruitNumLeft = 0;
        int MultiLeft = 0;
        int marryNum=0;
        //计算每根线是否中将,实现左右搜索,每根线左右两边中奖的水果，中奖的个数，中奖的倍数,都要统计出来，方便前端显示结果
        m_GameLogic.Multi_Line(FruitIndexRight, FruitNumRight,MultiRight, FruitIndexLeft,FruitNumLeft,MultiLeft,marryNum,SceneOneEnd.m_Table, SceneOneEnd.TableLight, ICON_MAX, LineIndex);

        Multi=MultiRight+MultiLeft;
        SceneOneEnd.nMarryNum+=marryNum;

        if(MultiRight>0)
        {
            CMD_S_Result res;
            res.fruitIndex = FruitIndexRight;
            res.fruitNum = FruitNumRight+1;
            res.mutical = MultiRight;
            SceneOneEnd.vtResultList.push_back(res);
        }
        if(MultiLeft>0)
        {
            CMD_S_Result res;
            res.fruitIndex = FruitIndexLeft;
            res.fruitNum = FruitNumLeft+1;
            res.mutical = MultiLeft;
            SceneOneEnd.vtResultList.push_back(res);
        }
        SceneOneEnd.isAll=0;
        //累加中奖分数
        SceneOneEnd.Score += (MultiRight+MultiLeft) * SingleLine;
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

    }
    int xcount=0;
    for(int i=0;i<14;i++)
    {
        if(SceneOneEnd.m_Table[i]!=SceneOneEnd.m_Table[i+1])
        {
            break;
        }
        xcount++;
    }
    //全屏奖特别处理一下
    if(xcount==14)
    {
        SceneOneEnd.vtResultList.clear();
        SceneOneEnd.isAll=1;
        switch (SceneOneEnd.m_Table[0])
        {
            case IMAGE_AX:
            {

                SceneOneEnd.Score=50 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_AX;
                res.fruitNum = 15;
                res.mutical = 50;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_TASSEL:
            {
                SceneOneEnd.Score=100 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_TASSEL;
                res.fruitNum = 15;
                res.mutical = 100;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_DAGGER:
            {
                SceneOneEnd.Score=150 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_DAGGER;
                res.fruitNum = 15;
                res.mutical = 150;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_ZHISHENLU:
            {
                SceneOneEnd.Score=250 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_ZHISHENLU;
                res.fruitNum = 15;
                res.mutical = 250;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_CHONGLIN:
            {
                SceneOneEnd.Score=400 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_CHONGLIN;
                res.fruitNum = 15;
                res.mutical = 400;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_JIANGSONG:
            {
                SceneOneEnd.Score=500 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_JIANGSONG;
                res.fruitNum = 15;
                res.mutical = 500;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_KILLFORGOD:
            {
                SceneOneEnd.Score=1000 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_KILLFORGOD;
                res.fruitNum = 15;
                res.mutical = 1000;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_LOYALTYHAUSE:
            {
                SceneOneEnd.Score=2500 * SingleLine*LINE_MAX;
                CMD_S_Result res;
                res.fruitIndex = IMAGE_LOYALTYHAUSE;
                res.fruitNum = 15;
                res.mutical = 2500;
                SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        case IMAGE_LEGEND:
            {
                SceneOneEnd.Score=5000 * SingleLine*LINE_MAX;
                 SceneOneEnd.nMarryNum=27;
                 CMD_S_Result res;
                 res.fruitIndex = IMAGE_LEGEND;
                 res.fruitNum = 15;
                 res.mutical = 5000;
                 SceneOneEnd.vtResultList.push_back(res);
            }
            break;
        default:
            break;
        }
    }

    xcount=0;
    for(int i=0;i<15;i++)
    {
        if(SceneOneEnd.m_Table[i]!=IMAGE_ZHISHENLU
           &&SceneOneEnd.m_Table[i]!=IMAGE_CHONGLIN
           &&SceneOneEnd.m_Table[i]!=IMAGE_JIANGSONG)
        {
            break;
        }
        xcount++;
    }
    if(xcount==15&&!SceneOneEnd.isAll)
    {
        SceneOneEnd.isAll=2;
        SceneOneEnd.Score=50 * SingleLine*LINE_MAX;
        for(int i=0;i<15;i++)
        {
            SceneOneEnd.TableLight[i]=1;
        }
        CMD_S_Result res;
        res.fruitIndex = IMAGE_AX;
        res.fruitNum = 15;
        res.mutical = 50;
        SceneOneEnd.vtResultList.push_back(res);
    }
     xcount=0;
    for(int i=0;i<15;i++)
    {
        if(SceneOneEnd.m_Table[i]!=IMAGE_AX
           &&SceneOneEnd.m_Table[i]!=IMAGE_TASSEL
           &&SceneOneEnd.m_Table[i]!=IMAGE_DAGGER)
        {
            break;
        }
        xcount++;
    }
    if(xcount==15&&!SceneOneEnd.isAll)
    {
        SceneOneEnd.isAll=3;
        SceneOneEnd.Score=15 * SingleLine*LINE_MAX;
        for(int i=0;i<15;i++)
        {
            SceneOneEnd.TableLight[i]=1;
        }
        CMD_S_Result res;
        res.fruitIndex = IMAGE_AX;
        res.fruitNum = 15;
        res.mutical = 15;
        SceneOneEnd.vtResultList.push_back(res);
    }
    return true;
}
//玛丽游戏写分
bool CTableFrameSink::MarrySceneWriteScore(const int32_t wChairID, const int64_t lWinScore, int64_t &lGameTax,string cardValueStr)
{
    AssertReturn(wChairID < GAME_PLAYER, return false);
    //AssertReturn(lWinScore > 0L, return false);

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
 

    openSLog("@@@@@@@@@@@@@@@@玛利结算@@@@@@@@@@@@@@@@@@");
    openSLog("玛利游戏本次赢分 = %d",lplayerWinScore-lGameTax);
    openSLog("玛利游戏本次玩家分增加以后分数 = %d",m_llUserSourceScore[wChairID]);
    openSLog("玛利游戏本次玩家玛利总赢分 = %d",m_lMarryWinScore);
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
bool CTableFrameSink::RoundSceneWriteScore(const int32_t wChairID,const int64_t lBetScore, const int64_t lWinScore, int64_t &lGameTax,string cardValueStr)
{
    AssertReturn(wChairID < GAME_PLAYER, return false);
 
    int64_t lplayerWinScore = 0;
    lplayerWinScore = lWinScore - lBetScore;//0 - 900

    m_pGameRoomInfo->totalStock -= lplayerWinScore;
    m_llTodayStorage -= lplayerWinScore;

    //算税收
    if (lWinScore > 0)  lGameTax = 0;//m_pITableFrame->CalculateRevenue(lWinScore);
       
    //写积分
    tagScoreInfo ScoreData;
    ScoreData.chairId = wChairID;
    ScoreData.addScore = lplayerWinScore - lGameTax; // 当局输赢分数 
    ScoreData.betScore = lBetScore;         // 总压注
    ScoreData.revenue = lGameTax;           // 当局税收
    ScoreData.rWinScore = ScoreData.betScore;//lBetScore;//lWinScore; //有效投注额：税前输赢
    ScoreData.cardValue = cardValueStr;
    ScoreData.startTime = m_startTime;

    m_llUserSourceScore[wChairID] += ScoreData.addScore;

    openSLog("****************正常结算*****************");
    if(m_pITableFrame->GetTableUserItem(wChairID)) openSLog("玩家[%d]",m_pITableFrame->GetTableUserItem(wChairID)->GetUserId());
    openSLog("lBetScore = %d",lBetScore);
    openSLog("本次赢分 = %d",lplayerWinScore);
    openSLog("当局税收 = %d",lGameTax);
    openSLog("写分后的玩家分数m_llUserSourceScore[wChairID] = %d",m_llUserSourceScore[wChairID]);

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
    if(m_pITableFrame->GetTableUserItem(wChairID)) openSLog("获取数据库的玩家分数 = %d",(m_pITableFrame->GetTableUserItem(wChairID))->GetUserScore());
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
    m_lMarryTotalWin[dwChairID] = 0L;//玛利总赢分
    m_lUserWinScore[dwChairID] = 0L; //玩家赢分
    m_lUserBetScore[dwChairID] = 0L; //玩家总压住
    m_nMarrayNumLeft[dwChairID] = 0; //剩余玛利次数
    m_nFreeRoundLeft[dwChairID] = 0; //剩余免费次数
    m_nMarryNum[dwChairID] = 0;      //小玛利次数
    m_lFreeRoundWinScore[dwChairID] = 0L;//免费旋转赢得分数
    m_bIsSpecilUser[dwChairID] = false;//

    m_tLastOperate[dwChairID] = 0L;    //
}
//复位桌子
void CTableFrameSink::RepositionSink()
{
#if DEBUG_INFO
    LOG(WARNING) << "复位桌子 round:"<<m_Round;
#endif
}
void CTableFrameSink::openSLog(const char *p, ...)
{

    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//shz//shz_table_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
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
