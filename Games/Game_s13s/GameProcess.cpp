#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

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
#include "proto/s13s.Message.pb.h"

#include "../QPAlgorithm/s13s.h"
#include "../QPAlgorithm/funcC.h"
#include "../QPAlgorithm/cfg.h"

#include "GameProcess.h"

class CGLogInit
{
public:
	CGLogInit()
	{
		string dir = "./log/s13s/";
		// initialize the glog content.
		FLAGS_colorlogtostderr = true;
		FLAGS_minloglevel = google::GLOG_INFO;
		FLAGS_log_dir = dir;
		FLAGS_logbufsecs = 3;

		if (!boost::filesystem::exists(dir))
		{
			boost::filesystem::create_directories(dir);
		}

		// set std output special log content value.
		google::SetStderrLogging(FLAGS_minloglevel);
		// initialize the log prefix name now.
		google::InitGoogleLogging("s13s");

		//设置文件名
		//=====config=====
		if (!boost::filesystem::exists(INI_FILENAME))
		{
			LOG_INFO << INI_FILENAME << " not exists";
			return;
		}

		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(INI_FILENAME, pt);
		int errorlevel = pt.get<int>("Global.errorlevel", -1);
		if (errorlevel >= 0)
		{
			FLAGS_minloglevel = errorlevel;
		}

		int printlevel = pt.get<int>("Global.printlevel", -1);
		if (printlevel >= 0)
		{
			// set std output special log content.
			google::SetStderrLogging(printlevel);
		}
	}

	virtual ~CGLogInit()
	{
		// google::ShutdownGoogleLogging();
	}
};

CGLogInit glog_init;

CGameProcess::CGameProcess(void)
	:storageInfo_({ 0 })
{

    stockWeak = 0.0;
	groupStartTime_ = 0;
    groupEndTime_ = 0;
	groupTotalTime_ = 0;

	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//超时匹配时长(可配置)
	timeoutMatchSeconds_ = 2.6f;
	//机器人候补空位超时时长(可配置)
	timeoutAddAndroidSeconds_ = 0.4f;
	//放大倍数
	ratioScale_ = 1000;
	//5/4/3/2/单人局的概率
	ratio5_ = 0, ratio4_ = 100, ratio3_ = 0, ratio2_ = 0, ratio1_ = 0;
	//初始化桌子游戏人数概率权重
	initPlayerRatioWeight();
	MaxGamePlayers_ = 0;

	//默认换牌概率
	ratioSwap_ = 100;

	//系统输了换牌概率 80%
	ratioSwapLost_ = 80;
	//系统赢了换牌概率 70%
	ratioSwapWin_ = 70;
	
	//IP访问限制
	checkIp_ = false;
	//一张桌子真实玩家数量限制
	checkrealuser_ = false;
	//是否开始比牌
	isCompareCards_ = false;
	memset(userChooseCards_, 0, MAX_COUNT * sizeof(uint8_t));
}

CGameProcess::~CGameProcess(void)
{
}

//初始化桌子游戏人数概率权重
void CGameProcess::initPlayerRatioWeight() {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (i == 4) {
			//5人局的概率0
			ratioGamePlayerWeight_[i] = ratio5_ * ratioScale_;
		}
		else if (i == 3) {
			//4人局的概率10%
			ratioGamePlayerWeight_[i] = ratio4_ * ratioScale_;
		}
		else if (i == 2) {
			//3人局的概率10%
			ratioGamePlayerWeight_[i] = ratio3_ * ratioScale_;
		}
		else if (i == 1) {
			//2人局的概率80%
			ratioGamePlayerWeight_[i] = ratio2_ * ratioScale_;
		}
		else if (i == 0) {
			//单人局的概率0
			ratioGamePlayerWeight_[i] = ratio1_ * ratioScale_;
		}
		else {
			ratioGamePlayerWeight_[i] = 0;
		}
	}
	//计算桌子要求标准游戏人数
	poolGamePlayerRatio_.init(ratioGamePlayerWeight_, GAME_PLAYER);
}

//设置指针
bool CGameProcess::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
	//桌子指针
	m_pTableFrame = pTableFrame; assert(m_pTableFrame);
	//定时器指针
	m_TimerLoopThread = m_pTableFrame->GetLoopThread();
	//初始化游戏数据
    InitGameData();
	//读取配置
    ReadStorageInformation();
	//初始化桌子游戏人数概率权重
	initPlayerRatioWeight();
	//放大倍数
	int scale = 1000;
	//默认换牌概率
	int dw[MaxExchange] = {
		ratioSwap_*scale,
		(100 - ratioSwap_)*scale };
	sysSwapCardsRatio_.init(dw, MaxExchange);
	//系统输了换牌概率 80%
	int lw[MaxExchange] = {
		ratioSwapLost_*scale,
		(100 - ratioSwapLost_)*scale };
	sysLostSwapCardsRatio_.init(lw, MaxExchange);
	//系统赢了换牌概率 70%
	int ww[MaxExchange] = {
		ratioSwapWin_*scale,
		(100 - ratioSwapWin_)*scale };
	sysWinSwapCardsRatio_.init(ww, MaxExchange);

	m_replay.cellscore = CellScore;//房间底注
	m_replay.roomname = ThisRoomName;
	//m_replay.gameid = ThisGameId;//游戏类型
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    return true;
}

//清理游戏数据
void CGameProcess::ClearGameData()
{

}

//初始化游戏数据
void CGameProcess::InitGameData()
{
	assert(m_pTableFrame);
	
	//roundStartTime_ = 0;
	//roundEndTime_ = 0;
	
	for (int i = 0; i < GAME_PLAYER; i++) {
		saveUserIDs_[i] = 0;
	}
	groupStartTime_ = 0;
    groupEndTime_ = 0;
	//groupTotalTime_ = 0;

	//计算最低基准积分
	m_pTableFrame->GetGameRoomInfo()->floorScore;
	//计算最高基准积分
	m_pTableFrame->GetGameRoomInfo()->ceilScore;
	//进入房间最小积分
	m_pTableFrame->GetGameRoomInfo()->enterMinScore;
	//进入房间最大积分，玩家超过这个积分，只能进入更高级房间
	m_pTableFrame->GetGameRoomInfo()->enterMaxScore;
	
	//初始化
	g.InitCards();
	//洗牌
	g.ShuffleCards();
	//初始化玩家手牌
	memset(handCards_, 0, GAME_PLAYER * MAX_COUNT * sizeof(uint8_t));
	//确定牌型的玩家数
	selectcount = 0;
	//内部使用游戏状态
	gameStatus_ = GAME_STATUS_GROUP;
	//玩家比牌结果数据
	for (int i = 0; i < GAME_PLAYER; ++i) {
		compareCards_[i].Clear();
	}
	//如果开始比牌返回
	isCompareCards_ = false;
	memset(userChooseCards_, 0, MAX_COUNT * sizeof(uint8_t));
}

//清除所有定时器
void CGameProcess::ClearAllTimer()
{
	ThisThreadTimer->cancel(timerIdGameReadyOver);
	ThisThreadTimer->cancel(timerIdManual_);
	ThisThreadTimer->cancel(timerIdGameEnd_);
}

//用户进入
bool CGameProcess::OnUserEnter(int64_t userId, bool islookon)
{
	//没有初始化
	if (!m_pTableFrame) {
		return false;
	}
	//桌椅玩家无效
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId);// assert(!userItem);
	if (!userItem) {
		return false;
	}
	char msg[1024] = { 0 };
	assert(userItem->GetChairId() >= 0);
	assert(userItem->GetChairId() < GAME_PLAYER);
	snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d] status[%d] OnUserEnter userID[%d] chairID[%d]\n",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), userId, userItem->GetChairId());
	LOG(ERROR) << msg;
	//首个玩家加入桌子，初始化MaxGamePlayers_
	if (m_pTableFrame->GetPlayerCount(true) == 1) {
		if (m_pTableFrame->GetGameStatus() != GAME_STATUS_INIT) {
			snprintf(msg, sizeof(msg), "--- *** tableID[%d] OnUserEnter GameStatus[%d]\n", m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus());
			LOG(ERROR) << msg;
			m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		}
		//一定是游戏初始状态，否则清理有问题
		//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT);
		//首个加入桌子的必须是真实玩家
		//assert(m_pTableFrame->GetPlayerCount(false) == 1);
		//权重随机乱排
		poolGamePlayerRatio_.shuffleSeq();
		//初始化桌子当前要求的游戏人数
		MaxGamePlayers_ = poolGamePlayerRatio_.getResultByWeight() + 1;
		//重置累计时间
		totalMatchSeconds_ = 0;

		LOG(ERROR) << "\n\n--- *** ==============================================================================\n"
				   << "--- *** ==============================================================================";
		snprintf(msg, sizeof(msg), "--- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ tableID[%d]初始化游戏人数 MaxGamePlayers_：%d", m_pTableFrame->GetTableId(), MaxGamePlayers_);
		LOG(ERROR) << msg;
		assert(MaxGamePlayers_ >= MIN_GAME_PLAYER);
		assert(MaxGamePlayers_ <= GAME_PLAYER);
	}
	//非游戏进行中，修改玩家准备
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_START) {
		userItem->SetUserStatus(sReady);
	}
	//游戏进行中，修改玩家游戏中
	else {
		userItem->SetUserStatus(sPlaying);
	}
	//开局前(<GAME_STATUS_START)达到桌子要求游戏人数，开始游戏
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_START &&
		m_pTableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
		//首个玩家加入桌子
		if (m_pTableFrame->GetPlayerCount(true) == 1) {
			//一定是游戏初始状态，否则清理有问题
			//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT);
			//修改游戏空闲准备
			m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		}
		//不超过桌子要求最大游戏人数
		assert(m_pTableFrame->GetPlayerCount(true) == MaxGamePlayers_);
		snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d:%d:%d)，开始游戏",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << msg;
		GameTimerReadyOver();
	}
	//首个玩家加入桌子
	else if (m_pTableFrame->GetPlayerCount(true) == 1) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 首个玩家加入桌子 ...", m_pTableFrame->GetTableId());
		LOG(ERROR) << msg;
		//一定是游戏初始状态，否则清理有问题
		//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT);
		//修改游戏空闲准备
		m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		//首个加入桌子的必须是真实玩家
		//assert(m_pTableFrame->GetPlayerCount(false) == 1);
		//清理所有定时器
		ClearAllTimer();
		//定时器监控等待其他玩家或机器人加入桌子
		timerIdGameReadyOver = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
		snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d)，等待人数(%d)", m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << msg;
	}
	//游戏初始状态，非首个玩家加入桌子
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 游戏初始状态，非首个玩家加入桌子 ...", m_pTableFrame->GetTableId());
		LOG(ERROR) << msg;
		assert(false);
	}
	//游戏进行中玩家加入桌子，当作断线重连处理
	else if (m_pTableFrame->GetGameStatus() >= GAME_STATUS_START) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 游戏进行中玩家加入桌子，当作断线重连处理 ...", m_pTableFrame->GetTableId());
		LOG(ERROR) << msg;
		//断线重连加入桌子玩家chairID必须前后一致
		//assert(saveUserIDs_[userItem->GetChairId()] == userItem->GetUserId());
		//修改玩家游戏进行中
		userItem->SetUserStatus(sPlaying);
		//进入房间广播通知到其他玩家
		m_pTableFrame->BroadcastUserInfoToOther(userItem);
		//进入房间推送其他玩家给该玩家
		m_pTableFrame->SendAllOtherUserInfoToUser(userItem);
		m_pTableFrame->BroadcastUserStatus(userItem, true);
		//断线重连，发送游戏场景
		OnEventGameScene(userItem->GetChairId(), false);
		snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d] 游戏进行中玩家加入桌子，处理断线重连", m_pTableFrame->GetTableId());
		LOG(ERROR) << msg;
	}
	return true;
}

//判断是否能进入游戏
bool CGameProcess::CanJoinTable(shared_ptr<CServerUserItem>& pUser) {
	char msg[1024] = { 0 };
	//初始化或空闲准备阶段，可以进入桌子
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_START) {

		//达到游戏人数要求禁入游戏
		if (m_pTableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
			return false;
		}

		//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
		if (!pUser || pUser->GetUserId() == -1) {
			if (totalMatchSeconds_ < timeoutMatchSeconds_) {
				//printf("--- *** tableID[%d]%.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", m_pTableFrame->GetTableId(), timeoutMatchSeconds_, pUser->GetUserId(), totalMatchSeconds_);
				return false;
			}
		}
		else {
#if 0
			//timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(pUser->GetUserId());
			if (userItem) {
				if (userItem->IsAndroidUser()) {
					if (totalMatchSeconds_ < timeoutMatchSeconds_) {
						printf("--- *** tableID[%d]%.2fs内不能让机器人userID:%d进入桌子，当前累计匹配时长为：%.2fs\n", m_pTableFrame->GetTableId(), timeoutMatchSeconds_, userId, totalMatchSeconds_);
						return false;
					}

					//机器人AI积分不足，不能进入游戏桌子
					if (userItem->GetUserScore() < m_pTableFrame->GetGameRoomInfo()->enterMinScore) {

						//设置离线状态
						userItem->SetUserStatus(sOffline);

						//清理机器人AI信息
						m_pTableFrame->ClearTableUser(userItem->GetChairId(), false, false);
						return false;
					}
				}
				else {

					//真实玩家积分不足，不能进入游戏桌子
					if (userItem->GetUserScore() < m_pTableFrame->GetGameRoomInfo()->enterMinScore) {

						//设置离线状态
						userItem->SetUserStatus(sOffline);

						//清理玩家信息
						m_pTableFrame->ClearTableUser(userItem->GetChairId(), true, true, ERROR_ENTERROOM_SCORENOENOUGH);
						printf("--- *** tableID[%d] CanJoinTable真实玩家userID:%d积分不足，不能进入游戏桌子，当前累计匹配时长为：%.2fs\n", m_pTableFrame->GetTableId(), userId, totalMatchSeconds_);
						return false;
					}
				}
			}
#endif
#if 0
			//IP访问限制
			if (checkIp_) {
				//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId); assert(!userItem);
				//真实玩家防止作弊检查
				snprintf(msg, sizeof(msg), "\n\n--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]真实玩家防止作弊检查 userId[%d] userIp[%d][%s]\n",
					m_pTableFrame->GetTableId(),
					pUser->GetUserId(),
					pUser->GetIp(), Inet2Ipstr(pUser->GetIp()).c_str());
				LOG(ERROR) << msg;
				for (int i = 0; i < GAME_PLAYER; ++i) {
					if (!m_pTableFrame->IsExistUser(i)) {
						continue;
					}
					if (m_pTableFrame->IsAndroidUser(i)) {
						continue;
					}
					shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
					if (CheckSubnetInetIp(pUser->GetIp(), userItem->GetIp())) {
						snprintf(msg, sizeof(msg), "\n\n--- *** &&&&&&&&&&&&&&&&&&& tableID[%d] userIp[%d][%s] curUserIp[%d][%s] 相同网段，不能进入房间\n",
							m_pTableFrame->GetTableId(),
							pUser->GetIp(), Inet2Ipstr(pUser->GetIp()).c_str(),
							userItem->GetIp(), Inet2Ipstr(userItem->GetIp()).c_str());
						LOG(ERROR) << msg;
						return false;
					}
					snprintf(msg, sizeof(msg), "\n\n--- *** &&&&&&&&&&&&&&&&&&& tableID[%d] userIp[%d][%s] curUserIp[%d][%s] 不同网段\n",
						m_pTableFrame->GetTableId(),
						pUser->GetIp(), Inet2Ipstr(pUser->GetIp()).c_str(),
						userItem->GetIp(), Inet2Ipstr(userItem->GetIp()).c_str());
					LOG(ERROR) << msg;
				}
			}
			//一张桌子真实玩家数量限制
			if (checkrealuser_) {
				snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]房间内只允许进入一个真人玩家\n", m_pTableFrame->GetTableId());
				LOG(ERROR) << msg;
				//房间内只允许进入一个真人玩家
				if (m_pTableFrame->GetPlayerCount(false) >= 1) {
					return false;
				}
			}
#endif
		}
		return true;
	}
	//游戏结束时玩家信息已被清理掉
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_END) {
		//assert(!m_pTableFrame->IsExistUser(pUser->GetUserId()));
		return false;
	}
	else if (m_pTableFrame->GetGameStatus() >= GAME_STATUS_START) {
		//游戏进行状态，处理断线重连，玩家信息必须存在
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(pUser->GetUserId());
		return userItem != NULL;
	}
	//游戏状态为GAME_STATUS_START(100)时，不会进入该函数
	//assert(false);
	return false;
}

//判断是否能离开游戏
bool CGameProcess::CanLeftTable(int64_t userId)
{
	shared_ptr<CServerUserItem> userItem = ByUserId(userId);
	if (!userItem) {
		return true;
	}
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_START ||
		m_pTableFrame->GetGameStatus() == GAME_STATUS_END) {
		return true;
	}
	return false;
}

//用户准备
bool CGameProcess::OnUserReady(int64_t userId, bool islookon)
{
    return true;
}

//用户离开
bool CGameProcess::OnUserLeft(int64_t userId, bool bIsLookUser)
{
	char msg[1024] = { 0 };
	//没有初始化
	assert(m_pTableFrame);
	//桌椅玩家无效
	shared_ptr<CServerUserItem> userItem = ByUserId(userId); assert(userItem);
	assert(userItem->GetChairId() >= 0);
	assert(userItem->GetChairId() < GAME_PLAYER);
	assert(!userItem->IsAndroidUser());
	//桌子非初始状态，玩家离开
	//assert((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_INIT);
	//桌子非游戏状态，玩家离开(中途不能离开)，刷新页面时OnUserLeft调用
	//assert((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_START);
	//桌子非结束状态，玩家离开
	//assert((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_END);
	//桌子空闲准备或游戏中状态
	//assert(
	//	(int)m_pTableFrame->GetGameStatus() == GAME_STATUS_READY ||
	//	(int)m_pTableFrame->GetGameStatus() >= GAME_STATUS_START);
	bool ret = false;
	uint32_t chairId = userItem->GetChairId();
	snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d]-OnUserLeft gamestatus[%d] 用户 [%d] 离开桌子 playerCount[%d:%d]",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), chairId,
		m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true));
	LOG(ERROR) << msg;
	//桌子空闲准备状态 ///
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_READY) {
		//清除保留信息
		saveUserIDs_[chairId] = 0;
		//用户空闲状态
		userItem->SetUserStatus(sFree);
		//玩家离开广播
		m_pTableFrame->BroadcastUserStatus(userItem, true);
		//清除用户信息
		m_pTableFrame->ClearTableUser(chairId, true, true);
		ret = true;
	}
	//桌子空闲准备状态 ///
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_READY) {
		//没有真实玩家 //////
		if (m_pTableFrame->GetPlayerCount(false) == 0) {
			snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d]-OnUserLeft gamestatus[%d] 用户 [%d] 离开桌子，重置初始状态 playerCount[%d:%d]",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), chairId,
				m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true));
			LOG(ERROR) << msg;
			//清理机器人
			if (m_pTableFrame->GetPlayerCount(true) > 0) {
				for (int i = 0; i < GAME_PLAYER; ++i) {
					if (!m_pTableFrame->IsExistUser(i)) {
						continue;
					}
					assert(m_pTableFrame->IsAndroidUser(i));
					//清除用户信息
					m_pTableFrame->ClearTableUser(i, true, false);
				}
			}
			//清理所有定时器
			ClearAllTimer();
			//重置桌子初始状态
			m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		}
		//还有其他真实玩家，不清理机器人，不清理定时器，不重置桌子 //////
		else {
			snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d]-OnUserLeft gamestatus[%d] 用户 [%d] 离开桌子，清理用户 playerCount[%d:%d]",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), chairId,
				m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true));
			LOG(ERROR) << msg;
		}
	}
	return ret;
}

//准备定时器
void CGameProcess::OnTimerGameReadyOver() {
	//清除游戏准备定时器
	ThisThreadTimer->cancel(timerIdGameReadyOver);
	//匹配中没有真实玩家，玩家离开/取消匹配
	if (m_pTableFrame->GetPlayerCount(false) == 0) {
		//桌子已被重置初始状态
		//assert((int)m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT);
		if ((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_INIT) {
			//清理房间内玩家
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				//清除保留信息
				saveUserIDs_[i] = 0;
				shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
				//用户空闲状态
				userItem->SetUserStatus(sFree);
				//清除用户信息
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
			}
			//清理所有定时器
			ClearAllTimer();
			//初始化桌子数据
			InitGameData();
			//重置桌子初始状态
            ReadStorageInformation();
			m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		}
		return;
	}
	//匹配中准备状态
	//assert((int)m_pTableFrame->GetGameStatus() == GAME_STATUS_READY);
	if ((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_READY) {
		//清理房间内玩家
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//清除保留信息
			saveUserIDs_[i] = 0;
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			//用户空闲状态
			userItem->SetUserStatus(sFree);
			//清除用户信息
			m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
		}
		//清理所有定时器
		ClearAllTimer();
		//初始化桌子数据
		InitGameData();
		//重置桌子初始状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		return;
	}
	//匹配中有真实玩家
	assert(m_pTableFrame->GetPlayerCount(false) > 0);
	//计算累计匹配时间，当totalMatchSeconds_ >= timeoutMatchSeconds_时，
	//如果桌子游戏人数不足会自动开启 CanJoinTable 放行机器人进入桌子补齐人数
	totalMatchSeconds_ += sliceMatchSeconds_;
	//达到游戏人数要求开始游戏
	if (m_pTableFrame->GetPlayerCount(true) >= MaxGamePlayers_) {
		assert(m_pTableFrame->GetPlayerCount(true) == MaxGamePlayers_);
	}
	else {
		//桌子游戏人数不足，且没有匹配超时，重启游戏准备定时器，继续等待
		if (totalMatchSeconds_ < timeoutMatchSeconds_) {
			timerIdGameReadyOver = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
			//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]桌子游戏人数不足，且没有匹配超时，再次启动定时器\n", m_pTableFrame->GetTableId());
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) {
			//桌子游戏人数不足，且机器人候补空位超时
			if (m_pTableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
				//达到最小游戏人数，开始游戏
				//printf("--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]达到最小游戏人数(%d:%d:%d)，开始游戏\n",
				//	m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
				GameTimerReadyOver();
			}
			else {
				//仍然没有达到最小游戏人数，重启游戏准备定时器，继续等待
				timerIdGameReadyOver = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
				//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]仍然没有达到最小游戏人数(%d)，继续等待...\n", m_pTableFrame->GetTableId(), MIN_GAME_PLAYER);
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
			//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", m_pTableFrame->GetTableId(), totalMatchSeconds_, MaxGamePlayers_);
			//定时器检测机器人候补空位后是否达到游戏要求人数
			timerIdGameReadyOver = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
		}
	}
}

//结束准备，开始游戏
void CGameProcess::GameTimerReadyOver()
{
	//确定游戏状态：初始化空闲准备阶段，而非游戏中
	//assert(m_pTableFrame->GetGameStatus() < GAME_STATUS_START);
	//清除游戏准备定时器
	ThisThreadTimer->cancel(timerIdGameReadyOver);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
		if (userItem) {
			if (userItem->IsAndroidUser()) {
				//机器人AI积分不足，不能进入游戏桌子
				if (userItem->GetUserScore() < EnterMinScore) {
					//设置离线状态
					userItem->SetUserStatus(sOffline);
					//清理机器人AI信息
					m_pTableFrame->ClearTableUser(i, false, false);
				}
			}
			else {
				//游戏准备阶段玩家离线了
				if (userItem->GetUserStatus() == sOffline) {
					//清理玩家信息
					m_pTableFrame->ClearTableUser(i, true, true);
				}
				//真实玩家积分不足，不能进入游戏桌子
				else if (userItem->GetUserScore() < EnterMinScore) {
					//设置离线状态
					userItem->SetUserStatus(sOffline);
					//清理玩家信息
					m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
				}
			}
		}
	}
	//如果玩家人数达到了最小游戏人数
	if (m_pTableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
		//清理游戏数据
		ClearGameData();
		//初始化数据
		InitGameData();
		//修改各玩家状态
		for (int i = 0; i < GAME_PLAYER; ++i) {
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			if (userItem) {
				//修改玩家游戏进行中
				userItem->SetUserStatus(sPlaying);
			}
		}
		//广播各玩家状态
		for (int i = 0; i < GAME_PLAYER; ++i) {
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			if (userItem) {
				//进入房间广播通知到其他玩家
				m_pTableFrame->BroadcastUserInfoToOther(userItem);
				//进入房间推送其他玩家给该玩家
				m_pTableFrame->SendAllOtherUserInfoToUser(userItem);
				m_pTableFrame->BroadcastUserStatus(userItem, true);
			}
		}
		//游戏进行中
		m_pTableFrame->SetGameStatus(GAME_STATUS_START);
		//printf("--- *** &&&&&&&&&&&&&&&&&&& tableID[%d] OnGameStart 游戏开始发牌(%d)\n", m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(true));
		//开始游戏
		OnGameStart();
	}
	else {
		//assert(false);
		//没有达到最小游戏人数
		for (int i = 0; i < GAME_PLAYER; ++i) {
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			if (userItem) {
				
				//重置准备状态
				userItem->SetUserStatus(sReady);
			}
		}
		//清除所有定时器
		ClearAllTimer();
		//重置桌子游戏初始状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
	}
}

//游戏开始
void CGameProcess::OnGameStart()
{
	char msg[4096] = { 0 };
	m_replay.clear();
	//确定游戏进行中
	//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_START);
	//清除所有定时器
    ClearAllTimer();
	//系统当前库存
	m_pTableFrame->GetStorageScore(storageInfo_);
	//牌局编号
	strRoundID_ = m_pTableFrame->GetNewRoundId();
	//对局日志
	m_replay.gameinfoid = strRoundID_;
	//游戏开始时间
	roundStartTime_ = chrono::system_clock::now();
	//本局黑名单玩家数
	int blackUserNum = 0;
	//点杀概率值(千分比)
	int rateLostMax = 0;
	//点杀概率值判断是否本局点杀
	bool killBlackUsers = false;
	//本局含机器人
	if (m_pTableFrame->GetPlayerCount(false) != m_pTableFrame->GetPlayerCount(true)) {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (m_pTableFrame->IsAndroidUser(i)) {
				continue;
			}
			//黑名单点杀数据
			tagBlacklistInfo& blackUser = ByChairId(i)->GetBlacklistInfo();
			//控制中status=1
			//需要杀current<total
			if (blackUser.status > 0 &&
				blackUser.current < blackUser.total) {
				if (blackUser.weight > rateLostMax) {
					rateLostMax = blackUser.weight;
				}
				++blackUserNum;
				LOG(ERROR) << " --- *** [" << strRoundID_ << "] *** blackList"
					<< "\nuserId=" << UserIdBy(i) << " rate=" << blackUser.weight << " current=" << blackUser.current
					<< " total=" << blackUser.total << " status=" << blackUser.status;
			}
		}
		//点杀黑名单存在，且满足概率点杀条件
		if (blackUserNum > 0 &&
			RandomBetween(1, 1000) <= rateLostMax) {
			killBlackUsers = true;
		}
	}
	//配置牌型
	std::vector<std::string> lines;
	int flag = 0;
	int enum_group_sz = 100;
	if (!INI_CARDLIST_.empty() && boost::filesystem::exists(INI_CARDLIST_)) {
		//读取配置
		readFile(INI_CARDLIST, lines, ";;");
		//1->文件读取手牌 0->随机生成13张牌
		flag = atoi(lines[0].c_str());
		//默认最多枚举多少组墩，开元/德胜是3组
		enum_group_sz = atoi(lines[1].c_str());
	}
	//1->文件读取手牌 0->随机生成13张牌
	if (flag > 0) {
		std::map<int, bool> imap;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			int index = RandomBetween(2, lines.size() - 1);
			do {
				std::map<int, bool>::iterator it = imap.find(index);
				if (it == imap.end()) {
					imap[index] = true;
					break;
				}
				index = RandomBetween(2, lines.size() - 1);
			} while (true);
			//构造一副手牌13张
			int n = S13S::CGameLogic::MakeCardList(lines[index], &(handCards_[i])[0], MAX_COUNT);
			assert(n == MAX_COUNT);
			m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
		}
	}
	else {
		//给各个玩家发牌
		{
		restart:
			assert(m_pTableFrame->GetPlayerCount(true) <= GAME_PLAYER);
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
				//余牌不够则重新洗牌，然后重新分发给各个玩家
				if (g.Remaining() < MAX_COUNT) {
					g.ShuffleCards();
					goto restart;
				}
				//发牌
				g.DealCards(MAX_COUNT, &(handCards_[i])[0]);
				m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
			}
		}
	}
	//各个玩家手牌分析
	{
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			//snprintf(msg, sizeof(msg), "--- *** chairID = [%d]", i);
				//LOG(ERROR) << msg;
			//机器人AI
			if (m_pTableFrame->IsAndroidUser(i)) {
				LOG(ERROR) << "\n========================================================================\n"
					<< "--- *** [" << strRoundID_ << "] 机器人 [" << i << "] 手牌 ["
					<< S13S::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT) << "]\n";
			}
			else {
				LOG(ERROR) << "\n========================================================================\n"
					<< "--- *** [" << strRoundID_ << "] 玩家 [" << i << "] 手牌 ["
					<< S13S::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT) << "]\n";
			}
			//一副手牌
			//S13S::CGameLogic::PrintCardList(&(handCards_[i])[0], MAX_COUNT);
			//手牌排序
			S13S::CGameLogic::SortCards(&(handCards_[i])[0], MAX_COUNT, true, true, true);
			//手牌牌型分析
			int c = S13S::CGameLogic::AnalyseHandCards(&(handCards_[i])[0], MAX_COUNT, enum_group_sz, handInfos_[i],
				ThisRoomId, ThisTableId, i);
			phandInfos_[i] = &handInfos_[i];
			//有特殊牌型时
			//if (handInfos_[i].specialTy_ == S13S::Tysp) {
			//	goto restart;
			//}
			//查看所有枚举牌型
			//handInfos_[i].rootEnumList->PrintEnumCards(false, S13S::Ty123sc);
			//查看手牌特殊牌型
			//handInfos_[i].PrintSpecCards();
			//查看手牌枚举三墩牌型
			//handInfos_[i].PrintEnumCards();
			//查看重复牌型和散牌
			//handInfos_[i].classify.PrintCardList();
			//snprintf(msg, sizeof(msg), "--- *** c = %d %s\n\n\n\n", c, phandInfos_[i]->StringSpecialTy().c_str());
			//LOG(ERROR) << msg;
		}
	}
	//换牌策略分析
	{
		AnalysePlayerCards(killBlackUsers);
	}
	//给机器人发送所有人的牌数据
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//座椅有玩家
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//跳过真人玩家
		if (!m_pTableFrame->IsAndroidUser(i)) {
			continue;
		}
		//给机器人发数据
 		s13s::CMD_S_AndroidCard reqdata;
		//牌局编号
		reqdata.set_roundid(strRoundID_);
 		for (int j = 0; j < GAME_PLAYER; ++j) {
 			//座椅有玩家
 			if (!m_pTableFrame->IsExistUser(j)) {
 				continue;
 			}
			//跳过自己，自己的手牌数据从CMD_S_GameStart中传递
			if (j == i) {
				continue;
			}
 			//其它玩家数据(真实玩家/其它机器人)
 			s13s::AndroidPlayer* player = reqdata.add_players();
 			//桌椅ID
 			player->set_chairid(j);
 			//玩家手牌
 			s13s::HandCards* handcards = player->mutable_handcards();
 			//一副13张手牌
 			handcards->set_cards(&(handCards_[j])[0], MAX_COUNT);
 			//标记手牌特殊牌型
 			handcards->set_specialty(phandInfos_[j]->SpecialTy());
 			//
 		}
		//发送机器人数据
 		std::string content = reqdata.SerializeAsString();
 		m_pTableFrame->SendTableData(i, s13s::SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
	}
	//理牌总的时间
	//groupTotalTime_ = 3000;
	//理牌剩余时间
	uint32_t wTimeLeft = groupTotalTime_;
	//理牌开始时间
	groupStartTime_ = (uint32_t)time(NULL);
	//理牌结束时间
	groupEndTime_ = groupStartTime_ + wTimeLeft;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//座椅有玩家
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//真实/机器人玩家
		//if (m_pTableFrame->IsAndroidUser(i)) {
		//	continue;
		//}
		//printf("\n\n========================================================================\n");
		//printf("--- *** chairID = [%d]\n", i);
		//一副手牌
		//S13S::CGameLogic::PrintCardList(&(handCards_[i])[0], MAX_COUNT);
		//查看重复牌型和散牌
		//phandInfos_[i]->classify.PrintCardList();
		//查看所有枚举牌型
		//phandInfos_[i]->rootEnumList->PrintEnumCards(false, S13S::Ty123sc);
		//查看手牌特殊牌型
		//phandInfos_[i]->PrintSpecCards();
		//查看手牌枚举三墩牌型
		//phandInfos_[i]->PrintEnumCards();
		//printf("--- *** c = %d %s\n\n\n\n", phandInfos_[i]->groups.size(), phandInfos_[i]->StringSpecialTy().c_str());
		//游戏开始，填充相关数据
 		s13s::CMD_S_GameStart reqdata;
		//一副手牌
		s13s::HandCards* handcards = reqdata.mutable_handcards();
		//一副13张手牌
		handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
		//标记手牌特殊牌型
		handcards->set_specialty(phandInfos_[i]->SpecialTy());
		int j = 0;
		for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[i]->spec_groups.begin();
			it != phandInfos_[i]->spec_groups.end(); ++it) {
			assert(phandInfos_[i]->spec_groups.size() == 1);
			//特殊牌型放在枚举几组最优解前面
			s13s::GroupDunData* group = handcards->add_groups();
			//从哪墩开始的
			group->set_start(it->start);
			//总体对应特殊牌型
			group->set_specialty(it->specialTy);
			//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
			//printf("第[%d]组\t=>>\t", j++ + 1);
			for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				s13s::DunData* dun_i = group->add_duns();
				//标记0-头/1-中/2-尾
				dun_i->set_id(k);
				//墩对应普通牌型
				dun_i->set_ty(it->duns[k].ty_);
				//墩对应牌数c(3/5/5)
				dun_i->set_c(it->duns[k].c);
				//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
				dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
				printf("dun[%d] c:=%d\t", k, it->duns[k].c);
			}
			//printf("\n\n");
		}
		for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[i]->enum_groups.begin();
			it != phandInfos_[i]->enum_groups.end(); ++it) {
			//枚举几组最优墩
			s13s::GroupDunData* group = handcards->add_groups();
			//从哪墩开始的
			group->set_start(it->start);
			//总体对应特殊牌型
			group->set_specialty(it->specialTy);
			//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
			//printf("第[%d]组\t=>>\t", j++ + 1);
			for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				s13s::DunData* dun_i = group->add_duns();
				//标记0-头/1-中/2-尾
				dun_i->set_id(k);
				//墩对应普通牌型
				dun_i->set_ty(it->duns[k].ty_);
				//墩对应牌数c(3/5/5)
				dun_i->set_c(it->duns[k].c);
				//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
				dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
				//printf("dun[%d] c:=%d\t", k, it->duns[k].c);
			}
			//printf("\n\n");
		}
		//局数编号
		reqdata.set_roundid(strRoundID_);
		//基础积分
		reqdata.set_ceilscore(FloorScore);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			int64_t userscore = 0;
			int playerstatus = 0;
			if (m_pTableFrame->IsExistUser(i)) {
				shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
				userscore = userItem->GetUserScore();
				playerstatus = userItem->GetUserStatus();
			}
					
			//玩家当前状态
			reqdata.add_playstatus(playerstatus);
					
			//玩家当前积分
			reqdata.add_userscore(userscore);
		}
		//理牌剩余时间
		reqdata.set_wtimeleft(wTimeLeft);
		//序列化std::string
		//string content = reqdata.SerializeAsString();
		//m_pTableFrame->SendTableData(i, s13s::SUB_S_GAME_START, (uint8_t *)content.data(), content.size());
		//序列化bytes
		int len = reqdata.ByteSizeLong();//len
		uint8_t *data = new uint8_t[len];
		reqdata.SerializeToArray(data, len);//data
		std::string const& typeName = reqdata.GetTypeName();//typename
		m_pTableFrame->SendTableData(i, s13s::SUB_S_GAME_START, data, len);
		delete[] data;
		//转换json格式
		//std::string jsonstr;
		//PB2JSON::Pb2Json::PbMsg2JsonStr(reqdata, jsonstr, true);
		//snprintf(msg, sizeof(msg), "\n--- *** %s\n", typeName.c_str());
		//LOG(ERROR) << msg << jsonstr << "\n\n";
		//printf("%s\n\n", jsonstr.c_str());
		
	//	}
		//机器人
	//	else {
			//给机器人发送所有人牌数据
			//CMD_S_AndroidCard AndroidCard;
			//for (int j = 0; j < GAME_PLAYER; ++j) {
			//	for (int k = 0; k < MAX_COUNT; ++k) {
			//		//handCards_[i][j];
			//	}
			//}
			//string content = AndroidCard.SerializeAsString();
			//m_pTableFrame->SendTableData(i, SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
	//	}
	}
	//玩家组墩定时器
	ThisThreadTimer->cancel(timerIdManual_);
	timerIdManual_ = ThisThreadTimer->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::OnTimerGroupOver, this));
}

//发送场景
bool CGameProcess::OnEventGameScene(uint32_t chairId, bool bIsLookUser)
{
	assert(chairId >= 0);
	assert(chairId < GAME_PLAYER);
	LOG(ERROR) << "--- *** OnEventGameScene chairID[" << chairId << "] GameStatus[" << m_pTableFrame->GetGameStatus() << "]";
    switch(m_pTableFrame->GetGameStatus())
    {
    case GAME_STATUS_INIT:
    case GAME_STATUS_READY: {
        s13s::CMD_S_StatusFree StatusFree;
        StatusFree.set_ceilscore(FloorScore);//ceilscore
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			if (userItem) {
				::s13s::PlayerItem* player = StatusFree.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userID
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_status(userItem->GetUserStatus());//userstatus
				player->set_score(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
			}
		}
        //发送场景
        std::string content = StatusFree.SerializeAsString();
        m_pTableFrame->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_FREE, (uint8_t *)content.data(), content.size());
        break;
    }
	//游戏状态，玩家手牌/是否理牌/玩家比牌，游戏结束
    case GAME_STATUS_START:
	case GAME_STATUS_GROUP:
	case GAME_STATUS_COMPARE:
	case GAME_STATUS_END: {
		s13s::CMD_S_StatusPlay StatusPlay;
        StatusPlay.set_roundid(strRoundID_);//roundId
		StatusPlay.set_ceilscore(FloorScore);//ceilscore
		StatusPlay.set_status(gameStatus_);//playstatus
		//理牌状态
		if (gameStatus_ < GAME_STATUS_COMPARE) {
			StatusPlay.set_wtimeleft(groupEndTime_ - (uint32_t)time(NULL));//理牌剩余时间
			StatusPlay.set_wtotaltime(groupTotalTime_);//理牌总的时间 wTimeLeft
		}
		else {
			//比牌状态
			StatusPlay.set_wtimeleft(0);//剩余时间
			StatusPlay.set_wtotaltime(groupTotalTime_);//理牌总的时间 wTimeLeft
		}
		if (gameStatus_ < GAME_STATUS_COMPARE) {
			//玩家没有确认牌型，发送手牌
			if (!phandInfos_[chairId]->HasSelected()) {
				//玩家手牌
				s13s::HandCards* handcards = StatusPlay.mutable_handcards();
				//一副13张手牌
				handcards->set_cards(&(handCards_[chairId])[0], MAX_COUNT);
				//标记手牌特殊牌型
				handcards->set_specialty(phandInfos_[chairId]->SpecialTy());
				int j = 0;
				for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[chairId]->spec_groups.begin();
					it != phandInfos_[chairId]->spec_groups.end(); ++it) {
					assert(phandInfos_[chairId]->spec_groups.size() == 1);
					//特殊牌型放在枚举几组最优解前面
					s13s::GroupDunData* group = handcards->add_groups();
					//从哪墩开始的
					group->set_start(it->start);
					//总体对应特殊牌型
					group->set_specialty(it->specialTy);
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					//printf("第[%d]组\t=>>\t", j++ + 1);
					for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
						//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
						s13s::DunData* dun_i = group->add_duns();
						//标记0-头/1-中/2-尾
						dun_i->set_id(k);
						//墩对应普通牌型
						dun_i->set_ty(it->duns[k].ty_);
						//墩对应牌数c(3/5/5)
						dun_i->set_c(it->duns[k].c);
						//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
						dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
						//printf("dun[%d] c:=%d\t", k, it->duns[k].c);
					}
					//printf("\n\n");
				}
				for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = phandInfos_[chairId]->enum_groups.begin();
					it != phandInfos_[chairId]->enum_groups.end(); ++it) {
					//枚举几组最优墩
					s13s::GroupDunData* group = handcards->add_groups();
					//从哪墩开始的
					group->set_start(it->start);
					//总体对应特殊牌型
					group->set_specialty(it->specialTy);
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					//printf("第[%d]组\t=>>\t", j++ + 1);
					for (int k = S13S::DunFirst; k <= S13S::DunLast; ++k) {
						//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
						s13s::DunData* dun_i = group->add_duns();
						//标记0-头/1-中/2-尾
						dun_i->set_id(k);
						//墩对应普通牌型
						dun_i->set_ty(it->duns[k].ty_);
						//墩对应牌数c(3/5/5)
						dun_i->set_c(it->duns[k].c);
						//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
						dun_i->set_cards(it->duns[k].cards, it->duns[k].c);
						//printf("dun[%d] c:=%d\t", k, it->duns[k].c);
					}
					//printf("\n\n");
				}
			}
		}
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			if (userItem) {
				::s13s::PlayerItem* player = StatusPlay.add_players();
				player->set_chairid(i);//chairID
				player->set_userid(userItem->GetUserId());//userID
				player->set_nickname(userItem->GetNickName());//nickname
				player->set_headid(userItem->GetHeaderId());//headID
				player->set_status(userItem->GetUserStatus());//userstatus
				player->set_score(userItem->GetUserScore());//userScore
				player->set_location(userItem->GetLocation());//location
				//玩家发牌/理牌阶段
				if (gameStatus_ < GAME_STATUS_COMPARE) {
					//玩家手牌/是否理牌
					player->set_selected(phandInfos_[i]->HasSelected() ? 1 : 0); //selected
				}
			}
		}
		//比牌阶段
		if (gameStatus_ == GAME_STATUS_COMPARE) {
			//玩家比牌数据
			s13s::CMD_S_CompareCards* compareCards = StatusPlay.mutable_cmp();
			compareCards->CopyFrom(compareCards_[chairId]);
		}
		//发送场景
		std::string content = StatusPlay.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_PLAY, (uint8_t *)content.data(), content.size());
        break;
    }
	//游戏结束
    //case s13s::GAME_STATUS_END: {
    //   s13s::CMD_S_StatusEnd statusEnd;
	//	int32_t wTimeLeft = groupEndTime_ - (uint32_t)time(NULL);
	//	statusEnd.set_wtimeleft(wTimeLeft > 0 ? wTimeLeft : 0);
    //    std::string content = statusEnd.SerializeAsString();
    //    m_pTableFrame->SendTableData(chairId, s13s::SUB_SC_GAMESCENE_END, (uint8_t *)content.data(), content.size());
	//	break;
	//}
    default:
        break;
    }
    return true;
}

//玩家组牌定时器
void CGameProcess::OnTimerGroupOver() {
	ThisThreadTimer->cancel(timerIdManual_);
	//如果开始比牌返回
	if (isCompareCards_) {
		return;
	}
	//未确定牌型的玩家指定默认牌型
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
		{
			if (!phandInfos_[i]->HasSelected()) {
				phandInfos_[i]->Select(0);
				++selectcount;
				//对局日志详情
				{
					//单墩牌(3/5/5)
					std::string strDunCards;
					for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
						if (strDunCards.empty()) {
							strDunCards = S13S::CGameLogic::StringCards(
								phandInfos_[i]->GetSelected()->duns[d].cards,
								phandInfos_[i]->GetSelected()->duns[d].GetC());
						}
						else {
							strDunCards += "," +
								S13S::CGameLogic::StringCards(
									phandInfos_[i]->GetSelected()->duns[d].cards,
									phandInfos_[i]->GetSelected()->duns[d].GetC());
						}
					}
					m_replay.addStep((uint32_t)time(NULL) - groupStartTime_, strDunCards, -1, 0, i, i);
				}
			}
		}
	}
	//玩家之间两两比牌
	StartCompareCards();
}
bool CGameProcess::CompareAllCardsSame(uint8_t *handcards,uint8_t *clientcards,int32_t len)
{
    int cardsnum =0;
     for(int i=0;i<13;i++)
     {
        for(int j=0;j<len;j++)
        {
            if(handcards[i]==clientcards[j])
            {
                cardsnum ++;
            }
        }
     }
     if(cardsnum==len)
     {
         return true ;
     }
     return false;
}

bool CGameProcess::bCheckCard(uint8_t *handcards, uint8_t *clientcards, int32_t len)
{
	bool bCheckOK = true;

	uint8_t handcardTemp[MAX_COUNT] = { 0 };
	uint8_t clientcardTemp[MAX_COUNT] = { 0 };
	memcpy(handcardTemp, handcards, MAX_COUNT);
	memcpy(clientcardTemp, clientcards, len);

	//检查牌值是否重复
	uint8_t color = 0;
	uint8_t value = 0;
	uint8_t kingcount = 0;
	uint8_t handCardColorIndex[4][13] = { 0 };
    if (len > 13)
	{
		return false;
	}

	//是否存在
	bool bHave = false;
	for (int i = 0;i < len;i++)
	{
		bHave = false;
		for (int j = 0;j < 13;j++)
		{
            if (clientcards[i] == handcards[j])
			{
				bHave = true;
				break;
			}
		}
		if (!bHave)
		{
			return false;
		}
	}
	// 是否重复
	for (int j = 0; j < len; ++j)
	{
		if (clientcards[j] == 0)
		{
			return false;
		}
		color = (g.GetCardColor(clientcards[j]) >> 4) - 1;
		if (color < 4)
		{
			value = g.GetCardValue(clientcards[j]);
			handCardColorIndex[color][value - 1]++;
			if (handCardColorIndex[color][value - 1] > 1)
			{
				bCheckOK = false;
				break;
			}
		}
	}
	if (!bCheckOK)
		return false;

	return bCheckOK;
}

bool CGameProcess::bCheckCardIsValid(uint8_t *clientcards, int32_t len)
{
	bool bCheckOK = true;
	uint8_t color = 0;
	uint8_t value = 0;
	//牌值是否有效
	for (int i = 0; i < len; ++i)
	{
		if (clientcards[i] == 0)
		{
			return false;
		}
		color = (g.GetCardColor(clientcards[i]) >> 4) - 1;
		value = g.GetCardValue(clientcards[i]);
		if (color < 0 || color > 3)
		{
			return false;
		}
		if (value < 0 || value > 0x0D)
		{
			return false;
		}
	}

	return bCheckOK;
}
string CGameProcess::getCardString(uint8_t *cards, int32_t len)
{
	string cardStr = "";
	char buf[32] = { 0 };
	for (int j = 0; j < len; ++j)
	{
		snprintf(buf, sizeof(buf), "0x%02X ", (unsigned char)cards[j]);
		cardStr += buf;
	}
	return cardStr;
}
//游戏消息
bool CGameProcess::OnGameMessage(uint32_t chairId, uint8_t dwSubCmdID, const uint8_t* pDataBuffer, uint32_t dwDataSize)
{
	if (chairId == INVALID_CHAIR) {
		return false;
	}
    switch (dwSubCmdID) {
		//手动摆牌
	case s13s::SUB_C_MANUALCARDS: {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId);
		assert(userItem != NULL);
		//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_PLAYING);//bug???
		if (m_pTableFrame->GetGameStatus() != GAME_STATUS_PLAYING) {
			return false;
		}
		//////////////////////////////////////////////////////////////
		//客户端请求数据
		s13s::CMD_C_ManualCards reqdata;
		if (!reqdata.ParseFromArray(pDataBuffer, dwDataSize)) {
			return false;
		}
		{
			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(reqdata, jsonstr, true);
			//std::string const& typeName = reqdata.GetTypeName();//typename
			//printf("\n--- *** %s\n", typeName.c_str());
			//printf("%s\n\n", jsonstr.c_str());
		}

        /////
        ///
        ///

		//////////////////////////////////////////////////////////////
		//服务器应答数据
		s13s::CMD_S_ManualCards rspdata;
		//////////////////////////////////////////////////////////////
		S13S::CGameLogic::EnumTree enumList;
		S13S::CGameLogic::EnumTree* rootEnumList = NULL;
		//////////////////////////////////////////////////////////////
		//客户端选择了哪一墩
		S13S::DunTy dt = (S13S::DunTy)(reqdata.dt());
		//客户端选择了哪些牌
		uint8_t * cards = (uint8_t *)reqdata.cards().c_str();

        //客户端选择了几张牌
        int len = reqdata.cards().size();

		//判断敦数和牌数及牌值是否有效
		if (dt < S13S::DunFirst || dt > S13S::DunLast)
		{
			LOG(WARNING) << "=== 传过来的敦数不对： " << (int32_t)dt << "  userid:" << userItem->GetUserId();
			return false;
		}
		int32_t dunLen = (dt == S13S::DunFirst ? 3 : 5);
		if (len > 0 && len != dunLen)
		{
			LOG(WARNING) << "=== 传过来的敦数对应的牌数不对： " << (int32_t)dt << "  " << dunLen << "  userid:" << userItem->GetUserId();
			return false;
		}
		//牌值是否有效
		if (!bCheckCardIsValid(cards, len))
		{
			string str = getCardString(cards, len);
			LOG(WARNING) << "=== 传过来的牌值有无效值： " << str << "  userid:" << userItem->GetUserId();
			return false;
		}

        //判断玩家传过来的牌，必须要是玩家手牌中的牌
        if(!CompareAllCardsSame(handCards_[chairId],cards,len))
        {
			LOG(WARNING) << "=== CompareAllCardsSame 玩家搞事情,传过来的牌值跟手牌不对： " << "  userid:" << userItem->GetUserId();
            return false;
        }
		if (!bCheckCard(handCards_[chairId], cards, len))
		{			
			LOG(WARNING) << "=== bCheckCard 玩家搞事情,传过来的牌值跟手牌不对： " << "  userid:" << userItem->GetUserId();
			return false;
		}

		//printf("--- *** $$$$$$$$$$$$$$$$$$$$$$$$$$ len:%d\n", len);
		if (len > 0) {
			S13S::CGameLogic::SortCards(cards, len, true, true, true);
			S13S::CGameLogic::PrintCardList(cards, len);
			//////////////////////////////////////////////////////////////
			//对客户端选择的一组牌，进行单墩牌型判断
			S13S::HandTy ty = S13S::CGameLogic::GetDunCardHandTy(dt, cards, len);
			//////////////////////////////////////////////////////////////
			//手动摆牌，头墩要么三条/对子/乌龙，同花顺/同花/顺子都要改成乌龙
			if (dt == S13S::DunFirst && (ty == S13S::Ty123sc || ty == S13S::Tysc || ty == S13S::Ty123)) {
				ty = S13S::Tysp;
			}
			//////////////////////////////////////////////////////////////
			//判断手动摆牌是否倒水
			if (phandInfos_[chairId]->IsInverted(dt, cards, len, ty)) {
				phandInfos_[chairId]->ResetManual();
				rspdata.set_result(1);
				rspdata.set_errmsg("手动摆牌倒水");
				//序列化std::string
				string content = rspdata.SerializeAsString();//data
				m_pTableFrame->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, (uint8_t *)content.data(), content.size());
				std::string const& typeName = rspdata.GetTypeName();//typename
				//转换json格式
				//std::string jsonstr;
				//PB2JSON::Pb2Json::PbMsg2JsonStr(rspdata, jsonstr, true);
				//printf("\n--- *** %s\n", typeName.c_str());
				//printf("%s\n\n", jsonstr.c_str());
				return false;
			}
			//////////////////////////////////////////////////////////////
			//手动选牌组墩，给指定墩(头/中/尾墩)选择一组牌
			if (!phandInfos_[chairId]->SelectAs(dt, cards, len, ty)) {
				return false;
			}
			//返回组墩后剩余牌
			uint8_t cpy[S13S::MaxSZ] = { 0 };
			int cpylen = 0;
			phandInfos_[chairId]->GetLeftCards(&(handCards_[chairId])[0], MAX_COUNT, cpy, cpylen);
			//3/8/10
			len = phandInfos_[chairId]->GetManualC() < 10 ? 5 : 3;
			//////////////////////////////////////////////////////////////
			//从余牌中枚举所有牌型
			S13S::CGameLogic::classify_t classify = { 0 };
			S13S::CGameLogic::EnumCards(cpy, cpylen, len, classify, enumList, dt);
			//////////////////////////////////////////////////////////////
			//指向新的所有枚举牌型
			rootEnumList = &enumList;
			//查看所有枚举牌型
			//rootEnumList->PrintEnumCards(false, S13S::TyAllBase);
			//////////////////////////////////////////////////////////////
			//客户端选择了哪一墩，标记0-头/1-中/2-尾
			rspdata.set_dt(dt);
			//墩对应牌型
			rspdata.set_ty(ty);
			//剩余牌
			rspdata.set_cpy(cpy, cpylen);
		}
		else {
			//////////////////////////////////////////////////////////////
			//重新摆牌重置手动摆牌
			phandInfos_[chairId]->ResetManual();
			assert(phandInfos_[chairId]->GetManualC() == 0);
			//////////////////////////////////////////////////////////////
			//指向初始所有枚举牌型
			rootEnumList = phandInfos_[chairId]->rootEnumList;
		}
		//////////////////////////////////////////////////////////////
		//所有枚举牌型
		s13s::EnumCards* cards_enums = rspdata.mutable_enums();
		assert(rootEnumList != NULL);
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123sc.begin();
			it != rootEnumList->v123sc.end(); ++it) {
			cards_enums->add_v123sc(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v40.begin();
			it != rootEnumList->v40.end(); ++it) {
			cards_enums->add_v40(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v32.begin();
			it != rootEnumList->v32.end(); ++it) {
			cards_enums->add_v32(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->vsc.begin();
			it != rootEnumList->vsc.end(); ++it) {
			cards_enums->add_vsc(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123.begin();
			it != rootEnumList->v123.end(); ++it) {
			cards_enums->add_v123(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v30.begin();
			it != rootEnumList->v30.end(); ++it) {
			cards_enums->add_v30(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v22.begin();
			it != rootEnumList->v22.end(); ++it) {
			cards_enums->add_v22(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v20.begin();
			it != rootEnumList->v20.end(); ++it) {
			cards_enums->add_v20(&it->front(), it->size());
		}
		{
			//序列化std::string
			//string content = rspdata.SerializeAsString();
			//m_pTableFrame->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, (uint8_t *)content.data(), content.size());
			//序列化bytes
			int len = rspdata.ByteSizeLong();//len
			uint8_t *data = new uint8_t[len];
			rspdata.SerializeToArray(data, len);//data
			std::string const& typeName = rspdata.GetTypeName();//typename
			m_pTableFrame->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, data, len);
			delete[] data;
			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(rspdata, jsonstr, true);
			//printf("\n--- *** %s\n", typeName.c_str());
			//printf("%s\n\n", jsonstr.c_str());
		}

		break;
	}
	//手动摆牌清空重置指定墩的牌
	case s13s::SUB_C_CANCELCARDS: {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId); assert(userItem != NULL);
		//////////////////////////////////////////////////////////////
		//客户端请求数据
		s13s::CMD_C_CancelCards reqdata;
		if (!reqdata.ParseFromArray(pDataBuffer, dwDataSize)) {
			return false;
		}
		if (reqdata.dt() < S13S::DunFirst || reqdata.dt() > S13S::DunLast) {
			return false;
		}
		//////////////////////////////////////////////////////////////
		//服务器应答数据
		s13s::CMD_S_CancelCards rspdata;
		//////////////////////////////////////////////////////////////
		S13S::CGameLogic::EnumTree enumList;
		S13S::CGameLogic::EnumTree* rootEnumList = NULL;
		//重置指定墩的牌
		if (!phandInfos_[chairId]->ResetManual(S13S::DunTy(reqdata.dt()))) {
			rspdata.set_result(1);
			rspdata.set_errmsg("无效的一墩牌");
			//序列化std::string
			string content = rspdata.SerializeAsString();//data
			m_pTableFrame->SendTableData(chairId, s13s::SUB_S_CANCELCARDS, (uint8_t *)content.data(), content.size());
			std::string const& typeName = rspdata.GetTypeName();//typename
			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(rspdata, jsonstr, true);
			//printf("\n--- *** %s\n", typeName.c_str());
			//printf("%s\n\n", jsonstr.c_str());
			return false;
		}
		if (phandInfos_[chairId]->GetManualC() > 0) {
			//返回取消牌墩后剩余牌
			uint8_t cpy[S13S::MaxSZ] = { 0 };
			int cpylen = 0;
			phandInfos_[chairId]->GetLeftCards(&(handCards_[chairId])[0], MAX_COUNT, cpy, cpylen);
			//3/8/10
			int len = phandInfos_[chairId]->GetManualC() < 10 ? 5 : 3;
			S13S::DunTy dt = len == 5 ? S13S::DunLast : S13S::DunFirst;
			//////////////////////////////////////////////////////////////
			//从余牌中枚举所有牌型
			S13S::CGameLogic::classify_t classify = { 0 };
			S13S::CGameLogic::EnumCards(cpy, cpylen, len, classify, enumList, dt);
			//////////////////////////////////////////////////////////////
			//指向新的所有枚举牌型
			rootEnumList = &enumList;
			//查看所有枚举牌型
			//rootEnumList->PrintEnumCards(false, S13S::TyAllBase);
			//////////////////////////////////////////////////////////////
			//客户端选择了哪一墩，标记0-头/1-中/2-尾
			rspdata.set_dt(reqdata.dt());
			//剩余牌
			rspdata.set_cpy(cpy, cpylen);
		}
		else {
			//////////////////////////////////////////////////////////////
			//指向初始所有枚举牌型
			rootEnumList = phandInfos_[chairId]->rootEnumList;
		}
		//////////////////////////////////////////////////////////////
		//所有枚举牌型
		s13s::EnumCards* cards_enums = rspdata.mutable_enums();
		assert(rootEnumList != NULL);
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123sc.begin();
			it != rootEnumList->v123sc.end(); ++it) {
			cards_enums->add_v123sc(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v40.begin();
			it != rootEnumList->v40.end(); ++it) {
			cards_enums->add_v40(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v32.begin();
			it != rootEnumList->v32.end(); ++it) {
			cards_enums->add_v32(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->vsc.begin();
			it != rootEnumList->vsc.end(); ++it) {
			cards_enums->add_vsc(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v123.begin();
			it != rootEnumList->v123.end(); ++it) {
			cards_enums->add_v123(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v30.begin();
			it != rootEnumList->v30.end(); ++it) {
			cards_enums->add_v30(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v22.begin();
			it != rootEnumList->v22.end(); ++it) {
			cards_enums->add_v22(&it->front(), it->size());
		}
		for (std::vector<S13S::CGameLogic::EnumTree::CardData>::const_iterator it = rootEnumList->v20.begin();
			it != rootEnumList->v20.end(); ++it) {
			cards_enums->add_v20(&it->front(), it->size());
		}
		{
			//序列化std::string
			//string content = rspdata.SerializeAsString();
			//m_pTableFrame->SendTableData(chairId, s13s::SUB_S_MANUALCARDS, (uint8_t *)content.data(), content.size());
			//序列化bytes
			int len = rspdata.ByteSizeLong();//len
			uint8_t *data = new uint8_t[len];
			rspdata.SerializeToArray(data, len);//data
			std::string const& typeName = rspdata.GetTypeName();//typename
			m_pTableFrame->SendTableData(chairId, s13s::SUB_S_CANCELCARDS, data, len);
			delete[] data;
			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(rspdata, jsonstr, true);
			//printf("\n--- *** %s\n", typeName.c_str());
			//printf("%s\n\n", jsonstr.c_str());
		}
		break;
	}
	//确定牌型
	case s13s::SUB_C_MAKESUREDUNHANDTY: {
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId); assert(userItem != NULL);
		//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_PLAYING);
		//////////////////////////////////////////////////////////////
		//客户端请求数据
		s13s::CMD_C_MakesureDunHandTy reqdata;
		if (!reqdata.ParseFromArray(pDataBuffer, dwDataSize)) {
			return false;
		}
		{
			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(reqdata, jsonstr, true);
			//std::string const& typeName = reqdata.GetTypeName();//typename
			//printf("\n--- *** %s\n", typeName.c_str());
			//printf("%s\n\n", jsonstr.c_str());
		}
		//////////////////////////////////////////////////////////////
		//玩家已经确认过牌型
		if (phandInfos_[chairId]->HasSelected()) {
			//assert(false);
			printf("--- *** 玩家已经确认过牌型 ...\n");
			return false;
		}
		//////////////////////////////////////////////////////////////
		//是否进行过手动任意摆牌
		if (reqdata.groupindex() == -1 && !phandInfos_[chairId]->HasManualGroup()) {
			assert(phandInfos_[chairId]->GetManualC() < MAX_COUNT);
			return false;
		}
		//////////////////////////////////////////////////////////////
		//确认玩家手牌三墩牌型
		if (!phandInfos_[chairId]->Select(reqdata.groupindex())) {
			//assert(false);
			return false;
		}
		//获取玩家摆好的牌
		//客户端选择了哪些牌
		uint8_t cards[MAX_COUNT] = { 0 };
		//客户端选择了几张牌
		int len = 0;
		for (int i = S13S::DunFirst; i <= S13S::DunLast; ++i) {
			int cardcount = phandInfos_[chairId]->GetSelected()->duns[i].GetC();
			for (int j = 0;j < cardcount; ++j)
			{
				cards[len++] = phandInfos_[chairId]->GetSelected()->duns[i].cards[j];
			}
		}
		//牌值是否有效
		if (!bCheckCardIsValid(cards, len))
		{
			string str = getCardString(cards, len);
			LOG(WARNING) << "=== 传过来的牌值有无效值： " << str << "  userid:" << userItem->GetUserId();
			return false;
		}
        if (!bCheckCard(handCards_[chairId], cards, len))
		{
			LOG(WARNING) << "=== 选牌检验出错了====" << "  userid:" << userItem->GetUserId();
			return false;
		}

		//////////////////////////////////////////////////////////////
		//对局日志详情
		{
			//单墩牌(3/5/5)
			std::string strDunCards;
			for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
				if (strDunCards.empty()) {
					strDunCards = S13S::CGameLogic::StringCards(
						phandInfos_[chairId]->GetSelected()->duns[d].cards,
						phandInfos_[chairId]->GetSelected()->duns[d].GetC());
				}
				else {
					strDunCards += "," +
						S13S::CGameLogic::StringCards(
							phandInfos_[chairId]->GetSelected()->duns[d].cards,
							phandInfos_[chairId]->GetSelected()->duns[d].GetC());
				}
			}
			m_replay.addStep((uint32_t)time(NULL) - groupStartTime_, strDunCards, -1, 0, chairId, chairId);
		}
		//////////////////////////////////////////////////////////////
		//理牌完成广播状态
		s13s::CMD_S_MakesureDunHandTy rspdata;
		rspdata.set_chairid(chairId);
		string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, s13s::SUB_S_MAKESUREDUNHANDTY, (uint8_t *)content.data(), content.size());
		{
			//转换json格式
			//std::string jsonstr;
			//PB2JSON::Pb2Json::PbMsg2JsonStr(rspdata, jsonstr, true);
			//std::string const& typeName = rspdata.GetTypeName();//typename
			//printf("\n--- *** %s\n", typeName.c_str());
			//printf("%s\n\n", jsonstr.c_str());
		}
		//////////////////////////////////////////////////////////////
		//检查是否都确认牌型了
		if (++selectcount < m_pTableFrame->GetPlayerCount(true)) {
			return true;
		}
		//玩家之间两两比牌
		StartCompareCards();
		break;
	}
    }
    return false;
}

//玩家之间两两比牌
void CGameProcess::StartCompareCards() {
	ThisThreadTimer->cancel(timerIdManual_);
	//如果开始比牌返回
	if (isCompareCards_) {
		return;
	}
	assert(!isCompareCards_);
	//标记开始比牌
	isCompareCards_ = true;
	//////////////////////////////////////////////////////////////
	//玩家之间两两比牌，头墩与头墩比，中墩与中墩比，尾墩与尾墩比
	//s13s::CMD_S_CompareCards compareCards_[GAME_PLAYER];
	for (int i = 0; i < GAME_PLAYER; ++i) {
		compareCards_[i].Clear();
	}
	//////////////////////////////////////////////////////////////
	//所有玩家牌型都确认了，比牌
	int c = 0;
	int chairIDs[GAME_PLAYER] = { 0 };
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId); assert(userItem);
			//用于求两两组合
			chairIDs[c++] = i;
			//比牌玩家桌椅ID
			compareCards_[i].mutable_player()->set_chairid(i);
			//获取座椅玩家确定的三墩牌型
			S13S::CGameLogic::groupdun_t const *player_select = phandInfos_[i]->GetSelected();
			//桌椅玩家选择一组墩(含头墩/中墩/尾墩)
			s13s::GroupDunData* player_group = compareCards_[i].mutable_player()->mutable_group();
			//从哪墩开始的
			player_group->set_start(S13S::DunFirst);
			//总体对应特殊牌型 ////////////
			player_group->set_specialty(player_select->specialTy);
			//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
			for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				s13s::DunData* player_dun_i = player_group->add_duns();
				//标记0-头/1-中/2-尾
				player_dun_i->set_id(d);
				//墩对应普通牌型
				player_dun_i->set_ty(
					player_select->specialTy >= S13S::TyThreesc ?
					player_select->specialTy :
					player_select->duns[d].ty_);
				//墩对应牌数c(3/5/5)
				player_dun_i->set_c(player_select->duns[d].c);
				//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
				player_dun_i->set_cards(player_select->duns[d].cards, player_select->duns[d].c);
			}
		}
	}
	assert(c >= MIN_GAME_PLAYER);
	std::vector<std::vector<int>> vec;
	CFuncC fnC;
	fnC.FuncC(c, 2, vec);
	//CFuncC::Print(vec);
	for (std::vector<std::vector<int>>::const_iterator it = vec.begin();
		it != vec.end(); ++it) {
		assert(it->size() == 2);//两两比牌
		int src_chairid = chairIDs[(*it)[0]];//src椅子ID
		int dst_chairid = chairIDs[(*it)[1]];//dst椅子ID
		assert(src_chairid < GAME_PLAYER);
		assert(dst_chairid < GAME_PLAYER);
		//获取src确定的三墩牌型
		S13S::CGameLogic::groupdun_t const *src = phandInfos_[src_chairid]->GetSelected();
		//获取dst确定的三墩牌型
		S13S::CGameLogic::groupdun_t const *dst = phandInfos_[dst_chairid]->GetSelected();
		{
			//追加src比牌对象 ////////////
			s13s::ComparePlayer* src_peer = compareCards_[src_chairid].add_peers();
			{
				//比牌对象桌椅ID
				src_peer->set_chairid(dst_chairid);
				//比牌对象选择一组墩
				s13s::GroupDunData* src_peer_select = src_peer->mutable_group();
				//从哪墩开始的
				src_peer_select->set_start(S13S::DunFirst);
				//总体对应特殊牌型 ////////////
				src_peer_select->set_specialty(dst->specialTy);
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				for (int i = S13S::DunFirst; i <= S13S::DunLast; ++i) {
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					s13s::DunData* src_peer_dun_i = src_peer_select->add_duns();
					//标记0-头/1-中/2-尾
					src_peer_dun_i->set_id(i);
					//墩对应普通牌型
					src_peer_dun_i->set_ty(
						dst->specialTy >= S13S::TyThreesc ?
						dst->specialTy :
						dst->duns[i].ty_);
					//墩对应牌数c(3/5/5)
					src_peer_dun_i->set_c(dst->duns[i].c);
					//墩牌数据(头墩(3)/中墩(5)/尾墩(5))
					src_peer_dun_i->set_cards(dst->duns[i].cards, dst->duns[i].c);
				}
			}
			//追加src比牌结果 ////////////
			s13s::CompareResult* src_result = compareCards_[src_chairid].add_results();
		}
		{
			//追加dst比牌对象 ////////////
			s13s::ComparePlayer* dst_peer = compareCards_[dst_chairid].add_peers();
			{
				//比牌对象桌椅ID
				dst_peer->set_chairid(src_chairid);
				//比牌对象选择一组墩
				s13s::GroupDunData* dst_peer_select = dst_peer->mutable_group();
				//从哪墩开始的
				dst_peer_select->set_start(S13S::DunFirst);
				//总体对应特殊牌型 ////////////
				dst_peer_select->set_specialty(src->specialTy);
				//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
				for (int i = S13S::DunFirst; i <= S13S::DunLast; ++i) {
					//[0]头墩(3)/[1]中墩(5)/[2]尾墩(5)
					s13s::DunData* dst_peer_dun_i = dst_peer_select->add_duns();
					//标记0-头/1-中/2-尾
					dst_peer_dun_i->set_id(i);
					//墩对应普通牌型
					dst_peer_dun_i->set_ty(
						src->specialTy >= S13S::TyThreesc ?
						src->specialTy :
						src->duns[i].ty_);
					//墩对应牌数c(3/5/5)
					dst_peer_dun_i->set_c(src->duns[i].c);
					//墩牌数据(头墩3张)
					dst_peer_dun_i->set_cards(src->duns[i].cards, src->duns[i].c);
				}
			}
			//追加dst比牌结果 ////////////
			s13s::CompareResult* dst_result = compareCards_[dst_chairid].add_results();
		}
		//////////////////////////////////////////////////////////////
		//比较特殊牌型
		if (src->specialTy >= S13S::TyThreesc || dst->specialTy >= S13S::TyThreesc) {
			int winner = -1, loser = -1;
			if (src->specialTy != dst->specialTy) {
				//牌型不同比牌型
				if (src->specialTy > dst->specialTy) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->specialTy < dst->specialTy) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同，和了
			}
			if (winner == -1) {
				//和了
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//src输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//dst输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//至尊青龙获胜赢32水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyZZQDragon) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(32);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-32);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//一条龙获胜赢30水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyOneDragon) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(30);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-30);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//十二皇族获胜赢24水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::Ty12Royal) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(24);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-24);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三同花顺获胜赢20水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThree123sc) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(20);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-20);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三分天下获胜赢20水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThree40) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(20);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-20);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//全大获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyAllBig) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(10);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-10);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//全小获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyAllSmall) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(10);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-10);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//凑一色获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyAllOneColor) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(10);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-10);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//双怪冲三获胜赢8水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyTwo3220) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(8);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-8);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//四套三条获胜赢6水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyFour30) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(6);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-6);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//五对三条获胜赢5水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyFive2030) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(5);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-5);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//六对半获胜赢4水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TySix20) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(4);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-4);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三顺子获胜赢4水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThree123) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//赢家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(4);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-4);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			//三同花获胜赢3水
			else if (phandInfos_[winner]->GetSelected()->specialTy == S13S::TyThreesc) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(3);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->specialTy);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//输家输赢信息
					s13s::CompareItem* item = result->mutable_specitem();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-3);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->specialTy);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->specialTy);
					}
				}
			}
			continue;
		}
		//////////////////////////////////////////////////////////////
		//比较头墩
		{
			//单墩比牌，头墩要么三条/对子/乌龙，同花顺/同花/顺子都要改成乌龙
			if (src->duns[S13S::DunFirst].ty_ == S13S::Ty123sc ||
				src->duns[S13S::DunFirst].ty_ == S13S::Tysc ||
				src->duns[S13S::DunFirst].ty_ == S13S::Ty123) {
				//assert(src->duns[S13S::DunFirst].ty_ == S13S::Tysp);
				const_cast<S13S::CGameLogic::groupdun_t*>(src)->duns[S13S::DunFirst].ty_ = S13S::Tysp;
			}
			//单墩比牌，头墩要么三条/对子/乌龙，同花顺/同花/顺子都要改成乌龙
			if (dst->duns[S13S::DunFirst].ty_ == S13S::Ty123sc ||
				dst->duns[S13S::DunFirst].ty_ == S13S::Tysc ||
				dst->duns[S13S::DunFirst].ty_ == S13S::Ty123) {
				//assert(dst->duns[S13S::DunFirst].ty_ == S13S::Tysp);
				const_cast<S13S::CGameLogic::groupdun_t*>(dst)->duns[S13S::DunFirst].ty_ = S13S::Tysp;
			}
			int winner = -1, loser = -1;
			int dt = S13S::DunFirst;
			if (src->duns[dt].ty_ != dst->duns[dt].ty_) {
				//牌型不同比牌型
				if (src->duns[dt].ty_ > dst->duns[dt].ty_) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->duns[dt].ty_ < dst->duns[dt].ty_) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同比大小
				assert(src->duns[dt].GetC() == 3);
				assert(dst->duns[dt].GetC() == 3);
				int bv = S13S::CGameLogic::CompareCards(
					src->duns[dt].cards, dst->duns[dt].cards, dst->duns[dt].GetC(), false, dst->duns[dt].ty_);
				if (bv > 0) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (bv < 0) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					//头墩和
					//assert(false);
				}
			}
			if (winner == -1) {
				//头墩和
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			//乌龙/对子获胜赢1水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysp ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty20) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//头墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(1);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//头墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-1);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
			}
			//三条摆头墩获胜赢3水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty30) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(3);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//头墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-3);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			else {
				assert(false);
			}
		}
		//////////////////////////////////////////////////////////////
		//比较中墩
		{
			int winner = -1, loser = -1;
			int dt = S13S::DunSecond;
			if (src->duns[dt].ty_ != dst->duns[dt].ty_) {
				//牌型不同比牌型
				if (src->duns[dt].ty_ > dst->duns[dt].ty_) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->duns[dt].ty_ < dst->duns[dt].ty_) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同比大小
				assert(src->duns[dt].GetC() == 5);
				assert(dst->duns[dt].GetC() == 5);
				int bv = S13S::CGameLogic::CompareCards(
					src->duns[dt].cards, dst->duns[dt].cards, dst->duns[dt].GetC(), false, dst->duns[dt].ty_);
				if (bv > 0) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (bv < 0) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					//中墩和
					//assert(false);
				}
			}
			if (winner == -1) {
				//中墩和
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			//乌龙/对子/两对/三条/顺子/同花获胜赢1水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysp ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty20 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty22 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty30 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysc) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(1);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//中墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-1);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
			}
			//葫芦摆中墩获胜赢2水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty32) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(2);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-2);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			//铁支摆中墩获胜赢8水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty40) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(8);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-8);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			//同花顺摆中墩获胜赢10水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123sc) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(10);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//中墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-10);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			else {
				assert(false);
			}
		}
		//////////////////////////////////////////////////////////////
		//比较尾墩
		{
			int winner = -1, loser = -1;
			int dt = S13S::DunLast;
			if (src->duns[dt].ty_ != dst->duns[dt].ty_) {
				//牌型不同比牌型
				if (src->duns[dt].ty_ > dst->duns[dt].ty_) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (src->duns[dt].ty_ < dst->duns[dt].ty_) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					assert(false);
				}
			}
			else {
				//牌型相同比大小
				assert(src->duns[dt].GetC() == 5);
				assert(dst->duns[dt].GetC() == 5);
				int bv = S13S::CGameLogic::CompareCards(
					src->duns[dt].cards, dst->duns[dt].cards, dst->duns[dt].GetC(), false, dst->duns[dt].ty_);
				if (bv > 0) {
					winner = src_chairid; loser = dst_chairid;
				}
				else if (bv < 0) {
					winner = dst_chairid; loser = src_chairid;
				}
				else {
					//尾墩和
					//assert(false);
				}
			}
			if (winner == -1) {
				//尾墩和
				winner = src_chairid; loser = dst_chairid;
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//src比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//dst比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//和了
						item->set_winlost(0);
						//和分
						item->set_score(0);
						//和的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			//乌龙/对子/两对/三条/顺子/同花/葫芦获胜赢1水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysp ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty20 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty22 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty30 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123 ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Tysc ||
				phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty32) {
					{
						int index = compareCards_[winner].results_size() - 1;
						assert(index >= 0);
						//赢家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//赢了
							item->set_winlost(1);
							//赢分
							item->set_score(1);
							//赢的牌型
							item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						}
					}
					{
						int index = compareCards_[loser].results_size() - 1;
						assert(index >= 0);
						//输家比牌结果 ////////////
						s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
						//尾墩输赢信息
						s13s::CompareItem* item = result->add_items();
						{
							//输了
							item->set_winlost(-1);
							//输分
							item->set_score(-1);
							//输的牌型
							item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
							//对方牌型
							item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						}
					}
			}
			//铁支摆尾墩获胜赢4水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty40) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(4);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-4);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			//同花顺摆尾墩获胜赢5水
			else if (phandInfos_[winner]->GetSelected()->duns[dt].ty_ == S13S::Ty123sc) {
				{
					int index = compareCards_[winner].results_size() - 1;
					assert(index >= 0);
					//赢家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[winner].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//赢了
						item->set_winlost(1);
						//赢分
						item->set_score(5);
						//赢的牌型
						item->set_ty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
					}
				}
				{
					int index = compareCards_[loser].results_size() - 1;
					assert(index >= 0);
					//输家比牌结果 ////////////
					s13s::CompareResult* result = compareCards_[loser].mutable_results(index);
					//尾墩输赢信息
					s13s::CompareItem* item = result->add_items();
					{
						//输了
						item->set_winlost(-1);
						//输分
						item->set_score(-5);
						//输的牌型
						item->set_ty(phandInfos_[loser]->GetSelected()->duns[dt].ty_);
						//对方牌型
						item->set_peerty(phandInfos_[winner]->GetSelected()->duns[dt].ty_);
					}
				}
			}
			else {
				{
					assert(false);
				}
			}
		}
	}
	//玩家对其它玩家打枪
	std::map<uint8_t, std::vector<uint8_t>> shootIds;
	//统计判断打枪/全垒打
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			//判断是否全垒打
			int shootc = 0;
			//当前比牌玩家
			s13s::PlayerItem const& player = compareCards_[i].player();
			//遍历玩家所有比牌对象
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				s13s::CompareResult const& result = compareCards_[i].results(j);
				int winc = 0, lostc = 0, sumscore = 0;
				//自己特殊牌型或对方特殊牌型
				assert(result.items_size() == (
					player.group().specialty() >= S13S::TyThreesc ||
					peer.group().specialty() >= S13S::TyThreesc) ? 0 : 3);
				//玩家与当前比牌对象比头/中/尾三墩输赢得水总分，不考虑打枪
				for (int d = 0; d < result.items_size(); ++d) {
					if (result.items(d).winlost() == 1) {
						++winc;
					}
					else if (result.items(d).winlost() == -1) {
						++lostc;
					}
					sumscore += result.items(d).score();
				}
				//三墩不考虑打枪输赢得水总分 赢分+/和分0/输分-
				const_cast<s13s::CompareResult&>(result).set_score(sumscore);
				//特殊牌型不参与打枪
				if (player.group().specialty() >= S13S::TyThreesc ||
					peer.group().specialty() >= S13S::TyThreesc) {
					const_cast<s13s::CompareResult&>(result).set_shoot(0);//-1被打枪/0不打枪/1打枪
					continue;
				}
				if (winc == result.items_size()) {
					//玩家三墩全部胜过比牌对象，则玩家对比牌对象打枪，中枪者付给打枪者2倍的水
					const_cast<s13s::CompareResult&>(result).set_shoot(1);//-1被打枪/0不打枪/1打枪
					//统计当前玩家打枪次数
					++shootc;
					//玩家对比牌对象打枪
					shootIds[i].push_back(peer.chairid());
				}
				else if (lostc == result.items_size()) {
					//比牌对象三墩全部胜过玩家，则比牌对象对玩家打枪，中枪者付给打枪者2倍的水
					const_cast<s13s::CompareResult&>(result).set_shoot(-1);//-1被打枪/0不打枪/1打枪
				}
				else {
					const_cast<s13s::CompareResult&>(result).set_shoot(0);//-1被打枪/0不打枪/1打枪
				}
			}
			if (shootc == compareCards_[i].peers_size() && compareCards_[i].peers_size() > 1) {
				//全垒打，玩家三墩全部胜过其它玩家，且至少打2枪，中枪者付给打枪者4倍的水
				compareCards_[i].set_allshoot(1);//-1被全垒打/0无全垒打/1全垒打
				//其它比牌对象都是被全垒打
				for (int k = 0; k < GAME_PLAYER; ++k) {
					if (k != i) {
						if (m_pTableFrame->IsExistUser(k)) {
							//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(k); assert(userItem);
							//-1被全垒打/0无全垒打/1全垒打
							compareCards_[k].set_allshoot(-1);
							//allshoot=-1被全垒打，记下全垒打桌椅ID
							compareCards_[k].set_fromchairid(i);
						}
					}
				}
			}
		}
	}
	//其它玩家之间打枪
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//当前比牌玩家
			s13s::PlayerItem const& player = compareCards_[i].player();
			//遍历玩家所有比牌对象
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				//s13s::CompareResult const& result = compareCards_[i].results(j);
				std::map<uint8_t, std::vector<uint8_t>>::const_iterator it = shootIds.find(peer.chairid());
				if (it != shootIds.end()) {
					for (std::vector<uint8_t>::const_iterator ir = it->second.begin();
						ir != it->second.end(); ++ir) {
						//排除当前玩家
						if (*ir != i) {
							assert(player.chairid() == i);
							const_cast<s13s::ComparePlayer&>(peer).add_shootids(*ir);
						}
					}
				}
			}
		}
	}
	//计算包括打枪/全垒打在内输赢得水总分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			//玩家输赢得水总分，不含特殊牌型输赢分
			int deltascore = 0;
			//遍历玩家所有比牌对象
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				s13s::CompareResult const& result = compareCards_[i].results(j);
				//1打枪(对当前比牌对象打枪)
				if (result.shoot() == 1) {
					//1全垒打
					if (compareCards_[i].allshoot() == 1) {
						//-1被全垒打/0无全垒打/1全垒打
						deltascore += 4 * result.score();
					}
					else {
						//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
						if (compareCards_[i].allshoot() == -1) {
						}
						else {
						}
						//-1被打枪/0不打枪/1打枪
						deltascore += 2 * result.score();
					}
				}
				//-1被打枪(被当前比牌对象打枪)
				else if (result.shoot() == -1) {
					//-1被全垒打
					if (compareCards_[i].allshoot() == -1) {
						//被当前比牌对象全垒打
						if (peer.chairid() == compareCards_[i].fromchairid()) {
							//-1被全垒打/0无全垒打/1全垒打
							deltascore += 4 * result.score();
						}
						//被另外比牌对象全垒打
						else {
							//-1被打枪/0不打枪/1打枪
							deltascore += 2 * result.score();
						}
					}
					else {
						//-1被打枪/0不打枪/1打枪
						deltascore += 2 * result.score();
					}
				}
				//0不打枪(与当前比牌对象互不打枪)
				else {
					//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
					if (compareCards_[i].allshoot() == -1) {
					}
					else {
						//一定不是全垒打，-1被打枪/0不打枪/1打枪
						assert(compareCards_[i].allshoot() == 0);
					}
					assert(result.shoot() == 0);
					deltascore += result.score();
				}
			}
			//玩家输赢得水总分，不含特殊牌型输赢分
			compareCards_[i].set_deltascore(deltascore);
		}
	}
	//座椅玩家与其余玩家按墩比输赢分
	//不含打枪/全垒打与打枪/全垒打分开计算
	//计算特殊牌型输赢分
	//统计含打枪/全垒打/特殊牌型输赢得水总分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			//三墩打枪/全垒打输赢算水
			int allshootscore = 0;
			//当前比牌玩家
			s13s::PlayerItem const& player = compareCards_[i].player();
			//遍历各墩(头墩/中墩/尾墩)
			for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
				//按墩比输赢分
				int sumscore = 0;
				//按墩计算打枪输赢分
				int sumshootscore = 0;
				{
					//自己特殊牌型，不按墩比牌，输赢水数在specitem中
					if (player.group().specialty() >= S13S::TyThreesc) {
					}
					else {
						//遍历比牌对象
						for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
							s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
							s13s::CompareResult const& result = compareCards_[i].results(j);
							//对方特殊牌型，不按墩比牌，输赢水数在specitem中
							if (peer.group().specialty() >= S13S::TyThreesc) {
								continue;
							}
							//累加指定墩输赢得水积分，不含打枪/全垒打
							sumscore += result.items(d).score();
							//1打枪(对当前比牌对象打枪)
							if (result.shoot() == 1) {
								//1全垒打
								if (compareCards_[i].allshoot() == 1) {
									//-1被全垒打/0无全垒打/1全垒打
									sumshootscore += 3/*4*/ * result.items(d).score();
								}
								else {
									//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
									if (compareCards_[i].allshoot() == -1) {
									}
									else {
									}
									//-1被打枪/0不打枪/1打枪
									sumshootscore += 1/*2*/ * result.items(d).score();
								}
							}
							//-1被打枪(被当前比牌对象打枪)
							else if (result.shoot() == -1) {
								//-1被全垒打
								if (compareCards_[i].allshoot() == -1) {
									//被当前比牌对象全垒打
									if (peer.chairid() == compareCards_[i].fromchairid()) {
										//-1被全垒打/0无全垒打/1全垒打
										sumshootscore += 3/*4*/ * result.items(d).score();
									}
									//被另外比牌对象全垒打
									else {
										//-1被打枪/0不打枪/1打枪
										sumshootscore += 1/*2*/ * result.items(d).score();
									}
								}
								else {
									//-1被打枪/0不打枪/1打枪
									sumshootscore += 1/*2*/ * result.items(d).score();
								}
							}
							//0不打枪(与当前比牌对象互不打枪)
							else {
								//-1被全垒打(被另外比牌对象打枪，并且该比牌对象是全垒打)
								if (compareCards_[i].allshoot() == -1) {
								}
								else {
									//一定不是全垒打，-1被打枪/0不打枪/1打枪
									assert(compareCards_[i].allshoot() == 0);
								}
								assert(result.shoot() == 0);
								//sumshootscore += 0/*1*/ * result.items(d).score();
							}
						}
					}
				}
				//座椅玩家输赢得水积分(头/中/尾/特殊)
				compareCards_[i].add_itemscores(sumscore);
				//按墩计算打枪/全垒打输赢分
				compareCards_[i].add_itemscorepure(sumshootscore);
				//三墩打枪/全垒打输赢算水
				allshootscore += sumshootscore;
			}
			{
				//特殊牌型输赢算水(无打枪/全垒打)
				int sumspecscore = 0;
				{
					//遍历比牌对象
					for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
						s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
						s13s::CompareResult const& result = compareCards_[i].results(j);
						//自己普通牌型，对方特殊牌型
						//自己特殊牌型，对方普通牌型
						//自己特殊牌型，对方特殊牌型
						if (result.has_specitem()) {
							//累加特殊比牌算水
							sumspecscore += result.specitem().score();
						}
					}
				}
				//座椅玩家输赢得水积分(头/中/尾/特殊)
				compareCards_[i].add_itemscores(sumspecscore);
				//三墩打枪/全垒打输赢算水 + 特殊牌型输赢算水(无打枪/全垒打)
				compareCards_[i].add_itemscorepure(allshootscore + sumspecscore);
				//玩家输赢得水总分，含打枪/全垒打，含特殊牌型输赢分
				int32_t deltascore = compareCards_[i].deltascore();
				compareCards_[i].set_deltascore(deltascore + sumspecscore);
			}
		}
	}
	//比牌对方输赢得水总分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			for (int j = 0; j < compareCards_[i].peers_size(); ++j) {
				s13s::ComparePlayer const& peer = compareCards_[i].peers(j);
				int32_t deltascore = compareCards_[peer.chairid()].deltascore();
				const_cast<s13s::ComparePlayer&>(peer).set_deltascore(deltascore);
			}
		}
	}
	//游戏比牌状态
	gameStatus_ = GAME_STATUS_COMPARE;
	//比牌游戏结束
	OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
	//json格式在线view工具：http://www.bejson.com/jsonviewernew/
	//玩家之间两两比牌输赢得水总分明细，包括打枪/全垒打
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (m_pTableFrame->IsExistUser(i)) {
			//shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			{
				//序列化std::string
				std::string content = compareCards_[i].SerializeAsString();//data
				std::string const& typeName = compareCards_[i].GetTypeName();//typename
				//广播各玩家比牌结果
				//if (!m_pTableFrame->IsAndroidUser(i)) {
				//	m_pTableFrame->SendTableData(i, s13s::SUB_S_COMPARECARDS, (uint8_t *)content.data(), content.size());
				//}
				{
					//转换json格式
					//std::string jsonstr;
					//PB2JSON::Pb2Json::PbMsg2JsonStr(compareCards_[i], jsonstr, true);
					//printf("\n--- *** {\"%s\":\n", typeName.c_str());
					//printf("%s}\n\n", jsonstr.c_str());
				}
			}
			{
				//序列化bytes
				//int len = compareCards_[i].ByteSizeLong();//len
				//uint8_t *data = new uint8_t[len];
				//compareCards_[i].SerializeToArray(data, len);//data
				//std::string const& typeName = compareCards_[i].GetTypeName();//typename
				//广播玩家比牌结果
				//if (!m_pTableFrame->IsAndroidUser(i)) {
				//	m_pTableFrame->SendTableData(i, s13s::SUB_S_COMPARECARDS, data, len);
				//}
				//delete[] data;
				//{
				//	//转换json格式
				//	std::string jsonstr;
				//	PB2JSON::Pb2Json::PbMsg2JsonStr(compareCards_[i], jsonstr, true);
				//	printf("\n--- *** {\"%s\":\n", typeName.c_str());
				//	printf("%s}\n\n", jsonstr.c_str());
				//}
			}
		}
	}
}

int64_t CGameProcess::CalculateAndroidRevenue(int64_t score)
{
	//return m_pTableFrame->CalculateRevenue(score);
    return score * (m_pTableFrame->GetGameRoomInfo()->systemReduceRatio) / 1000.0;
}

//游戏结束
bool CGameProcess::OnEventGameConclude(uint32_t chairId, uint8_t reason)
{
	//游戏结束时间
	roundEndTime_ = chrono::system_clock::now();
	//游戏结束命令
	s13s::CMD_S_GameEnd GameEnd;
	//计算玩家积分
	std::vector<tagScoreInfo> scoreInfos;
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[4];
	//记录当局游戏详情binary
	s13s::CMD_S_GameRecordDetail details;
	details.set_gameid(ThisGameId);
	//玩家手牌字符串，根据选择的墩牌型拼接手牌
	//3b2c110,04352607384,323d2d1d0d7,3,10;342b0b1,2336062a1a2,22123929096,0,8;
	//一个分号";"分割一个玩家，每个分号内用逗号","隔开玩家的3墩牌，分号前一位表示玩家选牌结果，玩家选牌结果前一位表示玩家座位号(0/1/2/3)
	//两位表示1张牌，第1位是花色，第2位数是牌值，头墩3张，中墩和尾墩各5张，每墩最后一位字符表示牌型
	std::string strCardsList;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		s13s::PlayerRecordDetail* detail = details.add_detail();
		//boost::property_tree::ptree item, itemscores, itemscorePure;
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
		{
			//玩家手牌字符串，根据选择的墩牌型拼接手牌
			S13S::CGameLogic::groupdun_t const* group_select = phandInfos_[i]->GetSelected();
			std::string strCards/*, handcards*/;
			for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
				//单墩牌(3/5/5)
				std::string cards, ty;
				{
					//牌数据
					for (int k = 0; k < group_select->duns[d].c; ++k) {
						char ch[10] = { 0 };
						uint8_t color = S13S::CGameLogic::GetCardColor(group_select->duns[d].cards[k]);
						uint8_t value = S13S::CGameLogic::GetCardValue(group_select->duns[d].cards[k]);
						uint8_t card = S13S::CGameLogic::MakeCardWith(color - 0x10, value);
						sprintf(ch, "%02X", card);
						cards += ch;
					}
					//牌型
					ty = std::to_string(group_select->duns[d].ty_);
				}
				if (d == S13S::DunFirst) {
					strCards += cards + ty;
					//handcards += ty + ":" + cards;//ty:cards
				}
				else {
					strCards += "," + cards + ty;
					//handcards += "|" + ty + ":" + cards;//ty:cards|ty:cards|ty:cards
				}
				s13s::DunCards* pcards = detail->add_handcards();
				pcards->set_ty(group_select->duns[d].ty_);
				pcards->set_cards(group_select->duns[d].cards, group_select->duns[d].c);
				//头/中/尾/特殊，不含打枪/全垒打
				//item.put_value(compareCards_[i].itemscores(d));
				//itemscores.push_back(std::make_pair("", item));
				detail->add_itemscores(compareCards_[i].itemscores(d));
				//头/中/尾/特殊，含打枪/全垒打
				//item.put_value(compareCards_[i].itemscorepure(d));
				//itemscorePure.push_back(std::make_pair("", item));
				detail->add_itemscorepure(compareCards_[i].itemscorepure(d));
			}
			if (compareCards_[i].itemscores_size() > S13S::DunMax) {
				//头/中/尾/特殊，不含打枪/全垒打
				//item.put_value(compareCards_[i].itemscores(3));
				//itemscores.push_back(std::make_pair("", item));
				detail->add_itemscores(compareCards_[i].itemscores(3));
			}
			if (compareCards_[i].itemscorepure_size() > S13S::DunMax) {
				//头/中/尾/特殊，含打枪/全垒打
				//item.put_value(compareCards_[i].itemscorepure(3));
				//itemscorePure.push_back(std::make_pair("", item));
				detail->add_itemscorepure(compareCards_[i].itemscorepure(3));
			}
			//座位号
			strCards += "," + std::to_string(i);
			//选牌结果
			strCards += "," + std::to_string((int)phandInfos_[i]->GetSelectResult());
			//座椅玩家手牌信息
			if (strCardsList.empty()) {
				strCardsList += strCards;
			}
			else {
				strCardsList += ";" + strCards;
			}
			//账号/昵称
			//player[i].put("account", std::to_string(UserIdBy(i)));
			detail->set_account(std::to_string(UserIdBy(i)));
			//座椅ID
			//player[i].put("chairid", i);
			detail->set_chairid(i);
			//手牌/牌型 ty:cards|ty:cards|ty:cards
			//player[i].put("handcards", handcards.c_str());
			//携带金额
			//player[i].put("userscore", ScoreByChairId(i));
			detail->set_userscore(ScoreByChairId(i));
			//房间底注
			//player[i].put("cellscore", CellScore);
			detail->set_cellscore(CellScore);
			//输赢得水
			//player[i].put("deltascore", compareCards_[i].deltascore());
			detail->set_deltascore(compareCards_[i].deltascore());
			//头/中/尾/特殊，不含打枪/全垒打
			//player[i].add_child("itemscores", itemscores);
			//头/中/尾/特殊，含打枪/全垒打
			//player[i].add_child("itemscorePure", itemscorePure);
		}
	}
	switch (reason)
	{
		//正常退出
	case GER_NORMAL: {
		//系统抽水
		int64_t revenue[GAME_PLAYER] = { 0 };
		//携带积分
		int64_t userScore[GAME_PLAYER] = { 0 };
		//输赢积分
		int64_t userWinScore[GAME_PLAYER] = { 0 };
		//输赢积分，扣除系统抽水
		int64_t userWinScorePure[GAME_PLAYER] = { 0 };
		//剩余积分
		int64_t userLeftScore[GAME_PLAYER] = { 0 };
		//系统输赢积分
		int64_t systemWinScore = 0;
		//桌面输赢积分
		int64_t totalWinScore = 0, totalLostScore = 0;
		//桌面输赢人数
		int winners = 0, losers = 0;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			{
				//携带积分
				userScore[i] = userItem->GetUserScore();
				//输赢积分
				userWinScore[i] = compareCards_[i].deltascore() * FloorScore;
				//输赢分取绝对值最小
				if (userScore[i] < llabs(userWinScore[i])) {
					//玩家输赢积分
					userWinScore[i] = userWinScore[i] < 0 ?
						-userScore[i] : userScore[i];
				}
				if (userWinScore[i] < 0) {
					//桌面输的积分/人数
					totalLostScore += userWinScore[i];
					++losers;
				}
				else {
					//桌面赢的积分/人数
					totalWinScore += userWinScore[i];
					++winners;
				}
			}
		}
		assert(totalWinScore >= 0);
		assert(totalLostScore <= 0);
		//桌面输赢分取绝对值最小(|X|和|Y|比较大小)
		if (llabs(totalLostScore) < totalWinScore) {
			//|X| < |Y| 玩家赢分换算
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
				{
					if (userWinScore[i] > 0) {
						//以桌面输分|X|为标准按赢分比例计算赢分玩家赢的积分
						//玩家实际赢分 = |X| * 玩家赢分占比(玩家赢分 / |Y|)
						userWinScore[i] = llabs(totalLostScore) * userWinScore[i] / totalWinScore;
					}
				}
			}
		}
		else {
			//|X| > |Y| 玩家输分换算
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
				{
					if (userWinScore[i] < 0) {
						//以桌面赢分|Y|为标准按输分比例计算输分玩家输的积分
						//玩家实际输分 = |Y| * 玩家输分占比(玩家输分 / |X|)
						userWinScore[i] = totalWinScore * userWinScore[i] / llabs(totalLostScore);
					}
				}
			}
		}
		//计算输赢积分
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			{
				//携带积分
				//userScore[i] = userItem->GetUserScore();
				//输赢积分
				//userWinScore[i] = compareCards_[i].deltascore() * FloorScore;
				//盈利玩家
				if (userWinScore[i] > 0) {
					//系统抽水，真实玩家和机器人都按照5%抽水计算，前端显示和玩家扣分一致
					revenue[i] = m_pTableFrame->CalculateRevenue(userWinScore[i]);
				}
				//输赢积分，扣除系统抽水
				userWinScorePure[i] = userWinScore[i] - revenue[i];
				//剩余积分
				userLeftScore[i] = userScore[i] + userWinScorePure[i];
				//若是机器人AI
				if (m_pTableFrame->IsAndroidUser(i)) {
#ifdef _STORAGESCORE_SEPARATE_STAT_
					//系统输赢积分，扣除系统抽水
					systemWinScore += userWinScorePure[i];
#else
					//系统输赢积分
					systemWinScore += userWinScore[i];
#endif
				}
				//计算积分
				tagScoreInfo scoreInfo;
				scoreInfo.chairId = i;//座椅ID
				scoreInfo.cardValue = strCardsList;//本局开牌
				scoreInfo.rWinScore = llabs(userWinScore[i]);//税前输赢
				scoreInfo.addScore = userWinScorePure[i];//本局输赢
				scoreInfo.betScore = llabs(userWinScore[i]);//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = revenue[i];//本局税收
				scoreInfo.startTime = roundStartTime_;//本局开始时间
				scoreInfos.push_back(scoreInfo);
				//玩家积分
				s13s::GameEndScore* score = GameEnd.add_scores();
				score->set_chairid(i);
				//前端显示机器人和真实玩家抽水一致
				score->set_score(userWinScorePure[i]);
				score->set_userscore(userLeftScore[i]);
				//跑马灯信息
				if (userWinScorePure[i] > m_pTableFrame->GetGameRoomInfo()->broadcastScore) {
					m_pTableFrame->SendGameMessage(i, "", SMT_GLOBAL | SMT_SCROLL, userWinScorePure[i]);
					LOG(INFO) << " --- *** [" << strRoundID_ << "] 跑马灯信息 userid = " << UserIdBy(i)
						<< " score = " << userWinScorePure[i];
				}
				//各墩牌型
				std::string str;
				if (phandInfos_[i]->GetSelected()->specialTy >= S13S::TyThreesc) {
					//S13S::CGameLogic::StringHandTy(phandInfos_[i]->GetSelected()->specialTy);
					str += std::to_string(phandInfos_[i]->GetSelected()->specialTy);
				}
				else {
					for (int d = S13S::DunFirst; d <= S13S::DunLast; ++d) {
						if (str.empty()) {
							//S13S::CGameLogic::StringHandTy(phandInfos_[i]->GetSelected()->duns[d].ty_);
							str += std::to_string(phandInfos_[i]->GetSelected()->duns[d].ty_);
						}
						else {
							//S13S::CGameLogic::StringHandTy(phandInfos_[i]->GetSelected()->duns[d].ty_);
							str += std::string("|") + std::to_string(phandInfos_[i]->GetSelected()->duns[d].ty_);
						}
					}
				}
				//输赢得水(含打枪/全垒打)
				str += std::string(",") + std::to_string(compareCards_[i].deltascore());
				//座位号，下注分数，输赢分数，各墩牌型，输赢得水
				m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore, str, false);
				//输赢积分
				//player[i].put("winLostScore", userWinScorePure[i]);
				details.mutable_detail(i)->set_winlostscore(userWinScorePure[i]);
			}
			//items.push_back(std::make_pair("", player[i]));
		}
		//广播玩家比牌和结算数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			{
				//比牌数据，各玩家对应各自的比牌数据
				GameEnd.mutable_cmp()->CopyFrom(compareCards_[i]);
				//广播各玩家结算数据
				std::string content = GameEnd.SerializeAsString();
				m_pTableFrame->SendTableData(i, s13s::SUB_S_GAME_END, (uint8_t*)content.data(), content.size());
				std::string const& typeName = GameEnd.GetTypeName();//typename
				{
					//转换json格式
					//std::string jsonstr;
					//PB2JSON::Pb2Json::PbMsg2JsonStr(GameEnd, jsonstr, true);
					//printf("\n--- *** {\"%s\":\n", typeName.c_str());
					//printf("%s}\n\n", jsonstr.c_str());
				}
			}
		}
#ifndef _STORAGESCORE_SEPARATE_STAT_
		//系统输赢积分，扣除系统抽水1%
		if (systemWinScore > 0) {
			//systemWinScore -= m_pTableFrame->CalculateRevenue(systemWinScore);
			systemWinScore -= CalculateAndroidRevenue(systemWinScore);
		}
#endif
		//更新系统库存
		m_pTableFrame->UpdateStorageScore(systemWinScore);
		//系统当前库存变化
		StockScore += systemWinScore;

		//写入玩家积分
		m_pTableFrame->WriteUserScore(&scoreInfos[0], scoreInfos.size(), strRoundID_);

		//对局记录详情json
		if (!m_replay.saveAsStream) {
			//root.put("gameid", ThisGameId);
			//root.add_child("list", items);
			//std::stringstream s;
			//boost::property_tree::json_parser::write_json(s, root, false);
			//m_replay.detailsData = s.str();
			//LOG_ERROR << "\n" << m_replay.detailsData.c_str();
			//boost::replace_all<std::string>(m_replay.detailsData, "\n", "");
		}
		//对局记录详情stream
		else {
			m_replay.detailsData = details.SerializeAsString();
		}
		//保存对局结果
		m_pTableFrame->SaveReplay(m_replay);
		//广播各玩家结算数据
		//std::string content = GameEnd.SerializeAsString();
		//m_pTableFrame->SendTableData(INVALID_CHAIR, s13s::SUB_S_GAME_END, (uint8_t*)content.data(), content.size());
		break;
	}
		//玩家强制退出，执行托管流程
	case GER_USER_LEFT: {
		break;
	}
		//游戏解散(内部解散/外部解散)
	case GER_DISMISS: {
		break;
	}
	default: {
		assert(false);
		break;
	}
	}
	//通知框架结束游戏
	m_pTableFrame->ConcludeGame(GAME_STATUS_END);
	//设置游戏结束状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_END);
	//延时清理桌子数据
	//timerIdGameEnd_ = ThisThreadTimer->runAfter(0.2, boost::bind(&CGameProcess::OnTimerGameEnd, this));
	OnTimerGameEnd();
	return true;
}

//结束定时器，延时清理数据
void CGameProcess::OnTimerGameEnd() {
	//销毁当前定时器
	ThisThreadTimer->cancel(timerIdGameEnd_);
	//清理房间内玩家
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//清除保留信息
		saveUserIDs_[i] = 0;
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
		//用户空闲状态
		userItem->SetUserStatus(sFree);
		//清除用户信息
		m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
	}
	//清理所有定时器
	ClearAllTimer();
	//初始化桌子数据
	InitGameData();
	//重置游戏初始化
	m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
}

static bool GlobalCompareCardsByScore(S13S::CGameLogic::groupdun_t const* src, S13S::CGameLogic::groupdun_t const* dst) {
	//printf("\n\n--- *** --------------------------------------------------\n");
	//const_cast<S13S::CGameLogic::groupdun_t*&>(src)->PrintCardList(S13S::DunFirst);
	//const_cast<S13S::CGameLogic::groupdun_t*&>(src)->PrintCardList(S13S::DunSecond);
	//const_cast<S13S::CGameLogic::groupdun_t*&>(src)->PrintCardList(S13S::DunLast);
	//printf("--- *** --------------------------------------------------\n");
	//const_cast<S13S::CGameLogic::groupdun_t*&>(dst)->PrintCardList(S13S::DunFirst);
	//const_cast<S13S::CGameLogic::groupdun_t*&>(dst)->PrintCardList(S13S::DunSecond);
	//const_cast<S13S::CGameLogic::groupdun_t*&>(dst)->PrintCardList(S13S::DunLast);
	//printf("--- *** --------------------------------------------------\n");
	int bv = S13S::CGameLogic::CompareCardsByScore(src, dst, true, true, true);
	//printf("--- *** *** *** *** *** 结果 %d\n\n", bv);
	return bv > 0;
}

//换牌策略分析
//默认概率随机换牌(可配置)，换牌后机器人都拿好牌，真实玩家都拿差牌
//如果随机到不换牌，若是系统需要杀玩家，强制换牌，否则不换牌
//如果随机到换牌，若是系统需要放水，则不换牌，否则换牌
//如果玩家输分太多系统需要放水时，系统只是不做换牌处理，
//玩家牌好坏由系统随机玩家牌(手气)和玩家手动摆牌策略决定
//若玩家手动组到好牌可能会赢，如果组牌不好，输了也是正常
//避免故意放分导致系统一直输分情况
//真人玩家不能最大牌(随机概率)，否则与机器人牌中最大的换牌(待实现)
//如果真人玩家最大牌，并且赢得水数超过限制，则与机器人牌中最大的换牌(待实现)
void CGameProcess::AnalysePlayerCards(bool killBlackUsers)
{
	//没有机器人
	if (m_pTableFrame->GetPlayerCount(false) == m_pTableFrame->GetPlayerCount(true)) {
		return;
	}
	std::vector<S13S::CGameLogic::groupdun_t const*> v;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//座椅有人
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
#if 0 //没有考虑特殊牌型
		for (std::vector<S13S::CGameLogic::groupdun_t>::iterator it = handInfos_[i].enum_groups.begin();
			it != handInfos_[i].enum_groups.end(); ++it) {
			it->chairID = i;
		}
		//获取当前玩家的一组最优枚举解
		v.push_back(&handInfos_[i].enum_groups[0]);
#else //考虑了特殊牌型
		for (std::vector<S13S::CGameLogic::groupdun_t const*>::iterator it = handInfos_[i].groups.begin();
			it != handInfos_[i].groups.end(); ++it) {
			const_cast<S13S::CGameLogic::groupdun_t*>(*it)->chairID = i;
		}
		//获取当前玩家的一组最优枚举解
		v.push_back(handInfos_[i].groups[0]);
#endif
	}
	//按输赢得水排序(不计打枪/全垒打)
	std::sort(&v.front(), &v.front() + v.size(), GlobalCompareCardsByScore);
	int c = 0;
	//换牌前按牌大小座位排次
	int info[GAME_PLAYER] = { 0 };
	//换牌后按牌大小座位排次(AI排前面，真实玩家排后面)
	std::vector<int> vinfo;
	for (std::vector<S13S::CGameLogic::groupdun_t const*>::const_iterator it = v.begin();
		it != v.end();++it) {
		info[c++] = (*it)->chairID;
		//如果是机器人
		if (m_pTableFrame->IsAndroidUser((*it)->chairID)) {
			vinfo.push_back((*it)->chairID);
		}
	}
	for (std::vector<S13S::CGameLogic::groupdun_t const*>::const_iterator it = v.begin();
		it != v.end(); ++it) {
		//如果是真实玩家
		if (!m_pTableFrame->IsAndroidUser((*it)->chairID)) {
			vinfo.push_back((*it)->chairID);
		}
	}
	for (int i = 0; i < vinfo.size(); ++i) {
		//需要换牌
		if (info[i] != vinfo[i]) {
			//都是机器人，保持原来位置不变
			//都是真实玩家，保持原来位置不变
			if ((
				m_pTableFrame->IsAndroidUser(info[i]) &&
				m_pTableFrame->IsAndroidUser(vinfo[i])) ||
				(
				!m_pTableFrame->IsAndroidUser(info[i]) &&
				!m_pTableFrame->IsAndroidUser(vinfo[i])) ) {
				int j = 0;
				for (; j < vinfo.size(); ++j) {
					//恢复原来的位置
					if (vinfo[j] == info[i]) {
						break;
					}
				}
				std::swap(vinfo[i], vinfo[j]);
			}
		}
	}
	char msg[1024] = { 0 };
	//概率随机是否换牌
	bool isSwapCard = (Exchange == CalcExchangeOrNot2(sysSwapCardsRatio_));

    //难度干涉值
    //m_difficulty
    m_difficulty =m_pTableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
        isSwapCard = true;
    }
    else
    {
        //如果随机到不换牌
        if (!isSwapCard) {
            //系统输分不得低于库存下限，否则赢分
            if (StockScore < StockLowLimit) { //杀玩家
                snprintf(msg, sizeof(msg), "\n--- *** 随机不到换牌，要通杀(StockScore:%lld < StockLowLimit:%lld)，换牌 ...\n",
                    StockScore, StockLowLimit);
                LOG(ERROR) << msg;
                isSwapCard = true;
            }
        }
        //如果随机到换牌
        else {
            assert(isSwapCard);
            //系统赢分不得大于库存上限，否则输分
            if (StockScore > StockHighLimit) { //放水
                snprintf(msg, sizeof(msg), "\n--- *** 随机到换牌，要放水(StockScore:%lld > StockHighLimit:%lld)，不换牌 ...\n",
                    StockScore, StockHighLimit);
                LOG(ERROR) << msg;
                isSwapCard = false;
            }
        }
        //点杀黑名单用户
        if (killBlackUsers) {
            isSwapCard = true;
            LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 点杀换牌 ......";
        }
    }

	//如果不用换牌直接返回
	if (!isSwapCard) {
		return;
	}
	std::string strmsg, strtmp, strtmp2;
	for (int i = 0; i < c; ++i) {
		snprintf(msg, sizeof(msg), "[%d] chairID = %d %s\n",
			i, info[i],
			m_pTableFrame->IsAndroidUser(info[i]) ? "机器人" : "玩家");
		strtmp += msg;
	}
	strmsg += std::string("\n\n--- *** 换牌前按牌大小座位排次\n") + strtmp;
	for (int i = 0; i < vinfo.size(); ++i) {
		snprintf(msg, sizeof(msg), "[%d] chairID = %d %s\n",
			i, vinfo[i],
			m_pTableFrame->IsAndroidUser(vinfo[i]) ? "机器人" : "玩家");
		strtmp2 += msg;
	}
	strmsg += std::string("--- *** 换牌后按牌大小座位排次\n") + strtmp2;
	for (int i = 0; i < vinfo.size(); ++i) {
		//需要换牌，并且换牌后是机器人则换牌
		if (info[i] != vinfo[i] && m_pTableFrame->IsAndroidUser(vinfo[i])) {
			assert(m_pTableFrame->IsExistUser(info[i]));
			assert(m_pTableFrame->IsExistUser(vinfo[i]));
			//真实玩家
			assert(!m_pTableFrame->IsAndroidUser(info[i]));
			//若机器人特殊牌型则不换
			if (handInfos_[vinfo[i]].SpecialTy() >= S13S::TyThreesc &&
				handInfos_[vinfo[i]].SpecialTy() >= handInfos_[info[i]].SpecialTy()) {
				continue;
			}
#if 0
			S13S::CGameLogic::handinfo_t::swap(handInfos_[info[i]], handInfos_[vinfo[i]]);
#else
			//交换手牌信息
			std::swap(phandInfos_[info[i]], phandInfos_[vinfo[i]]);
			//交换手牌
			uint8_t handcards[MAX_COUNT];
			memcpy(handcards, &(handCards_[info[i]])[0], MAX_COUNT);
			snprintf(msg, sizeof(msg), "--- *** 换牌前，玩家 %d 手牌 [%s]\n", info[i], S13S::CGameLogic::StringCards(&(handCards_[info[i]])[0], MAX_COUNT).c_str());
			strmsg += msg;
			memcpy(&(handCards_[info[i]])[0], &(handCards_[vinfo[i]])[0], MAX_COUNT);
			snprintf(msg, sizeof(msg), "--- *** 换牌后，玩家 %d 手牌 [%s]\n", info[i], S13S::CGameLogic::StringCards(&(handCards_[info[i]])[0], MAX_COUNT).c_str());
			strmsg += msg;
			memcpy(&(handCards_[info[i]])[0], &(handCards_[vinfo[i]])[0], MAX_COUNT);
			snprintf(msg, sizeof(msg), "--- *** 换牌前，机器人 %d 手牌 [%s]\n", vinfo[i], S13S::CGameLogic::StringCards(&(handCards_[vinfo[i]])[0], MAX_COUNT).c_str());
			strmsg += msg;
			memcpy(&(handCards_[vinfo[i]])[0], handcards, MAX_COUNT);
			snprintf(msg, sizeof(msg), "--- *** 换牌后，机器人 %d 手牌 [%s]\n", vinfo[i], S13S::CGameLogic::StringCards(&(handCards_[vinfo[i]])[0], MAX_COUNT).c_str());
			strmsg += msg;
#endif
		}
	}
	LOG(ERROR) << strmsg;
}

//读取配置
void CGameProcess::ReadStorageInformation()
{
	assert(m_pTableFrame);
	//系统当前库存
	m_pTableFrame->GetStorageScore(storageInfo_);

	CINIHelp ini;
	ini.Open(INI_FILENAME);


    stockWeak=ini.GetFloat("Global", "STOCK_WEAK",1.0);
	//匹配规则
	{
		//分片匹配时长
		std::string str1(ini.GetString("MatchRule", "sliceMatchSeconds_"));
		sliceMatchSeconds_ = atof(str1.c_str());
		//超时匹配时长
		std::string str2(ini.GetString("MatchRule", "timeoutMatchSeconds_"));
		timeoutMatchSeconds_ = atof(str2.c_str());
		//机器人候补空位超时时长
		std::string str3(ini.GetString("MatchRule", "timeoutAddAndroidSeconds_"));
		timeoutAddAndroidSeconds_ = atof(str3.c_str());
		//IP访问限制
		std::string str4(ini.GetString("MatchRule", "checkIp_"));
		checkIp_ = atoi(str4.c_str()) > 0;
		//一张桌子真实玩家数量限制
		std::string str5(ini.GetString("MatchRule", "checkrealuser_"));
		checkrealuser_ = atoi(str5.c_str()) > 0;
	}
	//匹配概率
	{
		//放大倍数
		std::string str1(ini.GetString("MatchProbability", "ratioScale_"));
		ratioScale_ = atoi(str1.c_str());
		//5人局的概率
		std::string str2(ini.GetString("MatchProbability", "ratio5_"));
		ratio5_ = atoi(str2.c_str());
		//4人局的概率
		std::string str3(ini.GetString("MatchProbability", "ratio4_"));
		ratio4_ = atoi(str3.c_str());
		//3人局的概率
		std::string str4(ini.GetString("MatchProbability", "ratio3_"));
		ratio3_ = atoi(str4.c_str());
		//2人局的概率
		std::string str5(ini.GetString("MatchProbability", "ratio2_"));
		ratio2_ = atoi(str5.c_str());
		//单人局的概率
		std::string str6(ini.GetString("MatchProbability", "ratio1_"));
		ratio1_ = atoi(str6.c_str());
	}
	//理牌总时长
	{
		if (ThisRoomId >= 6305) {
			//极速场
			std::string str1(ini.GetString("GroupTotalTime", "speedseconds_"));
			groupTotalTime_ = atoi(str1.c_str());
		}
		else {
			//常规场
			std::string str1(ini.GetString("GroupTotalTime", "seconds_"));
			groupTotalTime_ = atoi(str1.c_str());
		}
	}
	//默认换牌概率
	{
		if (ThisRoomId >= 6305) {
			//极速场
			std::string str1(ini.GetString("SwapProbability", "ratiospeed_"));
			ratioSwap_ = atoi(str1.c_str());
		}
		else {
			//常规场
			std::string str1(ini.GetString("SwapProbability", "ratio_"));
			ratioSwap_ = atoi(str1.c_str());
		}
	}
	{
		std::string str1(ini.GetString("path", "cardList"));
		INI_CARDLIST_ = str1;
	}

}

//得到桌子实例
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink()
{
    shared_ptr<CGameProcess> pGameProcess(new CGameProcess());
    shared_ptr<ITableFrameSink> pITableFrameSink = dynamic_pointer_cast<ITableFrameSink>(pGameProcess);
    return pITableFrameSink;
}

//删除桌子实例
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pITableFrameSink)
{
    pITableFrameSink.reset();
}

