#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE

#define GAME_PLAYER						10001

//#define CWHArray std::deque

#define MAX_CARDS                  52


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

        lTwentyAllUserScore = 0;
        lTwentyWinScore = 0;
        lTwentyWinCount = 0;

        isDivineMath = false;

        memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
        memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
        memset(m_EndWinJettonScore, 0, sizeof(m_EndWinJettonScore));

        m_EndWinScore = 0;
        m_Revenue = 0;

        m_JettonScore.clear();
        m_WinScore.clear();
        NotBetJettonCount = 0;
    }

    void clear()
    {
        memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
        memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
        memset(m_EndWinJettonScore, 0, sizeof(m_EndWinJettonScore));

        m_EndWinScore = 0;
        m_Revenue = 0;
    }

public:
    int64_t     userId;	//玩家ID.
    uint32_t    chairId;
    uint8_t     headerId;	//玩家头像.
    string      nickName;	//玩家昵称.
    string      location;

    int64_t     lTwentyAllUserScore;	//20局总下注
    int64_t     lTwentyWinScore;        //20局嬴分值. 在线列表显示用
    uint8_t     lTwentyWinCount;        //20局嬴次数. 在线列表显示用

    bool        isDivineMath;           //是否为神算子 (0,1是)


    int64_t             m_lUserJettonScore[5];          // 个人总注
    int64_t             m_lRobotUserJettonScore[5];     // robot 个人总注
    int64_t             m_EndWinScore;                  // 前端显示输赢
    int64_t             m_Revenue;
    int64_t             m_EndWinJettonScore[5];         // 前端显示区域输赢

    list<int64_t>       m_JettonScore;                  //20局下注
    list<int64_t>       m_WinScore;                     //20局输赢

    int32_t             NotBetJettonCount; // 连续不下注的局数

};

struct ClassComp
{
  bool operator() (const shared_ptr<UserInfo>& lhs, const shared_ptr<UserInfo>& rhs) const
  {
      int64_t jettonLhs = 0;
      int64_t jettonRhs = 0;
      for(auto &it : lhs->m_JettonScore)
          jettonLhs += it;
      for(auto &it : rhs->m_JettonScore)
          jettonRhs += it;

      return jettonLhs>jettonRhs;
  }
};


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct JettonInfo {
    bool bJetton =false;
    int64_t  iAreaJetton[5]={0,0,0,0,0};
};

template<typename T>
std::vector<T> to_array(const std::string& s)
{
    std::vector<T> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        result.push_back(boost::lexical_cast<T>(item));
    }
    return result;
}
//游戏桌子类
class CTableFrameSink :public ITableFrameSink
{
    //函数定义
public:
    //构造函数
    CTableFrameSink();
    //析构函数
    virtual ~CTableFrameSink();

public:
    //游戏开始
    virtual void OnGameStart() override;
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag) override;
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon) override;

    virtual bool OnUserEnter(int64_t userId, bool islookon) override;
    //用户起立
    virtual bool OnUserLeft(int64_t userId, bool islookon) override;
    //用户同意    
    virtual bool OnUserReady(int64_t userId, bool islookon) override;

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser) override;
    virtual bool CanLeftTable(int64_t userId) override;

    //游戏消息处理
    virtual bool OnGameMessage(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize) override;

    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;
    //复位桌子
    virtual void  RepositionSink() override {}

public:
//    bool clearTableUserId(uint32_t chairId);
    //读取配置
    bool ReadConfigInformation();
    //读取牌值文件
//    bool ReadCardFile();
    //读取配置
//    void ReadConfigInformation(bool bReadFresh);
    //设置路径
    bool  OnMakeDir();
public:
    void OnTimerJetton();
    void OnTimerEnd();

    //游戏事件
protected:
    //根据系统输赢发牌
    void GetOpenCards();
    void MakeUserWin(int32_t weight=0);
    void MakeUserLose(int32_t weight=0);
    void SafeSwitch();
    void ClearTableData();

    //改变玩家下注输赢信息
    void ChangeUserInfo();

    //加注事件
    bool OnUserAddScore(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore);
    void AddPlayerJetton(int64_t chairId,uint8_t cbJettonArea,int64_t lJettonScore);
    void SendPlaceBetFail(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore, int32_t cbErrorcode = 0);
    //pPlayerList
//    bool OnPlayerList(uint32_t chairId);
    void SendOpenRecord(uint32_t chairId);
    bool SendPmdMsg(tagScoreInfo arScore[]);
    void SendStartMsg(uint32_t chairId);
    void ChangeVipUserInfo();
    //void SendStartjettonMsg(dword wChairID);
    void SendEndMsg(uint32_t chairId);

    bool isInVipList(uint32_t chairId);
    void GameTimerJettonBroadcast();
    //保存玩家结算结果
    void WriteUserScore();

    void clearTableUser();
    void SendWinSysMessage();

	//用户申请上庄
	bool OnUserApplyBanker(uint32_t chairId, bool applyOrCancel);
	//用户申请下庄
	bool OnUserCancelBanker(uint32_t chairId);
	//是否需要换庄
	bool OnGameChangeBanker();
	// 申请上庄玩家列表
	bool OnApplyBankerList();
	// 清理上庄玩家列表中钱不够的玩家
	void ClearNotEnoughScoreInApplyBankerList();
    //功能函数
protected:
////    百人牛牛控制相关函数
    //获取玩家的总下注
    int64_t GetTotalUserJettonScore();
    //是否有用户下注
    bool IsUserBeted();
    //计算保守盘口极限
    int64_t CalculateSafeHandicapLimit();
    //计算押注比例类型(A(1): 最大区域押注组超过总注50% B(2): 最大两组之和超过总注的67% C(3): 其他属于均匀情况)
    uint8_t CalculateAreaBetRatioType();
    //排序后的牌值
    void SetSortCardInfo();
    //排序后的下注信息
    void SetSortAreaBetInfo();
    //计算押注盘口值
    int64_t CalculateBetHandicap();
    //计算潜在赢家的计算
    int GetRandData();
    void AllocationCard(uint8_t byCardBuffer[], int nLen, int nBankerIndex);
    //Read storage information
    void ReadStorageInfo();
    //Calculate current wins and decide whether exchang cards
    int64_t CurrentWin();
    //连续5局未下注踢出房间
    void CheckKickOutUser();

    void  openLog(const char *p, ...);
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);
protected:


public:
//    CString					   m_GameRecord;

    //总下注数
protected:
    int64_t                          m_lAllJettonScore[5];		        //全体总注
	int64_t						     m_lBankerAllEndScore[5];		    //庄家各区域的赢分
    //玩家真实下注
    int64_t                          m_lTrueUserJettonScore[5];     	//玩家真实下注
    int64_t                          m_lRobotserJettonAreaScore[5];	    //robot下注
    int64_t                             m_lMaxLose;//safe trigger lose score

    //个人下注
protected:
    int64_t                          m_lShenSuanZiJettonScore[5];
    int64_t                          m_ShenSuanZiId;

    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    multiset<shared_ptr<UserInfo>, ClassComp> m_SortUserInfo;
    vector<int>                      m_openRecord;                         //Long Hu He

    map<int64_t, uint32_t>           m_Tian;
    map<int64_t, uint32_t>           m_Di;
    map<int64_t, uint32_t>           m_Xuan;
    map<int64_t, uint32_t>           m_Huang;

    uint32_t                         m_IsEnableOpenlog;                 //is enable open log.
    uint32_t                         m_IsXuYaoRobotXiaZhu;              //算开奖结果是否需要机器人下注
    int64_t                          m_WinScoreSystem;                  //pao ma deng
    vector<int32_t>                  m_winRates;
    vector<int32_t>                  m_lostRates;
    int32_t                         m_interfere_rate;
    int32_t                         m_interfere_line;
    int64_t                          m_nPlaceJettonTime;				//下注时间 All Jetton Time
//    int64_t                          m_nPlaceJettonLeveTime;			//Left 下注时间 Left Jetton Time
    int64_t                          m_nGameTimeJettonBroadcast;

    int64_t                          m_nGameEndTime;				    //结束时间 All End Time
//    int64_t                          m_nGameEndLeveTime;			  	//Left 结束时间 Left End Time

protected:

//    IServerUserItem               * m_pUserItem;						        //框架接口
    //const tagGameServiceOption		* m_pGameServiceOption;				//配置参数
//    CHistoryScore			        m_HistoryScore;						    	//历史成绩

//    uint8_t						    m_cbPlayStatus[GAME_PLAYER];		    //游戏状态

//    uint8_t						  m_cbCardCount[4];						    //扑克数目
    uint8_t                         m_cbCurrentRoundTableCards[5][5];		    //桌面扑克
    uint8_t                         m_cbTempTableCards[5][5];                   //桌面扑克
    static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
    vector<uint8_t>                 m_CardVecData;                              //All Cards 52
    uint8_t                         m_cbSortAreaCard[5];                        //区域扑克排行
    int                             m_nTempTimes[5];                            //排序后的倍数
    int64_t                         m_lTempSortBet[5];                          //排序后的下注
//    int                             m_nTempBetIdx[5];                           //排序后的索引
    bool                            m_bTongShaFlag;                             //是否上把配牌通杀
    int64_t                         m_openCardCount;



    BrnnAlgorithm                   brnnAlgorithm;
    int                             m_allMultical[4];                           //天地玄黄的倍数
    int                             m_allCardType[5];                           //桌面所有牌的牌型
    JettonInfo m_array_jetton[GAME_PLAYER];
    int64_t  m_array_tmparea_jetton[MAX_COUNT];
    vector<uint8_t>                  m_listTestCardArray;
//    BrnnControlParam                 m_BrnnParam;


    int64_t                         m_userScoreLevel[4];
    int64_t                         m_userChips[4][7];
    int64_t                         m_currentchips[GAME_PLAYER][7];
    int64_t                         useIntelligentChips;

protected:
    string                          m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //
    chrono::system_clock::time_point  m_endTime;

private:
    tagGameRoomInfo *               m_pGameRoomInfo;
    tagGameReplay           m_replay;//game replay
    uint32_t                m_dwReadyTime;

private:
    //CGameLogic                      m_GameLogic;						//游戏逻辑
    shared_ptr<ITableFrame>         m_pITableFrame;						//框架接口

//    mutable boost::shared_mutex     m_mutex;

private:
    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
    muduo::net::TimerId                         m_TimerJettonBroadcast;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;
private://Storage Related
    static int64_t		m_llStorageControl;									//库存控制
    static int64_t		m_llStorageLowerLimit;								//库存控制下限值
    static int64_t		m_llStorageHighLimit;								//库存控制上限值
    static int64_t      m_llStorageAverLine;          //average line
    static int64_t      m_llGap;            //refer gap:for couting rate

    double              stockWeak;
    double                          m_dControlkill;
    int64_t                         m_lLimitPoints;

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制

	// 玩家坐庄信息
	//map<int64_t, shared_ptr<UserInfo>>  m_UserBankerList;
	vector<shared_ptr<UserInfo>>		m_UserBankerList;
	WORD								m_wBankerUser;						//当前庄家用户
	int32_t								m_wBankerLimitCount;				//限制坐庄的最大局数
	int32_t								m_wBankerContinuityCount;			//连续坐庄局数
	int64_t								m_wBankerLimitScore;				//上庄分数限制
	int32_t								m_wBankerApplyLimitCount;			//限制申请上庄人数
	bool								m_buserCancelBanker;				//当前庄家选择了下庄
	int32_t								m_bOpenBankerApply;					//是否开发玩家上庄功能 1:开放 0:不开放

};

//////////////////////////////////////////////////////////////////////////


#endif
