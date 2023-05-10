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

#include "ServerUserItem.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

#include "proto/sg.Message.pb.h"

#include "../Game_SanGongLandy/CMD_Game.h"
#include "../Game_SanGongLandy/GameLogic.h"

using namespace sg;
#include "AndroidUserItemSink.h"

#define ID_TIME_CALL_BANKER				100					//叫庄定时器(3S)
#define ID_TIME_ADD_SCORE				101					//下注定时器(3S)
#define ID_TIME_OPEN_CARD				102					//开牌定时器(5S)

//定时器时间
#define TIME_BASE						1000				//基础定时器
//#define TIME_CALL_BANKER				5					//叫庄定时器(3S)
//#define TIME_ADD_SCORE					7					//下注定时器(3S)
//#define TIME_OPEN_CARD					7					//开牌定时器(5S)


//uint8_t	CAndroidUserItemSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 5,10,15,20 };	//下注倍数
//uint8_t	CAndroidUserItemSink::m_cbBankerMultiple[MAX_BANKER_MUL] = { 1,2,4 };		//叫庄倍数

CAndroidUserItemSink::CAndroidUserItemSink()
    : m_pTableFrame(NULL)
    , m_wChairID(INVALID_CHAIR)
    , m_pAndroidUserItem(NULL)
{
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_wCallBankerNum = 0;
    m_wAddScoreNum = 0;
    m_wOpenCardNum = 0;
    m_wPlayerCount = 0;
    randomRound=0;
    CountrandomRound=0;
    srand(time(NULL));
    ClearAndroidCount=0;
    m_AndroidPlayerCount=RandomInt(10,20);
    m_icishu=0;
//    memset(m_qzbeishu, 0, sizeof(m_qzbeishu));
//    memset(m_jiabeibeishu, 0, sizeof(m_jiabeibeishu));
    m_m_cbPlayStatus = false;
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{

}

bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
    //LOG(INFO) << "CAndroidUserItemSink::Initialization pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
    // //openLog("Initialization。。。");
    bool bSuccess = false;
    //  check the table and user item now.
    if ((!pTableFrame) || (!pUserItem)) {
        //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
        return false;
    }

    //查询接口
    // m_pIAndroidUserItem=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,IAndroidUserItem);
    // if (m_pIAndroidUserItem==NULL) return false;
    m_pTableFrame   =  pTableFrame;
    m_pAndroidUserItem = pUserItem;

    // try to update the specdial room config data item.
    if(m_pTableFrame)
    {
        m_pGameRoomInfo = m_pTableFrame->GetGameRoomInfo();
        m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
    }

    // check if user is android user item.
    if (!pUserItem->IsAndroidUser()) {
        // LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserID();
    }
    srand((unsigned)time(NULL));
    ReadConfigInformation();
    randomRound=RandomInt(10,20);
    CountrandomRound=0;
    m_wChairID = pUserItem->GetChairId();
    bSuccess = true;

    return (bSuccess);
}

bool CAndroidUserItemSink::RepositionSink()
{
    m_pTableFrame = NULL;
    m_wChairID = INVALID_CHAIR;
//    m_pAndroidUserItem = NULL;
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_wCallBankerNum = 0;
    m_wAddScoreNum = 0;
    m_wOpenCardNum = 0;
    m_wPlayerCount = 0;
    return true;
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pUserItem)
{
//    if(pUserItem)
//    {
//        m_pAndroidUserItem = pUserItem;
//    }

//    bool bSuccess = false;
    //  check the table and user item now.
    if ( (!pUserItem)) {
       return false;
    }

    //查询接口

    m_pAndroidUserItem = pUserItem;

    srand((unsigned)time(NULL));
    ReadConfigInformation();
    randomRound=RandomInt(10,20);
    CountrandomRound=0;
    m_wChairID = pUserItem->GetChairId();
    return true;
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
//    if(pTableFrame)
//    {
//        m_pTableFrame   =  pTableFrame;
//        m_pGameRoomInfo  =  m_pTableFrame->GetGameRoomInfo();
//        if(m_pGameRoomInfo)
//        {
//            m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
//        }
//        ReadConfigInformation();
//    }

    //  check the table and user item now.
    if (!pTableFrame) {
          return ;
    }

    //查询接口
    m_pTableFrame   =  pTableFrame;
    // try to update the specdial room config data item.
    if(m_pTableFrame)
    {
        m_pGameRoomInfo = m_pTableFrame->GetGameRoomInfo();
        m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
    }

}
//void CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, uint32_t wChairID, shared_ptr<IServerUserItem>& pUserItem)
//{
//    m_pTableFrame = pTableFrame;

//    m_pAndroidUserItem = pUserItem;
////    m_wChairID = wChairID;

//}
void CAndroidUserItemSink::GameTimerCallBanker()
{
//    m_TimerCallBanker.stop();
    m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerCallBanker);
    uint8_t cbCallBankerMul = 0;
    cbCallBankerMul = rand()%2; //随机叫庄

//    uint32_t chairId = m_pAndroidUserItem->GetChairId();
    openLog("111111 i=%d cbCallBankerMul=%d",m_wChairID,(int)cbCallBankerMul);

    SG_CMD_C_CallBanker callBanker;
    callBanker.set_cbcallflag(cbCallBankerMul);

    //发送信息
    string content = callBanker.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_wChairID,SG_SUB_C_CALL_BANKER, (uint8_t*)content.data(), content.size());
    LOG(INFO)<<"******************************Send SG_SUB_C_CALL_BANKER";
    LOG(INFO)<<"******************************cbCallBankerMul"<<(int)cbCallBankerMul;
}
void CAndroidUserItemSink::GameTimeAddScore()
{
//    m_TimerAddScore.stop();
    m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerAddScore);
    SG_CMD_C_AddScore addScore;

    uint8_t cbJettonIndex = 0;
    uint64_t cbJetton = 0;
    //随机下注
    int iIndex = 0;
    for(int i = 0; i < 5; ++i)
    {
        if(m_cbMaxJettonMultiple[i] == 0) break;
        iIndex++;
    }
    assert(iIndex != 0);
    cbJetton = m_cbMaxJettonMultiple[rand()%iIndex];
    addScore.set_cbjetton(cbJetton);
    openLog("111111 i=%d cbJetton=%d",m_wChairID,(int)cbJetton);

    string content = addScore.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_wChairID,SG_SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
    LOG(INFO)<<"******************************Send SG_SUB_C_ADD_SCORE";
    LOG(INFO)<<"******************************cbJettonIndex"<<(int)cbJettonIndex;
}

void CAndroidUserItemSink::GameTimeOpencard()
{
    //m_pAndroidUserItem->KillTimer(ID_TIME_OPEN_CARD);
//    m_TimerOpenCard.stop();
    m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerOpenCard);
    m_pTableFrame->OnGameEvent(m_wChairID, SG_SUB_C_OPEN_CARD, NULL, 0);
}

bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    switch (wSubCmdID)
    {
    case SG_SUB_S_GAME_START_AI:
    {
        sg::CMD_S_GameStartAi GameStartAi;
        if (!GameStartAi.ParseFromArray(pData,wDataSize))
            return false;
        m_cbSGCardData = GameStartAi.cbsgcarddata();
        m_isMaxCard = GameStartAi.ismaxcard();
        openLog("SG_SUB_S_GAME_START_AI m_cbSGCardData=%d m_isMaxCard=%d",m_cbSGCardData,m_isMaxCard);
        m_m_cbPlayStatus = true;
        return true;
    }
    case SG_SUB_S_GAME_START:
    {
        ClearAndroidCount=0;
        m_m_cbPlayStatus = true;
        return OnSubGameStart(wSubCmdID,pData, wDataSize);
    }
        break;
    case SG_SUB_S_CALL_BANKER:
    {
        return OnSubCallBanker(wSubCmdID,pData, wDataSize);
    }
        break;
    case SG_SUB_S_CALL_BANKER_RESULT:
    {
        return OnSubCallBankerResult(wSubCmdID,pData, wDataSize);
    }
        break;
    case SG_SUB_S_ADD_SCORE_RESULT:
    {
        return OnSubAddScore(wSubCmdID,pData, wDataSize);
    }
        break;
    case SG_SUB_S_SEND_CARD:
    {
        return OnSubSendCard(wSubCmdID,pData, wDataSize);
    }
    case SG_SUB_S_OPEN_CARD_RESULT:
    {
        return OnSubOpenCard(wSubCmdID,pData, wDataSize);
    }
        break;
    case SG_SUB_S_GAME_END:
    {
        return ClearAllTimer();
    }
        break;
    default: break;
    }
    return true;
}

bool CAndroidUserItemSink::OnSubGameStart(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    SG_CMD_S_GameStart GameStart;
    if (!GameStart.ParseFromArray(pData,wDataSize))
        return false;
    m_icishu++;
    CountrandomRound++;
    m_wPlayerCount = GameStart.cbplaystatus().size();

//    if(m_pAndroidUserItem != nullptr)
        m_wChairID = m_pAndroidUserItem->GetChairId();
    int dwShowTime = 500;//发牌动画  1.5
    int callTime = GameStart.cbcallbankertime();
    int dwCallBankerTime = dwShowTime + rand() % ((callTime -1)*TIME_BASE);
    //m_pAndroidUserItem->SetTimer(ID_TIME_CALL_BANKER, dwCallBankerTime);

    double dValue = (double)dwCallBankerTime/1000;
//    m_TimerCallBanker.start(dwCallBankerTime, bind(&CAndroidUserItemSink::GameTimerCallBanker, this, std::placeholders::_1), NULL, 1, false);
    m_TimerCallBanker = m_pTableFrame->GetLoopThread()->getLoop()->runAfter(dValue, boost::bind(&CAndroidUserItemSink::GameTimerCallBanker, this));
    LOG(INFO)<<"dwCallBankerTime"<<"="<<dwCallBankerTime;

    //LOG(WARNING) << "set call banker time, time: " << dwCallBankerTime << ", chair id: " << m_wChairID;
    return true;
}

bool CAndroidUserItemSink::OnSubCallBanker(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    SG_CMD_S_CallBanker CallBanker;
    if (!CallBanker.ParseFromArray(pData,wDataSize))
        return false;

    //自己已经叫庄 则检测定时器是否还存在
    if (CallBanker.wcallbankeruser() == m_wChairID)
//    if (CallBanker.wcallbankeruser() == m_pAndroidUserItem->GetChairId())
    {
//        m_TimerCallBanker.stop();
        m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerCallBanker);
        m_wCallBankerNum++;
    }


    return true;
}

bool CAndroidUserItemSink::OnSubCallBankerResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    SG_CMD_S_CallBankerResult BankerResult;
    if (!BankerResult.ParseFromArray(pData,wDataSize))
        return false;

    int32_t bankerChardId = BankerResult.dwbankeruser();
//    uint32_t currentChairid = m_pAndroidUserItem->GetChairId();
    if (BankerResult.dwbankeruser() != m_wChairID && m_m_cbPlayStatus)
    {
        //机器人不是庄
        int dwShowTime = 1000;//抢庄动画  3.0
        int jettontime = BankerResult.cbaddjettontime();
        int dwAddScore = dwShowTime + rand() % ((jettontime - 1)* TIME_BASE);
        for(int i = 0; i < 5; ++i)
        {
            m_cbMaxJettonMultiple[i] = BankerResult.lmaxjettonmultiple(m_wChairID*5+i);
        }
//        m_TimerAddScore.start(dwAddScore, bind(&CAndroidUserItemSink::GameTimeAddScore, this, std::placeholders::_1), NULL, 1, false);
        m_TimerAddScore = m_pTableFrame->GetLoopThread()->getLoop()->runAfter(double(dwAddScore)/1000, boost::bind(&CAndroidUserItemSink::GameTimeAddScore, this));
    }

    return true;
}

bool CAndroidUserItemSink::OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    SG_CMD_S_AddScoreResult AddScoreResult;
    if (!AddScoreResult.ParseFromArray(pData,wDataSize))
        return false;

    if (AddScoreResult.waddjettonuser()== m_wChairID && m_m_cbPlayStatus)
//    if (AddScoreResult.waddjettonuser()== m_pAndroidUserItem->GetChairId() && m_m_cbPlayStatus)
    {
//        m_TimerAddScore.stop();
        m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerAddScore);
        m_wAddScoreNum++;
    }


    return true;
}

bool CAndroidUserItemSink::OnSubSendCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    SG_CMD_S_SendCard SendCard;
    if (!SendCard.ParseFromArray(pData,wDataSize))
        return false;

    if(!m_m_cbPlayStatus)
        return true;

//    int dwShowTime = 3300;//抢庄动画  3.0
    int dwShowTime = 1000;//抢庄动画  3.0
    int opentime = SendCard.cbopentime();
    int dwOpenCardTime = dwShowTime + rand() % ((opentime - 1)* TIME_BASE);
//    m_TimerOpenCard.start(dwOpenCardTime, bind(&CAndroidUserItemSink::GameTimeOpencard, this, std::placeholders::_1), NULL, 1, false);
    m_TimerOpenCard = m_pTableFrame->GetLoopThread()->getLoop()->runAfter((double)dwOpenCardTime/1000, boost::bind(&CAndroidUserItemSink::GameTimeOpencard, this));
    return true;
}

bool CAndroidUserItemSink::OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    SG_CMD_S_OpenCardResult OpenCardResult;
    if (!OpenCardResult.ParseFromArray(pData,wDataSize))
        return false;

    if (OpenCardResult.wopencarduser() == m_wChairID)
//    if (OpenCardResult.wopencarduser() == m_pAndroidUserItem->GetChairId())
    {
//        m_TimerOpenCard.stop();
        m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerOpenCard);
    }

    return true;
}

bool CAndroidUserItemSink::OnSubOpenCardResult(uint8_t wSubCmdID, void* pData, uint32_t wDataSize)
{
    return true;
}

bool CAndroidUserItemSink::ClearAllTimer()
{
    if(CountrandomRound>=randomRound)
    {
        if(m_pAndroidUserItem)
            m_pAndroidUserItem->setOffline();
        CountrandomRound=0;
    }
    int getout=RandomInt(0,100);
    if(m_pTableFrame->GetPlayerCount(false)>=3&&getout<50)
    {
        if(m_pAndroidUserItem)
            m_pAndroidUserItem->setOffline();
    }

    if(m_pAndroidUserItem != nullptr && m_pTableFrame != nullptr)
        if(m_pAndroidUserItem->GetUserScore()<m_pTableFrame->GetGameRoomInfo()->enterMinScore
                || m_pAndroidUserItem->GetUserScore()>20000  )
        {
            m_pAndroidUserItem->setOffline();
        }

    if(m_pTableFrame != nullptr)
    {
        m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerCallBanker);
        m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerAddScore);
        m_pTableFrame->GetLoopThread()->getLoop()->cancel(m_TimerOpenCard);
    }

    return true;
}


//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{
#if 0
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/longhu_config.ini"))
    {
        LOG_INFO << "conf/longhu_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/longhu_config.ini", pt);

    //Time
    m_JettonTime           = pt.get<uint32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 15);

    //区域下注权重
    //Long
    m_AreaProbability[0]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.longAreaProbability", 45);
    //Hu
    m_AreaProbability[1]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.huAreaProbability", 45);
    //He
    m_AreaProbability[2]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.heAreaProbability", 10);

    //游戏区域最多下注金额
    //Long
    m_AreaRobotAddGold[0]  = pt.get<int64_t>("ANDROID_NEWCONFIG.longAreaRobotAddGold", 1000);
    //Hu
    m_AreaRobotAddGold[1]  = pt.get<int64_t>("ANDROID_NEWCONFIG.huAreaRobotAddGold", 1000);
    //He
    m_AreaRobotAddGold[2]  = pt.get<int64_t>("ANDROID_NEWCONFIG.heAreaRobotAddGold", 100);

    //可下注百分比权重
    m_ProbabilityWeight[0] = pt.get<uint32_t>("ANDROID_NEWCONFIG.1robability", 20);   //1
    m_ProbabilityWeight[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.5robability", 30);  //5
    m_ProbabilityWeight[2] = pt.get<uint32_t>("ANDROID_NEWCONFIG.10robability", 40); //10
    m_ProbabilityWeight[3] = pt.get<uint32_t>("ANDROID_NEWCONFIG.15robability", 50);  //15
    m_ProbabilityWeight[4] = pt.get<uint32_t>("ANDROID_NEWCONFIG.25robability", 60);  //25
    m_ProbabilityWeight[5] = pt.get<uint32_t>("ANDROID_NEWCONFIG.50robability", 100);  //50

    //最大下注筹码
    m_AddMaxGold[0]        = pt.get<int64_t>("ANDROID_NEWCONFIG.200AddMaxGold", 1);   //200
    m_AddMaxGold[1]        = pt.get<int64_t>("ANDROID_NEWCONFIG.500AddMaxGold", 10);   //500
    m_AddMaxGold[2]        = pt.get<int64_t>("ANDROID_NEWCONFIG.2000AddMaxGold", 50);  //2000
    m_AddMaxGold[3]        = pt.get<int64_t>("ANDROID_NEWCONFIG.5000AddMaxGold", 100);  //5000
    m_AddMaxGold[4]        = pt.get<int64_t>("ANDROID_NEWCONFIG.5001AddMaxGold", 500);  //5001

    //筹码权重
    m_ChouMaWeight[0]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.1ChouMa", 20);    //1
    m_ChouMaWeight[1]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.10ChouMa", 50);  //10
    m_ChouMaWeight[2]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.50ChouMa", 60);   //50
    m_ChouMaWeight[3]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.100ChouMa", 70);  //100
    m_ChouMaWeight[4]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.500ChouMa", 8);   //500

    //退出概率
    m_TimeMinuteProbability[0]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.5Minute", 50);   //5
    m_TimeMinuteProbability[1]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.7Minute", 60);   //7
    m_TimeMinuteProbability[2]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.9Minute", 70);   //9
    m_TimeMinuteProbability[3]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.11Minute", 80);  //11
    m_TimeMinuteProbability[4]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.13Minute", 90);  //13
    m_TimeMinuteProbability[5]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.15Minute", 100);  //15

    //开始下注间隔
    m_StartIntervalInit         = pt.get<uint32_t>("ANDROID_NEWCONFIG.StartIntervalInit", 3500);
    //每注间隔
    m_JettonIntervalInit        = pt.get<uint32_t>("ANDROID_NEWCONFIG.JettonIntervalInit", 12000);
    //最后3秒下注概率
    m_End3s                     = pt.get<uint32_t>("ANDROID_NEWCONFIG.END3", 18);
#endif
    return true;
}

void  CAndroidUserItemSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//sangong//sangong_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pTableFrame->GetGameRoomInfo()->roomId);
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
        return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] [Android TABLEID:%d usserid=%d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,
            m_pTableFrame->GetTableId(),m_pAndroidUserItem->GetUserId());
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}

extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

// reset all the data now.
extern "C" void DeleteAndroidSink(IAndroidUserItemSink* pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
    //Cleanup:
    delete pSink;
    pSink = NULL;
}
