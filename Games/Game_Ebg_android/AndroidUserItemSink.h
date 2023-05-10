#pragma once

//////////////////////////////////////////////////////////////////////////
static int  RandomInt(int a,int b)
{
    return rand() %(b-a+1)+ a;
}
//游戏对话框
class CAndroidUserItemSink : public IAndroidUserItemSink
{
	//控件变量
public:
    muduo::net::TimerId                           m_TimerJetton;
    muduo::net::TimerId                           m_TimerEnd;

    shared_ptr<ITableFrame>                    m_pITableFrame;                         // table frame item.
    shared_ptr<IServerUserItem>                m_pIAndroidUserItem;					//用户接口
    tagGameRoomInfo*                m_pGameRoomInfo;                        //配置房间
    string                         m_strRoomName;                          //配置房间

	//机器人存取款
    int64_t                           m_lRobotint64_tRange[2];					//最大范围
    int64_t                           m_lRobotBankGetint64_t;					//提款数额
    int64_t                           m_lRobotBankGetint64_tBanker;				//提款数额 (庄家)
	int								m_nRobotBankStorageMul;					//存款倍数
    //机器人行为控制
 public:
     int                          m_Areaprobability[3];//区域下注权重
     double                          m_AreaRobotAddGold[3];//游戏区域最多下注金额
     int                            m_probabilityWeight[5];//可下注百分比权重

     double                          m_RobotAddGold[3];//机器人龙虎和区域下注
     int                             m_TimeMinuteprobability[6];//退出概率
     time_t                          m_timerStart;
     time_t                          m_lasttimer;
     time_t                          m_endtimer;
     int                             m_Jetton;
     int                                count;
     double                         m_startsScore;
     time_t                         m_jionTime;
     int                            m_MaxWinTime;
     bool                           m_isShensuanzi;
     int                            m_hensuanziMaxArea;
     vector<int64_t>                  m_sendArea;

     int                            m_lianxuxiazhuCount;//连续下注同一个筹码次数
     double                         m_JettonScore;//上一轮下注筹码
     int                            m_kaishijiangeInit;//开始下注间隔
     int                            m_jiangeTimeInit;//每注间隔
     int                            m_end3s;//最后3秒下注概率
     uint64_t                       m_RoundJettonScore; //这轮下注金额
     uint64_t                       m_RoundJettonCount; //这轮下注次数
     uint32_t                       m_AndroidExitTimes; //机器人退出时间
     double                         m_AndroidJettonProb; //机器人押注比例>1000

     int                            m_ipalyTimes;
     int                            CountrandomRound;
     int64_t                        m_currentChips[5];
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
    virtual void * QueryInterface(const IID & Guid, int64_t dwQueryVer);
*/

	//控制接口
public:
	//初始接口
    // virtual bool Initialization(IUnknownEx * pIUnknownEx);
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)override;
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem) override;
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;
	//重置接口
    virtual bool RepositionSink()override;

	//游戏事件
public:
	//时间消息
    //virtual bool OnEventTimer(UINT nTimerID);
    //virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam) override;

    //游戏消息
    //virtual bool OnEventGameMessage(WORD wSubCmdID, void * pData, WORD wDataSize);
    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size)override;

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
    virtual void OnEventUserint64_t(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
	//用户状态
	virtual void OnEventUserStatus(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
	//用户段位
	virtual void OnEventUserSegment(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
*/
public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy){ m_strategy = strategy;  };
    tagAndroidStrategyParam* GetAndroidStrategy(){ return m_strategy;   };

protected:
    tagAndroidStrategyParam*            m_strategy;
	//消息处理
protected:

    //用户加注
    bool OnSubAddScore();
    //随机可下注金额比例
    int RandAddProbability();
    //随机下注筹码
    double RandChouMa(int64_t CanxiazuGold,int8_t GoldId);
    //随机下注区域
    int RandArea();
    void  openLog(const char *p, ...);
	//库存操作
private:
	//读取配置
    bool ReadConfigInformation();
	//银行操作
//	void BankOperate(BYTE cbType);

//	void NoWatchCardOperate();
//	bool WatchCardOperate();
//    void WinOperate(WORD wMeChairID, BYTE cbint64_tTimes, BYTE p1, BYTE p2, BYTE p3, BYTE p4, BYTE p5);
//    void FailOperate(WORD wMeChairID, BYTE cbint64_tTimes, BYTE p1, BYTE p2, BYTE p3, BYTE p4, BYTE p5, BYTE p6, BYTE p7);
//	bool ToWatchCard();
//	int GetNumRealPlayer();

    void OnTimerJetton();
    void OnTimerGameEnd();
	//逻辑辅助
protected:
	//推断胜者
//    WORD GetBit(WORD num, int pos);
//    WORD EstimateWinner();
};

//////////////////////////////////////////////////////////////////////////
