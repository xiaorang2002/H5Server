#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
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

#include "proto/shz.Message.pb.h"

#include "../Game_SGJ/CMD_Game.h"
#include "../Game_SGJ/GameLogic.h"

using namespace shz;
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
	m_pIAndroidUserItem = NULL;
	m_jionTime = 0;
	m_Lasttimer = 0;
	m_Endtimer= 0;
	m_StartsScore = 0;
	srand((unsigned)time(NULL));
	m_lianxuxiazhuCount = 0;
	m_GameRound = 0;
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
	LOG(WARNING) << "------SetTableFrame-------" << __FUNCTION__;


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

		// for (int i = 0; i < m_pGameRoomInfo->jettons.size(); ++i)
		// {
		// 	LOG(ERROR) <<"jettons = " << i << " " <<m_pGameRoomInfo->jettons[i];
		// }
		
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

	LOG(ERROR) << "------重置接口------" << __FUNCTION__;

	return true;
}

//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{
	LOG(WARNING) << "------读取配置------" << __FUNCTION__;
	//设置文件名
	if (!boost::filesystem::exists("./conf/sgj_config.ini"))
	{
		LOG(WARNING) << "conf sgj_config.ini not exists";
		return false;
	}
	//
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/sgj_config.ini", pt);
 
	// 可下注百分比权重
	int betItemCount = pt.get<int32_t>("FruitConfig.BetItemCount", 9);
	LOG(WARNING) << "------Bet Item Count------" << betItemCount;
	// 筹码权重
	char buf[32] = { 0 };
    for(int i = 0;i< betItemCount;i++){
        sprintf(buf, "ANDROID_CONFIG.ChouMa%d",i);
		m_ChouMaWeight[i] = pt.get<int32_t>(buf, 30);
		// LOG(WARNING) << "------读取配置------" << m_ChouMaWeight[i];
	}

    //机器人随机押注时间(在两者之间)
    m_MaxAndroidBetInterval = pt.get<int32_t>("ANDROID_CONFIG.MaxAndroidBetInterval", 10); 
    m_MinAndroidBetInterval = pt.get<int32_t>("ANDROID_CONFIG.MinAndroidBetInterval", 5); 

	return true;
}
  
void CAndroidUserItemSink::OnTimerGameEnd()
{
	// LOG(WARNING) << "----押分结束---" << __FUNCTION__; 
	m_SendArea.clear();
	m_lianxuxiazhuCount = 0;
	count = 0;
	//判断是否需要离开
	ExitGame();
	return;
}
 
//随机下注筹码
int64_t CAndroidUserItemSink::RandChouMa(int64_t CanxiazuGold)
{
	LOG(WARNING) << "----机器人随机下注筹码---" << __FUNCTION__;
  
	int i = m_pGameRoomInfo->jettons.size() - 1;
	for (; i >= 0; --i)
	{
		if (m_pGameRoomInfo->jettons[i] <= CanxiazuGold)
		{
			return m_pGameRoomInfo->jettons[i];
		}
	}
	return m_pGameRoomInfo->jettons[0];
}
 
//游戏消息(服务器发来)
bool CAndroidUserItemSink::OnGameMessage(uint8_t subId, const uint8_t *pData, uint32_t size)
{
	if(!m_pIAndroidUserItem) {
		LOG(ERROR) << "--机器人实例为空---" << (int)subId << __FUNCTION__;
		return false;
	}

	int64_t userId = m_pIAndroidUserItem->GetUserId();
	// LOG(WARNING) << "---OnGameMessage--" << (int)subId << ", user Id:" << userId;

	switch (subId)
	{
		case SUB_SC_GAMESCENE_FREE: //场景消息
		case SUB_S_NORMAL: //场景消息
		case SUB_S_MARRY:
		case SUB_S_STATUSFREE:
		{
			return (OnEventSceneMessage(subId, pData, size));
		}
		break;  
		default:
			return true;
	}

	return true;
}

//游戏消息
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t *pData, uint32_t size)
{
	switch (cbGameStatus)
	{
		case SUB_SC_GAMESCENE_FREE: //游戏状态
		{
            LOG(WARNING)<< "------场景消息------>";
			m_TimerLoopThread->getLoop()->cancel(m_TimerEnd);
			m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
			//随机拉取时间
			int32_t timeinterval = Random(m_MinAndroidBetInterval,m_MaxAndroidBetInterval);
			m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(timeinterval, CALLBACK_0(CAndroidUserItemSink::CmdRound, this));
			return true;
		}
		case SUB_S_NORMAL: //普通拉霸消息
		{
			//效验数据
			CMD_S_RoundResult pRoundMsg;
			if (!pRoundMsg.ParseFromArray(pData, size)){
				LOG_ERROR << "------普通拉霸消息解析出错-----" << (int)cbGameStatus;
				return false;
			}

			m_StartsScore = m_pIAndroidUserItem->GetUserScore();

			// 解析
            int32_t freeCount = 0;
			int32_t marryCount = pRoundMsg.nmarrynum();
            int32_t njackpot = 0;
			
			m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
			//随机拉取时间
			int32_t timeinterval = Random(m_MinAndroidBetInterval,m_MaxAndroidBetInterval);
			if(marryCount > 0 ){
				m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(timeinterval, CALLBACK_0(CAndroidUserItemSink::CmdMarry, this));
				LOG(WARNING) << "拉霸出玛丽,freeCount:" << freeCount<< " marryCount:" << marryCount<< " njackpot:" << njackpot <<" "<<m_StartsScore;
			}
			else{ 
				m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(timeinterval, CALLBACK_0(CAndroidUserItemSink::CmdRound, this));
			}
	
			m_GameRound++;
			if ( m_GameRound >= 30 ){	
				ReadConfigInformation();
				m_GameRound  = 0;
			}

			return true;
		}
		case SUB_S_MARRY: //开玛丽机奖
		{
			//效验数据
			CMD_S_MarryResult pMarryMsg;
			if (!pMarryMsg.ParseFromArray(pData, size)){
				LOG_ERROR << "------开玛丽机奖消息解析出错-----" << (int)cbGameStatus;
				return false;
			}
			// 
			m_TimerLoopThread->getLoop()->cancel(m_TimerJetton);
			//随机拉取时间
			int32_t timeinterval = Random(m_MinAndroidBetInterval,m_MaxAndroidBetInterval);
			int32_t marryCount = pMarryMsg.nmarryleft();
			int32_t nMarryNum = pMarryMsg.nmarrynum();
			if(marryCount > 0 ){
				m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(timeinterval, CALLBACK_0(CAndroidUserItemSink::CmdMarry, this));
			}
			else{
				// timeinterval =  1.0f * (rand()%5 + 1);
				m_TimerJetton = m_TimerLoopThread->getLoop()->runAfter(timeinterval, CALLBACK_0(CAndroidUserItemSink::CmdRound, this));
			}

			LOG(WARNING) << "------开玛丽机奖------" << marryCount <<" "<< nMarryNum;
			return true;
		}
		default:
			LOG(INFO) << "------开玛丽机奖------" << cbGameStatus;
		break;
	}
	
	return false;
}

bool CAndroidUserItemSink::CmdMarry()
{
	m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(), SUB_C_MARRY, nullptr, 0);
	LOG(WARNING) << "------CmdMarry------"<<m_pIAndroidUserItem->GetUserId() <<" UserScore "<< m_pIAndroidUserItem->GetUserScore();
	return true;
}

bool CAndroidUserItemSink::CmdRound()
{
	// 根据筹码权重
	int32_t itemCount = m_pGameRoomInfo->jettons.size();
	int32_t idx = RandomChouma(itemCount,m_ChouMaWeight); 
	int32_t lbetscore = 90;
	if (idx < itemCount){
		lbetscore = m_pGameRoomInfo->jettons[idx] * 9;
	}

	//判断是否需要离开
	if (CheckExitGame()){
		m_pIAndroidUserItem->setOffline();
		LOG(ERROR) << " 设置机器人离线 " << m_pIAndroidUserItem->GetUserId() << " " << m_pIAndroidUserItem->GetUserScore();
		return false;
	}

	//如果机器人分数不够
	if(m_pIAndroidUserItem->GetUserScore() < lbetscore ){
		ExitGame();
		return false;
	}

	CMD_C_GameStart pGameStart;
	pGameStart.set_llbetscore(lbetscore);
	string content = pGameStart.SerializeAsString();
	m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(), SUB_C_START, (uint8_t *)content.data(), content.size());
	LOG(WARNING) << "------CmdRound------"<<idx <<" "<< lbetscore << " "<< m_pIAndroidUserItem->GetUserId() <<"  UserScore "<< m_pIAndroidUserItem->GetUserScore();
	return true;
}


//按权重获取下注筹码
int CAndroidUserItemSink::RandomChouma(int nLen,int32_t nArray[])
{
    int nIndex = 0;
    int nACTotal = 0;
    for (int i = 0; i < nLen; i++)
    {
        nACTotal += nArray[i]; 
    }
 
    assert(nACTotal > 0);

    if (nACTotal > 0)
    {
        int nRandNum = Random(0, nACTotal - 1);
        for (size_t i = 0; i < nLen; i++)
        {
            nRandNum -= nArray[i];
            if (nRandNum < 0)
            {
                nIndex = i;
                break;
            }
        }
    }
    else {
        nIndex = Random(0, nLen - 1);//rand() % nLen;
    }
    
    return nIndex;
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
	LOG(ERROR) << " 设置机器人离线 " << m_pIAndroidUserItem->GetUserId() << " " << m_pIAndroidUserItem->GetUserScore();

	// 设置离线
	// m_pIAndroidUserItem->SetUserStatus(5);//sOffLine);
	m_pIAndroidUserItem->setOffline();
	m_pIAndroidUserItem->setTrustee(true);
	m_jionTime = 0;
	m_MaxWinTime = 0;

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
