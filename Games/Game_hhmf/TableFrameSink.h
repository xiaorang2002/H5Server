#pragma once
#include "HhmfAlgorithm.h"

#define MAX_PLAYER          300


#define RES_RECORD_COUNT			31			//记录数量
#define BEST_PLAYER_LIST_COUNT		20			//最好成绩玩家记录数量
#define OPEN_RESULT_LIST_COUNT		100			//开奖结果记录数量

#define JETTON_STATUS               120         //新增加的一个下注状态
#define BET_ITEM_COUNT       12 //
#define MIN_LEFT_CARDS_RELOAD_NUM 133           //剩余多少张牌就重新洗牌

#define MAX_CHOUMA_NUM             5            //筹码最大个数值
/*
* 大， 0
* 小， 1
* 单， 2
* 双， 3
* 全骰(任意一个点数的围骰)，4
* 1点对子，5
* 2点对子，6
* 3点对子，7
* 4点对子，8
* 5点对子，9
* 6点对子，10
* 1点围骰，11
* 2点围骰，12
* 3点围骰，13
* 4点围骰，14
* 5点围骰，15
* 6点围骰，16
* 和4点，17
* 和5点，18
* 和6点，19
* 和7点，20
* 和8点，21
* 和9点，22
* 和10点，23
* 和11点，24
* 和12点，25
* 和13点，26
* 和14点，27
* 和15点，28
* 和16点，29
* 和17点，30
* 1点三军，31
* 2点三军，32
* 3点三军，33
* 4点三军，34
* 5点三军，35
* 6点三军，36
*/ 
    
struct tagBetInfo
{
    int64_t betScore;
    int32_t betIdx;
};


using namespace std;

class UserInfo
{
public:
    UserInfo()
    {
        JettonValues.clear();
        ListWinOrLose.clear();
        ListAllJetton.clear();
		ListAllWinScore.clear();
        dCurrentPlayerJetton = 0;
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dAreaJetton[i] = 0;
            dLastJetton[i] = 0;
            dsliceSecondsAllAreaJetton[i] = 0;
			dAreaWin[i] = 0;
			dAreaOut[i] = 0;
        }

        wCharid = -1;
        wUserid = -1;
		headerId = 0;
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
		lTwentyAllUserScore = 0;	//20局总下注
		lTwentyWinScore = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount = 0;        //20局嬴次数. 在线列表显示用
		iResultId = -1;
		lTwentyAllUserScore_last = 0;	 //20局总下注
		lTwentyWinScore_last = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount_last = 0;        //20局嬴次数. 在线列表显示用
        lResultWangSeven =0;
    }
    // 清空玩家全部数据
    void Allclear()
    {
        JettonValues.clear();
        ListWinOrLose.clear();
        ListAllJetton.clear();
		ListAllWinScore.clear();
        dCurrentPlayerJetton = 0;
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dAreaJetton[i] = 0;
            iMulticle[i] = -1;
            dLastJetton[i] = 0;
            dsliceSecondsAllAreaJetton[i] = 0;
			dAreaWin[i] = 0;
			dAreaOut[i] = 0;
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
		lTwentyAllUserScore = 0;	//20局总下注
		lTwentyWinScore = 0;        //20局嬴分值. 在线列表显示用
		lTwentyWinCount = 0;        //20局嬴次数. 在线列表显示用
		iResultId = -1;
        lResultWangSeven = 0;
    }
    void clearJetton()
    {
        JettonValues.clear();
    }
    void clear()
    {
        dCurrentPlayerJetton = 0;
        lResultWangSeven = 0;
        dsliceSecondsAllJetton = 0;
        for (int i = 0; i < BET_ITEM_COUNT; i++)
        {
            dAreaJetton[i] = 0;
            iMulticle[i] = -1;
            dsliceSecondsAllAreaJetton[i] = 0;
			dAreaWin[i] = 0;
			dAreaOut[i] = 0;
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
		iResultId = -1;

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
    void Culculate( float valshui,strResultInfo resInfo) //shared_ptr<ITableFrame> &pITableFrame,
    {
        int64_t res = 0;
        taxRate = valshui;
        YingshuJuduizhi = 0;
        m_winAfterTax = 0;
        platformTax = 0;
        
        dAllOut = 0;
        TotalIn = 0;
        m_userTax = 0; 
        int64_t resTax = 0;
		RealMoney = 0;

        int64_t tempWin = 0;

        if(resInfo.winList[eWang])
        {
            lResultWangSeven=2;
            for(int i=0;i<eMax;i++)
            {
                if(i==eWang)
                {

                    tempWin += dAreaJetton[i] * resInfo.muticlList[i];
                    dAreaWin[i] = dAreaJetton[i] * resInfo.muticlList[i];
                    resTax += dAreaJetton[i] * resInfo.muticlList[i];
                }
                else if(i==eHei||i==eHong||i==eMei||i==eFang)
                {
                    tempWin += 0;
                    dAreaWin[i] = 0;
                    resTax += 0;
                }
                else
                {
                    tempWin -= -dAreaJetton[i];
                    dAreaWin[i] = -dAreaJetton[i];
                    resTax -= dAreaJetton[i];
                }

            }
        }
        else if(resInfo.winList[eSeven])
        {
            lResultWangSeven=1;
            for(int i=0;i<eMax;i++)
            {
                if(i==eDa||i==eXiao||i==eDan||i==eShuang)
                {
                    tempWin += 0;
                    dAreaWin[i] = 0;
                    resTax += 0;
                }
                else
                {
                    if(!resInfo.winList[i])
                    {
                        tempWin -= dAreaJetton[i];
                        dAreaWin[i] = -dAreaJetton[i];
                        resTax -= dAreaJetton[i];
                    }
                    else
                    {


                        tempWin += dAreaJetton[i] * resInfo.muticlList[i];
                        dAreaWin[i] = dAreaJetton[i] * resInfo.muticlList[i];
                        resTax += dAreaJetton[i] * resInfo.muticlList[i];
                    }
                }
            }

        }
        else
        {
            lResultWangSeven=0;
            for(int i=0;i<eMax;i++)
            {
                if(!resInfo.winList[i])
                {
                    tempWin -= dAreaJetton[i];
                    dAreaWin[i] = -dAreaJetton[i];
                    resTax -= dAreaJetton[i];
                }
                else
                {


                    tempWin += dAreaJetton[i] * resInfo.muticlList[i];
                    dAreaWin[i] = dAreaJetton[i] * resInfo.muticlList[i];
                    resTax += dAreaJetton[i] * resInfo.muticlList[i];
                }
            }
        }

        //赢输绝对值
        YingshuJuduizhi += tempWin;
        // 总赢分
        TotalIn         += tempWin;
        // 玩家本局总得分
        ReturnMoney     += tempWin;
        RealMoney += resTax;
        m_winAfterTax += resTax;

        // 赢输记录
        if ((ListWinOrLose.size()) >= 20)
            ListWinOrLose.erase(ListWinOrLose.begin());
        ListWinOrLose.push_back((m_winAfterTax > 0) ? 1 : 0);

        // 当前玩家押分记录
        if ((ListAllJetton.size()) >= 20)
            ListAllJetton.erase(ListAllJetton.begin());
        ListAllJetton.push_back(dCurrentPlayerJetton);

		// 当前玩家输赢分数记录
		if ((ListAllWinScore.size()) >= 20)
			ListAllWinScore.erase(ListAllWinScore.begin());
		ListAllWinScore.push_back(RealMoney);
    }
    
    int64_t GetTwentyWin()
    {
        if (ListWinOrLose.size() <= 0) 
            return 0;
        
		lTwentyWinCount = 0;
        for (vector<int64_t>::iterator it = ListWinOrLose.begin(); it != ListWinOrLose.end(); it++)
        {
            if ((*it) > 0)
				lTwentyWinCount += 1;
        }
        return lTwentyWinCount_last;
    }
    int64_t GetTwentyJetton()
    {
        if (ListAllJetton.size() <= 0)
            return 0; 

		lTwentyAllUserScore = 0;
        for (vector<int64_t>::iterator it = ListAllJetton.begin(); it != ListAllJetton.end(); it++)
        {
			lTwentyAllUserScore += (*it);
        }
        return lTwentyAllUserScore_last;
    }
    int64_t GetTwentyWinScore()
	{
		if (ListAllWinScore.size() <= 0)
			return 0;

		lTwentyWinScore = 0;
        for (vector<int64_t>::iterator it = ListAllWinScore.begin(); it != ListAllWinScore.end(); it++)
		{
			lTwentyWinScore += (*it);
		}
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
    vector<int64_t> ListWinOrLose;
    vector<int64_t> ListAllJetton;
    vector<int64_t> ListAllWinScore; //20局的输赢
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
    int64_t platformTax;
    int64_t m_userTax;
    int64_t ReturnMoney;
    int64_t RealMoney;
    int32_t NotBetJettonCount; // 连续不下注的局数
    int wCharid;
    int64_t wUserid;
	uint8_t     headerId;	//玩家头像.
	string      nickName;	//玩家昵称.
	string      location;
    // 是否游戏中
    bool Isplaying;
    bool Isleave;
    float taxRate;
    //一段时间内各区域其他玩家的总下注
    int64_t dsliceSecondsAllJetton;
    int64_t dsliceSecondsAllAreaJetton[BET_ITEM_COUNT];

//private:
	int64_t  lTwentyAllUserScore;		//20局总下注
	int64_t  lTwentyWinScore;			//20局嬴分值. 在线列表显示用
	uint8_t  lTwentyWinCount;			//20局嬴次数. 在线列表显示用
	int		 iResultId;					//本局开奖结果
    int64_t  lTwentyAllUserScore_last;	//20局总下注
    int64_t  lTwentyWinScore_last;		//20局嬴分值. 在线列表显示用
    int32_t  lTwentyWinCount_last;		//20局嬴次数. 在线列表显示用

    int32_t  lResultWangSeven;          //0普通结果，1出七的结果，2出王

};

class OpenResultInfo
{
public:
    OpenResultInfo()
    {
        iHongSeHeiseWang = 0;
        iIsDanShuangSevenWang = 0;
        iIsDaXiaoSevenWang = 0;
        iHhmfw = 0;
        notOpenCount = 0;
        card =0;
    }
    void clear()
    {
        iHongSeHeiseWang = 0;
        iIsDanShuangSevenWang = 0;
        iIsDaXiaoSevenWang = 0;
        iHhmfw = 0;
        notOpenCount = 0;
        card =0;
    }
    void init(strResultInfo result)
    {
        iHongSeHeiseWang = result.iHonghei;
        iIsDanShuangSevenWang = result.iDanshuang;
        iIsDaXiaoSevenWang = result.iDaxiao;
        iHhmfw = result.iHhmf;
        card = result.icard;

    }
    int iHongSeHeiseWang;    //红色或者黑色或者王
    int iIsDanShuangSevenWang;//单双或者七或者王
    int iIsDaXiaoSevenWang;   //大小或者七或者王
    int iHhmfw;   //黑红梅方王
    uint8_t card;
    int notOpenCount;  //多少局没开

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
	  int32_t m_testRet;
    int m_testInex;
    int m_testResultInex;
	int m_testInex_sd;
	int m_testResultInex_sd;

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
	//20局累计输赢从大到小排序
    static bool Comparingconditions2(UserInfo a, UserInfo b)
	{		
        if (a.GetTwentyWinScore() > b.GetTwentyWinScore())
		{
			return true;
		}
        else if (a.GetTwentyJetton() > b.GetTwentyJetton() && a.GetTwentyWinScore() == b.GetTwentyWinScore())
		{
			return true;
		}
        else if (a.GetTwentyWin() > b.GetTwentyWin() && a.GetTwentyJetton() == b.GetTwentyJetton())
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
    void GamePlayerJetton1(int64_t score, int index, int chairid,shared_ptr<CServerUserItem> &pSelf);
    void SendGameSceneStart(int64_t userid, bool lookon);
    void SendGameSceneEnd(int64_t userid, bool lookon);
    void PlayerRichSorting(bool iChooseShenSuanZi = true);
    void PlayerCalculate();
    void ResetTableDate();
    void SendPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void WriteUserScore();
    void writeRecored();
    void readRecored();
    void ArgorithResult();
    void Algorithm();
    void BulletinBoard();
    void GamePlayerQueryPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd = false);
    void ReadInI();
    void GameTimerStart();
    void GameTimerEnd();
    void GameTimerJetton();
      int PlayerJettonPanDuan(int index, int64_t score, int32_t chairid, shared_ptr<CServerUserItem> &play);
    void QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
    void ReadStorageInfo();
    void OnTimerSendOtherBetJetton();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
    //测试游戏数据
    void TestGameMessage(uint32_t wChairID,int type);
    void openSLog(const char *p, ...);
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);
	//更新100局开奖结果信息
    void updateResultList(strResultInfo result);
	// 100局游戏记录
	void QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize);
	void sendGameRecord(int32_t chairid);
	// 获取本局金鲨开奖的倍数
	void getJSodds();
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);
	//重排近20局玩家盈利
	void SortPlayerList();

    void updateGameMsg(bool isInit);
    int32_t getGameCountTime(int gameStatus);
private:
    int	 m_iGameState; 
    int  m_bBetStatus;
    int	 m_iCurrBankerId;
    int	 m_iShenSuanZiChairId;
    int	 m_iShenSuanZiUserId;
    int	 m_iRetMul;
    int	 m_iResultId;
    strResultInfo m_strResultInfo;
    int	 m_iStopIndex;
    int	 m_BestPlayerUserId;
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
	float m_fGameStartAniTime;		//预留前端开始动画的时间
	float m_fGameEndAniTime;		//预留前端结束动画的时间
	int	 m_iResultId_sd;			//若开金鲨或银鲨赠送的动物类型
    bool testIsOpen;
    int32_t testList[3];

	float m_fResetPlayerTime;		//本局游戏结束刷新玩家排序的时间

    //干涉概率
    float m_ctrlRate;
    float m_stockPercentRate;
    strResultInfo			m_lastRetId;

    int64_t					m_BestPlayerWinScore;
    int32_t					m_EapseTime; 
    int64_t					m_MarqueeScore;
    int64_t					m_nAreaMaxJetton[BET_ITEM_COUNT];

	

	int64_t                          m_lShenSuanZiJettonScore[BET_ITEM_COUNT];
	int64_t                          m_ShenSuanZiId;

    // new玩家列表
    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    // 玩家列表
    vector<shared_ptr<UserInfo>>     m_pPlayerInfoList;
	// 20局玩家列表排序
    vector<UserInfo>     m_pPlayerInfoList_20;
    // 最佳玩家列表
    vector<BestPlayerList>  m_vPlayerList;
    vector<strResultInfo>   m_vResultRecored;
    vector<int64_t>         m_cbJettonMultiple; //筹码值
	// 100局开奖记录
    map<int32_t, vector<OpenResultInfo>>  m_vOpenResultListCount;
    map<int32_t, vector<OpenResultInfo>>  m_vOpenResultListCount_last;

    shared_ptr<BetArea>     m_pBetArea;
    shared_ptr<ITableFrame> m_pITableFrame;
public:
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
    tagGameRoomInfo         *m_pGameRoomInfo;
    tagGameReplay           m_replay; 
    string                  m_strRoundId;
    uint32_t                m_dwReadyTime;
    chrono::system_clock::time_point  m_startTime;

    muduo::MutexLock        m_mutexLock;
    muduo::net::TimerId     m_TimerIdStart;
    muduo::net::TimerId     m_TimerIdEnd;
    muduo::net::TimerId     m_TimerIdJetton;
    muduo::net::TimerId     m_TimerIdTest; 
    muduo::net::TimerId     m_TimerOtherBetJetton;
	muduo::net::TimerId     m_TimerIdResetPlayerList;
    muduo::net::TimerId     m_TimerIdUpdateGameMsg;
    shared_ptr<muduo::net::EventLoopThread> m_LoopThread;

    int32_t mfGameTimeLeftTime;
    float m_fUpdateGameMsgTime;
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

    int64_t             m_lLimitPoints;
    double              m_dControlkill;

    int64_t             m_lBlackBetLimit;
    int64_t             m_lControlAndLoseLimit;

    HhmfAlgorithm         hhmfAlgorithm;

    int64_t                 m_useIntelligentChips;
    int64_t                 m_userScoreLevel[4];
    int64_t                 m_userChips[4][MAX_CHOUMA_NUM];
    int64_t                 m_currentchips[MAX_PLAYER][MAX_CHOUMA_NUM];

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
	bool					m_bGameEnd;		//本局是否结束
	bool					m_bRefreshPlayerList;		//本局是否刷新玩家排序

	//开金鲨是的倍数和彩金倍数
	int32_t m_ctrlGradePercentRate[2];  // 比例[下标0:低倍;1:中倍]
	int32_t m_ctrlGradeMul[2];			// 倍数[下标0:金鲨倍数;1:彩金倍数]

    int64_t					   m_lLimitScore[2];								  //限红

    int64_t         m_lOpenResultAllCount;//总共开过的局数

    CGameLogic  logic;
    static uint8_t                  m_cbTableCard[54*8];		            //桌面扑克
    vector<uint8_t>                 m_CardVecData;


    string          		   m_strTableId;									  //桌号
    int32_t					   m_ipCode;
    int32_t                    m_iRoundCount;
    int32_t                    m_iHhmfwNum[5];              //300局黑红梅方王的统计值

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
