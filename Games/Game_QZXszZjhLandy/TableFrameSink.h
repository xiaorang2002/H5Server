
#pragma once


//#define CWHArray std::deque
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//配置文件
#define INI_FILENAME "./conf/xjh_config.ini"
#define INI_CARDLIST "./conf/xjh_cardList.ini"

class UserInfo
{
public:
    UserInfo()
    {
        userId = 0;
        chairId = 0;
        headerId = 0;
        nickName = "";
        location = "";
    }



public:
    int64_t     userId;	//玩家ID.
    uint32_t    chairId;
    uint8_t     headerId;	//玩家头像.
    string      nickName;	//玩家昵称.
    string      location;

};

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
    //分配手牌给玩家
    void DispenseUserHandCard();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t wChairID, uint8_t cbReason);
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairid, bool bisLookon);
    bool RepayRecordGameScene(int64_t chairid, bool bisLookon);

    //事件接口
public:
    //定时器事件
    //virtual bool  OnTimerMessage(DWORD wTimerID, WPARAM wBindParam);
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
    //私人房游戏大局是否结束
    //virtual bool IsGameRoundClearing(bool bDismissGame);
    //获取大局结算的总分
    //virtual int GetGameRoundTotalScore(int dwChairID);
    //设置桌子库存
    virtual void SetTableStock(int64_t llTotalStock) { m_llStorageControl = llTotalStock; }
    //设置桌子库存下限
    virtual void SetTableStockLowerLimit(int64_t llStockLowerLimit) { m_llStorageLowerLimit = llStockLowerLimit; }
    //设置机器人库存
    //virtual void SetAndroidStock(SCORE llAndroidStock) {};
    //设置机器人库存下限
    //virtual void SetAndStockLowerLimit(SCORE llAndroidStockLowerLimit) {};
    //设置系统通杀概率
    virtual void SetSystemAllKillRatio(int wSystemAllKillRatio) { m_wKillRatio = wSystemAllKillRatio; }
    //设置今日库存值
    virtual void SetTodayStock(int64_t llTodayStock) { m_llTodayStorage = llTodayStock; }
    //设置今日库存上限
    virtual void SetTodayStockHighLimit(int64_t llTodayHighLimit) { m_llStorageHighLimit = llTodayHighLimit; }
    //设置换牌概率
    virtual void SetChangeCardRatio(int wChangeCardRatio) {}

public:
    //读取配置
    // bool ReadConfigInformation();
    //更新机器人配置
    void UpdateConfig();
private:
    //发送数据
    bool SendTableData(int64_t dwChairID, uint8_t wSubCmdID, const void* pBuf=nullptr, int64_t dwLen=0, bool bRecord = true);

    //功能函数
private:
    //初始化游戏数据
    void InitGameData();
//    //更新游戏配置
//    void UpdateGameConfig();
    //清除所有定时器
    void ClearAllTimer();

    void ResetTodayStorage();
    //牌型获取胜率和权值
    // std::pair<double, double>* GetWinRatioAndWeight(uint32_t chairId);
    // //序列化机器人配置
    // void SerialConfig(std::vector<std::pair<double, double>>& robot);
private:

    //游戏事件
private:

    //托管事件
private:

    //机器人相关函数
private:
    //CardProvider c;
    //分析
    int64_t AnalyseAndroidWinScore(bool bAndroidBanker, uint8_t cbCardData[GAME_PLAYER][MAX_COUNT]);

    void PaoMaDeng();
    int IsMaxCard(int wChairID);

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
    // void GameTimerAddScore();
    //玩家自动下注
    // int AutoUserAddScore();
    //玩家下注
    // int OnUserAddScore(int64_t wChairID, int64_t wJetton);

    //发手牌给玩家
    int OnSendHandCard();
    //扑克分析
    void AnalyseCardEx();
//    void AnalyseCard();

    //调换黑名单的牌
    void TrageBlackCard(int64_t chairid);
    void GameTimerOpenCard();
    //玩家自动开牌
    int AutoUserOpenCard();
    //玩家开牌
    int OnUserOpenCard(int wChairID,int cbOpenFlag, uint8_t cbCardData[MAX_COUNT]);
    string GetCardValueName(int index);

    void GameTimerEnd();
    //普通场游戏结算
    void NormalGameEnd(int dwChairID);

    //保存玩家结算结果
    void WriteUserScore();
	//设置当局游戏详情
    void SetGameRecordDetail(int64_t userScore[], int64_t userWinScorePure[], int64_t calcMulti[]);
//    //记录
//    std::string StringCards(uint8_t cbHandCardData[], uint8_t count);


//    //发送总结算
//    void SendTotalClearing(std::string strTime);
//    //解释私人场数据
//    void ParsePrivateData(char *pBuf);

    //1号牌型库
    uint8_t GetCardTypeStorgOne();
    //2号牌型库
    uint8_t GetCardTypeStorgTwo();

    //tmp......
    bool RepayRecorduserinfo(int wChairID);
    bool RepayRecorduserinfoStatu(int wChairID,int status);
    bool RepayRecordStart();
    bool RepayRecordEnd();
    bool RepayRecord(int wChairID,uint8_t wSubCmdId, const char* data, int64_t wSize);
    //获取玩家庄家最大筹码
    bool GetUserMaxAddScore(int64_t wChairID, int64_t lUserJettonScore[5]);
    //获取所有闲家的最大下注倍数
    void GetAllUserMaxAddScore();
    //clear user info
    void ClearUserInfo();
    int ChangCardIndex(uint8_t cbCardData);
    void openSLog(const char *p, ...);
protected:
    //读取库存配置
    void ReadStorageInformation();
    //读取机器人配置
    void ReadConfig();
    //按行读取文件
    void readFile(char const* filename, std::vector<std::string>& lines, char const* skip);
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
    //放大倍数
    int ratioScale_;
    //5/4/3/2/单人局的概率
    int ratio5_, ratio4_, ratio3_, ratio2_, ratio1_;
	//计算桌子要求标准游戏人数
	CWeight poolGamePlayerRatio_;
    //初始化桌子游戏人数概率权重
    void initPlayerRatioWeight();
	void OnTimerGameReadyOver();
    //拼接各玩家手牌cardValue
    std::string StringCardValue();
    //换牌策略分析
    void AnalysePlayerCards();
    //输出玩家手牌/对局日志初始化
    // void ListPlayerCards();
private:
    //系统当前库存
    tagStorageInfo storageInfo_;
    //系统换牌概率
    int ratioSwap_;
    CWeight sysSwapCardsRatio_;
    //系统通杀概率
    int ratioSwapLost_;
    CWeight sysLostSwapCardsRatio_;
    //系统放水概率
    int ratioSwapWin_;
    CWeight sysWinSwapCardsRatio_;
    //发牌时尽量都发好牌的概率，玩起来更有意思
    int ratioDealGood_;
private:
    //IP访问限制
    bool checkIp_;
    //一张桌子真实玩家数量限制
    bool checkrealuser_;
private:
    //chairId对应userID，断线重连时验证
    int64_t                                     saveUserIDs_[GAME_PLAYER];
    bool                                        m_bReplayRecordStart;			//是否开始录像
    //回放的数据现在没有使用 -delete by guansheng 20190316
//    ::Game::Common::GamePlaybackRecford                  m_UserReplayRecord;						//对四个位置用户的数据的记录，用于回放
    int64_t                                     m_iUserWinScore[GAME_PLAYER];
    int                                         m_nLottery;								//发牌次数
    muduo::net::TimerId                         m_TimerIdReadyOver;
    muduo::net::TimerId                         m_TimerIdEnterPlaying;
    muduo::net::TimerId                         m_TimerIdCallBanker;
    // muduo::net::TimerId                         m_TimerIdAddScore;
    muduo::net::TimerId                         m_TimerIdOpenCard;
    muduo::net::TimerId                         m_TimerIdEnd;

    //造牌配置文件路径
    std::string                                 INI_CARDLIST_;
    //机器人配置
    // robotconfig_t                               robot_;
    // 是否写日记
    bool                                        m_bWritelog;

    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;                      //thread.
private:
    //游戏信息
    uint8_t                 m_cbPlayStatus[GAME_PLAYER];						//游戏中玩家
    uint8_t                 m_cbPlayStatusRecord[GAME_PLAYER];                  //游戏中玩家


    atomic_int              m_wPlayCount;										//游戏中玩家个数
    atomic_int              m_wAndroidCount;									//游戏中AI的个数

    int32_t                 m_sCallBanker[GAME_PLAYER];                         //玩家叫庄(-1:未操作, 0:不叫, 1:叫庄)
    int64_t                 m_sAddScore[GAME_PLAYER];							//玩家下注(-1:未下注, 大于-1:对应筹码索引)
    int                     m_sCardType[GAME_PLAYER];							//玩家开牌(-1:未开牌, 大于-1:对应牌型点数)
    int                     m_iMultiple[GAME_PLAYER];							//倍数

    
    uint32_t                m_currentWinUser;                                    //当前最大牌用户
    //玩家标识
    int                     m_wBankerUser;										//庄家
    uint8_t                 m_cbHandCardData[GAME_PLAYER][MAX_SENDCOUNT];       //玩家4张手牌数据
    uint8_t                 m_cbCardData[GAME_PLAYER][MAX_COUNT];				//最优牌组合数据
    uint8_t                 m_cbMaxCard[GAME_PLAYER][MAX_COUNT];					//最大牌组合数据
    //手牌牌型
    uint8_t                 m_handCardsType[GAME_PLAYER];                      // 手牌类型
//    uint8_t                 m_cbSGCardRecord[GAME_PLAYER][MAX_COUNT];           //最大牌组合数据存入数据库
    uint8_t                 m_cbSGRecord[GAME_PLAYER*MAX_SENDCOUNT];           //手牌数据和庄家号（最后一位是庄家号）存入数据库


    int                     PaoMaDengCardType[GAME_PLAYER];
    int                     PaoMaDengWinScore[GAME_PLAYER];

    bool                    m_bPlayerIsOperation[GAME_PLAYER];                  // palyer is Operation
private:
    //房间信息
    int64_t                 m_dwCellScore;										//基础积分
protected:
    string                  m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //

private:
    //扑克信息
    uint8_t					m_cbLeftCardCount; 									//剩余数目
    uint8_t					m_cbRepertoryCard[MAX_CARD_TOTAL];					//库存扑克
private:
    //三公只需要知道开始是否叫庄和可下注 -modify by guansheng 20190424
    int64_t			m_cbMaxJetton[GAME_PLAYER][GAME_JETTON_COUNT];          //闲家最大下注倍数
//    uint8_t                 m_cbCurJetton[GAME_PLAYER];						    //当前下注筹码(庄家不用下注0表示没有下注)

private:
    shared_ptr<ITableFrame>         m_pITableFrame;								//游戏指针
     tagGameRoomInfo*                 m_pGameRoomInfo;
    CGameLogic				m_GameLogic;										//游戏逻辑
    tagScoreInfo			m_RecordScoreInfo[GAME_PLAYER];						//记录积分

    static int64_t		m_llStorageControl;									//库存控制
    static int64_t		m_llStorageLowerLimit;								//库存控制下限值
    static int64_t		m_llStorageHighLimit;								//库存控制上限值
    static int64_t		m_llTodayStorage;									//今日库存
    static bool         m_isReadstorage;
    static bool         m_isReadstorage1;
    static bool         m_isReadstorage2;
    int					m_wKillRatio;
    int64_t				m_iTodayTime;

    std::map<int, std::string>      m_mOutCardRecord;
    std::map<int64_t, shared_ptr<UserInfo>>      m_mUserInfo;
    char							m_RecordOutCard[256];						//出牌记录
    tagGameReplay       m_replay;//game replay
    //玩家出局(弃牌/比牌输)，若离开房间，信息将被清理，用户数据副本用来写记录
    struct userinfo_t {
        uint32_t chairId;
        int64_t userId;
        uint8_t headerId;
        std::string nickName;
        std::string location;
        std::string account;
        uint32_t agentId;
        //携带积分
        int64_t userScore;
        //是否机器人
        bool isAndroidUser;
    };
    typedef std::map<uint32_t, userinfo_t> UserInfoList;
    UserInfoList userinfos_;
    //是否有效玩家
    inline bool IsExistUser(uint32_t chairId, int64_t* puserId = NULL) {
        UserInfoList::const_iterator it = userinfos_.find(chairId);
        if (it != userinfos_.end()) {
            if (puserId) {
                *puserId = it->second.userId;
            }
            return true;
        }
        return false;
    }

    std::map<std::string, std::string> m_UserTest; //first string strAccount, second string value
private:
    uint32_t                m_dwInitTime;
    uint32_t                m_dwReadyTime;										//ready时间
    uint32_t                m_dwEnterPlayingTime;						        //EnterPlaying时间
    uint32_t  				m_dwCallTime;										//叫庄时间
    uint32_t 				m_dwScoreTime;										//下注时间
    uint32_t 				m_dwOpenTime;										//开牌时间
    uint32_t 				m_dwEndTime;										//结束时间
    uint8_t                 m_cbStatus;                                         //桌子的游戏状态  
    double                  stockWeak;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
};

//////////////////////////////////////////////////////////////////////////

