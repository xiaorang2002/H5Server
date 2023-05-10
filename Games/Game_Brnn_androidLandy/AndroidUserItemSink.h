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
    // virtual bool Initialization(IUnknownEx * pIUnknownEx);
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem) override;
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem) override;
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;
    //重置接口
    virtual bool RepositionSink() override;

public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy){ m_strategy = strategy;  };
    tagAndroidStrategyParam* GetAndroidStrategy(){ return m_strategy;   };

protected:
    tagAndroidStrategyParam*            m_strategy;
    mt19937::result_type        seed;

    //游戏事件
public:
    //时间消息
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

    //消息处理
protected:

    //用户加注
    bool OnSubAddScore();
    //随机可下注金额比例
    int RandAddProbability();
    //随机下注筹码
    int64_t RandChouMa(int64_t CanxiazuGold);
    //随机下注区域
    int RandArea();
//    //游戏结束
//    bool OnSubGameEnd(const void * pBuffer, uint32_t wDataSize);
//    void  openLog(const char *p, ...);
    //库存操作
private:
    //读取配置
    bool ReadConfigInformation();

    //逻辑辅助
protected:
//	//推断胜者
//    WORD GetBit(WORD num, int pos);
//    WORD EstimateWinner();

private:
    muduo::net::TimerId                     m_TimerJetton;
    muduo::net::TimerId                     m_TimerEnd;
    shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;

    //控件变量
public:
    shared_ptr<ITableFrame>              m_pITableFrame;                         // table frame item.
    shared_ptr<IServerUserItem>          m_pIAndroidUserItem;					//用户接口

    tagGameRoomInfo*                     m_pGameRoomInfo;                        //配置房间
    string                               m_strRoomName;                          //配置房间

private:
    std::mt19937                         m_rand;
    uint32_t                            m_totalWeight;

    //机器人行为控制
 public:
     uint32_t                           m_AreaProbability[4];//区域下注权重
     int64_t                            m_AreaRobotAddGold[4];//游戏区域最多下注金额
     uint32_t                           m_ProbabilityWeight[6];//可下注百分比权重
     int64_t                            m_AddMaxGold[5];//最大下注筹码
     uint32_t                           m_ChouMaWeight[6];//筹码权重
     uint32_t                           m_TimeMinuteProbability[6];//退出概率
    uint32_t                           m_AndroidExitTimes; //机器人退出时间
//     double                          m_RobotAddGold[3];//机器人龙虎和区域下注

     uint32_t                           m_StartIntervalInit;//开始下注间隔
     uint32_t                           m_JettonIntervalInit;//每注间隔
     uint32_t                           m_End3s;//最后3秒下注概率

     int64_t                             m_TimerStart;
     int64_t                             m_Endtimer;
     int64_t                             m_Lasttimer;

     uint32_t                           m_JettonTime;

//     int                            m_count;
     int64_t                            m_StartsScore;
     int64_t                            m_BetScore;

     int64_t                             m_JionTime;
     uint32_t                           m_MaxWinTime;

     bool                               m_IsShenSuanZi;
     uint32_t                           m_ShenSuanZiMaxArea;
     vector<uint32_t>                   m_SendArea;

     uint32_t                           m_ContinueJettonCount;//连续下注同一个筹码次数
     int64_t                            m_JettonScore;//上一轮下注筹码
     int64_t                            m_currentChips[7];
	 int32_t							m_betOnlyArea; //AI只下一个筹码并固定一个区域[1234], 0的时候随机
	 bool								m_testHaveBet; //测试之下一个区域，已下注了
};

//////////////////////////////////////////////////////////////////////////

#endif
