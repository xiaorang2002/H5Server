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

#include "proto/Hhdn.Message.pb.h"

using namespace Hhdn;

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
    m_currentChips[0]=100;
    m_currentChips[1]=500;
    m_currentChips[2]=1000;
    m_currentChips[3]=5000;
    m_currentChips[4]=10000;
    m_currentChips[5]=50000;
    m_currentChips[6]=100000;
    return;
}

//析构函数
CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

/*
//接口查询
void * CAndroidUserItemSink::QueryInterface(REFGUID Guid, DWORD dwQueryVer)
{
	QUERYINTERFACE(IAndroidUserItemSink,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAndroidUserItemSink,Guid,dwQueryVer);
	return NULL;
}
*/

//初始接口
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
    //LOG(INFO) << "CAndroidUserItemSink::Initialization pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
    // openLog("Initialization。。。");
    bool bSuccess = false;
    do
    {
        //  check the table and user item now.
        if((!pTableFrame) || (!pUserItem))
        {
            //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
            break;
        }

        //查询接口
        // m_pIAndroidUserItem=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,IAndroidUserItem);
        // if (m_pIAndroidUserItem==NULL) return false;
        m_pITableFrame   =  pTableFrame;
        m_pIAndroidUserItem = pUserItem;

        // try to update the specdial room config data item.
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        if(m_pGameRoomInfo)
        {
            m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
        }

        // check if user is android user item.
        if(!pUserItem->IsAndroidUser())
        {
            //LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserID();
        }

        //查询接口
        ReadConfigInformation();

        bSuccess = true;

    }while(0);

//Cleanup:
    return (bSuccess);
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem> &pUserItem)
{
    m_pIAndroidUserItem = pUserItem;
    // check if user is android user item.
    if(!pUserItem->IsAndroidUser())
    {
        //LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserID();
    }

    //查询接口
    ReadConfigInformation();
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{
     m_pITableFrame   =  pTableFrame;
     m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
     if(m_pGameRoomInfo)
     {
         m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
     }
	 ReadConfigInformation();
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
    if(!boost::filesystem::exists("./conf/hhdn_config.ini"))
    {
        LOG_INFO << "conf/hhdn_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/hhdn_config.ini", pt);

    //Time
    m_JettonTime           = pt.get<uint32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 15);

	//区域下注权重
	//He
	m_AreaProbability[0] = pt.get<uint32_t>("ANDROID_NEWCONFIG.HEAreaprobability", 45);
	//黑牛
	m_AreaProbability[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.HEIAreaprobability", 400);
    //红牛
    m_AreaProbability[2]   = pt.get<uint32_t>("ANDROID_NEWCONFIG.HONGAreaprobability", 400);
	string strIndex = "";
	for (int i = 3; i < MAX_AREA; ++i)
	{
		strIndex = "ANDROID_NEWCONFIG.NIUAreaprobability_" + std::to_string(i - 2);
		if (i == MAX_AREA-1)
		{
			m_AreaProbability[i] = pt.get<uint32_t>(strIndex, 5);
		} 
		else
		{
			m_AreaProbability[i] = pt.get<uint32_t>(strIndex, 15);
		}
	}

    //可下注百分比权重
    m_ProbabilityWeight[0] = pt.get<uint32_t>("ANDROID_NEWCONFIG.1robability", 20);   //1
    m_ProbabilityWeight[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.2robability", 30);  //5
    m_ProbabilityWeight[2] = pt.get<uint32_t>("ANDROID_NEWCONFIG.3robability", 40); //10
    m_ProbabilityWeight[3] = pt.get<uint32_t>("ANDROID_NEWCONFIG.4robability", 50);  //15
    m_ProbabilityWeight[4] = pt.get<uint32_t>("ANDROID_NEWCONFIG.5robability", 60);  //25
    m_ProbabilityWeight[5] = pt.get<uint32_t>("ANDROID_NEWCONFIG.6robability", 100);  //50

    //退出概率
    m_TimeMinuteProbability[0]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.5Minute", 50);   //5
    m_TimeMinuteProbability[1]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.7Minute", 60);   //7
    m_TimeMinuteProbability[2]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.9Minute", 70);   //9
    m_TimeMinuteProbability[3]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.11Minute", 80);  //11
    m_TimeMinuteProbability[4]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.13Minute", 90);  //13
    m_TimeMinuteProbability[5]  = pt.get<uint32_t>("ANDROID_NEWCONFIG.15Minute", 100);  //15

    //开始下注间隔
    m_StartIntervalInit         = pt.get<uint32_t>("ANDROID_NEWCONFIG.kaishijiangeInit", 3500);
    //每注间隔
    m_JettonIntervalInit        = pt.get<uint32_t>("ANDROID_NEWCONFIG.jiangeTimeInit", 12000);
    //最后3秒下注概率
    m_End3s                     = pt.get<uint32_t>("ANDROID_NEWCONFIG.END3", 18);
    m_AndroidJettonProb         = pt.get<double>("ANDROID_NEWCONFIG.JettonProb", 0.3);

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
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
        //m_TimerJetton.stop();
        return;
    }
    //下注结束
    //openLog("nowTime=%lld m_timerStart=%lld cha=%lld",time(0),m_timerStart,time(0) - m_timerStart);

    if(m_pIAndroidUserItem->GetUserStatus() == sOffline)
    {
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
        //m_TimerJetton.stop();
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t NowTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if(NowTime - m_TimerStart > m_Endtimer * 1000)
    {
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
        //m_TimerJetton.stop();
        return;
    }

    if(NowTime - m_TimerStart < (m_Endtimer - 3) * 1000)  //last 3s
    {
        int kaiShiJianGe = rand() % 4000 + m_StartIntervalInit;
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
//            int jianGeTime = rand() % m_StartIntervalInit + 1000;
//            if(NowTime - m_Lasttimer >= jianGeTime)
//            {
//                m_Lasttimer = NowTime;
//                OnSubAddScore();
//                return;
//            }
            if(rand()%3==1)
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
    //m_TimerEnd.stop();
    //删除定时器
//  m_pIAndroidUserItem->KillTimer(IDI_USER_ADD_SCORE);
//  m_pIAndroidUserItem->KillTimer(IDI_GameEnd);
    //大赢家次数判断
    // m_MaxWinTime++;
    m_SendArea.clear();
    //判断是否需要离开
    ExitGame();
    return;
}

////时间消息
////bool CAndroidUserItemSink::OnEventTimer(UINT nTimerID)
//bool CAndroidUserItemSink::OnTimerMessage(uint32_t nTimerID, uint32_t dwParam)
//{
//    //LOG(INFO) << "CAndroidUserItemSink::OnTimerMessage timerid:" << nTimerID << ", dwParam:" << dwParam;
//	switch (nTimerID)
//	{
//        case IDI_USER_ADD_SCORE:
//        {
//            // openLog("OnTimerMessage IDI_USER_ADD_SCORE.........");
//            SCORE wGold = m_pIAndroidUserItem->GetUserScore();
//            //金币不足50
//            if(wGold < 50)
//                return true;
//            //下注结束
//            //openLog("nowTime=%lld m_timerStart=%lld cha=%lld",time(0),m_timerStart,time(0) - m_timerStart);

//            if(m_pIAndroidUserItem->GetUserStatus() == sOffline)
//                return true;

//            struct timeval tv;
//            gettimeofday(&tv, NULL);
//            time_t NowTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
//            if(NowTime - m_TimerStart > m_Endtimer * 1000)
//                return true;

//            if(NowTime - m_TimerStart < (m_Endtimer - 3) * 1000)  //last 3s
//            {
//                int kaiShiJianGe = rand() % 500 + m_StartIntervalInit;
//                if(m_Lasttimer == 0)
//                {
//                    if(NowTime - m_TimerStart < kaiShiJianGe)
//                        return true;
//                    else
//                    {
//                        m_Lasttimer = NowTime;
//                        OnSubAddScore();
//                        return true;
//                    }
//                }else
//                {
//                    int jianGeTime = rand() % m_StartIntervalInit + 500;
//                    if(NowTime - m_Lasttimer >= jianGeTime)
//                    {
//                        m_Lasttimer = NowTime;
//                        OnSubAddScore();
//                        return true;
//                    }
//                }
//            }else
//            {
//                if(rand() % 100 <= m_End3s)
//                {
//                    m_Lasttimer = NowTime;
//                    OnSubAddScore();
//                    return true;
//                }
//            }
//            return true;
//         }
//        case IDI_GameEnd:
//        {
//            //删除定时器
////            m_pIAndroidUserItem->KillTimer(IDI_USER_ADD_SCORE);
////            m_pIAndroidUserItem->KillTimer(IDI_GameEnd);
//            //大赢家次数判断
//            // m_MaxWinTime++;
//            m_SendArea.clear();
//            //判断是否需要离开
//            ExitGame();
//            return true;
//        }
//        default:
//              break;
//    }
//    return false;
//}

//用户加注
bool CAndroidUserItemSink::OnSubAddScore()
{
    // SCORE wAddGold = m_pIAndroidUserItem->GetUserScore()-50;//可下注金额
    int64_t wAddGold = m_StartsScore - 50;//可下注金额
    //随机可下注金额
    int prob = RandAddProbability();
    int64_t canGold = wAddGold ;//* wAddGold / 100;
    //随机下注筹码
    double JettonScore = 0;
    if (m_ContinueJettonCount == 0L)
    {
        JettonScore = RandChouMa(canGold,prob);
        m_JettonScore = JettonScore;
        m_ContinueJettonCount = rand() % 3 + 2;
    }
    else
    {
        JettonScore = m_JettonScore;
        m_ContinueJettonCount--;
    }
    if(JettonScore ==0||JettonScore>canGold)
    {
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
        return true;
    }
    //随机下注区域
    int bJettonArea = RandArea();

    if(m_IsShenSuanZi && m_SendArea.size()== m_ShenSuanZiMaxArea)
    {
//         srand((int)time(0));
         int index = rand() % m_SendArea.size();
         bJettonArea = m_SendArea[index];
    }

    //同时押两个区域的判断
    int m_2Area = rand() % 100;
    if (!m_IsShenSuanZi && m_2Area <= 95) //95%只能押一个区域（龙虎）
    {
        for (int i = 0; i < m_SendArea.size(); ++i)
        {
            if (bJettonArea == 1 && m_SendArea[i] == 2)
            {
                bJettonArea = 2;
                break;
            }
            if (bJettonArea == 2 && m_SendArea[i] == 1)
            {
                bJettonArea = 1;
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

//    //和区域下注不能大于10
//    if(bJettonArea == HE && JettonScore > m_pITableFrame->GetGameRoomInfo()->jettons[HE])
//    {
//        if (rand() % 100 < 50)
//         {
//              JettonScore = m_pITableFrame->GetGameRoomInfo()->jettons[0];
//              m_JettonScore = JettonScore;
//         }
//         else
//         {
//             JettonScore = m_pITableFrame->GetGameRoomInfo()->jettons[1];
//             m_JettonScore = JettonScore;
//         }
//    }

    if( bJettonArea < 0 && bJettonArea > MAX_AREA)
        bJettonArea = 2;

    m_RoundJettonScore+=JettonScore;

    CMD_C_PlaceJet mPlaceJet;
    mPlaceJet.set_cbjettonarea(bJettonArea);
    mPlaceJet.set_ljettonscore(JettonScore);

    //openLog("userid =%d  wAddGold = %lld prob =%d canGold =%lld area =%d jettonScore =%f",\
    //        m_pIAndroidUserItem->GetUserID(),wAddGold,prob,canGold,bJettonArea,JettonScore);
    string content = mPlaceJet.SerializeAsString();
    m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(), SUB_C_PLACE_JETTON, (uint8_t *)content.data(), content.size());
    //LOG(INFO) << " >>> OnSubAddScore SUB_C_PLACE_JETTON";
    m_RoundJettonCount++;
    if(m_pIAndroidUserItem->GetUserScore()>10000)
    {
        if(m_RoundJettonScore>m_pIAndroidUserItem->GetUserScore()*m_AndroidJettonProb)
            m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
    }else if(m_pIAndroidUserItem->GetUserScore()>5000)
    {
        if(m_RoundJettonCount==3)
            m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
    }
    else
    {
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
    }
}

//随机下注区域
int CAndroidUserItemSink::RandArea()
{
    uint32_t TotalWeight = 0;
    for(int i = 0; i < MAX_AREA; ++i)
        TotalWeight += m_AreaProbability[i];

    uint32_t randWeight = rand() % TotalWeight;
    uint32_t tempweightsize = 0;
    for (int i = 0; i < MAX_AREA; i++)
    {
        tempweightsize += m_AreaProbability[i];
        if (randWeight <= tempweightsize)
        {
			return i;
        }
    }
   // openLog("test");
}

//随机下注筹码
double CAndroidUserItemSink::RandChouMa(int64_t CanxiazuGold,int8_t GoldId)
{

    if(!m_currentChips[GoldId])
        GoldId=0;
    uint64_t socre=0;
    uint64_t whileCount=20;
    while(!socre&&whileCount--)
    {
        if(CanxiazuGold>m_currentChips[GoldId])
        {
            CanxiazuGold=RandAddProbability();
        }else
            socre=m_currentChips[GoldId];
    }
    return socre;
}

//随机可下注金额
int CAndroidUserItemSink::RandAddProbability()
{
    uint32_t TotalWeight = 0;
    for(int i = 0; i < 6; ++i)
        TotalWeight += m_ProbabilityWeight[i];

    uint32_t randWeight = rand() % TotalWeight;
    uint32_t tempweightsize = 0;
    int goldId=0;
    for (int i = 0; i < 6; ++i)
    {
        tempweightsize += m_ProbabilityWeight[i];
        if (randWeight <= tempweightsize)
        {
            goldId=i;
            break;
        }
    }
    if(m_pIAndroidUserItem->GetUserScore()<10000)
    {
        goldId=0;
    }
    return goldId;
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
                m_StartsScore -= JetSuccess.ljettonscore();
            return true;
        }
        break;
        case SUB_S_SEND_RECORD:
        case SUB_S_PLACE_JET_FAIL:
        case SUB_S_QUERY_PLAYLIST:
        case SUB_S_GAME_FREE:
        case SUB_S_JETTON_BROADCAST:
		case SUB_S_REPEAT_JETTON_OK:
            return true;
        break;
        default:
            //错误断言
            assert(false);
            return false;
	}

	return true;
}

/*
//游戏消息
bool CAndroidUserItemSink::OnEventFrameMessage(WORD wSubCmdID, void * pData, WORD wDataSize)
{
	return true;
}
*/

//场景消息
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData,  uint32_t size)
{
    //LOG(INFO) << "CAndroidUserItemSink::OnEventSceneMessage cbGameStatus:" << (int)cbGameStatus << ", wDataSize:" << wDataSize;
    //openLog("OnEventSceneMessage...");
    switch(cbGameStatus)
	{
        case SUB_S_GAME_START:              //游戏状态
        {
            //效验数据
            // if (wDataSize!=sizeof(CMD_Scene_StatusPlay)) return false;
            // CMD_Scene_StatusPlay * pStatusPlay = (CMD_Scene_StatusPlay *)pData;
            //openLog("OnEventSceneMessage SUB_S_GAME_START..");
            CMD_S_GameStart pGameStart;
            if(!pGameStart.ParseFromArray(pData, size))
                return false;
            GameDeskInfo deskinfo = pGameStart.deskdata();
            const ::google::protobuf::RepeatedField<int64_t>& chips = deskinfo.userchips();
            for(int i=0;i<7;i++)
            {
                m_currentChips[i]=chips[i];
            }
            return true;
        }
        case SUB_S_START_PLACE_JETTON:		//游戏状态
        {
            //效验数据
            // if (wDataSize!=sizeof(CMD_Scene_StatusPlay)) return false;
            // CMD_Scene_StatusPlay * pStatusPlay = (CMD_Scene_StatusPlay *)pData;
            // openLog("OnEventSceneMessage SUB_S_PLACE_JETTON.. %d",m_pIAndroidUserItem->GetUserID());
            CMD_S_StartPlaceJetton pJETTON;
            if(!pJETTON.ParseFromArray(pData, size))
                return false;
            m_RoundJettonCount=0;
            m_RoundJettonScore=0;
            m_StartsScore = m_pIAndroidUserItem->GetUserScore();

            m_IsShenSuanZi = false;
            m_ShenSuanZiMaxArea = 1;

            GameDeskInfo deskinfo = pJETTON.deskdata();
            const ::google::protobuf::RepeatedPtrField<::Hhdn::PlayerListItem>& player = deskinfo.players();

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
            float fd=(rand()%20)/10;
            if(m_Endtimer > 0)
                m_TimerJetton=m_pITableFrame->GetLoopThread()->getLoop()->runEvery(fd,bind(&CAndroidUserItemSink::OnTimerJetton, this));
            //shen suan zi
            if(true)
                return true;
        }
        case SUB_S_GAME_END:		//游戏状态
        {
            //效验数据
            // if (wDataSize!=sizeof(CMD_Scene_StatusPlay)) return false;
            // CMD_Scene_StatusPlay * pStatusPlay = (CMD_Scene_StatusPlay *)pData;
            //openLog("OnEventSceneMessage SUB_S_GAME_END..");
            m_RoundJettonCount=0;
            m_RoundJettonScore=0;
            CMD_S_GameEnd pGameEnd;
            if (!pGameEnd.ParseFromArray(pData, size))
                return false;
            GameDeskInfo deskinfo = pGameEnd.deskdata();
            const ::google::protobuf::RepeatedField<int64_t>& chips = deskinfo.userchips();
            for(int i=0;i<7;i++)
            {
                m_currentChips[i]=chips[i];
            }
            //m_TimerEnd.start(100, bind(&CAndroidUserItemSink::OnTimerGameEnd, this, std::placeholders::_1), NULL, 1, false);
            m_TimerEnd=m_pITableFrame->GetLoopThread()->getLoop()->runAfter(0.1,bind(&CAndroidUserItemSink::OnTimerGameEnd, this));
            return true;
        }
	}
    assert(false);
	return false;
}

bool CAndroidUserItemSink::CheckExitGame()
{
    //机器人结算后携带金币小于55自动退场
//    return m_pIAndroidUserItem->GetUserScore() >= 55 ? false : true;
    int64_t userScore = m_pIAndroidUserItem->GetUserScore();
//    LOG(INFO) << "User Score : " << (int)userScore << " ,High Line : " << (int)m_strategy->exitHighScore <<", Low Line : "<< (int)m_strategy->exitLowScore;
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
     int PlayTime = tv.tv_sec/60 - m_JionTime/60;
     if (/*(PlayTime == 5 && randTime < m_TimeMinuteProbability[0])  ||
         (PlayTime == 7 && randTime < m_TimeMinuteProbability[1])  ||
         (PlayTime == 9 && randTime<  m_TimeMinuteProbability[2])  ||
         (PlayTime == 11 && randTime < m_TimeMinuteProbability[3]) ||
         (PlayTime == 13 && randTime < m_TimeMinuteProbability[4]) ||
         (PlayTime == 15 && randTime < m_TimeMinuteProbability[5]) ||*/
        m_AndroidExitTimes<PlayTime||
         (m_MaxWinTime >= 10) ||
         CheckExitGame())//(m_pIAndroidUserItem->GetUserScore()<55))
     {
		 LOG(INFO) << "===需踢出的AI PlayTime=" << PlayTime << ", exituserid:" << m_pIAndroidUserItem->GetUserId();
         //openLog("===需踢出的AI PlayTime=%d exituserid=%d", PlayTime, m_pIAndroidUserItem->GetUserId());
         //m_pITableFrame->OnUserLeft(m_pIAndroidUserItem);
         //m_pITableFrame->ClearTableUser(m_pIAndroidUserItem->GetChairID());
         m_pIAndroidUserItem->setOffline();
         m_pIAndroidUserItem->setTrustee(true);
         m_JionTime = 0;
         m_MaxWinTime= 0;
         return true;
     }

   return true;
}



//void  CAndroidUserItemSink::openLog(const char *p, ...)
//{
//    char Filename[256] = { 0 };
//    struct tm *t;
//    time_t tt;
//    time(&tt);
//    t = localtime(&tt);
//    sprintf(Filename, "log//Hhdn//Hhdn_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
//    FILE *fp = fopen(Filename, "a");
//    if (NULL == fp)
//    {
//        return;
//    }
//    va_list arg;
//    va_start(arg, p);
//    fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
//    vfprintf(fp, p, arg);
//    fprintf(fp, "\n");
//    fclose(fp);
//}


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
