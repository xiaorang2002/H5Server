#pragma once

#define MAX_PLAYER          500
#define BET_ITEM_COUNT       8

#define OPEN_RESULT_LIST_COUNT		100			//开奖结果记录数量

using namespace std;

enum class CardType{
    Lexus = 0,
    Daz,
    Benz,
    Bmw,
    Bsj,
    Marati,
    Lambo,
    Ferrari,
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
class TableFrameSink : public ITableFrameSink
{

public:
    // 能取上限
    static int RandomInt(int a, int b)
    {
        return rand() % (b - a + 1) + a;
    }

    float m_fTaxRate; 
    float m_fTMJettonBroadCard; 

    struct tagBetInfo
    {
        int64_t betScore;
        int32_t betIdx;
    };

    struct BestPlayerList
    {
        BestPlayerList()
        {
            NikenName = "";
            WinScore = 0;
            onTime = "";
            bestID = 0;

        }
        string NikenName;
        int64_t WinScore;
        string onTime;
        int bestID;
    };
    struct strPlalyers
    {
        strPlalyers()
        {
            JettonValues.clear();
            ListWinOrLose.clear();
            ListAllJetton.clear();
            ListWinScore.clear();
            dCurrentPlayerJetton = 0;
            dsliceSecondsAllJetton = 0;
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                dAreaJetton[i] = 0;
                dLastJetton[i] = 0;
                dsliceSecondsAllAreaJetton[i] = 0;
            }

            dsliceSecondsAllJetton = 0;
            wCharid = -1;
            wUserid = -1;
            Isplaying = false;
            Isleave = false;
            m_winAfterTax = 0;
            YingshuJuduizhi = 0;
            platformTax = 0;

            dAllOut = 0;
            TotalIn = 0;
            m_userTax = 0;
            ReturnMoney = 0;
            RealMoney =0;
            taxRate = 0.05;
            NotBetJettonCount = 0;
            wheadid = 0;

            lTwentyAllUserScore_last = 0;	 //20局总下注
            lTwentyWinScore_last = 0;        //20局嬴分值. 在线列表显示用
            lTwentyWinCount_last = 0;        //20局嬴次数. 在线列表显示用

            dTwentyAllJetton = 0;
            iTwentyWinCount = 0;
            lTwentyWinScore = 0;
        }
        // 清空玩家全部数据
        void Allclear()
        {
            JettonValues.clear();
            ListWinOrLose.clear();
            ListAllJetton.clear();
            ListWinScore.clear();
            dCurrentPlayerJetton = 0;
            dsliceSecondsAllJetton = 0;
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                dAreaJetton[i] = 0;
                iMulticle[i] = -1;
                dLastJetton[i] = 0;
                dsliceSecondsAllAreaJetton[i] = 0;
            }

            wCharid = -1;
            wUserid = -1;
            Isplaying = false;
            Isleave = false;
            m_winAfterTax = 0;
            YingshuJuduizhi = 0;
            platformTax = 0;
            m_userTax = 0;
            ReturnMoney = 0;
            RealMoney = 0;
            NotBetJettonCount = 0;
            wheadid = 0;
            dTwentyAllJetton = 0;
            iTwentyWinCount = 0;
            lTwentyWinScore = 0;
        }
        void clearJetton()
        {
            JettonValues.clear();
        }
        void clear()
        {
            dCurrentPlayerJetton = 0;
            dsliceSecondsAllJetton = 0;
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                dAreaJetton[i] = 0;
                iMulticle[i] = -1;
                dsliceSecondsAllAreaJetton[i] = 0;
            }
            YingshuJuduizhi = 0;
            m_winAfterTax = 0;
            dAllOut = 0;
            if (Isleave)
            {
                Isplaying = false;
                Isleave = false;
            }
            platformTax = 0;
            m_userTax = 0;
            ReturnMoney = 0; 
            RealMoney = 0;
        }
        void clearOtherJetton()
        {
            dsliceSecondsAllJetton = 0;
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                dsliceSecondsAllAreaJetton[i] = 0;
            }
        }
        // 计算得分
        void Culculate(shared_ptr<ITableFrame> &pITableFrame, float valshui)
        {
            int64_t res = 0;
            taxRate = valshui;
            YingshuJuduizhi = 0;
            m_winAfterTax = 0;
            platformTax = 0;
            
            dAllOut = 0;
            TotalIn = 0;
            m_userTax = 0;
            RealMoney = 0;
            ReturnMoney = 0;
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                // 本押注区开奖
                if (iMulticle[i] > 0)
                {
                    // 净赢分(减去押分本金)
                    res = dAreaJetton[i] * (iMulticle[i] - 1);
                    //赢输绝对值
                    YingshuJuduizhi += dAreaJetton[i] * (iMulticle[i]);
                    // 总赢分
                    TotalIn += dAreaJetton[i] * (iMulticle[i]);
                    // 玩家本局总得分
                    ReturnMoney += dAreaJetton[i] * (iMulticle[i]);
                    RealMoney += res;
                }
                else
                {
                    YingshuJuduizhi += dAreaJetton[i];
                    res = dAreaJetton[i] * iMulticle[i];
                    ReturnMoney += res;
                    RealMoney += res;
                }
                
                dAreaWin[i] = res;

                // 税后赢分
                m_winAfterTax += res;
                // 玩家本局总押分
                dAllOut += dAreaJetton[i];
                // 平台税收
                platformTax += (int32_t)(taxRate * dAreaJetton[i]);
                m_userTax += 0;
            }

            if (m_winAfterTax > 0)
            {
                if ((ListWinOrLose.size()) < 20)
                {
                    ListWinOrLose.push_back(1);
                }
                else
                {
                    ListWinOrLose.erase(ListWinOrLose.begin());
                    ListWinOrLose.push_back(1);
                }
            }
            else
            {
                if ((ListWinOrLose.size()) < 20)
                {
                    ListWinOrLose.push_back(0);
                }
                else
                {
                    ListWinOrLose.erase(ListWinOrLose.begin());
                    ListWinOrLose.push_back(0);
                }
            }
            if ((ListAllJetton.size()) < 20)
            {
                ListAllJetton.push_back(dCurrentPlayerJetton);
            }
            else
            {
                ListAllJetton.erase(ListAllJetton.begin());
                ListAllJetton.push_back(dCurrentPlayerJetton);
            }

            if ((ListWinScore.size()) < 20)
            {
                ListWinScore.push_back(m_winAfterTax);
            }
            else
            {
                ListWinScore.erase(ListWinScore.begin());
                ListWinScore.push_back(m_winAfterTax);
            }
        }
        // @index 开奖下标
        // @mutical 开奖对应倍数
        void SetPlayerMuticl(int index, int mutical)
        {
            if (index < 0 || index >= BET_ITEM_COUNT || mutical < 0)
            {
                LOG(ERROR) << "No Resulet form Algorithmc";
                return;
            }
            for (int i = 0; i < BET_ITEM_COUNT; i++)
            {
                if (index == i)
                {
                    iMulticle[i] = mutical;
                }
                else
                {
                    iMulticle[i] = -1;
                }
            }
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
                iTwentyWinCount = 0;
                for (vector<int64_t>::iterator it = ListWinOrLose.begin(); it != ListWinOrLose.end(); it++)
                {
                    if ((*it) > 0)
                        iTwentyWinCount += 1;
                }
                dTwentyAllJetton = 0;
                for (vector<int64_t>::iterator it = ListAllJetton.begin(); it != ListAllJetton.end(); it++)
                {
                    dTwentyAllJetton += (*it);
                }
                lTwentyWinScore = 0;
                for (vector<int64_t>::iterator it = ListWinScore.begin(); it != ListWinScore.end(); it++)
                {
                    lTwentyWinScore += (*it);
                }
                lTwentyAllUserScore_last = dTwentyAllJetton;
                lTwentyWinScore_last = lTwentyWinScore;
                lTwentyWinCount_last = iTwentyWinCount;
            }
        }
        vector<int64_t> ListWinOrLose;
        vector<int64_t> ListAllJetton;
        vector<int64_t> ListWinScore;
        vector<tagBetInfo> JettonValues; //所有押分筹码值

        int64_t dCurrentPlayerJetton;
        // 
        int64_t dAreaJetton[BET_ITEM_COUNT];
        int64_t dAreaWin[BET_ITEM_COUNT];
        int64_t dLastJetton[BET_ITEM_COUNT];

        int64_t TotalIn;
        int32_t iMulticle[BET_ITEM_COUNT];
        int64_t dAreaOut[BET_ITEM_COUNT];
        int64_t dAllOut;
        int64_t YingshuJuduizhi;
        int64_t m_winAfterTax; //税后赢分
        int32_t platformTax;
        int32_t m_userTax;
        int64_t ReturnMoney;
        int64_t RealMoney;
        int32_t NotBetJettonCount; // 连续不下注的局数
        int wCharid;
        int wUserid;
        int wheadid;
        // 是否游戏中
        bool Isplaying;
        bool Isleave;
        float taxRate;
        //一段时间内各区域其他玩家的总下注
        int64_t dsliceSecondsAllJetton;
        int64_t dsliceSecondsAllAreaJetton[BET_ITEM_COUNT];

    private:
        int64_t dTwentyAllJetton;
        int32_t iTwentyWinCount;
        int64_t lTwentyWinScore;

        int64_t  lTwentyAllUserScore_last;	//20局总下注
        int64_t  lTwentyWinScore_last;		//20局嬴分值. 在线列表显示用
        int64_t  lTwentyWinCount_last;		//20局嬴次数. 在线列表显示用
    };

    static bool Comparingconditions(strPlalyers a, strPlalyers b)
    {
        if (a.GetTwentyWin() > b.GetTwentyWin())
        {
            return true;
        }
        else if (a.GetTwentyWin() == b.GetTwentyWin() && a.GetTwentyJetton() > b.GetTwentyJetton())
        {
            return true;
        }
        return false;
    }
    static bool Comparingconditions1(strPlalyers a, strPlalyers b)
    {
        if (a.GetTwentyJetton() > b.GetTwentyJetton())
        {
            return true;
        }
        else if (a.GetTwentyWin() > b.GetTwentyWin() && a.GetTwentyJetton() == b.GetTwentyJetton())
        {
            return true;
        }
        return false;
    }
    static bool Comparingconditions2(strPlalyers a, strPlalyers b)
    {
        if (a.GetTwentyWinScore() > b.GetTwentyWinScore())
        {
            return true;
        }
        else if (a.GetTwentyWin() > b.GetTwentyWin() && a.GetTwentyWinScore() == b.GetTwentyWinScore())
        {
            return true;
        }
        return false;
    }
    TableFrameSink();
    virtual ~TableFrameSink();

public: //game.
    virtual void OnGameStart();
    virtual bool OnEventGameConclude(uint32_t chairid, uint8_t nEndTag);
    virtual bool OnEventGameScene(uint32_t chairid, bool bisLookon);
    virtual void OnEventGameEnd(int32_t chairid, uint8_t nEndTag);

public: //users.
    virtual bool OnUserEnter(int64_t userid, bool islookon);
    virtual bool OnUserReady(int64_t userid, bool islookon);
    virtual bool OnUserLeft(int64_t userid, bool islookon);
    virtual bool CanJoinTable(shared_ptr<CServerUserItem> &pUser);
    virtual bool CanLeftTable(int64_t userId);
  
public: //events.
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
public: // setting.
    virtual bool SetTableFrame(shared_ptr<ITableFrame> &pTableFrame);

public:
    void GamefirstInGame();
    void NormalCalculate();
    void NormalGameStart();
    bool GamePlayerJetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    bool GamePlayerJetton1(int32_t score, int index, int chairid,shared_ptr<CServerUserItem> &pSelf);
    void SendGameSceneStart(int64_t userid, bool lookon);
    void SendGameSceneEnd(int64_t userid, bool lookon);
    void PlayerRichSorting();
    void PlayerCalculate();
    void ResetTableDate();
    void SendPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void WriteUserScore();
    void writeRecored();
    void readRecored();
    void ArgorithResult();
    void BulletinBoard();
    void GamePlayerQueryPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd = false);
    void ReadInI();
    void GameTimerStart();

    void GameTimerEnd();

    void GameRefreshData();
	int PlayerJettonPanDuan(int index, int32_t score, int32_t chairid, shared_ptr<CServerUserItem> &play);
    void QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void ReadStorageInfo();
    void OnTimerSendOtherBetJetton();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
    //测试游戏数据
    void TestGameMessage(uint32_t wChairID,int type);
    void openSLog(const char *p, ...);

    void Algorithm();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, int64_t userWinScorePure);

    void updateResultList(int resultId);
    void sendGameRecord(int32_t chairid);
    void QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
private:
    int	 m_iGameState; 
    int	 m_iCurrBankerId;
    int	 m_iShenSuanZiChairId;
    int	 m_iShenSuanZiUserId;
    int	 m_iMul;
    int	 m_iResIdx;
    int	 m_iStopIdx;
    int64_t	 m_BestPlayerUserId;
    int	 m_BestPlayerChairId;
    int	 m_bIsIncludeAndroid;
    int	 m_nReadCount;
    int	 m_betCountDown;
    int	 m_endCountDown; 
    int	 m_timetick;
    int	 m_timetickAll;
    bool m_bIsOpen;
    bool m_bIsContinue;
    bool m_bIsClearUser;

    //干涉概率
    float m_ctrlRate;
    float m_stockPercentRate;
    int32_t					m_lastRetId;

    int64_t					m_BestPlayerWinScore;
    int32_t					m_EapseTime; 
    int64_t					m_MarqueeScore;
    int64_t					m_nAreaMaxJetton[BET_ITEM_COUNT];
    int64_t					m_nPlayerScore[MAX_PLAYER];
    int32_t					m_retWeight[BET_ITEM_COUNT];
    int32_t					m_nMuilt[BET_ITEM_COUNT];// 倍率表
	
    //
    int64_t                 m_useIntelligentChips;
    int64_t                 m_userScoreLevel[4];
    int64_t                 m_userChips[4][6];
    int64_t                 m_currentchips[MAX_PLAYER][6];
    // 玩家列表
    strPlalyers             m_pPlayerList[MAX_PLAYER];
    // 玩家列表
    vector<strPlalyers>     m_pPlayerInfoList;
    // 最佳玩家列表
    vector<BestPlayerList>  m_vPlayerList;
    vector<int32_t>         m_vResultRecored;
    vector<int64_t>         m_cbJettonMultiple; //筹码值

    shared_ptr<BetArea>     m_pBetArea;
    shared_ptr<ITableFrame> m_pITableFrame;

    // 100局开奖记录
    map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount;
    map<int32_t, vector<shared_ptr<OpenResultInfo>>>  m_vOpenResultListCount_last;
private:
    tagGameRoomInfo         *m_pGameRoomInfo;
    tagGameReplay           m_replay; 
    string                  m_strRoundId;
    uint32_t                m_dwReadyTime;
    chrono::system_clock::time_point  m_startTime;

    muduo::MutexLock        m_mutexLock;
    muduo::net::TimerId     m_TimerIdStart;
    muduo::net::TimerId     m_TimerIdEnd;
    muduo::net::TimerId     m_TimerIdTest; 
    muduo::net::TimerId     m_TimerOtherBetJetton;

    muduo::net::TimerId     m_TimerRefresh;
    shared_ptr<muduo::net::EventLoopThread> m_LoopThread;
private: 
    static int64_t		m_llStorageControl;									//库存控制
    static int64_t		m_llStorageLowerLimit;								//库存控制下限值
    static int64_t		m_llStorageHighLimit;								//库存控制上限值
    static int64_t      m_llStorageAverLine;          //average line
    static int64_t      m_llGap;            //refer gap:for couting rate
    int32_t             m_lOpenResultCount;                                 //累计开奖次数
    int32_t             m_lOpenResultIndexCount[BET_ITEM_COUNT];            //各下注项累计开奖次数
    int32_t             m_bTest;            //是否测试
    int32_t             m_nTestTimes;        //测试的局数
    int32_t             m_nTestType;        //测试的类型
    bool                m_bTestAgain;       
    // 是否写日记
    bool                m_bWritelog;

    double              stockWeak;

    BcbmAlgorithm       bcbmAlgorithm;
    float               m_dControlkill;
    int64_t             m_lLimitPoints;

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
    float               m_fResetPlayerTime;
};

bool Compare(shared_ptr<CServerUserItem> ait, shared_ptr<CServerUserItem> bit)
{
    if (ait->GetUserScore() > bit->GetUserScore())
    {
        return true;
    }
    else if (ait->GetUserScore() > bit->GetUserScore() && ait->GetUserId() < bit->GetUserId())
    {
        return true;
    }
    else
    {
        return false;
    }
}

// ITableFrameSink *GetTableFrameSinkFromDLL(void);
