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

#include "ServerUserItem.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

#include "proto/Brnn.Message.pb.h"

#include "AndroidUserItemSink.h"

using namespace Brnn;
//////////////////////////////////////////////////////////////////////////

//辅助时间
#define TIME_LESS                       2                                   //最少时间
#define TIME_DELAY_TIME					3                                   //延时时间

//游戏时间
#define TIME_START_GAME					3                                   //开始时间
#define TIME_USER_ADD_SCORE				7                                   //下注时间

//构造函数
CAndroidUserItemSink::CAndroidUserItemSink():m_rand(time(0))
{
    //游戏变量
    m_JionTime = 0;
    m_MaxWinTime = 0;
    m_IsShenSuanZi = false;
    m_ShenSuanZiMaxArea = 1;
    m_ContinueJettonCount = 0;
    m_JettonScore = 0;
    m_StartIntervalInit = 2000;
    m_JettonIntervalInit = 3000;
    m_End3s = 25;
    m_BetScore = 0;
    m_currentChips[0]=100;
    m_currentChips[1]=500;
    m_currentChips[2]=1000;
    m_currentChips[3]=5000;
    m_currentChips[4]=10000;
    m_currentChips[5]=50000;
    m_currentChips[6]=100000;
	m_testHaveBet = false;
    return;
}

//析构函数
CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

//初始接口
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
    bool bSuccess = false;
    do
    {
//        //  check the table and user item now.
        if((!pTableFrame) || (!pUserItem))
        {
            //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
            break;
        }


        m_pITableFrame   =  pTableFrame;
        m_pIAndroidUserItem = pUserItem;
        m_pIAndroidUserItem = pUserItem;

//        // try to update the specdial room config data item.
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        if(m_pGameRoomInfo)
        {
            m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
        }

//        // check if user is android user item.
        if(!pUserItem->IsAndroidUser())
        {
//            //LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserID();
        }

//        //查询接口
        ReadConfigInformation();

        bSuccess = true;

    }while(0);

//Cleanup:
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
        m_pGameRoomInfo  =  m_pITableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
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
    //LOG(INFO) << "CAndroidUserItemSink::RepositionSink.";

    return true;
}

//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    string ConfPath="";
    if(m_pGameRoomInfo->roomId == 9301)
    {
        ConfPath= "./conf/brnn_config3.ini";
    }
    else
    {
        ConfPath= "./conf/brnn_config10.ini";
    }



    if(!boost::filesystem::exists(ConfPath.c_str()))
    {
        LOG_INFO << "conf/brnn_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(ConfPath.c_str(), pt);

    //Time
    m_JettonTime           = pt.get<uint32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 15);

    //区域下注权重
    //天
    m_AreaProbability[0]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.tianAreaProbability", 45);
    //地
    m_AreaProbability[1]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.diAreaProbability", 45);
    //玄
    m_AreaProbability[2]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.xuanAreaProbability", 45);
    //黄
    m_AreaProbability[3]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.huangAreaProbability", 45);

    m_totalWeight = m_AreaProbability[0] + m_AreaProbability[1]  + m_AreaProbability[2] + m_AreaProbability[3];

    //游戏区域最多下注金额
    //天
//    m_AreaRobotAddGold[0]  = pt.get<int64_t>("ANDROID_NEWCONFIG.tianAreaRobotAddGold", 1000);
//    //地
//    m_AreaRobotAddGold[1]  = pt.get<int64_t>("ANDROID_NEWCONFIG.diAreaRobotAddGold", 1000);
//    //玄
//    m_AreaRobotAddGold[2]  = pt.get<int64_t>("ANDROID_NEWCONFIG.xuanAreaRobotAddGold", 1000);
//    //黄
//    m_AreaRobotAddGold[3]  = pt.get<int64_t>("ANDROID_NEWCONFIG.huangAreaRobotAddGold", 1000);

    //可下注百分比权重
    m_ProbabilityWeight[0] = pt.get<uint32_t>("ANDROID_NEWCONFIG.1robability", 20);   //1
    m_ProbabilityWeight[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.5robability", 30);  //5
    m_ProbabilityWeight[2] = pt.get<uint32_t>("ANDROID_NEWCONFIG.10robability", 40); //10
    m_ProbabilityWeight[3] = pt.get<uint32_t>("ANDROID_NEWCONFIG.15robability", 50);  //15
    m_ProbabilityWeight[4] = pt.get<uint32_t>("ANDROID_NEWCONFIG.25robability", 60);  //25
    m_ProbabilityWeight[5] = pt.get<uint32_t>("ANDROID_NEWCONFIG.50robability", 100);  //50

    //最大下注筹码
//    m_AddMaxGold[0]        = pt.get<int64_t>("ANDROID_NEWCONFIG.200AddMaxGold", 1);   //200
//    m_AddMaxGold[1]        = pt.get<int64_t>("ANDROID_NEWCONFIG.500AddMaxGold", 10);   //500
//    m_AddMaxGold[2]        = pt.get<int64_t>("ANDROID_NEWCONFIG.2000AddMaxGold", 50);  //2000
//    m_AddMaxGold[3]        = pt.get<int64_t>("ANDROID_NEWCONFIG.5000AddMaxGold", 100);  //5000
//    m_AddMaxGold[4]        = pt.get<int64_t>("ANDROID_NEWCONFIG.5001AddMaxGold", 500);  //5001

    //筹码权重
//    m_ChouMaWeight[0]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.1ChouMa", 20);    //1
//    m_ChouMaWeight[1]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.10ChouMa", 50);  //10
//    m_ChouMaWeight[2]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.50ChouMa", 60);   //50
//    m_ChouMaWeight[3]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.100ChouMa", 70);  //100
//    m_ChouMaWeight[4]      = pt.get<uint32_t>("ANDROID_NEWCONFIG.500ChouMa", 8);   //500

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

	//只下一个筹码并固定一个区域[1234],0的时候随机
	m_betOnlyArea = pt.get<uint32_t>("ANDROID_NEWCONFIG.onlyArea", 0);
	m_testHaveBet = false;

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


void CAndroidUserItemSink::OnTimerJetton()
{
    // openLog("OnTimerMessage IDI_USER_ADD_SCORE.........");
    int64_t wGold = m_pIAndroidUserItem->GetUserScore();
    //金币不足50
    if(wGold < 50)
    {
        //m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
        //LOG_ERROR << " Android No Money!!!!!!!!!!!!!!!!!!!! ";
        return;
    }
    //下注结束 

    if(m_pIAndroidUserItem->GetUserStatus() == sOffline)
    {
        //m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
        //LOG_ERROR << " Android Is sOffline!!!!!!!!!!!!!!!!!!!! ";
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t NowTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if(NowTime - m_TimerStart > m_Endtimer * 1000)
    {
        //m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
        //LOG_ERROR << " Android Have Notime!!!!!!!!!!!!!!!!!!!! ";
        return;
    }

    m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(0.1f,boost::bind(&CAndroidUserItemSink::OnTimerJetton, this));



    if(NowTime - m_TimerStart < (m_Endtimer - 3) * 1000)  //last 3s
    {
        int kaiShiJianGe = rand() % 500 + m_StartIntervalInit;

        //LOG_ERROR << " Android Have Notime!!!!!!!!!!!!!!!!!!!! "<<m_StartIntervalInit      <<kaiShiJianGe;
        if(m_Lasttimer == 0)
        {
            if(NowTime - m_TimerStart < kaiShiJianGe)
                return;
            else
            {
                m_Lasttimer = NowTime;
                OnSubAddScore();
                return;
            }
        }else
        {
            int jianGeTime = rand() % m_StartIntervalInit + 500;
            if(NowTime - m_Lasttimer >= jianGeTime)
            {
                m_Lasttimer = NowTime;
                OnSubAddScore();
                return;
            }
        }
    }else
    {
        if(rand() % 100 <= m_End3s)
        {
            m_Lasttimer = NowTime;
            OnSubAddScore();
            return;
        }
    }
}

void CAndroidUserItemSink::OnTimerGameEnd()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
    //大赢家次数判断
    // m_MaxWinTime++;
    m_SendArea.clear();
    //判断是否需要离开
    ExitGame();
    return;
}

//用户加注
bool CAndroidUserItemSink::OnSubAddScore()
{
    if (m_testHaveBet && m_betOnlyArea != 0)
	{
		return true;
	}
    // SCORE wAddGold = m_pIAndroidUserItem->GetUserScore()-50;//可下注金额
//    int64_t wAddGold = m_StartsScore - 50;//可下注金额
    if(m_StartsScore - m_BetScore <m_BetScore*3)// || m_StartsScore/3 <= m_BetScore)
    {
        //LOG_ERROR<<"fenshu bu zu "<<m_StartsScore;
        m_pIAndroidUserItem->setOffline();
        m_pIAndroidUserItem->setTrustee(true);
        return false;
    }

    int64_t wAddGold = (m_StartsScore - m_BetScore)/3;   //根据最大倍数确定可下注金额
    //随机可下注金额
    int prob = RandAddProbability();
    int64_t canGold = prob * wAddGold / 100;
    //随机下注筹码
    double JettonScore = 0;
    if (m_ContinueJettonCount == 0)
    {
        JettonScore = RandChouMa(canGold);
        m_JettonScore = JettonScore;
        m_ContinueJettonCount = rand() % 3 + 2;
    }
    else
    {
        JettonScore = m_JettonScore;
        m_ContinueJettonCount--;
    }

    //随机下注区域
    int bJettonArea = RandArea();

    if(m_IsShenSuanZi && m_SendArea.size()== m_ShenSuanZiMaxArea)
    {
         int index = rand() % m_SendArea.size();
         bJettonArea = m_SendArea[index];
    }

    //同时押两个区域的判断
    int m_2Area = rand() % 100;
    if (!m_IsShenSuanZi && m_2Area <= 50) //50%只能押一个区域（天地玄黄）
    {
        for (int i = 0; i < m_SendArea.size(); ++i)
        {
            if (bJettonArea != 1 && m_SendArea[i] == 1) //天
            {
                bJettonArea = 1;
                break;
            }

            if ( bJettonArea != 2 && m_SendArea[i] == 2) //地
            {
                bJettonArea = 2;
                break;
            }

            if ( bJettonArea != 3 && m_SendArea[i] == 3) //玄
            {
                bJettonArea = 3;
                break;
            }

            if ( bJettonArea != 4 && m_SendArea[i] == 4) //黄
            {
                bJettonArea = 4;
                break;
            }

        }
    }

     bool isfind = false;
     for(int i = 0; i < m_SendArea.size(); ++i)
     {
         if(m_SendArea[i] == bJettonArea)
         {
             isfind = true;
             break;
         }
     }
     if(!isfind)
         m_SendArea.push_back(bJettonArea);

    if( bJettonArea < 1 && bJettonArea > 5)
        bJettonArea = 2;

    if(JettonScore > 500 && JettonScore < 1)
    {
        if (rand() % 100 < 50)
         {
              JettonScore = 1;
              m_JettonScore = JettonScore;
         }
         else
         {
             JettonScore = 10;
             m_JettonScore = JettonScore;
         }
    }
	if (m_betOnlyArea!=0)
	{
		bJettonArea = m_betOnlyArea;
        JettonScore = 10000;
	}
    CMD_C_PlaceJet mPlaceJet;
    mPlaceJet.set_cbjettonarea(bJettonArea);
//    mPlaceJet.set_ljettonscore(JettonScore*100);
    mPlaceJet.set_ljettonscore(JettonScore);

    //openLog("userid =%d  wAddGold = %lld prob =%d canGold =%lld area =%d jettonScore =%f",\
    //        m_pIAndroidUserItem->GetUserID(),wAddGold,prob,canGold,bJettonArea,JettonScore);
    string content = mPlaceJet.SerializeAsString();
    m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(), SUB_C_PLACE_JETTON, (uint8_t *)content.data(), content.size());
    //LOG(INFO) << " >>> OnSubAddScore SUB_C_PLACE_JETTON";
	if (m_betOnlyArea != 0)
	{
		m_testHaveBet = true;
	}
}

//随机下注区域
int CAndroidUserItemSink::RandArea()
{
//    uint32_t TotalWeight = 0;
//    for(int i = 0; i < 4; ++i)
//        TotalWeight += m_AreaProbability[i];

    uint32_t randWeight = m_rand() % m_totalWeight;
    uint32_t tempweightsize = 0;
    for (int i = 0; i < 4; i++)
    {
        tempweightsize += m_AreaProbability[i];
        if (randWeight <= tempweightsize)
        {
           return i+1;
        }
    }
   // openLog("test");
    return 4;
}

//随机下注筹码
int64_t CAndroidUserItemSink::RandChouMa(int64_t CanxiazuGold)
{

    int i = 7 - 1;
    for(; i >= 0; --i)
    {
        if(m_currentChips[i] <= CanxiazuGold)
        {
//            return m_pGameRoomInfo->jettons[i];
            break;
        }
    }
    if(i >= 0)
        return m_currentChips[m_rand()%(i+1)];
    else
        return m_currentChips[0];
}

//随机可下注金额
int CAndroidUserItemSink::RandAddProbability()
{
    uint32_t TotalWeight = 0;
    for(int i = 0; i < 6; ++i)
        TotalWeight += m_ProbabilityWeight[i];

    uint32_t randWeight = rand() % TotalWeight;
    uint32_t tempweightsize = 0;
    for (int i = 0; i < 6; ++i)
    {
        tempweightsize += m_ProbabilityWeight[i];
        if (randWeight <= tempweightsize)
        {
            if(i == 0)
                return 1;
            else if(i == 1)
                 return 5;
            else if(i == 2)
                 return 10;
            else if(i == 3)
                 return 15;
            else if(i == 4)
                 return 25;
            else if(i == 5)
                 return 50;
        }
    }
}

// convert the special command to game scene.
uint8_t SubCmdIDToStatus(uint8_t wSubCmdID)
{
    uint8_t cbGameStatus = SUB_S_GAME_START;
    switch (wSubCmdID)
    {
    case SUB_S_GAME_START:
        cbGameStatus = SUB_S_GAME_START;
        break;
    case SUB_S_START_PLACE_JETTON:
        cbGameStatus = SUB_S_START_PLACE_JETTON;
        break;
    case SUB_S_GAME_END:
        cbGameStatus = SUB_S_GAME_END;
        break;
    }
//Cleanup:
    return (cbGameStatus);
}

//游戏消息
//bool CAndroidUserItemSink::OnEventGameMessage(WORD wSubCmdID, void * pData, WORD wDataSize)
bool CAndroidUserItemSink::OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size)
{
    int64_t userId = m_pIAndroidUserItem->GetUserId();
    //LOG(INFO) << "CAndroidUserItemSink::OnGameMessage wSubCmdID:" << (int)wSubCmdID << ", dwUserID:" << dwUserID;
   // openLog("OnGameMessage...");
    switch (subId)
    {
        // filter the game scene.
        case SUB_S_GAME_START:
        case SUB_S_START_PLACE_JETTON:
        case SUB_S_GAME_END:
        {
       // openLog("OnGameMessage %d..",wSubCmdID);
          //  uint8_t cbGameStatus = SubCmdIDToStatus(wSubCmdID);
            return (OnEventSceneMessage(subId, pData, size));
        }
        break;
        case SUB_S_PLACE_JETTON:		//xiazhu
        {
            CMD_S_PlaceJetSuccess JetSuccess;
            if (!JetSuccess.ParseFromArray(pData, size))
                return false;
            if(JetSuccess.dwuserid() == userId)
                m_BetScore += JetSuccess.ljettonscore();
            return true;
        }
        break;
        case SUB_S_SEND_RECORD:
        case SUB_S_PLACE_JET_FAIL:
        case SUB_S_QUERY_PLAYLIST:
        case SUB_S_GAME_FREE:
            return true;
        break;
        default:
            //错误断言
            assert(false);
            return false;
    }

    return true;
}

//场景消息
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData,  uint32_t size)
{
    switch(cbGameStatus)
    {
        case SUB_S_GAME_START:              //游戏状态
        {
            //效验数据
            // if (wDataSize!=sizeof(CMD_Scene_StatusPlay)) return false;
            // CMD_Scene_StatusPlay * pStatusPlay = (CMD_Scene_StatusPlay *)pData;
            //openLog("OnEventSceneMessage SUB_S_GAME_START..");
			if (m_betOnlyArea != 0)
			{
				ReadConfigInformation();
			}
            CMD_S_GameStart pGameStart;
            if(!pGameStart.ParseFromArray(pData, size))
                return false;

            GameDeskInfo deskinfo = pGameStart.deskdata();
            const ::google::protobuf::RepeatedField<int64_t>& chips = deskinfo.userchips();
            for(int i=0;i<7;i++)
            {
                m_currentChips[i]=chips[i];
            }
            m_BetScore = 0;
            m_StartsScore = pGameStart.luserscore();
            return true;
        }
        case SUB_S_START_PLACE_JETTON:		//游戏状态
        {
			ReadConfigInformation();
            CMD_S_StartPlaceJetton pJETTON;
            if(!pJETTON.ParseFromArray(pData, size))
            {
                //LOG_ERROR << " 数据错误!!!!!!!!!!!!!!!!!!!! ";
                return false;
            }


            m_StartsScore = pJETTON.luserscore();//m_pIAndroidUserItem->GetUserScore();

            m_IsShenSuanZi = false;
            m_ShenSuanZiMaxArea = 1;

            GameDeskInfo deskinfo = pJETTON.deskdata();
            const ::google::protobuf::RepeatedPtrField<::Brnn::PlayerListItem>& player = deskinfo.players();

            const ::google::protobuf::RepeatedField<int64_t>& chips = deskinfo.userchips();
            for(int i=0;i<7;i++)
            {
                m_currentChips[i]=chips[i];
            }
            for(int i = 0;i < player.size(); ++i)
            {
                if(player[i].dwuserid() == m_pIAndroidUserItem->GetUserId() && player[i].isdivinemath()==1)
                {
                    m_IsShenSuanZi = true;
                    m_ShenSuanZiMaxArea = 1;
                    break;
                }
                if(player[i].dwuserid() == m_pIAndroidUserItem->GetUserId() && player[i].index()==2)
                {
                    m_MaxWinTime++;
                    break;
                }
            }
            //设置定时器
            struct timeval tv;
            gettimeofday(&tv,NULL);
            m_TimerStart = tv.tv_sec*1000 + tv.tv_usec/1000;
            m_Endtimer = pJETTON.cbtimeleave();
            m_Lasttimer = 0;
            m_ContinueJettonCount = 0;
            m_JettonScore = 0;
            // openLog("endtime=%d",m_endtimer);
            if(m_Endtimer > 0)
            {
                m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(0.1f,boost::bind(&CAndroidUserItemSink::OnTimerJetton, this));
            }
            else
            {
                LOG_ERROR << " 过去的时间小于零!!!!!!!!!!!!!!!!!!!! ";
            }
            //shen suan zi
            return true;
        }
        case SUB_S_GAME_END:		//游戏状态
        {
            CMD_S_GameEnd pGameEnd;
            m_BetScore = 0;
            if (!pGameEnd.ParseFromArray(pData, size))
                return false;
            GameDeskInfo deskinfo = pGameEnd.deskdata();
            const ::google::protobuf::RepeatedField<int64_t>& chips = deskinfo.userchips();
            for(int i=0;i<7;i++)
            {
                m_currentChips[i]=chips[i];
            }
            m_TimerEnd = m_TimerLoopThread->getLoop()->runAfter(0.1f,boost::bind(&CAndroidUserItemSink::OnTimerGameEnd, this));
			ReadConfigInformation();
                return true;
        }
    }
    assert(false);
    return false;
}

bool CAndroidUserItemSink::CheckExitGame()
{
    //机器人结算后携带金币小于55自动退场
    int64_t userScore = m_pIAndroidUserItem->GetUserScore();
    return userScore >= m_strategy->exitHighScore || userScore <= m_strategy->exitLowScore;
}

bool CAndroidUserItemSink::ExitGame()
{
    //判断是否需要离开
    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(m_JionTime == 0)
    {
        m_JionTime = tv.tv_sec;
        return true;
    }

     int randTime = rand() % 100;
     int64_t PlayTime = tv.tv_sec/60 - m_JionTime/60;
     if (

         (PlayTime == 2 && randTime < m_TimeMinuteProbability[0])  ||
         (PlayTime == 4 && randTime < m_TimeMinuteProbability[1])  ||
         (PlayTime == 6 && randTime<  m_TimeMinuteProbability[2])  ||
         (PlayTime == 8 && randTime < m_TimeMinuteProbability[3]) ||
         (PlayTime == 10 && randTime < m_TimeMinuteProbability[4]) ||
         (PlayTime == 12 &&randTime < m_TimeMinuteProbability[5]) ||
          m_AndroidExitTimes < PlayTime || (m_MaxWinTime >= 10) || CheckExitGame())//(m_pIAndroidUserItem->GetUserScore()<55))
     {
//         openLog("11111111 %d exituser=%d",PlayTime,m_pIAndroidUserItem->GetUserID());
         //m_pITableFrame->OnUserLeft(m_pIAndroidUserItem);
         //m_pITableFrame->ClearTableUser(m_pIAndroidUserItem->GetChairID());
         m_pIAndroidUserItem->setOffline();
         m_pIAndroidUserItem->setTrustee(true);
         m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
         m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerEnd);
         m_JionTime = 0;
         m_MaxWinTime= 0;
         return true;
     }

   return true;
}

//组件创建函数
// DECLARE_CREATE_MODULE(AndroidUserItemSink);
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

// reset all the data now.
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink>& pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
//Cleanup:
    pSink.reset();
}





//////////////////////////////////////////////////////////////////////////
