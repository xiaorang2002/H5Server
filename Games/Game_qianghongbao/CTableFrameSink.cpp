#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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

#include "proto/qhb.Message.pb.h"

#include "GameLogic.h"
//#include "HistoryScore.h"
// #include "IAicsdragon.h"

#include <muduo/base/Mutex.h>

#include "LongHuAlgorithm.h"
#include "public/StdRandom.h"
using namespace qhb;

#include "CTableFrameSink.h"

#define MIN_LEFT_CARDS_RELOAD_NUM  24
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0



// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = "./log/qhb/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("qhb");
        // google::SetStderrLogging(google::GLOG_ERROR);
        google::SetStderrLogging(google::GLOG_INFO);
    }

    virtual ~CGLogInit()
    {
        google::ShutdownGoogleLogging();
    }
};

CGLogInit glog_init;

class RomMgr
{
public:
    RomMgr()
    {
        bisInit = false;

        if(access("./log/qhb", 0) == -1)
           mkdir("./log/qhb",0777);
    }

    virtual ~RomMgr()
    {
        google::ShutdownGoogleLogging();
    }

public:
    static RomMgr& Singleton()
    {
        static RomMgr* sp = 0;
        if (!sp)
        {
             sp = new RomMgr();
        }
        return (*sp);
    }
private:
     bool bisInit;
};


//////////////////////////////////////////////////////////////////////////


//构造函数
CTableFrameSink::CTableFrameSink()// : m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "LongHuTimerEventLoopThread")
{
    m_strRoundId = "";
    m_IsEnableOpenlog = false;
    m_nReadyTime = 0;
    m_nStartTime = 0;
    m_nGameEndTime = 0;
    m_UserInfo.clear();
    m_pGameRoomInfo = nullptr;
    m_dControlkill=0.0;
    stockWeak = 0.0;

    m_nBankerUserId = INVALID_CHAIR;
    m_nLeftEnvelopeNum = 0;

    m_SendEnvelopeScore=0;                //每次发红包得总钱数
    m_SendEnvelopeNum=10;                  //每次发送红包最大个数
    m_BestLuckyScore=0;                   //最佳手气的钱数
    m_BankerLeftSocre=0;                  //本次没有抢完的红包钱数
    return;
}
//析构函数
CTableFrameSink::~CTableFrameSink(void)
{

}

//读取配置
bool CTableFrameSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/qhb_config.ini"))
    {
        LOG_INFO << "./conf/qhb_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/qhb_config.ini", pt);

    stockWeak=  pt.get<double>("GAME_CONFIG.STOCK_WEAK", 2.0);
    //Time
    m_nReadyTime         = pt.get<int64_t>("GAME_CONFIG.m_nReadyTime", 5);
    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.m_nGameEndTime", 5);
    m_nStartTime         = pt.get<float>("GAME_CONFIG.m_nStartTime", 5);
    m_dControlkill       = pt.get<double>("GAME_CONFIG.Controlkill", 0.8);
    m_IsEnableOpenlog    = pt.get<uint32_t>("GAME_CONFIG.IsEnableOpenlog", 0);
    m_SendEnvelopeNum  = pt.get<uint32_t>("GAME_CONFIG.m_SendEnvelopeNum", 10);

    string path="GAME_ROOM"+to_string(m_pGameRoomInfo->roomId)+".m_SendEnvelopeScore";
    m_SendEnvelopeScore  = pt.get<uint32_t>(path.c_str(), 10000);
}

bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{

    LOG(INFO) << "CTableFrameSink::SetTableFrame pTableFrame:" << pTableFrame;

    m_pITableFrame = pTableFrame;
    if (pTableFrame)
    {
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        m_replay.cellscore = m_pGameRoomInfo->floorScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata

        ReadConfigInformation();
        return true;
    }else
        return false;
}

//游戏开始
void CTableFrameSink::OnGameStart()
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
bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    return true;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;

    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
        return true;
    }else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(userInfo->isBanker||userInfo->haveGrab)
        {
            canLeft=false;
        }
    }

    return canLeft;
}

bool CTableFrameSink::OnUserEnter(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;
    shared_ptr<CServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL userId:"<<userId;
        return false;
    }
    LOG_DEBUG << "OnUserEnter userId:"<<userId;
    uint32_t chairId = pIServerUserItem->GetChairId();


    auto it = m_UserInfo.find(userId);
    if(m_UserInfo.end() == it)
    {
        shared_ptr<UserInfo> userInfo(new UserInfo());
        userInfo->userId = userId;
        userInfo->chairId = chairId;
        userInfo->headerId = pIServerUserItem->GetHeaderId();
        userInfo->nickName = pIServerUserItem->GetNickName();
        userInfo->location = pIServerUserItem->GetLocation();
        m_UserInfo[userId] = userInfo;
    }
    pIServerUserItem->SetUserStatus(sPlaying);

    OnEventGameScene(chairId, false);
    //第一次开始并且有机器人的时候,才启动定时器，设定机器人为庄家
    if(GAME_STATUS_INIT == m_pITableFrame->GetGameStatus()&&pIServerUserItem->IsAndroidUser())
    {
        if(m_nBankerUserId == INVALID_CHAIR)
        {
            m_nBankerUserId = pIServerUserItem->GetUserId();
        }
        auto it = m_UserInfo.find(m_nBankerUserId);
        if(m_UserInfo.end() != it)
        {
            m_UserInfo[m_nBankerUserId]->isBanker =true;
        }
        m_startTime = chrono::system_clock::now();
        m_strRoundId = m_pITableFrame->GetNewRoundId();
        m_replay.gameinfoid = m_strRoundId;
        //m_dwReadyTime = (uint32_t)time(NULL);
        m_TimerReadyId = m_TimerLoopThread->getLoop()->runAfter(0.1, boost::bind(&CTableFrameSink::OnTimerReady, this));
        m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
    }
    return true;
}
void CTableFrameSink::SendReady(int32_t chairId)
{
    shared_ptr<IServerUserItem> player = m_pITableFrame ->GetTableUserItem(chairId);
    shared_ptr<IServerUserItem> banker = m_pITableFrame ->GetUserItemByUserId(m_nBankerUserId);

    if(!player||!banker)
    {
        if(!player)
        {
            LOG_ERROR<<"player Not exist !";
        }
        if(!banker)
        {
            LOG_ERROR<<"banker Not exist !";
        }
        return;
    }
    int64_t playerId = player->GetUserId();
    int64_t bankerId = banker->GetUserId();
    CMD_S_Ready readyMsg;
    readyMsg.set_bankheaderid(banker->GetHeaderId());
    readyMsg.set_bankuserid(bankerId);
    readyMsg.set_isbanker(m_UserInfo[playerId]->isBanker);
    readyMsg.set_roundid(m_strRoundId);
    std::string data = readyMsg.SerializeAsString();
    //LOG(WARNING) << "发送READY消息  :"<<(int)data.size();
    m_pITableFrame->SendTableData(chairId, qhb::SUB_S_READY, (uint8_t *)data.c_str(), data.size());


}
void CTableFrameSink::SendStart(int32_t chairId)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame ->GetTableUserItem(chairId);
    shared_ptr<CServerUserItem> banker = m_pITableFrame ->GetUserItemByUserId(m_nBankerUserId);

    if(!player||!banker)
    {
        if(!player)
        {
            LOG_ERROR<<"player Not exist !";
        }
        if(!banker)
        {
            LOG_ERROR<<"banker Not exist !";
        }
        return;
    }

    int64_t playerId = player->GetUserId();
    int64_t bankerId = banker->GetUserId();

    CMD_S_GameStart gameStart;
    gameStart.set_bankheaderid(banker->GetHeaderId());
    gameStart.set_bankuserid(bankerId);
    gameStart.set_leftenvelope(10);
    gameStart.set_isbanker(m_UserInfo[playerId]->isBanker);

    //LOG_ERROR<<"banker id "<<bankerId <<"   m_UserInfo[playerId]->isBanker "<<m_UserInfo[playerId]->isBanker;
    std::string data = gameStart.SerializeAsString();
    //LOG(WARNING) << "发送开始抢红包消息  :"<<(int)data.size();
    m_pITableFrame->SendTableData(chairId, qhb::SUB_S_START_GRABENVELOPE, (uint8_t *)data.c_str(), data.size());
   
}
void CTableFrameSink::SendEnd(int32_t chairId)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame ->GetTableUserItem(chairId);
    if(!player)
    {
        LOG_ERROR<<"player Not exist !";
        return;
    }
    int64_t playerId = player->GetUserId();

    CMD_S_GameEnd gameEnd;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if (userInfoItem->haveGrab&&!userInfoItem->isBanker)
        {
            qhb::CMD_S_GrabPlayerList *player = gameEnd.add_playerlist();
            player->set_headid(userInfoItem->headerId);
            player->set_isbanker(userInfoItem->isBanker);
            player->set_userid(userInfoItem->userId);
            player->set_winscore(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue);
            //LOG(WARNING) << "  userInfoItem->m_EndWinScore:"<<userInfoItem->m_EndWinScore;
           
        }
        else if(userInfoItem->haveGrab&&userInfoItem->isBanker)
        {
            qhb::CMD_S_GrabPlayerList *player = gameEnd.add_playerlist();
            player->set_headid(userInfoItem->headerId);
            player->set_isbanker(userInfoItem->isBanker);
            player->set_userid(userInfoItem->userId);
            player->set_winscore(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue-m_SendEnvelopeScore);
            //LOG(WARNING) << "  userInfoItem->m_EndWinScore:"<<userInfoItem->m_EndWinScore;
        }
    }
    gameEnd.set_allwinscore(m_UserInfo[playerId]->m_UserAllWin);

    gameEnd.set_isbanker(m_UserInfo[playerId]->isBanker);
    if(m_UserInfo[playerId]->isBanker)
    {
        gameEnd.set_winscore(m_UserInfo[playerId]->m_EndWinScore-m_UserInfo[playerId]->m_Revenue);
        
    }
    else
    {
        gameEnd.set_winscore(m_UserInfo[playerId]->m_EndWinScore-m_UserInfo[playerId]->m_Revenue);
        
    }
    gameEnd.set_bankerleftscore(m_UserInfo[playerId]->m_SendLeftScore);
    gameEnd.set_userscore(player->GetUserScore());

    string data = gameEnd.SerializeAsString();
    //LOG(WARNING) << "发送结算消息  :"<<(int)data.size();
    m_pITableFrame->SendTableData(chairId,qhb::SUB_S_GAME_END,(uint8_t *)data.c_str(),data.size());
   
}
//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL chairId:"<<chairId;
        return false;
    }
    uint8_t cbGameStatus = m_pITableFrame->GetGameStatus();
    int64_t userId = pIServerUserItem->GetUserId();
    LOG(INFO) << "CTableFrameSink::OnEventGameScene chairId:" << chairId << ", userId:" << userId << ", cbGameStatus:" << cbGameStatus;

//    switch(cbGameStatus)
//    {
//        case GAME_STATUS_FREE:		//空闲状态
//        {

//        }
//        break;
//        case GAME_STATUS_START:	//游戏状态
//        {

//        }
//        break;
//        case GAME_STATUS_END:
//        {

//            return true;
//        }
//        break;
//    }
    SendScenes(chairId);
    return true;
    //效验错误
    assert(false);

}
//这里产生的红包只能是整数,只是扩大了一百倍,而且最佳手气只有一个，不会重复，只是不对外公布
void CTableFrameSink::GetResultArr(int32_t arrsize,std::vector<vecHongBao> &vec,int64_t money,int64_t bestluck)
{
    int Arrsize = arrsize;
    int weight[arrsize];
    int allWeight=0;
    int leftScore = 0;
    vec.clear();
    bool isOk = false;
    do
    {
        isOk = false;
        vec.clear();
        leftScore = 0;
        allWeight = 0;
        for(int i=0;i<Arrsize;i++)
        {
            weight[i]=m_random.betweenInt(1,100).randInt_mt(true);
            allWeight+=weight[i];
        }
        int ranInt = m_random.betweenInt(1,allWeight).randInt_mt(true);;

        for(int i=0;i<Arrsize;i++)
        {
            vecHongBao hongbao;
            if(i==Arrsize-1)
            {
                hongbao.money = money-leftScore;
                vec.push_back(hongbao);
            }
            else
            {
                hongbao.money = (weight[i]*money)/allWeight+1;
                vec.push_back(hongbao);
                leftScore+=(weight[i]*money)/allWeight+1;
            }
        }
        for(int k=0;k<Arrsize;k++)
        {
            for(int j=k+1;j<Arrsize;j++)
            {
                if(vec[k].money>=vec[j].money)
                {
                   continue;
                }
                else
                {
                   int32_t res= vec[k].money;
                    vec[k].money=vec[j].money;
                    vec[j].money=res;
                }
            }
        }
        vec[0].isBest = false;
        //最佳玩家有相同重来
        if(vec[0].money==vec[1].money)
        {
            isOk =true;
        }
    }while(isOk);
    bestluck = vec[0].money;
}
bool CTableFrameSink::AnswerPlayersGrab(int32_t chairId ,shared_ptr<IServerUserItem> player)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {        
        return false;
    }
    CMD_S_PlayerGrab playerGrab;
    playerGrab.set_headid(player->GetHeaderId());
    playerGrab.set_leftenvelopenum(m_nLeftEnvelopeNum);
    playerGrab.set_userid(player->GetUserId());
    string data = playerGrab.SerializeAsString();   
    m_pITableFrame->SendTableData(chairId,qhb::SUB_S_GRAB_ENVELOPE,(uint8_t *)data.c_str(), data.size());
    return true;
}
//发送场景消息
void CTableFrameSink::SendScenes(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " SendOpenRecord pIServerUserItem==NULL chairId:"<<chairId;
        return;
    }
    int64_t userid = pIServerUserItem->GetUserId();
    int64_t headid = pIServerUserItem->GetHeaderId();
    int64_t userScore = pIServerUserItem->GetUserScore();
    CMD_S_Scenes scenes;
    scenes.set_encelopenum(m_SendEnvelopeNum);   //发多少个红包
    scenes.set_encelopescore(m_SendEnvelopeScore); //发红包的金额
    scenes.set_sendtime(m_nReadyTime);         //等待发红包的时间
    scenes.set_endtime(m_nGameEndTime);          //抢完红包结算的时间
    scenes.set_grabtime(m_nStartTime);         //抢红包时间
    scenes.set_headid(headid);           //玩家头像id
    scenes.set_userid(userid);      //玩家id
    scenes.set_userscore(userScore);//玩家分数

    std::string data = scenes.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, qhb::SUB_S_SCENSES, (uint8_t *)data.c_str(), data.size());
}


//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool bClear = false;

    shared_ptr<CServerUserItem> pServerUserItem;
    pServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
    }else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;

        bool totalxz = false;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(userInfo->isBanker||userInfo->haveGrab)
        {
            totalxz=false;
        }
        else
        {
            totalxz=true;
        }    
        if(totalxz)
        {
            pServerUserItem->setOffline();
            m_UserInfo.erase(userId);
            m_pITableFrame->ClearTableUser(chairId);
            bClear = true;
        }else
        {
            pServerUserItem->SetTrustship(true);
            bClear = false;
        }
    }


    return bClear;
}


bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    return true;
}




//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason;
    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        GetOpenCards();
        uint8_t cardslist[2] = {0,0};


        //结算

        LOG(INFO) << "OnEventGameConclude chairId = " << chairId;
        return true;
    }
    return false;
}

void CTableFrameSink::clearTableUser()
{
    int64_t min_score = MIN_PLAYING_ANDROID_USER_SCORE;
    if(m_pGameRoomInfo)
        min_score = std::max(m_pGameRoomInfo->enterMinScore, min_score);


    vector<uint64_t> clearUserIdVec;
    vector<uint32_t> clearChairIdVec;
    {
        shared_ptr<CServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(pUserItem)
            {
                if(m_pGameRoomInfo->serverStatus !=SERVER_RUNNING|| ((pUserItem->GetUserStatus() == sOffline&&!userInfoItem->isBanker) ||
                   (pUserItem->IsAndroidUser() && pUserItem->GetUserScore() < m_SendEnvelopeScore)&&!userInfoItem->isBanker))
                {
                    pUserItem->setOffline();
                    m_pITableFrame->BroadcastUserStatus(pUserItem, true);
                    clearUserIdVec.push_back(userInfoItem->userId);
                    clearChairIdVec.push_back(userInfoItem->chairId);
                }
            }
        }
    }

    uint32_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {

        m_pITableFrame->ClearTableUser(clearChairIdVec[i]);
        m_UserInfo.erase(clearUserIdVec[i]);
    }
}

//保存玩家结算结果
void CTableFrameSink::WriteUserScore()
{
    vector<tagScoreInfo> scoreInfoVec;

    tagScoreInfo scoreInfo;
    int64_t userid = 0;
    m_replay.detailsData = "";
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(pUserItem)
        {

            scoreInfo.clear();
            if(userInfoItem->haveGrab&&!userInfoItem->isBanker&&userInfoItem->userId != m_nBankerUserId)
            {

               scoreInfo.chairId = userInfoItem->chairId;
               scoreInfo.betScore = 0;
               scoreInfo.rWinScore = 0;
               scoreInfo.addScore = userInfoItem->m_EndWinScore-userInfoItem->m_Revenue;
               scoreInfo.revenue = userInfoItem->m_Revenue;
               scoreInfo.cardValue = "赢";  //
               scoreInfo.startTime = m_startTime;
               scoreInfoVec.push_back(scoreInfo);
               //安全判断
               if(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue>m_SendEnvelopeScore
                       ||userInfoItem->m_EndWinScore-userInfoItem->m_Revenue<-m_SendEnvelopeScore
                       ||scoreInfo.addScore<-ERROESCORE||scoreInfo.addScore>ERROESCORE)
               {
                   LOG(ERROR)<<"A serious error has occurred  Exceeded all the evelope money!";
                   assert(false);
               }

            }
            if(userInfoItem->haveGrab&&userInfoItem->isBanker&&userInfoItem->userId != m_nBankerUserId)
            {
               scoreInfo.chairId = userInfoItem->chairId;
               scoreInfo.betScore = m_SendEnvelopeScore-userInfoItem->m_EndWinScore;
               scoreInfo.rWinScore = m_SendEnvelopeScore-userInfoItem->m_EndWinScore;
               scoreInfo.addScore = userInfoItem->m_EndWinScore-userInfoItem->m_Revenue-m_SendEnvelopeScore;
               scoreInfo.revenue = userInfoItem->m_Revenue;
               scoreInfo.cardValue = "";
               scoreInfo.startTime = m_startTime;
               scoreInfoVec.push_back(scoreInfo);
               //安全判断
               if(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue-m_SendEnvelopeScore <-m_SendEnvelopeScore
                       ||scoreInfo.addScore<-ERROESCORE||scoreInfo.addScore>ERROESCORE)
               {
                   LOG(ERROR)<<"A serious error has occurred  Exceeded all the evelope money!";
                   assert(false);
               }
            }
            //退还玩家发剩余的红包钱数
            if(userInfoItem->userId == m_nBankerUserId&&userInfoItem->haveGrab&&!userInfoItem->isBanker)
            {
                scoreInfo.chairId = userInfoItem->chairId;
                scoreInfo.betScore = 0;
                scoreInfo.rWinScore = 0;
                scoreInfo.addScore = userInfoItem->m_SendLeftScore+userInfoItem->m_EndWinScore-userInfoItem->m_Revenue;
                scoreInfo.revenue = userInfoItem->m_Revenue;
                scoreInfo.cardValue = "";
                scoreInfo.startTime = m_startTime;
                scoreInfoVec.push_back(scoreInfo);
                //安全判断
                if(scoreInfo.addScore <-m_SendEnvelopeScore
                        ||scoreInfo.addScore<-ERROESCORE||scoreInfo.addScore>ERROESCORE)
                {
                    LOG(ERROR)<<"A serious error has occurred  Exceeded all the evelope money!";
                    assert(false);
                }
                 openLog("WriteUserScore 玩家id： %d  上次发红包没抢完的钱:  %d    ",userInfoItem->userId,userInfoItem->m_SendLeftScore);
            }
            //退还玩家发剩余的红包钱数
            if(userInfoItem->userId == m_nBankerUserId&&userInfoItem->haveGrab&&userInfoItem->isBanker)
            {
                scoreInfo.chairId = userInfoItem->chairId;
                scoreInfo.betScore = m_SendEnvelopeScore-userInfoItem->m_EndWinScore;
                scoreInfo.rWinScore = m_SendEnvelopeScore-userInfoItem->m_EndWinScore;
                scoreInfo.addScore = userInfoItem->m_SendLeftScore+userInfoItem->m_EndWinScore-userInfoItem->m_Revenue-m_SendEnvelopeScore;
                scoreInfo.revenue = userInfoItem->m_Revenue;
                scoreInfo.cardValue = "";
                scoreInfo.startTime = m_startTime;
                scoreInfoVec.push_back(scoreInfo);
                //安全判断
                if(scoreInfo.addScore <-m_SendEnvelopeScore
                        ||scoreInfo.addScore<-ERROESCORE||scoreInfo.addScore>ERROESCORE)
                {
                    LOG(ERROR)<<"A serious error has occurred  Exceeded all the evelope money!";
                    assert(false);
                }
                 openLog("WriteUserScore 玩家id： %d  上次发红包没抢完的钱:  %d    ",userInfoItem->userId,userInfoItem->m_SendLeftScore);
            }
            if(userInfoItem->userId == m_nBankerUserId&&!userInfoItem->haveGrab&&!userInfoItem->isBanker&&userInfoItem->m_SendLeftScore>0)
            {
                scoreInfo.chairId = userInfoItem->chairId;
                scoreInfo.betScore = 0;
                scoreInfo.rWinScore = 0;
                scoreInfo.addScore = userInfoItem->m_SendLeftScore;
                scoreInfo.revenue = 0;
                scoreInfo.cardValue = "";
                scoreInfo.startTime = m_startTime;
                scoreInfoVec.push_back(scoreInfo);
                openLog("WriteUserScore 玩家id： %d  上次发红包没抢完的钱:  %d    ",userInfoItem->userId,userInfoItem->m_SendLeftScore);
                //安全判断
                if(scoreInfo.addScore <-m_SendEnvelopeScore
                        ||scoreInfo.addScore<-ERROESCORE||scoreInfo.addScore>ERROESCORE)
                {
                    LOG(ERROR)<<"A serious error has occurred  Exceeded all the evelope money!";
                    assert(false);
                }

            }
            //if(!pUserItem->IsAndroidUser())
            {
                //对局记录详情
                SetGameRecordDetail(userInfoItem->userId, userInfoItem->chairId, pUserItem->GetUserScore(), userInfoItem);
            }

        }
    }

    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_strRoundId);
    m_pITableFrame->SaveReplay(m_replay);
    //计算系统赢分
    tagStorageInfo storageInfo;
    int64_t res=0;
    int64_t allWinScore = 0;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        if(userInfoItem->isBanker)
        {
            if(!pUserItem->IsAndroidUser())
            {
                allWinScore+=m_SendEnvelopeScore;
            }
        }
        //退还
        if(!pUserItem->IsAndroidUser())
        {
            if(userInfoItem->userId == m_nBankerUserId)
            {
                allWinScore-=userInfoItem->m_SendLeftScore;
            }
        }
        if(pUserItem->IsAndroidUser()) continue;

        allWinScore-=userInfoItem->m_EndWinScore;
    }
    LOG(ERROR) << "this turn  System all win :  "<<allWinScore<<"     shuajinlv "<<(int64_t)stockWeak;
    openLog("本局写入库存值，衰减前:  %d    衰减率",allWinScore,(int64_t)stockWeak);
    res = allWinScore;
    if(res>0)
    {

        res=res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    LOG(ERROR) << "Write in Storage score is  :"<<res;
    openLog("本局写入库存值:  %d   ",res);
    m_pITableFrame->UpdateStorageScore(res);
    m_pITableFrame->GetStorageScore(storageInfo);
}

//改变玩家20注下注输赢信息
void CTableFrameSink::ChangeUserInfo()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    int64_t totalJetton = 0;

}


// 发送玩家列表
void CTableFrameSink::SendPlayerList(int32_t chairid)
{
    CMD_S_PlayerList playerList;
    //这里计算税收
    vector<shared_ptr<UserInfo>> vecUser;
    vecUser.clear();

    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        userInfoItem->m_UserScore = pUserItem->GetUserScore();
        vecUser.push_back(userInfoItem);
    }
    sort(vecUser.begin(),vecUser.end(),Compare);
    int32_t myRank = 0;
    int32_t rank = 0;
    for(auto &it :vecUser)
    {
        userInfoItem = it;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        rank++;
        if(userInfoItem->chairId == chairid)
        {
            myRank=rank;
        }
        Player * player= playerList.add_players();
        player->set_headid(userInfoItem->headerId);
        player->set_roundcount(userInfoItem->m_PlayCount);
        player->set_userid(userInfoItem->userId);
        player->set_userscore(pUserItem->GetUserScore());
        player->set_winscore(userInfoItem->m_UserAllWin);
        player->set_bestcount(userInfoItem->bestCount);
    }
    playerList.set_myranking(myRank);
    string data = playerList.SerializeAsString();
    m_pITableFrame->SendTableData(chairid, qhb::SUB_S_SEND_PLAYERLIST, (uint8_t *)data.c_str(), data.size());
}
void CTableFrameSink::SetAndroidGrab()
{
    //先设置十个机器人
    int32_t setNum = 0;
    float_t settime[15][2] = {{0.8f,4.0f},{0.8f,4.0f},{0.8f,4.0f},{0.8f,4.0f},{0.8f,4.0f},{0.8f,4.0f},{0.8f,4.0f},{4.0f,4.9f},{4.0f,4.9f},{4.0f,4.9f},{4.0f,4.9f},{4.0f,4.9f},{4.0f,4.9f},{4.0f,4.9f},{4.0f,4.9f}};
    shared_ptr<UserInfo> userInfoItem;
    float_t sendTime = 0.0;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        if(!pUserItem->IsAndroidUser()||userInfoItem->isBanker) continue;
        if(setNum>12) continue;
        userInfoItem->m_IsUseAndroid = true;
        sendTime=m_random.betweenFloat(settime[setNum][0],settime[setNum][1]).randFloat_mt(true);
        userInfoItem->m_AndroidGrabTime=sendTime;
        //LOG(ERROR)<<"setNum=="<<setNum<<"______________settime[setNum][0]____________________shijian ="<<settime[setNum][0];
        //LOG(ERROR)<<"setNum=="<<setNum<<"______________settime[setNum][1]____________________shijian ="<<settime[setNum][1];
        //LOG(ERROR)<<"_________________________________________shijian ="<<sendTime;
        CMD_S_Android_Grab android;
        android.set_grabtime(userInfoItem->m_AndroidGrabTime);

        string data = android.SerializeAsString();
        m_pITableFrame->SendTableData(userInfoItem->chairId, qhb::SUB_S_ANDROID_GRAB, (uint8_t *)data.c_str(), data.size());
        setNum++;
    }
}
//定时器事件 --通知开始抢红包
void CTableFrameSink::OnTimerReady()
{
    m_dwReadyTime=time(NULL);
    CMD_S_GameStart startGrab;
    m_endTime = chrono::system_clock::now();
    {

        m_TimerLoopThread->getLoop()->cancel(m_TimerReadyId);
        m_TimerStartId = m_TimerLoopThread->getLoop()->runAfter(m_nStartTime, boost::bind(&CTableFrameSink::OnTimerStart, this));
        //设置状态
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
        LOG_DEBUG <<"GameEnd UserCount:"<<m_UserInfo.size();
    }
    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<IServerUserItem> palyer = m_pITableFrame->GetTableUserItem(i);
        if(!palyer) continue;
        SendStart(i);

    }
    //通知机器人抢红包
    SetAndroidGrab();
    //刚发完红包数为最大值
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        userInfoItem->isBanker =false;
        userInfoItem->haveGrab = false;
        userInfoItem->clear();
    }
    m_nLeftEnvelopeNum = m_SendEnvelopeNum;

}
//结算
void CTableFrameSink::Settlement()
{
    //先随机产生十个红包
    m_VecEnvelop.clear();
    //红包是经过排序的,第一个是最佳手气
    GetResultArr(m_SendEnvelopeNum,m_VecEnvelop,m_SendEnvelopeScore,m_BestLuckyScore);
    for(int i=0;i<(int)m_VecEnvelop.size();i++)
    {
        openLog("算法产生得红包数值[%d]===%d",i,m_VecEnvelop[i]);
    }
    int  isHaveBlackPlayer[GAME_PLAYER]={0};
    int hasBlackPlayer = 0;

    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }

        if(!m_UserInfo[user->GetUserId()]->haveGrab) continue;
        if(user->GetBlacklistInfo().listRoom.size()>0)
        {
            auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
            if(it != user->GetBlacklistInfo().listRoom.end())
            {
                if(it->second>0)
                {
                    isHaveBlackPlayer[i]=it->second;//
                    //证明是有黑名单
                    if(it->second>hasBlackPlayer)
                    {
                        hasBlackPlayer = it->second;
                    }

                }

                openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
            }
        }


    }
    //证明是有黑名单
    if(hasBlackPlayer)
    {   int allWeight = 0;

        for(int i=0;i<GAME_PLAYER;i++)
        {
            if(isHaveBlackPlayer[i]>0)
            {
                 allWeight+=isHaveBlackPlayer[i];
            }
        }
        float blackHei=hasBlackPlayer/1000.0f;
        float probability= m_random.betweenFloat(0.0f,1.0f).randFloat_mt(true);
        //命中黑名单目标 ,对不起了兄弟，最佳手气就在黑名单中抽取
        int32_t balckChairId = INVALID_CHAIR;
        if(probability<=blackHei)
        {
            int32_t probabilityWeight = m_random.betweenInt(0,allWeight).randInt_mt(true);
            int32_t currentWeight = 0;
            for(int i=0;i<GAME_PLAYER;i++)
            {
                if(isHaveBlackPlayer[i]>0)
                {
                     currentWeight+=isHaveBlackPlayer[i];
                     if(currentWeight>=probabilityWeight)
                     {
                        balckChairId = i;
                        break;
                     }
                }
            }
            if(balckChairId!=INVALID_CHAIR)
            {
                std::vector<vecHongBao>  temlist;
                temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                shared_ptr<UserInfo> userInfoItem;
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                    if(userInfoItem->chairId==balckChairId)
                    {
                        userInfoItem->m_EndWinScore = temlist[0].money;
                        userInfoItem->isBanker = temlist[0].isBest;
                        temlist.erase(temlist.begin()+0);
                        openLog("黑名单控制命中概率 被杀玩家userid: %d   winscore:  %d",userInfoItem->userId,userInfoItem->m_EndWinScore);
                        break;
                    }

                }
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                    if(userInfoItem->chairId!=balckChairId)
                    {
                        int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                        userInfoItem->m_EndWinScore = temlist[index].money;
                        temlist.erase(temlist.begin()+index);
                    }
                    openLog("黑名单控制命中概率 userid: %d   winscore:  %d",userInfoItem->userId,userInfoItem->m_EndWinScore);
                }

            }
            else
            {
                shared_ptr<UserInfo> userInfoItem;
                std::vector<vecHongBao>  temlist;
                temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                int64_t money=0;
                int64_t bestid= 0;
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                    int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                    if(temlist[index].money>money)
                    {
                        money=temlist[index].money;
                        bestid = userInfoItem->userId;
                    }
                    userInfoItem->m_EndWinScore = temlist[index].money;
                    temlist.erase(temlist.begin()+index);
                }
                if(bestid!=0)
                {
                   m_UserInfo[bestid]->isBanker=true;
                }
            }
        }
        else
        {
            shared_ptr<UserInfo> userInfoItem;
            std::vector<vecHongBao>  temlist;
            temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
            int64_t money=0;
            int64_t bestid= 0;
            for(auto &it :m_UserInfo)
            {
                userInfoItem = it.second;
                shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem) continue;
                if(!userInfoItem->haveGrab) continue;
                int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                if(temlist[index].money>money)
                {
                    money=temlist[index].money;
                    bestid = userInfoItem->userId;
                }
                userInfoItem->m_EndWinScore = temlist[index].money;
                temlist.erase(temlist.begin()+index);

                openLog("黑名单控制没命中概率 userid: %d   winscore:  %d",userInfoItem->userId,userInfoItem->m_EndWinScore);
            }
            if(bestid!=0)
            {
               m_UserInfo[bestid]->isBanker=true;
            }

        }

    }
    //库存控制
    // 没有真人随机
    if(true)
    {
        shared_ptr<UserInfo> userInfoItem;
        int32_t realPlayer = 0;
        int32_t allPalyer = 0;
        for(auto &it :m_UserInfo)
        {
            userInfoItem = it.second;
            shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem) continue;
            if(!userInfoItem->haveGrab)
            {
                allPalyer++;
                continue;
            }
            allPalyer++;
            if(pUserItem->IsAndroidUser())continue;
             realPlayer++;
             allPalyer++;
        }
        //全是真人或者全是机器人,只有黑名单起作用，其它全部随机
        if(realPlayer==0||realPlayer==allPalyer)
        {

            openLog("抢红包全是真人或者全是机器人----真人数量是:%d===所有人数量=====%d",realPlayer,allPalyer);
            if(hasBlackPlayer)
            {
                if(realPlayer==0)
                {
                    LOG(WARNING)<<"All user is Android!!!!";
                }
                if(realPlayer==allPalyer)
                {
                    LOG(WARNING)<<"All user is realplayer!!!!";
                }
                return;
            }
            else
            {
                shared_ptr<UserInfo> userInfoItem;
                std::vector<vecHongBao>  temlist;
                temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                int64_t money=0;
                int64_t bestid= 0;
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                    int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                    if(temlist[index].money>money)
                    {
                        money=temlist[index].money;
                        bestid = userInfoItem->userId;
                    }
                    userInfoItem->m_EndWinScore = temlist[index].money;                    
                    temlist.erase(temlist.begin()+index);
                    openLog("全是真人或者全是机器人情况下随机出结果 userid:  %d   winscore:  %d",userInfoItem->userId,userInfoItem->m_EndWinScore);
                }
                if(bestid!=0)
                {
                   m_UserInfo[bestid]->isBanker=true;
                }

                return;
            }
        }
    }
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);


    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        //必杀调用
        int64_t bestchairid= 0;
        int64_t bestid= 0;
        do
        {
            bestchairid= 0;
            bestid = 0;
            shared_ptr<UserInfo> userInfoItem;
            std::vector<vecHongBao>  temlist;
            temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
            int64_t money=0;

            for(auto &it :m_UserInfo)
            {
                userInfoItem = it.second;
                shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem) continue;
                if(!userInfoItem->haveGrab) continue;
                int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                userInfoItem->m_EndWinScore = temlist[index].money;
                if(temlist[index].money>money)
                {
                    money=temlist[index].money;
                    bestid = userInfoItem->userId;
                    bestchairid = userInfoItem->chairId;
                }
                temlist.erase(temlist.begin()+index);
            }

        }while(m_pITableFrame->IsAndroidUser(bestchairid));
        if(bestid!=0)
        {
           m_UserInfo[bestid]->isBanker=true;
        }
        openLog("低于库存下限，而且命中杀分概率，控制概率是: %f",m_dControlkill);
    }
    else
    {
        openLog("当前库存值[%d]",storageInfo.lEndStorage);
        openLog("库存上限值[%d]",storageInfo.lUplimit);
        openLog("库存下限值[%d]",storageInfo.lLowlimit);
        //超出库存上限
        if(storageInfo.lEndStorage>= storageInfo.lUplimit)
        {

            float probility = m_random.betweenFloat(0.0f,1.0f).randFloat_mt(true);
            //库存概率命中 ,在原来命中概率基础上增加命中
            if(0.9f+m_dControlkill*0.1f>=probility)
            {

                int64_t bestchairid= 0;
                int64_t bestid= 0;
                do
                {
                    shared_ptr<UserInfo> userInfoItem;
                    bestchairid= 0;
                    bestid = 0;
                    std::vector<vecHongBao>  temlist;
                    temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                    int64_t money=0;

                    for(auto &it :m_UserInfo)
                    {
                        userInfoItem = it.second;
                        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                        if(!pUserItem) continue;
                        if(!userInfoItem->haveGrab) continue;
                        int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                        userInfoItem->m_EndWinScore = temlist[index].money;
                        if(temlist[index].money>money)
                        {
                            money=temlist[index].money;
                            bestid = userInfoItem->userId;
                            bestchairid = userInfoItem->chairId;
                        }
                        temlist.erase(temlist.begin()+index);
                    }

                }while(!m_pITableFrame->IsAndroidUser(bestchairid));
                if(bestid!=0)
                {
                   m_UserInfo[bestid]->isBanker=true;
                }

                shared_ptr<UserInfo> userInfoItem;
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                     openLog("userid :%d  win: %d     当前库存超出库存上限 库存控制概率是:  %f    "
                             "已经命中控制概率  产生得最佳手气应该是机器人: %d",
                             userInfoItem->userId,userInfoItem->m_EndWinScore,m_dControlkill,m_pITableFrame->GetTableUserItem(bestchairid)->GetUserId());
                }

            }
            //不命中依然随机
            else
            {
                shared_ptr<UserInfo> userInfoItem;
                std::vector<vecHongBao>  temlist;
                temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                int64_t money=0;
                int64_t bestid= 0;
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                    int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                    userInfoItem->m_EndWinScore = temlist[index].money;
                    if(temlist[index].money>money)
                    {
                        money=temlist[index].money;
                        bestid = userInfoItem->userId;
                    }

                    temlist.erase(temlist.begin()+index);
                    openLog("当前库存超出库存上限 但是没有命中概率，所以随机出结果,抢红包得玩家ID：%d    抢到的红包：   %d",userInfoItem->userId,userInfoItem->m_EndWinScore);
                }
                if(bestid!=0)
                {
                   m_UserInfo[bestid]->isBanker=true;
                }
            }
        }
        //低于库存下限
        else if(storageInfo.lEndStorage<= storageInfo.lLowlimit)
        {
            float probility = m_random.betweenFloat(0.0f,1.0f).randFloat_mt(true);
            //库存概率命中
            if(m_dControlkill>=probility)
            {
                int64_t bestchairid= 0;
                int64_t bestid= 0;
                do
                {
                    bestchairid= 0;
                    bestid = 0;
                    shared_ptr<UserInfo> userInfoItem;
                    std::vector<vecHongBao>  temlist;
                    temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                    int64_t money=0;

                    for(auto &it :m_UserInfo)
                    {
                        userInfoItem = it.second;
                        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                        if(!pUserItem) continue;
                        if(!userInfoItem->haveGrab) continue;
                        int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                        userInfoItem->m_EndWinScore = temlist[index].money;
                        if(temlist[index].money>money)
                        {
                            money=temlist[index].money;
                            bestid = userInfoItem->userId;
                            bestchairid = userInfoItem->chairId;
                        }
                        temlist.erase(temlist.begin()+index);
                    }

                }while(m_pITableFrame->IsAndroidUser(bestchairid));
                if(bestid!=0)
                {
                   m_UserInfo[bestid]->isBanker=true;
                }
                openLog("低于库存下限，而且命中杀分概率，控制概率是: %f",m_dControlkill);
            }
            //不命中依然随机
            else
            {
                if(hasBlackPlayer)
                {
                    openLog("低于库存下限没有命中概率，但是有黑单，所以照样按照黑名单控制流程");
                }
                else
                {
                    shared_ptr<UserInfo> userInfoItem;
                    std::vector<vecHongBao>  temlist;
                    temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                    int64_t money=0;
                    int64_t bestid= 0;
                    for(auto &it :m_UserInfo)
                    {
                        userInfoItem = it.second;
                        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                        if(!pUserItem) continue;
                        if(!userInfoItem->haveGrab) continue;
                        int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                        userInfoItem->m_EndWinScore = temlist[index].money;
                        if(temlist[index].money>money)
                        {
                            money=temlist[index].money;
                            bestid = userInfoItem->userId;
                        }
                        temlist.erase(temlist.begin()+index);
                    }
                    if(bestid!=0)
                    {
                       m_UserInfo[bestid]->isBanker=true;
                    }
                    openLog("低于库存下限没有命中概率，也没有黑名单，所以随机出结果,庄家抢红包得玩家ID：%d    庄家抢到的红包：    %d", m_UserInfo[bestid]->userId, m_UserInfo[bestid]->m_EndWinScore);
                }

            }
        }
        //正常范围
        else
        {
            //正常范围黑名单照杀
            if(hasBlackPlayer)
            {
                openLog("正常库存，有黑名单，所以走黑名单流程，直接返回");
            }
            else
            {
                openLog("正常库存，没有黑名单，随机出结果");
                shared_ptr<UserInfo> userInfoItem;
                std::vector<vecHongBao>  temlist;

                temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
                int64_t money=0;
                int64_t bestid= 0;
                for(auto &it :m_UserInfo)
                {
                    userInfoItem = it.second;
                    shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                    if(!pUserItem) continue;
                    if(!userInfoItem->haveGrab) continue;
                    int32_t index = m_random.betweenInt(0,(int)temlist.size()-1).randInt_mt(true);
                    userInfoItem->m_EndWinScore = temlist[index].money;
                    if(temlist[index].money>money)
                    {
                        money=temlist[index].money;
                        bestid = userInfoItem->userId;
                    }
                    temlist.erase(temlist.begin()+index);
                    openLog("正常库存，没有黑名单，随机出结果,抢红包得玩家ID：%d    抢到的红包：  %d",userInfoItem->userId,userInfoItem->m_EndWinScore);
                }
                if(bestid!=0)
                {
                   m_UserInfo[bestid]->isBanker=true;
                }
            }
        }

    }

    //万一抢的人数不够，没有抢到最大的红包,也就没有最佳手气,设置抢到最大金额的玩家为最佳手气
    shared_ptr<UserInfo> userInfoItem;
    std::vector<vecHongBao>  temlist;
    temlist.assign(m_VecEnvelop.begin(), m_VecEnvelop.end());
    bool hasBanker =false;
    int32_t  moreBanker = 0;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        if(!userInfoItem->haveGrab) continue;
        if(userInfoItem->isBanker)
        {
            hasBanker=true;
            moreBanker++;
        }
    }
    if(!hasBanker||moreBanker>1)
    {
        int64_t bigOneUserid = 0;
        int64_t getMoney = 0;
        for(auto &it :m_UserInfo)
        {
            userInfoItem = it.second;
            shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem) continue;
            if(!userInfoItem->haveGrab) continue;
            if(getMoney<=userInfoItem->m_EndWinScore)
            {
                getMoney = userInfoItem->m_EndWinScore;
                bigOneUserid = userInfoItem->userId;
            }
            userInfoItem->isBanker =false;
        }
        m_UserInfo[bigOneUserid]->isBanker =true;
        openLog("找不到庄家，所以设置庄家[%d],产生庄家数[%d]",m_UserInfo[bigOneUserid]->userId,moreBanker);
    }
//    for(auto &it :m_UserInfo)
//    {
//        userInfoItem = it.second;
//        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
//        if(!pUserItem) continue;
//        if(!userInfoItem->haveGrab) continue;
//        userInfoItem->m_Revenue = userInfoItem->m_EndWinScore*0.05;

//        //LOG(WARNING)<<"grab red evelope userid:  "<<userInfoItem->userId <<" grab money is "<<userInfoItem->m_EndWinScore <<"revent is : "<<userInfoItem->m_Revenue;
//    }


}


//实际是抢红包结束 ,结算并且发送结束状态
void CTableFrameSink::OnTimerStart()
{
    //-------在此结算-------//并产生最佳玩家，即是下一个庄家
    m_pITableFrame->SetGameStatus(GAME_STATUS_END);

    m_TimerLoopThread->getLoop()->cancel(m_TimerStartId);
    m_TimerEndId = m_TimerLoopThread->getLoop()->runAfter(m_nGameEndTime, boost::bind(&CTableFrameSink::OnTimerEnd, this));
    //抢完红包就把上局的庄家清掉了 m_nBankerUserId 这个还记录着,退还剩余钱用
    shared_ptr<UserInfo> userInfoItem;

    Settlement();
    //这里计算发剩余的红包钱数
    int64_t allGrapMoney = 0;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        if(!userInfoItem->haveGrab)
        {
            continue;
        }

        allGrapMoney  +=  userInfoItem->m_EndWinScore;
    }
    if(m_SendEnvelopeScore - allGrapMoney>=0)
    {
        m_BankerLeftSocre = m_SendEnvelopeScore - allGrapMoney;
    }
    else
    {
        LOG(ERROR)<<"A serious error has occurred  Exceeded all the money!";

        m_BankerLeftSocre = 0;
    }
    openLog("所有人抢走的钱数 :  %d 庄家剩下的钱 :   %d",allGrapMoney,m_BankerLeftSocre);
    //这里计算税收
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem) continue;
        if(userInfoItem->isBanker)
        {
            userInfoItem->bestCount++;
            LOG(WARNING)<<"Banker   userid  is:  "<<userInfoItem->userId;
            openLog("庄家id： %d  税收:  %d    抢红包次数 : %d",userInfoItem->userId,userInfoItem->m_Revenue,userInfoItem->m_PlayCount);
        }
        if(userInfoItem->userId==m_nBankerUserId&&m_BankerLeftSocre>0)
        {
            //退还余钱
            userInfoItem->m_SendLeftScore = m_BankerLeftSocre;
            m_replay.addStep(5,to_string(m_BankerLeftSocre),-1,opLkOrCall,userInfoItem->chairId,0);
        }

        if(!userInfoItem->haveGrab)
        {
            userInfoItem->NotBetJettonCount+=1;
            continue;
        }


        userInfoItem->NotBetJettonCount = 0;
        userInfoItem->m_PlayCount+=1;
        userInfoItem->m_Revenue = userInfoItem->m_EndWinScore*5/100;
        userInfoItem->m_UserAllWin+=(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue);
        //if(!pUserItem->IsAndroidUser())
        {

            if(userInfoItem->isBanker)
            {
                m_replay.addResult(userInfoItem->chairId,0,userInfoItem->m_EndWinScore-userInfoItem->m_Revenue-m_SendEnvelopeScore,(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue-m_SendEnvelopeScore),"",userInfoItem->isBanker);
                m_replay.addStep(5,to_string(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue),-1,opCmprOrLook,userInfoItem->chairId,0);
            }
            else
            {
                m_replay.addStep(5,to_string(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue),-1,opCmprOrLook,userInfoItem->chairId,0);
                m_replay.addResult(userInfoItem->chairId,0,0,(userInfoItem->m_EndWinScore-userInfoItem->m_Revenue),"",userInfoItem->isBanker);
            }
        }
        LOG(WARNING)<<"grab red evelope userid:  "<<userInfoItem->userId <<" grab money is "<<userInfoItem->m_EndWinScore <<"        revent is : "<<userInfoItem->m_Revenue;
        openLog("玩家id： %d  税收:  %d    抢红包次数 : %d    抢到的红包: %d",userInfoItem->userId,userInfoItem->m_Revenue,userInfoItem->m_PlayCount,userInfoItem->m_EndWinScore);
    }
    LOG(WARNING)<<"WriteUserScore() ";
    WriteUserScore();
    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<IServerUserItem> palyer = m_pITableFrame->GetTableUserItem(i);
        if(!palyer) continue;
        SendEnd(i);

    }

     m_replay.clear();
    //五局不操作踢出玩家
    CheckKickOutUser();
    //清理离线玩家
    clearTableUser();
}
//结算结束就是开始准备发红包了,通知玩家发红包，准备抢红包
void CTableFrameSink::OnTimerEnd()
{
    //

    m_strRoundId = m_pITableFrame->GetNewRoundId();
    m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
    m_startTime = chrono::system_clock::now();
    m_replay.gameinfoid = m_strRoundId;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata	
    {

        ClearTableData();        
        ReadConfigInformation();//重新读取配置
        //结束游戏

         LOG_DEBUG <<"GameStart UserCount:"<<m_UserInfo.size();
         shared_ptr<IServerUserItem> pUserItem;
         shared_ptr<UserInfo> userInfoItem;
         for(auto &it : m_UserInfo)
         {
             userInfoItem = it.second;
             pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
             if(!pUserItem)
                 continue;
             SendReady(userInfoItem->chairId);
         }        
        m_TimerReadyId = m_TimerLoopThread->getLoop()->runAfter(m_nReadyTime, boost::bind(&CTableFrameSink::OnTimerReady, this));

    }
}

void CTableFrameSink::ClearTableData()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;

        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if(userInfoItem->isBanker)
        {
            m_nBankerUserId = userInfoItem->userId;
        }
        userInfoItem->clear();
    }
    m_BankerLeftSocre = 0;
}

bool  CTableFrameSink::OnMakeDir()
{
	std::string dir="./Log";  
    if(access(dir.c_str(), 0) == -1)
    {  
        if(mkdir(dir.c_str(), 0755)== 0)
		{ 
            dir="./Log/qhb";

            if(access(dir.c_str(), 0) == -1)
			{  
                return (mkdir(dir.c_str(), 0755)== 0);
			}  
		}
    }  	

	return false;
}

bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{ 
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if (!pIServerUserItem)
    {
        LOG(INFO) << "OnGameMessage pServerOnLine is NULL  wSubCmdID:" << wSubCmdID;
		return false;
    }

	switch (wSubCmdID)
    {
        //有玩家抢红包
        case qhb::SUB_C_GRAB_ENVELOPE:
        {
             if(m_pITableFrame->GetGameStatus()!=GAME_STATUS_START)
             {
                 SendErrorCode(wChairID, ERROR_WRONG_STATUS);
                 LOG(ERROR) << "status  is not  GAME_STATUS_START:" << m_pITableFrame->GetGameStatus();
                 return false;
             }
             shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
             if(!pIServerUserItem)
             {
                 LOG_ERROR << "pIServerUserItem==NULL chairId:"<<wChairID;
                 return false;
             }
             if(pIServerUserItem->GetUserScore()<m_SendEnvelopeScore)
             {
                 SendErrorCode(wChairID, ERROR_NOT_ENOUGH_SOCRE);
                 LOG(ERROR) << "User score is not enough:" << pIServerUserItem->GetUserScore()<<"<<<<<<<"<<m_SendEnvelopeScore;
                 return false;
             }
             int64_t userid = pIServerUserItem->GetUserId();
             auto it = m_UserInfo.find(userid);
             if(m_UserInfo.end() == it)
             {
                 return false;
             }
             if(m_UserInfo[userid]->haveGrab)
             {
                 return false;
             }
             if(m_nLeftEnvelopeNum<=0)
             {
                 return false;
             }

             m_UserInfo[userid]->haveGrab = true;
             //if(!pIServerUserItem->IsAndroidUser())
             {
                 m_replay.addPlayer(userid,pIServerUserItem->GetAccount(),pIServerUserItem->GetUserScore(),wChairID);
                 m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,"",-1,opBet,wChairID,0);
                 LOG_ERROR << "m_replay.addPlayer chairId:"<<wChairID;
             }

             m_nLeftEnvelopeNum--;

             for(int i=0;i<GAME_PLAYER;i++)
             {
                 AnswerPlayersGrab(i,pIServerUserItem);
             }
             return true;
             break;
        }
        case qhb::SUB_C_SEND_ENVELOPE:
        {
             //状态不对不能抢红包
             if(m_pITableFrame->GetGameStatus()!=GAME_STATUS_FREE)
             {
                 SendErrorCode(wChairID, ERROR_WRONG_STATUS);
                 LOG(ERROR) << "status  is not  GAME_STATUS_FREE:" << m_pITableFrame->GetGameStatus();
                 return false;
             }
             //不是庄家不能发红包
             if(pIServerUserItem->GetUserId()!=m_nBankerUserId)
             {
                 SendErrorCode(wChairID, ERROR_NOT_BANKER);
                 LOG(ERROR) << "this user is not banker,banker is:" << m_nBankerUserId;
                 return false;
             }
             CMD_C_Grab qiangHongbao;
             qiangHongbao.ParseFromArray(pData, wDataSize);

             m_TimerLoopThread->getLoop()->cancel(m_TimerReadyId);
             OnTimerReady();
              return true;
             break;
        }
        case qhb::SUB_C_ASKFOR_PLAYERLIST:
        {
            shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
            if(!pIServerUserItem)
            {
                LOG_ERROR << "pIServerUserItem==NULL chairId:"<<wChairID;
                return false;
            }
            SendPlayerList(wChairID);
            break;
        }
        return true;
    }
}
//跑马灯消息
void CTableFrameSink::SendWinSysMessage()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if(userInfoItem->m_EndWinScore >= m_pITableFrame->GetGameRoomInfo()->broadcastScore)
        {
           m_pITableFrame->SendGameMessage(userInfoItem->chairId, "", SMT_GLOBAL|SMT_SCROLL, userInfoItem->m_EndWinScore);
        }
    }
}

// try to create the special table frame sink item now.
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

void  CTableFrameSink::openLog(const char *p, ...)
{
    if (m_IsEnableOpenlog)
    {

        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/qhb/qhb_table%d_%d%d%d.log",m_pITableFrame->GetTableId(),t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
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

void CTableFrameSink::CheckKickOutUser()
{
    vector<int> clearUserIdVec;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo)
    {
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if (userInfoItem->NotBetJettonCount>=5&&!userInfoItem->isBanker)
        {

        }
        if (pUserItem->IsAndroidUser()&&userInfoItem->NotBetJettonCount>=5&&!userInfoItem->isBanker)
        {
            pUserItem->SetUserStatus(sOffline);
            clearUserIdVec.push_back(userInfoItem->userId);
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);
            m_pITableFrame->ClearTableUser(userInfoItem->chairId,true,true);
            //LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<userInfoItem->chairId<<" userId="<<userInfoItem->userId;

        }
//        if (!pUserItem->IsAndroidUser()&&userInfoItem->NotBetJettonCount>=5&&!userInfoItem->isBanker)
//        {
//            pUserItem->SetUserStatus(sGetoutAtplaying);
//            clearUserIdVec.push_back(userInfoItem->userId);
//            m_pITableFrame->BroadcastUserStatus(pUserItem, true);
//            m_pITableFrame->ClearTableUser(userInfoItem->chairId,true,true);
//            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<userInfoItem->chairId<<" userId="<<userInfoItem->userId;
//        }
    }
    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_UserInfo.erase(clearUserIdVec[i]);
    }
}

//设置当局游戏详情
void CTableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo)
{
    //记录当局游戏详情json
    //boost::property_tree::ptree root, items, player[5];
    //记录当局游戏详情binary
    string strDetail = "";
    qhb::CMD_S_GameRecordDetail details;
    details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
    details.set_userscore(userScore);			//携带积分
    details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分

    if(userInfo->isBanker)
    {
       details.set_writetype("输");
    }
    else if(!userInfo->isBanker)
    {
        details.set_writetype("赢");
    }
    if (!m_replay.saveAsStream) {

    }
    else {
        strDetail = details.SerializeAsString();
    }
    m_replay.addDetail(userid, chairId, strDetail);
}


