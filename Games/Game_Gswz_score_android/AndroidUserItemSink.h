#pragma once

enum Wait_Operate			//操作类型
{
    OP_ADD,                 //加注
    OP_FOLLOW,				//跟注
    OP_PASS,				//不加
    OP_ALLIN,				//梭哈
    OP_GIVE_UP,				//弃牌
    OP_MAX,
};
//不同库存下机器人操作概率
enum StockStatus
{
    Stock_Ceil,            //库存封顶
    Stock_Normal,
    Stock_Floor,
    Stock_Sataus_Count
};

class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
	CAndroidUserItemSink();
	virtual ~CAndroidUserItemSink();
private:
    void readIni();
public:
    virtual bool RepositionSink();
	//初始接口
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pAndroidUserItem);
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//游戏事件
public:
	//游戏事件
    virtual bool OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize);

    //消息处理
protected:
	//机器人消息
    bool OnSubAndroidCard(const void * pBuffer, int32_t wDataSize);
	//游戏开始
    bool OnSubGameStart(const void * pBuffer, int32_t wDataSize);
	//用户放弃
    bool OnSubGiveUp(const void * pBuffer, int32_t wDataSize);
	//用户加注
    bool OnSubAddScore(const void * pBuffer, int32_t wDataSize);
    //不加事件
    bool OnSubPassScore(const void * pBuffer, int32_t wDataSize);
    //发牌事件
    bool OnSubSendCard(const void * pBuffer, int32_t wDataSize);
    //跟注事件
    bool OnSubFollowScore(const void * pBuffer, int32_t wDataSize);
    //梭哈事件
    bool OnSubAllIn(const void * pBuffer, int32_t wDataSize);
	//游戏结束
    bool OnSubGameEnd(const void * pBuffer, int32_t wDataSize);
private:
    void GetLeftBufCards(vector<uint8_t>& cards);
    void GetMaxCards(vector<uint8_t>& buffcards,uint8_t cards[],uint8_t chairId);
    bool CalViewCardsMayWin();
	//逻辑辅助
protected:
    bool HandleAndroidAddScore();
    int32_t GetChairIdByUserId(int64_t userid);
    int32_t GetUserIdByChiarId(int32_t chairId);
    int64_t GetCurrentUserScore();
    int64_t GetMaxHandCardUser(); //推断胜者
    Wait_Operate GetWaitOperate(); //思考出这次的操作
    void JiaZhu();
public:
	bool ClearAllTimer();
    void  SC_JH_WAIT_OPT();
protected:
    void OnTimerAddScore();
    void OnTimerLookCard();
    void OnTimerRush(void* args);

public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy)
    { m_strategy = strategy;}
    tagAndroidStrategyParam* GetAndroidStrategy()
    { return m_strategy;   }

protected:
    tagAndroidStrategyParam*            m_strategy;

private:
	CGameLogic			m_GameLogic;								//游戏逻辑
    shared_ptr<ITableFrame> m_pTableFrame;							//桌子指针
    shared_ptr<IServerUserItem>	m_pAndroidUserItem;					//机器人对象
    int32_t				m_wChairID;									//机器人椅子ID
    int64_t             m_lUserId;                                  //机器人玩家ID

	//游戏变量
protected:
    int64_t				m_lStockScore;								//当前库存
    int64_t             m_llTodayHighLimit;                         //最高
    int64_t				m_wCurrentUser;								//当前用户
    uint8_t				m_cbCardType;								//用户牌型
    double              m_OperateTimes;                             //操作时间
	//加注信息
protected:
    int64_t				m_lCellScore;								//单元下注
    int64_t				m_lMaxCellScore;							//最大下注
    int64_t				m_lUserMaxScore;							//最大分数
    int64_t             m_MinAddScore;                              //加注最小金额
    int64_t				m_lMinJettonScore;							//当前倍数(其实就是当前的下注额度)
    int64_t				m_lTableScore[GAME_PLAYER];					//下注数目
    int32_t				m_cbHandCardCount;                          //手牌数量
	//用户状态
protected:
    uint8_t				m_cbPlayStatus[GAME_PLAYER];				//游戏状态
    uint8_t				m_cbRealPlayer[GAME_PLAYER];				//真人玩家
    uint8_t				m_cbAndroidStatus[GAME_PLAYER];				//机器玩家
    uint8_t				m_cbLookCardStatus[GAME_PLAYER];			//看牌状态
    int64_t				m_cbUserIds[GAME_PLAYER];                   //每个位置上的玩家ID
	//用户扑克
protected:
    uint8_t				m_cbHandCardData[MAX_COUNT];				//用户数据
    uint8_t				m_cbAllHandCardData[GAME_PLAYER][MAX_COUNT];//桌面扑克
    uint8_t				m_cbHandCardType[GAME_PLAYER];				//玩家的牌型
    uint8_t             m_cbJettonScroe[4];                         //固定注额
    vector<uint8_t>     m_canViewCard;                              //可以看到的牌

    int							m_nRandGenZhuNum;									//连续跟注次数
    int							m_nCurGenZhu;										//当前跟注次数
    int64_t                     m_currentMaxCardUser;
#define IDI_USER_ADD_SCORE				(101)								//
#define IDI_RUSH						(102)								//
#define IDI_OPEN_CARD					(103)								//


protected:
    muduo::net::TimerId             m_timerAddScore;                     //定时器
    muduo::net::TimerId             m_timerLookCard;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;                      //thread.

protected:
    Wait_Operate				m_DoOpt;											//机器人做这个操作
    bool						m_bIsComp;
private:
    float                       winAllInProb_[Stock_Sataus_Count];
    float                       winAddScoreProb_[Stock_Sataus_Count];
    float                       winReAddSoreProb_[Stock_Sataus_Count];  //反加注概率
    float                       winPassScoreProb_[Stock_Sataus_Count];
    float                       winFollowScoreProb_[Stock_Sataus_Count];
    float                       winGiveUpProb_[Stock_Sataus_Count];

    float                       lostAllInProb_[Stock_Sataus_Count];
    float                       lostAddScoreProb_[Stock_Sataus_Count];
    float                       lostReAddSoreProb_[Stock_Sataus_Count];  //反加注概率
    float                       lostPassScoreProb_[Stock_Sataus_Count];
    float                       lostFollowScoreProb_[Stock_Sataus_Count];
    float                       lostGiveUpProb_[Stock_Sataus_Count];

    int                         OperateTimeProb_[5];
};
