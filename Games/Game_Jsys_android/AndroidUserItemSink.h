#ifndef ANDROID_USER_ITEM_SINK_HEAD_FILE
#define ANDROID_USER_ITEM_SINK_HEAD_FILE

#define BET_ITEM_COUNT (8+4)


//游戏对话框
class CAndroidUserItemSink : public IAndroidUserItemSink
{
	//函数定义
public:
	//构造函数
	CAndroidUserItemSink();
	//析构函数
	virtual ~CAndroidUserItemSink();
	//控制接口
public:
	//初始接口
	virtual bool Initialization(shared_ptr<ITableFrame> &pTableFrame, shared_ptr<IServerUserItem> &pUserItem) override;
	virtual bool SetUserItem(shared_ptr<IServerUserItem> &pUserItem) override;
	virtual void SetTableFrame(shared_ptr<ITableFrame> &pTableFrame) override;
	//重置接口
	virtual bool RepositionSink() override;

public:
	void SetAndroidStrategy(tagAndroidStrategyParam *strategy) { m_strategy = strategy; };
	tagAndroidStrategyParam *GetAndroidStrategy() { return m_strategy; };

protected:
	tagAndroidStrategyParam *m_strategy;

	//游戏事件
public:
	//时间消息
	void OnTimerJetton();
	void OnTimerGameEnd();
	void OnTimerJettonOld();
	//游戏消息
	virtual bool OnGameMessage(uint8_t subId, const uint8_t *pData, uint32_t size);
	//场景消息
	virtual bool OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t *pData, uint32_t size);

	virtual bool CheckExitGame();
	virtual bool ExitGame();

protected:
	//用户加注
	bool OnSubAddScore();
	//随机可下注金额比例
	int RandAddProbability();
	//随机下注筹码
	int64_t RandChouMa(int64_t CanxiazuGold);
	//随机下注区域
	int RandArea();

private:
	//读取配置
	bool ReadConfigInformation();

	int Random(int start, int end){
		return (int)(((double)rand()) / RAND_MAX * (end - start) + start);
	}

private:
	muduo::net::TimerId m_TimerJetton;
	muduo::net::TimerId m_TimerEnd;
	shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;

	//控件变量
public:
	shared_ptr<ITableFrame> m_pITableFrame;			 // table frame item.
	shared_ptr<IServerUserItem> m_pIAndroidUserItem; //用户接口

	tagGameRoomInfo *m_pGameRoomInfo; //配置房间
	string m_strRoomName;			  //配置房间

	//机器人行为控制
public:    
	uint32_t m_AndroidExitTimes;		 //机器人退出时间

	uint32_t m_StartIntervalInit;  //开始下注间隔
	uint32_t m_JettonIntervalInit; //每注间隔 

	time_t m_TimerStart;   
	uint32_t m_JettonTime;  
	uint32_t m_ShenSuanZiMaxArea;  
	uint32_t m_ContinueJettonCount;		//连续下注同一个筹码次数 
										//机器人存取款
	int64_t m_lRobotScoreRange[2];		//最大范围 
	int64_t m_lRobotBankGetScoreBanker; //提款数额  (庄家)
	int m_nRobotBankStorageMul;			//存款倍数
										//机器人行为控制
public:
	int32_t m_AreaProbability[BET_ITEM_COUNT];		 //区域下注权重
	int64_t m_AreaRobotAddGold[BET_ITEM_COUNT];	//游戏区域最多下注金额
	int32_t m_CurrentRobotAddGold[BET_ITEM_COUNT]; //当前机器人下注
	int m_ProbabilityWeight[6];		 //可下注百分比权重
	int32_t m_AddMaxGold[5];		 //最大下注筹码
	int m_ChouMaWeight[5];			 //筹码权重
	int32_t m_RobotAddGold[BET_ITEM_COUNT];		 //机器人龙虎和区域下注
	int m_TimeMinuteProbability[6];  //退出概率 m_TimeMinuteProbability
	int64_t m_dwBetTotalTime;		 //下注总时间
	int64_t m_dwStartBetTime;		 //开始下注时间
	int64_t m_kaishijiange;
	int64_t m_jiangeTime;
	int m_Jetton;
	int count;
	int64_t m_StartsScore;
	time_t m_jionTime;
	time_t m_Endtimer;
	time_t m_Lasttimer;
	int m_MaxWinTime;
	bool m_isShensuanzi;
	int m_hensuanziMaxArea;
	vector<int> m_SendArea;

	int m_lianxuxiazhuCount; //连续下注同一个筹码次数
	int64_t m_JettonScore;	//上一轮下注筹码
	int m_kaishijiangeInit;  //开始下注间隔
	int m_jiangeTimeInit;	//每注间隔
	int m_End3s;			 //最后3秒下注概率

	int m_ipalyTimes;
	int CountrandomRound;

	int32_t m_wChairID;
	int32_t m_GameRound;
    int32_t                            m_currentChips[6];
};

#endif
