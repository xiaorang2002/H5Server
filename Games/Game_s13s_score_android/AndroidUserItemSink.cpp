#include <glog/logging.h>

#include <boost/filesystem.hpp>

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

#include "proto/s13s.Message.pb.h"

#include "../QPAlgorithm/s13s.h"
#include "../QPAlgorithm/funcC.h"
#include "../QPAlgorithm/cfg.h"

#include "AndroidUserItemSink.h"

CAndroidUserItemSink::CAndroidUserItemSink()
    : m_pTableFrame(NULL)
    , m_pAndroidUserItem(NULL)
{
    srand((unsigned)time(NULL));
	RepositionSink();
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

//桌子重置
bool CAndroidUserItemSink::RepositionSink()
{
	//确定牌型的真人玩家数
	realPlayerCount = 0;
	//是否重置随机思考时长
	randWaitResetFlag_ = false;
	//累计思考时长
	//totalWaitSeconds_ = 0;
	//分片思考时间
	//sliceWaitSeconds_ = 0.6f;
	//随机思考时长(1.0~18.0s之间)
	//randWaitSeconds_ = CalcWaitSeconds(ThisChairId, true);
    return true;
}

//桌子初始化
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pAndroidUserItem)
{
	if (!pTableFrame || !pAndroidUserItem) {
		return false;
	}
	//桌子指针
    m_pTableFrame = pTableFrame;
	//机器人对象
	m_pAndroidUserItem = pAndroidUserItem;
	//定时器指针
    m_TimerLoopThread = m_pTableFrame->GetLoopThread();
}

//用户指针
bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pAndroidUserItem)
{
	if (!pAndroidUserItem) {
		return false;
	}
	//机器人对象
	m_pAndroidUserItem = pAndroidUserItem;
}

//桌子指针
void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
	if (!pTableFrame) {
		return;
	}
	//桌子指针
	m_pTableFrame = pTableFrame;
	//定时器指针
	m_TimerLoopThread = m_pTableFrame->GetLoopThread();
	//ReadConfigInformation();
}

//消息处理
bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize) {
	switch (wSubCmdID)
	{
	case s13s::SUB_S_ANDROID_CARD: {//机器人牌
		s13s::CMD_S_AndroidCard reqdata;
		if (!reqdata.ParseFromArray(pData, wDataSize)) {
			return false;
		}
		//LOG(ERROR) << "--- *** [" << reqdata.roundid() << "] CAndroidUserItemSink 机器人牌 ...";
		for (int i = 0; i < reqdata.players_size(); ++i) {
			s13s::AndroidPlayer const& player = reqdata.players(i);
			int j = player.chairid();
			//memcpy(&(handCards_[j])[0], player.handcards().cards().data(), MAX_COUNT);
		}
		break;
	}
	case s13s::SUB_S_GAME_START: {//游戏开始
		s13s::CMD_S_GameStart reqdata;
		if (!reqdata.ParseFromArray(pData, wDataSize)) {
			return false;
		}
		//LOG(ERROR) << "--- *** [" << reqdata.roundid() << "] CAndroidUserItemSink 游戏开始 ...";
		//牌局编号
		strRoundID_ = reqdata.roundid();
		//剩余时间
		groupTotalTime_ = reqdata.wtimeleft();
		//设置状态
		//m_pAndroidUserItem->SetUserStatus(sPlaying);
		//确定牌型的真人玩家数
		realPlayerCount = 0;
		//是否重置随机思考时长
		randWaitResetFlag_ = false;
		//累计思考时长
		totalWaitSeconds_ = 0;
		//分片思考时间
		sliceWaitSeconds_ = 0.2f;
		//随机思考时长(1.0~18.0s之间)
		randWaitSeconds_ = (ThisRoomId >= 6305) ?
			CalcWaitSecondsSpeed(ThisChairId, true) :
			CalcWaitSeconds(ThisChairId, true);
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "--- *** [%s] 游戏开始，机器人 %d 随机思考时长(%.2f) ...", strRoundID_.c_str(), ThisChairId, randWaitSeconds_);
		LOG(ERROR) << msg;
		//理牌定时器
		ThisThreadTimer->cancel(timerIdThinking);
		timerIdThinking = ThisThreadTimer->runAfter(sliceWaitSeconds_, boost::bind(&CAndroidUserItemSink::ThinkingTimerOver, this));
		break;
	}
	case s13s::SUB_S_MANUALCARDS: {//手动摆牌
		//LOG(ERROR) << "--- *** [" << strRoundID_ << "] CAndroidUserItemSink 手动摆牌 ...";
		break;
	}
	case s13s::SUB_S_MAKESUREDUNHANDTY: {//确定牌型
		s13s::CMD_S_MakesureDunHandTy reqdata;
		if (!reqdata.ParseFromArray(pData, wDataSize)) {
			return false;
		}
		//座椅玩家有效
		if (!m_pTableFrame->IsExistUser(reqdata.chairid())) {
			return false;
		}
		//机器人
		if (m_pTableFrame->IsAndroidUser(reqdata.chairid())) {
			//LOG(ERROR) << "--- *** [" << strRoundID_ << "] CAndroidUserItemSink 机器人 " << reqdata.chairid() << " 确定牌型 ...";
		}
		else {
			//确定牌型的真人玩家数量
			++realPlayerCount;
			//LOG(ERROR) << "--- *** [" << strRoundID_ << "] CAndroidUserItemSink 玩家 " << reqdata.chairid() << " 确定牌型 ...";
		}
		break;
	}
	case s13s::SUB_S_CANCELCARDS: {//重置墩牌
		//LOG(ERROR) << "--- *** [" << strRoundID_ << "] CAndroidUserItemSink 重置墩牌 ...";
		break;
	}
	case s13s::SUB_S_GAME_END: {//游戏结束
		//LOG(ERROR) << "--- *** [" << strRoundID_ << "] CAndroidUserItemSink 游戏结束 ...";
		s13s::CMD_S_GameEnd GameEnd;
		if (!GameEnd.ParseFromArray(pData, wDataSize)) {
			return false;
		}
		ClearAllTimer();
		RepositionSink();
		break;
	}
	default: break;
	}
	return true;
}

//随机思考时间
double CAndroidUserItemSink::CalcWaitSeconds(int chairId, bool isinit) {
	int j = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (m_pTableFrame->IsAndroidUser(i) && !randWaitResetFlag_) {
			++j;
		}
		if(chairId == i) {
			return isinit ?
				//随机思考时长(1.0~18.0s之间)
				RandomBetween(1, 15) + 0.1f * RandomBetween(5, 10) * 1.5 * (j/* + 1*/):
				//随机思考时长(1.0~3.0s之间)
				RandomBetween(0,  1) + 0.1f * RandomBetween(5, 10) * 1.5 * (j/* + 1*/);
		}
	}
}

//随机思考时间(极速房)
double CAndroidUserItemSink::CalcWaitSecondsSpeed(int chairId, bool isinit) {
	int j = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (m_pTableFrame->IsAndroidUser(i) && !randWaitResetFlag_) {
			++j;
		}
		if (chairId == i) {
			return isinit ?
				//随机思考时长(1.0~18.0s之间)
				RandomBetween(0, 1) + 0.1f * RandomBetween(5, 10) * (j/* + 1*/) :
				//随机思考时长(1.0~3.0s之间)
				/*RandomBetween(0, 1) + */0.1f * RandomBetween(5, 10) * (j/* + 1*/);
		}
	}
}

//思考定时器
void CAndroidUserItemSink::ThinkingTimerOver() {
	char msg[1024] = { 0 };
	//snprintf(msg, sizeof(msg), "--- *** [%s] 机器人 %d 思考中(%.2f) ...", strRoundID_.c_str(), ThisChairId, randWaitSeconds_);
	//LOG(ERROR) << msg;
	ThisThreadTimer->cancel(timerIdThinking);
	totalWaitSeconds_ += sliceWaitSeconds_;
	//思考时长不够
	if (totalWaitSeconds_ < randWaitSeconds_) {
		//所有真人玩家都确定牌型，减少AI思考时间
		if (!randWaitResetFlag_ &&
			realPlayerCount == m_pTableFrame->GetPlayerCount(false)) {
			//随机思考时长(1.0~3.0秒之间)
			resetWaitSeconds_ = (ThisRoomId >= 6305) ?
				CalcWaitSecondsSpeed(ThisChairId, false) :
				CalcWaitSeconds(ThisChairId, false);
			//累计思考时长 + 重置思考时长
			randWaitSeconds_ = totalWaitSeconds_ + resetWaitSeconds_;
			//重置随机思考时长
			randWaitResetFlag_ = true;
			memset(msg, 0, sizeof(msg));
			snprintf(msg, sizeof(msg), "--- *** [%s] 机器人 %d 重置随机思考时长(%.2f) ...", strRoundID_.c_str(), ThisChairId, resetWaitSeconds_);
			LOG(ERROR) << msg;
		}
		//重启思考定时器
		timerIdThinking = ThisThreadTimer->runAfter(sliceWaitSeconds_, boost::bind(&CAndroidUserItemSink::ThinkingTimerOver, this));
	}
	else {
		memset(msg, 0, sizeof(msg));
		snprintf(msg, sizeof(msg), "--- *** [%s] 机器人 %d 理牌完成(%.2f) ...", strRoundID_.c_str(), ThisChairId, totalWaitSeconds_);
		LOG(ERROR) << msg;
		//机器人确定牌型
		s13s::CMD_C_MakesureDunHandTy rspdata;
		rspdata.set_groupindex(0);
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->OnGameEvent(ThisChairId, s13s::SUB_C_MAKESUREDUNHANDTY, (uint8_t *)content.data(), content.size());
	}
}

//清理所有定时器
void CAndroidUserItemSink::ClearAllTimer()
{
	ThisThreadTimer->cancel(timerIdThinking);
}

//CreateAndroidSink 创建实例
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

//DeleteAndroidSink 删除实例
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink>& pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
    pSink.reset();
}

