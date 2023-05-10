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
#include "proto/zjh.Message.pb.h"

#include "MSG_ZJH.h"
#include "../QPAlgorithm/zjh.h"
#include "../QPAlgorithm/cfg.h"

#include "AndroidConfig.h"

#include "GameProcess.h"

class CGLogInit
{
public:
   CGLogInit()
   {
       string dir = "./log/zjh/";
       // initialize the glog content.
       FLAGS_colorlogtostderr = true;
       FLAGS_minloglevel = google::GLOG_INFO;
       FLAGS_log_dir     = dir;
       FLAGS_logbufsecs  = 3;

       if(!boost::filesystem::exists(dir))
       {
           boost::filesystem::create_directories(dir);
       }

       // set std output special log content value.
       google::SetStderrLogging(FLAGS_minloglevel);
       // initialize the log prefix name now.
       google::InitGoogleLogging("zjh");

       //设置文件名
       //=====config=====
       if(!boost::filesystem::exists(INI_FILENAME))
       {
           LOG_INFO << INI_FILENAME <<" not exists";
           return;
       }

       boost::property_tree::ptree pt;
       boost::property_tree::read_ini(INI_FILENAME, pt);
       int errorlevel = pt.get<int>("Global.errorlevel", -1);
       if(errorlevel >= 0)
       {
           FLAGS_minloglevel = errorlevel;
       }

       int printlevel = pt.get<int>("Global.printlevel", -1);
       if(printlevel >= 0)
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

#define TIME_GAME_ADD_SCORE       15 //下注时长(second)
#define TIME_GAME_COMPARE_DELAY   4  //比牌动画时长

CGameProcess::CGameProcess(void)
	:storageInfo_({ 0 })
{
    bankerUser_ = INVALID_CHAIR;
    currentUser_ = INVALID_CHAIR;
    firstUser_ = INVALID_CHAIR;
    currentWinUser_ = INVALID_CHAIR;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		saveUserIDs_[i] = 0;
		comparedUser_[i].clear();
	}
	memset(tableScore_, 0, sizeof(tableScore_));
	//当前第几轮
	currentTurn_ = -1;
	//是否超出最大轮
	isExceed_ = false;
    tableAllScore_ = 0;
    opEndTime_ = 0;
    opTotalTime_ = 0;
    m_mOutCardRecord.clear();

	memset(handCards_,  0, sizeof(handCards_));
    memset(handCardsType_,  0, sizeof(handCardsType_));
    
	memset(playTurn_, 0, sizeof(playTurn_));
    memset(&robot_,  0, sizeof(robot_));
    memset(madeCardNum_,0,sizeof(madeCardNum_));
    memset(madeMaxCards_,0,sizeof(madeMaxCards_));
    memset(handCardColorIndex,0,sizeof(handCardColorIndex));
    memset(&userControlinfo_,0,sizeof(userControlinfo_));
    memset(currentHaveWinScore_,0,sizeof(currentHaveWinScore_));
    controlWinScoreRate_[0] = 20;
    controlWinScoreRate_[1] = 100;
    blastAndroidChangeCard = false;
    kcTimeoutHour_ = 1.0;
	newUserTimeoutMinute_ = 30.0;
    ratiokcreduction_ = 10.0;
    stockWeak = 0.0;
	//当前筹码值
	jettonValue_ = 0;
	//基础跟注分
	followScore_ = 0;

	//累计匹配时长
	totalMatchSeconds_ = 0;
	//分片匹配时长(可配置)
	sliceMatchSeconds_ = 0.2f;
	//超时匹配时长(可配置)
	timeoutMatchSeconds_ = 0.8f;
	//机器人候补空位超时时长(可配置)
	timeoutAddAndroidSeconds_ = 0.4f;
	//放大倍数
	ratioScale_ = 1000;
	//5/4/3/2/单人局的概率
	ratio5_ = 0, ratio4_ = 100, ratio3_ = 0, ratio2_ = 0, ratio1_ = 0;
	//初始化桌子游戏人数概率权重
	initPlayerRatioWeight();
	MaxGamePlayers_ = 0;

	//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
	ratioSwap_ = 100;
	//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
	ratioSwapLost_ = 80;
	//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
	ratioSwapWin_ = 70;
	//发牌时尽量都发好牌的概率，玩起来更有意思
	ratioDealGood_ = 50;
	//点杀时尽量都发好牌的概率
	ratioDealGoodKill_ = 60;
	//剩最后一个AI和真人比牌,需控制系统赢时,换牌的概率
	ratioChangeGoodCard_=60;
	//IP访问限制
	checkIp_ = false;
	//一张桌子真实玩家数量限制
	checkrealuser_ = false;
	//游戏结束是否全部摊牌
	openAllCards_ = false;
	controlType_BlackRoom = 0;
	memset(userOperate_, 0, sizeof(userOperate_));
	userPlayCount = 0;
	memset(haveMadeCardType, 0, sizeof(haveMadeCardType));
	memset(isHaveUserPlay_, 0, sizeof(isHaveUserPlay_));

	controlledProxy.clear();
	controllPoint = 10;
	memset(addBaseTimes_, 0, sizeof(addBaseTimes_));
	isGemeEnd_ = false;
}

CGameProcess::~CGameProcess(void)
{
}

void CGameProcess::openLog(const char *p, ...)
{
	char Filename[256] = { 0 };

	struct tm *t;
	time_t tt;
	time(&tt);
	t = localtime(&tt);

	sprintf(Filename, "log//zjh//zjh_table_%d%d%d_%d_%d.log", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, m_pTableFrame->GetGameRoomInfo()->roomId, m_pTableFrame->GetTableId());
	FILE *fp = fopen(Filename, "a");
	if (NULL == fp)
	{
		return;
	}
	va_list arg;
	va_start(arg, p);
	fprintf(fp, "[%d%d%d %02d:%02d:%02d] [TABLEID:%d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, m_pTableFrame->GetTableId());
	vfprintf(fp, p, arg);
	fprintf(fp, "\n");
	fclose(fp);
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
	// 
	m_pGameRoomInfo = m_pTableFrame->GetGameRoomInfo();
	//定时器指针
	m_TimerLoopThread = m_pTableFrame->GetLoopThread();
    //初始化游戏数据
    InitGameData();
    //读取配置
    ReadStorageInformation();
	//读取机器人配置
	ReadConfig();
	m_pTableFrame->setkcBaseTimes(robot_.kcBaseTimes_,kcTimeoutHour_,ratiokcreduction_);
	//初始化桌子游戏人数概率权重
	initPlayerRatioWeight();
	//放大倍数
	int scale = 1000;
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
	for (int i = 0; i < GAME_PLAYER; ++i) {
		saveUserIDs_[i] = 0;
		//玩家桌面分
		tableScore_[i] = 0;
		//比牌的用户
		comparedUser_[i].clear();
		//防超时弃牌
		noGiveUpTimeout_[i] = true;
		//玩家跟注次数
		followCount[i] = 0;
		//玩家下注详情
		betinfo_[i].clear();
	}
	//用户数据副本
	userinfos_.clear();
	//桌面总下注
    tableAllScore_ = 0;
	//庄家用户
	bankerUser_ == INVALID_CHAIR;
	//操作用户
	currentUser_ = INVALID_CHAIR;
	//首发用户
	firstUser_ = INVALID_CHAIR;
	//当前最大牌用户
	currentWinUser_ = INVALID_CHAIR;
	//当前第几轮
	currentTurn_ = -1;
	//是否超出最大轮
	isExceed_ = false;
    opEndTime_ = 0;
    opTotalTime_ = 0;
    m_mOutCardRecord.clear();
    //扑克变量
    memset(handCards_, 0, sizeof(handCards_));
    memset(handCardsType_, 0, sizeof(handCardsType_));

	memset(turninfo_, 0, sizeof(turninfo_));
	memset(playTurn_, 0, sizeof(playTurn_));

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
	//内部使用游戏状态

	//当前筹码值(初始底注)
	jettonValue_ = CellScore;
	//基础跟注分(初始底注)
	followScore_ = CellScore;

	//是否看牌
	memset(isLooked_, false, sizeof isLooked_);
	//是否弃牌
	memset(isGiveup_, false, sizeof isGiveup_);
	//是否比牌输
	memset(isComparedLost_, false, sizeof isComparedLost_);

	memset(handCardColorIndex,0,sizeof(handCardColorIndex));
	memset(madeMaxCards_,0,sizeof(madeMaxCards_));
	memset(&userControlinfo_,0,sizeof(userControlinfo_));
    memset(currentHaveWinScore_,0,sizeof(currentHaveWinScore_));
    controlWinScoreRate_[0] = 20;
    controlWinScoreRate_[1] = 100;
	memset(userOperate_, 0, sizeof(userOperate_));
	memset(haveMadeCardType, 0, sizeof(haveMadeCardType));
	memset(isHaveUserPlay_, 0, sizeof(isHaveUserPlay_));

	isGemeEnd_ = false;
	controlledProxy.clear();
	controllPoint = 10;
	memset(addBaseTimes_, 0, sizeof(addBaseTimes_));
}


//清除所有定时器
void CGameProcess::ClearAllTimer()
{
    m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	m_TimerLoopThread->getLoop()->cancel(timerIdGameEnd_);
}

//用户进入
bool CGameProcess::OnUserEnter(int64_t userId, bool islookon)
{
	//没有初始化
	if (!m_pTableFrame) {
		return false;
	}
	//桌椅玩家无效
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetUserItemByUserId(userId);
	if (!userItem) {
		return false;
	}
	char msg[1024] = { 0 };
	assert(userItem->GetChairId() >= 0);
	assert(userItem->GetChairId() < GAME_PLAYER);
	snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d] status[%d] OnUserEnter userID[%d] chairID[%d]\n",
		m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), userId, userItem->GetChairId());
	//LOG(ERROR) << msg;
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

// 		LOG(ERROR) << "\n\n--- *** ==============================================================================\n"
// 			<< "--- *** ==============================================================================";
		snprintf(msg, sizeof(msg), "--- *** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ tableID[%d]初始化游戏人数 MaxGamePlayers_：%d", m_pTableFrame->GetTableId(), MaxGamePlayers_);
		//LOG(ERROR) << msg;
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
        m_pTableFrame->GetPlayerCount(true) >= MaxGamePlayers_)
    {
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
		LOG(WARNING/*ERROR*/) << msg;
		GameTimerReadyOver();
	}
	//首个玩家加入桌子
	else if (m_pTableFrame->GetPlayerCount(true) == 1) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 首个玩家加入桌子 ...", m_pTableFrame->GetTableId());
		//LOG(ERROR) << msg;
		//一定是游戏初始状态，否则清理有问题
		//assert(m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT);
		//修改游戏空闲准备
		m_pTableFrame->SetGameStatus(GAME_STATUS_READY);
		//首个加入桌子的必须是真实玩家
		//assert(m_pTableFrame->GetPlayerCount(false) == 1);
		//清理所有定时器
		ClearAllTimer();
		//定时器监控等待其他玩家或机器人加入桌子
		m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
		timerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
		snprintf(msg, sizeof(msg), "--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]当前游戏人数(%d)，等待人数(%d)", m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(true), MaxGamePlayers_);
		//LOG(ERROR) << msg;
	}
	//游戏初始状态，非首个玩家加入桌子
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 游戏初始状态，非首个玩家加入桌子 ...", m_pTableFrame->GetTableId());
		//LOG(ERROR) << msg;
		//assert(false);
	}
	//游戏进行中玩家加入桌子，当作断线重连处理
	else if (m_pTableFrame->GetGameStatus() >= GAME_STATUS_START) {
		snprintf(msg, sizeof(msg), "--- *** tableID[%d] 游戏进行中玩家加入桌子，当作断线重连处理 ...", m_pTableFrame->GetTableId());
		LOG(WARNING/*ERROR*/) << msg;
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
		LOG(WARNING/*ERROR*/) << msg;
	}
	return true;
}


bool CGameProcess::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
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
						//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 积分不足 >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
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
		if (userItem == NULL) {
			return false;
		}
		//用户已出局(弃牌/比牌输)，未离开桌子，刷新页面，断线重连
		if (isGiveup_[userItem->GetChairId()] || isComparedLost_[userItem->GetChairId()]) {
			//清除保留信息
			saveUserIDs_[userItem->GetChairId()] = 0;
			//用户空闲状态
			userItem->SetUserStatus(sFree);
			//清除用户信息
			//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 断线重连 >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << userItem->GetChairId();
			m_pTableFrame->ClearTableUser(userItem->GetChairId(), true, false, ERROR_ENTERROOM_LONGTIME_NOOP);
			return false;
		}
		return true;
	}
	//游戏状态为GAME_STATUS_START(100)时，不会进入该函数
	//assert(false);
	return false;
}

bool CGameProcess::CanLeftTable(int64_t userId)
{
	shared_ptr<CServerUserItem> userItem = ByUserId(userId);
	if (!userItem) {
		return true;
	}
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_START ||
		m_pTableFrame->GetGameStatus() == GAME_STATUS_END ||
		isGiveup_[userItem->GetChairId()] || isComparedLost_[userItem->GetChairId()]) {
		return true;
	}
	return false;
}

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
	//assert(!userItem->IsAndroidUser());
	//桌子非初始状态，玩家离开
	//assert((int)m_pTableFrame->GetGameStatus() != GAME_STATUS_INIT);
	//桌子非游戏状态，玩家离开(中途弃牌/比牌输)
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
	LOG(WARNING/*ERROR*/) << msg;
	//桌子空闲准备状态/用户已出局(弃牌/比牌输) ///
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_READY || isGiveup_[chairId] || isComparedLost_[chairId]) {
		//清除保留信息
		saveUserIDs_[chairId] = 0;
		//用户空闲状态
		userItem->SetUserStatus(sFree);
		//玩家离开广播
		m_pTableFrame->BroadcastUserStatus(userItem, true);
		//清除用户信息
		//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 用户已出局 >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << chairId;
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
			LOG(WARNING/*ERROR*/) << msg;
			//清理机器人
			if (m_pTableFrame->GetPlayerCount(true) > 0) {
				for (int i = 0; i < GAME_PLAYER; ++i) {
					if (!m_pTableFrame->IsExistUser(i)) {
						continue;
					}
					assert(m_pTableFrame->IsAndroidUser(i));
					//清除用户信息
					//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 没有真实玩家 >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
					m_pTableFrame->ClearTableUser(i, true, false);
					currentHaveWinScore_[i] = 0;
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
			LOG(WARNING/*ERROR*/) << msg;
		}
	}
	//用户已出局(弃牌/比牌输) ///
	else if (isGiveup_[chairId] || isComparedLost_[chairId]) {
		snprintf(msg, sizeof(msg), "--- *** ++++++++++++++++ tableID[%d]-OnUserLeft gamestatus[%d] 用户 [%d] 离开桌子，清理用户 playerCount[%d:%d]",
			m_pTableFrame->GetTableId(), m_pTableFrame->GetGameStatus(), chairId,
			m_pTableFrame->GetPlayerCount(false), m_pTableFrame->GetPlayerCount(true));
		LOG(WARNING/*ERROR*/) << msg;
	}
	if (ret)
	{
		currentHaveWinScore_[chairId] = 0;
	}
	
	return ret;
}

//准备定时器
void CGameProcess::OnTimerGameReadyOver() {
	//清除游戏准备定时器
	m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
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
				//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 清理房间内玩家 >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
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
	//游戏已经开始了,就返回
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_START)
	{
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] " << " OnTimerGameReadyOver 游戏已经开始了 ";
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
			//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 清理房间内玩家 AA >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
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
			m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
			//LOG(WARNING) << " --- *** TableId[" << m_pTableFrame->GetTableId() << "] " << " OnTimerGameReadyOver 桌子游戏人数不足，且没有匹配超时，再次启动定时器 ";
			timerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
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
				m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
				//LOG(WARNING) << " --- *** TableId[" << m_pTableFrame->GetTableId() << "] " << " OnTimerGameReadyOver 仍然没有达到最小游戏人数，继续等待 ";
				timerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
				//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]仍然没有达到最小游戏人数(%d)，继续等待...\n", m_pTableFrame->GetTableId(), MIN_GAME_PLAYER);
			}
		}
		else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
			//匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
			//printf("--- *** @@@@@@@@@@@@@@@ tableID[%d]匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", m_pTableFrame->GetTableId(), totalMatchSeconds_, MaxGamePlayers_);
			//定时器检测机器人候补空位后是否达到游戏要求人数
			m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
			//LOG(WARNING) << " --- *** TableId[" << m_pTableFrame->GetTableId() << "] " << " OnTimerGameReadyOver 仍然没有达到最小游戏人数(%d)，继续等待 ";
			timerIdGameReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CGameProcess::OnTimerGameReadyOver, this));
		}
	}
}

//结束准备，开始游戏
void CGameProcess::GameTimerReadyOver()
{
	//确定游戏状态：初始化空闲准备阶段，而非游戏中
	//assert(m_pTableFrame->GetGameStatus() < GAME_STATUS_START);
	//清除游戏准备定时器
	m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
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
				//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " sOffline >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
				m_pTableFrame->ClearTableUser(i, true, true);
			}
			//真实玩家积分不足，不能进入游戏桌子
			else if (userItem->GetUserScore() < EnterMinScore) {
				//设置离线状态
				userItem->SetUserStatus(sOffline);
				//清理玩家信息
				//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " EnterMinScore >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
			}
		}
	}
	//如果玩家人数达到了最小游戏人数
	if (m_pTableFrame->GetPlayerCount(true) >= MIN_GAME_PLAYER) {
		//清理游戏数据
		ClearGameData();
		//初始化数据
		InitGameData();
        //读取游戏配置
        ReadStorageInformation();
		//牌局编号
		strRoundID_ = m_pTableFrame->GetNewRoundId();
		//设置个人库存控制等级
		m_pTableFrame->setkcBaseTimes(robot_.kcBaseTimes_,kcTimeoutHour_,ratiokcreduction_);
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
				//控制初始值
				if (!userItem->IsAndroidUser())
				{
					//黑屋控制数据
					tagBlackRoomlistInfo& blackRoomUser = ByChairId(i)->GetBlackRoomlistInfo();
					int64_t curWinTatalScore = blackRoomUser.roomWin;
					userControlinfo_[i].curIndex = GetCurrentControlLv(i,blackRoomUser.controlTimes);	
					userControlinfo_[i].conScore = blackRoomUser.current;
					userControlinfo_[i].curWeight = robot_.ratiokcBaseTimes_[userControlinfo_[i].curIndex];			
					CalculateControlLv(i, curWinTatalScore);
					currentHaveWinScore_[i] = RandomBetween(controlWinScoreRate_[0]*CellScore,controlWinScoreRate_[1]*CellScore);					
					LOG(ERROR) << "==== 获取控制值,玩家 " << i << " userid:" << userItem->GetUserId() << " curWinTatalScore:" << curWinTatalScore << " conScore:" << userControlinfo_[i].conScore
							   << " curIndex:" << userControlinfo_[i].curIndex << " curWeight:" << userControlinfo_[i].curWeight;	
					
					openLog(" === strRoundID[%s] 获取小黑屋控制值,玩家 userid:%d, curWinTatalScore:%d, conScore:%d, curIndex:%d, curWeight:%d;", strRoundID_.c_str(), userItem->GetUserId(), curWinTatalScore, userControlinfo_[i].conScore, userControlinfo_[i].curIndex, userControlinfo_[i].curWeight);
				}				
			}
		}
		//游戏进行中
		m_pTableFrame->SetGameStatus(GAME_STATUS_START);
		//printf("--- *** &&&&&&&&&&&&&&&&&&& tableID[%d] OnGameStart 游戏开始发牌(%d)\n", m_pTableFrame->GetTableId(), m_pTableFrame->GetPlayerCount(true));
		//开始游戏
		OnGameStart();
		// 当前系统库存
        LOG(WARNING) << "===== [" << strRoundID_ << "] " << "StockScore:" << StockScore <<",StockHighLimit:"<<StockHighLimit<<",StockLowLimit:"<<StockLowLimit;
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
		//重置桌子初始状态
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
	m_TimerLoopThread->getLoop()->cancel(timerIdGameReadyOver);
	//清除已选的牌值
	g.clearHaveCard();
	if (madeCardNum_[0]>10000000)
	{
		memset(madeCardNum_,0,sizeof(madeCardNum_));
	}
	if (++userPlayCount > 200)
	{
		userPlayCount = 1;
	}
	//系统当前库存
	m_pTableFrame->GetStorageScore(storageInfo_);
	// ReReadStock(storageInfo_);
	openLog(" === strRoundID[%s] 当前库存=%d,库存下限=%d,库存上限=%d;", strRoundID_.c_str(), StockScore, StockLowLimit, StockHighLimit);
	//LOG(INFO) << " === strRoundID[" << strRoundID_ << "] 当前库存=" << StockScore << ",库存下限=" << StockLowLimit << ", 库存上限=" << StockHighLimit;
	//牌局编号
    //strRoundID_ = m_pTableFrame->GetNewRoundId();
	//对局日志
	m_replay.gameinfoid = strRoundID_;
	//本局开始时间
    roundStartTime_ = chrono::system_clock::now();
    blastAndroidChangeCard = false;	

	//本局黑名单玩家数
	int blackUserNum = 0;

    //个人库控制开启，并且该玩家超出了上限或者下限，需要个人控制
    int personCtrNum = 0;
	//点杀概率值(千分比)
	int rateLostMax = 0;
	//点杀概率值判断是否本局点杀
	bool killBlackUsers = false;
	bool bTestCard = false;
    int  killOrLose = 0;
    float killOrLoseRatio = 0.0;
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
            tagPersonalProfit& personalCtr = ByChairId(i)->GetPersonalPrott();
            if(personalCtr.isOpenPersonalAction!=0)
            {
                personCtrNum++;
                killOrLose=personalCtr.isOpenPersonalAction;
                killOrLoseRatio=personalCtr.playerInterferenceRatio;

                LOG(ERROR) << "     isOpenPersonalAction"<<personalCtr.isOpenPersonalAction
                           << "     InterferenceRatiolAction"<<personalCtr.playerInterferenceRatio
                           << "     agentRatio"<<personalCtr.agentRatio
                           << "     agentTendayProfit"<<personalCtr.agentTendayProfit
                           << "     isOpenPersonalAction"<<personalCtr.isOpenPersonalAction
                           << "     agentTendayProfit"<<personalCtr.agentTendayProfit
                           << "     playerBetWinRatio"<<personalCtr.playerBetWinRatio
                           << "     playerAllProfit"<<personalCtr.playerAllProfit
                           << "     playerALlbet"<<personalCtr.playerALlbet
                           << "     playerLowerLimit"<<personalCtr.playerLowerLimit
                           << "     playerHighLimit"<<personalCtr.playerHighLimit
                           << "     userid  ="<<m_pTableFrame->GetTableUserItem(i)->GetUserId();

            }

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
        //个人库存触发控制,超出上限或者下限
        if(personCtrNum>0)
        {

            LOG(ERROR) << " 玩家需要个人库存控制";
            //这个值等于一是系统要杀分
            if(killOrLose==1)
            {
                if(RandomBetween(1, 100) <=killOrLoseRatio*100)
                {
                    killOrLose=1;
                }
                else
                {
                    killOrLose=0;
                }
            }
            else if(killOrLose==-1)
            {
                if(RandomBetween(1, 100) <=killOrLoseRatio*100)
                {
                    killOrLose=-1;
                }
                else
                {
                    killOrLose=0;
                }
            }
            else
            {
                killOrLose=0;
            }

        }
        else
        {
            LOG(ERROR) << " 个人库存没有需要控制的玩家";
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
		int scale = 1000;
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
			//清除已选的牌值
			g.clearHaveCard();
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
				/*uint8_t cardType = getMadeCardTypeIndex(robot_.ratioMadeCard_, userControlinfo_[i].curIndex);
				g.MadeHandCards(&(handCards_[i])[0],cardType);
				uint8_t cardIndex = ZJH::CGameLogic::GetHandCardsType(&(handCards_[i])[0]);
				madeCardNum_[0]++;
				madeCardNum_[cardIndex]++;
				haveMadeCardType[cardIndex]++;
                LOG(ERROR) << "===== 需造牌玩家[" << i << "] 牌型 [" << g.StringHandTy(ZJH::HandTy(cardType)) << "] " << " 得到的牌型:" 
                		   << g.StringHandTy(ZJH::HandTy(cardIndex)) << " [" << getCardString(handCards_[i]) << "]";*/
			}
			//做两手好牌，一手给AI，一手给玩家			
			getTwoGoodCard();
		}
		//获取第6手牌
		{
			uint8_t cType = getMadeCardTypeIndex(robot_.ratioMadeCard_);
			g.MadeHandCards(&(madeMaxCards_)[0],cType);
			uint8_t cIndex = ZJH::CGameLogic::GetHandCardsType(&(madeMaxCards_)[0]);
			madeMaxCardsType_ = cIndex;
			madeCardNum_[0]++;
			madeCardNum_[cIndex]++;
            LOG(ERROR) << "===== 需造牌第[6]手牌 牌型 [" << g.StringHandTy(ZJH::HandTy(cType)) << "] " << " 得到的牌型:" 
            		   << g.StringHandTy(ZJH::HandTy(cIndex)) << " [" << getCardString(madeMaxCards_) << "]";
			/*g.DealCards(MAX_COUNT, &(madeMaxCards_)[0]);
			uint8_t cIndex = ZJH::CGameLogic::GetHandCardsType(&(madeMaxCards_)[0]);
			madeMaxCardsType_ = cIndex;*/
		}
		//检查牌值是否重复
		uint8_t color = 0;
		uint8_t value = 0;
		bool bHaveSameCard = false;
		for (int i = 0; i < GAME_PLAYER+1; ++i)
		{
			if (i < GAME_PLAYER)
			{
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				for (int j = 0; j < MAX_COUNT; ++j)
				{
					if (handCards_[i][j]==0)
					{
						continue;
					}
					color = g.GetCardColorValue(handCards_[i][j]);
					value = g.GetCardValue(handCards_[i][j]);
					handCardColorIndex[color][value-1]++;
					if (handCardColorIndex[color][value-1]>1)
					{
						bHaveSameCard = true;
						break;
					}
				}
			}
			else
			{
				for (int j = 0; j < MAX_COUNT; ++j)
				{
					if (madeMaxCards_[j]==0)
					{
						continue;
					}
					color = g.GetCardColorValue(madeMaxCards_[j]);
					value = g.GetCardValue(madeMaxCards_[j]);
					handCardColorIndex[color][value-1]++;
					if (handCardColorIndex[color][value-1]>1)
					{
						bHaveSameCard = true;
						break;
					}
				}
			}
			if (bHaveSameCard)
				break;
		}	
		LOG(ERROR) << "===== 总造牌次数:[" << madeCardNum_[0] << "] 散牌 [" << madeCardNum_[1] << "] " << "大对子 [" << madeCardNum_[2] << "] "
			<< "顺子 [" << madeCardNum_[3] << "] " << "金花 [" << madeCardNum_[4] << "] "
			<< "顺金 [" << madeCardNum_[5] << "] " << "豹子 [" << madeCardNum_[6] << "] ";
			
        if (bHaveSameCard)
		{
			memset(handCardColorIndex,0,sizeof(handCardColorIndex));
			memset(madeMaxCards_,0,sizeof(madeMaxCards_));
			LOG(ERROR) << "===== 造牌重复了,重新造牌========";
			goto restart;
		}
		//if (bTestCard)
		//{
		//	uint8_t testCard[GAME_PLAYER][MAX_COUNT] = {
		//								   {0x07, 0x27, 0x05},
		//								   {0x0d, 0x36, 0x24},
		//								   {0x3c, 0x39, 0x09},
		//								   {0x11, 0x21, 0x37},
		//								   {0x0b, 0x2a, 0x29}} ;
		//   uint8_t testCard1[MAX_COUNT] = { 0x06, 0x17, 0x18 };
		//   uint8_t testCard6[MAX_COUNT] = { 0x08, 0x09, 0x0a };
		//   memcpy(madeMaxCards_, testCard6, sizeof(testCard6));
		//   uint8_t cIndex = ZJH::CGameLogic::GetHandCardsType(&(madeMaxCards_)[0]);
		//   madeMaxCardsType_ = cIndex;
		//   for (int i = 0; i < GAME_PLAYER; ++i)
		//   {
		//	   if (!m_pTableFrame->IsExistUser(i)) {
		//		   continue;
		//	   }
		//	   if (m_pTableFrame->IsAndroidUser(i))
		//	   {
		//		   memcpy(&handCards_[i],testCard[i],sizeof(testCard[i]));
		//	   }
		//	   else
		//	   {
		//		   memcpy(&handCards_[i],testCard1,sizeof(testCard1));
		//	   }
		//   }
		//   //当前最大牌用户
		//   currentWinUser_ = INVALID_CHAIR;
		//   //比牌判断赢家
		//   for (int i = 0; i < GAME_PLAYER; ++i) {
		//	   if (!m_pTableFrame->IsExistUser(i)) {
		//		   continue;
		//	   }
		//	   //初始化
		//	   if (currentWinUser_ == INVALID_CHAIR) {
		//		   currentWinUser_ = i;
		//		   continue;
		//	   }
		//	   //比牌判断当前最大牌用户
		//	   if (ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[currentWinUser_])[0], special235_) > 0) {
		//		   currentWinUser_ = i;
		//	   }
		//   }
		//}
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
			//	1.让点杀用户都拿好牌
			//	2.随机一个机器人拿最大牌
			if (killBlackUsers) {
				//一定概率发好牌，好牌数量为blackList_.size()+1
				if (ratioDealGood <= ratioDealGoodKill_ && c < (blackUserNum + 1)) {
					goto restart;
				}
			}
			//好牌数量不够，重新洗牌发牌
    //         else if (ratioDealGood <= ratioDealGood_ && c < (m_pTableFrame->GetPlayerCount(true) - 1)) {
				// goto restart;
    //         }
		}
	}
	{
		//确定庄家用户
		if (bankerUser_ == INVALID_CHAIR) {
			bankerUser_ = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);
		}
		while (!m_pTableFrame->IsExistUser(bankerUser_)) {
			bankerUser_ = (bankerUser_ + 1) % GAME_PLAYER;
		}
		//确定操作用户
		currentUser_ = (bankerUser_ + 1) % GAME_PLAYER;
		while (!m_pTableFrame->IsExistUser(currentUser_) || currentUser_ == bankerUser_) {
			currentUser_ = (currentUser_ + 1) % GAME_PLAYER;
		}
		memset(turninfo_, 0, sizeof(turninfo_));
		//首发用户
		firstUser_ = currentUser_;
		//增加轮数
		++currentTurn_;
		assert(currentTurn_ == 0);
		//当轮开始用户
		turninfo_[currentTurn_].startuser = firstUser_;
	}
	//玩家出局(弃牌/比牌输)，若离开房间，信息将被清理，用户数据副本用来写记录
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		assert(ByChairId(i));
		GSUserBaseInfo& baseinfo = ByChairId(i)->GetUserBaseInfo();
		userinfo_t& userinfo = userinfos_[i];
		userinfo.chairId = i;
		userinfo.userId = baseinfo.userId;
		userinfo.headerId = baseinfo.headId;
		userinfo.nickName = baseinfo.nickName;
		userinfo.location = baseinfo.location;
		userinfo.account = baseinfo.account;
		userinfo.agentId = baseinfo.agentId;
        userinfo.lineCode = baseinfo.lineCode;
		//携带积分
		userinfo.userScore = ScoreByChairId(i);
		//是否机器人
		userinfo.isAndroidUser = m_pTableFrame->IsAndroidUser(i);
		isHaveUserPlay_[i] = 1;
	}
	assert(userinfos_.size() == m_pTableFrame->GetPlayerCount(true));
	//换牌策略分析
	
    {
		if (!bTestCard)
		{
			if (controlType_BlackRoom == 0)
			{
				AnalysePlayerCards(killBlackUsers, killOrLose);
			}
			else
			{
				AnalysePlayerCards_BlackRoom(killBlackUsers);
			}
		}
		ListPlayerCards();
	}
	//给机器人发送所有人的牌数据
	{
		//给机器人发数据
		zjh::CMD_S_AndroidCard reqdata;
		//牌局编号
		reqdata.set_roundid(strRoundID_);
		//系统当前库存
		reqdata.set_stockscore(StockScore);
		//系统库存下限
		reqdata.set_stocklowlimit(StockLowLimit);
		//系统库存上限
		reqdata.set_stockhighlimit(StockHighLimit);
		//当前最大牌用户
		reqdata.set_winuser(currentWinUser_);
		//机器人配置
		zjh::AndroidConfig* cfg = reqdata.mutable_cfg();
		//判断特殊牌型
		cfg->set_special235_(special235_);
		//开启随时看牌
		cfg->set_freelookover_(freeLookover_);
		//开启随时弃牌
		cfg->set_freegiveup_(freeGiveup_);
		//启用三套决策(0,1,2)
		cfg->set_usestrategy_(useStrategy_);
		//放大倍数
		cfg->set_scale(robot_.scale);
		//看牌参数，权值倍率，修正参数
		for (int i = 0; i < 3; ++i) {
			zjh::param_t* param = cfg->add_param();
			param->add_k_(robot_.param[i].K_[0]);
			param->add_k_(robot_.param[i].K_[1]);
			param->add_w1_(robot_.param[i].W1_[0]);
			param->add_w1_(robot_.param[i].W1_[1]);
			param->set_p1_(robot_.param[i].P1_);
			param->add_w2_(robot_.param[i].W2_[0]);
			param->add_w2_(robot_.param[i].W2_[1]);
			param->set_p2_(robot_.param[i].P2_);
		}
		//看牌概率
		for (int i = 0; i < 3; ++i) {
			cfg->add_ratiokp(robot_.ratioKP[i]);
		}
		for (int i = 0; i < 3; ++i) {
			//收分时三套决策(0,1,2)几率
			cfg->add_sp0(robot_.SP0[i]);
			//正常时三套决策(0,1,2)几率
			cfg->add_sp1(robot_.SP1[i]);
			//放分时三套决策(0,1,2)几率
			cfg->add_sp2(robot_.SP2[i]);
		}
		//序列化机器人配置 ///
		std::vector<std::pair<double, double>> robot;
		SerialConfig(robot);
		for (std::vector<std::pair<double, double>>::const_iterator it = robot.begin();
			it != robot.end(); ++it) {
			zjh::Values* values = cfg->add_robots();
			values->add_values(it->first);//[0]胜率
			values->add_values(it->second);//[1]权重
		}
		//所有用户(玩家及机器人)手牌
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			zjh::AndroidPlayer* player = reqdata.add_players();
			//桌椅ID
			player->set_chairid(i);
			//用户手牌
			zjh::HandCards* handcards = player->mutable_handcards();
			handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
			//手牌牌型
			handcards->set_ty(handCardsType_[i]);
		}
		//给机器人发送所有人的牌数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//跳过真人玩家
			if (!m_pTableFrame->IsAndroidUser(i)) {
				continue;
			}
			//发送机器人数据
			string content = reqdata.SerializeAsString();
			m_pTableFrame->SendTableData(i, zjh::SUB_S_ANDROID_CARD, (uint8_t *)content.data(), content.size());
		}
	}
	//广播游戏开始消息
	zjh::CMD_S_GameStart reqdata;
	reqdata.set_roundid(strRoundID_);
	//庄家用户
	reqdata.set_bankeruser(bankerUser_);
	//操作用户
	reqdata.set_currentuser(currentUser_);
	//操作剩余时间
	uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
	//操作总的时间
	opTotalTime_ = wTimeLeft;
	//操作开始时间
	opStartTime_ = (uint32_t)time(NULL);
	//操作结束时间
	opEndTime_ = opStartTime_ + wTimeLeft;
	//操作剩余时间
	reqdata.set_wtimeleft(wTimeLeft);
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//各玩家下注/桌面总下注
	{
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (m_pTableFrame->IsExistUser(i)) {
				shared_ptr<CServerUserItem> userItem = ByChairId(i); assert(userItem);
				//各玩家下注(初始底注)
				tableScore_[i] += CellScore;
				//桌面总下注
				tableAllScore_ += CellScore;
				//对局日志
				m_replay.addStep(nowsec, to_string(CellScore), currentTurn_ + 1, opStart, i, i);
			}
			//各玩家下注
			reqdata.add_tablescore(tableScore_[i]);
		}
	}
	//桌面总下注
	reqdata.set_tableallscore(tableAllScore_);
    //玩家当前积分/游戏状态
    for(int i = 0; i < GAME_PLAYER; ++i) {
		int64_t userScore = 0;
		if (m_pTableFrame->IsExistUser(i)) {
			//当前积分 = 携带积分 - 累计下注
			userScore = ScoreByChairId(i) - tableScore_[i];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
		}
		//玩家当前积分
		reqdata.add_userscore(userScore);
		//标识有效座椅
		reqdata.add_playstatus(m_pTableFrame->IsExistUser(i) ? 1 : 0);
    }
	//可操作上下文
	zjh::OptContext* ctx = reqdata.mutable_ctx();
	{
		//当前筹码值
		ctx->set_jettonvalue(jettonValue_);
		//当前跟注分(初始基础跟注分)
		ctx->set_followscore(followScore_);
		//加注筹码表
		for (std::vector<int64_t>::const_iterator it = JettionList.begin();
			it != JettionList.end(); ++it) {
			ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
		}
		//筹码表可加注筹码起始索引
		ctx->set_start(GetCurrentAddStart());
	}
	//机器人
	if (m_pTableFrame->IsAndroidUser(currentUser_)) {
		//LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[C] " << currentUser_ << " 游戏开始了...........................................";
	}
	else {
		//LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_ << " 游戏开始了...........................................";
	}
	//先发送机器人牌消息，再发送给游戏开始消息
	//得先让机器人知道哪些是真实玩家，真实玩家牌是否最大
	string content = reqdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, zjh::SUB_S_GAME_START, (uint8_t *)content.data(), content.size());
	//等待操作定时器
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::onTimerWaitingOver, this));
}

//剩余游戏人数 ///
int CGameProcess::leftPlayerCount(bool includeAndroid, uint32_t* currentUser) {
	int c = 0;
	for (int32_t i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[i] || isComparedLost_[i]) {
			continue;
		}
		//不包括机器人
		if (!includeAndroid && m_pTableFrame->IsAndroidUser(i)) {
			continue;
		}
		if (currentUser) {
			*currentUser = i;
		}
		++c;
	}
	return c;
}

//下一个操作用户 ///
uint32_t CGameProcess::GetNextUser(bool& newflag, bool& exceed) {
	newflag = false;
	exceed = false;
	bool addflag = false;
	assert(currentUser_ != INVALID_CHAIR);
	assert(firstUser_ != INVALID_CHAIR);
	//下标从1开始
	for (int i = 1; i < GAME_PLAYER; ++i) {
		//currentUser_ + 1 开始
		uint32_t nextUser = (currentUser_ + i) % GAME_PLAYER;
		//如果firstUser_已出局(弃牌/比牌输)，当 nextUser == firstUser_ 时，
		//若firstUser_未离开桌子，m_pTableFrame->IsExistUser(nextUser)为true，currentTurn_ 能正常更新
		//若firstUser_已离开桌子，m_pTableFrame->IsExistUser(nextUser)为false，currentTurn_ 将无法更新
		int64_t nextUserId = -1;
		if (!IsExistUser(nextUser, &nextUserId)/*m_pTableFrame->IsExistUser(nextUser)*/) {
			continue;
		}
		//回到起点
		if (nextUser == firstUser_) {
			if (!addflag) {
				if ((currentTurn_ + 1) < MAX_ROUND) {
					//LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 firstUser_ " << firstUser_
					//	<< "[" << nextUserId << "] IsExistUser = " << (m_pTableFrame->IsExistUser(firstUser_) ? "true" : "false")
					//	<< " currentUser_ = " << currentUser_;
					//本轮结束用户
					turninfo_[currentTurn_].enduser = currentUser_;
					//assert(turninfo_[currentTurn_].enduser != turninfo_[currentTurn_].startuser);
					//增加轮数[0, MAX_ROUND - 1]
					++currentTurn_;
				}
				else {
					//超过最大轮数
					assert((currentTurn_ + 1) == MAX_ROUND);
					exceed = true;
				}
				//新的一轮
				newflag = true;
				//标记自增
				addflag = true;
			}
			else {
				//第二次回到起点
				//assert(false);
				break;
			}
		}
		//跳过操作用户
		if (nextUser == currentUser_) {
			continue;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[nextUser] || isComparedLost_[nextUser]) {
			continue;
		}
		//新的一轮
		if (newflag) {
			//本轮开始用户
			assert(currentTurn_ < MAX_ROUND);
			turninfo_[currentTurn_].startuser = nextUser;
		}
		return nextUser;
	}
	return INVALID_CHAIR;
}

//返回有效用户 ///
uint32_t CGameProcess::GetNextUser(uint32_t startUser, bool ignoreCurrentUser) {
	//下标从0开始
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//startUser 开始
		uint32_t nextUser = (startUser + i) % GAME_PLAYER;
		if (!m_pTableFrame->IsExistUser(nextUser)) {
			continue;
		}
		//跳过操作用户
		if (ignoreCurrentUser && nextUser == currentUser_) {
			continue;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[nextUser] || isComparedLost_[nextUser]) {
			continue;
		}
		return nextUser;
	}
	return INVALID_CHAIR;
}

//等待操作定时器
void CGameProcess::onTimerWaitingOver()
{
	//玩家防超时弃牌
	//		钱足够则比牌
	//		钱不够则跟注
	//			钱足够则跟注
	//			钱不够则孤注一掷
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	//assert(currentUser_ != INVALID_CHAIR);
	//assert(!isGiveup_[currentUser_]);
	//assert(!isComparedLost_[currentUser_]);
	//assert(m_pTableFrame->IsExistUser(currentUser_));
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家 " << currentUser_ << " onTimerWaitingOver 用户已出局(弃牌/比牌输)";
		return;
	}
	//操作用户不存在
	if (!m_pTableFrame->IsExistUser(currentUser_)) {
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家 " << currentUser_ << " onTimerWaitingOver 操作用户不存在";
		return;
	}
	shared_ptr<CServerUserItem> userItem = ByChairId(currentUser_); assert(userItem);
	int opValue = OP_INVALID;
	//机器人
	if (m_pTableFrame->IsAndroidUser(currentUser_)) {
		LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人 " << currentUser_
			<< " 手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
			<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时";
		opValue = OP_COMPARE;
	}
	//真实玩家
	else {
		//防超时弃牌 /////////
		if (isLooked_[currentUser_] && noGiveUpTimeout_[currentUser_]) {
			//不可比牌阶段，跟注/弃牌 ///
			if (currentTurn_ == 0) {
				//未看牌
				//assert(!isLooked_[currentUser_]);
				//顺子及以上牌型
				if (handCardsType_[currentUser_] >= ZJH::Ty123) {
					//当前跟注分
					int64_t currentFollowScore = CurrentFollowScore(currentUser_);
					if ((currentFollowScore + tableScore_[currentUser_]) > userItem->GetUserScore()) {
						//钱不够则弃牌
						opValue = OP_GIVEUP;
						LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家 " << currentUser_
							<< " 手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
							<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱不够则弃牌";
					}
					else {
						//钱足够则跟注
						opValue = OP_FOLLOW;
						LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家 " << currentUser_
							<< " 手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
							<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱足够则跟注";
					}
				}
				//顺子以下牌型
				else {
					//弃牌
					opValue = OP_GIVEUP;
					LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家 " << currentUser_
						<< " 手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
						<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，弃牌";
				}
			}
			//可比牌阶段，比牌/弃牌 ///
			else {
				assert(currentTurn_ > 0);
				//玩家已看牌
				if (isLooked_[currentUser_]) {
					//顺子及以上牌型
					if (handCardsType_[currentUser_] >= ZJH::Ty123) {
						//比牌加注分
						int64_t currentCompareScore = CurrentCompareScore(currentUser_);
						//钱不够比牌 ///
						if ((currentCompareScore + tableScore_[currentUser_]) > userItem->GetUserScore()) {
							//当前跟注分
							int64_t currentFollowScore = CurrentFollowScore(currentUser_);
							//钱不够跟注 ///
							if ((currentFollowScore + tableScore_[currentUser_]) > userItem->GetUserScore()) {
#if 0
								//钱不够则孤注一掷
								LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
									<< " 已看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
									<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱不够则孤注一掷";
#endif
							}
							else {
#if 0
								//钱足够则跟注
								opValue = OP_FOLLOW;
								LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
									<< " 已看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
									<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱足够则跟注";
#endif
							}
#if 0
							//钱不够则弃牌
							opValue = OP_GIVEUP;
							LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
								<< " 已看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
								<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱不够则弃牌";
#elif 1
							//钱不够则孤注一掷
							LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
								<< " 已看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
								<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱不够则孤注一掷";
							opValue = OP_ALLIN;
#endif
						}
						else {
							//钱足够则比牌
							opValue = OP_COMPARE;
							LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
								<< " 已看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
								<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，钱足够则比牌";
						}
					}
					//顺子以下牌型
					else {
						//弃牌
						opValue = OP_GIVEUP;
						LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
							<< " 已看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
							<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，弃牌";
					}
				}
				//玩家未看牌
				else {
					//弃牌
					opValue = OP_GIVEUP;
					LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
						<< " 未看牌，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
						<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，弃牌";
				}
			}
		}
		//未勾选防超时弃牌 /////////
		else {
			//弃牌
			opValue = OP_GIVEUP;
			LOG(WARNING/*ERROR*/) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[C] " << currentUser_
				<< " 未防超时，手牌 " << ZJH::CGameLogic::StringCards(&(handCards_[currentUser_])[0], MAX_COUNT)
				<< " 牌型 " << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentUser_])) << " 超时，弃牌";
		}
	}
	assert(opValue != OP_INVALID);
	LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家 " << currentUser_ << " onTimerWaitingOver 操作 " << opValue;
	switch (opValue) {
	case OP_GIVEUP: {//弃牌
		OnUserGiveUp(currentUser_, true);
		break;
	}
	case OP_FOLLOW: {//跟注
		OnUserAddScore(currentUser_, -1);
		break;
	}
	case OP_COMPARE: {//比牌
		uint32_t nextUser = GetNextUser(RandomBetween(0, GAME_PLAYER - 1));
		if (nextUser == INVALID_CHAIR) {
			//assert(leftPlayerCount() == 1);
			//assert(false);
		}
		else {
			//assert(leftPlayerCount() >= 2);
			OnUserCompareCard(currentUser_, nextUser);
		}
		break;
	}
	case OP_ALLIN: { //孤注一掷
		uint32_t nextUser = GetNextUser(currentUser_ + 1);
		if (nextUser == INVALID_CHAIR) {
			//assert(leftPlayerCount() == 1);
			//assert(false);
		}
		else {
			//assert(leftPlayerCount() >= 2);
			OnUserAllIn(currentUser_, nextUser);
		}
		break;
	}
	}
}

//用户看牌 ///
bool CGameProcess::OnUserLookCard(int chairId)
{
	//assert(chairId != INVALID_CHAIR);
	//assert(m_pTableFrame->IsExistUser(chairId));
	//座椅无效
	if (chairId == INVALID_CHAIR) {
		return false;
	}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(chairId) ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[chairId] || isComparedLost_[chairId] ||
		isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return false;
	}
	//用户已看牌
	if (isLooked_[chairId]) {
		return false;
	}
	LOG(WARNING/*ERROR*/) << "----------------------------------------------------------------------------------------------------------";
	//旧版本：真实玩家随时可以看牌，机器人只有当操作用户是自己时才可以看牌
	//新版本：真实玩家和机器人随时可以看牌
	//安全断言
	//assert(!isLooked_[chairId]);
	//标记看牌
	isLooked_[chairId] = true;
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
	m_replay.addStep(nowsec, "", currentTurn_ + 1, opLkOrCall, chairId, chairId);
	//非操作用户看牌
	if (chairId != currentUser_) {
		if (m_pTableFrame->IsAndroidUser(chairId)) {
 			LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
 				<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
 				<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
 				<< "] <<看牌>>" << " 游戏人数[" << leftPlayerCount() << "]";
		}
		else {
 			LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
 				<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
 				<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
 				<< "] <<看牌>>" << " 游戏人数[" << leftPlayerCount() << "]";
		}
	}
	//操作用户看牌
	else {
		if (m_pTableFrame->IsAndroidUser(chairId)) {
 			LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
 				<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
 				<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
 				<< "] <<看牌>>" << " 游戏人数[" << leftPlayerCount() << "]";
		}
		else {
 			LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
 				<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
 				<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
 				<< "] <<看牌>>" << " 游戏人数[" << leftPlayerCount() << "]";
		}
	}
	//消息结构
	zjh::CMD_S_LookCard rspdata;
	//看牌用户
	rspdata.set_lookcarduser(chairId);
	//当前第几轮
	rspdata.set_currentturn(currentTurn_ + 1);
	//操作用户 //////
	rspdata.set_currentuser(currentUser_);
	//可操作上下文
	zjh::OptContext* ctx = rspdata.mutable_ctx();
	{
		//当前筹码值
		ctx->set_jettonvalue(jettonValue_);
		//当前跟注分
		int64_t currentFollowScore = CurrentFollowScore(currentUser_);
		ctx->set_followscore(currentFollowScore);
		//比牌加注分
		int64_t currentCompareScore = CurrentCompareScore(currentUser_);
		ctx->set_comparescore(currentCompareScore);
		//加注筹码表
		for (std::vector<int64_t>::const_iterator it = JettionList.begin();
			it != JettionList.end(); ++it) {
			ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
		}
		//筹码表可加注筹码起始索引
		ctx->set_start(GetCurrentAddStart());
	}
	//操作剩余时间 = 操作结束时间 - 当前时间
	uint32_t wTimeLeft = opEndTime_ - (uint32_t)time(NULL);
	//assert(wTimeLeft > 0);
	if (wTimeLeft <= 0) {
		wTimeLeft = 1;
	}
	//操作剩余时间(针对操作用户currentUser_)
	rspdata.set_wtimeleft(wTimeLeft);
	//广播看牌消息
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//看牌用户，回复消息，附带手牌/牌型
		if (i == chairId) {
			//玩家看牌
			if (!m_pTableFrame->IsAndroidUser(i)) {
				//玩家手牌
				rspdata.set_cards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				rspdata.set_ty(handCardsType_[i]);
			}
			//机器人看牌，回复消息，不带手牌/牌型
			else {
				if (rspdata.has_cards()) rspdata.clear_cards();
				if (rspdata.has_ty()) rspdata.clear_ty();
			}
		}
		//其他用户，广播通知，不带手牌/牌型
		else {
			if (rspdata.has_cards()) rspdata.clear_cards();
			if (rspdata.has_ty()) rspdata.clear_ty();
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				//机器人
				if (m_pTableFrame->IsAndroidUser(i)) {
					continue;
				}
			}
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, zjh::SUB_S_LOOK_CARD, (uint8_t *)content.data(), content.size());
	}
	LOG(WARNING/*ERROR*/) << "\n\n\n\n";
	return true;
}

//用户弃牌 ///
bool CGameProcess::OnUserGiveUp(int chairId, bool timeout)
{
	//assert(chairId != INVALID_CHAIR);
	//assert(chairId == currentUser_);
	//assert(m_pTableFrame->IsExistUser(chairId));
	int64_t userid = 0;
	shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId);
	if (userItem)
	{
		userid = userItem->GetUserId();
	}
	muduo::Timestamp startGiveUpTime = muduo::Timestamp::now(); 
	LOG(WARNING) << "===== strRoundID[" << strRoundID_ << "]" << " OnUserGiveUp 玩家请求弃牌操作: userid:" << userid << " chairId : " << chairId;

	//座椅无效
	if (chairId == INVALID_CHAIR) {
		return false;
	}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(chairId) ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[chairId] || isComparedLost_[chairId] ||
		isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return false;
	}
	//LOG(ERROR) << "----------------------------------------------------------------------------------------------------------";
	//旧版本：真实玩家随时可以弃牌，机器人只有当操作用户是自己时才可以弃牌
	//新版本：真实玩家和机器人随时可以弃牌(机器人弃牌之前先看牌)
	//安全断言
	assert(!isGiveup_[chairId]);
	//标记弃牌
	isGiveup_[chairId] = true;
	playTurn_[chairId].turn = currentTurn_ + 1;
	playTurn_[chairId].result = playturn_t::GIVEUP;
	//机器人弃牌
	if (m_pTableFrame->IsAndroidUser(chairId)) {
		//非超时弃牌
		assert(!timeout);
		//必须看过牌
		assert(isLooked_[chairId]);
	}
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
	m_replay.addStep(nowsec, "", currentTurn_ + 1, opQuitOrBCall, chairId, chairId);
	userOperate_[chairId][currentTurn_] = opQuitOrBCall;
	//更新庄家
	if (bankerUser_ == chairId) {
		bankerUser_ = GetNextUser(bankerUser_ + 1, false);
		assert(bankerUser_ != INVALID_CHAIR);
		assert(!isGiveup_[bankerUser_]);
		assert(!isComparedLost_[bankerUser_]);
	}
	//玩家弃牌
	if (!m_pTableFrame->IsAndroidUser(chairId)) {
		//当前积分 = 携带积分 - 累计下注
		int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
		//assert(userScore >= 0);
		if (userScore < 0) {
			userScore = 0;
		}
		//计算积分
		tagSpecialScoreInfo scoreInfo;
		scoreInfo.chairId = chairId;							//座椅ID
		scoreInfo.cardValue = StringCardValue();				//本局开牌
		scoreInfo.rWinScore = tableScore_[chairId];				//税前输赢
		scoreInfo.addScore = -tableScore_[chairId];				//本局输赢
		scoreInfo.betScore = tableScore_[chairId];				//总投注/有效投注/输赢绝对值(系统抽水前)
		scoreInfo.revenue = 0;									//本局税收
		scoreInfo.startTime = roundStartTime_;					//本局开始时间
		scoreInfo.cellScore.push_back(tableScore_[chairId]);	//玩家桌面分
		scoreInfo.bWriteScore = true;                           //只写积分
		scoreInfo.bWriteRecord = false;                         //不写记录
		SetScoreInfoBase(scoreInfo, chairId, NULL);				//基本信息
		//写入玩家积分
		m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_);
	}
	//胜率/权重
	std::pair<double, double>* values = GetWinRatioAndWeight(chairId);
	assert(values != NULL);
	//权重减少2
	values->second -= 2;
	if (values->second < 1000) {
		values->second = 1000;
	}
	//消息结构
	zjh::CMD_S_GiveUp rspdata;
	//弃牌用户
	rspdata.set_giveupuser(chairId);
	//当前第几轮
	rspdata.set_currentturn(currentTurn_ + 1);
	uint32_t currentUserT = currentUser_;
    //游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//非操作用户弃牌，不用切换操作用户 ///
		if (chairId != currentUser_) {
			if (m_pTableFrame->IsAndroidUser(chairId)) {
// 				LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>> 游戏人数 [" << leftPlayerCount() << "]";
			}
			else {
// 				LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>> 游戏人数 [" << leftPlayerCount() << "]";
			}
			//标记是否新的一轮/是否超出最大轮
			//bool newflag = false;
			//返回下一个操作用户
			//currentUser_ = GetNextUser(newflag, isExceed_);
			//下一步操作 ///
			zjh::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
			//切换操作用户
			nextStep->set_nextuser(currentUser_);
			//当前积分 = 携带积分 - 累计下注
			int64_t userScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
			nextStep->set_userscore(userScore);
			//当前第几轮
			nextStep->set_currentturn(currentTurn_ + 1);
			//可操作上下文
			zjh::OptContext* ctx = nextStep->mutable_ctx();
			{
				//当前筹码值
				ctx->set_jettonvalue(jettonValue_);
				//当前跟注分
				int64_t currentFollowScore = CurrentFollowScore(currentUser_);
				ctx->set_followscore(currentFollowScore);
				//比牌加注分
				int64_t currentCompareScore = CurrentCompareScore(currentUser_);
				ctx->set_comparescore(currentCompareScore);
				//加注筹码表
				for (std::vector<int64_t>::const_iterator it = JettionList.begin();
					it != JettionList.end(); ++it) {
					ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
				}
				//筹码表可加注筹码起始索引
				ctx->set_start(GetCurrentAddStart());
			}
			//操作剩余时间
			uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
			//操作总的时间
			opTotalTime_ = wTimeLeft;
			//操作开始时间
			opStartTime_ = (uint32_t)time(NULL);
			//操作结束时间
			opEndTime_ = opStartTime_ + wTimeLeft;
			//操作剩余时间
			nextStep->set_wtimeleft(wTimeLeft);
			//桌面总下注
			nextStep->set_tableallscore(tableAllScore_);
		}
		//操作用户弃牌，切换下一个操作用户 ///
		else {
			//取消定时器
			m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
			if (m_pTableFrame->IsAndroidUser(chairId)) {
// 				LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>>，游戏人数 [" << leftPlayerCount() << "]";
			}
			else {
// 				LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>>，游戏人数 [" << leftPlayerCount() << "]";
			}
			//标记是否新的一轮/是否超出最大轮
			bool newflag = false;
			//返回下一个操作用户
#if 0
			currentUser_ = GetNextUser(newflag, isExceed_);
			assert(currentUser_ != INVALID_CHAIR);
			assert(currentUser_ != currentUserT);
			assert(!isGiveup_[currentUser_]);
			assert(!isComparedLost_[currentUser_]);
#else
			uint32_t nextUser = GetNextUser(newflag, isExceed_);
			assert(nextUser != INVALID_CHAIR);
			assert(nextUser != currentUser_);
			assert(!isGiveup_[nextUser]);
			assert(!isComparedLost_[nextUser]);
			currentUser_ = nextUser;
#endif
			//未超出最大轮数限制，继续下一轮 ///
			if (!isExceed_) {
				//下一步操作 ///
				zjh::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
				//切换操作用户
				nextStep->set_nextuser(currentUser_);
				//当前积分 = 携带积分 - 累计下注
				int64_t userScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
				//assert(userScore >= 0);
				if (userScore < 0) {
					userScore = 0;
				}
				nextStep->set_userscore(userScore);
				//当前第几轮
				nextStep->set_currentturn(currentTurn_ + 1);
				//可操作上下文
				zjh::OptContext* ctx = nextStep->mutable_ctx();
				{
					//当前筹码值
					ctx->set_jettonvalue(jettonValue_);
					//当前跟注分
					int64_t currentFollowScore = CurrentFollowScore(currentUser_);
					ctx->set_followscore(currentFollowScore);
					//比牌加注分
					int64_t currentCompareScore = CurrentCompareScore(currentUser_);
					ctx->set_comparescore(currentCompareScore);
					//加注筹码表
					for (std::vector<int64_t>::const_iterator it = JettionList.begin();
						it != JettionList.end(); ++it) {
						ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
					}
					//筹码表可加注筹码起始索引
					ctx->set_start(GetCurrentAddStart());
				}
				//操作剩余时间
				uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
				//操作总的时间
				opTotalTime_ = wTimeLeft;
				//操作开始时间
				opStartTime_ = (uint32_t)time(NULL);
				//操作结束时间
				opEndTime_ = opStartTime_ + wTimeLeft;
				//操作剩余时间
				nextStep->set_wtimeleft(wTimeLeft);
				//桌面总下注
				nextStep->set_tableallscore(tableAllScore_);
			}
		}
    }
	//游戏人数仅剩1人，结束游戏 //////
    else {
		//assert(leftPlayerCount() == 1);
		uint32_t nextUser = GetNextUser(chairId + 1, false);
		assert(nextUser != INVALID_CHAIR);
		//胜率/权重
		std::pair<double, double>* values = GetWinRatioAndWeight(nextUser);
		assert(values != NULL);
		//权重增加2
		values->second += 2;
		//非操作用户弃牌
		if (chairId != currentUser_) {
			if (m_pTableFrame->IsAndroidUser(chairId)) {
// 				LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>> 游戏人数 [" << leftPlayerCount() << "] 结束游戏...";
			}
			else {
// 				LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>> 游戏人数 [" << leftPlayerCount() << "] 结束游戏...";
			}
		}
		//操作用户弃牌
		else {
			//取消定时器
			m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
			if (m_pTableFrame->IsAndroidUser(chairId)) {
// 				LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>>，游戏人数 [" << leftPlayerCount() << "] 结束游戏...";
			}
			else {
// 				LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 					<< "] <<弃牌>>，游戏人数 [" << leftPlayerCount() << "] 结束游戏...";
			}
		}
    }
	//广播弃牌消息
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//弃牌用户，回复消息，附带手牌/牌型
		if (i == chairId) {
			//玩家弃牌
			if (!m_pTableFrame->IsAndroidUser(i)) {
				//玩家手牌
				rspdata.set_cards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				rspdata.set_ty(handCardsType_[i]);
			}
			//机器人弃牌，回复消息，不带手牌/牌型
			else {
				if(rspdata.has_cards()) rspdata.clear_cards();
				if (rspdata.has_ty()) rspdata.clear_ty();
			}
		}
		//其他用户，广播通知，不带手牌/牌型
		else {
			if (rspdata.has_cards()) rspdata.clear_cards();
			if (rspdata.has_ty()) rspdata.clear_ty();
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				//机器人
				if (m_pTableFrame->IsAndroidUser(i)) {
					continue;
				}
			}
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, zjh::SUB_S_GIVE_UP, (uint8_t *)content.data(), content.size());
	}
	double cbtimeleave = muduo::timeDifference(muduo::Timestamp::now(), startGiveUpTime); //经过了多少时间
	LOG(WARNING) << "===== strRoundID[" << strRoundID_ << "]" << " OnUserGiveUp  userid:" << userid << " chairId : " << chairId << " 弃牌操作耗时 : " << cbtimeleave << " 秒;";

	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//非操作用户弃牌，不用切换操作用户 ///
		if (chairId != currentUserT) {
		}
		//操作用户弃牌，切换下一个操作用户 ///
		else {
			//未超出最大轮数限制，继续下一轮 ///
			if (!isExceed_) {
				assert(rspdata.has_nextstep());
				//操作剩余时间
				uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
				//开启等待定时器
				timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::onTimerWaitingOver, this));
			}
			//已超出最大轮数限制，强制比牌 ///
			else {
				assert(!rspdata.has_nextstep());
				OnUserCompareCardForce();
			}
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//assert(leftPlayerCount() == 1);
		//结束游戏流程
		OnEventGameConclude(INVALID_CHAIR, GER_NO_PLAYER);
	}
	//LOG(ERROR) << "\n\n\n\n";
    return true;
}

//用户跟注/加注 ///
bool CGameProcess::OnUserAddScore(int chairId, int32_t index)
{
	//assert(chairId != INVALID_CHAIR);
	//assert(m_pTableFrame->IsExistUser(chairId));
	//LOG(ERROR) << "----------------------------------------------------------------------------------------------------------";
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	//assert(chairId == currentUser_);
	//assert(!isGiveup_[chairId]);
	//assert(!isComparedLost_[chairId]);
	//座椅无效
	if (chairId == INVALID_CHAIR) {
		return false;
	}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(chairId)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[chairId] || isComparedLost_[chairId]) {
		return false;
	}
	//非操作用户返回
	if (chairId != currentUser_) {
		return false;
	}
	//消息结构
	zjh::CMD_S_AddScore rspdata;
	//操作用户
	rspdata.set_opuser(chairId);
	//当前第几轮
	rspdata.set_currentturn(currentTurn_ + 1);
	//跟注(OP_FOLLOW=3)，加注(OP_ADD=4)
	int opValue = (index > 0) ? OP_ADD : OP_FOLLOW;
	if (opValue == OP_FOLLOW) {
		//assert(index == -1);
		//当前跟注分
		int64_t currentFollowScore = CurrentFollowScore(chairId);
		//跟注操作钱不够则孤注一掷
		if ((currentFollowScore + tableScore_[chairId]) >= ScoreByChairId(chairId)) {
			uint32_t nextUser = GetNextUser(chairId + 1);
			if (nextUser == INVALID_CHAIR) {
				//assert(leftPlayerCount() == 1);
				//assert(false);
				return false;
			}
			//assert(leftPlayerCount() >= 2);
			return OnUserAllIn(chairId, nextUser);
		}
		//比牌加注分
		//int64_t currentCompareScore = CurrentCompareScore(chairId);
		//当前筹码值
		rspdata.set_jettonvalue(jettonValue_);
		//基础跟注分
		rspdata.set_followscore(followScore_);
		//当前跟注分
		rspdata.set_addscore(currentFollowScore);
		//筹码值索引
		rspdata.set_index(-1);
		//玩家桌面分
		tableScore_[chairId] += currentFollowScore;
		rspdata.set_tablescore(tableScore_[chairId]);
		//桌面总下注
		tableAllScore_ += currentFollowScore;
		rspdata.set_tableallscore(tableAllScore_);
		//玩家下注详情
		betinfo_[chairId].push_back({ jettonValue_, currentFollowScore , OP_FOLLOW });
		uint32_t nowsec =
			chrono::system_clock::to_time_t(chrono::system_clock::now()) -
			chrono::system_clock::to_time_t(roundStartTime_);
		//对局日志
		m_replay.addStep(nowsec, to_string(currentFollowScore), currentTurn_ + 1, opFollow, chairId, -1);
		userOperate_[chairId][currentTurn_] = opFollow;
	}
	else {
		//assert(index > 0);
		//assert(opValue == OP_ADD);
		if (index <= 0) {
			return false;
		}
		//筹码表可加注筹码起始索引
		int start = GetCurrentAddStart();
		bool bAIAddError = false;
		//assert(start > 0);
		//加注操作(start=3, index=1)
		if (start <= 0 || index < start) {
			if (m_pTableFrame->IsAndroidUser(chairId)) {
				LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
					<< "] <<加注失败,AI切换跟注操作>>，start=" << start << " index=" << index;
				
				// AI加注失败
				{
					//当前跟注分
					int64_t currentFollowScore = CurrentFollowScore(chairId);
					//跟注操作钱不够则孤注一掷
					if ((currentFollowScore + tableScore_[chairId]) >= ScoreByChairId(chairId)) {
						uint32_t nextUser = GetNextUser(chairId + 1);
						if (nextUser == INVALID_CHAIR) {
							//assert(leftPlayerCount() == 1);
							//assert(false);
							return false;
						}
						//assert(leftPlayerCount() >= 2);
						return OnUserAllIn(chairId, nextUser);
					}
					//比牌加注分
					//int64_t currentCompareScore = CurrentCompareScore(chairId);
					//当前筹码值
					rspdata.set_jettonvalue(jettonValue_);
					//基础跟注分
					rspdata.set_followscore(followScore_);
					//当前跟注分
					rspdata.set_addscore(currentFollowScore);
					//筹码值索引
					rspdata.set_index(-1);
					//玩家桌面分
					tableScore_[chairId] += currentFollowScore;
					rspdata.set_tablescore(tableScore_[chairId]);
					//桌面总下注
					tableAllScore_ += currentFollowScore;
					rspdata.set_tableallscore(tableAllScore_);
					//玩家下注详情
					betinfo_[chairId].push_back({ jettonValue_, currentFollowScore , OP_FOLLOW });
					uint32_t nowsec =
						chrono::system_clock::to_time_t(chrono::system_clock::now()) -
						chrono::system_clock::to_time_t(roundStartTime_);
					//对局日志
					m_replay.addStep(nowsec, to_string(currentFollowScore), currentTurn_ + 1, opFollow, chairId, -1);
					userOperate_[chairId][currentTurn_] = opFollow;
					bAIAddError = true;
				}
			}
			else {
				LOG(ERROR) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
					<< "] <<加注失败>>，start=" << start << " index=" << index;

				return false;
			}
		}
		if (!bAIAddError)
		{
			if (index >= (int)JettionList.size())
			{
				LOG(ERROR) << " --- 有他妈的逼的玩家搞事情***  筹码列表长度:"
					<< JettionList.size() << "  传过来的下标是index:"
					<< index << "  操作玩家 userid 是" << m_pTableFrame->GetTableUserItem(chairId)->GetUserId();
				return false;
			}
			if (index < start)
			{
				LOG(ERROR) << " --- 有他妈的逼的玩家搞事情***  加注的下标 < 当前能加注的开始下标 , 筹码列表长度:" << JettionList.size() 
					<< "  传过来的下标是index:" << index << "  start:" << start
					<< "  操作玩家 userid 是" << m_pTableFrame->GetTableUserItem(chairId)->GetUserId();
				return false;
			}
			//更新当前筹码值jettonValue_ //////
			//更新基础跟注分followScore_ //////
			selectAddIndex(start, index);
			//当前加注分
			int64_t currentAddScore = CurrentAddScore(chairId);
			//加注操作钱不够则孤注一掷
			if ((currentAddScore + tableScore_[chairId]) >= ScoreByChairId(chairId)) {
				uint32_t nextUser = GetNextUser(chairId + 1);
				if (nextUser == INVALID_CHAIR) {
					//assert(leftPlayerCount() == 1);
					//assert(false);
					return false;
				}
				//assert(leftPlayerCount() >= 2);
				return OnUserAllIn(chairId, nextUser);
			}
			//比牌加注分
			//int64_t currentCompareScore = CurrentCompareScore(chairId);
			//本轮是否有加注过
			turninfo_[currentTurn_].addflag = true;
			//当前筹码值
			rspdata.set_jettonvalue(jettonValue_);
			//基础跟注分
			rspdata.set_followscore(followScore_);
			//当前加注分
			rspdata.set_addscore(currentAddScore);
			//筹码值索引
			rspdata.set_index(index);
			//玩家桌面分
			tableScore_[chairId] += currentAddScore;
			rspdata.set_tablescore(tableScore_[chairId]);
			//桌面总下注
			tableAllScore_ += currentAddScore;
			rspdata.set_tableallscore(tableAllScore_);
			//玩家下注详情
			betinfo_[chairId].push_back({ jettonValue_, currentAddScore , OP_ADD });
			uint32_t nowsec =
				chrono::system_clock::to_time_t(chrono::system_clock::now()) -
				chrono::system_clock::to_time_t(roundStartTime_);
			//对局日志
			m_replay.addStep(nowsec, to_string(currentAddScore), currentTurn_ + 1, opAddBet, chairId, -1);
			userOperate_[chairId][currentTurn_] = opAddBet;
		}
	}
// 	if (!m_pTableFrame->IsAndroidUser(chairId)) {
// 		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " 桌面分 " << tableScore_[chairId];
// 	}
// 	else {
// 		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " 桌面分 " << tableScore_[chairId];
// 	}
	//跟注或加注
	rspdata.set_opvalue(opValue);
	//当前积分 = 携带积分 - 累计下注
	int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
	if (userScore < 0) {
		userScore = 0;
	}
	//assert(userScore >= 0);
	rspdata.set_userscore(userScore);
	
	if (m_pTableFrame->IsAndroidUser(chairId)) {
// 		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId
// 			<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 			<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 			<< ((opValue == OP_ADD) ? "] <<加注>>" : "] <<跟注>>") << " 游戏人数[" << leftPlayerCount() << "]";
	}
	else {
// 		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId
// 			<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 			<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 			<< ((opValue == OP_ADD) ? "] <<加注>>" : "] <<跟注>>") << " 游戏人数[" << leftPlayerCount() << "]";
	}

	//标记是否新的一轮/是否超出最大轮
	bool newflag = false;
	//返回下一个操作用户
#if 0
	currentUser_ = GetNextUser(newflag, isExceed_);
	assert(currentUser_ != INVALID_CHAIR);
	assert(!isGiveup_[currentUser_]);
	assert(!isComparedLost_[currentUser_]);
#else
	uint32_t nextUser = GetNextUser(newflag, isExceed_);
	assert(nextUser != INVALID_CHAIR);
	assert(nextUser != currentUser_);
	assert(!isGiveup_[nextUser]);
	assert(!isComparedLost_[nextUser]);
	currentUser_ = nextUser;
#endif
	//未超出最大轮数限制，继续下一轮 ///
	if (!isExceed_) {
		//下一步操作 ///
		zjh::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
		//切换操作用户
		nextStep->set_nextuser(currentUser_);
		//当前积分 = 携带积分 - 累计下注
		userScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
		//assert(userScore >= 0);
		if (userScore < 0) {
			userScore = 0;
		}
		nextStep->set_userscore(userScore);
		//当前第几轮
		nextStep->set_currentturn(currentTurn_ + 1);
		//可操作上下文
		zjh::OptContext* ctx = nextStep->mutable_ctx();
		{
			//当前筹码值
			ctx->set_jettonvalue(jettonValue_);
			//当前跟注分
			int64_t currentFollowScore = CurrentFollowScore(currentUser_);
			ctx->set_followscore(currentFollowScore);
			//比牌加注分
			int64_t currentCompareScore = CurrentCompareScore(currentUser_);
			ctx->set_comparescore(currentCompareScore);
			//加注筹码表
			for (std::vector<int64_t>::const_iterator it = JettionList.begin();
				it != JettionList.end(); ++it) {
				ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
			}
			//筹码表可加注筹码起始索引
			ctx->set_start(GetCurrentAddStart());
		}
		//桌面总下注
		nextStep->set_tableallscore(tableAllScore_);
		//操作剩余时间
		uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
		//操作总的时间
		opTotalTime_ = wTimeLeft;
		//操作开始时间
		opStartTime_ = (uint32_t)time(NULL);
		//操作结束时间
		opEndTime_ = opStartTime_ + wTimeLeft;
		//操作剩余时间
		nextStep->set_wtimeleft(wTimeLeft);
		//比牌动画时长
		//nextStep->set_actionleft(TIME_GAME_COMPARE_DELAY);
	}
	//广播跟注/加注消息
	string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, zjh::SUB_S_ADD_SCORE, (uint8_t *)content.data(), content.size());
	//未超出最大轮数限制，继续下一轮 ///
	if (!isExceed_) {
		assert(rspdata.has_nextstep());
		//操作剩余时间
		uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
		//开启等待定时器
		timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::onTimerWaitingOver, this));
	}
	//已超出最大轮数限制，强制比牌 ///
	else {
		assert(!rspdata.has_nextstep());
		OnUserCompareCardForce();
	}
	//LOG(ERROR) << "\n\n\n\n";
	return true;
}

//用户比牌 ///
bool CGameProcess::OnUserCompareCard(int chairId, int nextChairId)
{
	//assert(chairId != INVALID_CHAIR);
	//assert(nextChairId != INVALID_CHAIR);
	//assert(m_pTableFrame->IsExistUser(chairId));
	//assert(m_pTableFrame->IsExistUser(nextChairId));
	//LOG(ERROR) << "----------------------------------------------------------------------------------------------------------";
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	//assert(chairId == currentUser_);
	//assert(chairId != nextChairId);
	//assert(currentTurn_ > 0);
	//座椅无效
	if (chairId == INVALID_CHAIR ||
		nextChairId == INVALID_CHAIR) {
		return false;
	}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(chairId) ||
		!m_pTableFrame->IsExistUser(nextChairId)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[chairId] || isComparedLost_[chairId] ||
		isGiveup_[nextChairId] || isComparedLost_[nextChairId]) {
		return false;
	}
	//非操作用户返回
	if (chairId != currentUser_) {
		return false;
	}
	//比牌对方是自己
	if (chairId == nextChairId) {
		return false;
	}
	//assert(!isGiveup_[chairId]);
	//assert(!isComparedLost_[chairId]);
	
	//assert(!isGiveup_[nextChairId]);
	//assert(!isComparedLost_[nextChairId]);
	// 是否剩下最后一个机器人比牌了
	if (!blastAndroidChangeCard)
	{
		AnalyseLastAndroidUser(chairId,nextChairId);
	}
	
	char msg[256] = { 0 };
	//比牌加注分
	int64_t currentCompareScore = CurrentCompareScore(chairId);
	//比牌操作钱不够则孤注一掷
	if ((currentCompareScore + tableScore_[chairId]) >= ScoreByChairId(chairId)) {
		snprintf(msg, sizeof(msg), "需要积分(%lld) >= 当前积分(%lld)",
			currentCompareScore, ScoreByChairId(chairId) - tableScore_[chairId]);
 		LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 "
 			<< (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
 			<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
 			<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
 			<< "]  *** PK ***  "
 			<< (m_pTableFrame->IsAndroidUser(nextChairId) ? "机器人[" : "玩家[") << (nextChairId == currentUser_ ? "C" : "R") << "] " << nextChairId
 			<< " " << ZJH::CGameLogic::StringCards(&(handCards_[nextChairId])[0], MAX_COUNT) << " ["
 			<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[nextChairId])) << "] " << msg;
		return OnUserAllIn(chairId, nextChairId);
	}
	//玩家下注详情
	betinfo_[chairId].push_back({ jettonValue_, currentCompareScore , OP_COMPARE });
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//对局日志
	m_replay.addStep(nowsec, to_string(currentCompareScore), currentTurn_ + 1, opCmprOrLook, chairId, nextChairId);
	userOperate_[chairId][currentTurn_] = opCmprOrLook;
	//比牌得到赢家与输家用户
	uint32_t winUser = INVALID_CHAIR, lostUser = INVALID_CHAIR;
	if (ZJH::CGameLogic::CompareHandCards(&(handCards_[chairId])[0], &(handCards_[nextChairId])[0], special235_) > 0) {
		winUser = chairId; lostUser = nextChairId;
	}
	else {
		winUser = nextChairId; lostUser = chairId;
	}
	//比过牌的用户
	comparedUser_[chairId].push_back(nextChairId);
	comparedUser_[nextChairId].push_back(chairId);
	//比牌输了出局 ///
	isComparedLost_[lostUser] = true;
	playTurn_[lostUser].turn = currentTurn_ + 1;
	playTurn_[chairId].result = playturn_t::CMPLOST;
	//对局日志
	m_replay.addStep(nowsec, "", currentTurn_ + 1, opCmprFail, lostUser, -1);
	userOperate_[lostUser][currentTurn_] = opCmprFail;
	//更新庄家
	if (bankerUser_ == lostUser) {
		bankerUser_ = GetNextUser(bankerUser_ + 1, false);
		assert(bankerUser_ != INVALID_CHAIR);
		assert(!isGiveup_[bankerUser_]);
		assert(!isComparedLost_[bankerUser_]);
	}
	//玩家桌面分
	tableScore_[chairId] += currentCompareScore;
	//桌面总下注
	tableAllScore_ += currentCompareScore;
	//当前积分 = 携带积分 - 累计下注
	int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
	//assert(userScore >= 0);
	if (userScore < 0) {
		userScore = 0;
	}
	//玩家比牌输了
	if (!m_pTableFrame->IsAndroidUser(lostUser)) {
		//当前积分 = 携带积分 - 累计下注
		int64_t userScore = ScoreByChairId(lostUser) - tableScore_[lostUser];
		if (userScore < 0) {
			userScore = 0;
		}
		//assert(userScore >= 0);
		//计算积分
		tagSpecialScoreInfo scoreInfo;
		scoreInfo.chairId = lostUser;							//座椅ID
		scoreInfo.cardValue = StringCardValue();				//本局开牌
		scoreInfo.rWinScore = tableScore_[lostUser];			//税前输赢
		scoreInfo.addScore = -tableScore_[lostUser];			//本局输赢
		scoreInfo.betScore = tableScore_[lostUser];				//总投注/有效投注/输赢绝对值(系统抽水前)
		scoreInfo.revenue = 0;									//本局税收
		scoreInfo.startTime = roundStartTime_;					//本局开始时间
		scoreInfo.cellScore.push_back(tableScore_[lostUser]);	//玩家桌面分
		scoreInfo.bWriteScore = true;                           //只写积分
		scoreInfo.bWriteRecord = false;                         //不写记录
		SetScoreInfoBase(scoreInfo, lostUser, NULL);			//基本信息
		//写入玩家积分
		m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_);
	}
	//比牌发起者比牌输了
	if (isComparedLost_[chairId]) {
		//胜率/权重
		std::pair<double, double>* values = GetWinRatioAndWeight(chairId);
		assert(values != NULL);
		//被动比牌者是玩家
		if (!m_pTableFrame->IsAndroidUser(nextChairId)) {
			//权重减少1
			--values->second;
			if (values->second < 1000) {
				values->second = 1000;
			}
		}
		else {
			//机器人之间比牌权值不变
		}
		//胜率减少1
		--values->first;
		if (values->first < 10) {
			values->first = 10;
		}
		//被动比牌者比牌赢了
		{
			assert(!isComparedLost_[nextChairId]);
			//胜率/权重
			std::pair<double, double>* values = GetWinRatioAndWeight(nextChairId);
			assert(values != NULL);
			//比牌发起者是玩家
			if (!m_pTableFrame->IsAndroidUser(chairId)) {
				//权重增加1
				++values->second;
			}
			else {
				//机器人之间比牌权值不变
			}
			//胜率增加1
			++values->first;
		}
	}
	//比牌发起者比牌赢了
	else {
		//胜率/权重
		std::pair<double, double>* values = GetWinRatioAndWeight(chairId);
		assert(values != NULL);
		//被动比牌者是玩家
		if (!m_pTableFrame->IsAndroidUser(nextChairId)) {
			//权重增加1
			++values->second;
		}
		else {
			//机器人之间比牌权值不变
		}
		//胜率增加1
		++values->first;
		//被动比牌者比牌输了
		{
			assert(isComparedLost_[nextChairId]);
			//胜率/权重
			std::pair<double, double>* values = GetWinRatioAndWeight(nextChairId);
			assert(values != NULL);
			//比牌发起者是玩家
			if (!m_pTableFrame->IsAndroidUser(chairId)) {
				//权重减少1
				--values->second;
				if (values->second < 1000) {
					values->second = 1000;
				}
			}
			else {
				//机器人之间比牌权值不变
			}
			//胜率减少1
			--values->first;
			if (values->first < 10) {
				values->first = 10;
			}
		}
	}
// 	if (!m_pTableFrame->IsAndroidUser(chairId)) {
// 		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 玩家[" << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " 桌面分 " << tableScore_[chairId];
// 	}
// 	else {
// 		LOG(WARNING) << " --- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << ((chairId == currentUser_) ? "C" : "R") << "] " << chairId << " 桌面分 " << tableScore_[chairId];
// 	}
// 	LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 "
// 		<< (m_pTableFrame->IsAndroidUser(chairId) ? "机器人[" : "玩家[") << (chairId == currentUser_ ? "C" : "R") << "] " << chairId
// 		<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
// 		<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId]))
// 		<< "]  *** PK ***  "
// 		<< (m_pTableFrame->IsAndroidUser(nextChairId) ? "机器人[" : "玩家[") << (nextChairId == currentUser_ ? "C" : "R") << "] " << nextChairId
// 		<< " " << ZJH::CGameLogic::StringCards(&(handCards_[nextChairId])[0], MAX_COUNT) << " ["
// 		<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[nextChairId]))
// 		<< ((winUser == chairId) ? "] 赢了" : "] 输了") << " 游戏人数[" << leftPlayerCount() << "]";
	//消息结构
	zjh::CMD_S_CompareCard rspdata;
	//比牌发起者，操作用户
	rspdata.set_compareuser(chairId);
	//被动比牌者
	rspdata.set_passiveuser(nextChairId);
	//当前第几轮
	rspdata.set_currentturn(currentTurn_ + 1);
	//比牌发起者输赢
	rspdata.set_iscomparedlost(isComparedLost_[chairId] == true);
	//比牌加注分
	rspdata.set_comparescore(currentCompareScore);
	//玩家桌面分
	rspdata.set_tablescore(tableScore_[chairId]);
	//桌面总下注
	rspdata.set_tableallscore(tableAllScore_);
	//当前积分 = 携带积分 - 累计下注
	rspdata.set_userscore(userScore);
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//标记是否新的一轮/是否超出最大轮
		bool newflag = false;
		//返回下一个操作用户
#if 0
		currentUser_ = GetNextUser(newflag, isExceed_);
		assert(currentUser_ != INVALID_CHAIR);
		assert(!isGiveup_[currentUser_]);
		assert(!isComparedLost_[currentUser_]);
#else
		uint32_t nextUser = GetNextUser(newflag, isExceed_);
		assert(nextUser != INVALID_CHAIR);
		assert(nextUser != currentUser_);
		assert(!isGiveup_[nextUser]);
		assert(!isComparedLost_[nextUser]);
		currentUser_ = nextUser;
#endif
		//未超出最大轮数限制，继续下一轮 ///
		if (!isExceed_) {
			//下一步操作 ///
			zjh::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
			//切换操作用户
			nextStep->set_nextuser(currentUser_);
			//当前积分 = 携带积分 - 累计下注
			int64_t userScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
			nextStep->set_userscore(userScore);
			//当前第几轮
			nextStep->set_currentturn(currentTurn_ + 1);
			//可操作上下文
			zjh::OptContext* ctx = nextStep->mutable_ctx();
			{
				//当前筹码值
				ctx->set_jettonvalue(jettonValue_);
				//当前跟注分
				int64_t currentFollowScore = CurrentFollowScore(currentUser_);
				ctx->set_followscore(currentFollowScore);
				//比牌加注分
				int64_t currentCompareScore = CurrentCompareScore(currentUser_);
				ctx->set_comparescore(currentCompareScore);
				//加注筹码表
				for (std::vector<int64_t>::const_iterator it = JettionList.begin();
					it != JettionList.end(); ++it) {
					ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
				}
				//筹码表可加注筹码起始索引
				ctx->set_start(GetCurrentAddStart());
			}
			//操作剩余时间，比牌动画时长
			uint32_t wTimeLeft = TIME_GAME_ADD_SCORE/* + TIME_GAME_COMPARE_DELAY*/;
			//操作总的时间
			opTotalTime_ = wTimeLeft;
			//操作开始时间
			opStartTime_ = (uint32_t)time(NULL);
			//操作结束时间
			opEndTime_ = opStartTime_ + wTimeLeft;
			//操作剩余时间
			nextStep->set_wtimeleft(wTimeLeft);
			//桌面总下注
			nextStep->set_tableallscore(tableAllScore_);
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//assert(leftPlayerCount() == 1);
	}
	//广播比牌消息
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//比牌输家，回复消息，附带手牌/牌型
		if (lostUser == i) {
			//玩家比牌输了
			if (!m_pTableFrame->IsAndroidUser(i)) {
				//输家手牌
				zjh::HandCards* handcards = rspdata.mutable_lostcards();
				handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				handcards->set_ty(handCardsType_[i]);
			}
			//机器人比牌输了，回复消息，不带手牌/牌型
			else {
				if(rspdata.has_lostcards()) rspdata.clear_lostcards();
			}
		}
		//其他用户，广播通知，不带手牌/牌型
		else {
			if (rspdata.has_lostcards()) rspdata.clear_lostcards();
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				//机器人
				if (m_pTableFrame->IsAndroidUser(i)) {
					continue;
				}
			}
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, zjh::SUB_S_COMPARE_CARD, (uint8_t *)content.data(), content.size());
	}
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//未超出最大轮数限制，继续下一轮 ///
		if (!isExceed_) {
			assert(rspdata.has_nextstep());
			//操作剩余时间
			uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
			//开启等待定时器
			timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::onTimerWaitingOver, this));
		}
		//已超出最大轮数限制，强制比牌 ///
		else {
			assert(!rspdata.has_nextstep());
			OnUserCompareCardForce();
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//assert(leftPlayerCount() == 1);
		//结束游戏流程
		OnEventGameConclude(winUser, GER_COMPARECARD);
	}
	//LOG(ERROR) << "\n\n\n\n";
	return true;
}

//孤注一掷 ///
bool CGameProcess::OnUserAllIn(int chairId, int startChairId)
{
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	//assert(chairId != INVALID_CHAIR);
	//assert(startChairId != INVALID_CHAIR);
	//assert(m_pTableFrame->IsExistUser(chairId));
	//assert(m_pTableFrame->IsExistUser(startChairId));
	//assert(chairId == currentUser_);
	//assert(chairId != startChairId);
	//assert(!isGiveup_[chairId]);
	//assert(!isComparedLost_[chairId]);
	//assert(!isGiveup_[startChairId]);
	//assert(!isComparedLost_[startChairId]);
	//assert(ScoreByChairId(chairId) >= 0);
	//座椅无效
	if (chairId == INVALID_CHAIR ||
		startChairId == INVALID_CHAIR) {
		return false;
	}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(chairId) ||
		!m_pTableFrame->IsExistUser(startChairId)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[chairId] || isComparedLost_[chairId] ||
		isGiveup_[startChairId] || isComparedLost_[startChairId]) {
		return false;
	}
	//非操作用户返回
	if (chairId != currentUser_) {
		return false;
	}
	//比牌对方是自己
	if (chairId == startChairId) {
		return false;
	}
	//当前跟注分
	int64_t currentFollowScore = CurrentFollowScore(chairId);
	//跟注操作钱不够
	//assert((currentFollowScore + tableScore_[chairId]) > ScoreByChairId(chairId));
	//当前加注分
	int64_t currentAddScore = CurrentAddScore(chairId);
	//加注操作钱不够
	//assert((currentAddScore + tableScore_[chairId]) > ScoreByChairId(chairId));
	//比牌加注分
	int64_t currentCompareScore = CurrentCompareScore(chairId);
	//比牌操作钱不够
	//assert((currentCompareScore + tableScore_[chairId]) >= ScoreByChairId(chairId));
	if ((currentCompareScore + tableScore_[chairId]) < ScoreByChairId(chairId)) {
		return false;
	}
	//剩余积分
	int64_t currentLeftScore = ScoreByChairId(chairId) - tableScore_[chairId];
	//assert(currentLeftScore >= 0);
	if (currentLeftScore < 0) {
		return false;
	}
	//玩家下注详情
	betinfo_[chairId].push_back({ jettonValue_, currentLeftScore , OP_ALLIN });
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//孤注一掷被动比牌的玩家
	bool isComparedUser[GAME_PLAYER] = { 0 };
	//下标从0开始
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//startChairId 开始
		uint32_t nextChairId = (startChairId + i) % GAME_PLAYER;
		if (!m_pTableFrame->IsExistUser(nextChairId)) {
			continue;
		}
		if (nextChairId == chairId) {
			continue;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[nextChairId] || isComparedLost_[nextChairId]) {
			continue;
		}
		//比过牌的用户
		comparedUser_[chairId].push_back(nextChairId);
		comparedUser_[nextChairId].push_back(chairId);
		isComparedUser[nextChairId] = true;
		// 是否剩下最后一个机器人比牌了
		if (!blastAndroidChangeCard)
		{
			AnalyseLastAndroidUser(chairId,nextChairId);
		}
		//对局日志
		m_replay.addStep(nowsec, to_string(currentLeftScore), currentTurn_ + 1, opCmprOrLook, chairId, nextChairId);
		//比牌判断输赢
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[chairId])[0], &(handCards_[nextChairId])[0], special235_) < 0) {
			//输了自己出局 ///
			isComparedLost_[chairId] = true;
			playTurn_[chairId].turn = currentTurn_ + 1;
			playTurn_[chairId].result = playturn_t::ALLINLOST;
			break;
		}
	}
	//消息结构
	zjh::CMD_S_AllIn rspdata;
	rspdata.set_allinuser(currentUser_);
	//当前第几轮
	rspdata.set_currentturn(currentTurn_ + 1);
	//玩家桌面分
	tableScore_[chairId] += currentLeftScore;
	rspdata.set_tablescore(tableScore_[chairId]);
	//桌面总下注
	tableAllScore_ += currentLeftScore;
	rspdata.set_tableallscore(tableAllScore_);
	//当前积分 = 携带积分 - 累计下注
	int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
	//assert(userScore == 0);
	if (userScore < 0) {
		userScore = 0;
	}
	rspdata.set_userscore(userScore);
	//孤注一掷玩家输了出局 ///
	rspdata.set_iscomparedlost(isComparedLost_[chairId]);
	//孤注一掷玩家最终胜出，淘汰其他用户 ///
	if (!isComparedLost_[chairId]) {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//跳过孤注一掷玩家
			if (i == chairId) {
				continue;
			}
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				continue;
			}
			//其他用户输了出局 ///
			isComparedLost_[i] = true;
			playTurn_[i].turn = currentTurn_ + 1;
			playTurn_[i].result = playturn_t::CMPLOST;
			//本轮防以小博大处理
			if (GetLastBetScore(i) > 0) {
				//最后下注分
				double x = (double)GetLastBetScore(i);
				//应扣百分比
				double y = (double)currentLeftScore / (double)currentCompareScore;
				//应返还积分
				double z = (x * (1.0f - y) + 0.5f);
				tableScore_[i] -= (int64_t)z;
				tableAllScore_ -= (int64_t)z;
				LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 防以小博大，返还 "
					<< i << " " << UserIdBy(i) << " 积分为 " << (int64_t)z << " ...";
			}
			//玩家输了
			if (!m_pTableFrame->IsAndroidUser(i)) {
				//当前积分 = 携带积分 - 累计下注
				int64_t userScore = ScoreByChairId(i) - tableScore_[i];
				//assert(userScore >= 0);
				if (userScore < 0) {
					userScore = 0;
				}
				//计算积分
				tagSpecialScoreInfo scoreInfo;
				scoreInfo.chairId = i;							//座椅ID
				scoreInfo.cardValue = StringCardValue();		//本局开牌
				scoreInfo.rWinScore = tableScore_[i];			//税前输赢
				scoreInfo.addScore = -tableScore_[i];			//本局输赢
				scoreInfo.betScore = tableScore_[i];			//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = 0;							//本局税收
				scoreInfo.startTime = roundStartTime_;			//本局开始时间
				scoreInfo.cellScore.push_back(tableScore_[i]);	//玩家桌面分
				scoreInfo.bWriteScore = true;                   //只写积分
				scoreInfo.bWriteRecord = false;                 //不写记录
				SetScoreInfoBase(scoreInfo, i, NULL);			//基本信息
				//写入玩家积分
				m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_);
			}
			//孤注一掷的玩家比牌赢了(游戏结束)，其他用户比牌输了
			rspdata.add_comparedlostuser(i);
			rspdata.add_tablescoreother(tableScore_[i]);
			rspdata.set_tableallscore(tableAllScore_);
		}
		//更新庄家
		if (bankerUser_ != chairId) {
			assert(isComparedLost_[bankerUser_]);
			bankerUser_ = chairId;
			assert(!isComparedLost_[bankerUser_]);
		}
		//assert(leftPlayerCount() == 1);
	}
	//孤注一掷玩家输了出局，其它用户不受影响
	else {
		//更新庄家
		if (bankerUser_ == chairId) {
			bankerUser_ = GetNextUser(bankerUser_ + 1, false);
			assert(bankerUser_ != INVALID_CHAIR);
			assert(!isGiveup_[bankerUser_]);
			assert(!isComparedLost_[bankerUser_]);
		}
		//玩家输了
		if (!m_pTableFrame->IsAndroidUser(chairId)) {
			//当前积分 = 携带积分 - 累计下注
			int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
			//计算积分
			tagSpecialScoreInfo scoreInfo;
			scoreInfo.chairId = chairId;						//座椅ID
			scoreInfo.cardValue = StringCardValue();			//本局开牌
			scoreInfo.rWinScore = tableScore_[chairId];			//税前输赢
			scoreInfo.addScore = -tableScore_[chairId];			//本局输赢
			scoreInfo.betScore = tableScore_[chairId];			//总投注/有效投注/输赢绝对值(系统抽水前)
			scoreInfo.revenue = 0;								//本局税收
			scoreInfo.startTime = roundStartTime_;				//本局开始时间
			scoreInfo.cellScore.push_back(tableScore_[chairId]);//玩家桌面分
			scoreInfo.bWriteScore = true;						//只写积分
			scoreInfo.bWriteRecord = false;						//不写记录
			SetScoreInfoBase(scoreInfo, chairId, NULL);			//基本信息
			//写入玩家积分
			m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_);
		}
	}
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//标记是否新的一轮/是否超出最大轮
		bool newflag = false;
		//返回下一个操作用户
#if 0
		currentUser_ = GetNextUser(newflag, isExceed_);
		assert(currentUser_ != INVALID_CHAIR);
		assert(!isGiveup_[currentUser_]);
		assert(!isComparedLost_[currentUser_]);
#else
		uint32_t nextUser = GetNextUser(newflag, isExceed_);
		assert(nextUser != INVALID_CHAIR);
		assert(nextUser != currentUser_);
		assert(!isGiveup_[nextUser]);
		assert(!isComparedLost_[nextUser]);
		currentUser_ = nextUser;
#endif
		//未超出最大轮数限制，继续下一轮 ///
		if (!isExceed_) {
			//下一步操作 ///
			zjh::CMD_S_Next_StepInfo* nextStep = rspdata.mutable_nextstep();
			//切换操作用户
			nextStep->set_nextuser(currentUser_);
			//当前积分 = 携带积分 - 累计下注
			int64_t userScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
			nextStep->set_userscore(userScore);
			//当前第几轮
			nextStep->set_currentturn(currentTurn_ + 1);
			//可操作上下文
			zjh::OptContext* ctx = nextStep->mutable_ctx();
			{
				//当前筹码值
				ctx->set_jettonvalue(jettonValue_);
				//当前跟注分
				int64_t currentFollowScore = CurrentFollowScore(currentUser_);
				ctx->set_followscore(currentFollowScore);
				//比牌加注分
				int64_t currentCompareScore = CurrentCompareScore(currentUser_);
				ctx->set_comparescore(currentCompareScore);
				//加注筹码表
				for (std::vector<int64_t>::const_iterator it = JettionList.begin();
					it != JettionList.end(); ++it) {
					ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
				}
				//筹码表可加注筹码起始索引
				ctx->set_start(GetCurrentAddStart());
			}
			//操作剩余时间
			uint32_t wTimeLeft = TIME_GAME_ADD_SCORE;
			//操作总的时间
			opTotalTime_ = wTimeLeft;
			//操作开始时间
			opStartTime_ = (uint32_t)time(NULL);
			//操作结束时间
			opEndTime_ = opStartTime_ + wTimeLeft;
			//操作剩余时间
			nextStep->set_wtimeleft(wTimeLeft);
			//桌面总下注
			nextStep->set_tableallscore(tableAllScore_);
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//assert(leftPlayerCount() == 1);
	}
	//广播比牌消息
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//比牌输家，回复消息，附带手牌/牌型
		if ((i == chairId || isComparedUser[i]) && isComparedLost_[i]) {
			//玩家比牌输了
			if (!m_pTableFrame->IsAndroidUser(i)) {
				//输家手牌
				zjh::HandCards* handcards = rspdata.mutable_handcards();
				handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				handcards->set_ty(handCardsType_[i]);
			}
			//机器人比牌输了，回复消息，不带手牌/牌型
			else {
				if (rspdata.has_handcards()) rspdata.clear_handcards();
			}
		}
		//其他用户，广播通知，不带手牌/牌型
		else {
			if (rspdata.has_handcards()) rspdata.clear_handcards();
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				//机器人
				if (m_pTableFrame->IsAndroidUser(i)) {
					continue;
				}
			}
		}
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(i, zjh::SUB_S_ALL_IN, (uint8_t *)content.data(), content.size());
	}
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//孤注一掷玩家输了出局 ///
		assert(isComparedLost_[rspdata.allinuser()]);
		//未超出最大轮数限制，继续下一轮 ///
		if (!isExceed_) {
			assert(rspdata.has_nextstep());
			//操作剩余时间
			uint32_t wTimeLeft = rspdata.nextstep().wtimeleft();
			//开启等待定时器
			timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(wTimeLeft + 1, boost::bind(&CGameProcess::onTimerWaitingOver, this));
		}
		//已超出最大轮数限制，强制比牌 ///
		else {
			assert(!rspdata.has_nextstep());
			OnUserCompareCardForce();
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//assert(leftPlayerCount() == 1);
		//结束游戏流程
		OnEventGameConclude(INVALID_CHAIR, GER_ALLIN);
	}

	return true;
}

//超出最大轮数限制，强制比牌 ///
bool CGameProcess::OnUserCompareCardForce() {
	//assert(currentUser_ != INVALID_CHAIR);
	//assert(!isGiveup_[currentUser_]);
	//assert(!isComparedLost_[currentUser_]);
	//assert(m_pTableFrame->IsExistUser(currentUser_));
	//assert(isExceed_);
	//if (currentUser_ == INVALID_CHAIR ||
	//	!m_pTableFrame->IsExistUser(currentUser_)) {
	//	return false;
	//}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return false;
	}
	uint32_t nowsec =
		chrono::system_clock::to_time_t(chrono::system_clock::now()) -
		chrono::system_clock::to_time_t(roundStartTime_);
	//当前比牌输赢用户
	uint32_t winUser = GetNextUser(currentUser_, false), lostUser = INVALID_CHAIR;
	//从庄的下家开始，依次与下家强制发起比牌，此时比牌无需进行下注
	uint32_t startUser = GetNextUser(bankerUser_ + 1);
	//下标从0开始
	for (int i = 0; i < GAME_PLAYER; ++i) {
		//startUser 开始
		uint32_t nextUser = (startUser + i) % GAME_PLAYER;
		if (!m_pTableFrame->IsExistUser(nextUser)) {
			continue;
		}
		if (nextUser == winUser) {
			continue;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[nextUser] || isComparedLost_[nextUser]) {
			continue;
		}
		winUser != INVALID_CHAIR;
		assert(!isGiveup_[winUser]);
		assert(!isComparedLost_[winUser]);
		assert(m_pTableFrame->IsExistUser(winUser));
		//比牌发起者/被动比牌者
		uint32_t chairId = winUser, nextChairId = nextUser;
		comparedUser_[chairId].push_back(nextChairId);
		comparedUser_[nextChairId].push_back(chairId);
		// 是否剩下最后一个机器人比牌了
		if (!blastAndroidChangeCard)
		{
			AnalyseLastAndroidUser(chairId,nextChairId);
		}
		//对局日志
		m_replay.addStep(nowsec, to_string(0), currentTurn_ + 1, opCmprOrLook, chairId, nextChairId);
		//与最大牌用户比牌
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[nextUser])[0], &(handCards_[winUser])[0], special235_) > 0) {
			lostUser = winUser;
			winUser = nextUser;
		}
		else {
			lostUser = nextUser;
		}
		//比牌输了出局 ///
		isComparedLost_[lostUser] = true;
		playTurn_[lostUser].turn = currentTurn_ + 1;
		playTurn_[chairId].result = playturn_t::CMPLOST;
		//玩家比牌输了
		if (!m_pTableFrame->IsAndroidUser(lostUser)) {
			//当前积分 = 携带积分 - 累计下注
			int64_t userScore = ScoreByChairId(lostUser) - tableScore_[lostUser];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
			//计算积分
			tagSpecialScoreInfo scoreInfo;
			scoreInfo.chairId = lostUser;							//座椅ID
			scoreInfo.cardValue = StringCardValue();				//本局开牌
			scoreInfo.rWinScore = tableScore_[lostUser];			//税前输赢
			scoreInfo.addScore = -tableScore_[lostUser];			//本局输赢
			scoreInfo.betScore = tableScore_[lostUser];				//总投注/有效投注/输赢绝对值(系统抽水前)
			scoreInfo.revenue = 0;									//本局税收
			scoreInfo.startTime = roundStartTime_;					//本局开始时间
			scoreInfo.cellScore.push_back(tableScore_[lostUser]);	//玩家桌面分
			scoreInfo.bWriteScore = true;                           //只写积分
			scoreInfo.bWriteRecord = false;                         //不写记录
			SetScoreInfoBase(scoreInfo, lostUser, NULL);			//基本信息
			//写入玩家积分
			m_pTableFrame->WriteSpecialUserScore(&scoreInfo, 1, strRoundID_);
		}
		//比牌发起者比牌输了
		if (isComparedLost_[chairId]) {
			//胜率/权重
			std::pair<double, double>* values = GetWinRatioAndWeight(chairId);
			assert(values != NULL);
			//被动比牌者是玩家
			if (!m_pTableFrame->IsAndroidUser(nextChairId)) {
				//权重减少1
				--values->second;
				if (values->second < 1000) {
					values->second = 1000;
				}
			}
			else {
				//机器人之间比牌权值不变
			}
			//胜率减少1
			--values->first;
			if (values->first < 10) {
				values->first = 10;
			}
			//被动比牌者比牌赢了
			{
				assert(!isComparedLost_[nextChairId]);
				//胜率/权重
				std::pair<double, double>* values = GetWinRatioAndWeight(nextChairId);
				assert(values != NULL);
				//比牌发起者是玩家
				if (!m_pTableFrame->IsAndroidUser(chairId)) {
					//权重增加1
					++values->second;
				}
				else {
					//机器人之间比牌权值不变
				}
				//胜率增加1
				++values->first;
			}
		}
		//比牌发起者比牌赢了
		else {
			//胜率/权重
			std::pair<double, double>* values = GetWinRatioAndWeight(chairId);
			assert(values != NULL);
			//被动比牌者是玩家
			if (!m_pTableFrame->IsAndroidUser(nextChairId)) {
				//权重增加1
				++values->second;
			}
			else {
				//机器人之间比牌权值不变
			}
			//胜率增加1
			++values->first;
			//被动比牌者比牌输了
			{
				assert(isComparedLost_[nextChairId]);
				//胜率/权重
				std::pair<double, double>* values = GetWinRatioAndWeight(nextChairId);
				assert(values != NULL);
				//比牌发起者是玩家
				if (!m_pTableFrame->IsAndroidUser(chairId)) {
					//权重减少1
					--values->second;
					if (values->second < 1000) {
						values->second = 1000;
					}
				}
				else {
					//机器人之间比牌权值不变
				}
				//胜率减少1
				--values->first;
				if (values->first < 10) {
					values->first = 10;
				}
			}
		}
		//消息结构
		zjh::CMD_S_CompareCard rspdata;
		//比牌发起者，操作用户
		rspdata.set_compareuser(chairId);
		//被动比牌者
		rspdata.set_passiveuser(nextChairId);
		//当前第几轮
		rspdata.set_currentturn(currentTurn_ + 1);
		//比牌发起者输赢
		rspdata.set_iscomparedlost(isComparedLost_[chairId] == true);
		//玩家桌面分
		rspdata.set_tablescore(tableScore_[chairId]);
		//桌面总下注
		rspdata.set_tableallscore(tableAllScore_);
		//当前积分 = 携带积分 - 累计下注
		int64_t userScore = ScoreByChairId(chairId) - tableScore_[chairId];
		//assert(userScore >= 0);
		if (userScore < 0) {
			userScore = 0;
		}
		rspdata.set_userscore(userScore);
		//广播比牌消息
		for (int j = 0; j < GAME_PLAYER; ++j) {
			if (!m_pTableFrame->IsExistUser(j)) {
				continue;
			}
			//比牌输家，回复消息，附带手牌/牌型
			if (lostUser == j) {
				//玩家比牌输了
				if (!m_pTableFrame->IsAndroidUser(j)) {
					//输家手牌
					zjh::HandCards* handcards = rspdata.mutable_lostcards();
					handcards->set_cards(&(handCards_[j])[0], MAX_COUNT);
					//手牌牌型
					handcards->set_ty(handCardsType_[j]);
				}
				//机器人比牌输了，回复消息，不带手牌/牌型
				else {
					if (rspdata.has_lostcards()) rspdata.clear_lostcards();
				}
			}
			//其他用户，广播通知，不带手牌/牌型
			else {
				if (rspdata.has_lostcards()) rspdata.clear_lostcards();
				//用户已出局(弃牌/比牌输)
				if (isGiveup_[j] || isComparedLost_[j]) {
					//机器人
					if (m_pTableFrame->IsAndroidUser(j)) {
						continue;
					}
				}
			}
			std::string content = rspdata.SerializeAsString();
			m_pTableFrame->SendTableData(j, zjh::SUB_S_COMPARE_CARD, (uint8_t *)content.data(), content.size());
		}
	}
	if (leftPlayerCount() != 1) {
		LOG(WARNING) << " @@@@@@@@@@@ leftPlayerCount = " << leftPlayerCount();
	}
	//assert(leftPlayerCount() == 1);
	//更新庄家
	assert(bankerUser_ != INVALID_CHAIR);
	if (isComparedLost_[bankerUser_]) {
		bankerUser_ = GetNextUser(bankerUser_ + 1, false);
		assert(bankerUser_ != INVALID_CHAIR);
		assert(!isGiveup_[bankerUser_]);
		assert(!isComparedLost_[bankerUser_]);
	}
	//结束游戏流程
	OnEventGameConclude(INVALID_CHAIR, GER_COMPARECARD);
	return true;
}

//防超时弃牌 ///
bool CGameProcess::OnUserTimeoutGiveUpSetting(int chairId, bool giveup)
{
	//assert(chairId != INVALID_CHAIR);
	//assert(m_pTableFrame->IsExistUser(chairId));
	//assert(!isGiveup_[chairId]);
	//assert(!isComparedLost_[chairId]);
	//座椅无效
	if (chairId == INVALID_CHAIR) {
		return false;
	}
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR) {
		return false;
	}
	//用户无效
	if (!m_pTableFrame->IsExistUser(chairId) ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[chairId] || isComparedLost_[chairId] ||
		isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return false;
	}
	//开启/关闭
	noGiveUpTimeout_[chairId] = giveup;
	//消息结构
	zjh::CMD_S_GIVEUP_TIMEOUT_OP rspdata;
	rspdata.set_isgiveup(giveup);
	std::string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(chairId, zjh::SUB_S_GIVEUP_TIMEOUT_OP, (uint8_t *)content.data(), content.size());
	return true;
}

//游戏消息
bool CGameProcess::OnGameMessage(uint32_t chairId, uint8_t dwSubCmdID, const uint8_t* pDataBuffer, uint32_t dwDataSize)
{
    switch (dwSubCmdID)
    {
	case zjh::SUB_C_LOOK_CARD: {	//用户看牌 ///
		//座椅无效
		if (chairId == INVALID_CHAIR) {
			return false;
		}
		//操作用户无效
		if (currentUser_ == INVALID_CHAIR) {
			return false;
		}
		//指针无效
		if (!m_pTableFrame) {
			return false;
		}
		//用户无效
		if (!m_pTableFrame->IsExistUser(chairId) ||
			!m_pTableFrame->IsExistUser(currentUser_)) {
			return false;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[chairId] || isComparedLost_[chairId] ||
			isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
			return false;
		}
		//用户已看牌
		if (isLooked_[chairId]) {
			return false;
		}
		return OnUserLookCard(chairId);
	}
    case zjh::SUB_C_GIVE_UP: {		//用户弃牌 ///
		//座椅无效
		if (chairId == INVALID_CHAIR) {
			return false;
		}
		//操作用户无效
		if (currentUser_ == INVALID_CHAIR) {
			return false;
		}
		//指针无效
		if (!m_pTableFrame) {
			return false;
		}
		//用户无效
		if (!m_pTableFrame->IsExistUser(chairId) ||
			!m_pTableFrame->IsExistUser(currentUser_)) {
			return false;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[chairId] || isComparedLost_[chairId] ||
			isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
			return false;
		}
        return OnUserGiveUp(chairId);
    }
	case zjh::SUB_C_ADD_SCORE: {	//用户跟注/加注 ///
		zjh::CMD_C_AddScore reqdata;
		if (!reqdata.ParseFromArray(pDataBuffer, dwDataSize)) {
			return false;
		}
		//座椅无效
		if (chairId == INVALID_CHAIR) {
			return false;
		}
		//操作用户无效
		if (currentUser_ == INVALID_CHAIR) {
			return false;
		}
		//指针无效
		if (!m_pTableFrame) {
			return false;
		}
		//用户无效
		if (!m_pTableFrame->IsExistUser(chairId)) {
			return false;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[chairId] || isComparedLost_[chairId]) {
			return false;
		}
		//非操作用户返回
		if (chairId != currentUser_) {
			return false;
		}
		
		//已经是最大押注
		if (reqdata.index() == (int)JettionList.size() - 1)
		{
			if (followScore_ == JettionList[JettionList.size() - 1])
			{
				LOG(ERROR) << "有人搞鬼，已经是最大押注 " << " reqdata.index()=" << reqdata.index() << " followScore_:" << followScore_ << " Userid:" << m_pTableFrame->GetTableUserItem(chairId)->GetUserId();
				return false;
			}
		}
        else if (reqdata.index() == 0 || reqdata.index() >= (int)JettionList.size())
		{
			LOG(ERROR) << " 有人搞鬼，超出加注下标  筹码列表长度:"  << JettionList.size() << "  传过来的下标是index:" << " reqdata.index()=" << reqdata.index() << " Userid:" << m_pTableFrame->GetTableUserItem(chairId)->GetUserId();
			return false;
		}
		return OnUserAddScore(chairId, reqdata.index());
	}
    case zjh::SUB_C_COMPARE_CARD: {	//用户比牌 ///
		zjh::CMD_C_CompareCard reqdata;
        if(!reqdata.ParseFromArray(pDataBuffer,dwDataSize)){
            return false;
        }
		//座椅无效
		if (chairId == INVALID_CHAIR) {
			return false;
		}
		//操作用户无效
		if (currentUser_ == INVALID_CHAIR) {
			return false;
		}
		//指针无效
		if (!m_pTableFrame) {
			return false;
		}
		//用户无效
		if (!m_pTableFrame->IsExistUser(chairId)) {
			return false;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[chairId] || isComparedLost_[chairId]) {
			return false;
		}
		//非操作用户返回
		if (chairId != currentUser_) {
			return false;
		}
		//比牌对方不存在
		if (!m_pTableFrame->IsExistUser(reqdata.peeruser())) {
			//仅有2个游戏玩家
			if (leftPlayerCount() == 2) {
				uint32_t nextUser = GetNextUser(chairId + 1);
				if (nextUser == INVALID_CHAIR) {
					return false;
				}
				return OnUserCompareCard(chairId, nextUser);
			}
			return false;
		}
		//assert(!isGiveup_[chairId]);
		//assert(!isComparedLost_[chairId]);
		//assert(!isComparedLost_[reqdata.peeruser()]);
		//assert(chairId != reqdata.peeruser());
		//assert(leftPlayerCount() >= 2);
		//对方已出局(弃牌/比牌输)
		if (isGiveup_[reqdata.peeruser()] || isComparedLost_[reqdata.peeruser()]) {
			//玩家请求比牌，直接返回失败
			if (!m_pTableFrame->IsAndroidUser(chairId)) {
				return false;
			}
			//随机一个可比牌用户
			uint32_t nextUser = GetNextUser(reqdata.peeruser() + 1);
			if (nextUser == INVALID_CHAIR) {
				//assert(leftPlayerCount() == 1);
				//assert(false);
				return false;
			}
			//assert(leftPlayerCount() >= 2);
			//机器人请求比牌
			return OnUserCompareCard(chairId, nextUser);
		}
		return OnUserCompareCard(chairId, reqdata.peeruser());
    }
    case zjh::SUB_C_ALL_IN: { //孤注一掷 ///
		//座椅无效
		if (chairId == INVALID_CHAIR) {
			return false;
		}
		//操作用户无效
		if (currentUser_ == INVALID_CHAIR) {
			return false;
		}
		//指针无效
		if (!m_pTableFrame) {
			return false;
		}
		//用户无效
		if (!m_pTableFrame->IsExistUser(chairId)) {
			return false;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[chairId] || isComparedLost_[chairId]) {
			return false;
		}
		//非操作用户返回
		if (chairId != currentUser_) {
			return false;
		}
		uint32_t nextUser = GetNextUser(chairId + 1);
		if (nextUser == INVALID_CHAIR) {
			//assert(leftPlayerCount() == 1);
			//assert(false);
			return false;
		}
		//assert(leftPlayerCount() >= 2);
		return OnUserAllIn(chairId, nextUser);
    }
    case zjh::SUB_C_GIVEUP_TIMEOUT_OP: { //防超时弃牌 ///
		zjh::CMD_C_GIVEUP_TIMEOUT_OP TimeoutSetting;
        if(!TimeoutSetting.ParseFromArray(pDataBuffer, dwDataSize)) {
            return false;
        }
		//座椅无效
		if (chairId == INVALID_CHAIR) {
			return false;
		}
		//操作用户无效
		if (currentUser_ == INVALID_CHAIR) {
			return false;
		}
		//指针无效
		if (!m_pTableFrame) {
			return false;
		}
		//用户无效
		if (!m_pTableFrame->IsExistUser(chairId) ||
			!m_pTableFrame->IsExistUser(currentUser_)) {
			return false;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[chairId] || isComparedLost_[chairId] ||
			isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
			return false;
		}
		return OnUserTimeoutGiveUpSetting(chairId, TimeoutSetting.isgiveup());
    }
    }
    return false;
}

//换牌策略分析
void CGameProcess::AnalysePlayerCards(bool killBlackUsers,int killOrLose)
{
    LOG(ERROR) << " 进入换牌判断，包括个人库存，黑名单，库存控制";
	//当前最大牌用户
    currentWinUser_ = INVALID_CHAIR;
	//比牌判断赢家
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//初始化
		if (currentWinUser_ == INVALID_CHAIR) {
			currentWinUser_ = i;
			continue;
		}
		//比牌判断当前最大牌用户
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[currentWinUser_])[0], special235_) > 0) {
			currentWinUser_ = i;
		}
	}
	//安全断言
	assert(currentWinUser_ != INVALID_CHAIR);
	//没有机器人
	if (m_pTableFrame->GetPlayerCount(false) == m_pTableFrame->GetPlayerCount(true)) {
		return;
	}
	//机器人最大牌
	uint32_t winAndroidUser = INVALID_CHAIR;
	//比牌判断机器人最大牌
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (!m_pTableFrame->IsAndroidUser(i)) {
			continue;
		}
		//初始化
		if (winAndroidUser == INVALID_CHAIR) {
			winAndroidUser = i;
			continue;
		}
		//比牌判断机器人最大牌
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[winAndroidUser])[0], special235_) > 0) {
			winAndroidUser = i;
		}
	}
	assert(winAndroidUser != INVALID_CHAIR);
    //个人库存控制换牌假如是上限以上，则玩家赢分超过个人控制线以上，就用概率控制玩家输。交换最大牌玩家和最大牌机器人手牌交换

    //个人库存控制杀
    if(killOrLose==1)
    {
        LOG(ERROR) << " 个人库存要求系统杀";
        if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
            //交换手牌
            uint8_t t[MAX_COUNT];
            memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
            memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
            memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
            //交换牌型
            std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);
            //当前最大牌用户
            currentWinUser_ = winAndroidUser;
            LOG(ERROR) << " --- *** [" << strRoundID_ << "] ################## 杀个人库存换牌 ......";
        }
    }
    else if(killOrLose==-1)//个人库存控制放
    {
        LOG(ERROR) << " 个人库存要求系统放";
        if (m_pTableFrame->IsAndroidUser(currentWinUser_)) {

            int realPlayer = INVALID_CHAIR;
            for(int i=0;i<GAME_PLAYER;i++)
            {
                if(m_pTableFrame->IsExistUser(i)&&!m_pTableFrame->IsAndroidUser(i))
                {
                    realPlayer = i;
                    break;
                }
            }
            //交换手牌
            uint8_t t[MAX_COUNT];
            memcpy(t, &(handCards_[realPlayer])[0], sizeof(uint8_t)*MAX_COUNT);
            memcpy(&(handCards_[realPlayer])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
            memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
            //交换牌型
            std::swap(handCardsType_[realPlayer], handCardsType_[currentWinUser_]);
            //当前最大牌用户
            currentWinUser_ = realPlayer;
            LOG(ERROR) << " --- *** [" << strRoundID_ << "] ################## 放个人库存换牌 ......";
        }
    }
    else
    {
        LOG(ERROR) << " 个人库存要求不干涉";
    }
    //点杀黑名单用户
    if (killBlackUsers) {
		//最大牌用户是真实玩家，则与最大牌机器人换牌
		if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
			//交换手牌
			uint8_t t[MAX_COUNT];
			memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
			//交换牌型
			std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);
			//当前最大牌用户
			currentWinUser_ = winAndroidUser;
			LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 点杀换牌 ......";
		}
        if(StockScore > StockHighLimit||StockScore<StockLowLimit)
        {

        }
        else
        {
            return;
        }
	}
	//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
	if (!m_pTableFrame->IsAndroidUser(currentWinUser_) &&
		//好牌：带A的散牌或对子以上
		(handCardsType_[winAndroidUser] >= ZJH::Ty20 ||
		(handCardsType_[winAndroidUser] == ZJH::Tysp && ZJH::CGameLogic::HasCardValue(&(handCards_[winAndroidUser])[0]))
			) &&
		Exchange == CalcExchangeOrNot2(sysSwapCardsRatio_)) {
		//交换手牌
		uint8_t t[MAX_COUNT];
		memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
		memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
		memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
		//交换牌型
		std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);
		//当前最大牌用户
		currentWinUser_ = winAndroidUser;
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 系统换牌 ......";
		return;
	}
	//系统输分则通杀
	if (StockScore < StockLowLimit) {
		//如果玩家赢
		if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
			//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
			if (Exchange == CalcExchangeOrNot2(sysLostSwapCardsRatio_)) {
				//玩家赢则换牌，让机器人赢
				uint32_t newWinUser = INVALID_CHAIR;
				do {
					//机器人赢
					newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);
					if (m_pTableFrame->IsExistUser(newWinUser) && m_pTableFrame->IsAndroidUser(newWinUser))
						break;
				} while (true);
				assert(newWinUser != INVALID_CHAIR);
				//交换手牌
				uint8_t t[MAX_COUNT];
				memcpy(t, &(handCards_[newWinUser])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[newWinUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
				//交换牌型
				std::swap(handCardsType_[newWinUser], handCardsType_[currentWinUser_]);
				//当前最大牌用户
				currentWinUser_ = newWinUser;
				LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 通杀换牌 ......";
			}
			else {
				//玩家赢则不处理
			}
		}
		else {
			//机器人赢则不处理
		}
	}
	//系统赢分则放水
	else if (StockScore > StockHighLimit) {
		//如果机器人赢
		if (m_pTableFrame->IsAndroidUser(currentWinUser_)) {
			//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌

            int res =m_random.betweenInt(0,100).randInt_mt(true);
            if (res<=ratioSwapWin_) {
				//机器人赢则换牌，让玩家赢
				uint32_t newWinUser = INVALID_CHAIR;
				do {
					//玩家赢
					newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);;
					if (m_pTableFrame->IsExistUser(newWinUser) && !m_pTableFrame->IsAndroidUser(newWinUser)) {
						break;
					}
				} while (true);
				//交换手牌
				uint8_t t[MAX_COUNT];
				memcpy(t, &(handCards_[newWinUser])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[newWinUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
				//交换牌型
				std::swap(handCardsType_[newWinUser], handCardsType_[currentWinUser_]);
				//当前最大牌用户
				currentWinUser_ = newWinUser;
				LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 放水换牌 ......";
			}
			else {
				//机器人赢则不处理
			}
		}
		else {
			//玩家赢则不处理
		}
	}
}

//换牌策略分析(小黑屋)
void CGameProcess::AnalysePlayerCards_BlackRoom(bool killBlackUsers)
{
	LOG(ERROR) << " 进入换牌策略分析，包括小黑屋库存，黑名单，库存控制";
	//当前最大牌用户
	currentWinUser_ = getWinUser();

	// if (!m_pTableFrame->IsAndroidUser(currentWinUser_))
	{
		// 如果是真人,跟第6手牌比较,获取最大的手牌
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[currentWinUser_])[0], &(madeMaxCards_)[0], special235_) > 0)
		{
			//交换手牌
			uint8_t t[MAX_COUNT];
			memcpy(t, madeMaxCards_, sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(madeMaxCards_)[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
			//交换牌型
			std::swap(madeMaxCardsType_, handCardsType_[currentWinUser_]);
			//当前最大牌用户
			currentWinUser_ = getWinUser();;
			LOG(ERROR) << "===== [" << strRoundID_ << "]  更换最大手牌 ......";
		}
	}
	if (!m_pTableFrame->IsAndroidUser(currentWinUser_))
	{
		LOG(ERROR) << "===== 最大手牌 [" << strRoundID_ << "] 真人 .."
			<< " StockScore:" << StockScore << " StockLowLimit:" << StockLowLimit << " StockHighLimit:" << StockHighLimit;
	}
	else
	{
		LOG(ERROR) << "===== 最大手牌 [" << strRoundID_ << "] 机器人 .."
			<< " StockScore:" << StockScore << " StockLowLimit:" << StockLowLimit << " StockHighLimit:" << StockHighLimit;
	}
	//安全断言
	assert(currentWinUser_ != INVALID_CHAIR);
	//没有机器人
	if (m_pTableFrame->GetPlayerCount(false) == m_pTableFrame->GetPlayerCount(true)) {
		return;
	}
	//机器人最大牌
	uint32_t winAndroidUser = INVALID_CHAIR;
	//比牌判断机器人最大牌
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (!m_pTableFrame->IsAndroidUser(i)) {
			continue;
		}
		//初始化
		if (winAndroidUser == INVALID_CHAIR) {
			winAndroidUser = i;
			continue;
		}
		//比牌判断机器人最大牌
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[winAndroidUser])[0], special235_) > 0) {
			winAndroidUser = i;
		}
	}
	assert(winAndroidUser != INVALID_CHAIR);

    //难度干涉值
    //m_difficulty
    m_difficulty =m_pTableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        //最大牌用户是真实玩家，则与最大牌机器人换牌
        if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
            //交换手牌
            uint8_t t[MAX_COUNT];
            memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
            memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
            memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
            //交换牌型
            std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);
            //当前最大牌用户
            currentWinUser_ = winAndroidUser;
            LOG(ERROR) << "===== [" << strRoundID_ << "] 点杀换牌 ......";
        }
        return;
    }
	//点杀黑名单用户
	if (killBlackUsers) {
		//最大牌用户是真实玩家，则与最大牌机器人换牌
		if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
			//交换手牌
			uint8_t t[MAX_COUNT];
			memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
			//交换牌型
			std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);
			//当前最大牌用户
			currentWinUser_ = winAndroidUser;
			LOG(ERROR) << "===== [" << strRoundID_ << "] 点杀换牌 ......";
		}
		return;
	}
	//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
	if (!m_pTableFrame->IsAndroidUser(currentWinUser_) &&
		//好牌：带A的散牌或对子以上
		(handCardsType_[winAndroidUser] >= ZJH::Ty20 ||
		(handCardsType_[winAndroidUser] == ZJH::Tysp && ZJH::CGameLogic::HasCardValue(&(handCards_[winAndroidUser])[0]))
			) &&
		Exchange == CalcExchangeOrNot2(sysSwapCardsRatio_)) {
		//交换手牌
		int32_t num = RandomBetween(1, 100);
		if (num < ratioSwap_) //换第二好的牌
		{
			uint8_t t[MAX_COUNT];
			memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
			//交换牌型
			std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);
			//当前最大牌用户
			currentWinUser_ = winAndroidUser;
		}
		else //随机换
		{
			uint32_t newWinUser = INVALID_CHAIR;
			do {
				//机器人赢
				newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);
				if (m_pTableFrame->IsExistUser(newWinUser) && m_pTableFrame->IsAndroidUser(newWinUser))
					break;
			} while (true);
			assert(newWinUser != INVALID_CHAIR);
			//交换手牌
			uint8_t t[MAX_COUNT];
			memcpy(t, &(handCards_[newWinUser])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[newWinUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
			memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
			//交换牌型
			std::swap(handCardsType_[newWinUser], handCardsType_[currentWinUser_]);
			//当前最大牌用户
			currentWinUser_ = newWinUser;
		}
		LOG(ERROR) << "===== [" << strRoundID_ << "] 系统换牌 ......";
		return;
	}
	//个人输赢控制
	int32_t num = RandomBetween(1, 100);
	if (!m_pTableFrame->IsAndroidUser(currentWinUser_) && num < userControlinfo_[currentWinUser_].curWeight)
	{
		//换第二好的牌
		uint8_t t[MAX_COUNT];
		memcpy(t, &(handCards_[winAndroidUser])[0], sizeof(uint8_t)*MAX_COUNT);
		memcpy(&(handCards_[winAndroidUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
		memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
		//交换牌型
		std::swap(handCardsType_[winAndroidUser], handCardsType_[currentWinUser_]);

		LOG(ERROR) << "===== [" << strRoundID_ << "] 个人输赢库存控制换牌 curWin:" << userControlinfo_[currentWinUser_].curWin
			<< " Randnum:" << num << " curWeight:" << userControlinfo_[currentWinUser_].curWeight;
		//当前最大牌用户
		currentWinUser_ = winAndroidUser;
		return;
	}
	//系统输分则通杀
	if (StockScore < StockLowLimit) {
		//如果玩家赢
		if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
			//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
			if (Exchange == CalcExchangeOrNot2(sysLostSwapCardsRatio_)) {
				//玩家赢则换牌，让机器人赢
				uint32_t newWinUser = INVALID_CHAIR;
				do {
					//机器人赢
					newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);
					if (m_pTableFrame->IsExistUser(newWinUser) && m_pTableFrame->IsAndroidUser(newWinUser))
						break;
				} while (true);
				assert(newWinUser != INVALID_CHAIR);
				//交换手牌
				uint8_t t[MAX_COUNT];
				memcpy(t, &(handCards_[newWinUser])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[newWinUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
				//交换牌型
				std::swap(handCardsType_[newWinUser], handCardsType_[currentWinUser_]);
				//当前最大牌用户
				currentWinUser_ = newWinUser;
				LOG(ERROR) << "===== [" << strRoundID_ << "] 通杀换牌 ......";
			}
			else {
				//玩家赢则不处理
			}
		}
		else {
			//机器人赢则不处理
		}
	}
	//系统赢分则放水
	else if (StockScore > StockHighLimit) {
		//如果机器人赢
		if (m_pTableFrame->IsAndroidUser(currentWinUser_)) {
			//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
			if (Exchange == CalcExchangeOrNot2(sysWinSwapCardsRatio_)) {
				//机器人赢则换牌，让玩家赢
				uint32_t newWinUser = INVALID_CHAIR;
				do {
					//玩家赢
					newWinUser = GlobalFunc::RandomInt64(0, GAME_PLAYER - 1);;
					if (m_pTableFrame->IsExistUser(newWinUser) && !m_pTableFrame->IsAndroidUser(newWinUser)) {
						break;
					}
				} while (true);
				//交换手牌
				uint8_t t[MAX_COUNT];
				memcpy(t, &(handCards_[newWinUser])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[newWinUser])[0], &(handCards_[currentWinUser_])[0], sizeof(uint8_t)*MAX_COUNT);
				memcpy(&(handCards_[currentWinUser_])[0], t, sizeof(uint8_t)*MAX_COUNT);
				//交换牌型
				std::swap(handCardsType_[newWinUser], handCardsType_[currentWinUser_]);
				//当前最大牌用户
				currentWinUser_ = newWinUser;
				LOG(ERROR) << "===== [" << strRoundID_ << "] 放水换牌 ......";
			}
			else {
				//机器人赢则不处理
			}
		}
		else {
			//玩家赢则不处理
		}
	}
}

//输出玩家手牌/对局日志初始化
void CGameProcess::ListPlayerCards() {
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
	assert(currentWinUser_ != INVALID_CHAIR);
	if (m_pTableFrame->IsAndroidUser(currentWinUser_)) {
		LOG(WARNING) << "\n--- *** [" << strRoundID_ << "] 机器人 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 手牌 ["
			<< ZJH::CGameLogic::StringCards(&(handCards_[currentWinUser_])[0], MAX_COUNT)
			<< "] 牌型 [" << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentWinUser_])) << "] 最大牌\n\n\n";
	}
	else {
		LOG(WARNING) << "\n--- *** [" << strRoundID_ << "] 玩家 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 手牌 ["
			<< ZJH::CGameLogic::StringCards(&(handCards_[currentWinUser_])[0], MAX_COUNT)
			<< "] 牌型 [" << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentWinUser_])) << "] 最大牌\n\n\n";
	}
}

//拼接各玩家手牌cardValue
std::string CGameProcess::StringCardValue() {
	char ch[10] = { 0 };
	std::string str = "";
	for (int i = 0; i < GAME_PLAYER; i++) {
		//if (m_pTableFrame->IsExistUser(i)) {
		if (isHaveUserPlay_[i] != 0) {
			string cards;
			for (int j = 0; j < MAX_COUNT; ++j) {
				sprintf(ch, "%02X", handCards_[i][j]);
				cards += ch;
			}
			str += cards;
		}
		else {
			str += "000000";
		}
	}
	if (INVALID_CHAIR != currentWinUser_) {
		sprintf(ch, "%02d", currentWinUser_);
		str += ch;
	}
	return str;
}

//计算机器人税收
int64_t CGameProcess::CalculateAndroidRevenue(int64_t score)
{
	//return m_pTableFrame->CalculateRevenue(score);
    return score * m_pTableFrame->GetGameRoomInfo()->systemReduceRatio / 1000;
}

//游戏结束
bool CGameProcess::OnEventGameConclude(uint32_t dchairId, uint8_t GETag)
{
	//清理所有定时器
	ClearAllTimer();
	//游戏结束时间
	roundEndTime_ = chrono::system_clock::now();
	//消息结构
	zjh::CMD_S_GameEnd rspdata;
	//计算玩家积分
	std::vector<tagSpecialScoreInfo> scoreInfos;
	//记录当局游戏详情binary
	zjh::CMD_S_GameRecordDetail details;
	details.set_gameid(ThisGameId);
	//确定赢家
	currentUser_ = INVALID_CHAIR;
	int c = leftPlayerCount(true, &currentUser_);
	assert(c == 1);
	assert(currentUser_ != INVALID_CHAIR);
	assert(!isGiveup_[currentUser_]);
	assert(!isComparedLost_[currentUser_]);
	if (currentWinUser_ != currentUser_) {
		currentWinUser_ = currentUser_;
	}
	//切换庄家
	assert(bankerUser_ == currentWinUser_);
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
	//桌面输赢人数
	int winners = 0, losers = 0;
	for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		//桌椅ID
		uint32_t i = it->second.chairId;
		//携带积分 >= 下注积分
		userScore[i] = it->second.userScore;
		assert(userScore[i] >= tableScore_[i]);
		//输赢积分
		if (isGiveup_[i] || isComparedLost_[i]) {
			//输的积分
			userWinScore[i] = -tableScore_[i];
		}
		else {
			//赢的积分 = 桌面总下注 - 玩家桌面分
			userWinScore[i] = tableAllScore_ - tableScore_[i];
		}
		if (userWinScore[i] < 0) {
			//桌面输的人数
			++losers;
		}
		else {
			//桌面赢的人数
			++winners;
			assert(i == currentWinUser_);
			playTurn_[i].turn = currentTurn_ + 1;
		}
	}
	assert(winners == 1);
	isGemeEnd_ = true;
	//计算输赢积分
    for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		//桌椅ID
		uint32_t i = it->second.chairId;
		//盈利玩家
		if (userWinScore[i] > 0) {
			assert(i == currentWinUser_);
			//系统抽水，真实玩家和机器人都按照5%抽水计算，前端显示和玩家扣分一致
			revenue[i] = m_pTableFrame->CalculateRevenue(userWinScore[i]);
		}
		//输赢积分，扣除系统抽水
		userWinScorePure[i] = userWinScore[i] - revenue[i];
		//刷新控制值
        if (!it->second.isAndroidUser)
		{
            LOG(ERROR) << "====游戏结束,玩家输赢 " << i << " userid:" << it->second.userId << " userWinScorePure:" << userWinScorePure[i];
			userControlinfo_[i].curWin += userWinScorePure[i] >= 0 ? userWinScorePure[i] : userWinScorePure[i] * (100 - ratiokcreduction_)/100;
            userControlinfo_[i].conScore += userWinScorePure[i] >= 0 ? userWinScorePure[i] : userWinScorePure[i] * (100 - ratiokcreduction_)/100;
			if (userControlinfo_[i].conScore<=0) //当前控制结束,刷新控制等级
			{
				userControlinfo_[i].conScore = 0;
			}
			CalculateControlLv(i, userControlinfo_[i].curWin);
			currentHaveWinScore_[i] += userWinScorePure[i];				
		}
		//剩余积分
		userLeftScore[i] = userScore[i] + userWinScorePure[i];
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
			LOG(INFO) << " --- *** [" << strRoundID_ << "] 跑马灯信息 userid = " << it->second.userId
				<< " " << msg << " score = " << userWinScorePure[i];
		}
		//若是机器人AI
		if (it->second.isAndroidUser) {
#ifdef _STORAGESCORE_SEPARATE_STAT_
			//系统输赢积分，扣除系统抽水
			systemWinScore += userWinScorePure[i];
#else
			//系统输赢积分
			systemWinScore += userWinScore[i];
#endif
		}
		//赢家
		if (i == currentWinUser_) {
			//真实玩家
			if(!it->second.isAndroidUser) {
				//计算积分
				tagSpecialScoreInfo scoreInfo;
				scoreInfo.chairId = i;							//座椅ID
				scoreInfo.cardValue = StringCardValue();		//本局开牌
				//scoreInfo.rWinScore = llabs(userWinScore[i]);	//税前输赢
				scoreInfo.rWinScore = tableScore_[i];			//有效投注
				scoreInfo.addScore = userWinScorePure[i];		//本局输赢
				scoreInfo.betScore = tableScore_[i];			//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = revenue[i];					//本局税收
				scoreInfo.startTime = roundStartTime_;			//本局开始时间
				scoreInfo.cellScore.push_back(tableScore_[i]);	//玩家桌面分
				scoreInfo.bWriteScore = true;                   //写赢家分
				scoreInfo.bWriteRecord = true;                  //也写记录
				SetScoreInfoBase(scoreInfo, i, &it->second);	//基本信息
				scoreInfos.push_back(scoreInfo);				
			}
			//对局日志
			m_replay.addResult(i, i, tableScore_[i], userWinScorePure[i],
				to_string(handCardsType_[i]) + ":" + GlobalFunc::converToHex(&(handCards_[i])[0], MAX_COUNT), false);
		}
		//输家
		else {
			//真实玩家
			if (!it->second.isAndroidUser) {
				//计算积分
				tagSpecialScoreInfo scoreInfo;
				scoreInfo.chairId = i;							//座椅ID
				scoreInfo.cardValue = StringCardValue();		//本局开牌
				scoreInfo.rWinScore = tableScore_[i];			//税前输赢
				scoreInfo.addScore = -tableScore_[i];			//本局输赢
				scoreInfo.betScore = tableScore_[i];			//总投注/有效投注/输赢绝对值(系统抽水前)
				scoreInfo.revenue = 0;							//本局税收
				scoreInfo.startTime = roundStartTime_;			//本局开始时间
				scoreInfo.cellScore.push_back(tableScore_[i]);	//玩家桌面分
				scoreInfo.bWriteScore = false;                  //不用写分，出局前写了
				scoreInfo.bWriteRecord = true;                  //仅写记录
				SetScoreInfoBase(scoreInfo, i, &it->second);	//基本信息
				scoreInfos.push_back(scoreInfo);
			}
			//对局日志
			m_replay.addResult(i, i, tableScore_[i], -tableScore_[i],
				to_string(handCardsType_[i]) + ":" + GlobalFunc::converToHex(&(handCards_[i])[0], MAX_COUNT), false);
		}
		zjh::PlayerRecordDetail* detail = details.add_detail();
		//账号/昵称
		detail->set_account(std::to_string(it->second.userId));
		//座椅ID
		detail->set_chairid(i);
		//是否庄家
		detail->set_isbanker(bankerUser_ == i ? true : false);
		//手牌/牌型
		zjh::HandCards* pcards = detail->mutable_handcards();
		pcards->set_cards(&(handCards_[i])[0], 3);
		pcards->set_ty(handCardsType_[i]);
		//携带积分
		detail->set_userscore(userScore[i]);
		//房间底注
		detail->set_cellscore(CellScore);
		//经历轮数
		detail->set_roundturn(playTurn_[i].turn);
		//输赢结果
		detail->set_roundresult(playTurn_[i].result);
		//玩家下注
		detail->set_tablescore(tableScore_[i]);
		//输赢积分
		detail->set_winlostscore(userWinScorePure[i]);
	}
#ifndef _STORAGESCORE_SEPARATE_STAT_

#endif
    //系统输赢积分，扣除系统抽水1%
    if (systemWinScore > 0) {
        //systemWinScore -= m_pTableFrame->CalculateRevenue(systemWinScore);
        systemWinScore -= CalculateAndroidRevenue(systemWinScore);		
		LOG(WARNING) << " === strRoundID[" << strRoundID_ << "] 库存变化:systemWinScore=" << systemWinScore ;
    }	
	openLog(" === strRoundID[%s] 库存变化:systemWinScore=%d;", strRoundID_.c_str(), systemWinScore);
	//更新系统库存
	m_pTableFrame->UpdateStorageScore(systemWinScore);
	//系统当前库存变化
	StockScore += systemWinScore;
	//更新机器人配置
	//UpdateConfig();
	//写入玩家积分
	m_pTableFrame->WriteSpecialUserScore(&scoreInfos[0], scoreInfos.size(), strRoundID_);
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		m_replay.detailsData = details.SerializeAsString();
	}
	//保存对局结果
	m_pTableFrame->SaveReplay(m_replay);

	//赢家用户
	rspdata.set_winuser(currentWinUser_);
	//玩家信息
	for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
		//桌椅ID
		uint32_t i = it->second.chairId;
		zjh::CMD_S_GameEnd_Player* player = rspdata.add_players();
		//座椅ID
		player->set_chairid(i);
		//用户手牌
		zjh::HandCards* handcards = player->mutable_handcards();
		handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
		//手牌牌型
		handcards->set_ty(handCardsType_[i]);
		//输赢得分
		if (i == currentWinUser_) {
			assert(userWinScorePure[i] > 0);
			player->set_deltascore(userWinScorePure[i]);
		}
		else {
			player->set_deltascore(-tableScore_[i]);
		}
		//剩余积分 = 携带积分 + 输赢积分
		player->set_userscore(userScore[i] + userWinScorePure[i]);
		//是否全部摊牌
		if (openAllCards_) {
			for (UserInfoList::const_iterator ir = userinfos_.begin(); ir != userinfos_.end(); ++ir) {
				if (i != ir->first) {
					player->add_compareuser(ir->first);
				}
			}
		}
		else {
			//比牌的玩家
			for (std::vector<uint32_t>::const_iterator it = comparedUser_[i].begin();
				it != comparedUser_[i].end(); ++it) {
				assert(*it < GAME_PLAYER);
				player->add_compareuser(*it);
			}
		}
		//写玩家操作记录
		string str,cardStr,dataStr;
		for (uint8_t j = 0;j < 20;++j)
		{
			if (userOperate_[i][j] != 0)
			{
				if (userOperate_[i][j] == opFollow) //跟注
				{
					str += " 第" + to_string(j + 1) + "轮=跟注,";
				} 
				else if (userOperate_[i][j] == opAddBet) //加注
				{
					str += " 第" + to_string(j + 1) + "轮=加注,";
				}
				else if (userOperate_[i][j] == opCmprOrLook) //比牌
				{
					str += " 第" + to_string(j + 1) + "轮=比牌,";
				}
				else if (userOperate_[i][j] == opQuitOrBCall) //弃牌
				{
					str += " 第" + to_string(j + 1) + "轮=弃牌,";
				}
				else if (userOperate_[i][j] == opCmprFail) //比牌输
				{
					str += " 第" + to_string(j + 1) + "轮=比牌输,";
				}
			}
		}
		cardStr = g.StringHandTy(ZJH::HandTy(handCardsType_[i]));
		if (handCardsType_[i] == ZJH::Tysp)
		{
			if (IsBigSanCard(handCards_[i]))
			{
				cardStr = "大散牌";
			}
			else
			{
				cardStr = "小散牌";
			}
		}
		else if (handCardsType_[i] == ZJH::Ty20)
		{
			if (IsBigDuizi(handCards_[i]))
			{
				cardStr = "大对子";
			}
			else
			{
				cardStr = "小对子";
			}
		}
		dataStr = getCardString(handCards_[i]);
		openLog(" === 第%d局:userId[%d],chairId[%d],handcard[%s],cardtype[%s],操作:%s", userPlayCount, it->second.userId, it->second.chairId, dataStr.c_str(), cardStr.c_str(), str.c_str());
		if (!it->second.isAndroidUser)
		{
			openLog(" === strRoundID[%s] 刷新小黑屋控制值,玩家 userid:%d, curWinTatalScore:%d, conScore:%d, curIndex:%d, curWeight:%d;", strRoundID_.c_str(), it->second.userId, userControlinfo_[i].curWin, userControlinfo_[i].conScore, userControlinfo_[i].curIndex, userControlinfo_[i].curWeight);
		}
	}
	openLog(" === 第%d局结束;", userPlayCount);
	//清理用户数据副本
	userinfos_.clear();
	//广播游戏结束消息
	std::string content = rspdata.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, zjh::SUB_S_GAME_END, (uint8_t *)content.data(), content.size());
	//通知框架结束游戏
	m_pTableFrame->ConcludeGame(GAME_STATUS_END);
	//设置游戏结束状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_END);
	//延时清理桌子数据
	//timerIdGameEnd_ = m_TimerLoopThread->getLoop()->runAfter(0.2, boost::bind(&CGameProcess::OnTimerGameEnd, this));
	OnTimerGameEnd();
	return true;
}

//游戏结束，清理数据
void CGameProcess::OnTimerGameEnd()
{
	//销毁当前定时器
	m_TimerLoopThread->getLoop()->cancel(timerIdGameEnd_);
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
		//LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << " 清理房间内玩家 BB >>> ClearTableUser tableid:" << m_pTableFrame->GetTableId() << ", chairid:" << i;
		m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
	}
	//清理所有定时器
	ClearAllTimer();
	//初始化桌子数据
	InitGameData();
	//重置桌子初始状态
	m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
}

//发送场景
bool CGameProcess::OnEventGameScene(uint32_t chairId, bool bIsLookUser)
{
	//assert(chairId != INVALID_CHAIR);
	if (chairId == INVALID_CHAIR) {
		return false;
	}
    if (!m_pTableFrame->IsExistUser(chairId)) {
		return false;
	}
	switch (m_pTableFrame->GetGameStatus())
	{
	case GAME_STATUS_INIT:
	case GAME_STATUS_READY: {		//空闲状态
		zjh::CMD_S_StatusFree rspdata;
		rspdata.set_cellscore(FloorScore);//ceilscore
		//玩家基础数据
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i); assert(userItem);
			::zjh::PlayerItem* player = rspdata.add_players();
			player->set_chairid(i);//chairID
			player->set_userid(userItem->GetUserId());//userID
			player->set_nickname(userItem->GetNickName());//nickname
			player->set_headid(userItem->GetHeaderId());//headID
			player->set_userscore(userItem->GetUserScore());//userScore
			player->set_location(userItem->GetLocation());//location
		}
		//发送场景
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, zjh::SUB_SC_GAMESCENE_FREE, (uint8_t *)content.data(), content.size());
		break;
	}
	case GAME_STATUS_PLAYING: {	//游戏状态
		zjh::CMD_S_StatusPlay rspdata;
		rspdata.set_roundid(strRoundID_);//牌局编号
		rspdata.set_cellscore(FloorScore);//底注
		rspdata.set_currentturn(currentTurn_ + 1);//轮数
		rspdata.set_bankeruser(bankerUser_);//庄家用户
		rspdata.set_currentuser(currentUser_);//操作用户
		rspdata.set_firstuser(firstUser_);//首发用户
		//可操作上下文(若非操作用户该项为空)
		zjh::OptContext* ctx = rspdata.mutable_ctx();
		{
			//当前筹码值
			ctx->set_jettonvalue(jettonValue_);
			//当前跟注分
			int64_t currentFollowScore = CurrentFollowScore(currentUser_);
			ctx->set_followscore(currentFollowScore);
			//比牌加注分
			int64_t currentCompareScore = CurrentCompareScore(currentUser_);
			ctx->set_comparescore(currentCompareScore);
			//加注筹码表
			for (std::vector<int64_t>::const_iterator it = JettionList.begin();
				it != JettionList.end(); ++it) {
				ctx->add_jettons(isLooked_[currentUser_] ? (*it * 2) : *it);
			}
			//筹码表可加注筹码起始索引
			ctx->set_start(GetCurrentAddStart());
		}
		rspdata.set_wtimeleft(opEndTime_ - (uint32_t)time(NULL));//操作剩余时间
		rspdata.set_wtotaltime(opTotalTime_);//操作总的时间
		//玩家基础数据
		for (UserInfoList::const_iterator it = userinfos_.begin(); it != userinfos_.end(); ++it) {
			//桌椅ID
			uint32_t i = it->second.chairId;
			::zjh::PlayerItem* player = rspdata.add_players();
			player->set_chairid(i);//chairID
			player->set_userid(it->second.userId);//userID
			player->set_nickname(it->second.nickName);//nickname
			player->set_headid(it->second.headerId);//headID
			//当前积分 = 携带积分 - 累计下注
			int64_t userScore = it->second.userScore - tableScore_[i];
			//assert(userScore >= 0);
			if (userScore < 0) {
				userScore = 0;
			}
			player->set_userscore(userScore);//userScore
			player->set_location(it->second.location);//location
			player->set_islooked(isLooked_[i]);
			player->set_isgiveup(isGiveup_[i]);
			player->set_iscomparedlost(isComparedLost_[i]);
			player->set_tablescore(tableScore_[i]);
			if (i == chairId && isLooked_[i]) {
				//用户手牌
				zjh::HandCards* handcards = player->mutable_handcards();
				handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				handcards->set_ty(handCardsType_[i]);
			}
			else {
				if (player->has_handcards()) player->clear_handcards();
				//用户手牌
				//zjh::HandCards* handcards = player->mutable_handcards();
				//handcards->set_cards(&(handCards_[i])[0], MAX_COUNT);
				//手牌牌型
				//handcards->set_ty(handCardsType_[i]);
			}
		}
		rspdata.set_tableallscore(tableAllScore_);
		rspdata.set_istimeoutgiveup(noGiveUpTimeout_[chairId]);
		//发送场景
		std::string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, zjh::SUB_SC_GAMESCENE_PLAY, (uint8_t *)content.data(), content.size());
		break;
	}
	case GAME_STATUS_END: {//结束状态
		zjh::CMD_S_StatusEnd rspdata;
		rspdata.set_wtimeleft(opEndTime_ - (uint32_t)time(NULL));
		string content = rspdata.SerializeAsString();
		m_pTableFrame->SendTableData(chairId, zjh::SUB_SC_GAMESCENE_END, (uint8_t *)content.data(), content.size());
	}
	break;
	default:
		break;
	}
	return true;
}

//读取配置
void CGameProcess::ReadStorageInformation()
{
	assert(m_pTableFrame);
	//系统当前库存
	m_pTableFrame->GetStorageScore(storageInfo_);
	// ReReadStock(storageInfo_);

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
	string ProStr = "Probability_" + to_string(m_pGameRoomInfo->roomId);
	//系统换牌概率，如果玩家赢，机器人最大牌也是好牌则换牌，让机器人赢
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratio_"));
		ratioSwap_ = atoi(str1.c_str());
	}
	//系统通杀概率，等于100%时，机器人必赢，等于0%时，机器人手气赢牌
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratioLost_"));
		ratioSwapLost_ = atoi(str1.c_str());
	}
	//系统放水概率，等于100%时，玩家必赢，等于0%时，玩家手气赢牌
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratioWin_"));
		ratioSwapWin_ = atoi(str1.c_str());
	}
	//发牌时尽量都发好牌的概率，玩起来更有意思
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratioDealGood_"));
		ratioDealGood_ = atoi(str1.c_str());
	}
	//点杀时尽量都发好牌的概率
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratioDealGoodKill_"));
		ratioDealGoodKill_ = atoi(str1.c_str());
	}	
	//剩最后一个AI和真人比牌,需控制系统赢时,换牌的概率
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratioChangeGoodCard_"));
		ratioChangeGoodCard_ = atoi(str1.c_str());
	}
	{
		std::string str1(ini.GetString("path", "cardList"));
		INI_CARDLIST_ = str1;
	}
	//剩最后一个AI和真人比牌,需控制系统赢时,玩家赢分的范围
	{
		std::string str1(ini.GetString(ProStr.c_str(), "controlWinScoreRate_"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					controlWinScoreRate_[i++] = atoi(str1.c_str());
				}
				break;
			}
			controlWinScoreRate_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//制造各牌型的权重
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratioMadeCard_"));
		int i = 1, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratioMadeCard_[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratioMadeCard_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//个人库存控制的基本底分倍数
	{
		std::string str1(ini.GetString(ProStr.c_str(), "kcBaseTimes_"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.kcBaseTimes_[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.kcBaseTimes_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
	//个人库存控制的概率
	{
		std::string str1(ini.GetString(ProStr.c_str(), "ratiokcBaseTimes_"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					robot_.ratiokcBaseTimes_[i++] = atoi(str1.c_str());
				}
				break;
			}
			robot_.ratiokcBaseTimes_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}
    stockWeak=ini.GetFloat("Global", "STOCK_WEAK",1.0);
    kcTimeoutHour_=ini.GetFloat("control", "kcTimeoutHour_",1.0);
    ratiokcreduction_ = ini.GetFloat("control", "ratiokcreduction_",10.0);
	newUserTimeoutMinute_ = ini.GetFloat("control", "newUserTimeoutMinute_", 1.0);
	//算法控制方式(0:个人库存控制；1:小黑屋控制)
	std::string str1(ini.GetString("Global", "CONTROLBLACKROOM"));
	controlType_BlackRoom = atoi(str1.c_str());

	controlledProxy.clear();
	//获取需控制的代理号
	{
		std::string str1(ini.GetString("Global", "AGENTID"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of(',');
			if (p == -1) {
				if (!str1.empty()) {
					controlledProxy.push_back(atoi(str1.c_str()));
				}
				break;
			}
            controlledProxy.push_back(atoi(str1.substr(0, p).c_str()));
			str1 = str1.substr(p + 1, -1);
		}
	}
	// 获取调整的难度
	controllPoint = ini.GetFloat("Global", "CONTROLLPOINT", 10.0);

	//调整个人库存控制的控制倍数
	{
		std::string str1(ini.GetString("Global", "addBaseTimes_"));
		int i = 0, p;
		while (!str1.empty()) {
			p = str1.find_first_of('|');
			if (p == -1) {
				if (!str1.empty()) {
					addBaseTimes_[i++] = atoi(str1.c_str());
				}
				break;
			}
			addBaseTimes_[i++] = atoi(str1.substr(0, p).c_str());
			str1 = str1.substr(p + 1, -1);
		}
	}

	//LOG(ERROR) << "===== ReadStorageInformation  roomID " << m_pGameRoomInfo->roomId << " 保险换牌 ratioChangeGoodCard_ " << ratioChangeGoodCard_ << " 做牌 ratioDealGood_ " << ratioDealGood_ ;
}

//更新控制的配置
void CGameProcess::UpdateControlConfig()
{
	CINIHelp ini;
	ini.Open(INI_FILENAME);
}
//是否大散牌
bool CGameProcess::IsBigSanCard(uint8_t cards[]) {
	//手牌按牌点从大到小排序(AK...32)
	bool bbigSp = false;
	uint8_t tempCards[MAX_COUNT] = {0};
	memcpy(tempCards,cards,sizeof(tempCards));
	ZJH::CGameLogic::SortCards(tempCards, MAX_COUNT, false, false, false);
	uint8_t cardvalue = ZJH::CGameLogic::GetCardPoint(tempCards[0]);
	if (cardvalue > 0x0B)
	{
		bbigSp = true;
	}
	return bbigSp;
}

//是否大对子
bool CGameProcess::IsBigDuizi(uint8_t cards[]) {
	//手牌按牌点从大到小排序(AK...32)
	bool bbigDz = false;
	uint8_t tempCards[MAX_COUNT] = {0};
	memcpy(tempCards,cards,sizeof(tempCards));
	ZJH::CGameLogic::SortCards(tempCards, MAX_COUNT, false, false, false);
	uint8_t cardvalue = ZJH::CGameLogic::GetCardPoint(tempCards[1]);
	if (cardvalue > 0x0B)
	{
		bbigDz = true;
	}
	return bbigDz;
}
//获取造牌的类型Index
uint8_t CGameProcess::getMadeCardTypeIndex(int ratio[],int32_t curIndex) {
	int allweight = 0;
	int value = 0;
	int choosevalue = 0;
    int index = 0;
	int currentProbability = 0;		
	int cardTypeRatio[7] = { 0 };
	CopyMemory(cardTypeRatio, ratio, sizeof(cardTypeRatio));
	//if (curIndex > 0)
	//{
	//	for (int i = 2;i < 7;++i)
	//	{
	//		cardTypeRatio[i] += 6 * curIndex * (7 - i);
	//	}
	//}
	int cardTypeIndexCount = 0;
	for (int i = 3;i < 7;++i)
	{
		if (haveMadeCardType[i]!=0) //已出现的牌型
		{
			cardTypeIndexCount = haveMadeCardType[i];
			for (int j = i;j < 7;++j)
			{
				/*if (curIndex > 0)
				{
					if (cardTypeRatio[j] >= (6 * curIndex * (7 - j) * cardTypeIndexCount))
					{
						cardTypeRatio[j] -= 6 * curIndex * (7 - j) * cardTypeIndexCount;
					}
					else
					{
						cardTypeRatio[j] = ratio[j];
					}
				}
				else*/ 
				if (cardTypeRatio[j] >= 10 * cardTypeIndexCount)
				{
					cardTypeRatio[j] -= 10 * cardTypeIndexCount;
				}
				else
				{
					cardTypeRatio[j] = 0;
				}
			}
		}
	}
	for (int i = 1; i < 7; ++i)
	{
		allweight += cardTypeRatio[i];
	}
	value = m_random.betweenInt(1, allweight).randInt_mt(true);//RandomBetween(1, allweight);
	choosevalue = value;
    for (int i = 1; i < 7; ++i)
	{
		value -= cardTypeRatio[i];
		if (value <= 0) {
			index = i;
			break;
		}
	}
	// LOG(ERROR) << "===== 选择造牌类型 allweight " << allweight << " value " << choosevalue  << " index " << index << " [" << g.StringHandTy(ZJH::HandTy(index)) << "] ";
	return index;
}
//计算控制的等级
void CGameProcess::CalculateControlLv(int chairId, int64_t allWinscore)
{
	userControlinfo_[chairId].curWin = allWinscore;
	float f = g.RandomBetweenFloat(0.5f,1.0f);
	//控制的系数
	int curTime = allWinscore/CellScore;
	//控制的等级
	int conIndex = 0;
	userControlinfo_[chairId].kcBaseTimes.clear();
	userControlinfo_[chairId].kcweights.clear();
	for (auto it : robot_.kcBaseTimes_)
	{
        userControlinfo_[chairId].kcBaseTimes.push_back(it);
	}
	for (auto it : robot_.ratiokcBaseTimes_)
	{
        userControlinfo_[chairId].kcweights.push_back(it);
	}
	conIndex = GetCurrentControlLv(chairId,curTime);
	if (conIndex > userControlinfo_[chairId].curIndex) //玩家继续赢球,相似刷新控制等级
	{
        userControlinfo_[chairId].conScore = userControlinfo_[chairId].curWin*f;
		userControlinfo_[chairId].curIndex = conIndex;
    	userControlinfo_[chairId].curWeight = robot_.ratiokcBaseTimes_[conIndex];
	}
	else if (userControlinfo_[chairId].conScore == 0 && userControlinfo_[chairId].curWin>=0) //当前控制结束,刷新控制等级
	{
        userControlinfo_[chairId].conScore = userControlinfo_[chairId].curWin*f;
		userControlinfo_[chairId].curIndex = conIndex;
    	userControlinfo_[chairId].curWeight = robot_.ratiokcBaseTimes_[conIndex];
	}
	else if (userControlinfo_[chairId].curWin < 0)
	{
		userControlinfo_[chairId].conScore = 0;
		userControlinfo_[chairId].curIndex = 0;
    	userControlinfo_[chairId].curWeight = robot_.ratiokcBaseTimes_[conIndex]/2;
	}
	if (isControlAgentId(chairId) /*&& !isGemeEnd_*/)
	{
		openLog(" === strRoundID[%s] 代理基础控制概率: curWin=%d, curWeight=%d", strRoundID_.c_str(), userControlinfo_[chairId].curWin, userControlinfo_[chairId].curWeight);
		userControlinfo_[chairId].curWeight += controllPoint;
		openLog(" === strRoundID[%s] 代理加难后控制概率: curWin=%d, curWeight=%d", strRoundID_.c_str(), userControlinfo_[chairId].curWin, userControlinfo_[chairId].curWeight);
	}
}
//是否剩余最后一个机器人了
void CGameProcess::AnalyseLastAndroidUser(int chairId, int nextChairId)
{
	int androidCount = IsLastAndroidUser();
	bool bfirstAndroid = m_pTableFrame->IsAndroidUser(chairId);
	bool bnextAndroid = m_pTableFrame->IsAndroidUser(nextChairId);
	bool bAndroidLost = false;
	uint32_t winUser = INVALID_CHAIR, lostUser = INVALID_CHAIR;
	uint32_t AndroidUser = INVALID_CHAIR;
	if (androidCount==1 && (bfirstAndroid || bnextAndroid))
	{
		//比牌得到赢家与输家用户
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[chairId])[0], &(handCards_[nextChairId])[0], special235_) > 0) {
			winUser = chairId; lostUser = nextChairId;
			if (bnextAndroid)
			{
				bAndroidLost = true;
				AndroidUser = nextChairId;
			}
		}
		else {
			winUser = nextChairId; lostUser = chairId;
			if (bfirstAndroid)
			{
				bAndroidLost = true;
				AndroidUser = chairId;
			}
		}
	
		int randNum = m_random.betweenInt(1, 100).randInt_mt(true);
		int64_t randControlScore = 0; //RandomBetween(25 * CellScore, 150 * CellScore);
		int64_t currentCanWinScore = (tableAllScore_ - tableScore_[winUser]);//currentHaveWinScore_[winUser];// + 
		int ChangeGoodCard = 0;
	    if (bAndroidLost)
	    {
	    	LOG(ERROR) << "==== [" << strRoundID_ << "] 最后一个AI[" << AndroidUser << "] 换牌概率 randNum:" << randNum << " ratioChangeGoodCard_:" << ratioChangeGoodCard_
	    		       << " winUser:" << winUser << " currentCanWinScore:" << currentCanWinScore << " randControlScore:" << currentHaveWinScore_[winUser];
			
			randControlScore = currentHaveWinScore_[winUser];
			//系统输分则通杀
			if (StockScore < StockLowLimit)
			{
				int64_t stockLoseScore = StockLowLimit - StockScore;
				double CellOddz = 1.0f * stockLoseScore / CellScore;
				randControlScore = currentHaveWinScore_[winUser] / 2;
				if (userControlinfo_[winUser].curWin >= 0)
				{
					ChangeGoodCard = ratioChangeGoodCard_ * (1 + CellOddz / 1000.0f) + userControlinfo_[winUser].curWeight;  // 概率>=45
					randControlScore = currentHaveWinScore_[winUser]*1.0 / (2*(1 + CellOddz / 1000.0f));
				}
				else
				{
					ChangeGoodCard = ratioChangeGoodCard_ * (1 + CellOddz / 1000.0f) + userControlinfo_[winUser].curWeight;  // 概率>=35
				}
				if (isControlAgentId(winUser))
				{
					ChangeGoodCard += controllPoint;
				}
			}
			else if (StockScore > StockHighLimit)
			{
				if (userControlinfo_[winUser].curWin >= 0)
				{
					ChangeGoodCard = ratioChangeGoodCard_ + userControlinfo_[winUser].curWeight / 2; // 概率>=30
				}
				else
				{
					ChangeGoodCard = ratioChangeGoodCard_ / 2; // 概率>=12
				}
			}
			else
			{
				if (userControlinfo_[winUser].curWin < (-controlWinScoreRate_[1] * CellScore * 15))
				{
					ChangeGoodCard = ratioChangeGoodCard_ / 2;//+userControlinfo_[winUser].curWeight / 2;  // 概率>=17
				}
				else if (userControlinfo_[winUser].curWin < (-controlWinScoreRate_[1] * CellScore * 6))
				{
					ChangeGoodCard = ratioChangeGoodCard_ / 2 + userControlinfo_[winUser].curWeight / 2;  // 概率>=22
				}
				else if (userControlinfo_[winUser].curWin < 0)
				{
					ChangeGoodCard = ratioChangeGoodCard_; //+ userControlinfo_[winUser].curWeight / 2;  // 概率>=30
				}
				else
				{
					ChangeGoodCard = ratioChangeGoodCard_ + userControlinfo_[winUser].curWeight;	  // 概率>=45			
				}
				if (isControlAgentId(winUser))
				{
					ChangeGoodCard += controllPoint;
				}
			}
			
	    }			
		openLog(" === strRoundID[%s] 最后一个AI和玩家比牌,bAndroidLost=%s, randNum=%d, ChangeGoodCard=%d, currentCanWinScore=%d, randControlScore=%d", strRoundID_.c_str(), bAndroidLost ? "true" : "false", randNum, ChangeGoodCard, currentCanWinScore, randControlScore);
		if (bAndroidLost && randNum < ChangeGoodCard && currentCanWinScore >= randControlScore)
		{
			//如果最大手牌是豹子,就不换牌了，如果是大顺金，少于8轮也不换牌
			blastAndroidChangeCard = true;
			if (bfirstAndroid && isNeedChangeCards())
			{
				LOG(ERROR) << "==== [" << strRoundID_ << "] 控制换牌 原牌:[ " << getCardString(handCards_[chairId]) << "] <<==>> [" << getCardString(madeMaxCards_) << "]";
				memcpy(&(handCards_[chairId])[0], &(madeMaxCards_)[0], sizeof(uint8_t)*MAX_COUNT);
				//交换牌型
				handCardsType_[chairId] = madeMaxCardsType_;
			}
			else if (bnextAndroid)
			{
				LOG(ERROR) << "==== [" << strRoundID_ << "] 控制换牌 原牌:[ " << getCardString(handCards_[nextChairId]) << "] <<==>> [" << getCardString(madeMaxCards_) << "]";
				memcpy(&(handCards_[nextChairId])[0], &(madeMaxCards_)[0], sizeof(uint8_t)*MAX_COUNT);
				//交换牌型
				handCardsType_[nextChairId] = madeMaxCardsType_;
			}
		}
	}
}
//获取当前的控制等级
int CGameProcess::GetCurrentControlLv(int chairId,int curTime)
{
	int TmpBaseTimes[6] = { 0 };
    if (isControlAgentId(chairId))
	{
		CopyMemory(TmpBaseTimes, addBaseTimes_, sizeof(TmpBaseTimes));
	}
	int conIndex = 0;	
    for (int i = 5; i >= 0; --i)
	{
		robot_.kcBaseTimes_[i] -= TmpBaseTimes[i];
        if (curTime >= robot_.kcBaseTimes_[i])
		{
			conIndex = i;
			break;
		}
	}
	return conIndex;
}
//是否剩下最后一个机器人
int CGameProcess::IsLastAndroidUser()
{
	int androidCount = 0;
	for (int i = 0; i < GAME_PLAYER; ++i)
	{
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//用户已出局(弃牌/比牌输)
		if (isGiveup_[i] || isComparedLost_[i]) {
			continue;
		}
		//统计机器人
		if (!m_pTableFrame->IsAndroidUser(i)) {
			continue;
		}
		++androidCount;
	}
	return androidCount;
}
string CGameProcess::getCardString(uint8_t cards[])
{
	string cardStr = "";
	char buf[32]={0};
	for (int j = 0; j < MAX_COUNT; ++j)
	{
		snprintf(buf,sizeof(buf),"%02X ", (unsigned char)cards[j]);
		cardStr += buf;
	}
    return cardStr;
}
//获取当前赢家
uint32_t CGameProcess::getWinUser()
{
	uint32_t WinUser = INVALID_CHAIR;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		//初始化
		if (WinUser == INVALID_CHAIR) {
			WinUser = i;
			continue;
		}
		//比牌判断当前最大牌用户
		if (ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[WinUser])[0], special235_) > 0) {
			WinUser = i;
		}
	}
	return WinUser;
}
//制造两手好牌
void CGameProcess::getTwoGoodCard()
{
	WORD wAndroidUser = INVALID_CHAIR;
	WORD wTrueUser = INVALID_CHAIR;
	WORD wAndroidArray[GAME_PLAYER] = { 0 };
	uint8_t androidCount = 0;
	for (int i = 0; i < GAME_PLAYER; ++i)
	{
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		if (m_pTableFrame->IsAndroidUser(i))
			wAndroidArray[androidCount++] = i;
		else
			wTrueUser = i;
	}
	if (wTrueUser != INVALID_CHAIR)
	{
		int32_t randNum = m_random.betweenInt(1, 100).randInt_mt(true);
		int32_t randOddz = m_random.betweenInt(4, 6).randInt_mt(true);
        if (userControlinfo_[wTrueUser].curWin > 0 && randNum <= ratioDealGood_ ) //userControlinfo_[wTrueUser].curWeight
		{
			if (androidCount > 0)
			{
				randNum = m_random.betweenInt(0, androidCount - 1).randInt_mt(true);
				wAndroidUser = wAndroidArray[randNum];
			}
			for (int i = 0; i < GAME_PLAYER; ++i)
			{
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				if (i != wAndroidUser && i != wTrueUser)
					g.addHaveChooseCard(&(handCards_[i])[0], MAX_COUNT);
			}
			for (int i = 0;i < GAME_PLAYER;++i)
			{
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				if (i == wAndroidUser || i == wTrueUser)
				{
					uint8_t cardType = getMadeCardTypeIndex(robot_.ratioMadeCard_);
					g.MadeHandCards(&(handCards_[i])[0], cardType);
					uint8_t cardIndex = ZJH::CGameLogic::GetHandCardsType(&(handCards_[i])[0]);
					madeCardNum_[0]++;
					madeCardNum_[cardIndex]++;
					haveMadeCardType[cardIndex]++;
					LOG(ERROR) << "===== 需造牌玩家[" << i << "] 牌型 [" << g.StringHandTy(ZJH::HandTy(cardType)) << "] " << " 得到的牌型:"
						<< g.StringHandTy(ZJH::HandTy(cardIndex)) << " [" << getCardString(handCards_[i]) << "]";
				}
			}
		}
        else if ((userControlinfo_[wTrueUser].curWin <= (-controlWinScoreRate_[1] * CellScore * 10) && randNum <= ratioDealGood_*2) ||
				 (userControlinfo_[wTrueUser].curWin <= (-controlWinScoreRate_[1] * CellScore * randOddz) && randNum <= ratioDealGood_))
		{
			for (int i = 0; i < GAME_PLAYER; ++i)
			{
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				if (i != wTrueUser)
					g.addHaveChooseCard(&(handCards_[i])[0], MAX_COUNT);
			}
			uint8_t cardType = getMadeCardTypeIndex(robot_.ratioMadeCard_);
			g.MadeHandCards(&(handCards_[wTrueUser])[0], cardType);
			uint8_t cardIndex = ZJH::CGameLogic::GetHandCardsType(&(handCards_[wTrueUser])[0]);
			madeCardNum_[0]++;
			madeCardNum_[cardIndex]++;
			haveMadeCardType[cardIndex]++;
			LOG(ERROR) << "===== 玩家输太多了需造牌玩家[" << wTrueUser << "] 牌型 [" << g.StringHandTy(ZJH::HandTy(cardType)) << "] " << " 得到的牌型:"
				<< g.StringHandTy(ZJH::HandTy(cardIndex)) << " [" << getCardString(handCards_[wTrueUser]) << "]";
			openLog(" === strRoundID[%s] 玩家输太多了需造牌玩家,玩家 userid:%d", strRoundID_.c_str(), m_pTableFrame->GetTableUserItem(wTrueUser)->GetUserId());
		}
		else
		{
			for (int i = 0; i < GAME_PLAYER; ++i)
			{
				if (!m_pTableFrame->IsExistUser(i)) {
					continue;
				}
				g.addHaveChooseCard(&(handCards_[i])[0], MAX_COUNT);
			}
		}
	}
}

//是否加难控制的代理
bool CGameProcess::isControlAgentId(int chairId)
{
	bool bControlAgentId = false;
    shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(chairId);
	if (userItem) {
		if (!userItem->IsAndroidUser())
		{
			int32_t agentId = userItem->GetUserBaseInfo().agentId;
			for (int i = 0; i < controlledProxy.size(); i++)
			{
				if (agentId == controlledProxy[i])
				{
					LOG_INFO << " ************ userRegSecond:" << userItem->GetUserRegSecond();
					if (userItem->GetUserRegSecond() < (int)newUserTimeoutMinute_ * 60)
					{
						bControlAgentId = true;
						openLog(" === strRoundID[%s] 代理控制加难:agentId=%d,addpro=%d,chairId=%d,userid:%d,RegSecond:%d;", strRoundID_.c_str(), agentId, controllPoint, chairId, userItem->GetUserId(), userItem->GetUserRegSecond());
						break;
					}
				}
			}
		}
	}
	return bControlAgentId;
}

//如果最大手牌是豹子,就不换牌了，如果是大顺金，少于8轮也不换牌
bool CGameProcess::isNeedChangeCards()
{
	bool bNeedChang = true;
	if (madeMaxCardsType_ == ZJH::Ty30)
	{
		bNeedChang = false;
		openLog(" === strRoundID[%s] 最大牌是炸弹,就不换牌;", strRoundID_.c_str());
	}
    else if (madeMaxCardsType_ == ZJH::Ty123sc)
	{
		//手牌按牌点从大到小排序(AK...32)
		uint8_t tempCards[MAX_COUNT] = { 0 };
		memcpy(tempCards, madeMaxCards_, sizeof(tempCards));
		ZJH::CGameLogic::SortCards(tempCards, MAX_COUNT, false, false, false);
		uint8_t cardvalue = ZJH::CGameLogic::GetCardPoint(tempCards[0]);
        if (cardvalue > 0x09 && currentTurn_ < 8)
		{
			bNeedChang = false;
			openLog(" === strRoundID[%s] 最大牌是大顺金,就不换牌;", strRoundID_.c_str());
		}
	}		

	return bNeedChang;
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

