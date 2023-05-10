#pragma once

//游戏消息缓存
struct GameMsgCache
{
	uint32_t chairid;
	uint8_t subid;
	uint8_t* data;
	uint32_t datasize;
};

enum ReplayOpType
{
	ReplayOpType_None,
	ReplayOpType_HandCards,
	ReplayOpType_ReplaceHua,
	ReplayOpType_AddCard,
	ReplayOpType_OutCard,
	ReplayOpType_Chi,
	ReplayOpType_Peng,
	ReplayOpType_MingGang,
	ReplayOpType_AnGang,
	ReplayOpType_Ting,
	ReplayOpType_Hu,
	ReplayOpType_NoHu, //流局
	ReplayOpType_Max
};

//游戏流程
class CGameProcess : public ITableFrameSink
{
public:
    CGameProcess(void);
    virtual ~CGameProcess(void);

public:
    //游戏开始
    virtual void OnGameStart() override;
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t dwChairID, uint8_t GETag) override;
    //发送场景
    virtual bool OnEventGameScene(uint32_t dwChairID, bool bIsLookUser) override;

    //事件接口
public:
    //游戏消息
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize) override;
private:
    bool onMsgOutCard(uint32_t chairid, const uint8_t* data, uint32_t datasize);
    bool onMsgChi(uint32_t chairid, const uint8_t* data, uint32_t datasize);
    bool onMsgPeng(uint32_t chairid, const uint8_t* data, uint32_t datasize);
    bool onMsgGang(uint32_t chairid, const uint8_t* data, uint32_t datasize);
    bool onMsgPass(uint32_t chairid, const uint8_t* data, uint32_t datasize);
    bool onMsgTing(uint32_t chairid, const uint8_t* data, uint32_t datasize);
	bool onMsgHu(uint32_t chairid, const uint8_t* data, uint32_t datasize);
	bool onMsgHosting(uint32_t chairid, const uint8_t* data, uint32_t datasize);
    //用户接口
public:
    //用户进入
    virtual bool OnUserEnter(int64_t dwUserID, bool bIsLookUser) override;
    //用户准备
    virtual bool OnUserReady(int64_t dwUserID, bool bIsLookUser) override;
    //用户离开
    virtual bool OnUserLeft(int64_t dwUserID, bool bIsLookUser) override;
    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser) override;
    virtual bool CanLeftTable(int64_t userId) override;

public:
    //设置指针
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;

private:
	bool Init();
    //清理游戏数据
    void ClearGameData();
    void UpdateGameRound(uint32_t nChairId, bool bAddHandCard);
    void OnTimerRound();
    void OnTimerOption();
    void OnTimerTianTing();
    bool OnOutCard(uint32_t chairid, int32_t card);
    bool OnHu(uint32_t nChairId);
	void OnHosting(uint32_t nChairId);
	void CancelHosting(uint32_t nChairId);
    bool ReplaceHua(uint32_t nChairId);
    void SetTargetInfo(int32_t card, TargetType type, int32_t chair);
    bool SendOption(uint32_t chair, int32_t updateOption);
    uint32_t GetCardNumInTable(uint32_t card);
    void CalculateScore(uint32_t winChair);
	bool IsGameMsgCacheExist(uint32_t chair);
	//发牌控制
	void DealHandCards();
	void DealHandCardsByCfg();
	int32_t DealCardToUser(bool isFront, uint32_t toChair);
	int32_t DealCardToUserNormal(bool isFront, uint32_t toChair);
	int32_t DealCardToUserControl(bool isFront, uint32_t toChair, bool isWinner);
    void ReduceCardWallFront();
    void ReduceCardWallTail();
	void CalculatePrInfo(vector<int32_t>& handCards, PrInfo& prInfo);
	void UpdateCtrlTypeByStorage();

    void WriteUserScore(uint32_t chairid);
private:
    tagGameRoomInfo* m_pGameRoomInfo;
    shared_ptr<ITableFrame> m_pTableFrame;
    shared_ptr<muduo::net::EventLoopThread> m_pLoopThread;
    shared_ptr<CGamePlayer> m_players[PLAYER_NUM];
    vector<int32_t> m_cards;
    std::string m_strRoundId;
    //庄家座位
    uint32_t m_nBankerChairId;
    //当前回合座位
    uint32_t m_nCurRoundChairId;
    //回合开始时间
    uint32_t m_nRoundStartTime;
    //回合定时器
    muduo::net::TimerId m_TimerRound;
    //选项开始时间
    uint32_t m_nOptionStartTime;
    //操作选项定时器
    muduo::net::TimerId m_TimerOption;
    //目标信息
    TargetInfo m_targetInfo;
    //闲家天听定时器
    muduo::net::TimerId m_TimerTianTing;
    bool m_tianTinging;
    //游戏操作计数[计：出牌，吃碰杠，摸牌(第一次摸牌为发完牌)] [1=游戏发完牌, 2=[庄家打第一张牌或庄家暗杠]]
    uint32_t m_gameOpCnt;
    //上一次操作是否为杠 [判断杠上开花]
	MjAction m_lastAction;
    uint32_t m_matchTime;
    chrono::system_clock::time_point  m_startTime;
    //跑马灯最小分值
    int64_t m_lMarqueeMinScore;
    //骰子
    uint32_t m_dices[2];
    //牌墙
    int32_t m_cardWall[2];
    //是否流局
    bool m_isGameFlow;
	//结算分数
	int64_t m_winScore;
	//开局回合
	bool m_startRound;
	//游戏控制类型
	ControlType m_ctrlType;
	//游戏消息缓存
	vector<GameMsgCache> m_gameMsgCache;
	//对局日志
	tagGameReplay m_replay;
	uint32_t m_startTimeForReplay;
	//-------------运营配置-----------
	bool m_testEnable;
	//赢家发好牌概率
	vector<int32_t> m_dealGoodPrsWin;
	//输家发好牌概率
	vector<int32_t> m_dealGoodPrsLose;
	float m_startAniTime;
	float m_roundTime;
	float m_optionTime;
	float m_hostingTime;
	float m_roundAfterTingTime;
	//-------------测试配置---------------------
	//手牌配置
	bool m_handCardsCfgEnable;
	vector<vector<int32_t>> m_handCardsCfg;
	//----------------------------------------测试功能-----------------------------------------//
	bool onMsgLeftCards(uint32_t chairid, const uint8_t* data, uint32_t datasize);
	bool onMsgReplaceCard(uint32_t chairid, const uint8_t* data, uint32_t datasize);
};
