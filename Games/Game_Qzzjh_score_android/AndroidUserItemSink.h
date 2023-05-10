#pragma once

#define FloorScore		(m_pTableFrame->GetGameRoomInfo()->floorScore)//底注
#define CeilScore		(m_pTableFrame->GetGameRoomInfo()->ceilScore)//顶注
#define CellScore		(FloorScore)//底注
#define JettionList     (m_pTableFrame->GetGameRoomInfo()->jettons)//筹码表

#define ThisTableId		(m_pTableFrame->GetTableId())
#define ThisRoomId		(m_pTableFrame->GetGameRoomInfo()->roomId)
#define ThisRoomName	(m_pTableFrame->GetGameRoomInfo()->roomName)
#define ThisThreadTimer	(m_pTableFrame->GetLoopThread()->getLoop())

#define ThisChairId		(m_pAndroidUserItem->GetChairId())
#define ThisUserId		(m_pAndroidUserItem->GetUserId())
#define ThisUserScore	(m_pAndroidUserItem->GetUserScore())

#define ByChairId(chairId)	(m_pTableFrame->GetTableUserItem(chairId))
#define ByUserId(userId)	(m_pTableFrame->GetUserItemByUserId(userId))

#define UserIdBy(chairId) ByChairId(chairId)->GetUserId()
#define ChairIdBy(userId) ByUserId(userId)->GetChairId()

#define ScoreByChairId(chairId) ByChairId(chairId)->GetUserScore()
#define ScoreByUserId(userId) ByUserId(userId)->GetUserScore()

#define StockScore (storageInfo_.lEndStorage) //系统当前库存
#define StockLowLimit (storageInfo_.lLowlimit)//系统输分不得低于库存下限，否则赢分
#define StockHighLimit (storageInfo_.lUplimit)//系统赢分不得大于库存上限，否则输分

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
    bool OnSubCallBanker(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubCallBankerResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubSendCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);
    bool OnSubOpenCardResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize);

public:
    bool ClearAllTimer();

    void GameTimerCallBanker();
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
    //int				m_wChairID;
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
    static BYTE		m_cbJettonMultiple[MAX_JETTON_MUL];
    static BYTE		m_cbBankerMultiple[MAX_BANKER_MUL];

    int             ClearAndroidCount;

//    Landy::GameTimer                           m_TimerCallBanker;
//    Landy::GameTimer                           m_TimerAddScore;
//    Landy::GameTimer                           m_TimerOpenCard;
    muduo::net::TimerId             m_TimerCallBanker;
    muduo::net::TimerId             m_TimerAddScore;
    muduo::net::TimerId             m_TimerOpenCard;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;
	//机器人牌型
    int            m_cbOxCardData;
	//是否最大牌
    int            m_isMaxCard;
    int             m_qzbeishu[4][4];
    int             m_jiabeibeishu[4][4];
    bool           m_m_cbPlayStatus;
};
