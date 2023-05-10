#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE


using namespace ddz;


//////////////////////////////////////////////////////////////////////////////////
//手牌分析
struct tagAnalyseHandInfo
{
    bool bBigKing;					//是否有大王
    bool bSmallKing;				//是否有小王
    WORD wTwoCount;					//2的数目
    WORD wAceCount;					//A的数目
    WORD wBombCount;				//炸弹数目
    WORD wTakeCount;				//三带数目
    WORD wSingleCardCount;			//小牌数目
    WORD wDoubleCardCount;			//小对数目
    WORD wBiggestBomb;				//最大炸弹

    WORD wHandCardLever;			//手牌级别，越大表示越好(硬性判断，这里只综合考虑大牌数目和小牌数目)

};
//////////////////////////////////////////////////////////////////////////////////

//游戏桌子
class CTableFrameSink  :public ITableFrameSink
{

    //游戏记录
protected:
    CString							m_GameRecord;
    TCHAR							m_szNickName[GAME_PLAYER][LEN_NICKNAME];
    //游戏变量
protected:
    WORD                            m_dwlCellScore;						//基础积分
    WORD							m_wFirstUser;						//首叫用户
    WORD							m_wBankerUser;						//庄家用户
    WORD							m_wCurrentUser;						//当前玩家
    //uint8_t							m_cbOutCardCount[GAME_PLAYER];		//出牌次数
    bool							m_bTrustee[GAME_PLAYER];			//是否托管

    uint8_t							m_cbNoCallCount;					//无人叫分的次数
    bool							m_bFisrtCall;

    //托管信息
    DWORD                           m_dwTime;						//Time时间d
    bool							m_bOffLineTrustee;					//离线托管
    int                             m_cbRemainTime;                     //剩余时间
    int                             timeCount;
    tagSearchCardResult				m_SearchCardResult;					//搜索结果

    //炸弹信息
protected:
    uint8_t							m_cbBombCount;						//炸弹个数
    uint8_t							m_cbEachBombCount[GAME_PLAYER];		//炸弹个数

    //叫分信息
protected:
    DWORD                           m_dwStartTime;						//初始倍数(叫分倍数)
    uint8_t							m_cbFirstCall;						//首叫椅子
    uint8_t							m_cbBankerScore;					//庄家叫分
    uint8_t							m_cbScoreInfo[GAME_PLAYER];			//叫分信息
protected:
    uint8_t							m_cbStakeInfo[GAME_PLAYER];			//下注加倍信息
    //出牌信息
protected:
    WORD							m_wTurnWiner;						//胜利玩家
    uint8_t							m_cbTurnCardCount;					//出牌数目
    uint8_t							m_cbTurnCardData[MAX_COUNT];		//出牌数据

    uint8_t							m_cbSurplusCard[FULL_COUNT];
    uint8_t							m_cbSurplusCount;

    //扑克信息
protected:
    uint8_t							m_cbOutCardCount[GAME_PLAYER];		//出牌次数
    uint8_t							m_cbOutCardListCount[GAME_PLAYER][MAX_COUNT];			//每个玩家每次出牌的
    uint8_t							m_cbOutCardList[GAME_PLAYER][MAX_COUNT][MAX_COUNT];		//每个玩家的出牌列表
    uint8_t							m_cbBankerCard[3];					//游戏底牌
    uint8_t							m_cbHandCardCount[GAME_PLAYER];		//扑克数目
    uint8_t							m_cbHandCardData[GAME_PLAYER][20];	//手上扑克
    uint8_t							m_cbRemainCardNum[GAME_PLAYER][MAX_TYPE1];	//玩家剩余扑克数目(依次是A - K,小王，大王)
	uint8_t							m_cbHandCardAllData[GAME_PLAYER][20];	//手牌扑克

    //组件变量
protected:
    uint8_t							m_cbGameStatus;						//游戏状态
    CGameLogic						m_GameLogic;						//游戏逻辑
    CHistoryScore					m_HistoryScore;						//历史成绩
	int64_t                          m_CalculateRevenue[GAME_PLAYER];	//tax
	int64_t                          m_CalculateAgentRevenue[GAME_PLAYER];//tax

    //组件接口
protected:
    shared_ptr<ITableFrame>			m_pITableFrame;						//框架接口
    shared_ptr<CServerUserItem>     m_pUserItem;						//框架接口
    tagGameRoomInfo*				m_pGameRoomKindInfo;				//房间配置
	tagGameReplay                   m_replay;//game replay

	int64_t                          m_lCellScore;
	int64_t                          m_lStockScore;
	int64_t                          m_lInScoreVal;
	int64_t                          m_lOutScoreVal;

    //组件变量
protected:
    uint32_t 						m_lStorageDeduct;						//回扣变量 千分比

	int64_t							m_lStorageMin;							//库存最小值
	int64_t							m_lStorageMax;							//库存最大值

    uint8_t							m_StorageRatio;							//库存初值输赢比例百分之N
    uint8_t							m_StorageMinRatio;						//库存最小值输赢比例百分之N		杀分模式
    uint8_t							m_StorageMaxRatio;						//库存最大值输赢比例百分之N		送分模式
	int64_t							m_WinScoreSystem;						//pao ma deng
	int64_t							m_EndWinScore[GAME_PLAYER];				//前端显示输赢
    int								m_MSGID;
    bool							m_isflag;
	int64_t							m_iUserWinScore[GAME_PLAYER];

    string							m_strRoundId;
	int32_t							m_bTestGame;			//做牌测试
	list<uint8_t>                   m_listTestCardArray;
	int32_t							m_cbGoodCardValue;
	int32_t							m_cbGoodCardTime;
	WORD							m_cbTestUser;
private:
    muduo::MutexLock				m_mutexLock;
public:
    mutable boost::shared_mutex     m_mutex;

private:
    muduo::net::TimerId             m_TimerId;
    muduo::net::TimerId             m_TimerIdReadyOver;
    muduo::net::TimerId             m_TimerCallBankerId;    //叫分
    muduo::net::TimerId             m_TimerCallStakeId;     //加倍
    muduo::net::TimerId             m_TimerTrustId;         //托管出牌
    //muduo::net::EventLoopThread     m_TimerLoopThread;
	shared_ptr<muduo::net::EventLoopThread>                 m_TimerLoopThread;

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
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

private:
    bool							m_bReplayRecordStartFlag;					//是否开始录像
    bool							m_bReplayRecordStart;						//是否开始录像
    //::Game::Common::GamePlaybackRecord                m_UserReplayRecord;						//对四个位置用户的数据的记录，用于回放。

	double								stockWeak;                                  //库存衰减
	uint32_t							m_dwReadyTime;								//ready时间
	uint32_t 							m_dwEndTime;								//结束时间
	chrono::system_clock::time_point 	m_dwStartGameTime;								//开始时间
	bool								m_bhaveCancelTrustee;						//是否申请了取消托管

    //函数定义
public:
    //构造函数
    CTableFrameSink();
    //析构函数
    virtual ~CTableFrameSink();

    int64_t GetCellScore();
    //基础接口
public:
    //释放对象
    virtual VOID Release() { delete this; }

    bool  SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);

    //管理接口
public:
    //复位桌子
    virtual VOID RepositionSink();
    //配置桌子
    virtual bool Initialization();

    //查询接口

    //查询接口
    virtual bool OnCheckGameStart();

    virtual int OnNeedAndroid();

public:
    //查询限额
    virtual double QueryConsumeQuota(shared_ptr<IServerUserItem> pIServerUserItem);
    //最少积分
    virtual double QueryLessEnterScore(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);

    //游戏事件
public:
    //游戏开始
    virtual void OnGameStart();

    //游戏结束
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag);

    //解散结束
    virtual bool  OnDismissGameConclude();

    //常规结束
    virtual bool  OnNormalGameConclude(WORD wChairID);

    //用户强退
    virtual bool  OnUserLeaveGameConclude(uint8_t cbReason, WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);

    //发送场景
    virtual bool OnEventGameScene(uint32_t chairid, bool bisLookon);
    bool		 RepayRecordGameScene(DWORD chairid, bool bisLookon);
    //事件接口
public:
    //时间事件
    //virtual bool OnTimerMessage(DWORD timerid, DWORD param);
    void		 OnTimer();
    void         OnTimerGameReadyOver();
    void         OnCallBankerTimer();   //叫分定时器
    void         OnCallStakeTimer();    //加倍
    void         OnUserTrustTimer();    //系统托管出牌定时器
    //数据事件
    virtual bool OnDataBaseMessage(WORD wRequestID, VOID * pData, WORD wDataSize);
    //积分事件
    virtual bool OnUserScroeNotify(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, uint8_t cbReason);

    //网络接口
public:
    //游戏消息
    virtual bool OnGameMessage(DWORD chairid, uint8_t subid, const uint8_t* data, uint32_t datasize);

    //框架消息
    virtual bool OnFrameMessage(DWORD chairid, uint8_t subid, const uint8_t* pData, DWORD wDataSize);


    virtual bool CanLeftTable(int64_t userId);

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);

    //用户事件
public:
    //用户断线
    virtual bool OnActionUserOffLine(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);
    //用户重入
    virtual bool OnActionUserConnect(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);
    //用户坐下
    virtual bool OnActionUserSitDown(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, bool bLookonUser);
    //用户起立
    virtual bool OnActionUserStandUp(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, bool bLookonUser);
    //用户同意
    virtual bool OnActionUserOnReady(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, VOID * pData, WORD wDataSize) { return true; }

    virtual bool OnUserEnter(int64_t userid, bool islookon);

    virtual bool OnUserReady(int64_t userid, bool islookon);

    virtual bool OnUserLeft( int64_t userid, bool islookon);

    //游戏状态
protected:
    //空闲状态
    bool OnSceneFree(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);
    //叫分状态
    bool OnSceneCall(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);
    //加倍状态
    bool OnSceneStake(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);
    //游戏状态
    bool OnScenePlay(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem);
    //游戏事件
protected:
    //用户放弃
    bool OnUserPassCard(WORD wChairID);
    //用户叫分
    bool OnUserCallScore(WORD wChairID, uint8_t cbCallScore);
    //加倍
    bool OnUserStakeScore(WORD wChairID, uint8_t cbStakeScore);
    //用户出牌
    bool OnUserOutCard(WORD wChairID, uint8_t cbCardData[], uint8_t cbCardCount,bool bSystem=false);
    //用户委托
    bool OnUserTrust(WORD wChairID, bool bTrust);
    //牌转文字
    CString TransformCardInfo( uint8_t cbCardData );
    //是否可以出掉最后一手牌，而无需等待
    bool IsCanOutAllCard(WORD wChairID);
    //自动出牌
    bool OnAutoOutCard(WORD wChairID);
    //自动叫分
    bool OnAutoCallScore(WORD wChairID);
    //自动加注
    bool OnAutoStakeScore(WORD wChairID);
    //获取剩余牌
    bool OnUserRemainCardNum(WORD wChairID);
    //辅助函数
protected:
    //分析扑克
    void AnalysebHandCard(uint8_t HandCardData[],tagAnalyseHandInfo &HandCardInfo);
    //黑白控制
    void ListControl();
    //比较扑克等级
    void CompareHandLever(tagAnalyseHandInfo &HandCardInfo1,tagAnalyseHandInfo &HandCardInfo2);

    //断线托管
    bool IsOfflineTrustee(WORD wChairID);
    bool CalRemainCardNum();
    //地主已经分配
    bool isBankerDecided();
    void StoreageControl();
    void  openLog(const char *p, ...);
    char * GetValue(uint8_t itmp[],uint8_t count, char * name);
    void SendWinSysMessage();
    bool CleanUser();
    void AutoWork();
    //tmp......
    bool RepayRecorduserinfo(int wChairID);
    bool RepayRecorduserinfoStatu(int wChairID,int status);
    bool RepayRecordStart();
    bool RepayRecordEnd();
    bool RepayRecord(int wChairID,WORD wSubCmdId, uint8_t* data, WORD wSize);
	//读取配置
	bool ReadConfig();
	void TestCard();
	bool ReadCardConfig();
	bool bHaveTheSameCard();
};

//////////////////////////////////////////////////////////////////////////////////
#endif
