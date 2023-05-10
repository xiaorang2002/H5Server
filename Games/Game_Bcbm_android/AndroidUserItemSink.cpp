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

#include "proto/BenCiBaoMa.Message.pb.h"

#include "AndroidUserItemSink.h"

using namespace BenCiBaoMa;
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
	m_pIAndroidUserItem = NULL;
	m_jionTime = 0;
	m_Lasttimer = 0;
	m_Endtimer= 0;
	m_StartsScore = 0;
	srand((unsigned)time(NULL));
	m_lianxuxiazhuCount = 0;
	m_GameRound = 0;

    memset(m_AreaRobotAddGold,0,sizeof m_AreaRobotAddGold);
	m_AreaRobotAddGold[0] = 1500;//小大众区域
	m_AreaRobotAddGold[1] = 700;//大大众区域
	m_AreaRobotAddGold[2] = 1500;//小奔驰区域
	m_AreaRobotAddGold[3] = 500;//大奔驰区域
	m_AreaRobotAddGold[4] = 1500;//小宝马区域
	m_AreaRobotAddGold[5] = 350;//大宝马区域
	m_AreaRobotAddGold[6] = 1500;//小保时捷区域
	m_AreaRobotAddGold[7] = 200;//大保时捷区域
	for (int i = 0; i < BET_ITEM_COUNT; i++)
		m_CurrentRobotAddGold[i] = 0;
    m_currentChips[0]=100;
    m_currentChips[1]=200;
    m_currentChips[2]=500;
    m_currentChips[3]=1000;
    m_currentChips[4]=5000;
    m_currentChips[5]=10000;
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
		// ReadConfigInformation();

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
	LOG(WARNING) << "------init android-------" << __FUNCTION__;

	if (pTableFrame)
	{
		m_pITableFrame = pTableFrame;
		m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
		m_TimerLoopThread = m_pITableFrame->GetLoopThread();
		if (m_pGameRoomInfo)
		{
			m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
			LOG(ERROR) <<"m_strRoomName = " << m_strRoomName;
		}

		for (int i = 0; i < m_pGameRoomInfo->jettons.size(); ++i)
		{
			LOG(ERROR) <<"jettons = " << i << " " <<m_pGameRoomInfo->jettons[i];
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
	LOG(WARNING) << "------读取配置------" << __FUNCTION__;
	//设置文件名
	if (!boost::filesystem::exists("./conf/bcbm_config.ini"))
	{
		LOG(WARNING) << "conf bcbm_config.ini not exists";
		return false;
	}

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/bcbm_config.ini", pt);
	// 区域下注权重
	m_AreaProbability[0] = pt.get<int32_t>("ANDROID_NEWCONFIG.BaoshiAAreaprobability", 750);
	m_AreaProbability[1] = pt.get<int32_t>("ANDROID_NEWCONFIG.BaoshiBAreaprobability", 500);
	m_AreaProbability[2] = pt.get<int32_t>("ANDROID_NEWCONFIG.BaomaAAreaprobability", 750);
	m_AreaProbability[3] = pt.get<int32_t>("ANDROID_NEWCONFIG.BaomaBAreaprobability", 300);
	m_AreaProbability[4] = pt.get<int32_t>("ANDROID_NEWCONFIG.BenchiAAreaprobability", 750);
	m_AreaProbability[5] = pt.get<int32_t>("ANDROID_NEWCONFIG.BenchiBAreaprobability", 250);
	m_AreaProbability[6] = pt.get<int32_t>("ANDROID_NEWCONFIG.DazhongAAreaprobability", 750);
	m_AreaProbability[7] = pt.get<int32_t>("ANDROID_NEWCONFIG.DazhongBAreaprobability", 160);

	// 可下注百分比权重
	m_ProbabilityWeight[0] = pt.get<int32_t>("ANDROID_NEWCONFIG.1robability", 20);
	m_ProbabilityWeight[1] = pt.get<int32_t>("ANDROID_NEWCONFIG.5robability", 30);
	m_ProbabilityWeight[2] = pt.get<int32_t>("ANDROID_NEWCONFIG.10robability", 40);
	m_ProbabilityWeight[3] = pt.get<int32_t>("ANDROID_NEWCONFIG.15robability", 30);
	m_ProbabilityWeight[4] = pt.get<int32_t>("ANDROID_NEWCONFIG.25robability", 10);
	m_ProbabilityWeight[5] = pt.get<int32_t>("ANDROID_NEWCONFIG.50robability", 10);

	// 最大下注筹码
	m_AddMaxGold[0] = pt.get<int32_t>("ANDROID_NEWCONFIG.200AddMaxGold", 0);
	m_AddMaxGold[1] = pt.get<int32_t>("ANDROID_NEWCONFIG.500AddMaxGold", 0);
	m_AddMaxGold[2] = pt.get<int32_t>("ANDROID_NEWCONFIG.2000AddMaxGold", 0);
	m_AddMaxGold[3] = pt.get<int32_t>("ANDROID_NEWCONFIG.5000AddMaxGold", 0);
	m_AddMaxGold[4] = pt.get<int32_t>("ANDROID_NEWCONFIG.5001AddMaxGold", 0);

	printf("m_AddMaxGold[0]=%d\n", (int)m_AddMaxGold[0]);
	printf("m_AddMaxGold[1]=%d\n", (int)m_AddMaxGold[1]);
	printf("m_AddMaxGold[2]=%d\n", (int)m_AddMaxGold[2]);
	printf("m_AddMaxGold[3]=%d\n", (int)m_AddMaxGold[3]);
	printf("m_AddMaxGold[4]=%d\n", (int)m_AddMaxGold[4]);

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

    m_AreaRobotAddGold[0] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton0", 1500);
    m_AreaRobotAddGold[1] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton1", 700);
    m_AreaRobotAddGold[2] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton2", 1500);
    m_AreaRobotAddGold[3] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton3", 500);
    m_AreaRobotAddGold[4] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton4", 1500);
    m_AreaRobotAddGold[5] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton5", 350);
    m_AreaRobotAddGold[6] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton6", 1500);
    m_AreaRobotAddGold[7] = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MaxJetton7", 200);

	m_Jetton = pt.get<int32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 15);
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

void CAndroidUserItemSink::OnTimerJettonOld(){

	//金币不足50块
	if ((m_StartsScore < 50 * COIN_RATE) || (m_pIAndroidUserItem->GetUserStatus() == sOffline))
	{
		m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
		return;
	}
	
	int64_t dwNow = (int64_t)time(NULL);
	int64_t UseTime = dwNow - m_dwStartBetTime;

	// LOG(WARNING) << "-------" << UseTime << " "<< dwNow << " " << m_dwStartBetTime << " " << m_Endtimer << " " << __FUNCTION__;
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
	int64_t wGold = m_pIAndroidUserItem->GetUserScore();

	LOG(WARNING) << "----机器人押分---" << m_pIAndroidUserItem->GetUserId() << " " << wGold << " " << m_pIAndroidUserItem->GetUserStatus() << " " << __FUNCTION__;

	//金币不足50块
	if ((wGold < 50 * COIN_RATE) || (m_pIAndroidUserItem->GetUserStatus() == sOffline))
	{
		m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
		return;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t NowTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	if (NowTime - m_TimerStart > m_Endtimer * 1000)
	{
		return;
	}

	if (NowTime - m_TimerStart < (m_Endtimer - 3) * 1000) //last 3s
	{
		int kaiShiJianGe = rand() % 500 + m_StartIntervalInit;
		if (m_Lasttimer == 0)
		{
			if (NowTime - m_TimerStart < kaiShiJianGe)
				return;
			else
			{
				m_Lasttimer = NowTime;
				OnSubAddScore();
				return;
			}
		}
		else
		{
			int jianGeTime = rand() % m_StartIntervalInit + 500;
			if (NowTime - m_Lasttimer >= jianGeTime)
			{
				m_Lasttimer = NowTime;
				OnSubAddScore();
				return;
			}
		}
	}
	else
	{
		if (rand() % 100 <= m_End3s)
		{
			m_Lasttimer = NowTime;
			OnSubAddScore();
			return;
		}
	}
}

void CAndroidUserItemSink::OnTimerGameEnd()
{
	// LOG(WARNING) << "----押分结束---" << __FUNCTION__;

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
	//随机可下注金额
	int64_t canGold = RandAddProbability(); 

	//随机下注筹码
	int64_t JettonScore = 0L;
	if (m_lianxuxiazhuCount == 0L){
		JettonScore = canGold;// * COIN_RATE;
		m_JettonScore = JettonScore;
		m_lianxuxiazhuCount = rand() % 3 + 2;
	}
	else{
		JettonScore = m_JettonScore;
		m_lianxuxiazhuCount--;
	}

	// LOG(WARNING) << "----机器人随机下注金额---" << JettonScore << " " << bJettonArea;
  
	// 区域限制
	if ( (m_CurrentRobotAddGold[bJettonArea] + JettonScore) >= m_AreaRobotAddGold[bJettonArea]* COIN_RATE)
		return false;

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
	LOG(WARNING) << "----机器人随机下注筹码---" << __FUNCTION__;
  
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
int64_t CAndroidUserItemSink::RandAddProbability()
{
	int32_t TotalWeight = 0;
    for (int i = 0; i < 6; ++i)
		TotalWeight += m_ProbabilityWeight[i];

	int32_t randWeight = rand() % TotalWeight;
	int32_t tempweightsize = 0;
    for (int i = 0; i < 6; ++i)
	{
		tempweightsize += m_ProbabilityWeight[i];
		if (randWeight <= tempweightsize)
		{
            return m_currentChips[i];
		}
	}
}

//游戏消息(服务器发来)
bool CAndroidUserItemSink::OnGameMessage(uint8_t subId, const uint8_t *pData, uint32_t size)
{
	if(!m_pIAndroidUserItem) {
		LOG(ERROR) << "--机器人实例为空---" << (int)subId << __FUNCTION__;
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
				
				// LOG_ERROR << "------下注成功:" << JetSuccess.ljettonscore() << ", dwUserID:" << userId;
#if 0
			//测试打印
			char szFile[256] = {0};
			char szTemp[64] = {0};
			for (int Index = 0; Index < JetSuccess.alljettonscore_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", JetSuccess.alljettonscore(Index));
				strcat(szFile, szTemp);
			}
			LOG(WARNING) << szFile;
			strcpy(szFile, "");

			for (int Index = 0; Index < JetSuccess.selfjettonscore_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", JetSuccess.selfjettonscore(Index));
				strcat(szFile, szTemp);
			}
			LOG(WARNING) << szFile;
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

            const ::google::protobuf::RepeatedField<int32_t>& chips = GameStart.betitems();
            for(int i=0;i<6;i++)
            {
                m_currentChips[i]=chips[i];
            }
            break;
        }
		case SUB_S_JETTON_FAIL: //下注状态
		{
			// LOG_ERROR << "------机器人下注失败:, dwUserID:" << userId; 
			return true;
		}
		break;
		case SUB_S_JETTON_SUCCESS_OTHER: //广播一段时间内其他所有玩家的下注
		{
			// LOG_ERROR << "------机器人下注失败:, dwUserID:" << userId; 
			return true;
		}
		break;
		default:
			LOG_ERROR << "------其它不可知游戏消息 wSubCmdID:" << (int)subId << ", dwUserID:" << userId;
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
            LOG(WARNING)<< "------场景消息------>";
			return true;
		}
		case SUB_S_GameStart: //游戏状态
		{
			m_TimerLoopThread->getLoop()->cancel(m_TimerEnd);

			m_GameRound++;
			if ( m_GameRound >= 10 ){	
				// ReadConfigInformation();
				m_GameRound = 0;
			}

			//效验数据
			CMD_S_GameStart pBetMsg;
			if (!pBetMsg.ParseFromArray(pData, size))
			{
				LOG_ERROR << "------押分消息解析出错-----" << (int)cbGameStatus;
				return false;
			}

			// LOG(WARNING) << "------开始押分-----" <<"总时间 " << pBetMsg.cbplacetime() << " 过去的时间 " << pBetMsg.cbtimeleave() << " 我的分数 " << pBetMsg.luserscore() << " 桌子人数 " << pBetMsg.onlinenum();
	#if 0
			//测试打印
			char szFile[256] = {0};
			char szTemp[64] = {0};
			//测试打印
			for (int Index = 0; Index < pBetMsg.mutical_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", pBetMsg.mutical(Index));
				strcat(szFile, szTemp);
				// if(Index % 5 == 4){ // 	LOG(WARNING) <<szFile; // 	strcpy(szFile,""); // }
			}
			LOG(WARNING) << szFile;
			strcpy(szFile, "");

			for (int Index = 0; Index < pBetMsg.lalljettonscore_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", pBetMsg.lalljettonscore(Index));
				strcat(szFile, szTemp);
			}
			LOG(WARNING) << szFile;
			strcpy(szFile, "");

			for (int Index = 0; Index < pBetMsg.selfjettonscore_size(); Index++)
			{
				snprintf(szTemp, sizeof(szTemp), "%d ", pBetMsg.selfjettonscore(Index));
				strcat(szFile, szTemp);
			}
			LOG(WARNING) << szFile;
	#endif

			m_StartsScore = m_pIAndroidUserItem->GetUserScore();

			m_dwStartBetTime = (int64_t)time(NULL);
			m_kaishijiange = 3 + (rand() % m_kaishijiangeInit) / 1000;
			m_jiangeTime = 1 + (rand() % m_jiangeTimeInit + 500) / 1000;
			m_Lasttimer = 0L;
			m_Endtimer = m_Jetton - pBetMsg.cbtimeleave();

			// LOG(WARNING) <<" Endtimer "<<m_Endtimer<< " Android User Score " << m_StartsScore << " " << pBetMsg.luserscore();
			//设置定时器
			struct timeval tv;
			gettimeofday(&tv, NULL);
			m_TimerStart = tv.tv_sec * 1000 + tv.tv_usec / 1000;

			m_TimerJetton = m_TimerLoopThread->getLoop()->runEvery(1.0f, CALLBACK_0(CAndroidUserItemSink::OnTimerJettonOld, this));
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
