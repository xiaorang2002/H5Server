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

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/qzpj.Message.pb.h"

#include "../QPAlgorithm/PJ.h"
#include "../QPAlgorithm/cfg.h"
//#include "pb2Json/pb2Json.h"

#include "CMD_Game.h"

#include "TableFrameSink.h"

//叫庄倍数 1,2,3,4
uint8_t s_CallBankerMulti_[MAX_BANKER_MUL] = { 1,2,3,4 };

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/qzpj";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("qzpj");
        // set std output special log content value.
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit;	// 声明全局变局,初始化库.


CTableFrameSink::CTableFrameSink(void) :  m_pTableFrame(NULL)
  ,m_isMatch(false)
{

    stockWeak = 0.0;
	//随机种子
	srand((unsigned)time(NULL));
    //清理游戏数据
    ClearGameData();
	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//超时匹配时长(可配置)
	timeoutMatchSeconds_ = 3.6f;
	//机器人候补空位超时时长(可配置)
	timeoutAddAndroidSeconds_ = 1.4f;
	//放大倍数
	ratioScale_ = 1000;
	//5/4/3/2/单人局的概率
	ratio5_ = 0, ratio4_ = 100, ratio3_ = 0, ratio2_ = 0, ratio1_ = 0;
	//初始化桌子游戏人数概率权重
	initPlayerRatioWeight();
	MaxGamePlayers_ = 0;
}

CTableFrameSink::~CTableFrameSink(void)
{

}

//初始化桌子游戏人数概率权重
void CTableFrameSink::initPlayerRatioWeight() {
	//for (int i = m_pGameRoomInfo->minPlayerNum - 1; i < m_pGameRoomInfo->maxPlayerNum; ++i) {
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
bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
	//桌子指针
	m_pTableFrame = pTableFrame; assert(m_pTableFrame);
	//定时器指针
	m_TimerLoopThread = m_pTableFrame->GetLoopThread();
	//初始化游戏数据
	InitGameData();
	//读取配置
	ReadConfig();
	//初始化桌子游戏人数概率权重
	initPlayerRatioWeight();
	//放大倍数
	int scale = 10;
	//系统换牌概率
	int dw[MaxExchange] = {
		ratioSwap_*scale,
		(100 - ratioSwap_)*scale };
	sysSwapCardsRatio_.init(dw, MaxExchange);
	//系统通杀概率
	int lw[MaxExchange] = {
		ratioSwapLost_*scale,
		(100 - ratioSwapLost_)*scale };
	sysLostSwapCardsRatio_.init(lw, MaxExchange);
	//系统放水概率
	int ww[MaxExchange] = {
		ratioSwapWin_*scale,
		(100 - ratioSwapWin_)*scale };
	sysWinSwapCardsRatio_.init(ww, MaxExchange);
	//对局日志
	m_replay.cellscore = CellScore;
	m_replay.roomname = ThisRoomName;
	//m_replay.gameid = ThisGameId;//游戏类型
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	//2401 - 240 * 10 > 20 即2421开始为比赛场
	if (ThisRoomId - ThisGameId * 10 > 20)
		m_isMatch = true;
	return true;
}

//清理游戏数据
void CTableFrameSink::ClearGameData()
{
    //庄家
    bankerUser_ = INVALID_CHAIR;
	//扑克变量
	memset(handCards_, 0, sizeof(handCards_));
	memset(handCardsType_, 0, sizeof(handCardsType_));
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//玩家叫庄(-1:未叫 0:不叫 1,2,3,4:叫庄)
		callBanker_[i] = -1;
		//玩家下注(-1:未下注 大于-1:对应筹码索引)
		jettonIndex_[i] = -1;
		//玩家摊牌(-1:未摊牌 大于-1:对应手牌牌型)
		openCardType_[i] = -1;
	}
	//可用的叫庄倍数
	memset(callableMulti_, 0, sizeof(callableMulti_));
	//可用的下注倍数
	memset(jettonableMulti_, 0, sizeof(jettonableMulti_));
	//
	memset(userLeftScore_, 0, sizeof(userLeftScore_));
	memset(userWinScorePure_, 0, sizeof(userWinScorePure_));
	//对局日志
	m_replay.clear();
	//进行第几局
	curRoundTurn_ = 0;
	isGameOver_ = false;
	strRoundID_.clear();
}

//初始化游戏数据
void CTableFrameSink::InitGameData()
{
	assert(m_pTableFrame);
	//庄家
	bankerUser_ = INVALID_CHAIR;
	//扑克变量
	memset(handCards_, 0, sizeof(handCards_));
	memset(handCardsType_, 0, sizeof(handCardsType_));
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//玩家叫庄(-1:未叫 0:不叫 1,2,3,4:叫庄)
		callBanker_[i] = -1;
		//玩家下注(-1:未下注 大于-1:对应筹码索引)
		jettonIndex_[i] = -1;
		//玩家摊牌(-1:未摊牌 大于-1:对应手牌牌型)
		openCardType_[i] = -1;
	}
	//可用的叫庄倍数
	memset(callableMulti_, 0, sizeof(callableMulti_));
	//可用的下注倍数
	memset(jettonableMulti_, 0, sizeof(jettonableMulti_));
	//
	memset(userLeftScore_, 0, sizeof(userLeftScore_));
	memset(userWinScorePure_, 0, sizeof(userWinScorePure_));
	//计算最低基准积分
	m_pTableFrame->GetGameRoomInfo()->floorScore;
	//计算最高基准积分
	m_pTableFrame->GetGameRoomInfo()->ceilScore;
	//进入房间最小积分
	m_pTableFrame->GetGameRoomInfo()->enterMinScore;
	//进入房间最大积分，玩家超过这个积分，只能进入更高级房间
	m_pTableFrame->GetGameRoomInfo()->enterMaxScore;
	if (curRoundTurn_ >= MAX_ROUND) {
		//进行第几局
		curRoundTurn_ = 0;
	}
	if (curRoundTurn_ == 0) {
		//初始化
		g.InitCards();
		//洗牌
		g.ShuffleCards();
	}
	//对局日志
	m_replay.clear();
	strRoundID_.clear();
}

//用户进入
bool CTableFrameSink::OnUserEnter(int64_t userId, bool bisLookon)
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
	//首个玩家加入桌子：仅有一个用户且是真实玩家，初始化MaxGamePlayers_
	if (m_pTableFrame->GetPlayerCount(true) == 1 && m_pTableFrame->GetPlayerCount(false) == 1 &&
		m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT) {
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
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_READY) {
		userItem->SetUserStatus(sReady);
	}
	//游戏进行中，修改玩家游戏中
	else if (m_pTableFrame->GetGameStatus() >= GAME_STATUS_START) {
		userItem->SetUserStatus(sPlaying);
	}
	//首个玩家加入桌子：仅有一个用户且是真实玩家
	if (m_pTableFrame->GetPlayerCount(true) == 1 && m_pTableFrame->GetPlayerCount(false) == 1 &&
		     m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 首个玩家加入桌子 ...", m_pTableFrame->GetTableId());
		LOG(ERROR) << msg;
		//修改游戏空闲准备
		m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		//修改玩家准备状态
		userItem->SetUserStatus(sReady);
		//清理所有定时器
		ClearAllTimer(true);
		//定时器监控等待其他玩家或机器人加入桌子
		timerGameReadyID_ = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d)，等待人数(%d)", m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << msg;
	}
	//开局前(<GAME_STATUS_START)达到桌子要求游戏人数，开始游戏
	else if (m_pTableFrame->GetPlayerCount(true) >= MaxGamePlayers_ &&
			m_pTableFrame->GetGameStatus() == GAME_STATUS_READY) {
			//不超过桌子要求最大游戏人数
			//assert(m_pTableFrame->GetPlayerCount(true) == MaxGamePlayers_);
			snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d:%d:%d)，开始游戏",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
			LOG(ERROR) << msg;
			//清理所有定时器
			ClearAllTimer(true);
			GameTimerReadyOver();
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
bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
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
		}
		{
			assert(pUser);
			uint32_t chairId = INVALID_CHAIR;
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(pUser->GetUserId());
			if (userItem) {
				chairId = userItem->GetChairId();
			}
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d] status[%d:%d:%d] CanJoinTable1[true] userID[%d] chairID[%d]\n",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_, pUser->GetUserId(), chairId);
			LOG(ERROR) << msg;
		}
		return true;
	}
	//游戏结束时玩家信息已被清理掉
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_END) {
		//assert(!m_pTableFrame->IsExistUser(pUser->GetUserId()));
		return false;
	}
	else if (m_pTableFrame->GetGameStatus() >= GAME_STATUS_START) {
// 		{
// 			assert(pUser);
// 			uint32_t chairId = INVALID_CHAIR;
// 			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(pUser->GetUserId());
// 			if (userItem) {
// 				chairId = userItem->GetChairId();
// 			}
// 			char msg[1024] = { 0 };
// 			snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d] status[%d:%d:%d] CanJoinTable2[%s] userID[%d] chairID[%d]\n",
// 				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_, (userItem != NULL ? "true" : "false"), pUser->GetUserId(), chairId);
// 			LOG(ERROR) << msg;
// 		}
		//游戏进行状态，处理断线重连，玩家信息必须存在
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(pUser->GetUserId());
		return userItem != NULL;
	}
	//游戏状态为GAME_STATUS_START(100)时，不会进入该函数
	//assert(false);
	return false;
}

//判断是否能离开游戏
bool CTableFrameSink::CanLeftTable(int64_t userId)
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
bool CTableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
	return true;
}

//用户离开
bool CTableFrameSink::OnUserLeft(int64_t userId, bool islookon)
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
		//saveUserIDs_[chairId] = 0;
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
			ClearAllTimer(true);
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
void CTableFrameSink::OnTimerGameReadyOver()
{
	//清除游戏准备定时器
	ThisThreadTimer->cancel(timerGameReadyID_);
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
				//saveUserIDs_[i] = 0;
				shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
				//用户空闲状态
				userItem->SetUserStatus(sFree);
				//清除用户信息
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
			}
			//清理所有定时器
			ClearAllTimer(true);
			//初始化桌子数据
			InitGameData();
			//重置桌子初始状态
			m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
		}
		//匹配中也没有机器人
		assert(m_pTableFrame->GetPlayerCount(true) == 0);
		return;
	}
	if ((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_READY) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d] OnTimerGameReadyOver status[%d:%d:%d]\n",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << msg;
		//定时器已经取消了又重复调用???
		return;
	}
	//匹配中非初始状态
	//assert((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_INIT);
	//匹配中准备状态
	//assert((int)m_pTableFrame->GetGameStatus() == GAME_STATUS_READY);
// 	if ((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_READY) {
// 		//清理房间内玩家
// 		for (int i = 0; i < GAME_PLAYER; ++i) {
// 			if (!m_pTableFrame->IsExistUser(i)) {
// 				continue;
// 			}
// 			//清除保留信息
// 			//saveUserIDs_[i] = 0;
// 			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
// 			//用户空闲状态
// 			userItem->SetUserStatus(sFree);
// 			//清除用户信息
// 			m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
// 		}
// 		//清理所有定时器
// 		ClearAllTimer();
// 		//初始化桌子数据
// 		InitGameData();
// 		//重置桌子初始状态
// 		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
// 		return;
// 	}
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
			timerGameReadyID_ = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
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
				timerGameReadyID_ = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
				//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]仍然没有达到最小游戏人数(%d)，继续等待...\n", m_pTableFrame->GetTableId(), MIN_GAME_PLAYER);
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
			//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", m_pTableFrame->GetTableId(), totalMatchSeconds_, MaxGamePlayers_);
			//定时器检测机器人候补空位后是否达到游戏要求人数
			timerGameReadyID_ = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		}
	}
}

//结束准备，开始游戏
void CTableFrameSink::GameTimerReadyOver()
{
	//确定游戏状态：初始化空闲准备阶段，而非游戏中
	//assert(m_pTableFrame->GetGameStatus() < GAME_STATUS_START);
	//清除游戏准备定时器
	ThisThreadTimer->cancel(timerGameReadyID_);
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
		assert(false);
		//没有达到最小游戏人数
// 		for (int i = 0; i < GAME_PLAYER; ++i) {
// 			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
// 			if (userItem) {
// 
// 				//重置准备状态
// 				userItem->SetUserStatus(sReady);
// 			}
// 		}
//		//清除所有定时器
//		ClearAllTimer();
//		//重置桌子游戏初始状态
//		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
	}
}

//游戏开始
void CTableFrameSink::OnGameStart()
{
	char msg[4096] = { 0 };
	m_replay.clear();
	//确定游戏进行中
	//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_START);
	//清除所有定时器
	ClearAllTimer(true);
	//初始化数据
	InitGameData();
	//系统当前库存
	m_pTableFrame->GetStorageScore(storageInfo_);
	//进行第几局
	++curRoundTurn_;
	if (curRoundTurn_ < MAX_COUNT) {
		strRoundIDNew_ = m_pTableFrame->GetNewRoundId();
	}
	else {
		//继续进行中
		m_pTableFrame->SetGameStatus(GAME_STATUS_START);
	}
	//牌局编号
	strRoundID_ += strRoundIDNew_ + "-" + std::to_string(curRoundTurn_);
	//对局日志
	m_replay.gameinfoid = strRoundID_;
	
	//本局开始时间
	roundStartTime_ = chrono::system_clock::now();
	aniPlay_.add_START_time(AnimatePlay::CallE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::ScoreE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::OpenE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::NextE, roundStartTime_);
	aniPlay_.add_START_time(AnimatePlay::GameEndE, roundStartTime_);
	//本局黑名单玩家数
	int blackUserNum = 0;
	//点杀概率值(千分比)
	//int rateLostMax = 0;
	//点杀概率值判断是否本局点杀
	//bool killBlackUsers = false;
	memset(isBlackUser_, 0, sizeof isBlackUser_);
	//本局含机器人
	if (m_pTableFrame->GetPlayerCount(false) != m_pTableFrame->GetPlayerCount(true)) {
		//随机概率值
		int randval = RandomBetween(1, 1000);
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
			//满足概率点杀条件
			if (blackUser.status > 0 &&
				blackUser.current < blackUser.total &&
				randval <= blackUser.weight) {
				++blackUserNum;
				isBlackUser_[i] = true;
				LOG(ERROR) << " --- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] *** blackList"
					<< "\nuserId=" << UserIdBy(i) << " rate=" << blackUser.weight << " current=" << blackUser.current
					<< " total=" << blackUser.total << " status=" << blackUser.status;
			}
		}
	}
	//配置牌型
	std::vector<std::string> lines;
	int flag = 0;
	if (!INI_CARDLIST_.empty() && boost::filesystem::exists(INI_CARDLIST_)) {
		//读取配置
		readFile(INI_CARDLIST_.c_str(), lines, ";;");
		//1->文件读取手牌 0->随机生成13张牌
		flag = atoi(lines[0].c_str());
		//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
		ratioSwap_ = atoi(lines[1].c_str());
		//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
		ratioSwapLost_ = atoi(lines[2].c_str());
		//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
		ratioSwapWin_ = atoi(lines[3].c_str());
		//放大倍数
		int scale = 10;
		//系统换牌概率
		int dw[MaxExchange] = {
			ratioSwap_*scale,
			(100 - ratioSwap_)*scale };
		sysSwapCardsRatio_.init(dw, MaxExchange);
		//系统通杀概率
		int lw[MaxExchange] = {
			ratioSwapLost_*scale,
			(100 - ratioSwapLost_)*scale };
		sysLostSwapCardsRatio_.init(lw, MaxExchange);
		//系统放水概率
		int ww[MaxExchange] = {
			ratioSwapWin_*scale,
			(100 - ratioSwapWin_)*scale };
		sysWinSwapCardsRatio_.init(ww, MaxExchange);
	}
	if (flag > 0) {
		std::map<int, bool> imap;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = ByChairId(i); assert(userItem);
			assert(4 <= lines.size() - 1);
			int index = RandomBetween(4, lines.size() - 1);
			do {
				std::map<int, bool>::iterator it = imap.find(index);
				if (it == imap.end()) {
					imap[index] = true;
					break;
				}
				index = RandomBetween(4, lines.size() - 1);
			} while (true);
			//构造一副手牌3张
			int n = PJ::CGameLogic::MakeCardList(lines[index], &(handCards_[i])[0], MAX_COUNT);
			assert(n == MAX_COUNT);
			//手牌排序
			PJ::CGameLogic::SortCards(&(handCards_[i])[0], MAX_COUNT);
			//手牌牌型
			handCardsType_[i] = PJ::CGameLogic::GetHandCardsType(&(handCards_[i])[0]);
			//对局日志
			//m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
		}
	}
	else {
		//65%概率发好牌
		int ratioDealGood = RandomBetween(1, 100);
		//给各个玩家发牌
		{
		restart:
			assert(m_pTableFrame->GetPlayerCount(true) <= GAME_PLAYER);
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				//余牌不够则重新洗牌，然后重新分发给各个玩家
				if (g.Remaining() < MAX_COUNT) {
					g.ShuffleCards();
					goto restart;
				}
				//发牌
				g.DealCards(MAX_COUNT, &(handCards_[i])[0]);
			}
		}
		//各个玩家手牌分析
		{
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				shared_ptr<CServerUserItem> userItem = ByChairId(i); assert(userItem);
				//手牌排序
				PJ::CGameLogic::SortCards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				handCardsType_[i] = PJ::CGameLogic::GetHandCardsType(&(handCards_[i])[0]);
				//对局日志
				//m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
			}
			//好牌数量
			int c = 0;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				if (PJ::HandTy(handCardsType_[i]) > PJ::PP9) {
					++c;
				}
			}
			//点杀条件：
			//	1.本局含机器人
			//	2.含待点杀用户
			//	3.满足点杀概率
			//发牌结果：
			//	1.让点杀用户都拿小牌
			//	2.让所有机器人都拿大牌
			if (blackUserNum > 0) {
				//一定概率发好牌，好牌数量为blackList_.size()+1
				if (ratioDealGood <= ratioDealGoodKill_ && c < (blackUserNum + 1)) {
					char msg[1024] = { 0 };
					snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
						m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
					LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 开始游戏，点杀发好牌 !!!...";
					goto restart;
				}
			}
			//好牌数量不够，重新洗牌发牌
			else if (ratioDealGood <= ratioDealGood_ && c < (m_pTableFrame->GetPlayerCount(true) - 1)) {
				char msg[1024] = { 0 };
				snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
					m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
				LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 开始游戏，概率发好牌 !!!...";
				goto restart;
			}
		}
	}
	//换牌策略分析
	{
		AnalysePlayerCards(blackUserNum > 0);
		ListPlayerCards();
	}
	//给机器人发送自己的牌数据
	{
		//给机器人发数据
		qzpj::CMD_S_GameStartAi  reqdata;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//跳过真人玩家
			if (!m_pTableFrame->IsAndroidUser(i)) {
				continue;
			}
			//机器人牌型
			reqdata.set_cboxcarddata(handCardsType_[i]);
			//是否最大牌
			reqdata.set_ismaxcard(IsMaxCard(i));
			std::string data = reqdata.SerializeAsString();
			m_pTableFrame->SendTableData(i, qzpj::SUB_S_GAME_START_AI, (uint8_t*)data.c_str(), data.size());
		}
	}
	//广播游戏开始消息
	{
		qzpj::CMD_S_GameStart reqdata;
		//牌局编号
		reqdata.set_roundid(strRoundID_);
		//进行第几局
		reqdata.set_roundturn(curRoundTurn_);
		//叫庄时间
		reqdata.set_cbcallbankertime(DELAY_Callbanker);
		//确定叫庄倍数 ///
		for (int i = 0; i < GAME_PLAYER; ++i) {
			reqdata.add_cbplaystatus(m_pTableFrame->IsExistUser(i));
			if (!m_pTableFrame->IsExistUser(i)) {
				for (int j = 0; j < MAX_BANKER_MUL; ++j) {
					callableMulti_[i][j] = 0;
					reqdata.add_cbcallbankermultiple(callableMulti_[i][j]);
				}
				continue;
			}
			//最大可叫庄倍数 玩家携带/底注/其他玩家人数/8
			int64_t maxCallTimes = ScoreByChairId(i) / (CellScore * (m_pTableFrame->GetPlayerCount(true) - 1) * 8);
			//最小只能叫1倍
			callableMulti_[i][0] = s_CallBankerMulti_[0];
			reqdata.add_cbcallbankermultiple(callableMulti_[i][0]);
			//其它倍数能叫或不能叫
			for (int j = 1; j < MAX_BANKER_MUL; ++j) {
				//可用的叫庄倍数
				if (maxCallTimes >= s_CallBankerMulti_[j]) {
					callableMulti_[i][j] = s_CallBankerMulti_[j];
					reqdata.add_cbcallbankermultiple(callableMulti_[i][j]);
				}
				else {
					//不可用的叫庄倍数
					callableMulti_[i][j] = 0;
					reqdata.add_cbcallbankermultiple(callableMulti_[i][j]);
				}
			}
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			std::string ss;
			char msg[512] = { 0 };
			for (int j = 0; j < MAX_BANKER_MUL; ++j) {
				if (j == 0) {
					snprintf(msg, sizeof(msg), "[%d", callableMulti_[i][j]);
				}
				else {
					snprintf(msg, sizeof(msg), ",%d", callableMulti_[i][j]);
				}
				ss += msg;
			}
			ss += "]";
			if (m_pTableFrame->IsAndroidUser(i)) {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 机器人 [" << i << "] " << UserIdBy(i)
					<< " 可叫庄倍数 " << ss;
			}
			else {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 玩家 [" << i << "] " << UserIdBy(i)
					<< " 可叫庄倍数 " << ss;
			}
		}
		//发送数据
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
	}
	roundCallTime_ = chrono::system_clock::now();//叫庄开始时间
	aniPlay_.add_START_time(AnimatePlay::CallE, roundCallTime_);
	//设置桌子状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_CALL);
	//游戏开始，开始叫庄(5s)
	IsTrustee();
}

//设置托管 ///
//游戏开始，开始叫庄(5s)
//都叫庄了，开始下注(5s)
//都下注了，开始摊牌(5s)
//都摊牌了，结束游戏(2s)
bool CTableFrameSink::IsTrustee(void)
{
	//游戏开始，开始叫庄
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_CALL) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 叫庄开始 ......";
		ThisThreadTimer->cancel(timerGameReadyID_);
		ThisThreadTimer->cancel(timerCallBankerID_);
		//叫庄倒计时      开始游戏 + 开始抢庄 + 抢庄倒计时
		bankerUser_ = INVALID_CHAIR;
		timerCallBankerID_ = ThisThreadTimer->runAfter(DELAY_Callbanker, boost::bind(&CTableFrameSink::OnTimerCallBanker, this));
	}
	//都叫庄了，开始下注
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_SCORE) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 下注开始 ......";
		ThisThreadTimer->cancel(timerCallBankerID_);
		ThisThreadTimer->cancel(timerAddScoreID_);
		ThisThreadTimer->cancel(timerGameReadyID_);
		//下注倒计时      定庄动画 + 下注倒计时
		timerAddScoreID_ = ThisThreadTimer->runAfter(DELAY_AddScore, boost::bind(&CTableFrameSink::OnTimerAddScore, this));
	}
	//都下注了，开始摊牌
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_OPEN) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 摊牌开始 ......";
		ThisThreadTimer->cancel(timerAddScoreID_);
		ThisThreadTimer->cancel(timerOpenCardID_);
		ThisThreadTimer->cancel(timerGameReadyID_);
		//开牌倒计时      摇骰子 + 发牌 + 开牌倒计时
		timerOpenCardID_ = ThisThreadTimer->runAfter(DELAY_OpenCard, boost::bind(&CTableFrameSink::OnTimerOpenCard, this));
	}
	//都摊牌了，继续下一轮
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_NEXT) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 继续游戏 ......";
		ThisThreadTimer->cancel(timerOpenCardID_);
		ThisThreadTimer->cancel(timerGameEndID_);
		ThisThreadTimer->cancel(timerGameReadyID_);
		//下一轮开始倒计时   开牌动画 + 下一轮开始倒计时
		ThisThreadTimer->runAfter(DELAY_NextRound, std::bind(&CTableFrameSink::OnGameStart, this));
	}
	//都摊牌了，结束游戏(2s)
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_PREEND) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 结束游戏 ......";
		ThisThreadTimer->cancel(timerOpenCardID_);
		ThisThreadTimer->cancel(timerGameEndID_);
		ThisThreadTimer->cancel(timerGameReadyID_);
		ClearAllTimer(true);
		//游戏结束倒计时   开牌动画 + 输赢动画
		if (m_isMatch)
			timerGameEndID_ = ThisThreadTimer->runAfter(DELAY_GameEnd/*TIME_MATCH_GAME_END*/, boost::bind(&CTableFrameSink::OnTimerGameEnd, this));
		else
			timerGameEndID_ = ThisThreadTimer->runAfter(DELAY_GameEnd/*DELAY_NextRound*/, boost::bind(&CTableFrameSink::OnTimerGameEnd, this));
		//OnTimerGameEnd();
	}
	return true;
}

//跑马灯 ///
void CTableFrameSink::PaoMaDeng()
{
}

//是否最大牌 ///
int CTableFrameSink::IsMaxCard(int chairId)
{
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (i == chairId) {
			continue;
		}
		if (PJ::CGameLogic::CompareHandCards(&(handCards_[chairId])[0], &(handCards_[i])[0]) < 0) {
			return 0;
		}
	}
	return 1;
}

//叫庄 ///
void CTableFrameSink::OnTimerCallBanker()
{
	//销毁定时器
	ThisThreadTimer->cancel(timerCallBankerID_);
	//定时器到期，未叫庄的强制叫庄
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//过滤已叫庄用户
		if (callBanker_[i] != -1) {
			continue;
		}
		//默认不叫 ///
		OnUserCallBanker(i, 0);
	}
	char msg[1024] = { 0 };
	snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
	LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 超时，结束叫庄 ......";
}

//已叫庄人数 ///
int CTableFrameSink::GetCallBankerCount() {
	int c = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//过滤没有叫庄玩家
		if (-1 == callBanker_[i]) {
			continue;
		}
		++c;
	}
	return c;
}

//随机庄家 ///
int CTableFrameSink::RandBanker()
{
	//没有人叫庄则积分最多的坐庄
	int bankerUser = INVALID_CHAIR;
	int64_t maxUserScore = 0;
	for (uint8_t i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
		if (userItem->GetUserScore() > maxUserScore) {
			maxUserScore = userItem->GetUserScore();
			bankerUser = i;
		}
	}
	return bankerUser;
}

#if 1
//玩家叫庄 ///
int CTableFrameSink::OnUserCallBanker(int chairId, int wCallFlag)
{
	//已叫过庄
	if (-1 != callBanker_[chairId]) {
		return 1;
	}
	//叫庄标志错误
	if (wCallFlag < 0) {
		return 2;
	}
	//检测是否正确的倍数
	bool bCorrectMultiple = false;
	if (wCallFlag > 0) {
		int callableFlag = 0;
		for (int i = 0; i != MAX_BANKER_MUL; ++i) {
			if (callableMulti_[chairId][i] == wCallFlag) {
				bCorrectMultiple = true;
				break;
			}
			if (callableMulti_[chairId][i] < wCallFlag) {
				callableFlag = (int)callableMulti_[chairId][i];
			}
		}
		if (bCorrectMultiple == false) {
			if (m_pTableFrame->IsAndroidUser(chairId)) {
				char msg[1024] = { 0 };
				snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
					m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
				LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 <<失败>> !!! callflag = " << wCallFlag;
			}
			else {
				char msg[1024] = { 0 };
				snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
					m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
				LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 <<失败>> !!! callflag = " << wCallFlag;
			}
			wCallFlag = callableFlag;
		}
	}
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 callflag = " << wCallFlag;
	}
	else {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 callflag = " << wCallFlag;
	}
    //记录叫庄 0:不叫 1,2,3,4:叫庄
    callBanker_[chairId] = wCallFlag;
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
    m_replay.addStep(nowsec, to_string(wCallFlag),-1,opBet,chairId,chairId);
	//玩家叫庄广播
	{
		//消息结构
		qzpj::CMD_S_CallBanker reqdata;
		reqdata.set_wcallbankeruser(chairId);
		reqdata.set_cbcallmultiple(wCallFlag);
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_CALL_BANKER, (uint8_t*)data.c_str(), data.size());
	}
	//都叫庄了，确定庄家，开始下注 ///
	if (GetCallBankerCount() == m_pTableFrame->GetPlayerCount(true)) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 都叫庄了，开始下注(5s) ......";
		//消息结构
		qzpj::CMD_S_CallBankerResult reqdata;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			reqdata.add_cbcallbankeruser(0);
		}
		//叫庄倍数对应叫了庄的玩家
		int callbankerUser[MAX_BANKER_CALL][GAME_PLAYER] = { 0 };
		//叫庄倍数对应被叫庄次数
		int callbankerCount[MAX_BANKER_CALL] = { 0 };  //0 1 2 3 4 count
		for (int i = 0; i < GAME_PLAYER; ++i) {
			reqdata.set_cbcallbankeruser(i, callBanker_[i]);
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			// 0,1,2,3,4
			if (callBanker_[i] > 0) {
				//叫庄倍数
				uint8_t callScore = (uint8_t)callBanker_[i];  //0,1,2,3,4
				uint8_t callCount = (uint8_t)callbankerCount[callScore];  //0,1,2,3,4 count
				//0,1,2,3,4 第 callCount 次叫庄玩家
				callbankerUser[callScore][callCount] = i;
				//每个叫庄倍数对应被叫的次数
				++callbankerCount[callScore];
			}
		}
		//确定庄家，倍数从大到小遍历4,2,3,1 0 -1
		for (int i = (MAX_BANKER_CALL - 1); i > 0; --i) {
			if (callbankerCount[i] > 0) {
				int c = callbankerCount[i];
				if (c == 1) {
					//当前倍数被叫了一次
					bankerUser_ = callbankerUser[i][0];
				}
				else {
					//当前倍数被叫了多次
					reqdata.set_cbrandbanker(1);
					//积分最多的玩家坐庄
					int64_t curMaxScore = 0;
					for (int j = 0; j < c; ++j) {
						shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(callbankerUser[i][j]);
						if (userItem->GetUserScore() > curMaxScore) {
							curMaxScore = userItem->GetUserScore();
							bankerUser_ = callbankerUser[i][j];
						}
					}
				}
				break;
			}
		}
		//没有人叫庄
		if (bankerUser_ == INVALID_CHAIR) {
			//随机庄家
			bankerUser_ = RandBanker();
			assert(bankerUser_ != INVALID_CHAIR);
			//默认叫1倍
			callBanker_[bankerUser_] = s_CallBankerMulti_[0];
			reqdata.set_cbrandbanker(1);
		}
		else {
			reqdata.set_cbrandbanker(0);
		}
		//对局日志
		m_replay.addStep(nowsec, to_string(bankerUser_), -1, opBanker, -1, -1);
		//庄家用户
		reqdata.set_dwbankeruser(bankerUser_);
		//确定下注倍数 庄家携带/底注/闲家人数/庄家抢庄倍数
		double maxBankerCallTimes = ((double)ScoreByChairId(bankerUser_)) / (CellScore * (m_pTableFrame->GetPlayerCount(true) - 1) * callBanker_[bankerUser_]);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (m_pTableFrame->IsExistUser(i) && i != bankerUser_) {
				//闲家携带/底注/庄家抢庄倍数
				double maxPlayerCallTimes = ((double)ScoreByChairId(i)) / (CellScore * callBanker_[bankerUser_]);
				double callMaxTimes = std::min<double>(maxBankerCallTimes, maxPlayerCallTimes);
				callMaxTimes = callMaxTimes <= 1 ? 1 : callMaxTimes;
				if (m_pTableFrame->IsAndroidUser(i)) {
					LOG(WARNING/*ERROR*/) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 机器人 [" << i << "] " << UserIdBy(i)
						<< " callMaxTimes " << callMaxTimes << (i == bankerUser_ ? " 庄" : " 闲");
				}
				else {
					LOG(WARNING) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 玩家 [" << i << "] " << UserIdBy(i)
						<< " callMaxTimes " << callMaxTimes << (i == bankerUser_ ? " 庄" : " 闲");
				}
				//X<5时，根据X的值设对应整数数量的筹码
				if (callMaxTimes < 5) {
					for (int j = 1; j <= JettionList.size(); ++j) {
						if (j <= callMaxTimes) {
							LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] jettion_case 1";
							//callMaxTimes==4时  1 2 3 4
							//callMaxTimes==3时  1 2 3
							//callMaxTimes==2时  1 2
							jettonableMulti_[i][j - 1] = j;
						}
						else {
							LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] jettion_case 2";
							jettonableMulti_[i][j - 1] = 0;
						}
						reqdata.add_cbjettonmultiple(jettonableMulti_[i][j - 1]);
					}
				}
				//X<30时，1 floor(2X/5) floor(3X/5) floor(4X/5) X
				else if (callMaxTimes < 30) {
					LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] jettion_case 3";
					jettonableMulti_[i][0] = 1;
					reqdata.add_cbjettonmultiple(jettonableMulti_[i][0]);
					for (int j = 1; j < JettionList.size(); ++j) {
						jettonableMulti_[i][j] = int(callMaxTimes * (j + 1) / 5 + 0.5);
						reqdata.add_cbjettonmultiple(jettonableMulti_[i][j]);
					}
				}
				//>=30时，1 8 15 22 30
				else {
					LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] jettion_case 4";
					for (int j = 0; j < JettionList.size(); ++j) {
						jettonableMulti_[i][j] = (uint8_t)JettionList[j];
						reqdata.add_cbjettonmultiple(jettonableMulti_[i][j]);
					}
				}
			}
			else {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] jettion_case 5";
				for (int j = 0; j < JettionList.size(); ++j) {
					jettonableMulti_[i][j] = 0;
					reqdata.add_cbjettonmultiple(jettonableMulti_[i][j]);
				}
			}
			//下注时间
			reqdata.set_cbaddjettontime(DELAY_AddScore);
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			std::string ss;
			char msg[512] = { 0 };
			for (int j = 0; j < JettionList.size(); ++j) {
				if (j == 0) {
					snprintf(msg, sizeof(msg), "[%d", jettonableMulti_[i][j]);
				}
				else {
					snprintf(msg, sizeof(msg), ",%d", jettonableMulti_[i][j]);
				}
				ss += msg;
			}
			ss += "]";
			if (m_pTableFrame->IsAndroidUser(i)) {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 机器人 [" << i << "] " << UserIdBy(i)
					<< " 可下注筹码 " << ss << (i == bankerUser_ ? " 庄" : " 闲");
			}
			else {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 玩家 [" << i << "] " << UserIdBy(i)
					<< " 可下注筹码 " << ss << (i == bankerUser_ ? " 庄" : " 闲");
			}
		}
		//广播消息
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_CALL_BANKER_RESULT, (uint8_t*)data.c_str(), data.size());
		roundScoreTime_ = chrono::system_clock::now();//下注开始时间
		aniPlay_.add_START_time(AnimatePlay::ScoreE, roundScoreTime_);
		//修改桌子状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_SCORE);
		//都叫庄了，开始下注(5s)
		IsTrustee();
    }
    return 4;
}
#else

#endif

//下注 ///
void CTableFrameSink::OnTimerAddScore()
{
    ThisThreadTimer->cancel(timerAddScoreID_);
	//定时器到期，未下注的强制下注
	for (int i = 0; i < GAME_PLAYER; i++) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (jettonIndex_[i] != -1) {
			continue;
		}
		//默认下1倍 ///
		OnUserAddScore(i, 0);
	}
	char msg[1024] = { 0 };
	snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
	LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 超时，结束下注 ......";
}

//已下注人数 ///
int CTableFrameSink::GetAddScoreCount() {
	int c = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//过滤庄家
		if (i == bankerUser_) {
			continue;
		}
		//过滤没有下注玩家
		if (-1 == jettonIndex_[i]) {
			continue;
		}
		++c;
	}
	return c;
}

//玩家下注 ///
int CTableFrameSink::OnUserAddScore(int chairId, int wJettonIndex)
{
	//庄家不能下注
	if (bankerUser_ == chairId) {
		return 1;
	}
	//已下过注
	if (-1 != jettonIndex_[chairId]) {
		return 2;
	}
	//下注筹码错误 5 10 15 20
	if (wJettonIndex < 0 || wJettonIndex >= JettionList.size()) {
		return 3;
	}
	//可下注倍数无效
	if (jettonableMulti_[chairId][wJettonIndex] <= 0) {
		if (m_pTableFrame->IsAndroidUser(chairId)) {
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
			LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 下注 <<失败>> !!! index = " << wJettonIndex;
		}
		else {
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
			LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 下注 <<失败>> !!! index = " << wJettonIndex;
		}
		wJettonIndex = 0;
		//return 4;
	}
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 下注 index = " << wJettonIndex;
	}
	else {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 下注 index = " << wJettonIndex;
	}
    //记录下注
    jettonIndex_[chairId] = wJettonIndex;
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
    m_replay.addStep(nowsec,to_string(jettonableMulti_[chairId][wJettonIndex]),-1,opAddBet,chairId,chairId);
	//广播玩家下注
	{
		qzpj::CMD_S_AddScoreResult reqdata;
		reqdata.set_waddjettonuser(chairId);
		reqdata.set_cbjettonmultiple(jettonableMulti_[chairId][wJettonIndex]);
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_ADD_SCORE_RESULT, (uint8_t*)data.c_str(), data.size());
	}
	//都下注了，开始发牌 ///
	if ((GetAddScoreCount() + 1) == m_pTableFrame->GetPlayerCount(true)) {
		char msg[1024] = { 0 };
		snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 都下注了，开始摊牌(5s) ......";
		//给各个玩家发牌
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			qzpj::CMD_S_SendCard sendCard;
			for (int j = 0; j < MAX_COUNT; ++j) {
				sendCard.add_cbsendcard(0);
				sendCard.add_cboxcard(0);
			}
			sendCard.set_cbcardtype(0);
			sendCard.set_cbopentime(DELAY_OpenCard);

			sendCard.set_cbcardtype(handCardsType_[i]);

			for (int j = 0; j < MAX_COUNT; ++j) {
				sendCard.set_cbsendcard(j, handCards_[i][j]);
				//sendCard.set_cboxcard(j, cbOXCard[j]);
			}
			sendCard.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
			std::string data = sendCard.SerializeAsString();
			m_pTableFrame->SendTableData(i, qzpj::SUB_S_SEND_CARD, (uint8_t*)data.c_str(), data.size());
		}
		roundOpenTime_ = chrono::system_clock::now();//摊牌开始时间
		aniPlay_.add_START_time(AnimatePlay::OpenE, roundOpenTime_);
		//修改桌子状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_OPEN);
		//都下注了，开始摊牌(5s)
		IsTrustee();
	}
    return 4;
}

//换牌策略分析 ///
//收分时，机器人都拿大牌，玩家拿小牌
//放分时，机器人都拿小牌，玩家拿大牌
//点杀时，机器人都拿大牌，点杀玩家拿最小牌，其他拿次大牌
void CTableFrameSink::AnalysePlayerCards(bool killBlackUsers)
{
	//无机器人
	if (m_pTableFrame->GetAndroidPlayerCount() == 0) {
		return;
	}
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pTableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    bool desc = true;
    if(randomRet<m_difficulty)
    {
        desc = true;
         LOG(ERROR) << "--- *** 总体控制杀分命中概率";
    }
    else
    {

        if(killBlackUsers)
        {
            char msg[1024] = { 0 };
            snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
                m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
            LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 点杀换牌 ...";
            //手牌从大到小对玩家排序，机器人拿大牌
            desc = true;
        }

        //收分
        if (StockScore <= StockLowLimit) {
            desc = true;
            char msg[1024] = { 0 };
            snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
                m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
            LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 收分阶段 ...";
        }
        //放分
        else if (StockScore > StockHighLimit) {
            desc = false;
            char msg[1024] = { 0 };
            snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
                m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
            LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 放分阶段 ...";
        }
        else
        {
            if(!killBlackUsers)
            {
                //正常情况，不用换牌
                char msg[1024] = { 0 };
                snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
                    m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
                LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 正常情况，不换牌 ...";
                return;
            }

        }
        //概率收分或放分
        if ((StockScore <= StockLowLimit && NoExchange == CalcExchangeOrNot2(sysLostSwapCardsRatio_)) ||
            (StockScore > StockHighLimit && NoExchange == CalcExchangeOrNot2(sysWinSwapCardsRatio_))) {

            if(!killBlackUsers)
            {
                char msg[1024] = { 0 };
                snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
                    m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
                LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 不换牌 ...";
                return;
            }

        }
    }

	char msg[1024] = { 0 };
	snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
	LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 换牌 !!!!!! ...";
	//临时变量
	uint8_t cards[GAME_PLAYER][MAX_COUNT];
	memcpy(cards, handCards_, sizeof(cards));
	uint8_t ty[GAME_PLAYER];
	memcpy(ty, handCardsType_, sizeof(ty));
	//收分时，手牌从大到小对玩家排序，机器人拿大牌
	//放分时，手牌从小到大对玩家排序，机器人拿小牌
	std::vector<uint32_t> sortedUsers;
	int c = m_pTableFrame->GetPlayerCount(true);
	for (int x = 0; x < c; ++x) {
		//收分时，当前玩家中最大牌
		//放分时，当前玩家中最小牌
		uint32_t chairId = INVALID_CHAIR;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//已经有了跳过
			if (std::find(
				std::begin(sortedUsers),
				std::end(sortedUsers), i) != sortedUsers.end()) {
				continue;
			}
			if (chairId == INVALID_CHAIR) {
				chairId = i;
				continue;
			}
			if ((PJ::CGameLogic::CompareHandCards(
				&(handCards_[i])[0], &(handCards_[chairId])[0]) > 0) == desc) {
				chairId = i;
			}
		}
		sortedUsers.push_back(chairId);
	}
	//需要点杀玩家
	if (killBlackUsers) {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			uint32_t chairId = INVALID_CHAIR;
			if (m_pTableFrame->IsAndroidUser(i)) {
				//靠前的换给机器人
				chairId = sortedUsers.front();
				sortedUsers.erase(sortedUsers.begin());
			}
			else if (isBlackUser_[i]) {
				//靠尾的换给待点杀玩家，拿最小的牌
				chairId = sortedUsers.back();
				sortedUsers.pop_back();
			}
			if (chairId != INVALID_CHAIR && chairId != i) {
				//手牌/牌型
				memcpy(&(handCards_[i])[0], &(cards[chairId])[0], MAX_COUNT);
				handCardsType_[i] = ty[chairId];
			}
		}
		//非点杀玩家的牌介于机器人和点杀玩家之间
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (m_pTableFrame->IsAndroidUser(i)) {
				continue;
			}
			//非点杀玩家拿次大的牌
			if (!isBlackUser_[i]) {
				uint32_t chairId = sortedUsers.back();
				sortedUsers.pop_back();
				if (chairId != i) {
					//手牌/牌型
					memcpy(&(handCards_[i])[0], &(cards[chairId])[0], MAX_COUNT);
					handCardsType_[i] = ty[chairId];
				}
			}
		}
	}
	else {
		//换牌操作
		uint32_t chairId = INVALID_CHAIR;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (m_pTableFrame->IsAndroidUser(i)) {
				//靠前的换给机器人
				chairId = sortedUsers.front();
				sortedUsers.erase(sortedUsers.begin());
			}
			else {
				//靠尾的换给玩家
				chairId = sortedUsers.back();
				sortedUsers.pop_back();
			}
			if (chairId != i) {
				//手牌/牌型
				memcpy(&(handCards_[i])[0], &(cards[chairId])[0], MAX_COUNT);
				handCardsType_[i] = ty[chairId];
			}
		}
	}
}

//输出玩家手牌/对局日志初始化
void CTableFrameSink::ListPlayerCards() {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//对局日志
		m_replay.addPlayer(UserIdBy(i), ByChairId(i)->GetAccount(), ScoreByChairId(i), i);
		//机器人AI
		if (m_pTableFrame->IsAndroidUser(i)) {
			LOG(WARNING/*ERROR*/) << "\n========================================================================\n"
				<< "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 机器人 [" << i << "] " << UserIdBy(i) << " 手牌 ["
				<< PJ::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT)
				<< "] 牌型 [" << PJ::CGameLogic::StringHandTy(PJ::HandTy(handCardsType_[i])) << "]\n";
		}
		else {
			LOG(WARNING) << "\n========================================================================\n"
				<< "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 玩家 [" << i << "] " << UserIdBy(i) << " 手牌 ["
				<< PJ::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT)
				<< "] 牌型 [" << PJ::CGameLogic::StringHandTy(PJ::HandTy(handCardsType_[i])) << "]\n";
		}
	}
}

//摊牌 ///
void CTableFrameSink::OnTimerOpenCard()
{
    //销毁定时器
    ThisThreadTimer->cancel(timerOpenCardID_);

	//发送摊牌结果
	qzpj::CMD_S_OpenCardResult OpenCardResult;
	//定时器到期，未摊牌的强制摊牌
 	for (int i = 0; i < GAME_PLAYER; i++) {
 		if (!m_pTableFrame->IsExistUser(i)) {
 			continue;
 		}
// 		//已摊牌了跳过
// 		if (-1 != openCardType_[i]) {
// 			continue;
// 		}
// 		OnUserOpenCard(i);

		//标记摊牌类型
		openCardType_[i] = handCardsType_[i];
		//
		::qzpj::PlayerResult* result = OpenCardResult.add_result();
		//
		result->set_chairid(i);
		//玩家手牌/牌型
		::qzpj::HandCards* pcards = result->mutable_handcards();
		pcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
		pcards->set_ty(handCardsType_[i]);
		//OpenCardResult.set_wopencarduser(chairId);
		//OpenCardResult.set_cbcardtype(handCardsType_[chairId]);
		//for (int j = 0; j < MAX_COUNT; ++j)
		//{
			//OpenCardResult.add_cbcarddata(handCards_[chairId][j]);
			//OpenCardResult.add_cboxcarddata(cbOXCard[j]);
		//}
 	}
	
	std::string data = OpenCardResult.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_OPEN_CARD_RESULT, (uint8_t*)data.c_str(), data.size());

	char msg[1024] = { 0 };
	snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
	LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 都摊牌了，结束游戏(2s) ......";
	//游戏结束
	OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
	if (curRoundTurn_ < MAX_ROUND) {
		if (isGameOver_) {
			roundEndTime_ = chrono::system_clock::now();//本局结束时间
			aniPlay_.add_START_time(AnimatePlay::GameEndE, roundEndTime_);
			//修改桌子状态
			m_pTableFrame->SetGameStatus(GAME_STATUS_PREEND);
		}
		else {
			roundEndTime_ = chrono::system_clock::now();//本局结束时间
			aniPlay_.add_START_time(AnimatePlay::NextE, roundEndTime_);
			//修改桌子状态
			m_pTableFrame->SetGameStatus(GAME_STATUS_NEXT);
		}
	}
	else {
		roundEndTime_ = chrono::system_clock::now();//本局结束时间
		aniPlay_.add_START_time(AnimatePlay::GameEndE, roundEndTime_);
		//修改桌子状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_PREEND);
	}
	//都摊牌了，结束游戏(2s)
	IsTrustee();

	//char msg[1024] = { 0 };
	snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
	LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " 超时，结束摊牌 ......";
}

//已摊牌人数 ///
int CTableFrameSink::GetOpenCardCount() {
	int c = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//过滤没有摊牌玩家
		if (-1 == openCardType_[i])
			continue;
		++c;
	}
	return c;
}

//玩家摊牌 ///
int CTableFrameSink::OnUserOpenCard(int chairId)
{
#if 0
	//已摊牌了返回
    if (-1 != openCardType_[chairId]) {
        return 1;
    }
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		LOG_WARN << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 摊牌 !!!!!!";
	}
	else {
		LOG_WARN << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 摊牌 !!!!!!";
	}
    //标记摊牌类型
	openCardType_[chairId] = handCardsType_[chairId];
    //发送摊牌结果
    qzpj::CMD_S_OpenCardResult OpenCardResult;
    OpenCardResult.set_wopencarduser( chairId);
    OpenCardResult.set_cbcardtype(handCardsType_[chairId]);
	for (int j = 0; j < MAX_COUNT; ++j)
    {
        OpenCardResult.add_cbcarddata(handCards_[chairId][j]);
        //OpenCardResult.add_cboxcarddata(cbOXCard[j]);
    }

    std::string data = OpenCardResult.SerializeAsString();
    m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_OPEN_CARD_RESULT, (uint8_t*)data.c_str(), data.size());

    //所有人都摊牌了
    if (GetOpenCardCount() == m_pTableFrame->GetPlayerCount(true)) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 都摊牌了，结束游戏(2s) ......";
        //游戏结束
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
		//修改桌子状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_END);
		//都摊牌了，结束游戏(2s)
		IsTrustee();
        return 10;
    }
#endif
    return 12;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    //清除所有定时器
    ClearAllTimer();
    switch (cbReason)
    {
        case GER_NORMAL:
        {//正常退出
            NormalGameEnd(chairId);
            break;
        }
        // case GER_USER_LEFT:
        // {//玩家强制退出,则把该玩家托管不出。
        //     break;
        // }
        case GER_DISMISS:
        {//游戏解散
            break;
        }
        default:
            break;
    }
	//通知框架结束游戏
    ///*m_bContinueServer = */m_pTableFrame->ConcludeGame(GAME_STATUS_END);
}

int64_t CTableFrameSink::CalculateAndroidRevenue(int64_t score)
{
	//return m_pTableFrame->CalculateRevenue(score);
    return score * (m_pTableFrame->GetGameRoomInfo()->systemReduceRatio) / 1000.0;
}

#if 0
//正常结束游戏结算 ///
void CTableFrameSink::NormalGameEnd(int dchairId)
{
	//游戏结束时间
	roundEndTime_ = chrono::system_clock::now();
	//游戏结束命令
	qzpj::CMD_S_GameEnd GameEnd;
	//计算玩家积分
	std::vector<tagScoreInfo> scoreInfos;
	//玩家手牌字符串
	std::string strCardsList;
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
	//闲家输的人数
	int losers = 0;
	//庄家抢庄倍数 ///
	int X = callBanker_[bankerUser_];
	//闲家下注倍数 ///
	int64_t Y[GAME_PLAYER] = { 0 };
	//各闲家与庄家比牌 //////
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//跳过庄家
		if (i == bankerUser_) {
			continue;
		}
		//闲家下注倍数 ///
		Y[i] = jettonableMulti_[i][jettonIndex_[i]];

		//A：房间底注
		//X：庄家抢庄倍数		Y：闲家下注倍数

		//庄家赢 ///
		if (PJ::CGameLogic::CompareHandCards(&(handCards_[bankerUser_])[0], &(handCards_[i])[0]) > 0) {
			//输赢积分
			int64_t deltascore = CellScore * X * Y[i];
			//闲家输分 = A(房间底注) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[i] -= deltascore;
			//庄家赢分 = A(房间底注) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[bankerUser_] += deltascore;
			//闲家输的人数
			++losers;
		}
		//闲家赢 ///
		else {
			//输赢积分
			int64_t deltascore = CellScore * X * Y[i];
			//庄家输分 = A(房间底注) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[bankerUser_] -= deltascore;
			//闲家赢分 = A(房间底注) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[i] += deltascore;
		}
	}
	//桌面总输赢积分 ///
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//闲家携带积分 ///
		userScore[i] = ScoreByChairId(i);
		//输赢分取绝对值最小 //////
		if (userScore[i] < llabs(userWinScore[i])) {
			//玩家输赢积分
			userWinScore[i] = userWinScore[i] < 0 ?
				-userScore[i] : userScore[i];
		}
		if (userWinScore[i] < 0) {
			//桌面输的积分
			totalLostScore += userWinScore[i];
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] " << i << " [" << UserIdBy(i) << "] 输了 " << userWinScore[i];
		}
		else {
			//桌面赢的积分
			totalWinScore += userWinScore[i];
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] " << i << " [" << UserIdBy(i) << "] 赢了 " << userWinScore[i];
		}
	}
	LOG(ERROR) << "--- *** [" << strRoundID_ << "] "
		<< "\n******************************** 桌面总输分：" << totalLostScore
		<< "\n******************************** 桌面总赢分：" << totalWinScore;
	assert(totalWinScore >= 0);
	assert(totalLostScore <= 0);
	//桌面输赢分取绝对值最小(|X|和|Y|比较大小)
	if (llabs(totalLostScore) < totalWinScore) {
		//|X| < |Y| 玩家赢分换算
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (userWinScore[i] > 0) {
				//以桌面输分|X|为标准按赢分比例计算赢分玩家赢的积分
				//玩家实际赢分 = |X| * 玩家赢分占比(玩家赢分 / |Y|)
				userWinScore[i] = llabs(totalLostScore) * userWinScore[i] / totalWinScore;
			}
		}
	}
	else {
		//|X| > |Y| 玩家输分换算
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (userWinScore[i] < 0) {
				//以桌面赢分|Y|为标准按输分比例计算输分玩家输的积分
				//玩家实际输分 = |Y| * 玩家输分占比(玩家输分 / |X|)
				userWinScore[i] = totalWinScore * userWinScore[i] / llabs(totalLostScore);
			}
		}
	}
	//庄家有效投注 = 所有闲家有效投注之和 ///
	uint64_t bankerBetScore = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (i != bankerUser_) {
			//[i]是闲家
			bankerBetScore += llabs(userWinScore[i]);
		}
	}
	std::stringstream iStr;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		iStr << GlobalFunc::converToHex(handCards_[i], MAX_COUNT);
		GameEnd.add_dlwscore(0);
		GameEnd.add_dtotalscore(0);
		GameEnd.add_cboperate(0);
	}
	iStr << bankerUser_;
	//计算输赢积分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		GameEnd.set_cboperate(i, 0);
		{
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
			if (i != bankerUser_) {
				//[i]是闲家
				uint8_t t[1] = { 0xFF & Y[i] };
				strCardsList = iStr.str() + GlobalFunc::converToHex(t, 1);
				//有效投注
				scoreInfo.rWinScore = llabs(userWinScore[i]);//税前输赢
			}
			else {
				//[i]是庄家
				uint8_t t[1] = { 0xFF & X };
				strCardsList = iStr.str() + GlobalFunc::converToHex(t, 1);
				scoreInfo.isBanker = 1;
				scoreInfo.rWinScore = bankerBetScore;
			}
			scoreInfo.cardValue = strCardsList;//本局开牌
			//scoreInfo.rWinScore = llabs(userWinScore[i]);//税前输赢
			scoreInfo.addScore = userWinScorePure[i];//本局输赢
			//scoreInfo.betScore = llabs(userWinScore[i]);//总投注/有效投注/输赢绝对值(系统抽水前)
			scoreInfo.betScore = scoreInfo.rWinScore;
			scoreInfo.revenue = revenue[i];//本局税收
			scoreInfo.startTime = roundStartTime_;//本局开始时间
			scoreInfos.push_back(scoreInfo);
			//对局日志
			m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore,
				to_string(handCardsType_[i]) + ":" + GlobalFunc::converToHex(handCards_[i], MAX_COUNT), i == bankerUser_);
			//前端显示机器人和真实玩家抽水一致
			GameEnd.set_dlwscore(i, userWinScorePure[i]);
			GameEnd.set_dtotalscore(i, userLeftScore[i]);
		}
	}
	//闲家输的人数，判断通杀通赔
	if (losers == 0) {
		//通赔
		GameEnd.set_cbendtype(2);
	}
	else if (losers == (m_pTableFrame->GetPlayerCount(true) - 1)) {
		//通杀
		GameEnd.set_cbendtype(1);
	}
	else {
		GameEnd.set_cbendtype(0);
	}
	//广播玩家比牌和结算数据
	std::string data = GameEnd.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
	//系统抽水
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

	//保存对局结果
	m_pTableFrame->SaveReplay(m_replay);

	//发送跑马灯
	//PaoMaDeng();
}
#else
//正常结束游戏结算 ///
void CTableFrameSink::NormalGameEnd(int dchairId)
{
	//游戏结束时间
	roundEndTime_ = chrono::system_clock::now();
	//游戏结束命令
	qzpj::CMD_S_GameEnd GameEnd;
	//计算玩家积分
	std::vector<tagScoreInfo> scoreInfos;
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	qzpj::CMD_S_GameRecordDetail details;
	details.set_gameid(ThisGameId);
	//玩家手牌字符串
	std::string strCardsList;
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
    double  stockWeak = 0.0;
	//闲家输赢积分
	int64_t totalWinScore = 0, totalLostScore = 0;
	//闲家输的人数
	int losers = 0;
	//庄家抢庄倍数 ///
	int X = callBanker_[bankerUser_];
	//闲家下注倍数 ///
	int64_t Y[GAME_PLAYER] = { 0 };
	//庄家携带积分 ///
	userScore[bankerUser_] = ScoreByChairId(bankerUser_);
	//庄家有效投注，所有闲家有效投注之和 ///
	uint64_t bankerBetScore = 0;
	//牌局倍数 ///
	int32_t calcMulti[GAME_PLAYER] = { 0 };
	//各闲家与庄家比牌 //////
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//跳过庄家
		if (i == bankerUser_) {
			continue;
		}
		//闲家携带积分 ///
		userScore[i] = ScoreByChairId(i);
		//闲家下注倍数 ///
		Y[i] = jettonableMulti_[i][jettonIndex_[i]];
		int result = PJ::CGameLogic::CompareHandCards(&(handCards_[bankerUser_])[0], &(handCards_[i])[0]);
		//A：房间底注
		//X：庄家抢庄倍数		Y：闲家下注倍数

		//庄家赢 ///
		if (result > 0) {
			//输赢积分 = A(房间底注) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			int64_t deltascore = CellScore * X * Y[i];
			if (userScore[i] < deltascore) {
				deltascore = userScore[i];
			}
			//闲家输分
			userWinScore[i] -= deltascore;
			//闲家总输分
			totalLostScore += deltascore;
			//庄家赢分
			userWinScore[bankerUser_] += deltascore;
			//闲家输的人数
			++losers;
			//牌局倍数
			calcMulti[i] = X * Y[i];
			calcMulti[bankerUser_] += X * Y[i];
		}
		//庄闲和 ///
		else if (result == 0) {
		}
		//闲家赢 ///
		else {
			//输赢积分 = A(房间底注) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			int64_t deltascore = CellScore * X * Y[i];
			if (userScore[i] < deltascore) {
				deltascore = userScore[i];
			}
			//闲家赢分
			userWinScore[i] += deltascore;
			//闲家总赢分
			totalWinScore += deltascore;
			//庄家输分
			userWinScore[bankerUser_] -= deltascore;
			//牌局倍数
			calcMulti[i] = X * Y[i];
			calcMulti[bankerUser_] -= X * Y[i];
		}
	}
	//庄家赢
	if (userWinScore[bankerUser_] > 0) {
		//庄家赢分 > 庄家携带积分，先赔付
		if (userScore[bankerUser_] < userWinScore[bankerUser_]) {
			//庄家赢分
			userWinScore[bankerUser_] = userScore[bankerUser_];
			//闲家应输的积分 = 庄家携带积分 + 庄家赢分 + 庄家赔付积分 - 庄家携带积分
			int64_t W = userWinScore[bankerUser_] + totalWinScore;
			//均摊给输的闲家
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				//跳过庄家
				if (i == bankerUser_) {
					continue;
				}
				//闲家输分
				if (userWinScore[i] < 0) {
					userWinScore[i] = W * userWinScore[i] / totalLostScore;
				}
			}
		}
	}
	//庄家输
	else {
		//庄家输分 > 庄家携带积分，不够赔，先赢
		if (userWinScore[bankerUser_] + userScore[bankerUser_] < 0) {
			//庄家输分
			userWinScore[bankerUser_] = -userScore[bankerUser_];
			//庄家赔付积分 = 庄家携带积分 + 闲家输的积分
			int64_t W = userScore[bankerUser_] + totalLostScore;
			//均摊给赢的闲家
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				//跳过庄家
				if (i == bankerUser_) {
					continue;
				}
				//闲家赢分
				if (userWinScore[i] > 0) {
					userWinScore[i] = W * userWinScore[i] / totalWinScore;
				}
			}
		}
	}
	//庄家有效投注 = 所有闲家有效投注之和 ///
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (i != bankerUser_) {
			//庄家有效投注
			bankerBetScore += llabs(userWinScore[i]);
		}
		if (userWinScore[i] < 0) {
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
			LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " " << (i == bankerUser_ ? "庄 " : "闲 ") << i << " [" << UserIdBy(i) << "] 输了 " << userWinScore[i];
		}
		else {
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%d:%d:%d]",
				m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
			LOG(ERROR) << "--- *** [" << strRoundID_ << "][" << curRoundTurn_ << "]" << msg << " " << (i == bankerUser_ ? "庄 " : "闲 ") << i << " [" << UserIdBy(i) << "] 赢了 " << userWinScore[i];
		}
	}
	std::stringstream iStr;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		iStr << PJ::CGameLogic::hexString(handCards_[i], MAX_COUNT);//GlobalFunc::converToHex(handCards_[i], MAX_COUNT);
		GameEnd.add_dlwscore(0);
		GameEnd.add_dtotalscore(0);
		GameEnd.add_cboperate(0);
	}
	iStr << bankerUser_;
	//计算输赢积分
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		qzpj::PlayerRecordDetail* detail = details.add_detail();
		//boost::property_tree::ptree item, itemscores, itemscorePure;
		//账号/昵称
		detail->set_account(std::to_string(UserIdBy(i)));
		//座椅ID
		detail->set_chairid(i);
		//是否庄家
		detail->set_isbanker(bankerUser_ == i ? true : false);
		//手牌/牌型
		qzpj::HandCards* pcards = detail->mutable_handcards();
		pcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
		pcards->set_ty(handCardsType_[i]);
		//携带积分
		detail->set_userscore(ScoreByChairId(i));
		//房间底注
		detail->set_cellscore(CellScore);
		//下注/抢庄倍数
		detail->set_multi((bankerUser_ == i) ? callBanker_[i] : jettonableMulti_[i][jettonIndex_[i]]);
		//牌局倍数
		detail->set_calcmulti(calcMulti[i]);
		//第几轮
		detail->set_roundturn(curRoundTurn_);
		GameEnd.set_cboperate(i, 0);
		{
			//盈利玩家
			if (userWinScore[i] > 0) {
				//系统抽水，真实玩家和机器人都按照5%抽水计算，前端显示和玩家扣分一致
				revenue[i] = m_pTableFrame->CalculateRevenue(userWinScore[i]);
			}
			//输赢积分，扣除系统抽水
			userWinScorePure[i] = userWinScore[i] - revenue[i];
			//剩余积分
			userLeftScore[i] = userScore[i] + userWinScorePure[i];
			//
			userLeftScore_[i] = userLeftScore[i];
			userWinScorePure_[i] = userWinScorePure[i];
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
			if (i != bankerUser_) {
				//[i]是闲家
				uint8_t t[1] = { 0xFF & Y[i] };
				strCardsList = iStr.str() + GlobalFunc::converToHex(t, 1);
				//有效投注
				scoreInfo.rWinScore = llabs(userWinScore[i]);//税前输赢
			}
			else {
				//[i]是庄家
				uint8_t t[1] = { 0xFF & X };
				strCardsList = iStr.str() + GlobalFunc::converToHex(t, 1);
				scoreInfo.isBanker = 1;
				scoreInfo.rWinScore = bankerBetScore;
			}
			scoreInfo.cardValue = strCardsList;//本局开牌
			//scoreInfo.rWinScore = llabs(userWinScore[i]);//税前输赢
			scoreInfo.addScore = userWinScorePure[i];//本局输赢
			//scoreInfo.betScore = llabs(userWinScore[i]);//总投注/有效投注/输赢绝对值(系统抽水前)
			scoreInfo.betScore = scoreInfo.rWinScore;
			scoreInfo.revenue = revenue[i];//本局税收
			scoreInfo.startTime = roundStartTime_;//本局开始时间
			scoreInfos.push_back(scoreInfo);
			//对局日志
			m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore,
				to_string(handCardsType_[i]) + ":" + PJ::CGameLogic::hexString(handCards_[i], MAX_COUNT), i == bankerUser_);
			//输赢积分
			detail->set_winlostscore(userWinScorePure[i]);
			//前端显示机器人和真实玩家抽水一致
			GameEnd.set_dlwscore(i, userWinScorePure[i]);
			GameEnd.set_dtotalscore(i, userLeftScore[i]);
			//跑马灯消息
			if (userWinScorePure[i] > m_pTableFrame->GetGameRoomInfo()->broadcastScore) {
				std::string msg;
// 				if (handCardsType_[i] == PJ::Tysp) { //散牌
// 					msg = "散牌";
// 				}
// 				else if (handCardsType_[i] == PJ::Ty20) { //对子
// 					msg = "对子";
// 				}
// 				else if (handCardsType_[i] == PJ::Ty123) { //顺子
// 					msg = "顺子";
// 				}
// 				else if (handCardsType_[i] == PJ::Tysc) {//金花(同花)
// 					msg = "金花";
// 				}
// 				else if (handCardsType_[i] == PJ::Ty123sc) {//顺金(同花顺)
// 					msg = "顺金";
// 				}
// 				else if (handCardsType_[i] == PJ::Ty30) {//豹子(炸弹)
// 					msg = "豹子";
// 				}
				m_pTableFrame->SendGameMessage(i, msg, SMT_GLOBAL | SMT_SCROLL, userWinScorePure[i]);
				LOG(INFO) << " --- *** [" << strRoundID_ << "][" << curRoundTurn_ << "] 跑马灯信息 userid = " << UserIdBy(i)
					<< " " << msg << " score = " << userWinScorePure[i];
			}
		}
	}
	//闲家输的人数，判断通杀通赔
	if (losers == 0) {
		//通赔
		GameEnd.set_cbendtype(2);
	}
	else if (losers == (m_pTableFrame->GetPlayerCount(true) - 1)) {
		//通杀
		GameEnd.set_cbendtype(1);
	}
	else {
		GameEnd.set_cbendtype(0);
	}
	//系统抽水
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
	//继续游戏
	int gameOver = 0;
	if (curRoundTurn_ < MAX_ROUND) {
		//玩家积分小于入场分则提前结束
		isGameOver_ = !CheckEnterScore(userLeftScore);
		if (isGameOver_) {
			//提前结束
			gameOver = 1;
		}
	}
	else {
		//正常结束
		gameOver = 2;
	}
	GameEnd.set_cbgameover(gameOver);
	//广播玩家比牌和结算数据
	std::string data = GameEnd.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, qzpj::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		m_replay.detailsData = details.SerializeAsString();
	}
	//保存对局结果
	m_pTableFrame->SaveReplay(m_replay);
}
#endif

bool CTableFrameSink::CheckEnterScore(int64_t* userLeftScore) {
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//真实玩家积分不足，不能进入游戏桌子
		if (userLeftScore[i]/*ScoreByChairId(i)*/ < EnterMinScore) {
			return false;
		}
	}
	return true;
}

//游戏结束，清理数据 ///
void CTableFrameSink::OnTimerGameEnd()
{
    //销毁定时器
	ThisThreadTimer->cancel(timerGameEndID_);
	//通知框架结束游戏
	/*m_bContinueServer = */m_pTableFrame->ConcludeGame(GAME_STATUS_END);
	//清理房间内玩家
	for (uint8_t i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
		//用户空闲状态
		userItem->SetUserStatus(sFree);
		//清除用户信息
		//m_pTableFrame->ClearTableUser(i, true, true);
		m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
	}
	//清理所有定时器
	ClearAllTimer(true);
	//初始化桌子数据
	ClearGameData();
	//重置桌子初始状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
}

//游戏消息
bool CTableFrameSink::OnGameMessage( uint32_t chairId, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
	switch (wSubCmdID)
	{
	case qzpj::SUB_C_CALL_BANKER: { //叫庄 ///
		//玩家无效
		if (chairId == INVALID_CHAIR ||
			!m_pTableFrame->IsExistUser(chairId)) {
			return false;
		}
		//游戏状态不对
		if (GAME_STATUS_CALL != m_pTableFrame->GetGameStatus()) {
			return false;
		}
		//消息结构
		qzpj::CMD_C_CallBanker reqdata;
		reqdata.ParseFromArray(pData, wDataSize);
		int wRtn = OnUserCallBanker(chairId, reqdata.cbcallflag());
		return wRtn > 0;
	}
	case qzpj::SUB_C_ADD_SCORE: { //下注 ///
		//玩家无效
		if (chairId == INVALID_CHAIR ||
			!m_pTableFrame->IsExistUser(chairId)) {
			return false;
		}
		//游戏状态不对
		if (GAME_STATUS_SCORE != m_pTableFrame->GetGameStatus()) {
			return false;
		}
		//消息结构
		qzpj::CMD_C_AddScore reqdata;
		reqdata.ParseFromArray(pData, wDataSize);
		int wRtn = OnUserAddScore(chairId, reqdata.cbjettonindex());
		return wRtn > 0;
	}
// 	case qzpj::SUB_C_OPEN_CARD: { //摊牌 ///
// 		//玩家无效
// 		if (chairId == INVALID_CHAIR ||
// 			!m_pTableFrame->IsExistUser(chairId)) {
// 			return false;
// 		}
// 		//游戏状态不对
// 		if (GAME_STATUS_OPEN != m_pTableFrame->GetGameStatus()) {
// 			return false;
// 		}
// 		int wRtn = OnUserOpenCard(chairId);
// 		return wRtn > 0;
// 	}
	case qzpj::SUB_C_SCENE: {
		OnEventGameScene2(chairId);
		break;
	}
	}
	return true;
}

//清除所有定时器
void CTableFrameSink::ClearAllTimer(bool bv)
{
	if (bv) {
		ThisThreadTimer->cancel(timerGameReadyID_);
	}
	ThisThreadTimer->cancel(timerCallBankerID_);
	ThisThreadTimer->cancel(timerAddScoreID_);
	ThisThreadTimer->cancel(timerOpenCardID_);
	ThisThreadTimer->cancel(timerGameEndID_);
}

//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    uint32_t dchairId = chairId;
    shared_ptr<CServerUserItem> pIServerUser = m_pTableFrame->GetTableUserItem(chairId);
    if(!pIServerUser)
    {
        return false;
    }

    dchairId = pIServerUser->GetChairId();
    if(dchairId >= GAME_PLAYER || dchairId != chairId)
    {
        return false;
    }
    switch (m_pTableFrame->GetGameStatus())
    {
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_START:
    {
        qzpj::MSG_GS_FREE rspdata;
		//房间底注
		rspdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundStartTime_);
		//uint32_t leftTime = nowsec >= DELAY_RoundStart ? 0 : nowsec;
		//rspdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_START, chrono::system_clock::now());
		rspdata.set_lefttime(aniPlay_.leftTime);
		//rspdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		rspdata.set_chairid(chairId);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = rspdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
		}
		//房间状态
		rspdata.set_status(m_pTableFrame->GetGameStatus());
		//第几轮
		rspdata.set_roundturn(curRoundTurn_);
		//剩余牌
		rspdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
        //发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
        qzpj::MSG_GS_CALL rspdata;
		//房间底注
		rspdata.set_cellscore(CellScore);
		//剩余时间
        //uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t( roundCallTime_);
        //uint32_t leftTime = nowsec >= DELAY_Callbanker ? 0 : nowsec;		//剩余时间
		//rspdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_CALL, chrono::system_clock::now());
		rspdata.set_lefttime(aniPlay_.leftTime);
		//rspdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		rspdata.set_chairid(chairId);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = rspdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			//玩家叫庄倍数(-1:未叫 0:不叫 1,2,3,4:叫庄)
			player->set_callbanker(callBanker_[i]);
			//可用的叫庄倍数
			if (callBanker_[i] == -1 && i == chairId) {
				for (int i = 0; i < MAX_BANKER_MUL; ++i) {
					player->add_callablemulti(callableMulti_[chairId][i]);
				}
			}
		}
		//房间状态
		rspdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		rspdata.set_roundid(strRoundID_);
		//第几轮
		rspdata.set_roundturn(curRoundTurn_);
		//剩余牌
		rspdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
        //发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE_CALL, (uint8_t*)data.c_str(), data.size());
        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
		qzpj::MSG_GS_SCORE rspdata;
		//房间底注
		rspdata.set_cellscore(CellScore);
		//剩余时间
        //uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundScoreTime_);
        //uint32_t leftTime = nowsec >=DELAY_AddScore ? 0 : nowsec;		//剩余时间
		//rspdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_SCORE, chrono::system_clock::now());
		rspdata.set_lefttime(aniPlay_.leftTime);
		//rspdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		rspdata.set_chairid(chairId);
		//庄家用户
		rspdata.set_bankeruser(bankerUser_);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = rspdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			if (i == bankerUser_) {
				//庄家叫庄倍数
				player->set_callbanker(callBanker_[bankerUser_]);
			}
			else {
				//闲家下注倍数
				player->set_jetton(jettonIndex_[i] >= 0 ? jettonableMulti_[i][jettonIndex_[i]] : -1);
				//可用的下注倍数
				if (jettonIndex_[i] == -1 && i == chairId) {
					assert(MAX_JETTON_MUL == JettionList.size());
					for (int i = 0; i < MAX_JETTON_MUL; ++i) {
						player->add_jettonablemulti(jettonableMulti_[chairId][i]);
					}
				}
			}
		}
		//房间状态
		rspdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		rspdata.set_roundid(strRoundID_);
		//第几轮
		rspdata.set_roundturn(curRoundTurn_);
		//剩余牌
		rspdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
        //发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE_SCORE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_OPEN:		//摊牌场景消息
    {
        qzpj::MSG_GS_OPEN rspdata;
		//房间底注
		rspdata.set_cellscore(CellScore);
		//剩余时间
        //uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundOpenTime_);
        //uint32_t leftTime = nowsec >= DELAY_OpenCard ? 0 : nowsec;		//剩余时间
		//rspdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_OPEN, chrono::system_clock::now());
		rspdata.set_lefttime(aniPlay_.leftTime);
		//rspdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		rspdata.set_chairid(chairId);
		//庄家用户
		rspdata.set_bankeruser(bankerUser_);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = rspdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			if (i == bankerUser_) {
				//庄家叫庄倍数
				player->set_callbanker(callBanker_[bankerUser_]);
			}
			else {
				//闲家下注倍数
				player->set_jetton(jettonIndex_[i] >= 0 ? jettonableMulti_[i][jettonIndex_[i]] : -1);
			}
			if (openCardType_[i] != -1) {
				player->set_ty(openCardType_[i]);
				player->set_cards((&handCards_[i])[0], MAX_COUNT);
			}
		}
		//房间状态
		rspdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		rspdata.set_roundid(strRoundID_);
		//第几轮
		rspdata.set_roundturn(curRoundTurn_);
		//剩余牌
		rspdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
        //发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE_OPEN, (uint8_t*)data.c_str(), data.size());
        break;
    }
	case GAME_STATUS_NEXT:
	case GAME_STATUS_PREEND:
    case GAME_STATUS_END:
	{
		qzpj::MSG_GS_END rspdata;
		//房间底注
		rspdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundEndTime_);
		//uint32_t leftTime = nowsec >= DELAY_NextRound ? 0 : nowsec;		//剩余时间
		//rspdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(m_pTableFrame->GetGameStatus(), chrono::system_clock::now());
		rspdata.set_lefttime(aniPlay_.leftTime);
		//rspdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		rspdata.set_chairid(chairId);
		//庄家用户
		rspdata.set_bankeruser(bankerUser_);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = rspdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			player->set_wlscore(userWinScorePure_[i]);
			if (i == bankerUser_) {
				//庄家叫庄倍数
				player->set_callbanker(callBanker_[bankerUser_]);
			}
			else {
				//闲家下注倍数
				player->set_jetton(jettonIndex_[i] >= 0 ? jettonableMulti_[i][jettonIndex_[i]] : -1);
			}
			player->set_ty(openCardType_[i]);
			player->set_cards((&handCards_[i])[0], MAX_COUNT);
		}
		//房间状态
		rspdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		rspdata.set_roundid(strRoundID_);
		//第几轮
		rspdata.set_roundturn(curRoundTurn_);
		//剩余牌
		rspdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
		//继续游戏
		int gameOver = 0;
		if (curRoundTurn_ < MAX_ROUND) {
			//玩家积分小于入场分则提前结束
			isGameOver_ = !CheckEnterScore(userLeftScore_);
			if (isGameOver_) {
				//提前结束
				gameOver = 1;
			}
		}
		else {
			//正常结束
			gameOver = 2;
		}
		rspdata.set_gameover(gameOver);
		//发送数据
		std::string data = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE_END, (uint8_t*)data.c_str(), data.size());
		break;
	}
    default:
        break;
    }

    if(pIServerUser->GetUserStatus() == sOffline)  //left failt
    {
        pIServerUser->SetUserStatus(sPlaying);
    }
    return true;
}

//发送场景
bool CTableFrameSink::OnEventGameScene2(uint32_t chairId)
{
    uint32_t dchairId = chairId;
    shared_ptr<CServerUserItem> pIServerUser = m_pTableFrame->GetTableUserItem(chairId);
    if(!pIServerUser)
    {
        return false;
    }

    dchairId = pIServerUser->GetChairId();
    if(dchairId >= GAME_PLAYER || dchairId != chairId)
    {
        return false;
    }
	qzpj::CMD_S_Scene rspdata;
    switch (m_pTableFrame->GetGameStatus())
    {
    case GAME_STATUS_READY:			//空闲状态
    case GAME_STATUS_START:
    {
		qzpj::MSG_GS_FREE msgdata;
		//房间底注
		msgdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundStartTime_);
		//uint32_t leftTime = nowsec >= DELAY_RoundStart ? 0 : nowsec;
		//msgdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_START, chrono::system_clock::now());
		msgdata.set_lefttime(aniPlay_.leftTime);
		//msgdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		msgdata.set_chairid(chairId);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = msgdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
		}
		//房间状态
		msgdata.set_status(m_pTableFrame->GetGameStatus());
		//第几轮
		msgdata.set_roundturn(curRoundTurn_);
		//剩余牌
		msgdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());

		rspdata.set_sceneid(qzpj::SC_GAMESCENE_FREE);
		rspdata.set_data(msgdata.SerializeAsString());
		//发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE/*qzpj::SC_GAMESCENE_FREE*/, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
		qzpj::MSG_GS_CALL msgdata;
		//房间底注
		msgdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundCallTime_);
		//uint32_t leftTime = nowsec >= DELAY_Callbanker ? 0 : nowsec;		//剩余时间
		//msgdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_CALL, chrono::system_clock::now());
		msgdata.set_lefttime(aniPlay_.leftTime);
		//msgdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		msgdata.set_chairid(chairId);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = msgdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			//玩家叫庄倍数(-1:未叫 0:不叫 1,2,3,4:叫庄)
			player->set_callbanker(callBanker_[i]);
			//可用的叫庄倍数
			if (callBanker_[i] == -1 && i == chairId) {
				for (int i = 0; i < MAX_BANKER_MUL; ++i) {
					player->add_callablemulti(callableMulti_[chairId][i]);
				}
			}
		}
		//房间状态
		msgdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		msgdata.set_roundid(strRoundID_);
		//第几轮
		msgdata.set_roundturn(curRoundTurn_);
		//剩余牌
		msgdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());

		rspdata.set_sceneid(qzpj::SC_GAMESCENE_CALL);
		rspdata.set_data(msgdata.SerializeAsString());
        //发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE/*SC_GAMESCENE_CALL*/, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
		qzpj::MSG_GS_SCORE msgdata;
		//房间底注
		msgdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundScoreTime_);
		//uint32_t leftTime = nowsec >= DELAY_AddScore ? 0 : nowsec;		//剩余时间
		//msgdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_SCORE, chrono::system_clock::now());
		msgdata.set_lefttime(aniPlay_.leftTime);
		//msgdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		msgdata.set_chairid(chairId);
		//庄家用户
		msgdata.set_bankeruser(bankerUser_);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = msgdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			if (i == bankerUser_) {
				//庄家叫庄倍数
				player->set_callbanker(callBanker_[bankerUser_]);
			}
			else {
				//闲家下注倍数
				player->set_jetton(jettonIndex_[i] >= 0 ? jettonableMulti_[i][jettonIndex_[i]] : -1);
				//可用的下注倍数
				if (jettonIndex_[i] == -1 && i == chairId) {
					assert(MAX_JETTON_MUL == JettionList.size());
					for (int i = 0; i < MAX_JETTON_MUL; ++i) {
						player->add_jettonablemulti(jettonableMulti_[chairId][i]);
					}
				}
			}
		}
		//房间状态
		msgdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		msgdata.set_roundid(strRoundID_);
		//第几轮
		msgdata.set_roundturn(curRoundTurn_);
		//剩余牌
		msgdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());

		rspdata.set_sceneid(qzpj::SC_GAMESCENE_SCORE);
		rspdata.set_data(msgdata.SerializeAsString());
        //发送数据
        std::string data = rspdata/*GameScore*/.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE/*SC_GAMESCENE_SCORE*/, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_OPEN:		//摊牌场景消息
    {
		qzpj::MSG_GS_OPEN msgdata;
		//房间底注
		msgdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundOpenTime_);
		//uint32_t leftTime = nowsec >= DELAY_OpenCard ? 0 : nowsec;		//剩余时间
		//msgdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(GAME_STATUS_OPEN, chrono::system_clock::now());
		msgdata.set_lefttime(aniPlay_.leftTime);
		//msgdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		msgdata.set_chairid(chairId);
		//庄家用户
		msgdata.set_bankeruser(bankerUser_);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = msgdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			if (i == bankerUser_) {
				//庄家叫庄倍数
				player->set_callbanker(callBanker_[bankerUser_]);
			}
			else {
				//闲家下注倍数
				player->set_jetton(jettonIndex_[i] >= 0 ? jettonableMulti_[i][jettonIndex_[i]] : -1);
			}
			if (openCardType_[i] != -1) {
				player->set_ty(openCardType_[i]);
				player->set_cards((&handCards_[i])[0], MAX_COUNT);
			}
		}
		//房间状态
		msgdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		msgdata.set_roundid(strRoundID_);
		//第几轮
		msgdata.set_roundturn(curRoundTurn_);
		//剩余牌
		msgdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());

		rspdata.set_sceneid(qzpj::SC_GAMESCENE_OPEN);
		rspdata.set_data(msgdata.SerializeAsString());
        //发送数据
        std::string data = rspdata.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE/*SC_GAMESCENE_OPEN*/, (uint8_t*)data.c_str(), data.size());
        break;
    }
	case GAME_STATUS_NEXT:
	case GAME_STATUS_PREEND:
	case GAME_STATUS_END: {
		qzpj::MSG_GS_END msgdata;
		//房间底注
		msgdata.set_cellscore(CellScore);
		//剩余时间
		//uint32_t nowsec =
		//	chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		//	chrono::system_clock::to_time_t(roundEndTime_);
		//uint32_t leftTime = nowsec >= DELAY_NextRound ? 0 : nowsec;		//剩余时间
		//msgdata.set_lefttime(leftTime);
		aniPlay_.calcAnimateDelay(m_pTableFrame->GetGameStatus(), chrono::system_clock::now());
		msgdata.set_lefttime(aniPlay_.leftTime);
		//msgdata.set_anity(aniPlay_.aniTy);
		//玩家座椅ID
		msgdata.set_chairid(chairId);
		//庄家用户
		msgdata.set_bankeruser(bankerUser_);
		//玩家数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userInfo = m_pTableFrame->GetTableUserItem(i);
			if (!userInfo) {
				continue;
			}
			::qzpj::PlayerInfo* player = msgdata.add_infos();
			player->set_userid(userInfo->GetUserId());
			player->set_nickname(userInfo->GetNickName());
			player->set_headindex(userInfo->GetHeaderId());
			player->set_location(userInfo->GetLocation());
			player->set_status(userInfo->GetUserStatus());
			player->set_chairid(userInfo->GetChairId());
			player->set_score(ScoreByChairId(i));
			player->set_wlscore(userWinScorePure_[i]);
			if (i == bankerUser_) {
				//庄家叫庄倍数
				player->set_callbanker(callBanker_[bankerUser_]);
			}
			else {
				//闲家下注倍数
				player->set_jetton(jettonIndex_[i] >= 0 ? jettonableMulti_[i][jettonIndex_[i]] : -1);
			}
			player->set_ty(openCardType_[i]);
			player->set_cards((&handCards_[i])[0], MAX_COUNT);
		}
		//房间状态
		msgdata.set_status(m_pTableFrame->GetGameStatus());
		//牌局编号
		msgdata.set_roundid(strRoundID_);
		//第几轮
		msgdata.set_roundturn(curRoundTurn_);
		//剩余牌
		msgdata.set_cards((char const*)g.LeftCards(), (size_t)g.Remaining());
		//继续游戏
		int gameOver = 0;
		if (curRoundTurn_ < MAX_ROUND) {
			//玩家积分小于入场分则提前结束
			isGameOver_ = !CheckEnterScore(userLeftScore_);
			if (isGameOver_) {
				//提前结束
				gameOver = 1;
			}
		}
		else {
			//正常结束
			gameOver = 2;
		}
		msgdata.set_gameover(gameOver);
		rspdata.set_sceneid(qzpj::SC_GAMESCENE_END);
		rspdata.set_data(msgdata.SerializeAsString());
		//发送数据
		std::string data = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(dchairId, qzpj::SC_GAMESCENE/*SC_GAMESCENE_END*/, (uint8_t*)data.c_str(), data.size());
		break;
	}
    }

    if(pIServerUser->GetUserStatus() == sOffline)  //left failt
    {
        pIServerUser->SetUserStatus(sPlaying);
    }
    return true;
}

//读取配置
void CTableFrameSink::ReadConfig()
{
	assert(m_pTableFrame);
	//系统当前库存
	//m_pTableFrame->GetStorageScore(storageInfo_);

	CINIHelp ini;
	ini.Open(INI_FILENAME);

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
	//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
	{
		std::string str1(ini.GetString("Probability", "ratio_"));
		ratioSwap_ = atoi(str1.c_str());
	}
	//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
	{
		std::string str1(ini.GetString("Probability", "ratioLost_"));
		ratioSwapLost_ = atoi(str1.c_str());
	}
	//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
	{
		std::string str1(ini.GetString("Probability", "ratioWin_"));
		ratioSwapWin_ = atoi(str1.c_str());
	}
	//发牌时尽量都发好牌的概率，玩起来更有意思
	{
		std::string str1(ini.GetString("Probability", "ratioDealGood_"));
		ratioDealGood_ = atoi(str1.c_str());
	}
	//点杀时尽量都发好牌的概率
	{
		std::string str1(ini.GetString("Probability", "ratioDealGoodKill_"));
		ratioDealGoodKill_ = atoi(str1.c_str());
	}
	{
		std::string str1(ini.GetString("path", "cardList"));
		INI_CARDLIST_ = str1;
	}
}

//得到桌子实例
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

//删除桌子实例
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pSink)
{
    pSink.reset();
}
