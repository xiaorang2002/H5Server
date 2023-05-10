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
#include "proto/ldyx.Message.pb.h"

#include "json/json.h"
#include "ConfigManager.h"
#include "CMD_Game.h"
#include "GameLogic.h"
#include "ConfigManager.h"

using namespace ldyx;
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
        string dir = "./log/ldyx";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if (!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("ldyx");
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
    memset(m_lUserBiBeiBetOld, 0, sizeof(m_lUserBiBeiBetOld));
    memset(m_lFreeRoundWinScore, 0, sizeof(m_lFreeRoundWinScore));
    memset(m_llUserJettonScore, 0, sizeof(m_llUserJettonScore));
    memset(m_llUserSourceScore, 0, sizeof(m_llUserSourceScore));
    memset(m_lUserIsAward, 0, sizeof(m_lUserIsAward));
    memset(m_lUserAwardIndex, 0, sizeof(m_lUserAwardIndex));  
    memset(m_IsPressJetton, 0, sizeof(m_IsPressJetton));
    memset(m_llUserCurrentScore, 0, sizeof(m_llUserCurrentScore));
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
    m_lUserAwardMutical[0]=20;
    m_lUserAwardMutical[1]=20;
    m_lUserAwardMutical[2]=30;
    m_lUserAwardMutical[3]=30;
    m_lUserAwardMutical[4]=40;
    m_lUserAwardMutical[5]=40;
    m_lUserAwardMutical[6]=50;
    m_lUserAwardMutical[7]=50;
    m_lUserAwardMutical[8]=60;
    m_lUserAwardMutical[9]=60;
    m_lUserAwardMutical[10]=80;
    m_lUserAwardMutical[11]=80;
    m_lUserAwardMutical[12]=100;
    m_lUserAwardMutical[13]=100;
    m_lUserAwardMutical[14]=200;
    m_lUserAwardMutical[15]=200;
    ////普通奖项倍率
    m_GameMutical[0] = 5;
    m_GameMutical[1] = 3;
    m_GameMutical[2] = 10;
    m_GameMutical[3] = 3;
    m_GameMutical[4] = 15;
    m_GameMutical[5] = 3;
    m_GameMutical[6] = 20;
    m_GameMutical[7] = 3;
    m_GameMutical[8] = 20;
    m_GameMutical[9] = 3;
    m_GameMutical[10] = 30;
    m_GameMutical[11] = 3;
    m_GameMutical[12] = 40;
    m_GameMutical[13] = 3;
    m_GameMutical[14] = 120;
    m_GameMutical[15] = 50;
    m_GameMutical[16] = 0;
    m_GameMutical[17] = 0;

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
    StatusFree.set_nsmallmarynum(m_nMarrayNumLeft[chairId]);
    StatusFree.set_totalwin(m_lMarryTotalWin[chairId]);
    StatusFree.set_headid(m_pITableFrame->GetTableUserItem(chairId)->GetHeaderId());

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



    //获取游戏配置
    cfgManager = ConfigManager::GetInstance();
    if (cfgManager == NULL){
        LOG(WARNING) << "ConfigManager失败 ";
        return false;
    }

    int ret = ConfigManager::GetInstance()->CheckReadConfig();
    if(ret != 0){
        LOG(WARNING) << "配置文件读取失败 " << ret;
        return false;
    }
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

    m_llUserCurrentScore[dwChairID] = m_lUserScore;
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
    if(m_AllConfig.m_runAlg == 1)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerIdTest);





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
//    if(m_AllConfig.m_runAlg == 1)
//    {
//        return false;
//    }
    //用户离开
    m_bUserIsLeft = true;

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

    // 判断服务器是否在维护
    if(m_pITableFrame->GetGameRoomInfo()->serverStatus != SERVER_RUNNING){
        //修改桌子状态
        m_pITableFrame->ConcludeGame(GAME_STATUS_FREE);
        OnTimerClearUser();
        OnUserLeft(m_userId,false);
    }

}
// 
void CTableFrameSink::LeftReward(int64_t userId,uint32_t wChairID)
{

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

    LOG(WARNING)<< "收到玩家 "<<wChairID <<"命令ID"<<(int )subid ;
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


    switch (subid)
    {
        case SUB_C_JETTON:
        {


            if(m_nMarrayNumLeft[wChairID]>0)
            {
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }
            if(!m_IsPressJetton[wChairID])
            {
                for(int i=0;i<FRUIT_MAX;i++)
                 m_llUserJettonScore[wChairID][i] = 0;

                m_lUserBetScore[wChairID] = 0;
                m_IsPressJetton[wChairID]=true;
            }
            //下注时获取玩家当前分数并且记录下来
            m_llUserSourceScore[wChairID] = pUserItem->GetUserScore();
            CMD_C_JETTON  pJettonScore;
            pJettonScore.ParseFromArray(data, dwDataSize);
            if(pJettonScore.jettonindex()<0||pJettonScore.jettonindex()>7)
            {
                SendErrorCode(wChairID,ERROR_BET_SCORE);
                return false;
            }
            if(m_llUserSourceScore[wChairID]<pJettonScore.score()+m_lUserBetScore[wChairID])
            {
                SendErrorCode(wChairID,ERROR_SCORE_NOT_ENOUGH);
                return false;
            }
            if(m_AllConfig.maxBetScore<pJettonScore.score()+m_llUserJettonScore[wChairID][pJettonScore.jettonindex()*2])
            {
                SendErrorCode(wChairID,ERROR_LOW_SCORE);
                return false;
            }
            vector<int> vBetItemList = ConfigManager::GetInstance()->GetBetItemList();
            bool isRight =false;
            for(int betItem :vBetItemList)
            {
                if(betItem == pJettonScore.score() )
                {
                    isRight = true;
                    break;
                }

            }
            if(pJettonScore.score()<=0||!isRight)
            {
                LOG_WARN << "有狗逼玩家搞事情     "<<pJettonScore.score();
                for(int betItem :vBetItemList)
                {
                    LOG_WARN << "列表分值     "<<betItem;
                }
                return false ;
            }
            //下注项只有八个，即是把种水果，但是每种水果又分为大小两种
            m_llUserJettonScore[wChairID][pJettonScore.jettonindex()*2]+=pJettonScore.score();
            m_llUserJettonScore[wChairID][pJettonScore.jettonindex()*2+1]+=pJettonScore.score();
            m_lUserBetScore[wChairID]+=pJettonScore.score();


            LOG(WARNING)<< "脚本下注分数是 " << m_lUserBetScore[wChairID] <<" UserId:"<< pUserItem->GetUserId();
            CMD_S_JETTON_SUSSES Jetton;
            Jetton.set_jettonindex(pJettonScore.jettonindex());
            Jetton.set_score(pJettonScore.score());
            string data = Jetton.SerializeAsString();

            m_pITableFrame->SendTableData(wChairID, SUB_S_JETTON_SUSSES, (uint8_t *)data.c_str(), data.size());
            return true;
        }
        case SUB_C_BIBEI_ADDDROP://
        {

            if(m_nMarrayNumLeft[wChairID]>0)
            {
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }
            if(m_lUserBiBeiBetOld[wChairID]<=0||m_lUserBiBeiBet[wChairID]<=0)
            {
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }
            if(m_nMarrayNumLeft[wChairID]>0)
            {
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }
            int64_t oldbetScore= m_lUserBiBeiBetOld[wChairID]*2;
            CMD_C_BIBEI_ADDDROP  pBibeiCut;
            pBibeiCut.ParseFromArray(data, dwDataSize);
            int64_t oldScore = m_lUserBiBeiBet[wChairID];
            //奇数
            int num=0;
            float cent=m_lUserBiBeiBetOld[wChairID]/100;
            std::vector<int> Iput;
            Iput.clear();
            //可以下两倍
            if(pUserItem->GetUserScore()-m_lUserBiBeiBetOld[wChairID]>m_lUserBiBeiBetOld[wChairID])
            {
                Iput.push_back(m_lUserBiBeiBetOld[wChairID]*2);
                Iput.push_back(m_lUserBiBeiBetOld[wChairID]);
            }
            else
            {
                Iput.push_back(m_lUserBiBeiBetOld[wChairID]);
            }
            do
            {
                num++;
                cent=cent/2;
                cent=ceil(cent);
                Iput.push_back(cent*100);
            }while(cent>1);

            int index = 0;
            if(pBibeiCut.zuoyou()==1) //减少比倍分数
            {
                for(int i=0;i<(int)Iput.size();i++)
                {
                    if(m_lUserBiBeiBet[wChairID]==Iput[i])
                    {
                        index = i;
                        break;
                    }
                }
                if(index==0)
                {
                    m_lUserBiBeiBet[wChairID]=Iput[0];
                }
                else
                {
                    m_lUserBiBeiBet[wChairID]=Iput[index-1];
                }
            }
            else if(pBibeiCut.zuoyou()==0)//增加比倍分数
            {
                for(int i=0;i<(int)Iput.size();i++)
                {
                    if(m_lUserBiBeiBet[wChairID]==Iput[i])
                    {
                        index = i;
                        break;
                    }
                }
                if(index==((int)Iput.size()-1))
                {
                    m_lUserBiBeiBet[wChairID]=Iput[(int)Iput.size()-1];
                }
                else
                {
                    m_lUserBiBeiBet[wChairID]=Iput[index+1];
                }
            }
            if( m_lUserBiBeiBet[wChairID]-m_lUserBiBeiBetOld[wChairID]>pUserItem->GetUserScore()-m_lUserBiBeiBetOld[wChairID])
            {

                SendErrorCode(wChairID,ERROR_SCORE_NOT_ENOUGH);
                return false;
            }
            m_llUserCurrentScore[wChairID]+=(oldScore -m_lUserBiBeiBet[wChairID]);
            CMD_C_BIBEI_ADDDROP   bebei;
            bebei.set_zuoyou( m_lUserBiBeiBet[wChairID]);
            string data = bebei.SerializeAsString();
            m_pITableFrame->SendTableData(wChairID, SUB_S_BIBEI_ADD_SUSSES, (uint8_t *)data.c_str(), data.size());
            return true;
        }
        case SUB_C_START:
        {
            if(m_nMarrayNumLeft[wChairID]>0)
            {
                SendErrorCode(wChairID,ERROR_STATUS);
                return false;
            }
            //频繁拉取控制变量
            if(m_nPullInterval == 0){ 
                m_nPullInterval = m_AllConfig.nPullInterval;
            }

            //频繁拉取控制变量
            if(m_lUserBetScore[wChairID] <= 0)
            {
                SendErrorCode(wChairID,ERROR_BET_SCORE);
                m_lUserBetScore[wChairID] = 0;
                return false;
            }
            CMD_C_GameStart pGamePulling;
            pGamePulling.ParseFromArray(data, dwDataSize);
            
            openSLog("------------------------玩家拉取普通开奖结果开始------------------------");
            for(int i=0;i<18;i++)
            {
                openSLog("玩家每门下注值m_llUserJettonScore[wChairID][%d]=%d",i,m_llUserJettonScore[wChairID][i]);
            }
            openSLog("总共下注 = %d",m_lUserBetScore[wChairID]);
            //押分分数太大，超过玩家分
            if ( m_lUserBetScore[wChairID] > pUserItem->GetUserScore())
            {
                openSLog("不好意思，玩家下注分值已经超过了玩家分数");
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
    
            //修改用户状态
            m_pITableFrame->SetUserReady(wChairID);
            //清除数据
            m_nMarrayNumLeft[wChairID] = 0;
            m_nMarryNum[wChairID] = 0;
            m_lBibeiAllWin[wChairID] = 0;
            m_lUserBiBeiBet[wChairID] = 0;
            m_lUserBiBeiBetOld[wChairID] = 0;
            m_lUserIsAward[wChairID]=0;       //是否中大奖,方便后面计算分数
            m_lUserAwardIndex[wChairID]=0;    //中大奖的具体水果下标
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

            m_IsPressJetton[wChairID] = false;//
//            for(int i=0;i<FRUIT_MAX;i++)
//             m_llUserJettonScore[wChairID][i] = 0;

//            m_lUserBetScore[wChairID] = 0;
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
                 //玛利机状态下不能操作比倍
                 SendErrorCode(wChairID,ERROR_STATUS);
                 return false;
             }
             if(m_lUserBiBeiBet[wChairID]<=0)
             {
                LOG(WARNING) << "没有可以比倍的分数!";
                openSLog("没有可以比倍的分数 m_lUserBiBeiBet[wChairID]=%d",m_lUserBiBeiBet[wChairID]);
                SendErrorCode(wChairID,(enErrorCode)1);
                return false;
             }
             if ((time(NULL) - m_tLastOperate[wChairID]) < 1)
             {
                 SendErrorCode(wChairID,ERROR_STATUS);
                 return false;
             }
             m_tLastOperate[wChairID]= time(NULL);

             if(m_lUserBiBeiBet[wChairID]>pUserItem->GetUserScore())
             {
                 SendErrorCode(wChairID,(enErrorCode)1);
                 return false;
             }
             CMD_C_BIBEI_BET pGameBetting;
             pGameBetting.ParseFromArray(data, dwDataSize);
             int32_t bettingPlace = pGameBetting.betplace();
             if(bettingPlace<0||bettingPlace>1)
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
    cfgManager = ConfigManager::GetInstance();
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

    bool isControlKill = false;
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        isControlKill = true;
    }
    do
    {
        memset(&CMDGameOneEnd, 0, sizeof(CMDGameOneEnd));      
        //超过五百次就出一个没有奖项的结果
        if ((++Count) > 500)
        {
            LOG(WARNING) << "Count > 500";
            //没有奖项的结果
            openSLog("开奖循环超过了500次，所以出一个没奖的结果 ");

            break;
        }
 
        //填充命令参数,并且按照水浒图标的权重随机出结果
        FillTable(CMDGameOneEnd);

        if(m_AllConfig.m_testLine>0)
        {
           CMDGameOneEnd.nResultIndex = m_AllConfig.m_line[0] ;
        }
        else if(m_AllConfig.m_testMarry>0)
        {
            CMDGameOneEnd.nResultIndex = IMAGE_YELLO_LUCKY ;
        }
        //计算分数
        CalculateScore(CMDGameOneEnd, wChairID);

        //记录当前图标数据
        iconResStr = "";
        int lineCount = 0;
        //string tmp97 = "";
        //记录
        int FreeCount = 0;
        string tmpRewardInfo = str(boost::format("%d")  % CMDGameOneEnd.nMarryNum);


        string strMultical="";
        if(CMDGameOneEnd.nResultIndex<IMAGE_BLUE_LUCKY)
        {
            strMultical= str(boost::format("%d")  % m_GameMutical[CMDGameOneEnd.nResultIndex]);
        }
        else
        {
            strMultical= "0";
        }
        //string tmpScatterWin = str(boost::format("%d-%d") % 0 % 0);
        cardValueStr =str(boost::format("%d|%d|%d|%d|%d") % CMDGameOneEnd.nResultIndex % CMDGameOneEnd.Score % CMDGameOneEnd.nMarryNum  % m_llUserJettonScore[wChairID][CMDGameOneEnd.nResultIndex] % strMultical);

        int64_t AllMarryWinScore=0;
        //填充超级玛利的结果列表
        GetAllResult(CMDGameOneEnd,wChairID ,CMDGameOneEnd.nMarryNum,AllMarryWinScore);
        //押注库存验证,然后重新出结果
        //超过二十万的结果暂时不开放，以后人数多了在考虑
        if (CMDGameOneEnd.Score > 0L||AllMarryWinScore>0) {
           if(!CheckStorageVal(CMDGameOneEnd.Score+AllMarryWinScore)||AllMarryWinScore>20000000)
           {
                continue;
           }
        } 
        if(isControlKill)
        {
            if( CMDGameOneEnd.Score+AllMarryWinScore>=lbetScore)
            {
                continue;
            }

        }
        break;

    } while (true);



    openSLog("循环出结果已经结束，产生的水果下表是 = %d", CMDGameOneEnd.nResultIndex);
    openSLog("循环出结果已经结束，产生的玛利游戏次数 = %d", CMDGameOneEnd.nMarryNum);
    openSLog("循环出结果已经结束，本次中奖的分数 = %d", CMDGameOneEnd.Score);
    openSLog("循环出结果已经结束，本次中奖的倍数 = %d", m_GameMutical[CMDGameOneEnd.nResultIndex]);
    if(CMDGameOneEnd.nResultIndex<IMAGE_BLUE_LUCKY)
     openSLog("循环出结果已经结束，本次中奖水果的下注分数 = %d", m_llUserJettonScore[wChairID][CMDGameOneEnd.nResultIndex]);
    //记录当前图标数据
    iconResStr = "";
    int lineCount = 0;

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
    m_lUserBiBeiBetOld[wChairID] = CMDGameOneEnd.Score;
    m_lUserBiBeiBet[wChairID] = CMDGameOneEnd.Score;
	m_lUserWinScore[wChairID] = CMDGameOneEnd.Score;
    m_nMarryNum[wChairID] = CMDGameOneEnd.nMarryNum;
	m_nMarrayNumLeft[wChairID] = CMDGameOneEnd.nMarryNum;

    m_llUserCurrentScore[wChairID]-=lbetScore;
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
    if(lbetScore<=0)
       lbetScore=1;
    if((lbetScore>0 && CMDGameOneEnd.Score/lbetScore >= m_AllConfig.m_MinBoardCastOdds)
    ||CMDGameOneEnd.Score > m_pGameRoomInfo->broadcastScore)
    {
        string muticle="";
        muticle = to_string(CMDGameOneEnd.Score/(lbetScore));
        m_pITableFrame->SendGameMessage(wChairID, muticle,SMT_GLOBAL|SMT_SCROLL, CMDGameOneEnd.Score);
    }

    //复制数据
    CMD_S_RoundResult cmdData;

    cmdData.set_score(CMDGameOneEnd.Score);
    cmdData.set_lcurrentscore(CMDGameOneEnd.lCurrentScore);
    cmdData.set_nmarrynum(CMDGameOneEnd.nMarryNum);//玛丽次数
    cmdData.set_roundid(mGameIds);//牌局id
    cmdData.set_fruitindex(CMDGameOneEnd.nResultIndex);
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

        m_replay.addResult(wChairID, wChairID,lBetScore , ScoreData.addScore, "", false);
        m_replay.addStep((uint32_t)time(NULL) - groupStartTime, "比倍", -1, 0, wChairID, wChairID);

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
    Item = ScoreGetProbability(GetTotalStock());
    //////////////////////////////////
    int count = 0;
    int64_t winScore = 0;
    int64_t betScore = m_lUserBiBeiBet[wChairID];
    int FruitIndex[1] = {0};
    int32_t FruitOdd = 0L;
    int PlayerbetPlace = bettingPlace;//玩家下注位置
    int openIndex=0;
    int winOrLose  = 0;
    do
    {
        // chongxin  tongji
        winScore = 0;
        FruitIndex[0] = m_random.betweenInt(1,14).randInt_mt(true);

        if(m_AllConfig.m_testBibei)
        {
            FruitIndex[0] =  m_AllConfig.m_bibiList[0];           
        }
        assert( FruitIndex[0] >= 1);
        assert( FruitIndex[0] <= 14);
        //0 = da 1 = xiao 2 = he

        if(FruitIndex[0]<=7)
        {
           openIndex=0;
        }
        else
        {
            openIndex=1;
        }
        if(PlayerbetPlace==0)//大
        {
           if(openIndex==0)
           {
               winScore=betScore*2;// 多出1倍
           }
           else
           {
               winScore=0; //从写分的角度那这次 他是输的
           }
        }
        else //等于7 和
        {
            if(openIndex==1)
            {
                 winScore=betScore*2; //从写分的角度那这次 他是输的
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

    cardValueStr = str(boost::format("%d|%d|%d") % FruitIndex[0] % (winScore-betScore) % PlayerbetPlace);
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
        m_lUserBiBeiBetOld[wChairID] = winScore;
        m_llUserCurrentScore[wChairID]+=winScore;
    }
    else
    {
        m_lUserBiBeiBet[wChairID] = 0;
        m_lUserBiBeiBetOld[wChairID] =0;
        m_lBibeiAllWin[wChairID] = 0;//全部清零
        m_llUserCurrentScore[wChairID]+=winScore;
        m_lUserBiBeiBet[wChairID] = 0;//全部清零
        m_lUserBiBeiBetOld[wChairID] = 0;
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
    BibeiEnd.set_daxiao(FruitIndex[0]);
    openSLog("本次比倍赢分 winScore=%d   m_lBibeiAllWin[wChairID]=%d",winScore,m_lBibeiAllWin[wChairID]);
    openSLog("结算后玩家分数 m_llUserSourceScore[wChairID]=%d  ",m_llUserSourceScore[wChairID]);
    openSLog("玩家下注分数 betscore=%d  ",betScore);
    openSLog("是否结束 isover=%d  ",winOrLose);
    openSLog("骰子一 =%d ",FruitIndex[0]);

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


void CTableFrameSink::GetAllResult(CMD_S_RoundResult_Tag &SceneOneEnd,int chairid ,int  marrytimes,int64_t &allMarryWin)
{

    UserProbabilityItem PersonConfig;
    memset(&PersonConfig, 0, sizeof(PersonConfig));
    PersonConfig =  ScoreGetProbability(GetTotalStock());
    //在这里出结果吧

    if(m_AllConfig.m_testMarry>0)
    {
       SceneOneEnd.nResultIndex = IMAGE_YELLO_LUCKY;
       SceneOneEnd.nMarryNum = m_AllConfig.m_marryIndex;
    }
    int32_t numMarry = SceneOneEnd.nMarryNum;
    int32_t sameFruit = 0;
    int32_t oldindex = 0;
    vtMarryResult.clear();
    int fruitNumCount[IMAGE_BLUE_LUCKY] = {0};
    int counttime=0;
    do
    {
        if(numMarry<=0)
        {
            break;
        }
        int32_t tsum = 0L;
        int32_t fruitIndex = 0;
        MarryInfo marryinfo;
        if(m_AllConfig.m_testMarry>0)
        {
           fruitIndex = m_AllConfig.m_marryList[counttime];
           fruitNumCount[fruitIndex]++;
           counttime++;
        }
        else
        {
            for (int i = 0; i < IMAGE_BLUE_LUCKY; i++)
            {
                tsum += PersonConfig.nFruitPro[i];
            }
            int32_t randint=m_random.betweenInt(0,tsum).randInt_mt(true);
            int addsum=0;

            for(int i = 0; i < IMAGE_BLUE_LUCKY; i++)
            {
                addsum+=PersonConfig.nFruitPro[i];
                if(addsum>randint)
                {
                    fruitIndex = i;
                    fruitNumCount[i]++;
                    break;
                }

            }
        }

        numMarry--;
        allMarryWin+=m_llUserJettonScore[chairid][fruitIndex]*m_GameMutical[fruitIndex];
        for(int i = 0; i < IMAGE_SMA_BAR; i+=2)
        {
            //这里证明有游戏机出现
            if(fruitNumCount[i]+fruitNumCount[i+1]>=3)
            {
               SceneOneEnd.nMarryNum = SceneOneEnd.nMarryNum -  numMarry;
               numMarry = 0;
               marryinfo.isAward = 1;
               m_lUserAwardIndex[chairid] = i;
               m_lUserIsAward[chairid] = 1;
               allMarryWin = m_lUserAwardMutical[i]*m_lUserBetScore[chairid];
               break;
            }
        }

        marryinfo.index = fruitIndex;
        marryinfo.currentWin = m_llUserJettonScore[chairid][fruitIndex]*m_GameMutical[fruitIndex];
        marryinfo.allWin = allMarryWin;       
        vtMarryResult.push_back(marryinfo);



    }while(numMarry>0);

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
    int fruitIndex = 0;
    int64_t WinScore = 0;

    //先减后写


    fruitIndex = vtMarryResult[0].index;

    WinScore = vtMarryResult[0].currentWin;

    m_lMarryTotalWin[wChairID] = vtMarryResult[0].allWin;
    vtMarryResult.erase(vtMarryResult.begin());
    openSLog("--------------------进入玛丽游戏拉去结果环节--------------------");

    openSLog("本次玛利的中奖下标是:  %d",fruitIndex);
    openSLog("本次玛丽的赢分是:  %d",WinScore);
    openSLog("本次玛利的总赢分是:  %d",m_lMarryTotalWin[wChairID]);
    openSLog("剩余玛利次数  = %d ",(int)vtMarryResult.size());



    //记录
    int iswin=((WinScore==0)?0:1);
    m_nMarrayNumLeft[wChairID]--;

    int64_t llGameTax=0;

    ///假如中大奖,那就每次都写但是赢分只写最后一次
    if(!m_lUserIsAward[wChairID])
    {
        cardValueStr = str(boost::format("%d|%d|%d|%d|%d|%d")%m_lUserIsAward[wChairID] % m_nMarrayNumLeft[wChairID] % fruitIndex %  m_llUserJettonScore[wChairID][fruitIndex] % WinScore % m_GameMutical[fruitIndex]);
        LOG(WARNING) << cardValueStr;
        if (MarrySceneWriteScore(wChairID, WinScore, llGameTax,cardValueStr) == false) {
            return ERROR_OPERATE;
        }
    }
    else
    {
         if(m_nMarrayNumLeft[wChairID]==0)
         {
             int64_t MarryMul= 0;
             if(m_llUserCurrentScore[wChairID]==0)
                 MarryMul = 0;
             else
                 MarryMul = m_lMarryTotalWin[wChairID]/m_lUserBetScore[wChairID];

             cardValueStr = str(boost::format("%d|%d|%d|%d|%d|%d")%m_lUserIsAward[wChairID] % m_nMarrayNumLeft[wChairID] % fruitIndex %  m_lUserBetScore[wChairID] % m_lMarryTotalWin[wChairID] % MarryMul);
             LOG(WARNING) << cardValueStr;
             if (MarrySceneWriteScore(wChairID, m_lMarryTotalWin[wChairID], llGameTax,cardValueStr) == false) {
                 return ERROR_OPERATE;
             }
         }
         else
         {
             cardValueStr = str(boost::format("%d|%d|%d|%d|%d|%d")%m_lUserIsAward[wChairID] % m_nMarrayNumLeft[wChairID] % fruitIndex %  m_llUserJettonScore[wChairID][fruitIndex] % 0 % 0);
             LOG(WARNING) << cardValueStr;
             if (MarrySceneWriteScore(wChairID, 0, llGameTax,cardValueStr) == false) {
                 return ERROR_OPERATE;
             }
         }
    }

    //小玛利的时候就写 最后一次结果就可以了
    if(m_nMarrayNumLeft[wChairID]==0)
    {
        //m_llUserCurrentScore[wChairID]-=m_lMarryTotalWin[wChairID];
        //玛利结束也可以进行比倍      
        m_lUserBiBeiBet[wChairID] = m_lMarryTotalWin[wChairID];
        m_lUserBiBeiBetOld[wChairID]  = m_lMarryTotalWin[wChairID];
    }




     
    //命令组装
    CMD_S_MarryResult MarryEnd;
    MarryEnd.set_currentwin(WinScore - llGameTax);
    MarryEnd.set_totalwin(m_lMarryTotalWin[wChairID]);
    MarryEnd.set_fruitindex(fruitIndex);
    MarryEnd.set_userscore(m_llUserSourceScore[wChairID]);
    MarryEnd.set_nmarryleft(m_nMarrayNumLeft[wChairID]);
    MarryEnd.set_nmarrynum(m_nMarryNum[wChairID]);
    MarryEnd.set_roundid(mGameIds);
    MarryEnd.set_betscore(m_lMarryTotalWin[wChairID]);
    openSLog("发送玛利数据 currentwin = %d    totalwin=%d   fruitIndex=%d   marryleft=%d   marrynum=%d",WinScore - llGameTax, m_lMarryTotalWin[wChairID] , fruitIndex,m_nMarrayNumLeft[wChairID],m_nMarryNum[wChairID]);
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

    //库存优先黑名单起主要作用
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
        int64_t allJetton=m_lUserBetScore[user->GetChairId()];
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
                 if(GetTotalStock() <GetHighStock())
                 {

                    if(m_random.betweenInt(0,1000).randInt_mt(true)<shuzhi)
                    {
                        //黑名单控制也同样用第0组杀分控制权重
                       PersonConfig = m_AllConfig.vProbability[0];
                    }
                     //openSLog("黑名单玩家 :%d 确定调用黑名单算法 每门传的黑名单控制概率是[wild]：%d",user->GetUserId(),PersonConfig.nFruitPro[8]);
                 }
            }
        }
     }

    //////////////////////////////////
    /// /////////////////////////////

    //测试打印
    char szFile[256] = { 0 };
    char szTemp[64] = { 0 };
    //在这里出结果吧
    int32_t tsum = 0L;
    for (int i = 0; i < FRUIT_MAX; i++)
    {
        tsum += PersonConfig.nFruitPro[i];
    }
    int32_t randint=m_random.betweenInt(0,tsum).randInt_mt(true);
    int addsum=0;
    for(int i = 0; i < FRUIT_MAX; i++)
    {
        addsum+=PersonConfig.nFruitPro[i];
        if(addsum>randint)
        {
            SceneOneEnd.nResultIndex =i;
            break;
        }
    }



    return true;
}

//scatter和bonus规则检查
bool CTableFrameSink::CheckRule(CMD_S_RoundResult_Tag &SceneOneEnd, bool IsFreeScene)
{
    return true;
}
//计算分数
bool CTableFrameSink::CalculateScore(CMD_S_RoundResult_Tag &SceneOneEnd, const int32_t wChairID)
{
    //不出lucky FRUIT_MAX
    if(SceneOneEnd.nResultIndex<IMAGE_BLUE_LUCKY)
    {
        SceneOneEnd.Score = m_llUserJettonScore[wChairID][SceneOneEnd.nResultIndex]*m_GameMutical[SceneOneEnd.nResultIndex];
        SceneOneEnd.nMarryNum = 0;
    }
    else if(SceneOneEnd.nResultIndex==IMAGE_BLUE_LUCKY)//蓝色lucky
    {
        //随机马丽机次数
        SceneOneEnd.nMarryNum = m_random.betweenInt(1,4).randInt_mt(true);
        SceneOneEnd.Score = 0;
    }
    else if(SceneOneEnd.nResultIndex==IMAGE_YELLO_LUCKY)//黄色lucky
    {
        //随机马丽机次数
        SceneOneEnd.nMarryNum=m_random.betweenInt(5,8).randInt_mt(true);
        SceneOneEnd.Score = 0;
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

//        m_replay.addResult(wChairID, wChairID, m_lUserBetScore[wChairID], ScoreData.addScore, "", false);
//        m_replay.addStep((uint32_t)time(NULL) - groupStartTime, "小玛利", -1, 0, wChairID, wChairID);

        m_replay.addResult(wChairID, 0, 0, ScoreData.addScore, "", false);
        m_replay.addStep((uint32_t)time(NULL) - groupStartTime, "小玛利", -1, 0, wChairID, 0);

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
    m_IsPressJetton[dwChairID] = false;//
    m_tLastOperate[dwChairID] = 0L;    //
    m_lUserBiBeiBet[dwChairID] = 0;//全部清零
    m_lUserBiBeiBetOld[dwChairID] = 0;
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

    sprintf(Filename, "log//ldyx//ldyx_table_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
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
