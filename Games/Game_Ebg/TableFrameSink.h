#pragma once



#define SCENE_NOPLAYER  1000

#define GAME_SCENESTATE_STARTGAME       1
#define GAME_SCENESTATE_JETTON      2
#define GAME_SCENESTATE_GAMEEND       3


#define GAME_MaxMulti      2

#define GAME_MAXAERA       3

#define EBG_MAX_NUM       10000
using namespace std;
const int miTongZI[40]=
{
    1,2,3,4,5,6,7,8,9,10,	//tongzi A - K
    1,2,3,4,5,6,7,8,9,10,	//tongzi A - K
    1,2,3,4,5,6,7,8,9,10,	//tongzi A - K
    1,2,3,4,5,6,7,8,9,10,	//tongzi A - K
};
enum ERR{
    SCORE_FILURE,
    EXCEED_AREA,
    EXCEED_AREA_SCORE,
    SHORT_SCORE,
    USER_FAILURE
};
class  TableFrameSink : public ITableFrameSink
{

public:
    static int  RandomInt(int a,int b)
    {
        return rand() %(b-a+1)+ a;
    }

    struct BestPlayerList
    {
        BestPlayerList()
        {
            NikenName="";
            WinScore=0;
            onTime="";
            bestID=0;
        }
        string NikenName;
        int64_t WinScore;
        string onTime;
        int    bestID;
    };
    struct strPlalyers
    {
        strPlalyers()
        {
            ListWinOrLose.clear();
            ListAllJetton.clear();
            dCurrentPlayerJetton=0.0;
            for(int i=0;i<GAME_MAXAERA;i++)
            {
                dAreaJetton[i]=0.0;
                dLastJetton[i]=0.0;
                dwinscore[i]=0;
				dRealwinscore[i] = 0;
            }

            wCharid=-1;
            wUserid=-1;
            Isleave=false;
            ShuiHouWin=0.0;
            usershuishou=0.0;
            ReturnMoney=0.0;
            bApplyBanker=false;
            NotBetJettonCount = 0;
        }
        void clear()
        {
            dCurrentPlayerJetton=0.0;
            for(int i=0;i<GAME_MAXAERA;i++)
            {
                dAreaJetton[i]=0.0;
                iMulticle[i]=-1;
                dwinscore[i]=0;
				dRealwinscore[i] = 0;
            }
            ShuiHouWin=0.0;
            if(Isleave)
            {
                Isleave=false;
            }
            usershuishou=0.0;
            ReturnMoney=0.0;
        }
        void Culculate(shared_ptr<ITableFrame>  mPItableFrame,int64_t& bankerWinLost,tagGameReplay& replay,bool isAndroid)
        {
            int64_t res=0.0;
            ShuiHouWin=0.0;
            usershuishou=0.0;
            for(int i=0;i<GAME_MAXAERA;i++)
            {
                if(iMulticle[i]>0)
                {
                   int64_t shuishou=0.0;
                   shuishou=mPItableFrame->CalculateRevenue(dAreaJetton[i]*(iMulticle[i]));
                   usershuishou+=shuishou;
                   res =dAreaJetton[i]*(iMulticle[i])-shuishou;
                   ShuiHouWin+=res;

                    ReturnMoney+=dAreaJetton[i]*(iMulticle[i]+1)-shuishou;
                    dwinscore[i]=dAreaJetton[i]*iMulticle[i];
                    bankerWinLost-=dwinscore[i];
                    if(dAreaJetton[i] != 0 && !isAndroid)
                        replay.addResult(wCharid,i+1,dAreaJetton[i],dAreaJetton[i]*iMulticle[i]-shuishou,"",false);
                }
                else
                {
                    res =dAreaJetton[i]*iMulticle[i];
                    ShuiHouWin+=res;
                    dwinscore[i]=0;
                    bankerWinLost+=dAreaJetton[i]*iMulticle[i];
                    if(dAreaJetton[i] != 0 && !isAndroid)
                        replay.addResult(wCharid,i+1,dAreaJetton[i],-dAreaJetton[i],"",false);
                }
				dRealwinscore[i] = res;

            }

            //recode last twenty games winlost 1/0
            if(ShuiHouWin>0)
            {
                if((ListWinOrLose.size())<20)
                {
                    ListWinOrLose.push_back(1);
                }
                else {
                    ListWinOrLose.erase(ListWinOrLose.begin());
                    ListWinOrLose.push_back(1);
                }
            }
            else
            {
                if((ListWinOrLose.size())<20)
                {
                    ListWinOrLose.push_back(0);
                }
                else {
                    ListWinOrLose.erase(ListWinOrLose.begin());
                    ListWinOrLose.push_back(0);
                }
            }
            //recode last twenty games all jetton score
            if((ListAllJetton.size())<20)
            {
                ListAllJetton.push_back(dCurrentPlayerJetton);
            }
            else {
                ListAllJetton.erase(ListAllJetton.begin());
                ListAllJetton.push_back(dCurrentPlayerJetton);
            }

        }

        void SetPlayerMuticl(int index,int mutical)
        {
            if(index<0||index>2)
            {
                LOG(ERROR)<<"No Resulet form Algorithmc";
                return;
            }
            iMulticle[index]=mutical;
        }
        int64_t GetTwentyWin()
        {
            if(ListWinOrLose.size()<=0)
            {
                return 0;
            }
            iTwentyWinCount=0;
            for(vector<int64_t>::iterator it=ListWinOrLose.begin();it!=ListWinOrLose.end();it++)
            {
                if((*it)>0) iTwentyWinCount+=1;
            }
            return iTwentyWinCount;
        }
        int64_t GetTwentyJetton()
        {
            if(ListAllJetton.size()<=0)
            {
                return 0;
            }
            dTwentyAllJetton=0;
            for(vector<int64_t>::iterator it=ListAllJetton.begin();it!=ListAllJetton.end();it++)
            {
                dTwentyAllJetton+=(*it);
            }
            return dTwentyAllJetton;
        }
        vector<int64_t> ListWinOrLose;
        vector<int64_t> ListAllJetton;

        int64_t  dCurrentPlayerJetton;
        int64_t  dAreaJetton[GAME_MAXAERA];
        int64_t  dLastJetton[GAME_MAXAERA];
        int     iMulticle[GAME_MAXAERA];
        int64_t  dAreaOut[GAME_MAXAERA];
        int64_t  dwinscore[GAME_MAXAERA];
		int64_t  dRealwinscore[GAME_MAXAERA];
        int64_t  ShuiHouWin;
        int64_t  usershuishou;
        int64_t  ReturnMoney;
        int     wCharid;
        int64_t     wUserid;
        bool    Isleave;
        bool    bApplyBanker;
        int32_t NotBetJettonCount; // 连续不下注的局数
    private:
        int64_t  dTwentyAllJetton;
        int     iTwentyWinCount;
    };

    struct JettonInfo {
        bool bJetton =false;
        int  iAreaJetton[GAME_MAXAERA]={0};
    };

    static  bool Comparingconditions(shared_ptr<strPlalyers> a,shared_ptr<strPlalyers> b)
    {
           if(a->GetTwentyWin()>b->GetTwentyWin())
           {
               return true;
           }
           else if(a->GetTwentyWin()==b->GetTwentyWin()
                   &&a->GetTwentyJetton()>b->GetTwentyJetton())
           {
                return true;
           }
           return false;
    }
    static  bool Comparingconditions1(shared_ptr<strPlalyers> a,shared_ptr<strPlalyers> b)
    {

           if(a->GetTwentyJetton()>b->GetTwentyJetton())
           {
               return true;
           }
           else if(a->GetTwentyWin()>b->GetTwentyWin()
                   &&a->GetTwentyJetton()==b->GetTwentyJetton())
           {
                return true;
           }
           return false;
    }
    TableFrameSink();
    virtual ~TableFrameSink();

public://game.
    virtual bool OnEventGameConclude(uint32_t chairid, uint8_t nEndTag) override {return false;}
    //virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag) = 0;


    virtual void OnGameStart()  ;
    virtual void OnEventGameEnd(  int64_t chairid, uint8_t nEndTag) ;
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon) override;


public://users.
    virtual bool OnUserEnter(int64_t userid, bool islookon) override;
    virtual bool OnUserReady(int64_t userid, bool islookon) override;
    virtual bool OnUserLeft( int64_t userid, bool islookon) override;

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser) override;
    virtual bool CanLeftTable(int64_t userId) override;

public://events.
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize) override;

public: // setting.
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) override;
    virtual void RepositionSink()override;

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
    void GamefirstInGame();
    void NormalCalculate();
    void NormalGameStart();
    void NormalGameJetton();

    bool GamePlayerJetton(int64_t chairid, uint8_t cbJettonArea, int64_t lJettonScore, bool bReJetton = false);
    void AddPlayerJetton(int64_t chairid,int areaid,int score);
    //申请上庄
    void GamePlayerApplyBanker(int64_t chairid);
    void GamePlayerCancelBanker(int64_t chairid);
    //
    void SendNotifyToChair(int64_t chairid,string& message);
    //更新庄家信息
    void UpdateBankerInfo2Client(int64_t chairid=INVALID_CHAIR);

    void SendGameSceneStart(int64_t staID,bool lookon);
    void SendGameSceneJetton(int64_t staID,bool lookon);
    void SendGameSceneEnd(int64_t staID,bool lookon);

    bool isInRichList(int64_t chairid);
    void PlayerRichSorting();
    int PlayerJettonPanDuan(int index,int64_t score,int64_t chairid,shared_ptr<CServerUserItem>& play);

    void PlayerCalculate();

    void ResetTableDate();

    void SendPlayerList(int64_t chairid, int64_t subid,const char*  data,int64_t datasize);


    void Rejetton(int64_t chairid, int64_t subid,const char*  data,int64_t datasize);
    void FillContainer();
    void writeRecored();
    void readRecored();

    void recoredResult();
    void ArgorithResult();
    void BulletinBoard();
    void GamePlayerQueryPlayerList(int64_t chairid, uint8_t subid, const char* data, int64_t datasize,bool IsEnd=false);

    void ReadInI();
    void ReadCardConfig();

    void GameTimerStart();
    void GameTimerJetton();
    void GameTimerEnd();
    void GameTimerJettonBroadcast();
    void clearTableUser();
    void  openLog(const char *p, ...);
    //连续5局未下注踢出房间
    void CheckKickOutUser();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<strPlalyers> userInfo);
	
	void updateGameMsg(bool isInit);
	//获取游戏状态对应的剩余时间
	int32_t getGameCountTime(int gameStatus);
	//本局游戏结束刷新玩家排序的时间
	void RefreshPlayerList(bool isGameEnd = true);

private:
    shared_ptr<ITableFrame>  mPItableFrame;
    BetArea * mpGameBetArea;
    EbgAlgorithm   erbaganAlgorithm;
    BankeManager * mpBankerManager;

    CardLogic * mpCardLogic;
    int curBankerChairID;
    int miShenSuanZiID;
    int miShenSuanZiUserID;

    int32_t mfGameTimeLeftTime;
    int32_t mfGameRoundTimes;
    int miGameSceneState;//youxi zhuangtai
    int8_t playerMuiltcle[GAME_MAXAERA];
    int ThisTurnPlaceMulticle[GAME_MAXAERA];
    int64_t AreaMaxJetton[GAME_MAXAERA];
    int miThisTurnMultical;
    int miThisTurnResultIndex;
    int miThisTurnStopIndex;
    int miThisTurnBestPlayerUserID;
    int miThisTurnBestPlayerChairID;
    int64_t miThisTurnBestPlayerWinScore;
    int8_t iopenResult[3];
    long SendTongBu;
    std::string m_ConfigPath;
    bool mbIsOpen;
    map<int64_t,shared_ptr<strPlalyers>> m_pPlayerInfo;
    vector<shared_ptr<strPlalyers>> pPlayerInfoList;
    vector<shared_ptr<BestPlayerList>> vecPlayerList;
    JettonInfo m_arr_jetton[EBG_MAX_NUM];
    int  m_iarr_tmparea_jetton[GAME_MAXAERA]={0};
    JettonInfo tmpJttonList[EBG_MAX_NUM];
    int iarr_tmparea_jetton[GAME_MAXAERA]={0};
    int iWriteUserLength;
    int64_t dPaoMaDengScore;
    std::vector<int> ResultRecored;
    int IsIncludeAndroid;
    int ReadCount;
    int ceshifasong;
    struct  recored
    {
         recored()
         {
             shunmen=0;
             tianmen=0;
             dimen=0;
         }
         int shunmen;
         int tianmen;
         int dimen;
    };
	recored m_thisResult;
    list<recored> LoadRecored;
	list<recored> LoadNewRecored;
    list<uint8_t>   m_listTestCardArray;
    int64_t mgameTimeStart;
    int64_t mgameTimeJetton;
    int64_t mgameTimeEnd;
    float_t mgameTimeJettonBroadcast;
	float m_fUpdateGameMsgTime;			   //更新游戏信息的时间	

    muduo::net::TimerId                         m_TimerIdStart;
    muduo::net::TimerId                         m_TimerIdJetton;
    muduo::net::TimerId                         m_TimerIdEnd;
    muduo::net::TimerId                         m_TimerJettonBroadcast;
	muduo::net::TimerId                         m_TimerIdUpdateGameMsg;
	muduo::net::TimerId							m_TimerIdResetPlayerList;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;
    double    m_dControlkill;//控制杀放的概率值
    int64_t   m_lLimitPoints;//红线阈值
private:
    string                              m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //
    boost::shared_mutex m_mutex;
    bool                               m_bControl;
    uint32_t                           m_applyBankerMaxCount;
    ErBaGang::BankerInfo*              m_bankerInfo;
    int64_t                            m_bankerWinLost;                 //庄家税后输赢分数
    tagGameReplay       m_replay;//game replay
    uint32_t            m_dwReadyTime;
    double              stockWeak;


    int64_t                         m_userScoreLevel[4];
    int64_t                         m_userChips[4][7];
    int64_t                         m_currentchips[EBG_MAX_NUM][5];
    int                             useIntelligentChips;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
	
	bool					   m_bGameEnd;										  //本局是否结束
	int						   m_cbGameStatus;									  //游戏状态
	int32_t          		   m_nGoodRouteType;						          //好路类型
	string				       m_strGoodRouteName;								  //好路名称
	float					   m_fResetPlayerTime;						          //本局游戏结束刷新玩家排序的时间
	int32_t 				   m_winOpenCount[EBG_MAX_NUM];						  //各下注项累计开奖的局数(0:和 1:龙 2:虎)
	int64_t					   m_lLimitScore[2];								  //限红
	string          		   m_strTableId;									  //桌号
	int32_t					   m_ipCode;

};

bool Compare(shared_ptr<CServerUserItem> ait,shared_ptr<CServerUserItem> bit)
{
    if(ait->GetUserScore()>bit->GetUserScore())
    {
        return true;
    }
    else if(ait->GetUserScore()>bit->GetUserScore()&&ait->GetUserId()<bit->GetUserId())
    {
        return true;

    }
    else
    {
        return false;
    }
}
ITableFrameSink *GetTableFrameSinkFromDLL(void);


