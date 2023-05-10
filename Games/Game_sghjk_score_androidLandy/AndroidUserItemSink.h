#pragma once

//宏定义
#ifndef _UNICODE
#define myprintf	_snprintf
#define mystrcpy	strcpy
#define mystrlen	strlen
#define myscanf		_snscanf
#define	myLPSTR		LPCSTR
#define mystchr		strchr
#define myatoi		atoi
#define mychar		char
#else
#define myprintf	swprintf
#define mystrcpy	wcscpy
#define mystrlen	wcslen
#define myscanf		_snwscanf
#define	myLPSTR		LPWSTR
#define mystchr		wcschr
#define myatoi		_wtoi
#define mychar		TCHAR
#endif

using namespace  sghjk;

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
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
    virtual bool RepositionSink();
    //初始接口
    virtual void Initialization(shared_ptr<ITableFrame>& pTableFrame, int wChairID, shared_ptr<IServerUserItem>& pUserItem);

   // virtual void SetDdzAiThread(CDdzAiThread* pAiThread) {}

    //设置斗地主AI处理后的数据
   // virtual void HandleDdzAiAction(std::shared_ptr<ai_action>& aiAction) {}

    //游戏事件
public:
    //virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam);

    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size) ;
public:
    bool OnFreeGameScene(const uint8_t* pData, uint32_t wDataSize);
    bool OnScoreGameScene(const uint8_t* pData, uint32_t wDataSize);
    bool OnInsureGameScene(const uint8_t* pData, uint32_t wDataSize);
    bool OnPlayGameScene(const uint8_t* pData, uint32_t wDataSize);

    bool OnSubGameStart(const uint8_t* pData, uint32_t wDataSize);
    bool OnSubAddScore(const uint8_t* pData, uint32_t wDataSize);

    bool OnSubDealCard(const uint8_t* pData, uint32_t wDataSize);
    bool OnSubInsure(const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOperate(const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOperateResult(const uint8_t* pData, uint32_t wDataSize);
    bool OnBankerDeals(const uint8_t* pData, uint32_t wDataSize);
    bool OnSubGameend(const uint8_t* pData, uint32_t wDataSize);
public:
    bool ClearAllTimer();

    void GameTimerOperate();
    void GameTimeAddScore();
    void GameTimeInsure();
    //读取配置
    void ReadConfigInformation();
    void  openLog(const char *p, ...);

public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy){ m_strategy = strategy;  };
    tagAndroidStrategyParam* GetAndroidStrategy(){ return m_strategy;   };

protected:
    tagAndroidStrategyParam*            m_strategy;

private:
    tagGameRoomInfo*                m_pGameRoomKind;                        //配置房间
    string							m_szRoomName;						//配置房间
    shared_ptr<ITableFrame>		m_pTableFrame;
    shared_ptr<IServerUserItem>	m_pAndroidUserItem;

    int				m_wChairID;
    uint8_t         m_cOperCode;    
    uint8_t			m_cardData[MAX_COUNT];
    uint8_t         m_cardtype;
    uint8_t         m_cardpoint;
    bool            m_hasA;

    uint32_t                        m_dwOpEndTime;							//操作结束时间
    uint32_t                        m_wTotalOpTime;							//操作总时间

//    int				m_wPlayerCount;
    vector<uint64_t>	m_cbJettonMultiple;

    muduo::net::TimerId             m_TimerOperate; 
    muduo::net::TimerId             m_TimerAddScore; 
    muduo::net::TimerId             m_TimerInsure;                    
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;                      //thread.

    int             m_bets[5];
    int             m_times[5];
    bool           m_m_cbPlayStatus;

private:
    void    AnalyseCard();
    uint8_t GetCardLogicValue(uint8_t carddata);
};
