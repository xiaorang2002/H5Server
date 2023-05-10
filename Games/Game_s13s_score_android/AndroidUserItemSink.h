#pragma once

#define FloorScore		(m_pTableFrame->GetGameRoomInfo()->floorScore)
#define CeilScore		(m_pTableFrame->GetGameRoomInfo()->ceilScore)
#define CellScore		(FloorScore)

#define ThisTableId		(m_pTableFrame->GetTableId())
#define ThisRoomId		(m_pTableFrame->GetGameRoomInfo()->roomId)
#define ThisRoomName	(m_pTableFrame->GetGameRoomInfo()->roomName)
#define ThisThreadTimer	(m_pTableFrame->GetLoopThread()->getLoop())

#define ThisChairId		(m_pAndroidUserItem->GetChairId())
#define ThisUserId		(m_pAndroidUserItem->GetUserId())

class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
	CAndroidUserItemSink();
	virtual ~CAndroidUserItemSink();
public:
	//桌子重置
	virtual bool RepositionSink();
	//桌子初始化
	virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
	//用户指针
	virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
	//桌子指针
	virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//消息处理
	virtual bool OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize);
	//机器人策略
	virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy) { strategy_ = strategy; }
	virtual tagAndroidStrategyParam* GetAndroidStrategy() { return strategy_; }
protected:
	//随机思考时间
	double CalcWaitSeconds(int chairId, bool isinit = false);
	//随机思考时间(极速房)
	double CalcWaitSecondsSpeed(int chairId, bool isinit = false);
	//思考定时器
	void ThinkingTimerOver();
	//清理所有定时器
	void ClearAllTimer();
protected:
	//累计思考时长
	double totalWaitSeconds_;
	//分片思考时长
	double sliceWaitSeconds_;
	//随机思考时长(1.0~18.0s之间)
	double randWaitSeconds_;
	//所有真人玩家理牌完毕，重置机器人随机思考时长，
	//若机器人随机思考时间太长的话，可以缩短随机思考时长
	double resetWaitSeconds_;
	//是否重置随机思考时长
	bool randWaitResetFlag_;
private:
	//牌局编号
	std::string strRoundID_;
	//游戏逻辑
	S13S::CGameLogic g;
	//各个玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//玩家牌型分析结果
	S13S::CGameLogic::handinfo_t handInfos_[GAME_PLAYER];
	//理牌总的时间/理牌剩余时间
	uint32_t groupTotalTime_;
	//理牌结束时间
	uint32_t groupEndTime_;
	//确定牌型的真人玩家数
	int realPlayerCount;
	//桌子指针
	std::shared_ptr<ITableFrame> m_pTableFrame;
	//机器人对象
    shared_ptr<IServerUserItem>	m_pAndroidUserItem;
	//理牌定时器
	muduo::net::TimerId timerIdThinking;
	//定时器指针
	shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;
	//机器人策略
	tagAndroidStrategyParam* strategy_;
};
