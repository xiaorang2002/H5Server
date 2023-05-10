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

#include "proto/Tb.Message.pb.h"

using namespace Tb;

#include "AndroidUserItemSink.h"

//////////////////////////////////////////////////////////////////////////

//辅助时间
#define TIME_LESS 			2	//最少时间
#define TIME_DELAY_TIME 	3 	//延时时间
//游戏时间
#define TIME_START_GAME 	3	//开始时间
#define TIME_USER_ADD_SCORE 7	//下注时间

//构造函数
CAndroidUserItemSink::CAndroidUserItemSink()
{
	//游戏变量 
	m_MaxWinTime = 0;
	m_isShensuanzi = false;
	m_ShenSuanZiMaxArea = 1;
	m_ContinueJettonCount = 0;
	m_JettonScore = 0;
	m_StartIntervalInit = 2000;
	m_JettonIntervalInit = 3000;
	m_End3s = 25; 

	m_pITableFrame = NULL;
	m_wChairID = INVALID_CHAIR;
    m_wUserID = 0;
	m_pIAndroidUserItem = NULL;
	m_jionTime = 0;
	m_Lasttimer = 0;
	m_Endtimer= 0;
	m_StartsScore = 0;
	srand((unsigned)time(NULL));
	m_lianxuxiazhuCount = 0;
	m_GameRound = 0;

    memset(m_AreaRobotAddGold,0,sizeof m_AreaRobotAddGold);
	m_AreaRobotAddGold[0] = 8000;//0:兔子
	m_AreaRobotAddGold[1] = 8000;//1:燕子
	m_AreaRobotAddGold[2] = 8000;//2:猴子
	m_AreaRobotAddGold[3] = 8000;//3:鸽子
	m_AreaRobotAddGold[4] = 5000;//4:熊猫
	m_AreaRobotAddGold[5] = 5000;//5:孔雀
	m_AreaRobotAddGold[6] = 4000;//6:狮子
	m_AreaRobotAddGold[7] = 4000;//7:老鹰
	m_AreaRobotAddGold[8] = 2000;//8:银鲨
	m_AreaRobotAddGold[9] = 1000;//9:金鲨
	m_AreaRobotAddGold[10] = 40000;//10:走兽
	m_AreaRobotAddGold[11] = 40000;//11:飞禽

	for (int i = 0; i < BET_ITEM_COUNT; i++)
		m_CurrentRobotAddGold[i] = 0;
}

//析构函数
CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

//初始接口(此方法没用到)
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame> &pTableFrame, shared_ptr<IServerUserItem> &pUserItem)
{
	LOG(WARNING) << "Table Id:" << pTableFrame->GetTableId() << ", UserId:" << pUserItem->GetUserId();

	bool bSuccess = false;
	do
	{
		//  check the table and user item now.
		if ((!pTableFrame) || (!pUserItem))
		{
			LOG_ERROR << "Android Userp Table frame or pUserItem is NULL!";
			break;
		}

		//查询接口
		m_pITableFrame = pTableFrame;
		m_pIAndroidUserItem = pUserItem;

		// try to update the specdial room config data item.
		m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
		if (m_pGameRoomInfo)
		{
			m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
		}

		// check if user is android user item.
		if (!pUserItem->IsAndroidUser())
		{
			LOG_ERROR << "pUserItem is not android!, userid:" << pUserItem->GetUserId();
		}

		LOG(WARNING) << "------查询接口------" << __FUNCTION__;

		//查询接口
		ReadConfigInformation();

		bSuccess = true;

	} while (0);

	//Cleanup:
	return (bSuccess);
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem> &pUserItem)
{
	if (pUserItem){
		m_pIAndroidUserItem = pUserItem;
		return true;
	}

	LOG(ERROR) << "------生成设置机器人错误------" << __FUNCTION__;
	return false;
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{

	if (pTableFrame)
	{
		m_pITableFrame = pTableFrame;
		m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
		m_TimerLoopThread = m_pITableFrame->GetLoopThread();


		if (m_pGameRoomInfo)
		{
			m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
		}
		ReadConfigInformation();
	}
	else
	{
		LOG(ERROR) << "------设置机器人桌子信息错误------" << __FUNCTION__;
	} 
}

//重置接口
bool CAndroidUserItemSink::RepositionSink()
{
	m_jionTime = 0;
	m_Lasttimer = 0;
	m_Endtimer = 0;
	m_StartsScore = 0;
	srand((unsigned)time(NULL));
	// m_pITableFrame = NULL;
	// m_pIAndroidUserItem = NULL;
	m_wChairID = INVALID_CHAIR;
	m_lianxuxiazhuCount = 0;
	for (int i=0;i<BET_ITEM_COUNT;i++){
		m_CurrentRobotAddGold[i] = 0;
	}

	LOG(ERROR) << "------读取配置------" << __FUNCTION__;

	return true;
}

//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{	
	//设置文件名
    if (!boost::filesystem::exists("./conf/tb_config.ini"))
	{
        LOG(WARNING) << "conf tb_config.ini not exists";
		return false;
	}

	boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/tb_config.ini", pt);

	// 区域下注权重
	char buf[64] = { 0 };
    for (int i = 0; i < BET_ITEM_COUNT; ++i)
	{
        sprintf(buf, "ANDROID_NEWCONFIG.Areapro_%d",i);
		m_AreaProbability[i] = pt.get<int32_t>(buf, 100);

		// 可下注百分比权重
		if(i < 6){
			sprintf(buf, "ANDROID_NEWCONFIG.Weightpro_%d",i);
			m_ProbabilityWeight[i] = pt.get<int32_t>(buf, 10);
		}

		// 最大下注筹码
		if(i < 5){
			sprintf(buf, "ANDROID_NEWCONFIG.MaxGold_%d",i);
			m_AddMaxGold[i] = pt.get<int32_t>(buf, 10);		
		}

		// 区域最大下注筹码
		sprintf(buf, "ANDROID_AREA_MAX_JETTON.AreaMaxBet_%d",i);
		m_AreaRobotAddGold[i] = pt.get<int32_t>(buf, 1000);
	}

	// 筹码权重
	m_ChouMaWeight[0] = pt.get<int32_t>("ANDROID_NEWCONFIG.1ChouMa", 30);
	m_ChouMaWeight[1] = pt.get<int32_t>("ANDROID_NEWCONFIG.10ChouMa", 60);
	m_ChouMaWeight[2] = pt.get<int32_t>("ANDROID_NEWCONFIG.50ChouMa", 40);
	m_ChouMaWeight[3] = pt.get<int32_t>("ANDROID_NEWCONFIG.100ChouMa", 5);
	m_ChouMaWeight[4] = pt.get<int32_t>("ANDROID_NEWCONFIG.500ChouMa", 0);

	m_TimeMinuteProbability[0] = pt.get<int32_t>("ANDROID_NEWCONFIG.5Minute", 50);
	m_TimeMinuteProbability[1] = pt.get<int32_t>("ANDROID_NEWCONFIG.7Minute", 60);
	m_TimeMinuteProbability[2] = pt.get<int32_t>("ANDROID_NEWCONFIG.9Minute", 70);
	m_TimeMinuteProbability[3] = pt.get<int32_t>("ANDROID_NEWCONFIG.11Minute", 80);
	m_TimeMinuteProbability[4] = pt.get<int32_t>("ANDROID_NEWCONFIG.13Minute", 90);
	m_TimeMinuteProbability[5] = pt.get<int32_t>("ANDROID_NEWCONFIG.15Minute", 100);
 
	// m_Jetton = pt.get<int32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 15);
	m_Jetton = pt.get<int32_t>("GAME_STATE_TIME.TM_GAME_START", 15);
	m_kaishijiangeInit = pt.get<int32_t>("ANDROID_NEWCONFIG.kaishijiangeInit", 3500);
	m_jiangeTimeInit = pt.get<int32_t>("ANDROID_NEWCONFIG.jiangeTimeInit", 7500);
	m_End3s = pt.get<int32_t>("ANDROID_NEWCONFIG.END3", 20);

	//计算退出时间
	uint32_t TotalWeight = 0;
	for (int i = 0; i < 6; ++i)
		TotalWeight += m_TimeMinuteProbability[i];

	uint32_t times[6] = {5, 9, 15, 19, 23, 30};
	uint32_t randWeight = rand() % TotalWeight;
	uint32_t tempweightsize = 0;

	for (int i = 0; i < 6; ++i)
	{
		tempweightsize += m_TimeMinuteProbability[i];
		if (randWeight <= tempweightsize)
		{
			m_AndroidExitTimes = times[i];
			break;
		}
	}

	return true;
}

void CAndroidUserItemSink::OnTimerJettonOld()
{
	//金币不足50块
	m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
	if ((m_StartsScore < 50 * COIN_RATE) || (m_pIAndroidUserItem->GetUserStatus() == sOffline))
	{
		// m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
		return;
	}
	
	int64_t dwNow = (int64_t)time(NULL);
	int64_t UseTime = dwNow - m_dwStartBetTime;

	// LOG(WARNING) << "-------" << UseTime << " "<< dwNow << " " << m_dwStartBetTime << " " << m_Endtimer << " " << __FUNCTION__;

	float ftime = 0.5 + STD::Random(0.1f, 0.8f).randFloat_mt();
	m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(ftime, CALLBACK_0(CAndroidUserItemSink::OnTimerJettonOld, this));

	// 超过押分时长则不能押
	if (UseTime >= m_Endtimer) return ; 

	// 押分最后3秒
	if (UseTime < (m_Endtimer - 3))
	{
		if (m_Lasttimer == 0 && UseTime < m_kaishijiange){
			return ;
		}

		if (m_Lasttimer == 0 && UseTime >= m_kaishijiange)
		{
			m_Lasttimer = dwNow;
			OnSubAddScore();
			return ;
		}
		
		if (dwNow - m_Lasttimer >= m_jiangeTime)
		{
			m_Lasttimer = dwNow;
			OnSubAddScore();
		}
	}
	else
	{
		if (rand() % 100 <= m_End3s)
		{
			m_Lasttimer = dwNow;
			OnSubAddScore();
		}
	}
}

void CAndroidUserItemSink::OnTimerJetton()
{
	// int64_t wGold = m_pIAndroidUserItem->GetUserScore();

	// LOG(WARNING) << "----机器人押分---" << m_pIAndroidUserItem->GetUserId() << " " << wGold << " " << m_pIAndroidUserItem->GetUserStatus() << " " << __FUNCTION__;

	// //金币不足50块
	// if ((wGold < 50 * COIN_RATE) || (m_pIAndroidUserItem->GetUserStatus() == sOffline))
	// {
	// 	m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
	// 	return;
	// }

	// struct timeval tv;
	// gettimeofday(&tv, NULL);
	// time_t NowTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	// if (NowTime - m_TimerStart > m_Endtimer * 1000)
	// {
	// 	return;
	// }

	// if (NowTime - m_TimerStart < (m_Endtimer - 3) * 1000) //last 3s
	// {
	// 	int kaiShiJianGe = rand() % 500 + m_StartIntervalInit;
	// 	if (m_Lasttimer == 0)
	// 	{
	// 		if (NowTime - m_TimerStart < kaiShiJianGe)
	// 			return;
	// 		else
	// 		{
	// 			m_Lasttimer = NowTime;
	// 			OnSubAddScore();
	// 			return;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		int jianGeTime = rand() % m_StartIntervalInit + 500;
	// 		if (NowTime - m_Lasttimer >= jianGeTime)
	// 		{
	// 			m_Lasttimer = NowTime;
	// 			OnSubAddScore();
	// 			return;
	// 		}
	// 	}
	// }
	// else
	// {
	// 	if (rand() % 100 <= m_End3s)
	// 	{
	// 		m_Lasttimer = NowTime;
	// 		OnSubAddScore();
	// 		return;
	// 	}
	// }
}

void CAndroidUserItemSink::OnTimerGameEnd()
{
    //LOG(WARNING) << "----押分结束---" << __FUNCTION__;

	for (int i = 0; i < BET_ITEM_COUNT; i++)
	{
		m_CurrentRobotAddGold[i] = 0;
	}

	m_SendArea.clear();
	m_lianxuxiazhuCount = 0;
	count = 0;
	//判断是否需要离开
	ExitGame();
	return;
}

//用户加注
bool CAndroidUserItemSink::OnSubAddScore()
{
	//可下注金额 
	int64_t wAddGold = m_StartsScore - 50 * COIN_RATE;
	//随机下注区域
	int32_t bJettonArea = RandArea();   

    if(m_isShensuanzi)
    {
        if(bJettonArea<2)
        {
            bJettonArea = m_iDaxiao;
        }
        else if(bJettonArea>1&&bJettonArea<4)
        {
            bJettonArea = m_iDanshuang;
        }
    }
	//随机可下注金额
	int32_t canGold = RandAddProbability(); 

	//随机下注筹码
	int64_t JettonScore = 0L;
	if (m_lianxuxiazhuCount == 0L){
		// JettonScore = RandChouMa(canGold) * COIN_RATE;
		JettonScore = canGold;// * COIN_RATE;
		m_JettonScore = JettonScore;
		m_lianxuxiazhuCount = rand() % 3 + 2;
	}
	else{
		JettonScore = m_JettonScore;
		m_lianxuxiazhuCount--;
	}
  
	// 区域限制(机器人不能下满)
	if ( (m_CurrentRobotAddGold[bJettonArea] + JettonScore) >= m_AreaRobotAddGold[bJettonArea]* COIN_RATE){
		return false;
	}

    //LOG(WARNING) << "----机器人随机下注金额---" << JettonScore << " " << bJettonArea;

	CMD_C_PlaceJet mPlaceJet;
	mPlaceJet.set_cbjettonarea(bJettonArea);
	mPlaceJet.set_ljettonscore(JettonScore);

	string content = mPlaceJet.SerializeAsString();
	m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(), SUB_C_USER_JETTON, (uint8_t *)content.data(), content.size());

}

//随机下注区域
int CAndroidUserItemSink::RandArea()
{
	int32_t TotalWeight = 0,nIndex = 0;
	for (int i = 0; i < BET_ITEM_COUNT; ++i)
		TotalWeight += m_AreaProbability[i];

	int tempweightsize = Random(0, TotalWeight - 1);
	for (size_t i = 0; i < BET_ITEM_COUNT; i++)
	{
		tempweightsize -= m_AreaProbability[i];
		if (tempweightsize < 0)
		{
			nIndex = i;
			break;
		}
	}
    return nIndex;
}

//随机下注筹码
int64_t CAndroidUserItemSink::RandChouMa(int64_t CanxiazuGold)
{
    //LOG(WARNING) << "----机器人随机下注筹码---" << __FUNCTION__;

	//判断最大下注筹码
#if 0
	int64_t Maxchouma = 1;
	int xb = 0;
	if (CanxiazuGold <= m_AddMaxGold[0])
	{
		Maxchouma = 1;
		xb = 0;
	}

	else if (CanxiazuGold > m_AddMaxGold[0] && CanxiazuGold <= m_AddMaxGold[1])
	{
		Maxchouma = 10;
		xb = 1;
	}
	else if (CanxiazuGold > m_AddMaxGold[1] && CanxiazuGold <= m_AddMaxGold[2])
	{
		Maxchouma = 50;
		xb = 2;
	}
	else if (CanxiazuGold > m_AddMaxGold[2] && CanxiazuGold <= m_AddMaxGold[3])
	{
		Maxchouma = 100;
		xb = 3;
	}
	else if (CanxiazuGold > m_AddMaxGold[3])
	{
		Maxchouma = 500;
		xb = 4;
	}

	int ChouMaWeight = 0;
	for (int i = 0; i <= xb; i++)
		ChouMaWeight += m_ChouMaWeight[i];

	int randWeight = srandomint % ChouMaWeight;
	int tempweightsize = 0;
	for (int i = 0; i <= xb; i++)
	{
		tempweightsize += m_ChouMaWeight[i];
		if (randWeight <= tempweightsize)
		{
			if (i == 0)
				return 1;
			else if (i == 1)
				return 10;
			else if (i == 2)
				return 50;
			else if (i == 3)
				return 100;
			else if (i == 4)
				return 500;
		}
	}
#endif

    int i = 6 - 1;
	for (; i >= 0; --i)
	{
        if (m_currentChips[i] <= CanxiazuGold)
		{
            return m_currentChips[i];
		}
	}
    return m_currentChips[0];
}

//随机可下注金额
int CAndroidUserItemSink::RandAddProbability()
{
	int32_t TotalWeight = 0;
	for (int i = 0; i < 5; ++i)
		TotalWeight += m_ProbabilityWeight[i];

	int32_t randWeight = rand() % TotalWeight;
	int32_t tempweightsize = 0;
	for (int i = 0; i < 5; ++i)
	{
		tempweightsize += m_ProbabilityWeight[i];
		if (randWeight <= tempweightsize)
		{
            return m_currentChips[i];
			// if (i == 0)
			// 	return 2;
			// else if (i == 1)
			// 	return 10;
			// else if (i == 2)
			// 	return 100;
			// else if (i == 3)
			// 	return 150;
			// else if (i == 4)
			// 	return 250;
			// else if (i == 5)
			// 	return 300;
		}
	}
}

//游戏消息(服务器发来)
bool CAndroidUserItemSink::OnGameMessage(uint8_t subId, const uint8_t *pData, uint32_t size)
{
	if(!m_pIAndroidUserItem) {
        //LOG(ERROR) << "--机器人实例为空---" << (int)subId << __FUNCTION__;
		return false;
	}

	int64_t userId = m_pIAndroidUserItem->GetUserId();
	// LOG(WARNING) << "---cmd" << (int)subId << ", user Id:" << userId;

	switch (subId)
	{

		case SUB_S_GameStart:
		case SUB_S_GameEnd:
		{
			return (OnEventSceneMessage(subId, pData, size));
		}
		break;
		case SUB_S_JETTON_SUCCESS: //下注状态
		{
			CMD_S_PlaceJetSuccess JetSuccess;
			if (!JetSuccess.ParseFromArray(pData, size))
				return false;

			// 下注成功
			if (JetSuccess.dwuserid() == userId){
				// m_StartsScore -= JetSuccess.ljettonscore();

				for (int i = 0;i < JetSuccess.alljettonscore_size(); i++)
				{
					m_CurrentRobotAddGold[i] = JetSuccess.alljettonscore(i);
				}
				
                //LOG_ERROR << "------下注成功:" << JetSuccess.ljettonscore() << ", dwUserID:" << userId;
#if 1
			//测试打印
			char szFile[256] = {0};
			char szTemp[64] = {0};
			for (int Index = 0; Index < JetSuccess.alljettonscore_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", JetSuccess.alljettonscore(Index));
				strcat(szFile, szTemp);
			}
            //LOG(WARNING) << szFile;
			strcpy(szFile, "");

			for (int Index = 0; Index < JetSuccess.selfjettonscore_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", JetSuccess.selfjettonscore(Index));
				strcat(szFile, szTemp);
			}
            //LOG(WARNING) << szFile;
#endif

			}
			return true;
		}
		break;
        case SUB_S_SCENE_START: //场景消息
        {
            CMD_S_Scene_GameStart GameStart;
            if (!GameStart.ParseFromArray(pData,size))
                return false;

            const ::google::protobuf::RepeatedField<int64_t>& chips = GameStart.betitems();
            for(int i=0;i<6;i++)
            {
                m_currentChips[i]=chips[i];
            }
            break;
        }
		case SUB_S_JETTON_FAIL: //下注状态
		{
            //LOG_ERROR << "------机器人下注失败:, dwUserID:" << userId;
			return true;
		}
        case SUB_S_QUERY_GAMERECORD: //下注状态
        {
            //LOG_ERROR << "------机器人下注失败:, dwUserID:" << userId;
            return true;
        }
		break;
		default:
            //LOG_ERROR << "------其它不可知游戏消息 wSubCmdID:" << (int)subId << ", dwUserID:" << userId;
			return true;
	}

	return true;
}

//游戏消息
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t *pData, uint32_t size)
{
	switch (cbGameStatus)
	{
		case SUB_S_SCENE_START: //游戏状态
		{
            //LOG(WARNING)<< "------场景消息------>";
			return true;
		}
		case SUB_S_GameStart: //游戏状态
		{
			m_TimerLoopThread->getLoop()->cancel(m_TimerEnd);

			m_GameRound++;
//			if ( m_GameRound >= 10 ){
                ReadConfigInformation();
//				m_GameRound = 0;
//			}



			//效验数据
			CMD_S_GameStart pBetMsg;
			if (!pBetMsg.ParseFromArray(pData, size))
			{
				LOG_ERROR << "------押分消息解析出错-----" << (int)cbGameStatus;
				return false;
			}

            //LOG(WARNING) << "------开始押分-----" <<"总时间 " << pBetMsg.cbplacetime() << " 过去的时间 " << pBetMsg.cbtimeleave() << " 我的分数 " << pBetMsg.luserscore() << " 桌子人数 " << pBetMsg.onlinenum();
	#if 1
//			//测试打印
//			char szFile[256] = {0};
//			char szTemp[64] = {0};
//			//测试打印
//			for (int Index = 0; Index < pBetMsg.mutical_size(); Index++)
//			{
//                snprintf(szTemp, sizeof(szTemp), "%d ", 0);
//				strcat(szFile, szTemp);
//				// if(Index % 5 == 4){ // 	LOG(WARNING) <<szFile; // 	strcpy(szFile,""); // }
//			}
//			LOG(WARNING) << szFile;
//			strcpy(szFile, "");

//			for (int Index = 0; Index < pBetMsg.lalljettonscore_size(); Index++)
//			{
//				snprintf(szTemp, sizeof(szTemp), "%d ", pBetMsg.lalljettonscore(Index));
//				strcat(szFile, szTemp);
//			}
//			LOG(WARNING) << szFile;
//			strcpy(szFile, "");

//			for (int Index = 0; Index < pBetMsg.selfjettonscore_size(); Index++)
//			{
//				snprintf(szTemp, sizeof(szTemp), "%d ", pBetMsg.selfjettonscore(Index));
//				strcat(szFile, szTemp);
//			}
//			LOG(WARNING) << szFile;
    #endif

            PlayerListItem bestUser;
            if(pBetMsg.players_size()>0)
            {
                bestUser = pBetMsg.players(0);

                if(m_pIAndroidUserItem->GetUserId()==bestUser.dwuserid())
                {
                    m_isShensuanzi=true;
                    m_iDaxiao = rand()%2;
                    m_iDanshuang =rand()%2+2;
                }
            }



			m_StartsScore = m_pIAndroidUserItem->GetUserScore();

			m_dwStartBetTime = (int64_t)time(NULL);
			m_kaishijiange = 4 + (rand() % m_kaishijiangeInit) / 1000;
			m_jiangeTime = 1 + (rand() % m_jiangeTimeInit + 500) / 1000;
			m_Lasttimer = 0L;
			m_Endtimer = m_Jetton - pBetMsg.cbtimeleave();

            //LOG(WARNING) <<" Endtimer "<<m_Endtimer<< " Android User Score " << m_StartsScore << " " << pBetMsg.luserscore();
			//设置定时器
			struct timeval tv;
			gettimeofday(&tv, NULL);
			m_TimerStart = tv.tv_sec * 1000 + tv.tv_usec / 1000;

			// m_TimerJetton = m_TimerLoopThread->getLoop()->runEvery(1.0f, CALLBACK_0(CAndroidUserItemSink::OnTimerJettonOld, this));
            m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(5.0f, CALLBACK_0(CAndroidUserItemSink::OnTimerJettonOld, this));
			return true;
		}
		case SUB_S_GameEnd: //游戏状态
		{
			// LOG(WARNING) << "------结束押分------" << (int)cbGameStatus;
			m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);  
			m_TimerEnd = m_TimerLoopThread->getLoop()->runAfter(0.1f, CALLBACK_0(CAndroidUserItemSink::OnTimerGameEnd, this));
			return true;
		}
	}
	
	return false;
}

bool CAndroidUserItemSink::CheckExitGame()
{
	//机器人结算后携带金币小于55自动退场
	int64_t userScore = m_pIAndroidUserItem->GetUserScore();
	//    LOG(INFO) << "User Score : " << (int)userScore << " ,High Line : " << (int)m_strategy->exitHighScore <<", Low Line : "<< (int)m_strategy->exitLowScore;
	return userScore >= m_strategy->exitHighScore || userScore <= m_strategy->exitLowScore;
}

bool CAndroidUserItemSink::ExitGame()
{
	//判断是否需要离开
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (m_jionTime == 0)
	{
		m_jionTime = tv.tv_sec;
		return true;
	}

	if(CheckExitGame()){
		LOG(ERROR) <<" 设置离线 CheckExitGame "<< m_pIAndroidUserItem->GetUserScore() << " "<< m_strategy->exitHighScore << " " << m_strategy->exitLowScore;
		m_pIAndroidUserItem->setOffline();
		m_pIAndroidUserItem->setTrustee(true);
		m_jionTime = 0;
		m_MaxWinTime = 0;
		return true;
	}
	if((m_pIAndroidUserItem->GetUserScore()  < 55 * COIN_RATE)){
		LOG(ERROR) <<" 设置离线 < 55 * COIN_RATE "<< m_pIAndroidUserItem->GetUserScore() << " "<< m_strategy->exitHighScore << " " << m_strategy->exitLowScore;
		m_pIAndroidUserItem->setOffline();
		m_pIAndroidUserItem->setTrustee(true);
		m_jionTime = 0;
		m_MaxWinTime = 0;
		return true;
	}
	int randTime = rand() % 100;
	int PlayTime = tv.tv_sec / 60 - m_jionTime / 60;
	if ((PlayTime == 5 && randTime < m_TimeMinuteProbability[0]) ||
		(PlayTime == 7 && randTime < m_TimeMinuteProbability[1]) ||
		(PlayTime == 9 && randTime < m_TimeMinuteProbability[2]) ||
		(PlayTime == 11 && randTime < m_TimeMinuteProbability[3]) ||
		(PlayTime == 13 && randTime < m_TimeMinuteProbability[4]) ||
		(PlayTime == 15 && randTime < m_TimeMinuteProbability[5]) ||
		(m_MaxWinTime >= 10)
        )
	{
		LOG(ERROR) <<" 设置离线 PlayTime "<<PlayTime << " " << m_pIAndroidUserItem->GetUserId() << " "<< m_pIAndroidUserItem->GetUserScore();

		// 设置离线
        // m_pIAndroidUserItem->SetUserStatus(5);//sOffLine);
		m_pIAndroidUserItem->setOffline();
		m_pIAndroidUserItem->setTrustee(true);
		m_jionTime = 0;
		m_MaxWinTime = 0;
		return true;
	}

	return true;
}

//组件创建函数
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
	shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
	shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
	return pIAndroidSink;
}

// reset all the data now.
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink> &pSink)
{
	if (pSink)
	{
		pSink->RepositionSink();
	}
	pSink.reset();
}
