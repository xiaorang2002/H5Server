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

    virtual bool RepositionSink();
    //初始接口
//    virtual void Initialization(shared_ptr<ITableFrame>& pTableFrame, int wChairID, shared_ptr<IServerUserItem>& pUserItem);
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
   // virtual void SetDdzAiThread(CDdzAiThread* pAiThread) {}

    //设置斗地主AI处理后的数据
   // virtual void HandleDdzAiAction(std::shared_ptr<ai_action>& aiAction) {}

    //游戏事件
public:
    //virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam);

    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size) ;
public:
    bool OnFreeGameScene(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubGameStart(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubSendCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOpenCardResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);

public:
    bool ClearAllTimer();
    void GameTimeAddScore();
    void GameTimeOpencard();
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
    int				m_wChairID;
    int             randomRound;
    int             CountrandomRound;
    shared_ptr<IServerUserItem>	m_pAndroidUserItem;

    BYTE				m_cardData[MAX_COUNT];
    int				m_wCallBankerNum;	//已叫庄人数
    int				m_wAddScoreNum;		//已下注人数
    int				m_wOpenCardNum;		//已开牌人数
    int				m_wPlayerCount;

    int             m_AndroidPlayerCount;
    int             m_icishu;
    static uint32_t		m_cbJettonMultiple[MAX_JETTON_MUL];
    //static BYTE		m_cbBankerMultiple[MAX_BANKER_MUL];

    int             ClearAndroidCount;

    muduo::net::TimerId             m_TimerAddScore;
    muduo::net::TimerId             m_TimerOpenCard;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    int            m_cbOxCardData;
    int            m_isMaxCard;
    int             m_jiabeibeishu[4][4];
    bool           m_m_cbPlayStatus;
};
