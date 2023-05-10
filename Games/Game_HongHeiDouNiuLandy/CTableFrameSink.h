#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE

#define GAME_PLAYER				   10000
#define MAX_CARDS                  52

//enum BetArea{
//    HE              = 0,        // 和
//    BLACK           = 1,        // 黑
//    RED             = 2,        // 紅
//    NIU_1           = 3,        // 牛一
//    NIU_2           = 4,        // 牛二
//    NIU_3           = 5,        // 牛三
//    NIU_4           = 6,        // 牛四
//    NIU_5           = 7,        // 牛五
//    NIU_6           = 8,        // 牛六
//    NIU_7           = 9,        // 牛七
//    NIU_8           = 10,       // 牛八
//    NIU_9           = 11,       // 牛九
//    NIU_NIU         = 12,       // 牛牛
//    NIU_SPECIAL     = 13,       // 特殊牛
//
//    MAX_AREA        = 14,
//};
struct tagBetInfo
{
	int32_t cbJettonArea;
	int64_t lJettonScore;
};

struct JettonInfo
{
	bool bJetton = false;
    int64_t  iAreaJetton[MAX_AREA] = { 0 };
};
//玩家info
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
        memset(m_lPreUserJettonScore,0,sizeof(m_lPreUserJettonScore));
        m_EndWinScore = 0;
        m_Revenue = 0;

        m_JettonScore.clear();
        m_WinScore.clear();
        NotBetJettonCount = 0;
		memset(dAreaWin, 0, sizeof(dAreaWin));
		JettonValues.clear();
		lTotalJettonScore = 0;
    }

    void clear()
    {
        memcpy(m_lPreUserJettonScore,m_lUserJettonScore,sizeof(m_lUserJettonScore));
        memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
        memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
        m_EndWinScore = 0;
        m_Revenue = 0;
		memset(dAreaWin, 0, sizeof(dAreaWin));
		lTotalJettonScore = 0;
    }
	void clearJetton()
	{
		JettonValues.clear();
	}

public:
    int64_t				userId;	//玩家ID.
    uint32_t			chairId;
    uint8_t				headerId;	//玩家头像.
    string				nickName;	//玩家昵称.
    string				location;

    int64_t				lTwentyAllUserScore;	//20局总下注
    int64_t				lTwentyWinScore;        //20局嬴分值. 在线列表显示用
    uint8_t				lTwentyWinCount;        //20局嬴次数. 在线列表显示用

    bool				isDivineMath;           //是否为神算子 (0,1是)


    int64_t             m_lUserJettonScore[MAX_AREA];          // 个人总注
    int64_t             m_lRobotUserJettonScore[MAX_AREA];     // robot 个人总注
    int64_t             m_lPreUserJettonScore[MAX_AREA];       //previous game jetton
    int64_t             m_EndWinScore;                  // 前端显示输赢
    int64_t             m_Revenue;

    list<int64_t>       m_JettonScore;                  //20局下注
    list<int64_t>       m_WinScore;                     //20局输赢
    int32_t             NotBetJettonCount;              // 连续不下注的局数
	int64_t				dAreaWin[MAX_AREA];				//各区域的输赢
	vector<tagBetInfo>  JettonValues;					//所有押分筹码值
	int64_t				lTotalJettonScore;				//总押注
};

struct ClassComp
{
  bool operator() (const shared_ptr<UserInfo>& lhs, const shared_ptr<UserInfo>& rhs) const
  {
      int64_t jettonLhs = 0;
      int64_t jettonRhs = 0;
      if(lhs->isDivineMath||rhs->isDivineMath)
          return lhs->isDivineMath;

      for(auto &it : lhs->m_JettonScore)
          jettonLhs += it;
      for(auto &it : rhs->m_JettonScore)
          jettonRhs += it;
      return jettonLhs>jettonRhs;
  }
};

//////////////////////////////////////////////////////////////////////////
//游戏桌子类
class CTableFrameSink :public ITableFrameSink
{
public:
	// 能取上限
	static int RandomInt(int a, int b)
	{
		return rand() % (b - a + 1) + a;
	}
	//函数定义
public:
	//构造函数
	CTableFrameSink();
	//析构函数
	virtual ~CTableFrameSink();

public:
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag);
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon);
    virtual bool OnUserEnter(int64_t userId, bool islookon);
    //用户起立
    virtual bool OnUserLeft(int64_t userId, bool islookon);
    //用户同意
    virtual bool OnUserReady(int64_t userId, bool islookon);
    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
    virtual bool CanLeftTable(int64_t userId);
    //游戏消息处理
    virtual bool OnGameMessage(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize);
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
    //复位桌子
    virtual void  RepositionSink() {}

public:
    //读取配置
    bool ReadConfigInformation();
    bool ReadCardConfig();
    //游戏状态

public:
    void OnTimerJetton();
    void OnTimerEnd();

	//游戏事件
protected:
    void openLog(const char *p, ...);
    //根据系统输赢发牌
    void GetOpenCards();
	//系统控制是否有黑名单时开牌
	void BlackPlayerGetOpenCards(tagStorageInfo storageInfo, uint8_t &heiType, uint8_t &hongType);
    void ClearTableData();

    string GetCardValueName(int index);
    //改变玩家下注输赢信息
    void ChangeUserInfo();

    //加注事件
    bool OnUserAddScore(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore , bool bRejetton=false);
    bool OnUserRejetton(uint32_t chairId);
    void AddPlayerJetton(int64_t chairid,int areaid,int64_t score);
    void SendPlaceBetFail(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore,string str_err);
	//续压
	void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    //pPlayerList
    //bool OnPlayerList(uint32_t chairId);
    void SendOpenRecord(uint32_t chairId);
    void SendStartMsg(uint32_t chairId);
    void ChangeVipUserInfo();
    //void SendStartjettonMsg(dword wChairID);
    void SendEndMsg(uint32_t chairId);

    bool isInVipList(int64_t chairid);
    void GameTimerJettonBroadcast();
    //保存玩家结算结果
    void WriteUserScore();

	//游戏即将开始倒计时时间
    //void AutoStartRemainTime(uint8_t cbReason);

    void clearTableUser();
    void SendWinSysMessage();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);
	// 是否需清零今日总局
	bool isNeedClearTodayRoundCount();
	//清理今日开奖统计
	void ClearTodayRoundCount();
	string GetAreaName(int index);
	//测试游戏数据
	void TestGameMessage(uint32_t wChairID, int int32_t);
	void updateResultList(int resultId);
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
	//功能函数
protected:
    //	扑克分析
	//void AnalyseStartCard();
	//黑白名单
	//void ListControl();

protected:


public:
    //CString					   m_GameRecord;

    //总下注数
protected:
    int64_t                          m_lAllJettonScore[MAX_AREA];		        //全体总注
    //玩家真实下注
    int64_t                          m_lTrueUserJettonScore[MAX_AREA];     	//玩家真实下注
    int64_t                          m_lRobotserJettonAreaScore[MAX_AREA];	    //robot下注

    //个人下注
protected:

    uint32_t                         m_ShenSuanZiId;
    int64_t                          m_lShenSuanZiJettonScore[MAX_AREA];

    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    multiset<shared_ptr<UserInfo>, ClassComp> m_SortUserInfo;
    //list<int64_t>                    m_JettonScore[GAME_PLAYER];           //20局下注
    //list<int64_t>                    m_WinScore[GAME_PLAYER];              //20局输赢
    deque<int>                      m_openRecord;                         //Long Hu He
    deque<int>                      m_openRecordType;
    //list<duint16_t>                 m_clearUser;

	map<int64_t, uint32_t>           m_AreaJetton[MAX_AREA];			//各区域下注的筹码的个数

	double                           m_HeTimes;                          //He Times     0
	double                           m_HeiTimes;                         //Hei  Times   1
	double                           m_HongTimes;                        //Hong Times   2
	double                           m_NiuOneTimes;                      //牛1  Times
	double                           m_NormalTimes;                      //常規牛 Times[牛2~牛牛]
	double                           m_SpecialTimes;                     //特殊牛 Times[四花牛、五花牛、四炸、五小牛]
	double                           m_nMuilt[MAX_AREA];

    int64_t                          m_AreaMaxJetton[MAX_AREA];	
    int64_t							 m_bAreaIsWin[MAX_AREA];			//各区域的输赢

    uint32_t                         m_IsEnableOpenlog;                 //is enable open log.
    uint32_t                         m_IsXuYaoRobotXiaZhu;              //算开奖结果是否需要机器人下注

    int64_t                          m_nPlaceJettonTime;				//下注时间 All Jetton Time
    int64_t                          m_nPlaceJettonLeveTime;			//Left 下注时间 Left Jetton Time
    float_t                          m_ngameTimeJettonBroadcast;

    int64_t                          m_nGameEndTime;				    //结束时间 All End Time
    int64_t                          m_nGameEndLeveTime;			  	//Left 结束时间 Left End Time

    list<uint8_t>                    m_listTestCardArray;

    double                           m_dControlkill;
    int64_t                          m_lLimitPoints;
protected:

    uint8_t							m_cbTableCardArray[2*MAX_COUNT];                   //桌面扑克
	uint8_t							m_cbOxCard[2][MAX_COUNT];			//排列后的牌
    static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
    vector<uint8_t>                 m_CardVecData;                    //All Cards 52*8

	uint8_t                         m_TableCardType[2] = { 0 };
    uint8_t                         m_winType;
    uint8_t                         m_winFlag;

    uint64_t                        m_gameRoundCount;
    uint64_t                        m_gameWinCount;
	uint64_t                        m_areaWinCount[MAX_AREA] = { 0 };		//各区域开的次数
	uint64_t                        m_todayGameRoundCount;				//今日总局
    float                           m_winPercent;

    int64_t                         m_curSystemWin;                 //本次系统赢输结果
    JettonInfo                      m_arr_jetton[GAME_PLAYER];
    int64_t                         m_iarr_tmparea_jetton[MAX_AREA]={0};

    JettonInfo                      tmpJttonList[GAME_PLAYER];
    int64_t                         iarr_tmparea_jetton[MAX_AREA]={0};


    int64_t                         m_userScoreLevel[4];
    int64_t                         m_userChips[4][7];
    int64_t                         m_currentchips[GAME_PLAYER][7];
    int64_t                         useIntelligentChips;
protected:
    string                          m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //
    chrono::system_clock::time_point  m_endTime ; //
    bool                            m_bControl;
private:
    tagGameRoomInfo *               m_pGameRoomInfo;
    tagGameReplay                   m_replay;//game replay
    uint32_t                        m_dwReadyTime;

private:
    //CGameLogic                      m_GameLogic;						//游戏逻辑

    HhdnAlgorithm                   m_hhdnAlgorithm;
    shared_ptr<ITableFrame>         m_pITableFrame;						//框架接口

    mutable boost::shared_mutex     m_mutex;

private:
    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
    muduo::net::TimerId                         m_TimerJettonBroadcast;
	muduo::net::TimerId                         m_TimerIdJudgeTime;		//判断当前时间
	muduo::net::TimerId							m_TimerIdTest;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    double                                      stockWeak;
	int32_t										m_bTestGame;			//测试
	bool										m_bWriteLvLog;			
	int32_t										m_nTestType;
	int32_t										m_nTestTimes;
	bool										m_bTestAgain;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
	int32_t 				   m_notOpenCount[MAX_AREA];				//各下注项连续未开累计的局数
	int32_t 				   m_notOpenCount_last[MAX_AREA];			//各下注项连续未开累计的局数
};

#endif
