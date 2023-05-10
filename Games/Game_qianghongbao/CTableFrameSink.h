#ifndef TABLE_FRAME_SINK_HEAD_FILE
#define TABLE_FRAME_SINK_HEAD_FILE

#define GAME_PLAYER						50

#define ERROESCORE          300000000            //红包游戏最大红包为六万超过十万就是产生了异常
enum enErrorCode
{
    ERROR_WRONG_STATUS      = 1,				//状态错误
    ERROR_NO_EVELOPE		= 2,			    //红包已抢完
    ERROR_NOT_ENOUGH_SOCRE	= 3,				//金币不足
    ERROR_ALREADY_GRAB      = 4,				//重复抢红包
    ERROR_NOT_BANKER        = 5,				//非庄家不能发红吧
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
        m_EndWinScore = 0;
        m_Revenue = 0;
        NotBetJettonCount = 0;
        isBanker = false;
        haveGrab = false;
        m_UserAllWin = 0;
        m_PlayCount = 0;
        m_UserScore = 0;
        myRanking = 0;
        bestCount = 0;
        m_SendLeftScore = 0;
    }

    void clear()
    {

        m_EndWinScore = 0;
        m_Revenue = 0;        
        haveGrab = false;
        m_Revenue = 0;
        m_AndroidGrabTime = 0.0;
        m_IsUseAndroid = false;
        m_SendLeftScore = 0;
        m_UserScore = 0;
        myRanking = 0;
    }

public:
    int64_t     userId;	//玩家ID.
    uint32_t    chairId;
    uint8_t     headerId;	//玩家头像.
    string      nickName;	//玩家昵称.
    string      location;

    bool        haveGrab;   //是否抢了红包
    bool        isBanker;   //是否是庄家
    int64_t     m_EndWinScore;      // 前端显示输赢
    int64_t     m_Revenue;
    int32_t     NotBetJettonCount;  // 连续不下注的局数

    double      m_AndroidGrabTime;  //机器人下注时间
    bool        m_IsUseAndroid;     // 需要抢的机器人
    int64_t     m_UserAllWin;       //玩家所有赢分
    int32_t     m_PlayCount;
    int64_t     m_SendLeftScore;    //玩家上次发红包剩余的钱
    int64_t     m_UserScore;        //玩家积分
    int32_t     myRanking;          //排名
    int32_t     bestCount;          //最佳手气次数
};
struct randomTim
{
    randomTim(double mx,double my)
    {
        x=mx;
        y=my;
    }
    double x;
    double y;
};
struct  vecHongBao
{
     vecHongBao()
     {
         money = 0;
         isBest = false;
     }
     int64_t money;
     bool isBest;
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
    //设置路径
    bool  OnMakeDir();



public:
    void OnTimerReady();
    void OnTimerStart();
    void OnTimerEnd();
    //结算
    void Settlement();

    //控制机器人下注
    void SetAndroidGrab();
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

    void SendScenes(uint32_t chairId);
    void SendReady(int32_t chairId);
    void SendStart(int32_t chairId);
    void SendEnd(int32_t chairId);
    bool AnswerPlayersGrab(int32_t chairId,shared_ptr<IServerUserItem> player);
    void SendPlayerList(int32_t chairId);


    void ChangeVipUserInfo();
    //保存玩家结算结果
    void WriteUserScore();

    void GetResultArr(int32_t arrsize,std::vector<vecHongBao> &vec,int64_t money,int64_t bestluck);
    void clearTableUser();
    void SendWinSysMessage();
    //连续5局未下注踢出房间
    void CheckKickOutUser();
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo);
    //发送
    bool SendErrorCode(int32_t wChairID, enErrorCode enCodeNum);
protected:
    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    uint32_t                         m_IsEnableOpenlog;                 //is enable open log.
    int64_t                          m_nReadyTime;                      //下注时间 All Jetton Time
    int64_t                          m_nStartTime;                      //1S
    int64_t                          m_nGameEndTime;				    //结束时间 All End Time

    int64_t                          m_nBankerUserId;                    //庄家id
    int32_t                          m_nLeftEnvelopeNum;                 //剩余红包数
    int64_t                          m_SendEnvelopeScore;                //每次发红包得总钱数
    int32_t                          m_SendEnvelopeNum;                  //每次发送红包最大个数
    int64_t                          m_BestLuckyScore;                   //最佳手气的钱数
    int64_t                          m_BankerLeftSocre;                  //本次没有抢完的红包钱数

    std::vector<vecHongBao>          m_VecEnvelop;                      //红包容器

protected:

protected:
    string                          m_strRoundId;
    chrono::system_clock::time_point  m_startTime;                      //
    chrono::system_clock::time_point  m_endTime;

private:
    tagGameRoomInfo *               m_pGameRoomInfo;
    tagGameReplay                   m_replay;//game replay
    uint32_t                        m_dwReadyTime;

private:
    CGameLogic                      m_GameLogic;						//游戏逻辑
    shared_ptr<ITableFrame>         m_pITableFrame;						//框架接口
    double                          m_dControlkill;

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
private:


    muduo::net::TimerId                         m_TimerReadyId;
    muduo::net::TimerId                         m_TimerStartId;
    muduo::net::TimerId                         m_TimerEndId;

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
    static bool Compare(const shared_ptr<UserInfo> p1,  const shared_ptr<UserInfo> p2 )
    {
        return p1->m_UserScore > p2->m_UserScore;
    }
};

#endif
