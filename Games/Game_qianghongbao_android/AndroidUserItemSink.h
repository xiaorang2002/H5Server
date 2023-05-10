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

public:
	//初始接口
    // virtual bool Initialization(IUnknownEx * pIUnknownEx);
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//重置接口
	virtual bool RepositionSink();

public:

    void OnTimerJetton();
    void OnTimerGameEnd();
    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size);

    //场景消息
    // virtual bool OnEventSceneMessage(BYTE cbGameStatus, bool bLookonOther, void * pData, WORD wDataSize);
    virtual bool OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData, uint32_t size);

    virtual bool CheckExitGame();
    virtual bool ExitGame();
	//消息处理
protected:

    void  PalyerGrab();
    void  PalyerSendEvelope();

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

    muduo::net::TimerId                    m_TimerGrab;
    muduo::net::TimerId                    m_TimerSend;
    //控件变量
public:
    shared_ptr<ITableFrame>              m_pITableFrame;                         // table frame item.
    shared_ptr<IServerUserItem>          m_pIAndroidUserItem;					//用户接口
    tagGameRoomInfo*                     m_pGameRoomInfo;                        //配置房间
    string                               m_strRoomName;                          //配置房间
    STD::Random                          m_random;
    int32_t                              m_TimeMinuteProbability[6];             //退出概率
    int64_t                              m_AndroidExitTimes;
    int64_t                              m_JionTime;



};

//////////////////////////////////////////////////////////////////////////

#endif
