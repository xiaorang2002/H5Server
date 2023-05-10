#pragma once

static int  RandomInt(int a,int b)
{
    return rand() %(b-a+1)+ a;
}
class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
    CAndroidUserItemSink();
    virtual ~CAndroidUserItemSink();

public:
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem) override;
    virtual bool RepositionSink() override;
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem) override;
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;
    //初始接口
//    virtual void Initialization(shared_ptr<ITableFrame>& pTableFrame, uint32_t wChairID, shared_ptr<IServerUserItem>& pUserItem);

   // virtual void SetDdzAiThread(CDdzAiThread* pAiThread) {}

    //设置斗地主AI处理后的数据
   // virtual void HandleDdzAiAction(std::shared_ptr<ai_action>& aiAction) {}

    //游戏事件
public:
    //virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam);

    //游戏消息
    virtual bool OnGameMessage(uint8_t cbGameStatus, const uint8_t* pData, uint32_t size);

public:
//    bool OnFreeGameScene(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize);
    bool OnSubGameStart(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubCallBanker(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubCallBankerResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    // bool OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubSendCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOpenCardResult(uint8_t wSubCmdID, void* pData, uint32_t wDataSize);
    bool OnSubOperateFail(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
public:
    bool ClearAllTimer();

    void GameTimerCallBanker();
    // void GameTimeAddScore();
    void GameTimeOpencard();
    //读取配置
    bool ReadConfigInformation();
    void  openLog(const char *p, ...);
public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy){ m_strategy = strategy;  };
    tagAndroidStrategyParam* GetAndroidStrategy(){ return m_strategy;   };

protected:
    tagAndroidStrategyParam*        m_strategy;
private:
    tagGameRoomInfo*                m_pGameRoomInfo;                        //配置房间
    string                          m_strRoomName;                          //配置房间
    shared_ptr<ITableFrame>		    m_pTableFrame;
    uint32_t				        m_wChairID;
    int                             randomRound;
    int                             CountrandomRound;
    shared_ptr<IServerUserItem>	    m_pAndroidUserItem;

    uint8_t				            m_cardData[MAX_COUNT];
    int                             m_HandcardData[MAX_SENDCOUNT];
    int				                m_wCallBankerNum;	//已叫庄人数
    int				                m_wAddScoreNum;		//已下注人数
    int				                m_wOpenCardNum;		//已开牌人数
    int				                m_wPlayerCount;

    int                             m_AndroidPlayerCount;
    int                             m_icishu;
    int64_t                         m_cbMaxJettonMultiple[5]; //最大下注倍数
   // static uint8_t		        m_cbBankerMultiple[MAX_BANKER_MUL];

    int                             ClearAndroidCount;

   // Landy::GameTimer              m_TimerCallBanker;
   // Landy::GameTimer              m_TimerAddScore;
   // Landy::GameTimer              m_TimerOpenCard;
    muduo::net::TimerId             m_TimerCallBanker;
    muduo::net::TimerId             m_TimerAddScore;
    muduo::net::TimerId             m_TimerOpenCard;
    
    int                             m_cbSGCardType;
    int                             m_isMaxCard;
    ////目前三公是通过抢庄牛牛修改，在机器人当中抢庄牛牛的抢庄倍数是通过配置文件得来
    ////可是在三公的玩法上只有叫庄和不叫
    ////现在机器人是随机叫庄、随机下注
   // int                           m_qzbeishu[4][4];
   // int                           m_jiabeibeishu[4][4];
    bool                            m_m_cbPlayStatus;
    // 是否写日记
    bool                            m_bWritelog;
};
