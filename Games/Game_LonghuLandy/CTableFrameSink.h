#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE



#define GAME_PLAYER						10001


//#define CWHArray std::deque

#define MAX_CARDS                  52*8
#define CHIP_COUNT                 5


////玩家info
//struct  PlayerInfo
//{
//    int64_t  dwUserID;	//玩家ID.
//    uint32_t ChairID;
//    int    headerID;	//玩家头像.
//    string nickName;	//玩家昵称.
//    string location;
////    char   Location[LEN_USER_LOCATE];

//    double lTwentyAllUserScore;	//20局总下注
//    double lTwentyWinScore;	//20局嬴分值. 在线列表显示用
//    int	   lTwentyWinCount;	//20局嬴次数. 在线列表显示用
//    int	   isDivineMath;	//是否为神算子 (0,1是)

//    bool operator<(const PlayerInfo& f)
//    {
//        if(f.lTwentyAllUserScore < this->lTwentyAllUserScore)
//        {
//            return true;
//        }
//        return false;
//    }
//};


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

        m_EndWinScore = 0;
        m_Revenue = 0;

        m_JettonScore.clear();
        m_WinScore.clear();
        NotBetJettonCount = 0;
        lLastWinOrLost = true;
        lLastJetton = 0;
        iDoubleCastTimes = 0;
        iLoseTimes = 0;
        lHistoryProfit = 0;
        isDoubleCast =false;
		memset(dAreaWin, 0, sizeof(dAreaWin));
		memset(dLastJetton, 0, sizeof(dLastJetton));
    }

    void clear()
    {
        memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
        memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));

        m_EndWinScore = 0;
        m_Revenue = 0;       
        isDoubleCast =false;
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

    int64_t     lHistoryProfit;          //记录玩家进来玩的赢输钱数
    bool        lLastWinOrLost;          //龙虎上一局玩家的赢输记录
    int64_t     lLastJetton;
    int32_t     iDoubleCastTimes;        //出现倍投的次数
    int32_t     iLoseTimes;              //输钱的次数
    bool        isDoubleCast;            //本次是否是倍投

    bool        isDivineMath;           //是否为神算子 (0,1是)

    int64_t             m_lUserJettonScore[4];          // 个人总注
    int64_t             m_lRobotUserJettonScore[4];     // robot 个人总注
    int64_t             m_EndWinScore;                  // 前端显示输赢
    int64_t             m_Revenue;

    list<int64_t>       m_JettonScore;                  //20局下注
    list<int64_t>       m_WinScore;                     //20局输赢
    int32_t             NotBetJettonCount; // 连续不下注的局数
	int64_t				dAreaWin[4];					//各区域的输赢

    int64_t				dLastJetton[4];
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
    int64_t areaJetton[4]={0,0,0,0};
};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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

    //判断玩家是不是倍投
    void JudgmentUserIsDoubleCast();
    //加注事件
    bool OnUserAddScore(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore, bool bReJetton = false);
    void AddPlayerJetton(int64_t chairId, uint8_t areaId, int64_t score, bool bCancel = false);
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

	void updateGameMsg(bool isInit);
	//获取游戏状态对应的剩余时间
	int32_t getGameCountTime(int gameStatus);
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);
	void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	//加倍或取消投注
	void DoubleOrCanceljetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	//取消下注事件
	bool OnUserCancelPlaceScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, bool bCancelJetton);

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
    int64_t                          m_lAllJettonScore[4];		        //全体总注
    //玩家真实下注
    int64_t                          m_lTrueUserJettonScore[4];     	//玩家真实下注
    int64_t                          m_lRobotserJettonAreaScore[4];	    //robot下注
    int64_t                          m_AreaMaxJetton[4];

    //个人下注
protected:
    int64_t                          m_ShenSuanZiId;
    int64_t                          m_lShenSuanZiJettonScore[4];

    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    multiset<shared_ptr<UserInfo>, ClassComp> m_SortUserInfo;
    vector<int>                      m_openRecord;                         //Long Hu He

    map<int64_t, uint32_t>           m_He;
    map<int64_t, uint32_t>           m_Long;
    map<int64_t, uint32_t>           m_Hu;
    bool                             m_bOpenContorl;
    int64_t                          m_LongTimes;                       //Long Times
    int64_t                          m_huTimes;                         //Hu Times
    int64_t                          m_heTimes;                         //He Times

    uint32_t                         m_IsEnableOpenlog;                 //is enable open log.
    uint32_t                         m_IsXuYaoRobotXiaZhu;              //算开奖结果是否需要机器人下注
    int64_t                          m_WinScoreSystem;                  //pao ma deng
//    int                               m_RobotMaxCount;

    int64_t                          m_nPlaceJettonTime;				//下注时间 All Jetton Time
//    int64_t                          m_nPlaceJettonLeveTime;			//Left 下注时间 Left Jetton Time
    int64_t                          m_nGameTimeJettonBroadcast;        //1S

    int64_t                          m_nGameEndTime;				    //结束时间 All End Time
//    int64_t                          m_nGameEndLeveTime;			  	//Left 结束时间 Left End Time
	float							 m_fUpdateGameMsgTime;			   //更新游戏信息的时间	

protected:

//    IServerUserItem               * m_pUserItem;						        //框架接口
    //const tagGameServiceOption		* m_pGameServiceOption;                 //配置参数
//    CHistoryScore			        m_HistoryScore;						    	//历史成绩

//    uint8_t						    m_cbPlayStatus[GAME_PLAYER];		    //游戏状态
    uint8_t                         m_cbCurrentRoundTableCards[2];		        //桌面扑克
    static uint8_t                  m_cbTableCard[MAX_CARDS];		            //桌面扑克
    vector<uint8_t>                 m_CardVecData;                              //All Cards 52*8

    JettonInfo                      m_array_jetton[GAME_PLAYER];
    int64_t                         m_array_tmparea_jetton[4];

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
    LongHuAlgorithm                 m_LongHuAlgorithm;                  //算法逻辑
    double                          m_dControlkill;
    int64_t                         m_lLimitPoints;
//    mutable boost::shared_mutex     m_mutex;

private:
//    Landy::GameTimer                           m_TimerJetton;
//    Landy::GameTimer                           m_TimerEnd;

    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
    muduo::net::TimerId                         m_TimerJettonBroadcast;
	muduo::net::TimerId                         m_TimerIdUpdateGameMsg;
	muduo::net::TimerId							m_TimerIdResetPlayerList;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    double                                      stockWeak;
	int32_t										m_nWinIndex; //输赢的区域[1和、2龙、3虎]

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
    int64_t                         m_userScoreLevel[4];
    int64_t                         m_userChips[4][CHIP_COUNT];
    int64_t                         m_currentchips[GAME_PLAYER][CHIP_COUNT];
    int                             useIntelligentChips;
    vector<int64_t> allchips;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制

	bool					   m_bGameEnd;										  //本局是否结束
	int						   m_cbGameStatus;									  //游戏状态
	int32_t          		   m_nGoodRouteType;						          //好路类型
	string				       m_strGoodRouteName;								  //好路名称
	float					   m_fResetPlayerTime;						          //本局游戏结束刷新玩家排序的时间
	int32_t 				   m_winOpenCount[MAX_COUNT];				          //各下注项累计开奖的局数(0:和 1:龙 2:虎)
	int64_t					   m_lLimitScore[2];								  //限红
	string          		   m_strTableId;									  //桌号
	int32_t					   m_ipCode;
	bool					   m_bUseCancelBet;						              //能否取消下注

};

//////////////////////////////////////////////////////////////////////////

//    int64_t                          m_TotalGain[GAME_PLAYER];           // total gain
//    int64_t                          m_TotalLost[GAME_PLAYER];           // total lost

//    int64_t                          m_CalculateRevenue[GAME_PLAYER];      //tax
//    int64_t                          m_CalculateAgentRevenue[GAME_PLAYER]; //tax



///*
////基础接口
//public:
////释放对象
//virtual VOID  Release();
////是否有效
//virtual bool  IsValid() { return AfxIsValidAddress(this,sizeof(CTableFrameSink))?true:false; }
////接口查询
//virtual void *  QueryInterface(const IID & Guid, DWORD dwQueryVer);

////管理接口
//public:
////初始化
//virtual bool  Initialization(IUnknownEx * pIUnknownEx);

////查询接口
//public:
////查询限额
//virtual SCORE QueryConsumeQuota(IServerUserItem * pIServerUserItem){return 0;}
////最少积分
//virtual SCORE QueryLessEnterScore(uint16_t wChairID, IServerUserItem * pIServerUserItem){return 0;}
////数据事件
//virtual bool OnDataBaseMessage(uint16_t wRequestID, VOID * pData, uint16_t wDataSize){return false;}
////积分事件
//virtual bool OnUserScroeNotify(WORD wChairID, IServerUserItem * pIServerUserItem, uint8_t cbReason){return false;}
////查询是否扣服务费
//virtual bool QueryBuckleServiceCharge(WORD wChairID){return true;}

////比赛接口
//public:
////设置基数
//virtual void SetGameBaseScore(LONG lBaseScore){};
//*/

////信息接口
//public:
////开始模式
//// virtual uint8_t  GetGameStartMode();

////游戏事件
//public:


////事件接口
//public:
////定时器事件
//// virtual bool  OnTimerMessage(DWORD wTimerID, WPARAM wBindParam);
////    virtual bool OnTimerMessage(dword timerid, dword param);

////框架消息处理
//// virtual bool  OnFrameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize, IServerUserItem * pIServerUserItem);

////用户事件
//public:

//private:


//牌转文字
//    CString TransformCardInfo(uint8_t cbCardData);
//    CString TransfromOpInfo(uint16_t OpId);

//	bool OnSubAmdinCommand(IServerUserItem*pIServerUserItem,const void*pDataBuffer);
//	BOOL IsAdmin( IServerUserItem * pIServerUserItem);

//    void SetAddScoreBit(int64_t userId, uint16_t& value, int index);
//    void SetBit(uint16_t& value, int index, bool bTrue);
//    uint16_t GetBit(uint16_t num, int pos);
//    void SetOpType(uint16_t wChiarID);
//    int  GetRound();
//    int64_t CalMaxALLInScore(uint16_t wChairID);
//    void SelectSort(int64_t a[], int len);
//	bool IsAllIn();

//    void WriteGameScore(uint16_t wWinner, google::protobuf::RepeatedField<int64_t>& lGameScore, int64_t lGameTax);
//由于玩家放弃，比牌，孤注一掷等提前出局，所以要提前写分
//	void WriteLosterUserScore(uint16_t wChairID);
//	//创建炸金花桌子日志
//	bool SaveTableLog();
//	//写库存
//    bool WriteStorage(uint16_t wWinner, google::protobuf::RepeatedField<SCORE>& lGameScore);
//    void InitAddScore(uint32_t chairId);


#endif
