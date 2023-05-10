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
#define MASK_COLOR					0xF0								//花色掩码
#define MASK_VALUE					0x0F								//数值掩码


#define OX_VALUE0					0									//混合牌型 meiniu 1-10 niu1-niuniu
#define OX_SILVERY_BULL				11									//银牛 sihua
#define OX_GOLDEN_BULL				12									//五花牛（金牛）
#define OX_BOMB						13									//炸弹（四梅）
#define OX_FIVE_SMALL				14									//五小牛
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
    bool ReadConfigInformation();


    uint8_t GetCardValueDian(BYTE m_cardData[]);
    bool GetSpecialValue(BYTE m_cardData[]);
    //获取数值
    uint8_t GetCardValue(uint8_t cbCardData) { return cbCardData&MASK_VALUE; }
    //获取花色
    uint8_t GetCardColor(uint8_t cbCardData) { return (cbCardData&MASK_COLOR)>>4; }
    //逻辑数值
    uint8_t GetLogicValue(uint8_t cbCardData);

    uint8_t GetCardType(uint8_t cbCardData[], uint8_t cbCardCount);
    STD::Random m_random;
    //抢庄阶段机器人抢庄概率分布表,牌大0，牌小1
     int32_t RobZhuangProbability0[2][12];//buqiang
     int32_t RobZhuangProbability1[2][12];//1 bei
     int32_t RobZhuangProbability2[2][12];//2 bei
     int32_t RobZhuangProbability4[2][12];//4 bei

    //庄家叫1倍下注阶段机器人下注概率分布,牌大0，牌小1
     int32_t Bank1RobotBetProbability5[2][12];//5 bei
     int32_t Bank1RobotBetProbability10[2][12];//10 bei
     int32_t Bank1RobotBetProbability15[2][12];//15 bei
     int32_t Bank1RobotBetProbability20[2][12];//20 bei
    //庄家叫2倍下注阶段机器人下注概率分布,牌大0，牌小1
     int32_t Bank2RobotBetProbability5[2][12];//5 bei
     int32_t Bank2RobotBetProbability10[2][12];//10 bei
     int32_t Bank2RobotBetProbability15[2][12];//15 bei
     int32_t Bank2RobotBetProbability20[2][12];//20 bei
    //庄家叫4倍下注阶段机器人下注概率分布,牌大0，牌小1
     int32_t Bank4RobotBetProbability5[2][12];//5 bei
     int32_t Bank4RobotBetProbability10[2][12];//10 bei
     int32_t Bank4RobotBetProbability15[2][12];//15 bei
     int32_t Bank4RobotBetProbability20[2][12];//20 bei


    int IswinforPlayer;//比真实玩家大或者小
    int  bankerCall;
public:
    void SetAndroidStrategy(tagAndroidStrategyParam* strategy)
    {
        m_strategy = strategy;
    }
    tagAndroidStrategyParam* GetAndroidStrategy()
    {
        return m_strategy;
    }

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

    BYTE			m_cardData[MAX_COUNT];
    int				m_wCallBankerNum;	//已叫庄人数
    int				m_wAddScoreNum;		//已下注人数
    int				m_wOpenCardNum;		//已开牌人数
    int				m_wPlayerCount;

    int             m_AndroidPlayerCount;
    int             m_icishu;
    static uint32_t		m_cbJettonMultiple[MAX_JETTON_MUL];
    static uint32_t		m_cbBankerMultiple[MAX_BANKER_MUL];

    int             ClearAndroidCount;

    muduo::net::TimerId             m_TimerCallBanker;
    muduo::net::TimerId             m_TimerAddScore;
    muduo::net::TimerId             m_TimerOpenCard;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    int             m_cbOxCardData;
    int             m_isMaxCard;
    int             m_qzbeishu[4][4];
    int             m_jiabeibeishu[4][4];
    bool            m_m_cbPlayStatus;
    float           openCardTime[5];
};
