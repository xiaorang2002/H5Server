#ifndef SGJ_GAMEPROCESS_H
#define SGJ_GAMEPROCESS_H

#define USER_HISTORY_COUNT 20 //玩家游戏记录
#define HISTORY_COUNT 70      //历史记录

class CTableFrameSink : public ITableFrameSink
{ 

public:
    CTableFrameSink();
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

    virtual bool SetTableFrame(shared_ptr<ITableFrame> &pTableFrame);
    virtual bool SendErrorCode(int32_t wChairID, enErrorCode enCodeNum);
    virtual int BuildRoundScene(const int32_t wChairID, int64_t lScore, const bool IsFreeScene);


    bool CheckRule(CMD_S_RoundResult_Tag &SceneOneEnd, bool IsFreeScene);
    bool CalculateScore(CMD_S_RoundResult_Tag &SceneOneEnd, int64_t SingleLine);
    bool FillTable(CMD_S_RoundResult_Tag &SceneOneEnd,bool noFootball,bool isFree=false);

    void FreeControl(bool &isFreeBous,int32_t table[]);
    UserProbabilityItem ScoreGetProbability(int64_t StoreScore,bool isFree=false);//, UserProbabilityItem &UserPro);
    bool MarrySceneWriteScore(const int32_t wChairID, const int64_t lWinScore, int64_t &llGameTax,string cardValueStr);
    bool RoundSceneWriteScore(const int32_t wChairID, const bool IsFreeScene, const int64_t lBetScore, const int64_t lWinScore, int64_t &llGameTax,string cardValuesStr);
    void WriteFreeGameRecord(int32_t wChairID,int64_t betInfo,int32_t freeLeft,int32_t marryLeft,int32_t allMarry);

    //复位桌子
    virtual void RepositionSink();

    void ResetTodayStorage();

    void LeftReward(int64_t userId,uint32_t wChairID);
    void checkKickOut();
    bool isAcrossTheDay(uint32_t curTime,uint32_t oldTime);

    void OnTimerClearUser();
    void CheckKickAndroid();
private:
    //初始化游戏数据
    void InitGameData();
    void ClearGameData(int chairId );
    bool CheckStorageVal(int64_t winScore);
    void Readfile();
    void openLog(const char *p, ...);

    inline int64_t GetTotalStock(){
       return m_pGameRoomInfo->totalStock;
    }
    inline int64_t GetLowerStock(){
       return m_pGameRoomInfo->totalStockLowerLimit;
    }
     inline int64_t GetHighStock(){
       return m_pGameRoomInfo->totalStockHighLimit;
    }
private:
    //游戏信息
    uint8_t m_wGameStatus;               //游戏状态
    uint8_t m_cbPlayStatus[GAME_PLAYER]; //游戏中玩家,下注了的玩家
    //历史记录
    uint8_t m_cbHistory[HISTORY_COUNT]; //历史记录(1 表示黑赢 2 表示红赢)
    uint8_t m_cbHistoryCount;           //历史记录局数
    vector<int> m_cbJettonMultiple;     //下注倍数
    int32_t m_dwCellScore;              //基础积分

private:
    //玩家信息
    int64_t m_llUserSourceScore[GAME_PLAYER]; //用户原始积分


	STD::Weight m_FruitWeight, m_MarryWeight;

    int32_t m_FreeRound;
    int32_t m_MarryRound;
    int32_t m_Round;
    int32_t t_table[15];
    int32_t t_light[15];
    int32_t t_line[9];

    time_t m_tLastOperate[GAME_PLAYER];    //上次操作时间
    int32_t m_nMarrayNumLeft[GAME_PLAYER]; //剩余小玛丽数量
    int32_t m_nMarryNum[GAME_PLAYER];      //总共玩多少次小玛丽
    int32_t m_nFreeRoundLeft[GAME_PLAYER]; //剩余免费旋转
    bool m_bIsSpecilUser[GAME_PLAYER];     //强控玩家

    int64_t m_lFreeRoundWinScore[GAME_PLAYER]; //免费旋转中的赢分
    int64_t m_lMarryTotalWin[GAME_PLAYER];     //玛丽的总赢分
    int64_t m_lUserBetScore[GAME_PLAYER];      //玩家下注金额
    int64_t m_lUserWinScore[GAME_PLAYER];      //玩家旋转场景赢分

    int64_t m_lFreeWinScore; //免费旋转中的赢分
    int64_t m_lMarryWinScore; //免费旋转中的赢分
    //牌型
    string cardValueStr;
    string iconResStr;
    string tmpWinLinesInfo ; 
    //理牌开始时间
	uint32_t groupStartTime;
    uint32_t m_nPullInterval;

    int32_t m_TimerTestCount;

    int64_t m_lUserScore;      //玩家旋转场景赢分
    int64_t m_lastJackpotScore;      //上次发送彩金分
    int64_t m_newJackPot      = 0;
 
    bool m_isAndroid = false;      //是否机器人

    ConfigManager * cfgManager;
    tagAllConfig m_AllConfig;
protected:
    string mGameIds;
	//本局开始时间/本局结束时间
    chrono::system_clock::time_point m_startTime; //
    chrono::system_clock::time_point m_endTime; 
    
    //用户指针
    shared_ptr<CServerUserItem> pUserItem;

private:
    tagGameRoomInfo *m_pGameRoomInfo;
    tagGameReplay m_replay; //game replay
    uint32_t m_dwReadyTime;
    //记录
    tagSgjFreeGameRecord m_SgjFreeGameRecord;
private:
    GameLogic m_GameLogic;                  //游戏逻辑
    shared_ptr<ITableFrame> m_pITableFrame; //框架接口
    shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;

    muduo::net::TimerId                         m_TimerIdTest;
    muduo::net::TimerId                         m_TimerIdKickOut;
    muduo::net::TimerId                         m_TimerIdJackpot;
private:                                  //Storage Related
    static int64_t m_llStorageControl;    //库存控制
    static int64_t m_llStorageLowerLimit; //库存控制下限值
    static int64_t m_llStorageHighLimit;  //库存控制上限值
    static int64_t m_llTodayStorage;      //今日库存
    static int64_t m_llStorageAverLine;   //average line
    static int32_t m_nAndroidCount;               //refer gap:for couting rate
    int32_t m_wSystemAllKillRatio;        //系统必杀概率
    int32_t m_wChangeCardRatio;           //系统改牌概率
    int32_t m_dwTodayTime;                //今日时间
    //    CMD_S_GameEnd				m_GameEnd;
    int32_t m_MinEnterScore;                     //税收
    int32_t m_kickTime;                     //踢出计时
    int32_t m_kickOutTime;                  //踢出时间
    int64_t m_userId;                   //用户Id
    int32_t m_ChairId;                   //用户Id
    
    bool m_bUserIsLeft;                   //用户是否离开

    string m_JackPotRecMsg = "";             //上次查询的m_JackPotRecMsg          
    int64_t m_lastTick = 0;
    double  stockWeak; 

    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制

    int32_t   footBallAddNum[GAME_PLAYER] ; //足球累计数量
    int32_t   lastFootBallNum[3][5]; // 上次免费开奖的时候屏幕中的足球

    bool mustNotOpenFootball ;
};

#endif // GAMEPROCESS_H
