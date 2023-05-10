#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE



#define GAME_PLAYER						10001



//#define CWHArray std::deque

#define MAX_CARDS                  54*8

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
        memset(m_lUserLastJettonScore, 0, sizeof(m_lUserLastJettonScore));

        m_EndWinScore = 0;
        m_Revenue = 0;

        m_JettonScore.clear();
        m_WinScore.clear();
        NotBetJettonCount = 0;
		memset(dAreaWin, 0, sizeof(dAreaWin));
    }

    void clear()
    {
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

    int64_t             m_lUserJettonScore[Max];          // 个人总注
    int64_t             m_lRobotUserJettonScore[Max];     // robot 个人总注
    int64_t             m_lUserLastJettonScore[Max];     // robot 个人总注
    int64_t             m_EndWinScore;                  // 前端显示输赢
    int64_t             m_Revenue;

    list<int64_t>       m_JettonScore;                  //20局下注
    list<int64_t>       m_WinScore;                     //20局输赢
    int32_t             NotBetJettonCount; // 连续不下注的局数
    int64_t				dAreaWin[Max];					//各区域的输赢
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

struct JettonInfo
{
    bool    bJetton = false;
    int64_t areaJetton[5]={0,0,0,0,0};
};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
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
    virtual void OnGameStart();
    //游戏结束
//     virtual bool OnEventGameConclude(uint16_t wChairID, IServerUserItem * pIServerUserItem, uint8_t cbReason);
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag);
    //发送场景
//	virtual bool OnEventSendGameScene(uint16_t wChiarID, IServerUserItem * pIServerUserItem, uint8_t bGameStatus, bool bSendSecret);
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon);

    //用户断线
    // virtual bool OnActionUserOffLine(uint16_t wChairID, IServerUserItem * pIServerUserItem);
    //用户重入
    // virtual bool OnActionUserConnect(uint16_t wChairID, IServerUserItem * pIServerUserItem);
    //用户坐下
    // virtual bool OnActionUserSitDown(uint16_t wChairID,IServerUserItem * pIServerUserItem, bool bLookonUser);
    virtual bool OnUserEnter(int64_t userId, bool islookon);
    //用户起立
    //virtual bool OnActionUserStandUp(uint16_t wChairID,IServerUserItem * pIServerUserItem, bool bLookonUser);
    virtual bool OnUserLeft(int64_t userId, bool islookon);
    //用户同意
    // virtual bool OnActionUserOnReady(uint16_t wChairID, IServerUserItem * pIServerUserItem, VOID * pData, uint16_t wDataSize);
    virtual bool OnUserReady(int64_t userId, bool islookon);

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
    virtual bool CanLeftTable(int64_t userId);

    //游戏消息处理
    // virtual bool  OnGameMessage(uint16_t wSubCmdID, VOID * pData, uint16_t wDataSize, IServerUserItem * pIServerUserItem);
    virtual bool OnGameMessage(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize);

    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
    //复位桌子
    virtual void  RepositionSink() {}

public:
//    bool clearTableUserId(uint32_t chairId);
    //读取配置
    bool ReadConfigInformation();
    //读取配置
//    void ReadConfigInformation(bool bReadFresh);
    //设置路径
    bool  OnMakeDir();
//    游戏状态
//    virtual bool  IsUserPlaying(uint32_t chairId);

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
    bool OnUserAddScore(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore);


    bool OnUserAddScore1(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore);

    void AddPlayerJetton(int64_t chairId, uint8_t areaId, int64_t score);
    void SendPlaceBetFail(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore, int32_t errCode);
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

	//游戏即将开始倒计时时间
//    void AutoStartRemainTime(uint8_t cbReason);

//	void KickPlayerNoOperate();
    void clearTableUser();
    void SendWinSysMessage();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);
	//功能函数
    void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);

    struct Result
    {
        Result()
        {
            wintag=0;
            card=0;
        }
        int wintag;
        int card;
    };


protected:


public:
//    CString					   m_GameRecord;

    //总下注数
protected:
    int64_t                          m_lAllJettonScore[Max];		        //全体总注
    //玩家真实下注
    int64_t                          m_lTrueUserJettonScore[Max];     	//玩家真实下注
    int64_t                          m_lRobotserJettonAreaScore[Max];	    //robot下注
    int64_t                          m_AreaMaxJetton[Max];

    //个人下注
protected:
    int64_t                          m_ShenSuanZiId;
    int64_t                          m_lShenSuanZiJettonScore[Max];

    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    multiset<shared_ptr<UserInfo>, ClassComp> m_SortUserInfo;
    vector<Result>                      m_openRecord;                         //hei hong mei fang  wang

    map<int64_t, uint32_t>           m_Hei;                           //黑桃区域下注筹码统计
    map<int64_t, uint32_t>           m_Hong;                          //红桃区域下注筹码统计
    map<int64_t, uint32_t>           m_Mei;                           //梅花区域下注筹码统计
    map<int64_t, uint32_t>           m_Fang;                          //方块区域下注筹码统计
    map<int64_t, uint32_t>           m_Wang;                          //王牌区域下注筹码统计

    bool                             m_bOpenContorl;

    bool                             m_isTest;
    vector<uint8_t>                  m_testCards;
    float                          m_HeiTimes;                       //黑桃倍数
    float                          m_HongTimes;                      //红桃倍数
    float                          m_MeiTimes;                       //梅花倍数
    float                          m_FangTimes;                      //方块倍数
    float                          m_WangTimes;                      //王牌倍数

    uint32_t                         m_IsEnableOpenlog;                 //is enable open log.
    uint32_t                         m_IsXuYaoRobotXiaZhu;              //算开奖结果是否需要机器人下注
    int64_t                          m_WinScoreSystem;                  //pao ma deng
//    int                               m_RobotMaxCount;

    int64_t                          m_nPlaceJettonTime;				//下注时间 All Jetton Time
//    int64_t                          m_nPlaceJettonLeveTime;			//Left 下注时间 Left Jetton Time
    int64_t                          m_nGameTimeJettonBroadcast;        //1S

    int64_t                          m_nGameEndTime;				    //结束时间 All End Time
//    int64_t                          m_nGameEndLeveTime;			  	//Left 结束时间 Left End Time

protected:

    uint8_t                         m_cbCurrentRoundTableCards[1];		        //桌面扑克
    static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
    vector<uint8_t>                 m_CardVecData;                              //All Cards 52*8

    JettonInfo                      m_array_jetton[GAME_PLAYER];
    int64_t                         m_array_tmparea_jetton[Max];

protected:
    string                          m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //
    chrono::system_clock::time_point  m_endTime;

private:
    tagGameRoomInfo *               m_pGameRoomInfo;
    tagGameReplay       m_replay;//game replay
    uint32_t                m_dwReadyTime;

private:
    CGameLogic                      m_GameLogic;						//游戏逻辑
    shared_ptr<ITableFrame>         m_pITableFrame;						//框架接口
    wxhhAlgorithm                   m_WXHHAlgorithm;                  //算法逻辑
    double                          m_dControlkill;
    int64_t                         m_lLimitPoints;


private:


    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
    muduo::net::TimerId                         m_TimerJettonBroadcast;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    double                                      stockWeak;
    int32_t										m_nWinIndex; //0黑桃，1红桃，2梅花。3方块，4大王

    int64_t                         m_userScoreLevel[4];
    int64_t                         m_userChips[4][CHIPS_SIZE];
    int64_t                         m_currentchips[GAME_PLAYER][CHIPS_SIZE];
    int64_t                             useIntelligentChips;
    vector<double>                   m_vmulticalList;

    vector<int64_t>                     m_allChips;
    int                             leftCards[Max];

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
};




#endif
