#ifndef ANDROID_USER_ITEM_SINK_HEAD_FILE
#define ANDROID_USER_ITEM_SINK_HEAD_FILE

//////////////////////////////////////////////////////////////////////////

//游戏对话框
class CAndroidUserItemSink : public IAndroidUserItemSink
{
    //函数定义
public:
	//构造函数
	CAndroidUserItemSink();
	//析构函数
	virtual ~CAndroidUserItemSink();

    /*
	//基础接口
public:
	//释放对象
	virtual VOID Release() { delete this; }
	//接口查询
	virtual void * QueryInterface(const IID & Guid, DWORD dwQueryVer);
*/

	//控制接口
public:
	//初始接口
    // virtual bool Initialization(IUnknownEx * pIUnknownEx);
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//重置接口
	virtual bool RepositionSink();

	//游戏事件
public:
	//时间消息
    //virtual bool OnEventTimer(UINT nTimerID);
//    virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam);
    void OnTimerJetton();
    void OnTimerGameEnd();

    //游戏消息
    //virtual bool OnEventGameMessage(WORD wSubCmdID, void * pData, WORD wDataSize);
    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size);

    //游戏消息
    //virtual bool OnEventFrameMessage(WORD wSubCmdID, void * pData, WORD wDataSize);

    //场景消息
    // virtual bool OnEventSceneMessage(BYTE cbGameStatus, bool bLookonOther, void * pData, WORD wDataSize);
    virtual bool OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData, uint32_t size);

    virtual bool CheckExitGame();
    virtual bool ExitGame();
    /*
    //用户事件
public:
	//用户进入
	virtual void OnEventUserEnter(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
	//用户离开
	virtual void OnEventUserLeave(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
	//用户积分
	virtual void OnEventUserScore(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
	//用户状态
	virtual void OnEventUserStatus(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
	//用户段位
	virtual void OnEventUserSegment(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
*/

	//消息处理
protected:

    //用户加注
    bool OnSubAddScore();
    //随机可下注金额比例
    int RandAddProbability();
    //随机下注筹码
    int64_t RandChouMa(int64_t CanxiazuGold,int8_t GoldId);
    //随机下注区域
    int RandArea();
//    //游戏结束
//    bool OnSubGameEnd(const void * pBuffer, uint32_t wDataSize);
//    void  openLog(const char *p, ...);
	//库存操作
private:
	//读取配置
    bool ReadConfigInformation();

//	//银行操作
//    void BankOperate(uint8_t cbType);

//	void NoWatchCardOperate();
//	bool WatchCardOperate();
//	void WinOperate(WORD wMeChairID, BYTE cbScoreTimes, BYTE p1, BYTE p2, BYTE p3, BYTE p4, BYTE p5);
//	void FailOperate(WORD wMeChairID, BYTE cbScoreTimes, BYTE p1, BYTE p2, BYTE p3, BYTE p4, BYTE p5, BYTE p6, BYTE p7);
//	bool ToWatchCard();
//	int GetNumRealPlayer();

	//逻辑辅助
protected:
//	//推断胜者
//    WORD GetBit(WORD num, int pos);
//    WORD EstimateWinner();
public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy){ m_strategy = strategy;  };
    tagAndroidStrategyParam* GetAndroidStrategy(){ return m_strategy;   };

protected:
    tagAndroidStrategyParam*            m_strategy;

private:
    //Landy::GameTimer                     m_TimerJetton;
    //Landy::GameTimer                     m_TimerEnd;
    muduo::net::TimerId                    m_TimerJettonId;
    muduo::net::TimerId                    m_TimerEndId;
    //控件变量
public:
    shared_ptr<ITableFrame>              m_pITableFrame;                         // table frame item.
    shared_ptr<IServerUserItem>          m_pIAndroidUserItem;					//用户接口

    tagGameRoomInfo*                     m_pGameRoomInfo;                        //配置房间
    string                               m_strRoomName;                          //配置房间

    //机器人存取款
//    SCORE                           m_lRobotScoreRange[2];					//最大范围
//    SCORE                           m_lRobotBankGetScore;					//提款数额
//    SCORE                           m_lRobotBankGetScoreBanker;				//提款数额 (庄家)
//    int								m_nRobotBankStorageMul;					//存款倍数

    //机器人行为控制
 public:
     uint32_t                           m_AreaProbability[6];//区域下注权重
     int64_t                            m_AreaRobotAddGold[6];//游戏区域最多下注金额
     uint32_t                           m_ProbabilityWeight[7];//可下注百分比权重
     uint32_t                           m_TimeMinuteProbability[6];//退出概率

//     double                          m_RobotAddGold[3];//机器人庄闲和区域下注

     uint32_t                           m_StartIntervalInit;//开始下注间隔
     uint32_t                           m_JettonIntervalInit;//每注间隔
     uint32_t                           m_End3s;//最后3秒下注概率

     time_t                             m_TimerStart;
     time_t                             m_Endtimer;
     time_t                             m_Lasttimer;

     uint32_t                           m_JettonTime;

//     int                            m_count;
     int64_t                            m_StartsScore;

     time_t                             m_JionTime;
     uint32_t                           m_MaxWinTime;

     bool                               m_IsShenSuanZi;
     uint32_t                           m_ShenSuanZiMaxArea;
     vector<uint64_t>                   m_SendArea;
	 uint32_t                           m_JettonScoreMaxCount; //下注筹码种类
     vector<uint64_t>                   m_SendJettonScore;
	 int32_t							m_JettonOneAreaProb;

     uint32_t                           m_ContinueJettonCount;//连续下注同一个筹码次数
     int64_t                            m_JettonScore;//上一轮下注筹码
     uint64_t                           m_RoundJettonScore; //这轮下注金额
     uint64_t                           m_RoundJettonCount; //这轮下注次数
     uint32_t                           m_AndroidExitTimes; //机器人退出时间
     double                             m_AndroidJettonProb; //机器人押注比例>1000

     uint64_t                            m_currentChips[7];
	 int32_t							m_cbGameStatus;							//游戏状态
};

//////////////////////////////////////////////////////////////////////////

#endif
