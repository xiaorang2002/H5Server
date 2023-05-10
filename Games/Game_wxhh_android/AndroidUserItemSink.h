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

	//控制接口
public:
	//初始接口
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//重置接口
	virtual bool RepositionSink();

	//游戏事件
public:
	//时间消息
    void OnTimerJetton();
    void OnTimerGameEnd();

    //游戏消息
    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size);



    //场景消息

    virtual bool OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData, uint32_t size);

    virtual bool CheckExitGame();
    virtual bool ExitGame();

protected:

    //用户加注
    bool OnSubAddScore();
    //随机可下注金额比例
    int RandAddProbability();
    //随机下注筹码
    int64_t RandChouMa(int64_t CanxiazuGold,int8_t GoldId);
    //随机下注区域
    int RandArea();
	//库存操作
private:
	//读取配置
    bool ReadConfigInformation();

public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy){ m_strategy = strategy;  };
    tagAndroidStrategyParam* GetAndroidStrategy(){ return m_strategy;   };

protected:
    tagAndroidStrategyParam*            m_strategy;

private:
    muduo::net::TimerId                    m_TimerJettonId;
    muduo::net::TimerId                    m_TimerEndId;
    //控件变量
public:
    shared_ptr<ITableFrame>              m_pITableFrame;                         // table frame item.
    shared_ptr<IServerUserItem>          m_pIAndroidUserItem;					//用户接口

    tagGameRoomInfo*                     m_pGameRoomInfo;                        //配置房间
    string                               m_strRoomName;                          //配置房间

    //机器人行为控制
 public:
     uint32_t                           m_AreaProbability[5];//区域下注权重
     int64_t                            m_AreaRobotAddGold[5];//游戏区域最多下注金额
     uint32_t                           m_ProbabilityWeight[6];//可下注百分比权重
     uint32_t                           m_TimeMinuteProbability[6];//退出概率

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
     vector<int64_t>                   m_SendArea;

     uint32_t                           m_ContinueJettonCount;//连续下注同一个筹码次数
     int64_t                            m_JettonScore;//上一轮下注筹码
     uint64_t                           m_RoundJettonScore; //这轮下注金额
     uint64_t                           m_RoundJettonCount; //这轮下注次数
     uint32_t                           m_AndroidExitTimes; //机器人退出时间
     double                             m_AndroidJettonProb; //机器人押注比例>1000

     int64_t                            m_currentChips[CHIPS_SIZE];
};

//////////////////////////////////////////////////////////////////////////

#endif
