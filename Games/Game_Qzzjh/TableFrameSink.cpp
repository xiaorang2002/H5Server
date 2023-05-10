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
#include "proto/Qzzjh.Message.pb.h"

#include "../QPAlgorithm/zjh.h"
#include "../QPAlgorithm/cfg.h"
#include "CMD_Game.h"

#include "TableFrameSink.h"

//////////////////////////////////////////////////////////////////////////
//定时器时间
#define TIME_READY						1					//准备定时器(2S)
#define TIME_ENTER_PLAYING              1
#define TIME_CALL_BANKER				5					//叫庄定时器(5S)
#define TIME_ADD_SCORE					5					//下注定时器(5S)
#define TIME_OPEN_CARD					5					//摊牌定时器(5S)
#define TIME_GAME_END					1					//结束定时器(5S)
#define TIME_MATCH_GAME_END             2				//结束定时器(5S)

//叫庄倍数 1,2,4
uint8_t s_CallBankerMulti_[MAX_BANKER_MUL] = { 1,2,4 };	

class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log/qzzjh";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("qzzjh");
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
		//玩家叫庄(-1:未叫, 0:不叫, 1,2,4:叫庄)
		callBankerFlag_[i] = -1;
		//玩家下注(-1:未下注, 大于-1:对应筹码索引)
		addScoreIndex_[i] = -1;
		//玩家摊牌(-1:未摊牌, 大于-1:对应手牌牌型)
		openCardType_[i] = -1;
	}
	//可用的叫庄倍数
	memset(multiCallable_, 0, sizeof(multiCallable_));
	//可用的下注倍数
	memset(multiJettonable_, 0, sizeof(multiJettonable_));
	//对局日志
	m_replay.clear();
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
		//玩家叫庄(-1:未叫, 0:不叫, 1,2,4:叫庄)
		callBankerFlag_[i] = -1;
		//玩家下注(-1:未下注, 大于-1:对应筹码索引)
		addScoreIndex_[i] = -1;
		//玩家摊牌(-1:未摊牌, 大于-1:对应手牌牌型)
		openCardType_[i] = -1;
	}
	//可用的叫庄倍数
	memset(multiCallable_, 0, sizeof(multiCallable_));
	//可用的下注倍数
	memset(multiJettonable_, 0, sizeof(multiJettonable_));
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
	//对局日志
    m_replay.clear();
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
		//assert(m_pTableFrame->GetPlayerCount(true) == MaxGamePlayers_);
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
		timerGameReadyID_ = ThisThreadTimer->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
		snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d)，等待人数(%d)", m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		LOG(ERROR) << msg;
	}
	//游戏初始状态，非首个玩家加入桌子
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 游戏初始状态，非首个玩家加入桌子 ...", m_pTableFrame->GetTableId());
		LOG(ERROR) << msg;
		//assert(false);
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
			ClearAllTimer();
			//初始化桌子数据
			InitGameData();
			//重置桌子初始状态
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
			//saveUserIDs_[i] = 0;
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
void CTableFrameSink::OnGameStart()
{
	char msg[4096] = { 0 };
	m_replay.clear();
	//确定游戏进行中
	//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_START);
	//清除所有定时器
	ClearAllTimer();
	//系统当前库存
	m_pTableFrame->GetStorageScore(storageInfo_);
	LOG_ERROR << "totalStock:" <<storageInfo_.lEndStorage<< ",Lowlimit:"<<storageInfo_.lLowlimit<< ",Uplimit:"<<storageInfo_.lUplimit;
	//牌局编号
	strRoundID_ = m_pTableFrame->GetNewRoundId();
	//对局日志
	m_replay.gameinfoid = strRoundID_;
	//本局开始时间
	roundStartTime_ = chrono::system_clock::now();
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
				LOG(ERROR) << " --- *** [" << strRoundID_ << "] *** blackList"
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
			int n = ZJH::CGameLogic::MakeCardList(lines[index], &(handCards_[i])[0], MAX_COUNT);
			assert(n == MAX_COUNT);
			//手牌排序
			//ZJH::CGameLogic::SortCards(&(handCards_[i])[0], MAX_COUNT, false, false, false);
			//手牌牌型
			handCardsType_[i] = ZJH::CGameLogic::GetHandCardsType(&(handCards_[i])[0]);
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
				//ZJH::CGameLogic::SortCards(&(handCards_[i])[0], MAX_COUNT, false, false, false);
				//手牌牌型
				handCardsType_[i] = ZJH::CGameLogic::GetHandCardsType(&(handCards_[i])[0]);
				//对局日志
				//m_replay.addPlayer(userItem->GetUserId(), userItem->GetAccount(), userItem->GetUserScore(), i);
			}
			//好牌数量
			int c = 0;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				if (ZJH::HandTy(handCardsType_[i]) > ZJH::Tysp) {
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
					LOG(ERROR) << "--- *** [" << strRoundID_ << "] 点杀发好牌 !!!...";
					goto restart;
				}
			}
			//好牌数量不够，重新洗牌发牌
			else if (ratioDealGood <= ratioDealGood_ && c < (m_pTableFrame->GetPlayerCount(true) - 1)) {
				LOG(ERROR) << "--- *** [" << strRoundID_ << "] 概率发好牌 !!!...";
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
		qzzjh::CMD_S_GameStartAi  reqdata;
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
			m_pTableFrame->SendTableData(i, qzzjh::SUB_S_GAME_START_AI, (uint8_t*)data.c_str(), data.size());
		}
	}
	//广播游戏开始消息
	{
		qzzjh::CMD_S_GameStart reqdata;
		//牌局编号
		reqdata.set_roundid(strRoundID_);
		//叫庄时间
		reqdata.set_cbcallbankertime(TIME_CALL_BANKER);
		//确定叫庄倍数 ///
		for (int i = 0; i < GAME_PLAYER; ++i) {
			reqdata.add_cbplaystatus(m_pTableFrame->IsExistUser(i));
			if (!m_pTableFrame->IsExistUser(i)) {
				for (int j = 0; j < MAX_BANKER_MUL; ++j) {
					multiCallable_[i][j] = 0;
					reqdata.add_cbcallbankermultiple(multiCallable_[i][j]);
				}
				continue;
			}
			//最大可叫庄倍数 玩家携带/底注/其他玩家人数/10
			int64_t maxCallTimes = ScoreByChairId(i) / (CellScore * (m_pTableFrame->GetPlayerCount(true) - 1) * 10);
			//最小只能叫1倍
			multiCallable_[i][0] = s_CallBankerMulti_[0];
			reqdata.add_cbcallbankermultiple(multiCallable_[i][0]);
			//其它倍数能叫或不能叫
			for (int j = 1; j < MAX_BANKER_MUL; ++j) {
				//可用的叫庄倍数
				if (maxCallTimes >= s_CallBankerMulti_[j]) {
					multiCallable_[i][j] = s_CallBankerMulti_[j];
					reqdata.add_cbcallbankermultiple(multiCallable_[i][j]);
				}
				else {
					//不可用的叫庄倍数
					multiCallable_[i][j] = 0;
					reqdata.add_cbcallbankermultiple(multiCallable_[i][j]);
				}
			}
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			std::string ss;
			char msg[512] = { 0 };
			for (int j = 0; j < JettionList.size(); ++j) {
				if (j == 0) {
					snprintf(msg, sizeof(msg), "[%d", multiCallable_[i][j]);
				}
				else {
					snprintf(msg, sizeof(msg), ",%d", multiCallable_[i][j]);
				}
				ss += msg;
			}
			ss += "]";
			if (m_pTableFrame->IsAndroidUser(i)) {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "] 机器人 [" << i << "] " << UserIdBy(i)
					<< " 可叫庄倍数 " << ss;
			}
			else {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "] 玩家 [" << i << "] " << UserIdBy(i)
					<< " 可叫庄倍数 " << ss;
			}
		}
		//发送数据
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());
	}
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
	int dwShowTime = 0;
	//游戏开始，开始叫庄(5s)
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_CALL) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 叫庄开始 ......";
		ThisThreadTimer->cancel(timerGameReadyID_);
		ThisThreadTimer->cancel(timerCallBankerID_);
		dwShowTime = 2; //发牌动画  1.5
		bankerUser_ = INVALID_CHAIR;
		roundCallTime_ = chrono::system_clock::now();//叫庄开始时间
		timerCallBankerID_ = ThisThreadTimer->runAfter(TIME_CALL_BANKER + dwShowTime, boost::bind(&CTableFrameSink::OnTimerCallBanker, this));
	}
	//都叫庄了，开始下注(5s)
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_SCORE) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 下注开始 ......";
		ThisThreadTimer->cancel(timerCallBankerID_);
		ThisThreadTimer->cancel(timerAddScoreID_);
		dwShowTime = 2; //抢庄动画  3.0
		roundScoreTime_ = chrono::system_clock::now();//下注开始时间
		timerAddScoreID_ = ThisThreadTimer->runAfter(TIME_ADD_SCORE + dwShowTime, boost::bind(&CTableFrameSink::OnTimerAddScore, this));
	}
	//都下注了，开始摊牌(5s)
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_OPEN) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 摊牌开始 ......";
		ThisThreadTimer->cancel(timerAddScoreID_);
		ThisThreadTimer->cancel(timerOpenCardID_);
		dwShowTime = 1; //摊牌动画  2.0
		roundOpenTime_ = chrono::system_clock::now();//摊牌开始时间
		timerOpenCardID_ = ThisThreadTimer->runAfter(TIME_OPEN_CARD + dwShowTime, boost::bind(&CTableFrameSink::OnTimerOpenCard, this));
	}
	//都摊牌了，结束游戏(2s)
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_END) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 结束游戏 ......";
		ThisThreadTimer->cancel(timerOpenCardID_);
		ThisThreadTimer->cancel(timerGameEndID_);
		roundEndTime_ = chrono::system_clock::now();//本局结束时间
		if (m_isMatch)
			timerGameEndID_ = ThisThreadTimer->runAfter(TIME_MATCH_GAME_END, boost::bind(&CTableFrameSink::OnTimerGameEnd, this));
		else
			timerGameEndID_ = ThisThreadTimer->runAfter(TIME_GAME_END, boost::bind(&CTableFrameSink::OnTimerGameEnd, this));
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
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[chairId])[0], &(handCards_[i])[0]) < 0) {
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
		if (callBankerFlag_[i] != -1) {
			continue;
		}
		//默认不叫 ///
		OnUserCallBanker(i, 0);
	}
	LOG(ERROR) << "--- *** [" << strRoundID_ << "] 超时，结束叫庄 ......";
}

//已叫庄人数 ///
int CTableFrameSink::GetCallBankerCount() {
	int c = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//过滤没有叫庄玩家
		if (-1 == callBankerFlag_[i]) {
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
	if (-1 != callBankerFlag_[chairId]) {
		return 1;
	}
	//叫庄标志错误
	if (wCallFlag < 0) {
		return 2;
	}
	//检测是否正确的倍数
	bool bCorrectMultiple = false;
	if (wCallFlag > 0) {
		for (int i = 0; i != MAX_BANKER_MUL; ++i) {
			if (multiCallable_[chairId][i] == wCallFlag) {
				bCorrectMultiple = true;
				break;
			}
		}
		if (bCorrectMultiple == false) {
			if (m_pTableFrame->IsAndroidUser(chairId)) {
				LOG(ERROR) << "--- *** [" << strRoundID_ << "] 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 <<失败>> !!! callflag = " << wCallFlag;
			}
			else {
				LOG(ERROR) << "--- *** [" << strRoundID_ << "] 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 <<失败>> !!! callflag = " << wCallFlag;
			}
			wCallFlag = 0;
			//return 3;
		}
	}
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 callflag = " << wCallFlag;
	}
	else {
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 叫庄 callflag = " << wCallFlag;
	}
    //记录叫庄
    callBankerFlag_[chairId] = wCallFlag;
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
    m_replay.addStep(nowsec, to_string(wCallFlag),-1,opBet,chairId,chairId);
	//玩家叫庄广播
	{
		//消息结构
		qzzjh::CMD_S_CallBanker reqdata;
		reqdata.set_wcallbankeruser(chairId);
		reqdata.set_cbcallmultiple(wCallFlag);
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_CALL_BANKER, (uint8_t*)data.c_str(), data.size());
	}
	//都叫庄了，确定庄家，开始下注 ///
	if (GetCallBankerCount() == m_pTableFrame->GetPlayerCount(true)) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 都叫庄了，开始下注(5s) ......";
		//消息结构
		qzzjh::CMD_S_CallBankerResult reqdata;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			reqdata.add_cbcallbankeruser(0);
		}
		//叫庄倍数对应叫了庄的玩家
		int callbankerUser[MAX_BANKER_CALL][GAME_PLAYER] = { 0 };
		//叫庄倍数对应被叫庄次数
		int callbankerCount[MAX_BANKER_CALL] = { 0 };  //0 1 2 4 count
		for (int i = 0; i < GAME_PLAYER; ++i) {
			reqdata.set_cbcallbankeruser(i, callBankerFlag_[i]);
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			// 1,2,4
			if (callBankerFlag_[i] > 0) {
				//叫庄倍数
				uint8_t callScore = (uint8_t)callBankerFlag_[i];  //1,2,4
				uint8_t callCount = (uint8_t)callbankerCount[callScore];  //1,2,4 count
				//1,2,4 第 callCount 次叫庄玩家
				callbankerUser[callScore][callCount] = i;
				//每个叫庄倍数对应被叫的次数
				++callbankerCount[callScore];
			}
		}
		//确定庄家，倍数从大到小遍历4,2,1 0 -1
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
			callBankerFlag_[bankerUser_] = s_CallBankerMulti_[0];
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
		int64_t maxBankerCallTimes = ScoreByChairId(bankerUser_) / (CellScore * (m_pTableFrame->GetPlayerCount(true) - 1) * callBankerFlag_[bankerUser_]);
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (m_pTableFrame->IsExistUser(i) && i != bankerUser_) {
				//闲家携带/底注/庄家抢庄倍数
				int64_t maxPlayerCallTimes = ScoreByChairId(i) / (CellScore * callBankerFlag_[bankerUser_]);
				int64_t callMaxTimes = std::min<int64_t>(maxBankerCallTimes, maxPlayerCallTimes);
				if (m_pTableFrame->IsAndroidUser(i)) {
					LOG(WARNING/*ERROR*/) << "--- *** [" << strRoundID_ << "] 机器人 [" << i << "] " << UserIdBy(i)
						<< " callMaxTimes " << callMaxTimes << (i == bankerUser_ ? " 庄" : " 闲");
				}
				else {
					LOG(WARNING) << "--- *** [" << strRoundID_ << "] 玩家 [" << i << "] " << UserIdBy(i)
						<< " callMaxTimes " << callMaxTimes << (i == bankerUser_ ? " 庄" : " 闲");
				}
				//X<5时，根据X的值设对应整数数量的筹码
				if (callMaxTimes < 5) {
					for (int j = 1; j <= JettionList.size(); ++j) {
						if (j <= callMaxTimes) {
							//callMaxTimes==4时  1 2 3 4
							//callMaxTimes==3时  1 2 3
							//callMaxTimes==2时  1 2
							multiJettonable_[i][j - 1] = j;
						}
						else {
							multiJettonable_[i][j - 1] = 0;
						}
						reqdata.add_cbjettonmultiple(multiJettonable_[i][j - 1]);
					}
				}
				//X<20时，floor(X/4) floor(2X/4) floor(3X/4) X
				else if (callMaxTimes < 20) {
					for (int j = 0; j < JettionList.size(); ++j) {
						multiJettonable_[i][j] = callMaxTimes * (j + 1) / 4;
						reqdata.add_cbjettonmultiple(multiJettonable_[i][j]);
					}
				}
				//>=20时，5 10 15 20
				else {
					for (int j = 0; j < JettionList.size(); ++j) {
						multiJettonable_[i][j] = (uint8_t)JettionList[j];
						reqdata.add_cbjettonmultiple(multiJettonable_[i][j]);
					}
				}
			}
			else {
				for (int j = 0; j < JettionList.size(); ++j) {
					multiJettonable_[i][j] = 0;
					reqdata.add_cbjettonmultiple(multiJettonable_[i][j]);
				}
			}
			//下注时间
			reqdata.set_cbaddjettontime(TIME_ADD_SCORE);
		}
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			std::string ss;
			char msg[512] = { 0 };
			for (int j = 0; j < JettionList.size(); ++j) {
				if (j == 0) {
					snprintf(msg, sizeof(msg), "[%d", multiJettonable_[i][j]);
				}
				else {
					snprintf(msg, sizeof(msg), ",%d", multiJettonable_[i][j]);
				}
				ss += msg;
			}
			ss += "]";
			if (m_pTableFrame->IsAndroidUser(i)) {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "] 机器人 [" << i << "] " << UserIdBy(i)
					<< " 可下注筹码 " << ss << (i == bankerUser_ ? " 庄" : " 闲");
			}
			else {
				LOG(WARNING) << "\n--- *** [" << strRoundID_ << "] 玩家 [" << i << "] " << UserIdBy(i)
					<< " 可下注筹码 " << ss << (i == bankerUser_ ? " 庄" : " 闲");
			}
		}
		//广播消息
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_CALL_BANKER_RESULT, (uint8_t*)data.c_str(), data.size());
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
		if (addScoreIndex_[i] != -1) {
			continue;
		}
		//默认下1倍 ///
		OnUserAddScore(i, 0);
	}
	LOG(ERROR) << "--- *** [" << strRoundID_ << "] 超时，结束下注 ......";
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
		if (-1 == addScoreIndex_[i]) {
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
	if (-1 != addScoreIndex_[chairId]) {
		return 2;
	}
	//下注筹码错误 5 10 15 20
	if (wJettonIndex < 0 || wJettonIndex >= JettionList.size()) {
		return 3;
	}
	//可下注倍数无效
	if (multiJettonable_[chairId][wJettonIndex] <= 0) {
		if (m_pTableFrame->IsAndroidUser(chairId)) {
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 下注 <<失败>> !!! index = " << wJettonIndex;
		}
		else {
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 下注 <<失败>> !!! index = " << wJettonIndex;
		}
		wJettonIndex = 0;
		//return 4;
	}
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 下注 index = " << wJettonIndex;
	}
	else {
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 下注 index = " << wJettonIndex;
	}
    //记录下注
    addScoreIndex_[chairId] = wJettonIndex;
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
    m_replay.addStep(nowsec,to_string(multiJettonable_[chairId][wJettonIndex]),-1,opAddBet,chairId,chairId);
	//广播玩家下注
	{
		qzzjh::CMD_S_AddScoreResult reqdata;
		reqdata.set_waddjettonuser(chairId);
		reqdata.set_cbjettonmultiple(multiJettonable_[chairId][wJettonIndex]);
		std::string data = reqdata.SerializeAsString();
		m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_ADD_SCORE_RESULT, (uint8_t*)data.c_str(), data.size());
	}
	//都下注了，开始发牌 ///
	if ((GetAddScoreCount() + 1) == m_pTableFrame->GetPlayerCount(true)) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 都下注了，开始摊牌(5s) ......";
		//给各个玩家发牌
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			qzzjh::CMD_S_SendCard sendCard;
			for (int j = 0; j < MAX_COUNT; ++j) {
				sendCard.add_cbsendcard(0);
				sendCard.add_cboxcard(0);
			}
			sendCard.set_cbcardtype(0);
			sendCard.set_cbopentime(TIME_OPEN_CARD);

			sendCard.set_cbcardtype(handCardsType_[i]);

			for (int j = 0; j < MAX_COUNT; ++j) {
				sendCard.set_cbsendcard(j, handCards_[i][j]);
				//sendCard.set_cboxcard(j, cbOXCard[j]);
			}
			std::string data = sendCard.SerializeAsString();
			m_pTableFrame->SendTableData(i, qzzjh::SUB_S_SEND_CARD, (uint8_t*)data.c_str(), data.size());
		}
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
	bool desc = true;
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pTableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {
        desc = true;
    }
    else
    {
        if (!killBlackUsers) {
            //收分
            if (StockScore <= StockLowLimit) {
                desc = true;
                LOG(ERROR) << "--- *** [" << strRoundID_ << "] 收分阶段 ...";
            }
            //放分
            else if (StockScore > StockHighLimit) {
                desc = false;
                LOG(ERROR) << "--- *** [" << strRoundID_ << "] 放分阶段 ...";
            }
            else {
                //正常情况，不用换牌
                LOG(ERROR) << "--- *** [" << strRoundID_ << "] 正常情况，不换牌 ...";
                return;
            }
            //概率收分或放分
            if ((StockScore <= StockLowLimit && NoExchange == CalcExchangeOrNot2(sysLostSwapCardsRatio_)) ||
                (StockScore > StockHighLimit && NoExchange == CalcExchangeOrNot2(sysWinSwapCardsRatio_))) {
                LOG(ERROR) << "--- *** [" << strRoundID_ << "] 不换牌 ...";
                return;
            }
        }
        else {
            LOG(ERROR) << "--- *** [" << strRoundID_ << "] 点杀换牌 ...";
            //手牌从大到小对玩家排序，机器人拿大牌
            desc = true;
        }
    }

	LOG(ERROR) << "--- *** [" << strRoundID_ << "] 换牌 !!!!!! ...";
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
			if ((ZJH::CGameLogic::CompareHandCards(
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
				<< "--- *** [" << strRoundID_ << "] 机器人 [" << i << "] " << UserIdBy(i) << " 手牌 ["
				<< ZJH::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT)
				<< "] 牌型 [" << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[i])) << "]\n";
		}
		else {
			LOG(WARNING) << "\n========================================================================\n"
				<< "--- *** [" << strRoundID_ << "] 玩家 [" << i << "] " << UserIdBy(i) << " 手牌 ["
				<< ZJH::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT)
				<< "] 牌型 [" << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[i])) << "]\n";
		}
	}
}

//摊牌 ///
void CTableFrameSink::OnTimerOpenCard()
{
    //销毁定时器
    ThisThreadTimer->cancel(timerOpenCardID_);
	//定时器到期，未摊牌的强制摊牌
	for (int i = 0; i < GAME_PLAYER; i++) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//已摊牌了跳过
		if (-1 != openCardType_[i]) {
			continue;
		}
		OnUserOpenCard(i);
	}
	LOG(ERROR) << "--- *** [" << strRoundID_ << "] 超时，结束摊牌 ......";
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
	//已摊牌了返回
    if (-1 != openCardType_[chairId]) {
        return 1;
    }
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 机器人 " << chairId << "[" << UserIdBy(chairId) << "] 摊牌";
	}
	else {
		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 玩家 " << chairId << "[" << UserIdBy(chairId) << "] 摊牌";
	}
    //标记摊牌类型
	openCardType_[chairId] = handCardsType_[chairId];
    //发送摊牌结果
    qzzjh::CMD_S_OpenCardResult OpenCardResult;
    OpenCardResult.set_wopencarduser( chairId);
    OpenCardResult.set_cbcardtype(handCardsType_[chairId]);
	for (int j = 0; j < MAX_COUNT; ++j)
    {
        OpenCardResult.add_cbcarddata(handCards_[chairId][j]);
        //OpenCardResult.add_cboxcarddata(cbOXCard[j]);
    }

    std::string data = OpenCardResult.SerializeAsString();
    m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_OPEN_CARD_RESULT, (uint8_t*)data.c_str(), data.size());

    //所有人都摊牌了
    if (GetOpenCardCount() == m_pTableFrame->GetPlayerCount(true)) {
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] 都摊牌了，结束游戏(2s) ......";
        //游戏结束
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);
		//修改桌子状态
		m_pTableFrame->SetGameStatus(GAME_STATUS_END);
		//都摊牌了，结束游戏(2s)
		IsTrustee();
        return 10;
    }

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
    /*m_bContinueServer = */m_pTableFrame->ConcludeGame(GAME_STATUS_END);
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
	qzzjh::CMD_S_GameEnd GameEnd;
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
	int X = callBankerFlag_[bankerUser_];
	//庄家牌型倍数 ///
	int M = GetMultiple(handCardsType_[bankerUser_]);
	//闲家下注倍数 ///
	int64_t Y[GAME_PLAYER] = { 0 };
	//闲家牌型倍数 ///
	int32_t N[GAME_PLAYER] = { 0 };
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
		Y[i] = multiJettonable_[i][addScoreIndex_[i]];
		//闲家牌型倍数 ///
		N[i] = GetMultiple(handCardsType_[i]);

		//A：房间底注			M：庄家牌型倍数	N：闲家牌型倍数
		//X：庄家抢庄倍数		Y：闲家下注倍数

		//庄家赢 ///
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[bankerUser_])[0], &(handCards_[i])[0]) > 0) {
			//输赢积分
			int64_t deltascore = CellScore * M * X * Y[i];
			//闲家输分 = A(房间底注) * M(庄家牌型倍数) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[i] -= deltascore;
			//庄家赢分 = A(房间底注) * M(庄家牌型倍数) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[bankerUser_] += deltascore;
			//闲家输的人数
			++losers;
		}
		//闲家赢 ///
		else {
			//输赢积分
			int64_t deltascore = CellScore * N[i] * X * Y[i];
			//庄家输分 = A(房间底注) * N(闲家牌型倍数) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			userWinScore[bankerUser_] -= deltascore;
			//闲家赢分 = A(房间底注) * N(闲家牌型倍数) * X(庄家抢庄倍数) * Y(闲家下注倍数)
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
		iStr << GlobalFunc::converToHex(handCards_[i], 3);
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
				to_string(handCardsType_[i]) + ":" + GlobalFunc::converToHex(handCards_[i], 3), i == bankerUser_);
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
	m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
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
	qzzjh::CMD_S_GameEnd GameEnd;
	//计算玩家积分
	std::vector<tagScoreInfo> scoreInfos;
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	qzzjh::CMD_S_GameRecordDetail details;
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
	int X = callBankerFlag_[bankerUser_];
	//庄家牌型倍数 ///
	int M = GetMultiple(handCardsType_[bankerUser_]);
	//闲家下注倍数 ///
	int64_t Y[GAME_PLAYER] = { 0 };
	//闲家牌型倍数 ///
	int32_t N[GAME_PLAYER] = { 0 };
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
		Y[i] = multiJettonable_[i][addScoreIndex_[i]];
		//闲家牌型倍数 ///
		N[i] = GetMultiple(handCardsType_[i]);

		//A：房间底注			M：庄家牌型倍数	N：闲家牌型倍数
		//X：庄家抢庄倍数		Y：闲家下注倍数

		//庄家赢 ///
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[bankerUser_])[0], &(handCards_[i])[0]) > 0) {
			//输赢积分 = A(房间底注) * M(庄家牌型倍数) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			int64_t deltascore = CellScore * M * X * Y[i];
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
			calcMulti[i] = M * X * Y[i];
			calcMulti[bankerUser_] += M * X * Y[i];
		}
		//闲家赢 ///
		else {
			//输赢积分 = A(房间底注) * N(闲家牌型倍数) * X(庄家抢庄倍数) * Y(闲家下注倍数)
			int64_t deltascore = CellScore * N[i] * X * Y[i];
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
			calcMulti[i] = N[i] * X * Y[i];
			calcMulti[bankerUser_] -= N[i] * X * Y[i];
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
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] " << (i == bankerUser_ ? "庄 " : "闲 ") << i << " [" << UserIdBy(i) << "] 输了 " << userWinScore[i];
		}
		else {
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] " << (i == bankerUser_ ? "庄 " : "闲 ") << i << " [" << UserIdBy(i) << "] 赢了 " << userWinScore[i];
		}
	}
	std::stringstream iStr;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		iStr << GlobalFunc::converToHex(handCards_[i], 3);
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
		qzzjh::PlayerRecordDetail* detail = details.add_detail();
		//boost::property_tree::ptree item, itemscores, itemscorePure;
		//账号/昵称
		detail->set_account(std::to_string(UserIdBy(i)));
		//座椅ID
		detail->set_chairid(i);
		//是否庄家
		detail->set_isbanker(bankerUser_ == i ? true : false);
		//手牌/牌型
		qzzjh::HandCards* pcards = detail->mutable_handcards();
		pcards->set_cards(&(handCards_[i])[0], 3);
		pcards->set_ty(handCardsType_[i]);
		//牌型倍数
		pcards->set_multi(GetMultiple(handCardsType_[i]));
		//携带积分
		detail->set_userscore(ScoreByChairId(i));
		//房间底注
		detail->set_cellscore(CellScore);
		//下注/抢庄倍数
		detail->set_multi((bankerUser_ == i) ? callBankerFlag_[i] : multiJettonable_[i][addScoreIndex_[i]]);
		//牌局倍数
		detail->set_calcmulti(calcMulti[i]);

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
				to_string(handCardsType_[i]) + ":" + GlobalFunc::converToHex(handCards_[i], 3), i == bankerUser_);
			//输赢积分
			detail->set_winlostscore(userWinScorePure[i]);
			//前端显示机器人和真实玩家抽水一致
			GameEnd.set_dlwscore(i, userWinScorePure[i]);
			GameEnd.set_dtotalscore(i, userLeftScore[i]);
			//跑马灯消息
			if (userWinScorePure[i] > m_pTableFrame->GetGameRoomInfo()->broadcastScore) {
				std::string msg;
				if (handCardsType_[i] == ZJH::Tysp) { //散牌
					msg = "散牌";
				}
				else if (handCardsType_[i] == ZJH::Ty20) { //对子
					msg = "对子";
				}
				else if (handCardsType_[i] == ZJH::Ty123) { //顺子
					msg = "顺子";
				}
				else if (handCardsType_[i] == ZJH::Tysc) {//金花(同花)
					msg = "金花";
				}
				else if (handCardsType_[i] == ZJH::Ty123sc) {//顺金(同花顺)
					msg = "顺金";
				}
				else if (handCardsType_[i] == ZJH::Ty30) {//豹子(炸弹)
					msg = "豹子";
				}
				m_pTableFrame->SendGameMessage(i, msg, SMT_GLOBAL | SMT_SCROLL, userWinScorePure[i]);
				LOG(INFO) << " --- *** [" << strRoundID_ << "] 跑马灯信息 userid = " << UserIdBy(i)
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
	//广播玩家比牌和结算数据
	std::string data = GameEnd.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, qzzjh::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
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

//游戏结束，清理数据 ///
void CTableFrameSink::OnTimerGameEnd()
{
    //销毁定时器
	ThisThreadTimer->cancel(timerGameEndID_);
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
	ClearAllTimer();
	//初始化桌子数据
	InitGameData();
	//重置桌子初始状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
}

//游戏消息
bool CTableFrameSink::OnGameMessage( uint32_t chairId, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
	switch (wSubCmdID)
	{
	case qzzjh::SUB_C_CALL_BANKER: { //叫庄 ///
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
		qzzjh::CMD_C_CallBanker reqdata;
		reqdata.ParseFromArray(pData, wDataSize);
		int wRtn = OnUserCallBanker(chairId, reqdata.cbcallflag());
		return wRtn > 0;
	}
	case qzzjh::SUB_C_ADD_SCORE: { //下注 ///
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
		qzzjh::CMD_C_AddScore reqdata;
		reqdata.ParseFromArray(pData, wDataSize);
		int wRtn = OnUserAddScore(chairId, reqdata.cbjettonindex());
		return wRtn > 0;
	}
	case qzzjh::SUB_C_OPEN_CARD: { //摊牌 ///
		//玩家无效
		if (chairId == INVALID_CHAIR ||
			!m_pTableFrame->IsExistUser(chairId)) {
			return false;
		}
		//游戏状态不对
		if (GAME_STATUS_OPEN != m_pTableFrame->GetGameStatus()) {
			return false;
		}
		int wRtn = OnUserOpenCard(chairId);
		return wRtn > 0;
	}
	}
	return true;
}

//清除所有定时器
void CTableFrameSink::ClearAllTimer()
{
	//ThisThreadTimer->cancel(timerGameReadyID_);
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
        qzzjh::MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((CellScore));//单元分

		uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundStartTime_);

		uint32_t cbReadyTime = nowsec >= TIME_READY ? 0 : nowsec;		//剩余时间
        GameFree.set_cbreadytime(cbReadyTime);

        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzzjh::SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_CALL:			//叫庄状态
    {
        qzzjh::MSG_GS_CALL GameCall;
        GameCall.set_dcellscore(CellScore);
        uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t( roundCallTime_);
        uint32_t cbTimeLeave = nowsec >= TIME_CALL_BANKER ? 0 : nowsec;		//剩余时间
        GameCall.set_cbtimeleave(cbTimeLeave);
        GameCall.set_roundid(strRoundID_);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
             GameCall.add_scallbanker(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameCall.add_cbplaystatus(m_pTableFrame->IsExistUser(i));//游戏中玩家(1打牌玩家)
            GameCall.set_scallbanker(i, callBankerFlag_[i]);//叫庄标志(-1:未叫, 0:不叫, 124:叫庄)
        }


        for (int i = 0; i < MAX_BANKER_MUL; ++i){
             GameCall.add_cbcallbankermultiple(multiCallable_[chairId][i]);
//             GameCall.add_cbcallbankermultiple(s_CallBankerMulti_[i]);
        }



        GameCall.set_dwuserid(pIServerUser->GetUserId());
//        GameCall.set_cbgender(pIServerUser->GetGender());
        GameCall.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameCall.set_cbviplevel(pIServerUser->GetVipLevel());
        GameCall.set_sznickname(pIServerUser->GetNickName());
//        GameCall.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameCall.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameCall.set_szlocation(pIServerUser->GetLocation());
        GameCall.set_usstatus(pIServerUser->GetUserStatus());
        GameCall.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameCall.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzzjh::SC_GAMESCENE_CALL, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_SCORE:		//下注状态
    {
        qzzjh::MSG_GS_SCORE GameScore;
        GameScore.set_dcellscore((CellScore));
        uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundScoreTime_);
        uint32_t cbTimeLeave = nowsec >=TIME_ADD_SCORE ? 0 : nowsec;		//剩余时间
        GameScore.set_cbtimeleave(cbTimeLeave);
        GameScore.set_wbankeruser(bankerUser_);										//庄家用户
        GameScore.set_cbbankermultiple(callBankerFlag_[bankerUser_]);
        GameScore.set_roundid(strRoundID_);
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbuserjettonmultiple(-1);
        }
        for (int i = 0; i < GAME_PLAYER; i++)
        {
            GameScore.add_cbplaystatus(m_pTableFrame->IsExistUser(i));//游戏中玩家(1打牌玩家)
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
            int cbUserJettonMultiple = ((int)addScoreIndex_[i] >= 0 ? (int)multiJettonable_[i][addScoreIndex_[i]] : 0) ;
                  
            GameScore.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < JettionList.size(); ++i)
        {
            GameScore.add_cbjettonmultiple(multiJettonable_[chairId][i]);
        }
//        for (int i = 0; i < MAX_BANKER_MUL; ++i)
//        {
//            GameScore.add_cbcallbankermultiple(multiCallable_[chairId][i]);//s_CallBankerMulti_[i]);
//        }

        GameScore.set_dwuserid(pIServerUser->GetUserId());
//        GameScore.set_cbgender(pIServerUser->GetGender());
        GameScore.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameScore.set_cbviplevel(pIServerUser->GetVipLevel());
        GameScore.set_sznickname(pIServerUser->GetNickName());
//        GameScore.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameScore.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameScore.set_szlocation(pIServerUser->GetLocation());
        GameScore.set_usstatus(pIServerUser->GetUserStatus());
        GameScore.set_wchairid(pIServerUser->GetChairId());

        //发送数据
        std::string data = GameScore.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzzjh::SC_GAMESCENE_SCORE, (uint8_t*)data.c_str(), data.size());

        break;
    }
    case GAME_STATUS_OPEN:		//摊牌场景消息
    {
        qzzjh::MSG_GS_OPEN GameOpen;
        GameOpen.set_dcellscore((CellScore));
        uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundOpenTime_);
        uint32_t cbTimeLeave = nowsec >= TIME_OPEN_CARD ? 0 : nowsec;		//剩余时间
        GameOpen.set_cbtimeleave(cbTimeLeave);
        GameOpen.set_wbankeruser((int)bankerUser_);
        GameOpen.set_roundid(strRoundID_);
        //庄家用户
        GameOpen.set_cbbankermutiple(callBankerFlag_[bankerUser_]);		//庄家倍数

        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            for(int j=0;j<MAX_COUNT;j++)
            {
                GameOpen.add_cbcarddata(0);
                GameOpen.add_cbhintcard(0);
            }
            GameOpen.add_cbisopencard(0);
            GameOpen.add_cbcardtype(0);
            GameOpen.add_cbplaystatus((int)m_pTableFrame->IsExistUser(i));//游戏中玩家(1打牌玩家)

            GameOpen.add_cbuserjettonmultiple(0);//游戏中玩家(1打牌玩家)

        }
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
            if (openCardType_[i] == (-1))
            {
                if (i == dchairId)
                {
                    GameOpen.set_cbcardtype(i,handCardsType_[i]);


                    int startidx = i * MAX_COUNT;
                    for(int j=0;j<MAX_COUNT;j++)
                    {
                        GameOpen.set_cbcarddata(startidx+j,(int) handCards_[i][j]);
                        //GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                    }
                }
                else
                {
                    GameOpen.set_cbcardtype(i, handCardsType_[i]);
                }
            }
            else
            {
                GameOpen.set_cbisopencard(i,1);
                GameOpen.set_cbcardtype(i, handCardsType_[i]);

                int startidx = i * MAX_COUNT;
                for(int j=0;j<MAX_COUNT;j++)
                {
                    GameOpen.set_cbcarddata(startidx+j,(int)handCards_[i][j]);
                    //GameOpen.set_cbhintcard(startidx+j,(int) m_cbOXCard[i][j]);
                }
            }

			int cbUserJettonMultiple = addScoreIndex_[i] >= 0 ? multiJettonable_[i][addScoreIndex_[i]] : 0;
            GameOpen.set_cbuserjettonmultiple(i,cbUserJettonMultiple);
        }

        for (int i = 0; i < JettionList.size(); ++i)
        {
            GameOpen.add_cbjettonmultiple(multiJettonable_[chairId][i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
            GameOpen.add_cbcallbankermultiple(multiCallable_[chairId][i]);//GameOpen.set_cbcallbankermultiple(i,s_CallBankerMulti_[i]);
        }


        GameOpen.set_dwuserid(pIServerUser->GetUserId());
//        GameOpen.set_cbgender(pIServerUser->GetGender());
        GameOpen.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameOpen.set_cbviplevel(pIServerUser->GetVipLevel());
        GameOpen.set_sznickname(pIServerUser->GetNickName());
//        GameOpen.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameOpen.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameOpen.set_szlocation(pIServerUser->GetLocation());
        GameOpen.set_usstatus(pIServerUser->GetUserStatus());
        GameOpen.set_wchairid(pIServerUser->GetChairId());
        
        //发送数据
        std::string data = GameOpen.SerializeAsString();

        const::google::protobuf::RepeatedField< ::google::protobuf::int32 >& cartype = GameOpen.cbcardtype();
        int res[5];
        for(int i = 0; i < cartype.size(); ++i)
        {
            res[i]=cartype[i];
        }
        m_pTableFrame->SendTableData(dchairId, qzzjh::SC_GAMESCENE_OPEN, (uint8_t*)data.c_str(), data.size());

        break;
    }
#if 0
    case GAME_STATUS_END:
    {
        //结束暂时发送空闲场景
        qzzjh::MSG_GS_FREE GameFree;
        GameFree.set_dcellscore((CellScore));
        GameFree.set_cbreadytime(TIME_READY);
        

        for (int i = 0; i < JettionList.size(); ++i)
        {
            GameFree.add_cbjettonmultiple(multiJettonable_[chairId][i]);
        }
        for (int i = 0; i < MAX_BANKER_MUL; ++i)
        {
//            GameFree.add_cbcallbankermultiple(s_CallBankerMulti_[i]);
            GameFree.add_cbcallbankermultiple(multiCallable_[chairId][i]);
        }


        GameFree.set_dwuserid(pIServerUser->GetUserId());
//        GameFree.set_cbgender(pIServerUser->GetGender());
        GameFree.set_cbheadindex(pIServerUser->GetHeaderId());
//        GameFree.set_cbviplevel(pIServerUser->GetVipLevel());
        GameFree.set_sznickname(pIServerUser->GetNickName());
//        GameFree.set_cbviplevel2(pIServerUser->GetVipLevel());
//        GameFree.set_szheadurl(pIServerUser->GetHeadBoxID());
        GameFree.set_szlocation(pIServerUser->GetLocation());
        GameFree.set_usstatus(pIServerUser->GetUserStatus());
        GameFree.set_chairId(pIServerUser->GetChairId());
        //发送数据
        std::string data = GameFree.SerializeAsString();
        m_pTableFrame->SendTableData(dchairId, qzzjh::SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());
        LOG(INFO)<<"GAME_STATUS_END-------------------------------User Enter"<<(int)dchairId;

        break;
    }
#endif
    default:
        break;
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

    stockWeak=ini.GetFloat("Game_Config", "STOCK_WEAK",1.0);
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
