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

#include "proto/zjh.Message.pb.h"

#include "../Game_zjh/MSG_ZJH.h"
#include "../QPAlgorithm/zjh.h"
#include "../QPAlgorithm/cfg.h"

#include "AndroidConfig.h"

#include "AndroidUserItemSink.h"

#define TIME_GAME_ADD_SCORE       15 //下注时长(second)
#define TIME_GAME_COMPARE_DELAY   4  //比牌动画时长

static std::string StringOpCode(int op) {
	switch (op)
	{
	case OP_ALLIN: return "孤注一掷";
	case OP_RUSH: return "火拼";
	case OP_COMPARE: return "比牌";
	case OP_GIVEUP: return  "弃牌";
	case OP_FOLLOW: return "跟注";
	case OP_ADD: return "加注";
	case OP_LOOKOVER: return "看牌";
	default: break;
	}
}

CAndroidUserItemSink::CAndroidUserItemSink()
    : m_pTableFrame(NULL)
    , m_pAndroidUserItem(NULL)
	, storageInfo_({0})
	, rengine(time(NULL))
{
	//当前最大牌用户
	currentWinUser_ = INVALID_CHAIR;
	//真实玩家中最大牌用户
	realWinUser_ = INVALID_CHAIR;
	//机器人中最大牌用户
	androidWinUser_ = INVALID_CHAIR;
    bankerUser_ = INVALID_CHAIR;
    currentUser_ = INVALID_CHAIR;
	firstUser_ = INVALID_CHAIR;
	currentTurn_ = -1;
	currentOp_ = OP_INVALID;
	//是否机器人
	memset(isAndroidUser_, 0, sizeof(isAndroidUser_));
	//手牌/牌型
    memset(handCards_, 0, sizeof(handCards_));
    memset(handCardsType_, 0, sizeof(handCardsType_));
	//是否看牌
	memset(isLooked_, 0, sizeof isLooked_);
	//是否弃牌
	memset(isGiveup_, 0, sizeof isGiveup_);
	//是否比牌输
	memset(isComparedLost_, 0, sizeof isComparedLost_);
	for (int i = 0; i < MAX_ROUND; ++i) {
		turninfo_[i].startuser = INVALID_CHAIR;
		turninfo_[i].enduser = INVALID_CHAIR;
		turninfo_[i].addflag = false;
	}
	special235_ = false;
	freeLookover_ = false;
	freeGiveup_ = false;
	useStrategy_ = false;
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

bool CAndroidUserItemSink::RepositionSink()
{
	//当前最大牌用户
	currentWinUser_ = INVALID_CHAIR;
	//真实玩家中最大牌用户
	realWinUser_ = INVALID_CHAIR;
	//机器人中最大牌用户
	androidWinUser_ = INVALID_CHAIR;
	bankerUser_ = INVALID_CHAIR;
	currentUser_ = INVALID_CHAIR;
	firstUser_ = INVALID_CHAIR;
	currentTurn_ = -1;
	currentOp_ = OP_INVALID;
	//是否机器人
	memset(isAndroidUser_, 0, sizeof(isAndroidUser_));
	//手牌/牌型
    memset(handCards_, 0, sizeof(handCards_));
    memset(handCardsType_, 0, sizeof(handCardsType_));
	//是否看牌
	memset(isLooked_, 0, sizeof isLooked_);
	//是否弃牌
	memset(isGiveup_, 0, sizeof isGiveup_);
	//是否比牌输
	memset(isComparedLost_, 0, sizeof isComparedLost_);
	memset(&robot_, 0, sizeof robot_);
	//是否已重置等待时长
	//randWaitResetFlag_ = false;
	//累计等待时长
	//totalWaitSeconds_ = 0;
	//分片等待时间
	sliceWaitSeconds_ = 0.5f;
	//随机等待时长
	//randWaitSeconds_ = randWaitSeconds(ThisChairId);
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
	//读取机器人配置
	//ReadConfig();
}

//消息处理
bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize)
{
	switch (wSubCmdID)
	{
	case zjh::SUB_S_ANDROID_CARD: {	//机器人牌 - 服务端广播 ///
		return onAndroidCard(pData, wDataSize);
	}
	case zjh::SUB_S_GAME_START: {	//游戏开始 - 服务端广播 ///
		return onGameStart(pData, wDataSize);
	}
	case zjh::SUB_S_LOOK_CARD: {	//看牌结果 - 服务端回复 ///
		return resultLookCard(pData, wDataSize);
	}
	case zjh::SUB_S_GIVE_UP: {		//弃牌结果 - 服务端回复 ///
		return resultGiveup(pData, wDataSize);
	}
	case zjh::SUB_S_ADD_SCORE: {	//跟注/加注结果 - 服务端回复 ///
		return resultFollowAdd(pData, wDataSize);
	}
	case zjh::SUB_S_COMPARE_CARD: {	//比牌结果 - 服务端回复 ///
		return resultCompareCard(pData, wDataSize);
	}
	case zjh::SUB_S_ALL_IN: {		//孤注一掷结果 - 服务端回复 ///
		return resultAllIn(pData, wDataSize);
	}
	case zjh::SUB_S_GAME_END: {		//游戏结束 - 服务端广播 ///
		return onGameEnd(pData, wDataSize);
	}
	}
    return true;
}

//机器人牌 - 服务端广播 ///
bool CAndroidUserItemSink::onAndroidCard(const void * pBuffer, int32_t wDataSize)
{
	zjh::CMD_S_AndroidCard rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	LOG(WARNING) << "--- *** [" << rspdata.roundid() << "] 机器人牌 ......";
	//牌局编号
	strRoundID_ = rspdata.roundid();
	//玩家信息
	for (int i = 0; i < rspdata.players_size(); ++i) {
		//座椅玩家
		zjh::AndroidPlayer const& player = rspdata.players(i);
		//座椅ID
		int j = player.chairid();
		//玩家手牌
		memcpy(&(handCards_[j])[0], player.handcards().cards().data(), MAX_COUNT);
		//手牌牌型
		handCardsType_[j] = ZJH::HandTy(player.handcards().ty());
		//是否机器人
		assert(m_pTableFrame->IsExistUser(j));
		isAndroidUser_[j] = m_pTableFrame->IsAndroidUser(j);
	}
	//系统当前库存
	storageInfo_.lEndStorage = rspdata.stockscore();
	//系统库存下限
	storageInfo_.lLowlimit = rspdata.stocklowlimit();
	//系统库存上限
	storageInfo_.lUplimit = rspdata.stockhighlimit();
	//当前最大牌用户
	currentWinUser_ = rspdata.winuser();
	assert(currentWinUser_ != INVALID_CHAIR);
	assert(m_pTableFrame->IsExistUser(currentWinUser_));
	//真实玩家中最大牌用户
	realWinUser_ = INVALID_CHAIR;
	//机器人中最大牌用户
	androidWinUser_ = INVALID_CHAIR;
	//牌型有效
	assert(handCardsType_[ThisChairId] != ZJH::TyNil);
	//带A散牌
	if (handCardsType_[ThisChairId] == ZJH::Tysp &&
		ZJH::CGameLogic::HasCardValue(&(handCards_[ThisChairId])[0])) {
	}
	else {
	}
	//机器人配置
	zjh::AndroidConfig const& cfg = rspdata.cfg();
	//判断特殊牌型
	special235_ = cfg.special235_();
	//开启随时看牌
	freeLookover_ = cfg.freelookover_();
	//开启随时弃牌
	freeGiveup_ = cfg.freegiveup_();
	//启用三套决策(0,1,2)
	useStrategy_ = cfg.usestrategy_();
	//放大倍数
	robot_.scale = cfg.scale();
	//看牌参数，权值倍率，修正参数
	for (int i = 0; i < cfg.param_size(); ++i) {
		robot_.param[i].K_[0] = cfg.param(i).k_(0);
		robot_.param[i].K_[1] = cfg.param(i).k_(1);
		range_K[i].param(std::uniform_real_distribution<float>::param_type{
			robot_.param[i].K_[0], robot_.param[i].K_[1] });
		robot_.param[i].W1_[0] = cfg.param(i).w1_(0);
		robot_.param[i].W1_[1] = cfg.param(i).w1_(1);
		range_W1[i].param(std::uniform_real_distribution<float>::param_type{
			robot_.param[i].W1_[0], robot_.param[i].W1_[1] });
		robot_.param[i].P1_ = cfg.param(i).p1_();
		robot_.param[i].W2_[0] = cfg.param(i).w2_(0);
		robot_.param[i].W2_[1] = cfg.param(i).w2_(1);
		range_W2[i].param(std::uniform_real_distribution<float>::param_type{
			robot_.param[i].W2_[0], robot_.param[i].W2_[1] });
		robot_.param[i].P2_ = cfg.param(i).p2_();
	}
	//看牌概率
	for (int i = 0; i < cfg.ratiokp_size(); ++i) {
		robot_.ratioKP[i] = cfg.ratiokp(i);
	}
	for (int i = 0; i < 3; ++i) {
		//收分时三套决策(0,1,2)几率
		robot_.SP0[i] = cfg.sp0(i);
		//正常时三套决策(0,1,2)几率
		robot_.SP1[i] = cfg.sp1(i);
		//放分时三套决策(0,1,2)几率
		robot_.SP2[i] = cfg.sp2(i);
	}
	//反序列化机器人配置 ///
	std::vector<std::pair<double, double>> robot;
	for (int i = 0; i < cfg.robots_size(); ++i) {
		//[0]胜率 [1]权重
		robot.push_back(std::make_pair<int,int>(cfg.robots(i).values(0), cfg.robots(i).values(1)));
	}
	UnserialConfig(robot);
	ReadOperateConfig();
	return true;
}

//游戏开始 - 服务端广播 ///
bool CAndroidUserItemSink::onGameStart(const void * pBuffer, int32_t wDataSize)
{
	zjh::CMD_S_GameStart rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	LOG(WARNING) << "--- *** [" << rspdata.roundid() << "] 游戏开始 !!!!!!";
	currentTurn_ = -1;
	//牌局编号
	strRoundID_ = rspdata.roundid();
	//当前第几轮
	turninfo_[++currentTurn_].startuser = currentUser_;
	//操作剩余时间
	uint32_t wTimeLeft = rspdata.wtimeleft();
	//操作总的时间
	opTotalTime_ = wTimeLeft;
	//操作开始时间
	opStartTime_ = (uint32_t)time(NULL);
	//操作结束时间
	opEndTime_ = opStartTime_ + wTimeLeft;
	//当前最大牌用户
	assert(currentWinUser_ != INVALID_CHAIR);
	assert(m_pTableFrame->IsExistUser(currentWinUser_));
    //庄家用户
    bankerUser_  = rspdata.bankeruser();
	assert(bankerUser_ != INVALID_CHAIR);
	assert(m_pTableFrame->IsExistUser(bankerUser_));
	//操作用户
    currentUser_ = rspdata.currentuser();
	assert(currentUser_ != INVALID_CHAIR);
	assert(m_pTableFrame->IsExistUser(currentUser_));
	//首发用户
	firstUser_ = currentUser_;
	assert(firstUser_ != INVALID_CHAIR);
	assert(m_pTableFrame->IsExistUser(firstUser_));
    //是否看牌
	memset(isLooked_, 0, sizeof isLooked_);
	//是否弃牌
	memset(isGiveup_, 0, sizeof isGiveup_);
	//是否比牌输
	memset(isComparedLost_, 0, sizeof isComparedLost_);
	//各玩家下注
	int64_t tableAllscore = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
		if (!m_pTableFrame->IsExistUser(i)) {
			continue;
		}
		tableScore_[i] = rspdata.tablescore(i);
		tableAllscore += tableScore_[i];
	}
	//可操作上下文
	{
		//当前筹码值
		jettonValue_ = rspdata.ctx().jettonvalue();
		//当前跟注分(初始基础跟注分)
		followScore_ = rspdata.ctx().followscore();
	}
	//桌面总下注
	tableAllScore_ = rspdata.tableallscore();
	assert(tableAllscore == tableAllScore_);
	//是否诈唬
	bluffFlag_ = false;
	//是否已重置等待时长
	//randWaitResetFlag_ = false;
	//累计等待时长
	//totalWaitSeconds_ = 0;
	//分片等待时间
	sliceWaitSeconds_ = robot_.ratiosliceWaitSeconds;
	//随机等待时长
	//randWaitSeconds_ = randWaitSeconds(ThisChairId, 1, -1);
	//等待机器人操作
	waitingOperate(1, -1);
    return true;
}

//看牌结果 - 服务端回复 ///
bool CAndroidUserItemSink::resultLookCard(const void * pBuffer, int32_t wDataSize)
{
	//旧版本：真实玩家随时可以看牌，机器人只有当操作用户是自己时才可以看牌
	//新版本：真实玩家和机器人随时可以看牌
	zjh::CMD_S_LookCard rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}

	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}

	//操作用户是自己
	if (ThisChairId == currentUser_) {
	}
	//操作用户是别人
	else {
	}
	
	//安全断言
	assert(!isLooked_[rspdata.lookcarduser()]);
	//标记看牌
	isLooked_[rspdata.lookcarduser()] = true;
	//当前筹码值
	//assert(jettonValue_ == rspdata.ctx().jettonvalue());
	//当前跟注分
	//int64_t currentFollowScore = CurrentFollowScore(rspdata.lookcarduser());
	//assert(currentFollowScore == rspdata.ctx().followscore());
	//比牌加注分
	//int64_t currentCompareScore = CurrentCompareScore(rspdata.lookcarduser());
	//assert(currentCompareScore == rspdata.ctx().comparescore());
	//操作用户看牌 ///
	if (rspdata.lookcarduser() == currentUser_) {
	}
	//非操作用户看牌 ///
	else {
	}
	if (rspdata.lookcarduser() == currentUser_ && ThisChairId == currentUser_) {
		//安全断言
		//assert(rspdata.currentuser() == currentUser_);
		//assert(m_pTableFrame->IsExistUser(currentUser_));
		//assert(m_pTableFrame->IsExistUser(rspdata.lookcarduser()));
		//操作剩余时间 = 操作结束时间 - 当前时间
		uint32_t wTimeLeft = rspdata.wtimeleft();
		assert(wTimeLeft > 0);
		//操作剩余时间(针对操作用户currentUser_)
		wTimeLeft = (ThisChairId == currentUser_) ? wTimeLeft : -1;
		//等待机器人操作
		waitingOperate(0, wTimeLeft);
	}
	return true;
}

//弃牌结果 - 服务端回复 ///
bool CAndroidUserItemSink::resultGiveup(const void * pBuffer, int32_t wDataSize)
{
	//旧版本：真实玩家随时可以弃牌，机器人只有当操作用户是自己时才可以弃牌
	//新版本：真实玩家和机器人随时可以弃牌(机器人弃牌之前先看牌)
	zjh::CMD_S_GiveUp rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	//当前第几轮
	assert(currentTurn_ + 1 == rspdata.currentturn());
	//安全断言
	assert(!isGiveup_[currentUser_]);
	assert(!isComparedLost_[currentUser_]);
	assert(!isGiveup_[rspdata.giveupuser()]);
	assert(m_pTableFrame->IsExistUser(currentUser_));
	//标记弃牌
	isGiveup_[rspdata.giveupuser()] = true;
#if 0
	{
		//当前最大牌用户
		if (isGiveup_[currentWinUser_]) {
			EstimateWinner(EstimateAll);
		}
		//真实玩家中最大牌用户
		if (realWinUser_ != INVALID_CHAIR && isGiveup_[realWinUser_]) {
			EstimateWinner(EstimatePlayer);
		}
		//机器人中最大牌用户
		if (androidWinUser_ != INVALID_CHAIR && isGiveup_[androidWinUser_]) {
			EstimateWinner(EstimateAndroid);
		}
	}
#endif
	//胜率/权重
	std::pair<double, double>* values = GetWinRatioAndWeight();
	assert(values != NULL);
	//自己弃牌 //////
	if (isGiveup_[ThisChairId]) {
		//权重减少2
		values->second -= 2;
		if (values->second < 1000) {
			values->second = 1000;
		}
		return false;
	}
	//操作用户是自己
	if (ThisChairId == currentUser_) {
	}
	//操作用户是别人
	else {
	}
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		//操作用户弃牌 ///
		if (rspdata.giveupuser() == currentUser_) {
			if (rspdata.has_nextstep()) {
				//安全断言
				//assert(rspdata.has_nextstep());
				//更新操作用户
				assert(currentUser_ != rspdata.nextstep().nextuser());
				uint32_t currentUserT = currentUser_;
				currentUser_ = rspdata.nextstep().nextuser();
				//开始新的一轮
				if (currentTurn_ + 1 != rspdata.nextstep().currentturn()) {
					//本轮结束用户
					turninfo_[currentTurn_].enduser = currentUserT;
					//更新本轮计数
					++currentTurn_;
					assert(currentTurn_ + 1 == rspdata.nextstep().currentturn());
					//本轮开始用户
					turninfo_[currentTurn_].startuser = currentUser_;
					//本轮是否有加注过
					turninfo_[currentTurn_].addflag = false;
				}
				if (ThisChairId == currentUser_) {
					//等待机器人操作
					waitingOperate();
				}
			}
		}
		//非操作用户弃牌 ///
		else {
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//安全断言
		assert(!rspdata.has_nextstep());
		assert(!isGiveup_[ThisChairId]);
		assert(!isComparedLost_[ThisChairId]);
		//权重增加2
		values->second += 2;
	}
    return true;
}

//跟注/加注结果 - 服务端回复 ///
bool CAndroidUserItemSink::resultFollowAdd(const void * pBuffer, int32_t wDataSize)
{
	zjh::CMD_S_AddScore rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	//必须是操作用户
	assert(rspdata.opuser() == currentUser_);
	//当前第几轮
	assert(currentTurn_ + 1 == rspdata.currentturn());
	//安全断言
	assert(!isGiveup_[currentUser_]);
	assert(!isComparedLost_[currentUser_]);
	assert(m_pTableFrame->IsExistUser(currentUser_));
	//跟注(OP_FOLLOW=3)
	if (rspdata.opvalue() == OP_FOLLOW) {
		assert(rspdata.index() == -1);
		//当前筹码值
        //assert(jettonValue_ == rspdata.jettonvalue());
		//基础跟注分
        //assert(followScore_ == rspdata.followscore());
        LOG(WARNING) << " --- *** [" << strRoundID_ << "] resultFollowAdd 跟注 当前基础跟注分: jettonValue_["
        << jettonValue_ << "] followScore_[" << followScore_ << "] rspdata.jettonvalue()["
        << rspdata.jettonvalue() << "] rspdata.followscore()[" << rspdata.followscore() << "] --- *** ";
        if (jettonValue_ != rspdata.jettonvalue())
        {
            jettonValue_ = rspdata.jettonvalue();
        }
        if (followScore_ != rspdata.followscore())
        {
            followScore_ = rspdata.followscore();
        }
		//当前跟注分
		//int64_t currentFollowScore = rspdata.addscore();
		//assert(currentFollowScore = CurrentFollowScore(currentUser_));
		//玩家桌面分
		//tableScore_[currentUser_] += currentFollowScore;
		tableScore_[currentUser_] = rspdata.tablescore();
		//桌面总下注
		//tableAllScore_ += currentFollowScore;
		tableAllScore_ = rspdata.tableallscore();
	}
	//加注(OP_ADD=4)
	else {
		assert(rspdata.index() > 0);
		int start = GetCurrentAddStart();
		assert(start > 0);
		//更新当前筹码值jettonValue_ //////
		//更新基础跟注分followScore_ //////
		selectAddIndex(start, rspdata.index());
		//当前筹码值
        //assert(jettonValue_ == rspdata.jettonvalue());
		//基础跟注分
        //assert(followScore_ == rspdata.followscore());
        LOG(WARNING) << " --- *** [" << strRoundID_ << "] resultFollowAdd 加注 更新基础跟注分: jettonValue_["
        << jettonValue_ << "] followScore_[" << followScore_ << "] rspdata.jettonvalue()["
        << rspdata.jettonvalue() << "] rspdata.followscore()[" << rspdata.followscore() << "] --- *** ";
        //当前筹码值
        // assert(jettonValue_ == rspdata.jettonvalue());
        //基础跟注分
        // assert(followScore_ == rspdata.followscore());
        if (jettonValue_ != rspdata.jettonvalue())
        {
            jettonValue_ = rspdata.jettonvalue();
        }
        if (followScore_ != rspdata.followscore())
        {
            followScore_ = rspdata.followscore();
        }
		//当前加注分
		//int64_t currentAddScore = rspdata.addscore();
		//assert(currentAddScore == CurrentAddScore(currentUser_));
		//玩家桌面分
		//tableScore_[currentUser_] += currentAddScore;
		tableScore_[currentUser_] = rspdata.tablescore();
		//桌面总下注
		//tableAllScore_ += currentAddScore;
		tableAllScore_ = rspdata.tableallscore();
		//本轮是否有加注过
		turninfo_[currentTurn_].addflag = true;
	}
	//玩家桌面分
	//assert(tableScore_[currentUser_] == rspdata.tablescore());
	//桌面总下注
	//assert(tableAllScore_ == rspdata.tableallscore());
	//操作用户是自己
	if (ThisChairId == currentUser_) {
	}
	//操作用户是别人
	else {
	}
	if (rspdata.has_nextstep()) {
		//安全断言
		//assert(rspdata.has_nextstep());
		//更新操作用户
		assert(currentUser_ != rspdata.nextstep().nextuser());
		uint32_t currentUserT = currentUser_;
		currentUser_ = rspdata.nextstep().nextuser();
		//开始新的一轮
		if (currentTurn_ + 1 != rspdata.nextstep().currentturn()) {
			//本轮结束用户
			turninfo_[currentTurn_].enduser = currentUserT;
			//更新本轮计数
			++currentTurn_;
			assert(currentTurn_ + 1 == rspdata.nextstep().currentturn());
			//本轮开始用户
			turninfo_[currentTurn_].startuser = currentUser_;
			//本轮是否有加注过
			turninfo_[currentTurn_].addflag = false;
		}
		//等待机器人操作
		waitingOperate();
	}
    return true;
}

//比牌结果 - 服务端回复 ///
bool CAndroidUserItemSink::resultCompareCard(const void * pBuffer, int32_t wDataSize)
{
	zjh::CMD_S_CompareCard rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	//必须是操作用户
	//assert(rspdata.compareuser() == currentUser_);
	//当前第几轮
	assert(currentTurn_ + 1 == rspdata.currentturn());
	//安全断言
	assert(!isGiveup_[rspdata.compareuser()]);
	assert(!isComparedLost_[rspdata.compareuser()]);
	assert(m_pTableFrame->IsExistUser(rspdata.compareuser()));
	assert(!isGiveup_[rspdata.passiveuser()]);
	assert(!isComparedLost_[rspdata.passiveuser()]);
	assert(m_pTableFrame->IsExistUser(rspdata.passiveuser()));
	//比牌发起者输了
	if (rspdata.iscomparedlost()) {
		//标记输了
		isComparedLost_[rspdata.compareuser()] = true;
	}
	//被动比牌者输了
	else {
		//标记输了
		isComparedLost_[rspdata.passiveuser()] = true;
	}
	//强制比牌，无比牌加注分
	if (rspdata.has_comparescore() || rspdata.comparescore() > 0) {
		//比牌加注分
		//int64_t currentCompareScore = CurrentCompareScore(rspdata.compareuser());
		//assert(currentCompareScore == rspdata.comparescore());
		//玩家桌面分
		//tableScore_[rspdata.compareuser()] += currentCompareScore;
		//assert(tableScore_[rspdata.compareuser()] == rspdata.tablescore());
		tableScore_[rspdata.compareuser()] = rspdata.tablescore();
		//桌面总下注
		//tableAllScore_ += currentCompareScore;
		//assert(tableAllScore_ == rspdata.tableallscore());
		tableAllScore_ = rspdata.tableallscore();
	}
	//胜率/权重
	std::pair<double, double>* values = GetWinRatioAndWeight();
	assert(values != NULL);
	//自己是比牌发起者
	if (rspdata.compareuser() == ThisChairId) {
		//比牌输了
		if (isComparedLost_[ThisChairId]) {
			//被动比牌者是玩家
			if (!m_pTableFrame->IsAndroidUser(rspdata.passiveuser())) {
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
		//比牌赢了
		else {
			//被动比牌者是玩家
			if (!m_pTableFrame->IsAndroidUser(rspdata.passiveuser())) {
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
	//自己是被动比牌者
	else if(rspdata.passiveuser() == ThisChairId){
		//比牌输了
		if (isComparedLost_[ThisChairId]) {
			//比牌发起者是玩家
			if (!m_pTableFrame->IsAndroidUser(rspdata.compareuser())) {
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
		//比牌赢了
		else {
			//比牌发起者是玩家
			if (!m_pTableFrame->IsAndroidUser(rspdata.compareuser())) {
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
#if 0
	{
		//当前最大牌用户
		if (isComparedLost_[currentWinUser_]) {
			EstimateWinner(EstimateAll);
		}
		//真实玩家中最大牌用户
		if (realWinUser_ != INVALID_CHAIR && isComparedLost_[realWinUser_]) {
			EstimateWinner(EstimatePlayer);
		}
		//机器人中最大牌用户
		if (androidWinUser_ != INVALID_CHAIR && isComparedLost_[androidWinUser_]) {
			EstimateWinner(EstimateAndroid);
		}
	}
#endif
	//自己比牌输了 //////
	if (isComparedLost_[ThisChairId]) {
		return false;
	}
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		if (rspdata.has_nextstep()) {
			//安全断言
			//assert(rspdata.has_nextstep());
			//必须是操作用户
			assert(rspdata.compareuser() == currentUser_);
			//更新操作用户
			assert(currentUser_ != rspdata.nextstep().nextuser());
			uint32_t currentUserT = currentUser_;
			currentUser_ = rspdata.nextstep().nextuser();
			//开始新的一轮
			if (currentTurn_ + 1 != rspdata.nextstep().currentturn()) {
				//本轮结束用户
				turninfo_[currentTurn_].enduser = currentUserT;
				//更新本轮计数
				++currentTurn_;
				assert(currentTurn_ + 1 == rspdata.nextstep().currentturn());
				//本轮开始用户
				turninfo_[currentTurn_].startuser = currentUser_;
				//本轮是否有加注过
				turninfo_[currentTurn_].addflag = false;
			}
			//等待机器人操作
			waitingOperate(TIME_GAME_COMPARE_DELAY, -1);
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//安全断言
		assert(!rspdata.has_nextstep());
	}
    return true;
}

//孤注一掷结果 - 服务端回复 ///
bool CAndroidUserItemSink::resultAllIn(const void * pBuffer, int32_t wDataSize) {
	zjh::CMD_S_AllIn rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		//currentUser_已不再更新了
		return true;
	}
	//必须是操作用户
	assert(rspdata.allinuser() == currentUser_);
	//当前第几轮
	//assert(currentTurn_ + 1 == rspdata.currentturn());
	//安全断言
	assert(!isGiveup_[currentUser_]);
	assert(!isComparedLost_[currentUser_]);
	assert(m_pTableFrame->IsExistUser(currentUser_));
	//孤注一掷玩家输了出局，其它用户不受影响 ///
	if (rspdata.iscomparedlost()) {
		//标记输了
		isComparedLost_[currentUser_] = true;
	}
	//孤注一掷玩家最终胜出，淘汰其他用户 ///
	else {
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (i == currentUser_) {
				continue;
			}
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				continue;
			}
			//其他用户输了出局 ///
			isComparedLost_[i] = true;
		}
	}
	//剩余积分，数据反馈过来之前，用户积分已更新入库，不应该从ScoreByChairId获取
	//int64_t currentLeftScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
	//assert(currentLeftScore >= 0);
	//玩家桌面分
	//tableScore_[currentUser_] += currentLeftScore;
	//assert(tableScore_[currentUser_] == rspdata.tablescore());
	tableScore_[currentUser_] = rspdata.tablescore();
	//桌面总下注
	//tableAllScore_ += currentLeftScore;
	//assert(tableAllScore_ == rspdata.tableallscore());
	tableAllScore_ = rspdata.tableallscore();
	//当前积分 = 携带积分 - 累计下注
	//int64_t userScore = ScoreByChairId(currentUser_) - tableScore_[currentUser_];
	//assert(userScore == 0);
	//assert(userScore == rspdata.userscore());
	//操作用户是自己
	if (ThisChairId == currentUser_) {
	}
	//操作用户是别人
	else {
	}
	//胜率/权重
	//std::pair<double, double>* values = GetWinRatioAndWeight();
	//assert(values != NULL);
	//自己是孤注一掷玩家
	if (rspdata.allinuser() == ThisChairId) {
		assert(ThisChairId == currentUser_);
	}
	//别人发起孤注一掷
	else {
	}
#if 0
	{
		//当前最大牌用户
		if (isComparedLost_[currentWinUser_]) {
			EstimateWinner(EstimateAll);
		}
		//真实玩家中最大牌用户
		if (realWinUser_ != INVALID_CHAIR && isComparedLost_[realWinUser_]) {
			EstimateWinner(EstimatePlayer);
		}
		//机器人中最大牌用户
		if (androidWinUser_ != INVALID_CHAIR && isComparedLost_[androidWinUser_]) {
			EstimateWinner(EstimateAndroid);
		}
	}
#endif
	//自己输了 //////
	if (isComparedLost_[ThisChairId]) {
		return false;
	}
	//游戏人数超过2人，继续游戏 //////
	if (leftPlayerCount() >= 2) {
		if (rspdata.has_nextstep()) {
			//安全断言
			//assert(rspdata.has_nextstep());
			//更新操作用户
			assert(currentUser_ != rspdata.nextstep().nextuser());
			uint32_t currentUserT = currentUser_;
			currentUser_ = rspdata.nextstep().nextuser();
			//开始新的一轮
			if (currentTurn_ + 1 != rspdata.nextstep().currentturn()) {
				//本轮结束用户
				turninfo_[currentTurn_].enduser = currentUserT;
				//更新本轮计数
				++currentTurn_;
				assert(currentTurn_ + 1 == rspdata.nextstep().currentturn());
				//本轮开始用户
				turninfo_[currentTurn_].startuser = currentUser_;
				//本轮是否有加注过
				turninfo_[currentTurn_].addflag = false;
			}
			//等待机器人操作
			waitingOperate();
		}
	}
	//游戏人数仅剩1人，结束游戏 //////
	else {
		//安全断言
		assert(!rspdata.has_nextstep());
	}
	return true;
}

//游戏结束 - 服务端广播 ///
bool CAndroidUserItemSink::onGameEnd(const void * pBuffer, int32_t wDataSize)
{
	zjh::CMD_S_GameEnd rspdata;
	if (!rspdata.ParseFromArray(pBuffer, wDataSize)) {
		return false;
	}
    return true;
}

//推断当前最大牌用户 ///
void CAndroidUserItemSink::EstimateWinner(EstimateTy ty)
{
	switch (ty) {
	case EstimateAll: {//当前最大牌用户
		currentWinUser_ = INVALID_CHAIR;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				continue;
			}
			//初始化
			if (currentWinUser_ == INVALID_CHAIR) {
				currentWinUser_ = i;
				continue;
			}
#if 0
			//i
			std::string s1 = ZJH::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT);
			std::string t1 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[i]));
			//winner
			std::string s2 = ZJH::CGameLogic::StringCards(&(handCards_[currentWinUser_])[0], MAX_COUNT);
			std::string t2 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[currentWinUser_]));
			uint32_t j = currentWinUser_;
#endif
			//当前最大牌用户
			int result = ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[currentWinUser_])[0], special235_);
			if (result > 0) {
				currentWinUser_ = i;
			}
#if 0
			std::string cc = (result > 0) ? ">" : "<";
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%s][%s] %s [%d][%s]%s currentWinUser_ = [%d]",
				//座椅ID/手牌/牌型
				i, s1.c_str(), t1.c_str(),
				cc.c_str(),
				//座椅ID/手牌/牌型
				j, s2.c_str(), t2.c_str(), currentWinUser_);
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << msg;
#endif
		}
		break;
	}
	case EstimatePlayer: {//真实玩家中最大牌用户
		realWinUser_ = INVALID_CHAIR;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//排除机器人
			if (m_pTableFrame->IsAndroidUser(i)) {
				continue;
			}
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				continue;
			}
			//初始化
			if (realWinUser_ == INVALID_CHAIR) {
				realWinUser_ = i;
				continue;
			}
#if 0
			//i
			std::string s1 = ZJH::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT);
			std::string t1 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[i]));
			//winner
			std::string s2 = ZJH::CGameLogic::StringCards(&(handCards_[realWinUser_])[0], MAX_COUNT);
			std::string t2 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[realWinUser_]));
			uint32_t j = realWinUser_;
#endif
			//当前最大牌用户
			int result = ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[realWinUser_])[0], special235_);
			if (result > 0) {
				realWinUser_ = i;
			}
#if 0
			std::string cc = (result > 0) ? ">" : "<";
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%s][%s] %s [%d][%s]%s realWinUser_ = [%d]",
				//座椅ID/手牌/牌型
				i, s1.c_str(), t1.c_str(),
				cc.c_str(),
				//座椅ID/手牌/牌型
				j, s2.c_str(), t2.c_str(), realWinUser_);
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << msg;
#endif
		}
		break;
	}
	case EstimateAndroid: {//机器人中最大牌用户
		androidWinUser_ = INVALID_CHAIR;
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			//排除真实玩家
			if (!m_pTableFrame->IsAndroidUser(i)) {
				continue;
			}
			//用户已出局(弃牌/比牌输)
			if (isGiveup_[i] || isComparedLost_[i]) {
				continue;
			}
			//初始化
			if (androidWinUser_ == INVALID_CHAIR) {
				androidWinUser_ = i;
				continue;
			}
#if 0
			//i
			std::string s1 = ZJH::CGameLogic::StringCards(&(handCards_[i])[0], MAX_COUNT);
			std::string t1 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[i]));
			//winner
			std::string s2 = ZJH::CGameLogic::StringCards(&(handCards_[androidWinUser_])[0], MAX_COUNT);
			std::string t2 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[androidWinUser_]));
			uint32_t j = androidWinUser_;
#endif
			//当前最大牌用户
			int result = ZJH::CGameLogic::CompareHandCards(&(handCards_[i])[0], &(handCards_[androidWinUser_])[0], special235_);
			if (result > 0) {
				androidWinUser_ = i;
			}
#if 0
			std::string cc = (result > 0) ? ">" : "<";
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%s][%s] %s [%d][%s]%s androidWinUser_ = [%d]",
				//座椅ID/手牌/牌型
				i, s1.c_str(), t1.c_str(),
				cc.c_str(),
				//座椅ID/手牌/牌型
				j, s2.c_str(), t2.c_str(), androidWinUser_);
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << msg;
#endif
		}
		break;
	}
	}
}

//返回决策(0,1,2) ///
int CAndroidUserItemSink::GetSPIndex() {
	if (!useStrategy_) {
		return -1;
	}
	//随机概率值
	int value = 0;
	//决策(0,1,2)
	int SPIndex = 0;
	//收分阶段
	if (StockScore < StockLowLimit) {
		//收分时三套决策(0,1,2)几率
		value = RandomBetween(1, robot_.SP0[0] + robot_.SP0[1] + robot_.SP0[2]);
		if (value <= robot_.SP0[0]) {
			SPIndex = 0;
		}
		else if (value > robot_.SP0[0] && value <= (robot_.SP0[0] + robot_.SP0[1])) {
			SPIndex = 1;
		}
		else /*if (value > (robot_.SP0[0] + robot_.SP0[1]))*/ {
			SPIndex = 2;
		}
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 收分阶段 决策[" << SPIndex << "]...";
	}
	//放分阶段
	else if (StockScore > StockHighLimit) {
		//放分时三套决策(0,1,2)几率
		value = RandomBetween(1, robot_.SP2[0] + robot_.SP2[1] + robot_.SP2[2]);
		if (value <= robot_.SP2[0]) {
			SPIndex = 0;
		}
		else if (value > robot_.SP2[0] && value <= (robot_.SP2[0] + robot_.SP2[1])) {
			SPIndex = 1;
		}
		else /*if (value > (robot_.SP2[0] + robot_.SP2[1]))*/ {
			SPIndex = 2;
		}
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 放分阶段 决策[" << SPIndex << "]...";
	}
	//正常情况
	else {
		//正常时三套决策(0,1,2)几率
		value = RandomBetween(1, robot_.SP1[0] + robot_.SP1[1] + robot_.SP1[2]);
		if (value <= robot_.SP1[0]) {
			SPIndex = 0;
		}
		else if (value > robot_.SP1[0] && value <= (robot_.SP1[0] + robot_.SP1[1])) {
			SPIndex = 1;
		}
		else /*if (value > (robot_.SP1[0] + robot_.SP1[1]))*/ {
			SPIndex = 2;
		}
		LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 正常情况 决策[" << SPIndex << "]...";
	}
	return SPIndex;
}

//推断当前操作 ///
void CAndroidUserItemSink::EstimateOperate(int SPIndex) {
	//初始化
	currentOp_ = OP_INVALID;
	//随机概率值
	int value = 0;
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return;
	}
	//用户无效
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		return;
	}
	//概率看牌 //////
	do {
		//如果没有看牌，首轮不看牌 ///
		if (!isLooked_[ThisChairId]/* && currentTurn_ > 0*/) {
			//放大倍数
			int scale = robot_.scale;
			//看牌参数
			float K_;
			//启用三套决策(0,1,2)
			if (useStrategy_) {
				//收分阶段(决策0)
				if (SPIndex == 0) {
					K_ = range_K[0](rengine);//robot_.param[0].K_[0];
				}
				//放分阶段(决策2)
				else if (SPIndex == 2) {
					K_ = range_K[2](rengine);//robot_.param[2].K_[0];
				}
				//正常情况(决策1)
				else /*if (SPIndex == 1)*/ {
					K_ = range_K[1](rengine);//robot_.param[1].K_[0];
				}
			}
			else {
				//收分阶段
				if (StockScore < StockLowLimit) {
					K_ = range_K[0](rengine);//robot_.param[0].K_[0];
					LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 收分阶段...";
				}
				//放分阶段
				else if (StockScore > StockHighLimit) {
					K_ = range_K[2](rengine);//robot_.param[2].K_[0];
					LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 放分阶段...";
				}
				//正常情况
				else {
					K_ = range_K[1](rengine);//robot_.param[1].K_[0];
				}
			}
			LOG(WARNING) << " --- *** [" << strRoundID_ << "] ************** 看牌参数 K = " << K_;
			//随机概率值
			value = RandomBetween(1, 100 * scale);
			//场上有用户看牌，看牌概率*1.5 ///
			float rate = 1.0f;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (isLooked_[i]) {
					rate = 1.5f;
					break;
				}
			}
			//看牌基准概率 K*20%(预设)
			if (value < K_ * robot_.ratioKP[0] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*20%";
				break;
			}
			//本轮有加注操作，看牌概率 K*35%
			if (turninfo_[currentTurn_].addflag && !isLooked_[ThisChairId] && value <= K_ * robot_.ratioKP[1] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*35%";
				break;
			}
#if 0
			//本轮以机器人开始或结束，看牌概率 K*40%
			if (((turninfo_[currentTurn_].startuser != INVALID_CHAIR && isAndroidUser_[turninfo_[currentTurn_].startuser]) ||
				(turninfo_[currentTurn_].enduser != INVALID_CHAIR && isAndroidUser_[turninfo_[currentTurn_].enduser]))
				&& !isLooked_[ThisChairId] && value <= K_ * robot_.ratioKP[2] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*40%";
				break;
			}
#else
			//回合开始和结束是机器人自己，看牌概率 K*40%
			if (((turninfo_[currentTurn_].startuser != INVALID_CHAIR && turninfo_[currentTurn_].startuser == ThisChairId) ||
				(turninfo_[currentTurn_].enduser != INVALID_CHAIR && turninfo_[currentTurn_].enduser == ThisChairId))
				&& !isLooked_[ThisChairId] && value <= K_ * robot_.ratioKP[2] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*40%";
				break;
			}
#endif
		}
	} while (0);
	//概率诈唬 //////
	if (currentOp_ == OP_INVALID) {
		//小于等于最大牌为J的散牌
		if (handCardsType_[ThisChairId] == ZJH::Tysp &&
			ZJH::CGameLogic::GetCardPoint(ZJH::CGameLogic::MaxCard(&(handCards_[ThisChairId])[0])) <= ZJH::J) {
			//首轮没有加注操作，加注概率5%
			//int value = RandomBetween(1, 100);
			//if (currentTurn_ == 0 && !turninfo_[currentTurn_].addflag && value <= 5) {
				//加注
				//currentOp_ = OP_ADD;
				//标记诈唬过
				//assert(!bluffFlag_);
				//bluffFlag_ = true;
			//}
		}
		//诈唬过直接弃牌
		if (bluffFlag_) {
			//assert(currentOp_ == OP_INVALID);
			//currentOp_ = OP_GIVEUP;
		}
	}
	//当前最大牌用户
	EstimateWinner(EstimateAll);
	//真实玩家中最大牌用户
	EstimateWinner(EstimatePlayer);
	//机器人中最大牌用户
	EstimateWinner(EstimateAndroid);
	//概率加注/跟注/比牌/弃牌 //////
	if (currentOp_ == OP_INVALID) {
		//自己不是操作用户，还没有轮到自己
		//if (ThisChairId != currentUser_) {
		//	return;
		//}
		//自己是否最大牌
		bool selfBestCards = (currentWinUser_ == ThisChairId);
		//牌比玩家大
		bool selfBetterCards = true;
		assert(ThisChairId != realWinUser_);
		if (realWinUser_ != INVALID_CHAIR) {
			//ThisChairId
			std::string s1 = ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT);
			std::string t1 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId]));
			//realWinUser_
			std::string s2 = ZJH::CGameLogic::StringCards(&(handCards_[realWinUser_])[0], MAX_COUNT);
			std::string t2 = ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[realWinUser_]));
			uint32_t j = currentWinUser_;
			if (ZJH::CGameLogic::CompareHandCards(&(handCards_[ThisChairId])[0], &(handCards_[realWinUser_])[0], special235_) < 0) {
				//牌比玩家小
				selfBetterCards = false;
			}
			std::string cc = selfBetterCards ? ">" : "<";
			char msg[1024] = { 0 };
			snprintf(msg, sizeof(msg), "[%d][%s][%s] %s [%d][%s]%s %s",
				//座椅ID/手牌/牌型
				ThisChairId, s1.c_str(), t1.c_str(),
				cc.c_str(),
				//座椅ID/手牌/牌型
				realWinUser_, s2.c_str(), t2.c_str(), (selfBetterCards ? "牌比玩家大" : "牌比玩家小"));
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] " << msg;
		}
		std::pair<double, double>* values = GetWinRatioAndWeight();
		assert(values != NULL);
		//胜率
		double ratioWin = values->first;
		//权重
		double weight = values->second;
		//放大倍数
		int scale = robot_.scale;
		//启用三套决策(0,1,2)
		if (useStrategy_) {
			//收分阶段(决策0)，权值倍率
			if (SPIndex == 0) {
				//weight *= (selfBetterCards ? robot_.param[0].W1_ : robot_.param[0].W2_);
				weight *= (selfBetterCards ? range_W1[0](rengine) : range_W2[0](rengine));
			}
			//放分阶段(决策2)，权值倍率
			else if (SPIndex == 2) {
				//weight *= (selfBetterCards ? robot_.param[2].W1_ : robot_.param[2].W2_);
				weight *= (selfBetterCards ? range_W1[2](rengine) : range_W2[2](rengine));
			}
			//正常情况(决策1)，权值倍率
			else /*if (SPIndex == 1)*/ {
				//weight *= (selfBetterCards ? robot_.param[1].W1_ : robot_.param[1].W2_);
				weight *= (selfBetterCards ? range_W1[1](rengine) : range_W2[1](rengine));
			}
		}
		else {
			//收分阶段，权值倍率
			if (StockScore < StockLowLimit) {
				//weight *= (selfBetterCards ? robot_.param[0].W1_ : robot_.param[0].W2_);
				weight *= (selfBetterCards ? range_W1[0](rengine) : range_W2[0](rengine));
				LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 收分阶段...";
			}
			//放分阶段，权值倍率
			else if (StockScore > StockHighLimit) {
				//weight *= (selfBetterCards ? robot_.param[2].W1_ : robot_.param[2].W2_);
				weight *= (selfBetterCards ? range_W1[2](rengine) : range_W2[2](rengine));
				LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 放分阶段...";
			}
			//正常情况，权值倍率
			else {
				//weight *= (selfBetterCards ? robot_.param[1].W1_ : robot_.param[1].W2_);
				weight *= (selfBetterCards ? range_W1[1](rengine) : range_W2[1](rengine));
			}
		}
		value = RandomBetween(1, 100 * scale);
		//权值/1000>底池，加注/跟注
		if (((double)weight / 1000) > ((double)tableAllScore_ / CellScore)) {
			if ((double)value <= pow(((double)ratioWin / robot_.scale), 2) * 100 * scale) {
				//加注概率胜率^2
				currentOp_ = OP_ADD;
			}
			else {
				//跟注概率
				currentOp_ = OP_FOLLOW;
			}
		}
		//底池>=权值/1000，比牌/弃牌
		else {
			//assert(currentOp_ == OP_INVALID);
			//比牌需要投入筹码，比牌加注分
			int64_t jettons = CurrentCompareScore(ThisChairId);
			//LOG(WARNING) << " --- *** 比牌需要投入筹码 " << jettons;
			//修正参数(不知道什么jb玩意)
			double P;
			//启用三套决策(0,1,2)
			if (useStrategy_) {
				//收分阶段(决策0)
				if (SPIndex == 0) {
					P = (selfBetterCards ? robot_.param[0].P1_ : robot_.param[0].P2_);
				}
				//放分阶段(决策2)
				else if (SPIndex == 2) {
					P = (selfBetterCards ? robot_.param[2].P1_ : robot_.param[2].P2_);
				}
				//正常情况(决策1)
				else /*if (SPIndex == 1)*/ {
					P = (selfBetterCards ? robot_.param[1].P1_ : robot_.param[1].P2_);
				}
			}
			else {
				//收分阶段
				if (StockScore < StockLowLimit) {
					P = (selfBetterCards ? robot_.param[0].P1_ : robot_.param[0].P2_);
					LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 收分阶段...";
				}
				//放分阶段
				else if (StockScore > StockHighLimit) {
					P = (selfBetterCards ? robot_.param[2].P1_ : robot_.param[2].P2_);
					LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 放分阶段...";
				}
				//正常情况
				else {
					P = (selfBetterCards ? robot_.param[1].P1_ : robot_.param[1].P2_);
				}
			}
			//assert(!isZero(P));
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮" 
				<< "\nratioWin = " << ratioWin
				<< "\nweight = " << weight
				<< "\ntableAllScore_ = " << tableAllScore_
				<< "\nCellScore = " << CellScore
				<< "\njettons = " << jettons
				<< "\nratioWin = " << ratioWin
				<< "\nscale = " << robot_.scale
				<< "\nP = " << P
				<< "\nratioKP_0 = " << robot_.ratioKP[0] << " ratioKP_1 = " << robot_.ratioKP[1] << " ratioKP_2 = " << robot_.ratioKP[2]
				<< "\nK_0 = " << robot_.param[0].K_[0] << "|" << robot_.param[0].K_[1] << " W1_0 = " << robot_.param[0].W1_[0] << "|" << robot_.param[0].W1_[1] << " P1_0 = " << robot_.param[0].P1_ << " W2_0 = " << robot_.param[0].W2_[0] << "|" << robot_.param[0].W2_[1] << " P2_0 = " << robot_.param[0].P2_
				<< "\nK_1 = " << robot_.param[1].K_[0] << "|" << robot_.param[1].K_[1] << " W1_1 = " << robot_.param[1].W1_[0] << "|" << robot_.param[1].W1_[1] << " P1_1 = " << robot_.param[1].P1_ << " W2_1 = " << robot_.param[1].W2_[0] << "|" << robot_.param[1].W2_[1] << " P2_1 = " << robot_.param[1].P2_
				<< "\nK_2 = " << robot_.param[2].K_[0] << "|" << robot_.param[2].K_[1] << " W1_2 = " << robot_.param[2].W1_[0] << "|" << robot_.param[2].W1_[1] << " P1_2 = " << robot_.param[2].P1_ << " W2_2 = " << robot_.param[2].W2_[0] << "|" << robot_.param[2].W2_[1] << " P2_2 = " << robot_.param[2].P2_
				<< "\nleftPlayerCount = " << leftPlayerCount();
			//比牌概率 = min(底池/比牌需要投入的筹码*胜率^2*P/剩余人数, 1)
			double ratioCompare = std::min<double>(
				(double)tableAllScore_ / ((double)jettons) * pow(((double)ratioWin / robot_.scale), 2)*P / (leftPlayerCount() - 1), 1);
			//assert(
			//	(double)tableAllScore_ / ((double)jettons) * pow(((double)ratioWin / robot_.scale), 2)*P / (leftPlayerCount() - 1) < 1);
			value = RandomBetween(1, 10000);
			if ((double)value <= ratioCompare * 10000) {
				//首轮不比牌 ///
				if (currentTurn_ == 0) {
					//弃牌
					currentOp_ = OP_GIVEUP;
 					LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
						<< " " << ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT) << " ["
 						<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId])) << "] 比牌概率 "
 						<< (double)tableAllScore_ / ((double)jettons) * pow(((double)ratioWin / robot_.scale), 2)*P / (leftPlayerCount() - 1)
 						<< "x10000 = " << ratioCompare * 10000 << " <<"
 						<< StringOpCode(currentOp_) << ">>";
				}
				else {
					//比牌概率
					currentOp_ = OP_COMPARE;
 					LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
 						<< " " << ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT) << " ["
 						<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId])) << "] 比牌概率 "
 						<< (double)tableAllScore_ / ((double)jettons) * pow(((double)ratioWin / robot_.scale), 2)*P / (leftPlayerCount() - 1)
 						<< "x10000 = " << ratioCompare * 10000 << " <<"
 						<< StringOpCode(currentOp_) << ">>";
				}
			}
			else {
				//弃牌
				currentOp_ = OP_GIVEUP;
 				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT) << " ["
 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId])) << "] 比牌概率 "
 					<< (double)tableAllScore_ / ((double)jettons) * pow(((double)ratioWin / robot_.scale), 2)*P / (leftPlayerCount() - 1)
 					<< "x10000 = " << ratioCompare * 10000 << " <<"
 					<< StringOpCode(currentOp_) << ">>";
			}
		}
	}
	//验证看牌操作是否合法
	if (currentOp_ == OP_LOOKOVER) {
		//assert(!isLooked_[ThisChairId]);
		//assert(!isGiveup_[ThisChairId]);
	}
	//验证弃牌操作是否合法
	else if (currentOp_ == OP_GIVEUP) {
		//assert(!isGiveup_[ThisChairId]);
	}
	//不能加注则跟注
	else if (currentOp_ == OP_ADD && ThisChairId == currentUser_) {
		int start = GetCurrentAddStart();
		if (start > 0) {
		}
		else {
			currentOp_ = OP_FOLLOW;
		}
	}
	//操作有效
	assert(currentOp_ != OP_INVALID);
	//assert(!isComparedLost_[ThisChairId]);
	//测试孤注一掷/超过最大轮数强制比牌
	//if (currentOp_ > 0) {
	//	currentOp_ = OP_COMPARE;
	//}
}

//等待机器人操作 ///
void CAndroidUserItemSink::waitingOperate(int32_t delay, int32_t wTimeLeft)
{
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return;
	}
	//用户无效
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return;
	}
	//用户已出局(弃牌或比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		return;
	}
	//决策(0,1,2)
	SPIndex_ = GetSPIndex();
	//推断当前操作
	EstimateOperate_New(SPIndex_);
	do {
		//自己不是操作用户，还没有轮到自己
		if (ThisChairId != currentUser_) {
			//随时可以看牌或弃牌 //////
			if ((currentOp_ == OP_LOOKOVER && freeLookover_) ||
				(currentOp_ == OP_GIVEUP && freeGiveup_)) {
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[R] " << ThisChairId
					<< " [" << StringOpCode(currentOp_) << "] "
					<< (m_pTableFrame->IsAndroidUser(currentUser_) ? " 机器人[C] " : " 玩家[C] ") << currentUser_;
				break;
			}
			return;
		}
	} while (0);
	//是否已重置等待时长
	//randWaitResetFlag_ = false;
	//累计等待时长
	//totalWaitSeconds_ = 0;
	//随机等待时长
	randWaitSeconds_ = randWaitSeconds(ThisChairId, delay, wTimeLeft);
	//开启等待定时器
	timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(randWaitSeconds_, boost::bind(&CAndroidUserItemSink::onTimerWaitingOver, this));
}

//剩余游戏人数 ///
int CAndroidUserItemSink::leftPlayerCount(bool includeAndroid, uint32_t* currentUser) {
	int c = 0;
	for (int i = 0; i < GAME_PLAYER; ++i) {
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

//返回有效用户 ///
uint32_t CAndroidUserItemSink::GetNextUser(uint32_t startUser, bool ignoreCurrentUser) {
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

#if 0
//随机等待时间 ///
double CAndroidUserItemSink::randWaitSeconds(int chairId, int32_t delay, int32_t wTimeLeft) {
	assert(currentOp_ != OP_INVALID);
	//计算浮动时长
	int j = RandomBetween(0, 2);
	//中间间隔时长
	double interval = 1.0f;
	//自己不是操作用户，还没有轮到自己
	if (ThisChairId != currentUser_) {
		//随时可以看牌或弃牌 //////
		if (currentOp_ == OP_LOOKOVER) {
			if (currentTurn_ == 0) {
                j = RandomBetween(1, 3);
			}
			else {
                j = RandomBetween(1, 3);
			}
			interval = 0.6f;
		}
		else if(currentOp_ == OP_GIVEUP && isLooked_[chairId]) {
			if (currentTurn_ == 0) {
				interval = 0.4f;
				j = RandomBetween(1, 3);
			}
			else {
				interval = 0.4f;
				j = RandomBetween(1, 3);
			}
		}
	}
	//跟注操作
	if (currentOp_ == OP_FOLLOW) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 85) {
			interval = 0;
		}
	}
	//加注操作
	else if (currentOp_ == OP_ADD) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 80) {
			interval = 0.4f;
		}
		else {
			interval = 0.6f;
		}
		j = RandomBetween(2, 3);
	}
	//看牌操作
	else if (currentOp_ == OP_LOOKOVER) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 75) {
			interval = 0;
		}
	}
	//弃牌操作
	else if (currentOp_ == OP_GIVEUP && isLooked_[chairId]) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 55) {
			interval = 0.4f;
			j = RandomBetween(1, 3);
		}
		else {
			interval = 0.4f;
			j = RandomBetween(1, 3);
		}
	}
	//防止返回随机时长等于0情况
	if (delay <= 0 && interval == 0 && j == 0) {
		interval = sliceWaitSeconds_;
		j = RandomBetween(1, 2);
	}
	//比牌动画延迟时长 + 中间间隔时长 + 浮动时长
	double d = (double)delay + interval + sliceWaitSeconds_ * RandomBetween(1, 5) * j;
	assert(d > 0);
	return d;
}
#else
//随机等待时间 ///
double CAndroidUserItemSink::randWaitSeconds(int chairId, int32_t delay, int32_t wTimeLeft) {
	assert(currentOp_ != OP_INVALID);
	//计算浮动时长
	int j = RandomBetween(0, 1);
	//中间间隔时长
	double interval = 1.0f;
	//跟注操作
	if (currentOp_ == OP_FOLLOW) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 85) {
			interval = 0;
		}
	}
	//加注操作
	else if (currentOp_ == OP_ADD) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 80) {
			interval = 0.4f;
		}
		else {
			interval = 0.6f;
		}
		j = RandomBetween(0, 1);
	}
	//看牌操作
	else if (currentOp_ == OP_LOOKOVER) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 75) {
			interval = 0;
		}
	}
	//弃牌操作
	else if (currentOp_ == OP_GIVEUP && isLooked_[chairId]) {
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 75) {
			interval = 0;
		}
	}
	//自己不是操作用户，还没有轮到自己
	if (ThisChairId != currentUser_) {
		//随时可以看牌或弃牌 //////
		if (currentOp_ == OP_LOOKOVER) {
			if (currentTurn_ == 0) {
				j = RandomBetween(1, 2);
			}
			else {
				j = RandomBetween(1, 2);
			}
			interval = 0.6f;
		}
		else if (currentOp_ == OP_GIVEUP) {
			if (currentTurn_ == 0) {
				j = RandomBetween(1, 2);
			}
			else {
				j = RandomBetween(1, 2);
			}
			interval = 0.4f;
		}
	}
	//防止返回随机时长等于0情况
	if (delay <= 0 && interval == 0 && j == 0) {
		interval = sliceWaitSeconds_;
		j = 1;//RandomBetween(1, 2);
	}
	//比牌动画延迟时长 + 中间间隔时长 + 浮动时长
	double d = (double)delay + interval + sliceWaitSeconds_ * RandomBetween(1, 5) * j + 0.5f;
	assert(d > 0);
	return d;
}
#endif

//等待操作定时器 ///
void CAndroidUserItemSink::onTimerWaitingOver() {
	//删除定时器
	m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return;
	}
	//用户无效
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		return;
	}
	//当前操作无效
	if (currentOp_ == OP_INVALID) {
		return;
	}
	if (currentUser_ == ThisChairId)
		LOG(ERROR) << "--- *** [" << strRoundID_ << "] " << "===== 当前玩家 等待操作定时器结束 " << currentUser_ << " 选择的操作 <<" << StringOpCode(currentOp_) << ">>";
	do {
		//自己不是操作用户，还没有轮到自己
		if (ThisChairId != currentUser_) {
			//随时可以看牌或弃牌 //////
			if ((currentOp_ == OP_LOOKOVER && freeLookover_) ||
				(currentOp_ == OP_GIVEUP && freeGiveup_)) {
				break;
			}
			return;
		}
	} while (0);
	//totalWaitSeconds_ += sliceWaitSeconds_;
	//等待时长不够
	//if (totalWaitSeconds_ <= randWaitSeconds_) {
		//当前操作用户操作完成，减少AI等待时间
		//if (!randWaitResetFlag_ && isLooked_[ThisChairId]) {
		//	//随机等待时长
		//	resetWaitSeconds_ = randWaitSeconds(ThisChairId);
		//	//累计等待时长 + 重置等待时长
		//	randWaitSeconds_ = totalWaitSeconds_ + resetWaitSeconds_;
		//	//重置随机等待时长
		//	randWaitResetFlag_ = true;
		//}
		//重启等待定时器
		//timerIdWaiting_ = m_TimerLoopThread->getLoop()->runAfter(sliceWaitSeconds_, boost::bind(&CAndroidUserItemSink::onTimerWaitingOver, this));
	//}
	//else {
		//当前最大牌用户
		if (((currentWinUser_ != INVALID_CHAIR) && (isGiveup_[currentWinUser_] || isComparedLost_[currentWinUser_])) ||
			//真实玩家中最大牌用户
			((realWinUser_ != INVALID_CHAIR) && (isGiveup_[realWinUser_] || isComparedLost_[realWinUser_])) ||
			//机器人中最大牌用户
			((androidWinUser_ != INVALID_CHAIR) && (isGiveup_[androidWinUser_] || isComparedLost_[androidWinUser_]))) {
			//推断当前操作
			EstimateOperate_New(SPIndex_);			
			do {
				//自己不是操作用户，还没有轮到自己
				if (ThisChairId != currentUser_) {
					//随时可以看牌或弃牌 //////
					if ((currentOp_ == OP_LOOKOVER && freeLookover_) ||
						(currentOp_ == OP_GIVEUP && freeGiveup_)) {
						break;
					}
					return;
				}
			} while (0);
		}
		//当前最大牌用户无效
		if (currentWinUser_ == INVALID_CHAIR ||
			!m_pTableFrame->IsExistUser(currentWinUser_)) {
			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 currentWinUser_ = "
				<< currentWinUser_ << "IsExistUser = " << ((currentWinUser_ != INVALID_CHAIR) ? m_pTableFrame->IsExistUser(currentWinUser_) : 0);
			return;
		}
		//自己是否最大牌
		bool selfBestCards = (currentWinUser_ == ThisChairId);
		//玩家最大牌
		if (!m_pTableFrame->IsAndroidUser(currentWinUser_)) {
 			LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
 				<< " " << ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT) << " ["
 				<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId]))
 				<< "] <<" << StringOpCode(currentOp_) << ">> " << randWaitSeconds_ << "(s) 玩家 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 最大牌";
		}
		//机器人最大牌
		else {
			//自己最大牌
			if (selfBestCards) {
 				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT) << " ["
 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId]))
 					<< "] <<" << StringOpCode(currentOp_) << ">> " << randWaitSeconds_ << "(s) 机器人 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 最大牌";
			}
			//另外机器人最大牌
			else {
 				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId
 					<< " " << ZJH::CGameLogic::StringCards(&(handCards_[ThisChairId])[0], MAX_COUNT) << " ["
 					<< ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[ThisChairId]))
 					<< "] <<" << StringOpCode(currentOp_) << ">> " << randWaitSeconds_ << "(s) 机器人 [" << currentWinUser_ << "] " << UserIdBy(currentWinUser_) << " 最大牌";
			}
		}
		if (currentUser_ == ThisChairId)
			LOG(ERROR) << "--- *** [" << strRoundID_ << "] " << "===== 当前玩家 " << currentUser_ << " 选择的操作 <<" << StringOpCode(currentOp_) << ">>";
		//请求操作
		sendOperateReq();
	//}
}

//请求操作 ///
bool CAndroidUserItemSink::sendOperateReq()
{
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return false;
	}
	//用户无效
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return false;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		return false;
	}
    switch (currentOp_) {
	case OP_LOOKOVER: {		//请求看牌 ///
		sendLookCardReq();
		break;
	}
	case OP_GIVEUP: {		//请求弃牌 ///
		sendGiveupReq();
		break;
	}
	case OP_FOLLOW: {		//请求跟注 ///
		sendFollowReq();
		break;
	}
	case OP_ADD: {			//请求加注 ///
		sendAddReq();
		break;
	}
	case OP_COMPARE: {		//请求比牌 ///
		sendCompareCardReq();
		break;
	}
	default: assert(false); break;
    }
    return true;
}

//请求看牌 ///
void CAndroidUserItemSink::sendLookCardReq() {
	assert(currentOp_ = OP_LOOKOVER);
	assert(!isGiveup_[ThisChairId]);
	assert(!isComparedLost_[ThisChairId]);
	assert(!isLooked_[ThisChairId]);

	m_pTableFrame->OnGameEvent(ThisChairId, zjh::SUB_C_LOOK_CARD, NULL, 0);
}

//请求弃牌 ///
void CAndroidUserItemSink::sendGiveupReq() {
	assert(currentOp_ = OP_GIVEUP);
	assert(!isGiveup_[ThisChairId]);
	assert(!isComparedLost_[ThisChairId]);
	//没有看牌则先看牌
	if (!isLooked_[ThisChairId]) {
		currentOp_ = OP_LOOKOVER;
		sendLookCardReq();
		return;
	}
	m_pTableFrame->OnGameEvent(ThisChairId, zjh::SUB_C_GIVE_UP, NULL, 0);
}

//请求跟注 ///
void CAndroidUserItemSink::sendFollowReq()
{
	assert(currentOp_ = OP_FOLLOW);
	assert(!isGiveup_[ThisChairId]);
	assert(!isComparedLost_[ThisChairId]);
	//自己不是操作用户，还没有轮到自己
	if (ThisChairId != currentUser_) {
		return;
	}
	zjh::CMD_C_AddScore reqdata;
	//指定-1表示跟注操作
	reqdata.set_index(-1);
	std::string content = reqdata.SerializeAsString();
	m_pTableFrame->OnGameEvent(ThisChairId, zjh::SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
}

//请求加注 ///
void CAndroidUserItemSink::sendAddReq()
{
	assert(currentOp_ = OP_ADD);
	assert(!isGiveup_[ThisChairId]);
	assert(!isComparedLost_[ThisChairId]);
	//自己不是操作用户，还没有轮到自己
	if (ThisChairId != currentUser_) {
		return;
	}
	//基础最小加注分
	int64_t minaddscore = 0;
	//筹码表可加注筹码起始索引
	int start = GetAddChipStart(&minaddscore);//GetCurrentAddStart(&minaddscore);
	if (start > 0) {
		//随机可加注筹码索引
		int index = start;//RandomBetween(start, JettionList.size() - 1);
		//当前加注分
		int64_t currentAddScore = CurrentAddScore(ThisChairId);
		//比牌加注分
		//int64_t currentCompareScore = CurrentCompareScore(ThisChairId);
		//当前跟注分
		int64_t currentFollowScore = CurrentFollowScore(ThisChairId);
		//最小加注分
		int64_t currentMinAddScore = CurrentMinAddScore(ThisChairId, minaddscore);
		//最小加注分 > 当前跟注分
		assert(currentMinAddScore > currentFollowScore);
		//消息结构
		zjh::CMD_C_AddScore reqdata;
		//指定可加注筹码索引
		reqdata.set_index(index);
		std::string content = reqdata.SerializeAsString();
		m_pTableFrame->OnGameEvent(ThisChairId, zjh::SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
	}
	else {
		//不能加注操作，只能跟注/比牌/弃牌
		currentOp_ = OP_FOLLOW;
		sendFollowReq();
	}
}

//请求比牌 ///
void CAndroidUserItemSink::sendCompareCardReq()
{
	assert(currentOp_ == OP_COMPARE);
	assert(!isGiveup_[ThisChairId]);
	assert(!isComparedLost_[ThisChairId]);
	//自己不是操作用户，还没有轮到自己
	if (ThisChairId != currentUser_) {
		return;
	}
	//随机一个可比牌用户
	uint32_t nextUser = GetNextUser(RandomBetween(0, GAME_PLAYER - 1));
	if (nextUser != INVALID_CHAIR) {
		assert(leftPlayerCount() >= 2);
		assert(m_pTableFrame->IsExistUser(nextUser));
		zjh::CMD_C_CompareCard reqdata;
		reqdata.set_peeruser(nextUser);
		std::string content = reqdata.SerializeAsString();
		m_pTableFrame->OnGameEvent(ThisChairId, zjh::SUB_C_COMPARE_CARD, (uint8_t*)content.data(), content.size());//m_pTableFrame->OnGameEvent(ThisChairId, SUB_C_ALL_IN, NULL, 0);
	}
	else {
		//assert(leftPlayerCount() == 1);
		//没有可以比牌的用户
		for (int i = 0; i < GAME_PLAYER; ++i) {
			if (!m_pTableFrame->IsExistUser(i)) {
				continue;
			}
			if (isGiveup_[i]) {
				//LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(i) ? " 机器人 ":" 玩家 ") << i << " 弃牌了";
				continue;
			}
			if (isComparedLost_[i]) {
				//LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(i) ? " 机器人 " : " 玩家 ") << i << " 比牌输";
				continue;
			}
		}
		//LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 没有可比牌的用户...";
	}
}

//清理所有定时器
void CAndroidUserItemSink::ClearAllTimer()
{
    m_TimerLoopThread->getLoop()->cancel(timerIdWaiting_);
}

//推断当前操作 ///
void CAndroidUserItemSink::EstimateOperate_New(int SPIndex) {
	//初始化
	currentOp_ = OP_INVALID;
	//随机概率值
	int value = 0;
	//操作用户无效
	if (currentUser_ == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(currentUser_)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[currentUser_] || isComparedLost_[currentUser_]) {
		return;
	}
	//用户无效
	if (ThisChairId == INVALID_CHAIR ||
		!m_pTableFrame->IsExistUser(ThisChairId)) {
		return;
	}
	//用户已出局(弃牌/比牌输)
	if (isGiveup_[ThisChairId] || isComparedLost_[ThisChairId]) {
		return;
	}
	//概率看牌 //////
	do {
		//如果没有看牌，首轮不看牌 ///
		if (!isLooked_[ThisChairId]/* && currentTurn_ > 0*/) {
			//放大倍数
			int scale = robot_.scale;
			//看牌参数
			float K_;
			//启用三套决策(0,1,2)
			if (useStrategy_) {
				//收分阶段(决策0)
				if (SPIndex == 0) {
					K_ = range_K[0](rengine);//robot_.param[0].K_[0];
				}
				//放分阶段(决策2)
				else if (SPIndex == 2) {
					K_ = range_K[2](rengine);//robot_.param[2].K_[0];
				}
				//正常情况(决策1)
				else /*if (SPIndex == 1)*/ {
					K_ = range_K[1](rengine);//robot_.param[1].K_[0];
				}
			}
			else {
				//收分阶段
				if (StockScore < StockLowLimit) {
					K_ = range_K[0](rengine);//robot_.param[0].K_[0];
					LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 收分阶段...";
				}
				//放分阶段
				else if (StockScore > StockHighLimit) {
					K_ = range_K[2](rengine);//robot_.param[2].K_[0];
					LOG(WARNING) << " --- *** [" << strRoundID_ << "] ################## 放分阶段...";
				}
				//正常情况
				else {
					K_ = range_K[1](rengine);//robot_.param[1].K_[0];
				}
			}
			LOG(WARNING) << " --- *** [" << strRoundID_ << "] ************** 看牌参数 K = " << K_;
			//随机概率值
			value = RandomBetween(1, 100 * scale);
			//场上有用户看牌，看牌概率*1.5 ///
			float rate = 1.5f;
			for (int i = 0; i < GAME_PLAYER; ++i) {
				if (isLooked_[i]) {
					rate += 0.2f;
					break;
				}
			}
			//随着轮数增加,看牌概率增加
			if (currentTurn_>=3)
			{
				rate += 1.5f*currentTurn_/3;
			}
			//看牌基准概率 K*20%(预设)
			if (value < K_ * robot_.ratioKP[0] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*20%";
				break;
			}
			//本轮有加注操作，看牌概率 K*35%
			if (turninfo_[currentTurn_].addflag && !isLooked_[ThisChairId] && value <= K_ * robot_.ratioKP[1] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*35%";
				break;
			}
			//回合开始和结束是机器人自己，看牌概率 K*40%
			if (((turninfo_[currentTurn_].startuser != INVALID_CHAIR && turninfo_[currentTurn_].startuser == ThisChairId) ||
				(turninfo_[currentTurn_].enduser != INVALID_CHAIR && turninfo_[currentTurn_].enduser == ThisChairId))
				&& !isLooked_[ThisChairId] && value <= K_ * robot_.ratioKP[2] * rate * scale) {
				currentOp_ = OP_LOOKOVER;
				LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (ThisChairId == currentUser_ ? "C" : "R") << "] " << ThisChairId << " 看牌概率 K*40%";
				break;
			}
		}
	} while (0);
	
	//当前最大牌用户
	// EstimateWinner(EstimateAll);
	// //真实玩家中最大牌用户
	// EstimateWinner(EstimatePlayer);
	// //机器人中最大牌用户
	// EstimateWinner(EstimateAndroid);
	//概率加注/跟注/比牌/弃牌 
	if (currentOp_ == OP_INVALID) {
		calculateCurrentOperate(ThisChairId);
	}	
	//验证看牌操作是否合法
	if (currentOp_ == OP_LOOKOVER) {
		//assert(!isLooked_[ThisChairId]);
		//assert(!isGiveup_[ThisChairId]);
	}
	//验证弃牌操作是否合法
	else if (currentOp_ == OP_GIVEUP) {
		//assert(!isGiveup_[ThisChairId]);
	}
	//不能加注则跟注
	else if (currentOp_ == OP_ADD && ThisChairId == currentUser_) {
		int start = GetCurrentAddStart();
		if (start > 0) {
		}
		else {
			currentOp_ = OP_FOLLOW;
		}
	}
	//操作有效
	assert(currentOp_ != OP_INVALID);
	//assert(!isComparedLost_[ThisChairId]);
	//测试孤注一掷/超过最大轮数强制比牌
	//if (currentOp_ > 0) {
	//	currentOp_ = OP_COMPARE;
	//}
}
//计算当前操作【概率加注/跟注/比牌/弃牌】
void CAndroidUserItemSink::calculateCurrentOperate(uint32_t chairId)
{
	int value = m_random.betweenInt(1, 1000).randInt_mt(true);//RandomBetween(1, 1000);
	int currentProbability = 0;
	uint8_t CardsType = handCardsType_[chairId];	
	int cardtyIndex = CardsType;
	bool IsBigSp = false;
	bool IsBigDz = false;	
    string OperateStr;
	if (CardsType==ZJH::Tysp)
	{
		IsBigSp = IsBigSanCard(handCards_[chairId]);
		if (IsBigSp)
		{
			cardtyIndex = 7; //大散牌
		}
	}	
	else if (CardsType==ZJH::Ty20)
	{
		IsBigDz = IsBigDuizi(handCards_[chairId]);
		if (IsBigDz)
		{
			cardtyIndex = 8; //大对子
		}
	}	
	//概率弃牌
	currentProbability = robot_.ratioQiPai[cardtyIndex][currentTurn_];	
	//根据当前跟注值,调整弃牌概率
	int addIndexPro = 0;
	// if (turninfo_[currentTurn_].addflag)
	// {
		int addIndex = 0;
		for (int i = 0; i < JettionList.size(); ++i)
		{
			if (followScore_ == JettionList[i]) {
				addIndex = i;
				break;
			}
		}
		addIndexPro = currentProbability*robot_.ratioAddQiPaiPro[addIndex]/1000;
	// }
	if (!isLooked_[chairId]) //不看牌,不做弃牌操作
		value = 1000;
	// LOG(ERROR) << "===== 计算当前操作,currentUser_:" << currentUser_ << " chairId:" << chairId << " cardtyIndex:" << cardtyIndex << " 弃牌: [" << currentProbability << "] followScore_:" << followScore_ 
	// 		   << " addIndex:" << addIndex << " addIndexPro:" << addIndexPro << " [value:" << value << " <= Probability:" << (currentProbability+addIndexPro) << "]";
	currentProbability += addIndexPro;
	if (value<=currentProbability)
	{
		currentOp_ = OP_GIVEUP; //弃牌
		OperateStr = "弃牌";
	}
	else 
	{
		//概率加注
		value = m_random.betweenInt(1, 1000).randInt_mt(true);//RandomBetween(1, 1000);
		currentProbability = robot_.ratioJiaZhu[cardtyIndex][currentTurn_];
		if (value<=currentProbability)
		{
			int Index = GetCurrentJettionIndex();
			if (Index == JettionList.size() -1)
			{
                currentOp_ = OP_FOLLOW; //跟注
                OperateStr = "弃牌失败,选跟注";
			} 
			else
			{
				currentOp_ = OP_ADD; //加注
				OperateStr = "弃牌失败,选加注";
			}
		}
		else
		{
			//概率比牌
			if (currentTurn_ == 0)  //首轮不比牌			
			{
				currentOp_ = OP_FOLLOW; //跟注
				OperateStr = "首轮不比牌,跟注";
			}
			else
			{
				value = m_random.betweenInt(1, 1000).randInt_mt(true);//RandomBetween(1, 1000);
				currentProbability = robot_.ratioBiPai[cardtyIndex][currentTurn_];
				if (cardtyIndex==8) //大对子的牌型
				{
					addIndexPro = currentProbability*robot_.ratioAddQiPaiPro[addIndex]/1000; //根据下注等级调整比牌概率
					currentProbability += addIndexPro/2;
				}				
				if (value<=currentProbability)
				{
					currentOp_ = OP_COMPARE; //比牌
					OperateStr = "加注失败,选比牌";
				}
				else
				{
					currentOp_ = OP_FOLLOW; //跟注
					OperateStr = "比牌失败,选跟注";
				}
			}
		}
	}
	if (currentUser_ == chairId)
	{
		LOG(ERROR) << "选择的操作 [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 机器人[" << (chairId == currentUser_ ? "C" : "R") << "] " << "<<currentUser_:" << currentUser_ << ">>"
		<< " " << ZJH::CGameLogic::StringCards(&(handCards_[chairId])[0], MAX_COUNT) << " ["
        << ZJH::CGameLogic::StringHandTy(ZJH::HandTy(handCardsType_[chairId])) << "] " << OperateStr << " 概率 "  << value << " < " << currentProbability << " addIndexPro[" << addIndexPro << "] "
		<< " <<" << StringOpCode(currentOp_) << ">>" ;
	}
	
}
//是否大散牌
bool CAndroidUserItemSink::IsBigSanCard(uint8_t cards[]) {
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
bool CAndroidUserItemSink::IsBigDuizi(uint8_t cards[]) {
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
//获取可加注的Index
int CAndroidUserItemSink::getAddIndex(int start) {
	assert(start > 0);
	if(start >= JettionList.size())
	{
		return -1;
	}
	int index = start;
	int allweight = 0;
	int value = 0;//RandomBetween(1, 100);
	int currentProbability = 0;
	uint8_t CardsType = handCardsType_[currentUser_];
	int cardtyIndex = CardsType;
	bool IsBigSp = false;
	bool IsBigDz = false;		
	if (CardsType==ZJH::Tysp)
	{
		IsBigSp = IsBigSanCard(handCards_[currentUser_]);
		if (IsBigSp)
		{
			cardtyIndex = 7;
		}
	}	
	else if (CardsType==ZJH::Ty20)
	{
		IsBigDz = IsBigDuizi(handCards_[currentUser_]);
		if (IsBigDz)
		{
			cardtyIndex = 8;
		}
	}		
	for (int i = start; i < JettionList.size(); ++i)
	{
		allweight += robot_.ratioJiaZhuChip[cardtyIndex][i];
	}
	value = m_random.betweenInt(1, allweight).randInt_mt(true);//RandomBetween(1, allweight);
	for (int i = start; i < JettionList.size(); ++i)
	{
		value -= robot_.ratioJiaZhuChip[cardtyIndex][i];
		if (value <= 0) {
			index = i;
			break;
		}
	}
	// LOG(WARNING) << "--- *** [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " << (m_pTableFrame->IsAndroidUser(currentUser_) ? " 机器人 " : " 玩家 ") << currentUser_ << " 加注下标 " << index << " 加注筹码:" << JettionList[index];
	return index;
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
