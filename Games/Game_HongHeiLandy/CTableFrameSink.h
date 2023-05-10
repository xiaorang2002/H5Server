#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE


#define GAME_PLAYER						10000


//#define CWHArray std::deque

#define MAX_CARDS                  52
#define CHIP_COUNT                 5

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
		memset(dLastJetton, 0, sizeof(dLastJetton));
    }

    void clear()
    {
        memcpy(m_lPreUserJettonScore,m_lUserJettonScore,sizeof(m_lUserJettonScore));
        memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
        memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
        m_EndWinScore = 0;
        m_Revenue = 0;
		memset(dAreaWin, 0, sizeof(dAreaWin));
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


    int64_t             m_lUserJettonScore[3];          // 个人总注
    int64_t             m_lRobotUserJettonScore[3];     // robot 个人总注
    int64_t             m_lPreUserJettonScore[3];       //previous game jetton
    int64_t             m_EndWinScore;                  // 前端显示输赢
    int64_t             m_Revenue;

    list<int64_t>       m_JettonScore;                  //20局下注
    list<int64_t>       m_WinScore;                     //20局输赢
    int32_t             NotBetJettonCount;              // 连续不下注的局数
	int64_t				dAreaWin[3];					//各区域的输赢
    int64_t				dLastJetton[3];
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

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct JettonInfo
{
    bool bJetton =false;
    int64_t  iAreaJetton[MAX_COUNT]={0};
};
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
    //读取配置
    bool  OnMakeDir();
//    游戏状态

public:
    void OnTimerJetton();
    void OnTimerEnd();

	//游戏事件
protected:
    void openLog(const char *p, ...);
    //根据系统输赢发牌
    void GetOpenCards();
    void ClearTableData();

    string GetCardValueName(int index);
    //改变玩家下注输赢信息
    void ChangeUserInfo();

    //加注事件
    bool OnUserAddScore(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore , bool bRejetton=false);
    bool OnUserRejetton(uint32_t chairId);
    void AddPlayerJetton(int64_t chairid,int areaid,int score);
    void SendPlaceBetFail(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore,string str_err);
    //pPlayerList
//    bool OnPlayerList(uint32_t chairId);
    void SendOpenRecord(uint32_t chairId);
    bool SendPmdMsg(tagScoreInfo arScore[]);
    void SendStartMsg(uint32_t chairId);
    void ChangeVipUserInfo();
    //void SendStartjettonMsg(dword wChairID);
    void SendEndMsg(uint32_t chairId);

    bool isInVipList(int64_t chairid);
    void GameTimerJettonBroadcast();
    //保存玩家结算结果
    void WriteUserScore();

	//游戏即将开始倒计时时间
//    void AutoStartRemainTime(uint8_t cbReason);

//	void KickPlayerNoOperate();
    void clearTableUser();
    void SendWinSysMessage();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);

	void updateGameMsg(bool isInit);
	//获取游戏状态对应的剩余时间
	int32_t getGameCountTime(int gameStatus);
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);
	//续压
	void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	//功能函数
protected:
//    //	扑克分析
////	void AnalyseStartCard();
//	//黑白名单
//	void ListControl();

protected:


public:
//    CString					   m_GameRecord;

    //总下注数
protected:
    int64_t                          m_lAllJettonScore[3];		        //全体总注
    //玩家真实下注
    int64_t                          m_lTrueUserJettonScore[3];     	//玩家真实下注
    int64_t                          m_lRobotserJettonAreaScore[3];	    //robot下注

    //个人下注
protected:

    uint32_t                         m_ShenSuanZiId;
    int64_t                          m_lShenSuanZiJettonScore[3];

    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    multiset<shared_ptr<UserInfo>, ClassComp> m_SortUserInfo;
//    list<int64_t>                    m_JettonScore[GAME_PLAYER];           //20局下注
//    list<int64_t>                    m_WinScore[GAME_PLAYER];              //20局输赢
    deque<int>                      m_openRecord;                         //Long Hu He
    deque<int>                      m_openRecordType;
    //list<duint16_t>                 m_clearUser;


    map<int64_t, uint32_t>           m_Special;
    map<int64_t, uint32_t>           m_Hong;
    map<int64_t, uint32_t>           m_Hei;

    int64_t                          m_HongTimes;                       //Long Times
    int64_t                          m_HeiTimes;                         //Hu Times
    int64_t                          m_SpecialTimes[6];                         //He Times
    int64_t                          m_AreaMaxJetton[3];
	int64_t                          m_nMuilt[3];		
	int32_t							 m_bAreaIsWin[3];					//各区域的输赢

    uint32_t                         m_IsEnableOpenlog;                 //is enable open log.
    uint32_t                         m_IsXuYaoRobotXiaZhu;              //算开奖结果是否需要机器人下注
    int64_t                          m_WinScoreSystem;                  //pao ma deng
//    int                               m_RobotMaxCount;

    int64_t                          m_nPlaceJettonTime;				//下注时间 All Jetton Time
    int64_t                          m_nPlaceJettonLeveTime;			//Left 下注时间 Left Jetton Time
    float_t                          m_ngameTimeJettonBroadcast;

    int64_t                          m_nGameEndTime;				    //结束时间 All End Time
    int64_t                          m_nGameEndLeveTime;			  	//Left 结束时间 Left End Time

    list<uint8_t>                    m_listTestCardArray;

    double                           m_dControlkill;
    int64_t                          m_lLimitPoints;
protected:

    uint8_t							m_cbTableCardArray[6];                   //桌面扑克
    //    uint8_t                         m_cbCurrentRoundTableCards[2];		        //桌面扑克
    static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
    vector<uint8_t>                 m_CardVecData;                    //All Cards 52*8

    uint8_t                            m_TableCardType[2];
    uint8_t                            m_winType;
    uint8_t                            m_winFlag;
    uint8_t                            m_wincardValue;


    uint64_t                m_duiZiCount;
    uint64_t                m_shunZiCount;
    uint64_t                m_jinHuaCount;
    uint64_t                m_shunJinCount;
    uint64_t                m_baoZiCount;
    uint64_t                m_gameRoundCount;
    uint64_t                m_gameWinCount;
    float                   m_winPercent;

    int64_t                 m_curSystemWin;                 //本次系统赢输结果
    JettonInfo m_arr_jetton[GAME_PLAYER];
    int64_t  m_iarr_tmparea_jetton[MAX_COUNT]={0};

    JettonInfo tmpJttonList[GAME_PLAYER];
    int64_t iarr_tmparea_jetton[MAX_COUNT]={0};


    int64_t                         m_userScoreLevel[4];
    int64_t                         m_userChips[4][CHIP_COUNT];
    int64_t                         m_currentchips[GAME_PLAYER][CHIP_COUNT];
    int                             useIntelligentChips;
protected:
    string                          m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //
    chrono::system_clock::time_point  m_endTime ; //
    bool                            m_bControl;
private:
    tagGameRoomInfo *               m_pGameRoomInfo;
    tagGameReplay       m_replay;//game replay
    uint32_t                m_dwReadyTime;

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
private:
    //CGameLogic                      m_GameLogic;						//游戏逻辑

    RedBlackAlgorithm               m_Algorithm;
    shared_ptr<ITableFrame>         m_pITableFrame;						//框架接口

    mutable boost::shared_mutex     m_mutex;

private:
    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
	muduo::net::TimerId                         m_TimerJettonBroadcast;
	muduo::net::TimerId                         m_TimerIdUpdateGameMsg;
	muduo::net::TimerId							m_TimerIdResetPlayerList;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    double                                      stockWeak;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制

    vector<int64_t>				   m_vAllchips;
	float					   m_fUpdateGameMsgTime;			                  //更新游戏信息的时间	
	bool					   m_bGameEnd;										  //本局是否结束
	int						   m_cbGameStatus;									  //游戏状态
	float					   m_fResetPlayerTime;						          //本局游戏结束刷新玩家排序的时间
	int32_t 				   m_winOpenCount[MAX_COUNT];				          //各下注项累计开奖的局数(0:特殊 1:黑 2:红)
	int64_t					   m_lLimitScore[2];								  //限红
	string          		   m_strTableId;									  //桌号
	int32_t					   m_ipCode;
	bool					   m_bAddRecord;									  //是否包含了新结果
};

#endif
