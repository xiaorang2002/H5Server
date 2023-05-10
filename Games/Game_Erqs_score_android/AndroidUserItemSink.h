#pragma once

class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
	CAndroidUserItemSink();
    virtual ~CAndroidUserItemSink();
public:
    virtual bool RepositionSink() override;
	//初始接口
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pAndroidUserItem) override;
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem) override;
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;
	//游戏事件
public:
	//游戏事件
    virtual bool OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize) override;

    virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy) override;
    virtual tagAndroidStrategyParam* GetAndroidStrategy() override;
private:
	bool Init();
	bool onAiLv(const uint8_t* pData, uint32_t wDataSize);
	bool onGameStart(const uint8_t* pData, uint32_t wDataSize);
	bool onReplaceHua(const uint8_t* pData, uint32_t wDataSize);
	bool onOutCard(const uint8_t* pData, uint32_t wDataSize);
	bool onChi(const uint8_t* pData, uint32_t wDataSize);
	bool onPeng(const uint8_t* pData, uint32_t wDataSize);
	bool onGang(const uint8_t* pData, uint32_t wDataSize);
	bool onTing(const uint8_t* pData, uint32_t wDataSize);
	bool onGameOptions(const uint8_t* pData, uint32_t wDataSize);
	bool onGameRound(const uint8_t* pData, uint32_t wDataSize);
    bool onGameEnd(const uint8_t*  pData, uint32_t wDataSize);
	void OnTimerOutCard();
	void OnTimerOption();

	//ai行为 [听牌前，听牌后]
	bool aiActionHu();
	bool aiActionTing();
	bool aiActionGang();
	bool aiActionPeng();
	bool aiActionChi();
	bool aiActionPass();
	//ai行为是否可执行
	bool aiActionExecutable(vector<int32_t>& handCardsAfterAction, MjType mjType);

	//计算胡牌概率信息
	void calculatePrInfo(vector<int32_t>& handCards, PrInfo& fastHuPrInfo);
	//正常输赢计算剩余牌，不包括不可见牌
	void calculateLeftTableNormal(array<int32_t, TABLE_SIZE>& leftTable);
	//系统赢计算剩余牌，包括不可见牌
	void calculateLeftTableSysWin(array<int32_t, TABLE_SIZE>& leftTable);
	//系统输计算剩余牌，包括不可见牌
	void calculateLeftTableSysLose(array<int32_t, TABLE_SIZE>& leftTable);
private:
    tagGameRoomInfo* m_pGameRoomInfo;
    shared_ptr<ITableFrame> m_pTableFrame;
    shared_ptr<muduo::net::EventLoopThread> m_pLoopThread;
	//此player无ServerUserItem 
	shared_ptr<CGamePlayer> m_players[PLAYER_NUM];
    shared_ptr<IServerUserItem> m_selfUser;
	int32_t m_selfChair;
    //出牌定时器
	muduo::net::TimerId m_timerOutCard;
	muduo::net::TimerId m_timerOption;
	//开局回合
	bool m_startRound;
	uint32_t m_curRoundChair;
	ControlType m_ctrlType;
	//配置
	float m_startAniTime;
	CWeight m_roundTimeWeight;
	CWeight m_optionTimeWeight;
};
