#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>

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

#include "ServerUserItem.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

#include "proto/qhb.Message.pb.h"

using namespace qhb;

#include "AndroidUserItemSink.h"

//////////////////////////////////////////////////////////////////////////

//辅助时间
#define TIME_LESS                       2                                   //最少时间
#define TIME_DELAY_TIME					3                                   //延时时间

//游戏时间
#define TIME_START_GAME					3                                   //开始时间
#define TIME_USER_ADD_SCORE				7                                   //下注时间

//构造函数
CAndroidUserItemSink::CAndroidUserItemSink()
{

    return;
}

//析构函数
CAndroidUserItemSink::~CAndroidUserItemSink()
{
}
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData, uint32_t size)
{
    return true;
}
//初始接口
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{

    bool bSuccess = false;
    return (bSuccess);
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pUserItem)
{
    if(pUserItem)
    {
        m_pIAndroidUserItem = pUserItem;
    }
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    if(pTableFrame)
    {
        m_pITableFrame   =  pTableFrame;
        pTableFrame->GetGameRoomInfo()->tableCount;//桌子数
        m_pGameRoomInfo  =  m_pITableFrame->GetGameRoomInfo();
        if(m_pGameRoomInfo)
        {
            m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
        }
        ReadConfigInformation();
    }
}

//重置接口
bool CAndroidUserItemSink::RepositionSink()
{

	return true;
}

//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{
    //设置文件名
//    //=====config=====
    if(!boost::filesystem::exists("./conf/qhb_config.ini"))
    {
        LOG_INFO << "conf/qhb_config.ini not exists";
        return false;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/qhb_config.ini", pt);
    //退出概率
    m_TimeMinuteProbability[0]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.5Minute", 50);   //5
    m_TimeMinuteProbability[1]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.9Minute", 60);   //7
    m_TimeMinuteProbability[2]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.15Minute", 70);   //9
    m_TimeMinuteProbability[3]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.19Minute", 80);  //11
    m_TimeMinuteProbability[4]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.23Minute", 90);  //13
    m_TimeMinuteProbability[5]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.30Minute", 100);  //15


    //计算退出时间
    uint32_t TotalWeight = 0;
    for(int i = 0; i < 6; ++i)
        TotalWeight += m_TimeMinuteProbability[i];
    uint32_t times[6]={5,9,15,19,23,30};
    uint32_t randWeight = rand() % TotalWeight;
    uint32_t tempweightsize = 0;
    for (int i = 0; i < 6; ++i)
    {
        tempweightsize += m_TimeMinuteProbability[i];
        if (randWeight <= tempweightsize)
        {
            m_AndroidExitTimes=times[i];
            break;
        }
    }
    return true;
}

//游戏消息
bool CAndroidUserItemSink::OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size)
{
    int64_t userId = m_pIAndroidUserItem->GetUserId();
    switch (subId)
    {
        case SUB_S_GAME_END:
        {
            //结算
            return true;
        }
        case SUB_S_START_GRABENVELOPE:
        {
            //庄家已经发红包通知玩家开始抢
            return true;
        }
        break;
        case SUB_S_GRAB_ENVELOPE:
        {
            //有玩家抢红包
            return true;
        }
        break;
        case SUB_S_READY:
        {
            //通知庄家发红包，通知玩家准备抢红包
            CMD_S_Ready readySend;
            if (!readySend.ParseFromArray(pData, size))
            return false;
            if(!readySend.isbanker())
            {
                return true;
            }

            double grabtime=m_random.betweenFloat(1.0f,5.0f).randFloat_mt(true);
            m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerSend);
            m_TimerSend = m_pITableFrame->GetLoopThread()->getLoop()->runAfter(grabtime,boost::bind(&CAndroidUserItemSink::PalyerSendEvelope,this));
            return true;
        }
        break;
        case SUB_S_SCENSES:
        {
            //场景消息
            return true;
        }
        break;
        case SUB_S_ANDROID_GRAB:
        {
            CMD_S_Android_Grab grab;
            if (!grab.ParseFromArray(pData, size))
                return false;
            if(m_pITableFrame->GetGameStatus()!=GAME_STATUS_START)
            {
                return false;
            }
            double grabtime=grab.grabtime();
            m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerGrab);
            m_TimerGrab = m_pITableFrame->GetLoopThread()->getLoop()->runAfter(grabtime,boost::bind(&CAndroidUserItemSink::PalyerGrab,this));
                //机器人消息
                return true;
        }
        break;
        default:
            //错误断言
            return false;
	}

	return true;
}
void  CAndroidUserItemSink::PalyerSendEvelope()
{
    CMD_C_Send sendHong;

    sendHong.set_headid(0);
    sendHong.set_userid(m_pIAndroidUserItem->GetUserId());

    string content = sendHong.SerializeAsString();
    m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),qhb::SUB_C_SEND_ENVELOPE,(uint8_t*) content.data(), content.size());

}
void  CAndroidUserItemSink::PalyerGrab()
{
    CMD_C_Grab grabHong;

    grabHong.set_headid(0);
    grabHong.set_userid(m_pIAndroidUserItem->GetUserId());

    string content = grabHong.SerializeAsString();
    m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),qhb::SUB_C_GRAB_ENVELOPE,(uint8_t*) content.data(), content.size());
    ExitGame();
}
bool CAndroidUserItemSink::CheckExitGame()
{
    int64_t userScore = m_pIAndroidUserItem->GetUserScore();
    return userScore >= m_strategy->exitHighScore || userScore <= m_strategy->exitLowScore;
}

bool CAndroidUserItemSink::ExitGame()
{
    int64_t userScore = m_pIAndroidUserItem->GetUserScore();
    if(userScore <= m_strategy->exitLowScore)
    {
        m_pIAndroidUserItem->setOffline();
        m_pIAndroidUserItem->setTrustee(true);
        m_JionTime = 0;
    }
    if(m_pITableFrame->GetAndroidPlayerCount()<12)
    {
        return true;
    }
    //判断是否需要离开
    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(m_JionTime == 0)
    {
        m_JionTime = tv.tv_sec;
        return true;
    }
    int PlayTime = tv.tv_sec/60 - m_JionTime/60;
    if ( m_AndroidExitTimes<PlayTime||CheckExitGame())
    {
        m_pIAndroidUserItem->setOffline();
        m_pIAndroidUserItem->setTrustee(true);
        m_JionTime = 0;
        return true;
    }
   return true;
}


extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<IAndroidUserItemSink> pIAndroidSink(new CAndroidUserItemSink());
    return pIAndroidSink;
}
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink>& pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
    pSink.reset();
}





//////////////////////////////////////////////////////////////////////////
