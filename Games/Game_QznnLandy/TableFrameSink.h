
#pragma once

//游戏桌子类
class CTableFrameSink :public ITableFrameSink
{

public:
	//构造函数
    CTableFrameSink();
	//析构函数
    virtual ~CTableFrameSink();


public:
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t wChairID, uint8_t cbReason);
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairid, bool bisLookon);

   // bool RepayRecordGameScene(int64_t chairid, bool bisLookon);

    //事件接口
public:
    //定时器事件
    //virtual bool  OnTimerMessage(int64_t wTimerID, WPARAM wBindParam);
    //游戏消息
    virtual bool OnGameMessage( uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize);
    //用户接口
public:
    virtual bool OnUserEnter(int64_t userid, bool islookon);
    virtual bool OnUserReady(int64_t userid, bool islookon);
    virtual bool OnUserLeft( int64_t userid, bool islookon) ;
    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
    virtual bool CanLeftTable(int64_t userId);
public:
    //设置指针
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
    //清理游戏数据
    virtual void ClearGameData();

    virtual void RepositionSink();

private:
    //发送数据
    bool SendTableData(int dwChairID, int wSubCmdID, const void* pBuf=nullptr, int dwLen=0, bool bRecord = true);

    //功能函数
private:
    //初始化游戏数据
    void InitGameData();
//    //更新游戏配置
//    void UpdateGameConfig();
    //清除所有定时器
    void ClearAllTimer();

    void ResetTodayStorage();


    //机器人相关函数
private:
    //CardProvider c;
    //分析
    int64_t AnalyseAndroidWinScore(bool bAndroidBanker, uint8_t cbCardData[GAME_PLAYER][MAX_COUNT]);

    void PaoMaDeng();
    int IsMaxCard(int wChairID);

//    char* GetValue(int itmp, char * name);

//    void  openLog(const char *p, ...);

private:
    //托管
    bool IsTrustee(void);

    void GameStartInit();

    void GameTimerReadyOver();
    void GameTimereEnterPlayingOver();

    //叫庄
    void GameTimerCallBanker();
    //玩家自动叫庄
    int AutoUserCallBanker();
    //玩家叫庄
    int OnUserCallBanker(int wChairID, int	wCallFlag);
    //随机庄家
    int RandBanker();

    //下注
    void GameTimerAddScore();
    //玩家自动下注
    int AutoUserAddScore();
    //玩家下注
    int OnUserAddScore(int wChairID, int wJettonIndex);

    //扑克分析
    void AnalyseCardEx();
    //黑名单分析
    void AnalyseForBlacklist();

    void GameTimerOpenCard();
    //玩家自动开牌
    int AutoUserOpenCard();
    //玩家开牌
    int OnUserOpenCard(int wChairID);

    void GameTimerEnd();
    //普通场游戏结算
    void NormalGameEnd(int dwChairID);

    void InitLoadini();


private:
    int64_t                                     m_iUserWinScore[GAME_PLAYER];
    int                                         m_nLottery;								//发牌次数
    muduo::net::TimerId                         m_TimerIdReadyOver;
    muduo::net::TimerId                         m_TimerIdEnterPlaying;
    muduo::net::TimerId                         m_TimerIdCallBanker;
    muduo::net::TimerId                         m_TimerIdAddScore;
    muduo::net::TimerId                         m_TimerIdOpenCard;
    muduo::net::TimerId                         m_TimerIdEnd;

    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;
private:
	////////////////////////////////////////////////////////////////////////////////
	//匹配规则文档：svn://Document/Design/策划文档/通用类需求/游戏匹配机制.doc
	//修改前即使用户量上来了也还是无法保证没有机器人参与进来
	//修改后用户量上来了肯定是没有机器人参与的，只要玩家在3.6s(可配置)之前匹配到真实用户(肯定的)
	//会立即进入游戏桌子，没有一点延迟时间，游戏秒开
	//有一种情况是，刚好概率随机到当前是个5人局，已经有3个人在匹配了，也就是还差2个人才会开始游戏，
	//正常情况是3.6s(可配置)超时之后若人数不足由机器人补齐空位，但是若机器人数量不足或被关闭掉了，
	//就会始终都处于匹配等待状态，无法进入游戏桌子，所以处理这种情况的办法是3.6s(可配置)超时之后
	//由机器人候补空位后判断仍然没有达到游戏要求的5个人时，只要达到最小游戏人数(MIN_GAME_PLAYER)即可进入游戏
	//否则继续等待玩家进入游戏桌子(此时机器人数量肯定不足)
	//不过有一种情况可能匹配不到人，比如有0号和1号两个桌子，0号桌子被占用了，这时新进来一个玩家A匹配游戏，
	//系统会分配1号桌子给该新玩家A，但是机器人没有库存了，此时之前的0号桌子游戏结束空闲下来，又新进来一个玩家B匹配游戏，
	//系统会重新把0号桌子分配给该新玩家B，这样玩家A和玩家B只会在各自分配的桌子当中一直等待其他玩家， 如果没有其他
	//玩家加入，玩家A和玩家B永远也匹配不到一块去，除非1号桌子的玩家退出重新开始匹配。框架处理游戏桌子分配的逻辑是从编号
	//为0的桌子开始遍历查找空闲桌子，找到则分配给当前进入匹配的玩家。原先匹配桌子编号靠后的玩家，在前面的桌子游戏结束空闲
	//下来后不会匹配到任何玩家，直到前面的桌子全部被重新占用，或者退出重新开始匹配游戏，逻辑没问题。
	////////////////////////////////////////																
	//累计匹配时长，当前累计已经匹配了多长时间
	double totalMatchSeconds_;
	//分片匹配时长，比如0.1s做一次检查
	double sliceMatchSeconds_;
	//超时匹配时长，累计匹配多长时间算超时了，比如3.6s算超时了
	//目前机制是匹配开始，间隔1s补一个机器人，把真实玩家的坑给占了
	//修改后3.6s之前禁入机器人，匹配超时到期(3.6s后)桌子人数不足则用机器人补齐空位
	//如果3.6秒之前匹配进行中，真实玩家人数达到游戏要求人数，撤销定时器立即开始游戏
	double timeoutMatchSeconds_;
	//机器人候补空位超时时长
	double timeoutAddAndroidSeconds_;
	////////////////////////////////////////
	//桌子要求标准游戏人数，最大人数上限GAME_PLAYER和概率值计算得到
	int MaxGamePlayers_;
	//桌子游戏人数概率权重
	int ratioGamePlayerWeight_[GAME_PLAYER];
	//放大倍数
	int ratioGamePlayerWeightScale_;
	//计算桌子要求标准游戏人数
	CWeight poolGamePlayerRatio_;
	void OnTimerGameReadyOver();
private:
    //游戏信息
    int                     m_wGameStatus;										//游戏状态
    uint8_t                 m_cbPlayStatus[GAME_PLAYER];						//游戏中玩家
    uint8_t                 m_cbPlayStatusRecord[GAME_PLAYER];						//游戏中玩家
    uint32_t                g_nRoundID;
//    uint8_t                 m_SitTableFrameStatus[GAME_PLAYER];				    //游戏中in play

    atomic_int              m_wPlayCount;										//游戏中玩家个数
    atomic_int              m_wAndroidCount;									//游戏中AI的个数

    int                     m_sCallBanker[GAME_PLAYER];							//玩家叫庄(-1:未叫, 0:不叫, 1,2,4:叫庄)
    int                     m_sAddScore[GAME_PLAYER];							//玩家下注(-1:未下注, 大于-1:对应筹码索引)
    int                     m_sCardType[GAME_PLAYER];							//玩家开牌(-1:未开牌, 大于-1:对应牌型点数)
    int                     m_iMultiple[GAME_PLAYER];							//倍数

    //玩家标识
    int                     m_wBankerUser;										//庄家
    uint8_t                 m_cbCardData[GAME_PLAYER][MAX_COUNT];				//牌数据
    uint8_t                 m_cbOXCard[GAME_PLAYER][MAX_COUNT];					//牛牛牌数据


    int                     PaoMaDengCardType[GAME_PLAYER];
    int                     PaoMaDengWinScore[GAME_PLAYER];
    bool                    m_bContinueServer;
    bool                    m_bPlayerIsOperation[GAME_PLAYER];                  // palyer is Operation
    int                     m_iBlacklistNum;
    bool                    m_bBlacklist[GAME_PLAYER];
private:
    //房间信息
    int32_t                  m_dwCellScore;										//基础积分
    string mGameIds;
//    uint8_t                 m_cbCurJetton[MAX_JETTON_MUL];						//当前筹码
private:
    //扑克信息
    uint8_t					m_cbLeftCardCount; 									//剩余数目
    uint8_t					m_cbRepertoryCard[MAX_CARD_TOTAL];					//库存扑克
    int32_t                 m_cbCallable[GAME_PLAYER][MAX_BANKER_MUL];//玩家可用的叫庄倍数
    int32_t                 m_cbJettonable[GAME_PLAYER][MAX_JETTON_MUL];//玩家可用的下注倍数
private:
    static int32_t			m_cbJettonMultiple[MAX_JETTON_MUL];					//下注倍数1, 5, 10, 15, 20
    static int32_t			m_cbBankerMultiple[MAX_BANKER_MUL];					//叫庄倍数 1, 2, 4

private:
    shared_ptr<ITableFrame>         m_pITableFrame;										//游戏指针
    tagGameRoomInfo*                 m_pGameRoomInfo;
    CGameLogic				m_GameLogic;										//游戏逻辑
//    tagScoreInfo			m_RecordScoreInfo[GAME_PLAYER];						//记录积分

    static int64_t		m_llStorageControl;									//库存控制
    static int64_t		m_llStorageLowerLimit;								//库存控制下限值
    static int64_t		m_llStorageHighLimit;								//库存控制上限值
    static int64_t		m_llTodayStorage;									//今日库存
    static bool         m_isReadstorage;
    static bool         m_isReadstorage1;
    static bool         m_isReadstorage2;
    int					m_wKillRatio;
    int64_t				m_iTodayTime;

    tagGameReplay       m_replay;//game replay
//    std::map<int, std::string>      m_mOutCardRecord;
//    char							m_RecordOutCard[256];						//出牌记录

//    CRITICAL_SECTION    m_lock;
private:
    uint32_t                m_dwInitTime;
    uint32_t                m_dwReadyTime;										//ready时间
    uint32_t                m_dwEnterPlayingTime;						        //EnterPlaying时间
    uint32_t  				m_dwCallTime;										//叫庄时间
    uint32_t 				m_dwScoreTime;										//下注时间
    uint32_t 				m_dwOpenTime;										//开牌时间
    uint32_t 				m_dwEndTime;										//结束时间
   chrono::system_clock::time_point 				m_dwStartTime;										//开始时间

    bool                    m_isMatch;
   //test case related
private:
    shared_ptr<CTestCase>      testCase;
    //STD::Random m_random;
    double          stockWeak;                                                  //库存衰弱
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制

//private:
    //muduo::MutexLock    m_mutexLock;


};

//////////////////////////////////////////////////////////////////////////

