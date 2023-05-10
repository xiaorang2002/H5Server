#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE


#include "GameLogic.h"

#define GAME_PLAYER						10001

#define ID_HEPAIR					0			//和对索引
#define ID_BANKERPAIR				1			//庄对索引
#define ID_PLAYERPAIR				2			//闲对索引

#define INDEX_BANKER				0			//庄家索引
#define INDEX_PLAYER				1			//闲家索引


//#define CWHArray std::deque

#define MAX_CARDS                  52*8

#define BET_ITEM_COUNT				6			// 0:和 1:庄 2:闲 3:和对 4:庄对 5:闲对

enum eGameStatus
{
	BJL_GAME_STATUS_INIT			= 0,
	BJL_GAME_STATUS_WASHCARD		= 1,			// 洗牌
	BJL_GAME_STATUS_SENDCARD		= 2,			// 发牌
	BJL_GAME_STATUS_PLACE_JETTON	= 3,			// 下注
	BJL_GAME_STATUS_START			= 100,			// 游戏进行
	BJL_GAME_STATUS_END				= 200,			// 游戏End
};
class OpenResultInfo
{
public:
	OpenResultInfo()
	{
		iResultId = 0;
		notOpenCount = 0;
	}
	void clear()
	{
		iResultId = 0;
		notOpenCount = 0;
	}
	int iResultId;
	int notOpenCount;
};

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
		lTwentyWinScore_last = 0;
		lTwentyWinScore_last = 0;
		iResultId = -1;
        isDivineMath = false;

        memset(dAreaJetton, 0, sizeof(dAreaJetton));
        //memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
		for (int i = 0; i < MAX_COUNT; i++)
		{
			nMuilt[i] = 0;
			iMulticle[i] = -1.0f;
			dLastJetton[i] = 0;
			dAreaWin[i] = 0;
			if (i < 3)
				iCardPair[i] = 0;
		}
		lCurrentPlayerJetton = 0;
        m_EndWinScore = 0;
        m_Revenue = 0;
		taxRate = 0.05;
        m_JettonScore.clear();
        m_WinScore.clear();
        NotBetJettonCount = 0;
        lLastWinOrLost = true;
        lLastJetton = 0;
        iDoubleCastTimes = 0;
        iLoseTimes = 0;
        lHistoryProfit = 0;
        isDoubleCast =false;
		
		Isplaying = false;
		Isleave = false;
		isAndroidUser = false;
    }

    void clear()
    {
        memset(dAreaJetton, 0, sizeof(dAreaJetton));
		for (int i = 0; i < MAX_COUNT; i++)
		{
			nMuilt[i] = 0;
			iMulticle[i] = -1.0f;
			dLastJetton[i] = 0;
			dAreaWin[i] = 0;
			if (i < 3)
				iCardPair[i] = 0;
		}
		lCurrentPlayerJetton = 0;
        m_EndWinScore = 0;
        m_Revenue = 0;  
		taxRate = 0.05;
        isDoubleCast =false;
		
		if (Isleave)
		{
			Isplaying = false;
			Isleave = false;
		}
		isAndroidUser = false;
    }
	void SetPlayerMuticl(int32_t index, double mutical, double fAreaMuilt[6], uint8_t iRetPair[3])
	{
		if (index < 0 || index >= MAX_COUNT || mutical < 0) {
			LOG(ERROR) << "No Resulet form Algorithmc" << " m_iResultId:" << index;
			return;
		}
		memcpy(iCardPair, iRetPair, sizeof(iCardPair));
		for (int i = 0; i < MAX_COUNT; i++)
		{
			nMuilt[i] = fAreaMuilt[i];
			iMulticle[i] = -1.0f;
		}
		for (int i = 0; i < MAX_COUNT; i++)
		{
			if (i<3)
			{
				iMulticle[i] = (index == i) ? mutical : (-1.0f);
			} 
			else
			{
				if (iRetPair[i-3] > 0)
				{
					iMulticle[i] = nMuilt[i];
				}
			}
		}
		iResultId = index;
	}
	// 计算得分
	void Culculate(float valshui, int32_t iRetId)
	{
        int64_t res = 0;
		taxRate = valshui;
		for (int i = 0;i < MAX_COUNT;i++)
		{
			if(dAreaJetton[i] <= 0) 
				continue;
            int64_t tempWin = 0;
			// 本押注区开奖
			if (iMulticle[i] > 0)
			{
				// 净赢分(减去押分本金)
				res = dAreaJetton[i] * (iMulticle[i] - 1);
			}
			else
			{
				if (iRetId == (int)WinTag::eHe) //开和
				{
					if (i == (int)WinTag::eZhuang || i == (int)WinTag::eXian)
						continue;
					res = dAreaJetton[i] * iMulticle[i];
				} 
				else
				{
					res = dAreaJetton[i] * iMulticle[i];
				}
			}
			dAreaWin[i] = res;
			m_EndWinScore += res;
			if (!isAndroidUser)
				LOG(INFO) << "本局算分 userId:" << userId << " dArea:" << i << " dAreaJetton:" << dAreaJetton[i] << " dAreaWin:" << dAreaWin[i];
		}
		if (!isAndroidUser)
			LOG(INFO) << "本局算分 userId:" << userId << " TotalWin:" << m_EndWinScore;
	}

    int64_t GetTwentyWin()
	{
		return lTwentyWinCount_last;
	}
    int64_t GetTwentyJetton()
	{
		return lTwentyAllUserScore_last;
	}
    int64_t GetTwentyWinScore()
	{
		return lTwentyWinScore_last;
	}
	void RefreshGameRusult(bool isGameEnd)
	{
		if (isGameEnd)
		{
			lTwentyAllUserScore_last = lTwentyAllUserScore;
			lTwentyWinScore_last = lTwentyWinScore;
			lTwentyWinCount_last = lTwentyWinCount;
		}
	}

public:
    int64_t				userId;						//玩家ID.
    uint32_t			chairId;
    uint8_t				headerId;					//玩家头像.
    string				nickName;					//玩家昵称.
    string				location;

    int64_t				lTwentyAllUserScore;		//20局总下注
    int64_t				lTwentyWinScore;			//20局嬴分值. 在线列表显示用
    uint8_t				lTwentyWinCount;			//20局嬴次数. 在线列表显示用
    int64_t				lTwentyAllUserScore_last;	//20局总下注
    int64_t				lTwentyWinScore_last;		//20局嬴分值. 在线列表显示用
	uint8_t				lTwentyWinCount_last;		//20局嬴次数. 在线列表显示用

    int64_t				lHistoryProfit;				//记录玩家进来玩的赢输钱数
    bool				lLastWinOrLost;				//庄闲上一局玩家的赢输记录
    int64_t				lLastJetton;
    int32_t				iDoubleCastTimes;			//出现倍投的次数
    int32_t				iLoseTimes;					//输钱的次数
    bool				isDoubleCast;				//本次是否是倍投

    bool				isDivineMath;				//是否为神算子 (0,1是)

    int64_t             dAreaJetton[MAX_COUNT];          // 个人总注
	int64_t				lCurrentPlayerJetton;					// 本局总下注
    int64_t             m_EndWinScore;							// 前端显示输赢
    int64_t             m_Revenue;
	float				taxRate;

    list<int64_t>       m_JettonScore;							//20局下注
    list<int64_t>       m_WinScore;								//20局输赢
	
    int32_t             NotBetJettonCount;						//连续不下注的局数
	int64_t				dAreaWin[MAX_COUNT];					//各区域的输赢
	double				iMulticle[MAX_COUNT];
	double 				nMuilt[MAX_COUNT];					    //赔率
	int32_t				iResultId;
	uint8_t				iCardPair[3];						    //是否牌对【 0:和对 1:庄对 2:闲对】、【0:不是 1:对子】

	bool				Isplaying;
	bool				Isleave;
	int32_t				dLastJetton[MAX_COUNT];
	bool				isAndroidUser;							//是否AI  (0,1是)
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
    int64_t areaJetton[6]={0,0,0,0,0,0 };
};

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

	// 20局输赢的次数排序
	static bool Comparingconditions(shared_ptr<UserInfo> a, shared_ptr<UserInfo> b)
	{
		if (a->GetTwentyWin() > b->GetTwentyWin())
		{
			return true;
		}
		else if (a->GetTwentyWin() == b->GetTwentyWin() && a->GetTwentyJetton() > b->GetTwentyJetton())
		{
			return true;
		}
		return false;
	}
	//投注量从大到小排序
	static bool Comparingconditions1(shared_ptr<UserInfo> a, shared_ptr<UserInfo> b)
	{
		if (a->GetTwentyJetton() > b->GetTwentyJetton())
		{
			return true;
		}
		else if (a->GetTwentyWin() > b->GetTwentyWin() && a->GetTwentyJetton() == b->GetTwentyJetton())
		{
			return true;
		}
		return false;
	}
public:
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag);
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon);
    //用户坐下
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
    //bool clearTableUserId(uint32_t chairId);
    //读取配置
    bool ReadConfigInformation();
    //设置路径
    bool  OnMakeDir();

public:
	void OnTimerWashCard();
	void OnTimerSendCard();
    void OnTimerJetton();
    void OnTimerEnd();
	void OnTimerStartPlaceJetton();
	//游戏事件
protected:
    void openLog(const char *p, ...);

    //根据系统输赢发牌
    void GetOpenCards();
	void GetSearchCards(int8_t winFlag, int8_t cardPair[3], uint8_t cbZhuangCardDate[2], uint8_t cbXianCardDate[2]);
	//判断是否补牌
	bool JudgmentRepairCard(int8_t winFlag);
	//判断牌是否有效
	bool IsMoreCard(uint8_t card);
	bool JudgmentIsValidCard();
	bool JudgmentIsValidAllCard();
    void ClearTableData();

    string GetCardValueName(uint8_t card);
    //改变玩家下注输赢信息
    void ChangeUserInfo();

    //判断玩家是不是倍投
    void JudgmentUserIsDoubleCast();
    //加注事件
    bool OnUserAddScore(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore, bool bReJetton=false);
    void AddPlayerJetton(int64_t chairId, uint8_t areaId, int64_t score,bool bCancel = false);
    void SendPlaceBetFail(uint32_t chairId, uint8_t cbJettonArea, int64_t lJettonScore, int32_t errCode);
    void SendOpenRecord(uint32_t chairId);
    bool SendPmdMsg(tagScoreInfo arScore[]);
	void SendGameStateMsg(uint32_t chairId);
	void SendGameStateEnd(uint32_t chairId);
    void SendStartJettonMsg(uint32_t chairId);
    void ChangeVipUserInfo();
    //void SendStartjettonMsg(dword wChairID);
    void SendEndMsg(uint32_t chairId);

    bool isInVipList(uint32_t chairId);
    void GameTimerJettonBroadcast();

    //保存玩家结算结果
    void WriteUserScore();

	//游戏即将开始倒计时时间
    //void AutoStartRemainTime(uint8_t cbReason);

	//void KickPlayerNoOperate();
    void clearTableUser();
    void SendWinSysMessage();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);

	void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	//加倍或取消投注
	void DoubleOrCanceljetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	//取消下注事件
	bool OnUserCancelPlaceScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, bool bCancelJetton);
	// 在线玩家列表
	void GamePlayerQueryPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd = false);
	void PlayerRichSorting(bool iChooseShenSuanZi = true);
	// 查询当前游戏状态
	void QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	//更新100局开奖结果信息
	void updateResultList(int resultId);
	//重排近20局玩家盈利
	void SortPlayerList();
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);
	//获取一张牌
	uint8_t getOneCard(int8_t winTag, int32_t bankerPoint, int32_t playerPoint, int8_t &winTagChang, int32_t &bankerIndex, int32_t &playerIndex, bool bBanker=false);
	void CalculatWeight();
	void getTwoCard(uint8_t cbData[2],int nPair, int starIndex, vector<uint8_t>	vecData, int cardSize, bool bBanker = false);
	//获取和对
	void getHePairCard(uint8_t cbBankerData[2], uint8_t cbData[2], int nPair, int starIndex, vector<uint8_t> vecData, int cardSize);
	void eraseVecCard(uint8_t cbData[], int cbCount);
	void updateGameMsg(bool isInit = false);
	//获取游戏状态对应的剩余时间
	int32_t getGameCountTime(int gameStatus);
	//获取游戏结束对应的发牌状态 0左翻牌 1右翻牌 2左补牌 3右补牌 4显示结果
	int32_t getGameEndCurState();
	void loadConf();
	//功能函数
protected:

protected:


public:
    //总下注数
protected:
    int64_t								m_lAllJettonScore[MAX_COUNT];		        //全体总注
    //玩家真实下注
    int64_t								m_lTrueUserJettonScore[MAX_COUNT];     	//玩家真实下注
    int64_t								m_lRobotserJettonAreaScore[MAX_COUNT];	    //robot下注
    int64_t								m_AreaMaxJetton[MAX_COUNT];

    //个人下注
protected:
    int64_t								m_iShenSuanZiUserId;
    int64_t								m_lShenSuanZiJettonScore[MAX_COUNT];

    map<int64_t, shared_ptr<UserInfo>>			m_UserInfo;
    multiset<shared_ptr<UserInfo>, ClassComp>	m_SortUserInfo;
    vector<int>									m_openRecord;                         //Zhuang Xian He

	vector<shared_ptr<UserInfo>>				m_pPlayerInfoList;		// 玩家列表	
	vector<UserInfo>						    m_pPlayerInfoList_20;	// 20局玩家列表排序

    map<int64_t, uint32_t>				m_He;
    map<int64_t, uint32_t>				m_Zhuang;
    map<int64_t, uint32_t>				m_Xian;
	map<int64_t, uint32_t>				m_HePair;
	map<int64_t, uint32_t>				m_ZhuangPair;
	map<int64_t, uint32_t>				m_XianPair;
	map<int32_t, vector<uint8_t>>		m_TestCards;						//测试牌值

    bool								m_bOpenContorl;
	double								m_ZhuangTimes;                     //Zhuang Times
	double								m_XianTimes;                       //Xian Times
	double								m_heTimes;                         //He Times
	double								m_ZhuangPairTimes;                 //ZhuangPair Times
	double								m_XianPairTimes;                   //XianPair Times
	double								m_hePairTimes;                     //HePair Times
	double								m_fMuilt[MAX_COUNT];
	double								m_fRetMul;
	int32_t								m_retWeight[MAX_COUNT];

    uint32_t							m_IsEnableOpenlog;                 //is enable open log.
    uint32_t							m_IsXuYaoRobotXiaZhu;              //算开奖结果是否需要机器人下注
    int64_t								m_WinScoreSystem;                  //pao ma deng

	int32_t								m_nPlaceJettonTime;				   //下注时间 All Jetton Time
	int32_t								m_nGameTimeJettonBroadcast;        //1S

	int32_t								m_nGameEndTime;					   //结束时间 All End Time
	int32_t								m_nWashCardTime;				   //洗牌时间 
	int32_t								m_nSendCardTime;				   //发牌时间 
	float								m_fUpdateGameMsgTime;			   //更新游戏信息的时间	
	float								m_fBupaiTime;					   //补一张牌的时间	
	float								m_fOpenCardTime;				   //游戏结束，系统开两张牌时间	
protected:

    //uint8_t						    m_cbPlayStatus[GAME_PLAYER];		    //游戏状态
	uint8_t								m_cbCardCount[2];						//桌面扑克牌数
    uint8_t								m_cbTableCards[2][3];					//桌面扑克
	uint8_t								m_cbCardPoint[2];						//桌面扑克点数
	uint8_t								m_cbCardPair[3];						//是否牌对【 0:和对 1:庄对 2:闲对】、【0:不是 1:对子】
	int32_t								m_nWinIndex;							//输赢的区域[0和、1庄、2闲]
	string								m_strResultMsg;							//开奖结果信息

    static uint8_t						m_cbTableCard[MAX_CARDS];		        //桌面扑克
    vector<uint8_t>						m_CardVecData;                          //All Cards 52*8
	vector<uint8_t>						m_vecData;								//临时取的牌
	int32_t								m_cbCardTypeCount[4][13];				//各种牌的涨数

    JettonInfo							m_array_jetton[GAME_PLAYER];
    int64_t								m_array_tmparea_jetton[MAX_COUNT];

protected:
    string								m_strRoundId;
    chrono::system_clock::time_point	m_startTime;                      //
    chrono::system_clock::time_point	m_endTime;
	chrono::system_clock::time_point	m_gameStateStartTime;             //各游戏状态开始的时间

private:
    tagGameRoomInfo *					m_pGameRoomInfo;
    tagGameReplay						m_replay;//game replay
    uint32_t							m_dwReadyTime;

private:
    CGameLogic							m_GameLogic;						//游戏逻辑
    shared_ptr<ITableFrame>				m_pITableFrame;						//框架接口
    BjlAlgorithm						m_BjlAlgorithm;                  //算法逻辑
    double								m_dControlkill;
    int64_t								m_lLimitPoints;

private:
	muduo::net::TimerId                         m_TimerIdWashCard;
	muduo::net::TimerId                         m_TimerIdSendCard;
    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
    muduo::net::TimerId                         m_TimerJettonBroadcast;
	muduo::net::TimerId							m_TimerIdResetPlayerList;
	muduo::net::TimerId                         m_TimerIdUpdateGameMsg;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

    double                                      stockWeak;

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
    int64_t                         m_userChips[4][7];
    int64_t                         m_currentchips[GAME_PLAYER][7];
    int64_t                             useIntelligentChips;
    vector<int64_t>					m_cbJettonMultiple;
    STD::Random						m_random;
    int32_t							m_difficulty;                            //玩家难度 千分制

	int								m_cbGameStatus;							//游戏状态
	bool							m_bGameEnd;								//本局是否结束
	bool							m_bRefreshPlayerList;					//本局是否刷新玩家排序
	float							m_fResetPlayerTime;						//本局游戏结束刷新玩家排序的时间
	int32_t 						m_notOpenCount[MAX_COUNT];				//各下注项连续未开累计的局数
	int32_t 						m_notOpenCount_last[MAX_COUNT];			//各下注项连续未开累计的局数
	int32_t 						m_winOpenCount[MAX_COUNT];				//各下注项累计开奖的局数(0:和 1:庄 2:闲 3:和对 4:庄对 5:闲对)
	float							m_fTaxRate;								//税收
	int64_t							m_lLimitScore[2];						//限红
	string          				m_strTableId;							//桌号
	int32_t          				m_nSendCardIndex;						//当前发第几张牌
	int32_t          				m_nLeftCardCount;						//剩余牌数
	int32_t          				m_nCutCardId;							//切牌id
	int32_t          				m_nJuHaoId;								//第几局
	int32_t          				m_nGoodRouteType;						//好路类型
	string							m_strGoodRouteName;						//好路名称
	int32_t							m_lLimitQiePai[2];						//切牌数范围
	int32_t							m_NotBetJettonKickOutCount;				//连续不下注的局数[0:不开放, n局踢出]
	int32_t							m_nCardFirstDeleteCount;				//开始洗牌后去掉几张

	int32_t							m_ipCode;	
	bool							m_bTestGame;
	bool							m_bUseCancelBet;						//能否取消下注
	uint8_t							m_cbTestBuCard[2];						//测试补的第三张牌

	int64_t							m_dAreaTestJetton[MAX_COUNT];
	int64_t							m_dAreaWinJetton[MAX_COUNT];
	int64_t							m_dCurAllBetJetton;
	int64_t							m_dCurAllWinJetton;
	int64_t							m_nTestJushu;
	int32_t							m_nTestTime;
	int32_t							m_nTestSameJushu;	//算法需求和最终结果相同的局数
	int32_t							m_nTestChangJushu;	//前两张庄闲调换的局数

	// 100局开奖记录
	/*map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount;
	map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount_last;*/

};

#endif
